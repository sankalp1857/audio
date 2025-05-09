/* Sensory Confidential
 * Copyright (C)2018-2025 Sensory, Inc. https://sensory.com/
 *
 *------------------------------------------------------------------------------
 * SnsrStream provider for Audio Queue Services on Darwin.
 * Currently capture-only.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <assert.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/AudioQueue.h>


#include "aqs-stream.h"

/* Initial size of the circular capture buffer. 500 ms at 16kHz */
#define CAPTURE_MINSIZE  16000
/* Maximum size of the circular capture buffer.  10 s  at 16kHz */
#define CAPTURE_MAXSIZE 320000

#define PERIOD_LOW_LATENCY  0.015
#define PERIOD_HIGH_LATENCY 0.200
#define BUFFER_SIZE_SECONDS 2.0

/* 10 ms at 16 kHz */
#define MIN_BUFFER_SIZE     320
/* 5 s at 16 kHz */
#define MAX_BUFFER_SIZE  160000
#define MIN_BUFFER_COUNT    4

typedef struct {
  SnsrStream           capture;       /* Captured audio buffer                */
  const char          *initErrorMsg;  /* NULL if initialization ok            */
  AudioStreamBasicDescription dataFormat;  /* recording format description    */
  AudioQueueRef        queue;              /* OS-level recording queue        */
  AudioQueueBufferRef *buffer;             /* queue buffers                   */
  pthread_mutex_t      lock;               /* protects capture stream         */
  pthread_cond_t       notEmpty;           /* signals capture stream          */
  size_t               bufferCount;        /* number of buffers in *buffer    */
  UInt32               bufferByteSize;     /* size of one queue buffer        */
  unsigned             isRunning:1;        /* 0 to wind down recording        */
} ProviderData;


static SnsrRC
ossErrorMessage(SnsrStream b, OSStatus s)
{
  const char *msg;
  switch (s) {
  case kAudioServicesNoError:
    msg = NULL;
  case kAudioServicesUnsupportedPropertyError:
    msg = "The property is not supported.";
    break;
  case kAudioServicesBadPropertySizeError:
    msg = "The size of the property data was not correct.";
    break;
  case kAudioServicesBadSpecifierSizeError:
    msg = "The size of the specifier data was not correct.";
    break;
  case kAudioServicesSystemSoundClientTimedOutError:
    msg = "System sound client message timed out.";
    break;
  case kAudioServicesSystemSoundUnspecifiedError:
  default:
    snsrStream_setDetail(b, "An unspecified error occurred, code %i.", (int)s);
    return SNSR_RC_ERROR;
  }
  snsrStream_setDetail(b, "%s", msg);
  return SNSR_RC_ERROR;
}


static void
audioCallback(void *privateData,
              AudioQueueRef inAQ,
              AudioQueueBufferRef inBuffer,
              const AudioTimeStamp *inStartTime,
              UInt32 inNumPackets,
              const AudioStreamPacketDescription *inPacketDesc)
{
  ProviderData *d = (ProviderData *)privateData;
  size_t written;

  if (!inNumPackets && d->dataFormat.mBytesPerPacket)
    inNumPackets = inBuffer->mAudioDataByteSize / d->dataFormat.mBytesPerPacket;
  if (inNumPackets) {
    pthread_mutex_lock(&d->lock);
    written = snsrStreamWrite(d->capture, inBuffer->mAudioData,
                              d->dataFormat.mBytesPerPacket, inNumPackets);
    if (written < inNumPackets)
      snsrStream_setRC(d->capture, SNSR_RC_BUFFER_OVERRUN);
    pthread_cond_broadcast(&d->notEmpty);
    pthread_mutex_unlock(&d->lock);
  }
  if (d->isRunning)
    AudioQueueEnqueueBuffer(d->queue, inBuffer, 0, NULL);
}


static OSStatus
audioInit(ProviderData *d, double sampleRate,
          double chunkSizeSeconds, double totalSizeSeconds)
{
  AudioStreamBasicDescription *f = &d->dataFormat;
  OSStatus s;
  int i;

  /* Recording format: 16-bit LPCM at 16 kHz */
  memset(f, 0, sizeof(*f));
  f->mFormatID = kAudioFormatLinearPCM;
  f->mSampleRate = sampleRate;
  f->mChannelsPerFrame = 1;
  f->mBitsPerChannel = 16;
  f->mBytesPerPacket = f->mBytesPerFrame =
    f->mChannelsPerFrame * sizeof(SInt16);
  f->mFramesPerPacket = 1;
  f->mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;

  /* Create recording queue */
  s = AudioQueueNewInput(f, audioCallback, d, NULL,
                         kCFRunLoopCommonModes, 0, &d->queue);
  if (s != kAudioServicesNoError) return s;

  /* Derive appropriate audio queue buffer size */
  d->bufferByteSize =
    (UInt32)(f->mSampleRate * f->mBytesPerPacket * chunkSizeSeconds);
  if (d->bufferByteSize > MAX_BUFFER_SIZE) d->bufferByteSize = MAX_BUFFER_SIZE;
  if (d->bufferByteSize < MIN_BUFFER_SIZE) d->bufferByteSize = MIN_BUFFER_SIZE;
  d->bufferCount = ceil(totalSizeSeconds / chunkSizeSeconds);
  if (d->bufferCount < MIN_BUFFER_COUNT) d->bufferCount = MIN_BUFFER_COUNT;

  /* Allocate audio buffers */
  d->buffer = malloc(sizeof(*d->buffer) * d->bufferCount);
  for (i = 0; i < d->bufferCount && s == kAudioServicesNoError; i++)
    s = AudioQueueAllocateBuffer(d->queue, d->bufferByteSize, d->buffer + i);

  return s;
}


