/* Sensory Confidential
 * Copyright (C)2017-2025 Sensory, Inc. https://sensory.com/
 *
 *------------------------------------------------------------------------------
 * SnsrStream provider for Windows Multimedia Extensions Waveform Audio.
 * Currently capture-only.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <windows.h>
#include <mmsystem.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


#include "wmme-stream.h"

/* Initial size of the circular capture buffer. 100 ms at 16kHz */
#define CAPTURE_MINSIZE   3200
/* Maximum size of the circular capture buffer.  10 s  at 16kHz */
#define CAPTURE_MAXSIZE 320000

/*  15 ms at 16 kHz */
#define PERIOD_SIZE_LOW_LATENCY   240
/* 200 ms at 16 kHz */
#define PERIOD_SIZE_HIGH_LATENCY 3200

/* Minimum number of periods the buffer should include */
#define MIN_PERIOD_COUNT   3
/* Buffer size in ms */
#define MIN_BUFFER_MS    300

typedef struct {
  SnsrStream capture;          /* Captured audio buffer                 */
  const char *initErrorMsg;    /* NULL if initialization was successful */
  HWAVEIN in;                  /* Capture handle                        */
  DWORD msgThreadId;           /* Messaging thread ID                   */
  WAVEFORMATEX format;         /* Audio format selector                 */
  WAVEHDR **audioChunk;        /* Audio buffers                         */
  size_t chunks;               /* number of allocated audio buffers     */
  UINT devId;                  /* Capture device ID                     */
  CONDITION_VARIABLE captureNotEmpty;
  CRITICAL_SECTION   captureLock;
} ProviderData;


static void
setAudioError(SnsrStream stream, SnsrRC rc, MMRESULT r, const char *tag)
{
#define ERRMSG_SIZE 512
  char errbuf[ERRMSG_SIZE];
  char *errmsg = errbuf;
  if (waveInGetErrorText(r, errmsg, ERRMSG_SIZE) == MMSYSERR_NOERROR) {
    snsrStream_setDetail(stream, "%s: %s", tag, errmsg);
  } else {
    snsrStream_setDetail(stream, "%s: error code %i", r);
  }
  snsrStream_setRC(stream, rc);
}


static void
setLastError(SnsrStream b, const char *tag)
{
  LPVOID lpMsgBuf;
  DWORD r = GetLastError();
  char *m;

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, r,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf, 0, NULL);
  if (lpMsgBuf) {
    m = (char *)lpMsgBuf;
    m[strlen(m) - 2] = '\0';
    snsrStream_setDetail(b, "%s error: %s", tag, m);
    LocalFree(lpMsgBuf);
  } else {
    snsrStream_setDetail(b, "%s error #%i", tag, (int)r);
  }
  snsrStream_setRC(b, SNSR_RC_ERROR);
}


static void
audioAvailable(HWAVEIN hwi, WAVEHDR *h)
{
  MMRESULT r = waveInUnprepareHeader(hwi, h, sizeof(*h));
  ProviderData *d = (ProviderData *)h->dwUser;
  size_t written;

  EnterCriticalSection(&d->captureLock);

  do {
    if (r != MMSYSERR_NOERROR) {
      setAudioError(d->capture, SNSR_RC_ERROR,
                    r, "waveInUnprepareHeader");
      break;
    }
    if (!d->msgThreadId) break;
    assert(d->format.nChannels == 1);
    written = snsrStreamWrite(d->capture, h->lpData, 1, h->dwBytesRecorded);
    if (written != h->dwBytesRecorded) {
      snsrStream_setRC(d->capture, SNSR_RC_BUFFER_OVERRUN);
      break;
    }
    r = waveInPrepareHeader(hwi, h, sizeof(*h));
    if (r != MMSYSERR_NOERROR) {
      setAudioError(d->capture, SNSR_RC_ERROR, r, "waveInPrepareHeader");
      break;
    }
    r = waveInAddBuffer(hwi, h, sizeof(*h));
    if (r != MMSYSERR_NOERROR) {
      waveInUnprepareHeader(hwi, h, sizeof(*h));
      setAudioError(d->capture, SNSR_RC_ERROR, r, "waveInAddBuffer");
    }
  } while (0);

  LeaveCriticalSection(&d->captureLock);
  WakeConditionVariable(&d->captureNotEmpty);
}


