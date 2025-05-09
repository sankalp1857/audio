/* Sensory Confidential
 * Copyright (C)2019-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK custom stream header. See aqs-stream.c.
 *------------------------------------------------------------------------------
 */

typedef enum {
  STREAM_LATENCY_LOW,  /* low latency, high CPU overhead          */
  STREAM_LATENCY_HIGH, /* higher latency, with lower CPU overhead */
} StreamLatency;

SnsrStream
streamFromAQS(const char *name, unsigned int rate,
              SnsrStreamMode mode, StreamLatency latency);