/*------------------------------------------------------------------------------
 */

static SnsrRC
streamOpen(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  OSStatus s;
  int i;

  if (d->initErrorMsg) {
    snsrStream_setDetail(b, "%s", d->initErrorMsg);
    return SNSR_RC_NOT_FOUND;
  }
  snsrStreamOpen(d->capture);
  s = kAudioServicesNoError;
  for (i = 0; i < d->bufferCount && s == kAudioServicesNoError; i++)
    s = AudioQueueEnqueueBuffer(d->queue, d->buffer[i], 0, NULL);
  if (s == kAudioServicesNoError)
    s = AudioQueueStart(d->queue, NULL);
  if (s != kAudioServicesNoError)
    return ossErrorMessage(b, s);
  d->isRunning = 1;
  return snsrStreamRC(b);
}


static SnsrRC
streamClose(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  SnsrRC r;

  d->isRunning = 0;
  AudioQueueStop(d->queue, true);

  /* Flush the capture buffer */
  r = snsrStreamRC(d->capture);
  snsrStreamSkip(d->capture, 1, CAPTURE_MAXSIZE);
  snsrStream_setRC(d->capture, SNSR_RC_OK);
  return r == SNSR_RC_OK? snsrStreamRC(b): r;
}


static void
streamRelease(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);

  snsrRelease(d->capture);
  AudioQueueDispose(d->queue, true);
  pthread_mutex_destroy(&d->lock);
  pthread_cond_destroy(&d->notEmpty);
  free(d->buffer);
  free(d);
}


static size_t
streamRead(SnsrStream b, void *buffer, size_t size)
{
  SnsrRC r;
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  size_t read = 0;

  pthread_mutex_lock(&d->lock);
  if (snsrStreamRC(d->capture) != SNSR_RC_OK) {
    snsrStream_setRC(b, snsrStreamRC(d->capture));
  } else {
    do {
      read += snsrStreamRead(d->capture, (char *)buffer + read, 1, size - read);
      r = snsrStreamRC(d->capture);
    } while ((r == SNSR_RC_OK || r == SNSR_RC_EOF)
             && read < size
             && !pthread_cond_wait(&d->notEmpty, &d->lock));
    if (r != SNSR_RC_OK) {
      snsrStream_setRC(b, r);
      snsrStream_setDetail(b, "%s", snsrStreamErrorDetail(d->capture));
    } else if (read < size) {
      snsrStream_setRC(b, SNSR_RC_EOF);
    }
  }
  pthread_mutex_unlock(&d->lock);
  return read;
}


static SnsrStream_Vmt ProviderDef = {
  "Darwin Audio Queue Services audio capture",
  &streamOpen, &streamClose, &streamRelease, &streamRead, NULL
};


SnsrStream
streamFromAQS(unsigned int rate,
                  SnsrStreamMode mode, StreamLatency latency)
{
  SnsrStream b;
  ProviderData *d = (ProviderData *)malloc(sizeof(*d));
  OSStatus s;
  int r;

  if (!d) return NULL;
  memset(d, 0, sizeof(*d));
  b = snsrStream_alloc(&ProviderDef, d, 1, 0);
  if (!b) {
    free(d);
    return NULL;
  }
  do {
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
    r = pthread_mutex_init(&d->lock, NULL);
    if (!r) r = pthread_cond_init(&d->notEmpty, NULL);
    if (r) {
      snsrStream_setRC(b, SNSR_RC_ERROR);
      snsrStream_setDetail(b, "%s", strerror(r));
      break;
    }

    /* Allocate buffers */
    s = audioInit(d, rate, latency == STREAM_LATENCY_LOW?
                  PERIOD_LOW_LATENCY: PERIOD_HIGH_LATENCY,
                  BUFFER_SIZE_SECONDS);
    if (s != kAudioServicesNoError) {
      snsrStream_setRC(b, ossErrorMessage(b, s));
      break;
    }
  } while (0);

  if (snsrStreamRC(b) != SNSR_RC_OK)
    d->initErrorMsg = strdup(snsrStreamErrorDetail(b));
  return b;
}
