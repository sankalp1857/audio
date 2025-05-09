/* Sensory Confidential
 * Copyright (C)2017-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK custom stream header. See wmme-stream.c.
 *------------------------------------------------------------------------------
 */

typedef enum {
  STREAM_LATENCY_LOW,  /* low latency, high CPU overhead          */
  STREAM_LATENCY_HIGH, /* higher latency, with lower CPU overhead */
} StreamLatency;

SnsrStream
streamFromWMME(int devid, unsigned int rate,
               SnsrStreamMode mode, StreamLatency latency);
