/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK keyword spotting minimal example using a custom stream.
 *------------------------------------------------------------------------------
 * SnsrStream ALSA (Linux audio) provider implementation.
 * Currently capture-only.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <alsa/asoundlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "alsa-stream.h"

/* 15 ms at 16 kHz */
#define PERIOD_SIZE_LOW_LATENCY   240
/* 200 ms at 16 kHz */
#define PERIOD_SIZE_HIGH_LATENCY 3200

/* Minimum number of periods the buffer should include */
#define MIN_PERIOD_COUNT   5
/* Buffer size in ms */
#define MIN_BUFFER_MS    500

typedef struct {
  snd_pcm_t *in;
  const char *initErrorMsg;    /* NULL if initialization was successful */
} ProviderData;


/* This wrapper macro is used to simplify ALSA library error checking.
 * Commands are not executed if an error condition exists.
 */
#define AE(cmd)\
  if (snsrStreamRC(b) == SNSR_RC_OK) {\
    int r = snd_pcm_ ## cmd;\
    if (r < 0) {\
      snsrStream_setDetail(b, "ALSA error: %s", snd_strerror(r));\
      snsrStream_setRC(b, SNSR_RC_ERROR);\
    }\
  }


static SnsrRC
streamOpen(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);

  if (!d->in) {
    if (d->initErrorMsg) snsrStream_setDetail(b, "%s", d->initErrorMsg);
    else snsrStream_setDetail(b, "Could not open ALSA device for capture.");
    return SNSR_RC_NOT_FOUND;
  }
  AE( prepare(d->in) );
  return snsrStreamRC(b);
}


static SnsrRC
streamClose(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);

  AE( drop(d->in) );
  return snsrStreamRC(b);
}


static void
streamRelease(SnsrStream b)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);

  AE( close(d->in) );
  free((void *)d->initErrorMsg);
  free(d);
}


static size_t
streamRead(SnsrStream b, void *buffer, size_t size)
{
  ProviderData *d = (ProviderData *)snsrStream_getData(b);
  snd_pcm_uframes_t read, total = 0, want = size / sizeof(short);
  short *sbuff = buffer;

  if (snd_pcm_state(d->in) == SND_PCM_STATE_XRUN) {
    snsrStream_setRC(b, SNSR_RC_BUFFER_OVERRUN);
    return 0;
  }
  do {
    read = snd_pcm_readi(d->in, sbuff + total, want - total);
    if ((int)read < 0) read = snd_pcm_recover(d->in, read, 0);
    if ((int)read < 0) {
      snsrStream_setDetail(b, "ALSA read error: %s", snd_strerror((int)read));
      snsrStream_setRC(b, SNSR_RC_ERROR);
      return 0;
    }
    total += read;
  } while (total < want);
  return total * sizeof(short);
}


static SnsrStream_Vmt ProviderDef = {
  "ALSA",
  &streamOpen, &streamClose, &streamRelease, &streamRead, NULL
};


SnsrStream
streamFromALSA(const char *name, unsigned int rate,
                   SnsrStreamMode mode, StreamLatency latency)
{
  SnsrStream b;
  ProviderData *d = (ProviderData *)malloc(sizeof(*d));
  snd_pcm_t *h = NULL;
  snd_pcm_hw_params_t *p = NULL;
  int dir = 0;
  snd_pcm_uframes_t frames;

  if (!d) return NULL;
  memset(d, 0, sizeof(*d));
  b = snsrStream_alloc(&ProviderDef, d, 1, 0);
  if (!b) {
    free(d);
    return NULL;
  }
  if (mode != SNSR_ST_MODE_READ) {
    snsrStream_setRC(b, SNSR_RC_INVALID_MODE);
    return b;
  }
  AE( open(&h, name, SND_PCM_STREAM_CAPTURE, 0) );
  AE( hw_params_malloc(&p) );
  AE( hw_params_any(h, p) );
  AE( hw_params_set_access(h, p, SND_PCM_ACCESS_RW_INTERLEAVED) );
  AE( hw_params_set_format(h, p, SND_PCM_FORMAT_S16_LE) );
  AE( hw_params_set_channels(h, p, 1) );
  AE( hw_params_set_rate(h, p, (unsigned)rate, 0) );
  switch (latency) {
  case STREAM_LATENCY_LOW:  frames = PERIOD_SIZE_LOW_LATENCY; break;
  case STREAM_LATENCY_HIGH: frames = PERIOD_SIZE_HIGH_LATENCY; break;
  }
  AE( hw_params_set_period_size_near(h, p, &frames, &dir) );
  AE( hw_params_get_period_size(p, &frames, &dir) );
  frames = MIN_PERIOD_COUNT * frames;
  if (frames < MIN_BUFFER_MS * rate / 1000.0 )
    frames *= (int)(MIN_BUFFER_MS * rate / 1000.0 / frames + 0.5);
  AE( hw_params_set_buffer_size_near(h, p, &frames) );
  AE( hw_params(h, p) );
  snd_pcm_hw_params_free(p);
  if (snsrStreamRC(b) == SNSR_RC_OK) d->in = h;
  else if (h) snd_pcm_close(h);
  if (!d->in) d->initErrorMsg = strdup(snsrStreamErrorDetail(b));
  return b;
}
