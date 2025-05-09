/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK custom stream header. See alsa-stream.c.
 *------------------------------------------------------------------------------
 */

typedef enum {
  STREAM_LATENCY_LOW,  /* low latency, high CPU overhead          */
  STREAM_LATENCY_HIGH, /* higher latency, with lower CPU overhead */
} StreamLatency;

SnsrStream
streamFromALSA(const char *name, unsigned int rate,
               SnsrStreamMode mode, StreamLatency latency);