static DWORD WINAPI
threadProc(LPVOID lpParameter)
{
  BOOL r;
  MSG m;

  while ((r = GetMessage(&m, (HWND)-1, 0, 0)) > 0) {
    switch (m.message) {
    case MM_WIM_DATA:
      audioAvailable((HWAVEIN)m.wParam, (WAVEHDR *)m.lParam);
      break;
    case WM_QUIT:
    case MM_WIM_CLOSE:
      return ERROR_SUCCESS;
    }
  }
  if (r < 0) return ERROR_INVALID_HANDLE;
  return ERROR_SUCCESS;
}

/*------------------------------------------------------------------------------
 */

static SnsrRC
streamOpen(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  HANDLE th;
  MMRESULT r;
  size_t i;

  if (d->initErrorMsg) {
    snsrStream_setDetail(b, "%s", d->initErrorMsg);
    return SNSR_RC_NOT_FOUND;
  }
  snsrStreamOpen(d->capture);
  assert(!d->msgThreadId);
  th = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)threadProc,
                    0, 0, &d->msgThreadId);
  if (!th) {
    setLastError(b, "CreateThread");
    return snsrStreamRC(b);
  }
  CloseHandle(th);
  do {
    r = waveInOpen(&d->in, d->devId, &d->format,
                   (DWORD_PTR)d->msgThreadId, 0, CALLBACK_THREAD);
    if (r != MMSYSERR_NOERROR) {
      setAudioError(b, SNSR_RC_NOT_FOUND, r, "waveInOpen");
      break;
    }
    for (i = 0; i < d->chunks; i++) {
      r = waveInPrepareHeader(d->in, d->audioChunk[i],
                              sizeof(*d->audioChunk[i]));
      if (r != MMSYSERR_NOERROR) {
        setAudioError(b, SNSR_RC_ERROR, r, "waveInPrepareHeader");
        break;
      }
      r = waveInAddBuffer(d->in, d->audioChunk[i],
                          sizeof(*d->audioChunk[i]));
      if (r != MMSYSERR_NOERROR) {
        setAudioError(b, SNSR_RC_ERROR, r, "waveInAddBuffer");
        break;
      }
    }
    r = waveInStart(d->in);
    if (r != MMSYSERR_NOERROR)
      setAudioError(b, SNSR_RC_ERROR, r, "waveInStart");
  } while (0);
  if (snsrStreamRC(b) != SNSR_RC_OK) {
    if (d->in) {
      waveInClose(d->in);
      d->in = NULL;
    }
    if (d->msgThreadId) {
      PostThreadMessage(d->msgThreadId, WM_QUIT, 0, 0);
      d->msgThreadId = 0;
    }
  }
  return snsrStreamRC(b);
}


static SnsrRC
streamClose(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  size_t i;

  /* Shut down the messaging thread. */
  PostThreadMessage(d->msgThreadId, WM_QUIT, 0, 0);
  d->msgThreadId = 0;
  /* Unprepare all headers. */
  waveInReset(d->in);
  for (i = 0; i < d->chunks; i++) {
    WAVEHDR *h = d->audioChunk[i];
    while (waveInUnprepareHeader(d->in, h, sizeof(*h)) == WAVERR_STILLPLAYING)
      Sleep(10);
  }
  while (waveInClose(d->in) == WAVERR_STILLPLAYING)
    Sleep(10);
  /* Flush the capture buffer */
  snsrStreamSkip(d->capture, 1, CAPTURE_MAXSIZE);
  snsrStream_setRC(d->capture, SNSR_RC_OK);
  return snsrStreamRC(b);
}


static void
streamRelease(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  size_t i;

  snsrRelease(d->capture);
  if (d->audioChunk) {
    for (i = 0; i < d->chunks; i++) {
      if (d->audioChunk[i]) free(d->audioChunk[i]->lpData);
      free(d->audioChunk[i]);
    }
    free(d->audioChunk);
  }
  free((void *)d->initErrorMsg);
  free(d);
}


static size_t
streamRead(SnsrStream b, void *buffer, size_t size)
{
  SnsrRC r;
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  size_t read = 0;

  EnterCriticalSection(&d->captureLock);
  do {
    read += snsrStreamRead(d->capture, (char *)buffer + read, 1, size - read);
    r = snsrStreamRC(d->capture);
  } while ((r == SNSR_RC_OK || r == SNSR_RC_EOF)
           && read < size
           && SleepConditionVariableCS(&d->captureNotEmpty,
                                       &d->captureLock, INFINITE));
  if (r != SNSR_RC_OK) {
    snsrStream_setRC(b, r);
    snsrStream_setDetail(b, "%s", snsrStreamErrorDetail(d->capture));
  } else if (read < size) {
    snsrStream_setRC(b, SNSR_RC_EOF);
  }
  LeaveCriticalSection(&d->captureLock);
  return read;
}


static SnsrStream_Vmt ProviderDef = {
  "WMME audio capture",
  &streamOpen, &streamClose, &streamRelease, &streamRead, NULL
};


SnsrStream
streamFromWMME(int deviceId, unsigned int rate,
                   SnsrStreamMode mode, StreamLatency latency)
{
  SnsrStream b;
  ProviderData *d = (ProviderData *)malloc(sizeof(*d));
  size_t chunkSize = 0, i;

  if (!d) return NULL;
  memset(d, 0, sizeof(*d));
  b = snsrStream_alloc(&ProviderDef, d, 1, 0);
  if (!b) {
    free(d);
    return NULL;
  }
  do {
    d->devId = deviceId == -1? WAVE_MAPPER: (UINT)deviceId;
    d->capture = snsrStreamFromBuffer(CAPTURE_MINSIZE, CAPTURE_MAXSIZE);
    if (!d->capture) {
      snsrStream_setRC(b, SNSR_RC_NO_MEMORY);
      break;
    }
    snsrRetain(d->capture);
    if (mode != SNSR_ST_MODE_READ) {
      snsrStream_setRC(b, SNSR_RC_INVALID_MODE);
      break;
    }
    /* Signalling and mutexes */
    InitializeCriticalSection(&d->captureLock);
    InitializeConditionVariable(&d->captureNotEmpty);

    /* Prepare capture format description */
    d->format.wFormatTag = WAVE_FORMAT_PCM;
    d->format.wBitsPerSample = 16;
    d->format.nChannels = 1;
    d->format.nSamplesPerSec = rate;
    d->format.nBlockAlign =
      d->format.nChannels * d->format.wBitsPerSample / 8;
    d->format.nAvgBytesPerSec =
      d->format.nBlockAlign * d->format.nSamplesPerSec;
    d->format.cbSize = 0;

    /* Allocate buffers */
    switch (latency) {
    case STREAM_LATENCY_LOW:  chunkSize = PERIOD_SIZE_LOW_LATENCY; break;
    case STREAM_LATENCY_HIGH: chunkSize = PERIOD_SIZE_HIGH_LATENCY; break;
    }
    d->chunks = (int)(MIN_BUFFER_MS * rate / 1000.0 / chunkSize + 0.5);
    if (d->chunks < MIN_PERIOD_COUNT) d->chunks = MIN_PERIOD_COUNT;
    d->audioChunk = malloc(d->chunks * sizeof(*d->audioChunk));
    if (!d->audioChunk) {
      snsrStream_setRC(b, SNSR_RC_NO_MEMORY);
      break;
    }
    memset(d->audioChunk, 0, d->chunks * sizeof(*d->audioChunk));
    for (i = 0; i < d->chunks; i++) {
      d->audioChunk[i] = malloc(sizeof(**d->audioChunk));
      if (!d->audioChunk[i]) {
        snsrStream_setRC(b, SNSR_RC_NO_MEMORY);
        break;
      }
      memset(d->audioChunk[i], 0, sizeof(**d->audioChunk));
      d->audioChunk[i]->dwBufferLength =
        (DWORD)(chunkSize * sizeof(short) * d->format.nChannels);
      d->audioChunk[i]->lpData = malloc(d->audioChunk[i]->dwBufferLength);
      if (!d->audioChunk[i]->lpData) {
        snsrStream_setRC(b, SNSR_RC_NO_MEMORY);
        break;
      }
      d->audioChunk[i]->dwUser = (DWORD_PTR)d;
    }
  } while (0);

  if (snsrStreamRC(b) != SNSR_RC_OK)
    d->initErrorMsg = _strdup(snsrStreamErrorDetail(b));
  return b;
}
