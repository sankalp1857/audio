/* Sensory Confidential
 * Copyright (C)2018-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK example illustrating the simplest way
 * to get data from whatever source (custom RTOS audio driver perhaps)
 * and push the data into the input stream and get results from the session.
 *
 * Illustrates the use of a custom memory allocator to avoid runtime
 * allocations from the system heap, and use of a panic function to
 * recover from otherwise fatal memory errors.
 *
 * Similar to sample push-audio.c but even simpler and does not use
 * a filesystem.
 *
 * The spotter model is loaded from code space. On platforms where code is
 * read directly from ROM, this will reduce heap requirements.
 *-----------------------------------------------------------------------------
 */

#include <setjmp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <snsr.h>


/* See spot-hbg-enUS-1.4.0-m.c */
extern SnsrCodeModel spot_hbg_enUS_1_3_0_m;

/* See data.c */
extern unsigned char audioData[];
extern unsigned int  audioDataLen;

/* Process one 15 ms audio block at a time.
 * Most spotters run at a 15 ms frame rate. Matching the processing block
 * size to the frame rate reduces live audio processing overhead to a minimum.
 * Larger blocks will result in fewer calls to snsrPush(), but add latency.
 * Smaller blocks are accumulated in snsrPush() until at least a frame's worth
 * is available.
 */
#define BLOCK_MS       15
#define SAMPLE_RATE 16000
#define BLOCK_BYTES (BLOCK_MS * SAMPLE_RATE / 1000 * sizeof(short))

/* Heap backing store, see snsrConfig(SNSR_CONFIG_ALLOC, ...) call in main.
 *
 * Set HEAP_SIZE to 100000 to trigger an out-of-memory panic and and
 * subsequent recovery.
 */
#define HEAP_SIZE 200000
static size_t HeapPool[HEAP_SIZE / sizeof(size_t)];

/* Utility, returns the lesser of a and b */
#define MIN(a, b) ((a) < (b)? (a): (b))


/* Result callback function, see snsrSetHandler() below.
 * Print the result text and the start and end sample indices of
 * the first spotted phrase.
 */
static SnsrRC
resultEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC rc;
  const char *phrase;
  double begin, end;

  /* Retrieve the phrase text and alignments from the session handle */
  snsrGetDouble(s, SNSR_RES_BEGIN_SAMPLE, &begin);
  snsrGetDouble(s, SNSR_RES_END_SAMPLE, &end);
  rc = snsrGetString(s, SNSR_RES_TEXT, &phrase);
  /* Quit early if an error occurred. */
  if (rc != SNSR_RC_OK) return rc;
  printf("\nSpotted \"%s\" from sample %d to sample %d\n", phrase, (int)begin,
         (int)end);
  /* This return code from the event handler sets the
   * return code in the SnsrSession.
   */
  return SNSR_RC_STOP;
}


/* Saved calling environment used by panicFunc() below */
static jmp_buf PanicJmp;

/* This function handles unrecoverable errors. Here it logs a message
 * to the console, then does a longjmp to the very start of the application
 * where a recovery attempt is made.
 */
static void
panicFunc(const char *format, va_list a)
{
  fprintf(stderr, "\nPANIC: ");
  vfprintf(stderr, format, a);
  fprintf(stderr, "\n\n");
  longjmp(PanicJmp, SNSR_RC_NO_MEMORY);
}


int
main(int argc, char **argv)
{
  SnsrRC rc;
  SnsrSession s = NULL;
  int jmp;
  unsigned i;

  /* Register a custom panic handler */
  snsrConfig(SNSR_CONFIG_PANIC_FUNC, panicFunc);

  if ((jmp = setjmp(PanicJmp))) {
    /* Out-of-memory error occurred. Abandon heap, re-initialize */
    snsrTearDown();
    fprintf(stderr, "Restarting application with default allocator.\n");

  } else {
    /* Use a custom allocator to avoid calls to malloc(), et al */
    rc = snsrConfig(SNSR_CONFIG_ALLOC,
                    snsrAllocTLSF(HeapPool, sizeof(HeapPool)));
    if (rc != SNSR_RC_OK) {
      fprintf(stderr, "Custom allocation failure: %s\n", snsrRCMessage(rc));
      return rc;
    }
  }

  rc = snsrNew(&s);
  if (rc != SNSR_RC_OK) {
    fprintf(stderr, "ERROR: %s\n", s? snsrErrorDetail(s) : snsrRCMessage(rc));
    return rc;
  }

  /* Load and validate the spotter model task from code space */
  snsrLoad(s, snsrStreamFromCode(spot_hbg_enUS_1_3_0_m));
  if (snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT) != SNSR_RC_OK) {
    fprintf(stderr, "ERROR: %s\n", snsrErrorDetail(s));
    return rc;
  }

  /* Register a result callback. Private data handle is not used. */
  snsrSetHandler(s, SNSR_RESULT_EVENT, snsrCallback(resultEvent, NULL, NULL));

  /* Main loop. Push all audio data through the recognizer pipeline. */
  for (i = 0; i < audioDataLen; i += BLOCK_BYTES) {
    rc = snsrPush(s, SNSR_SOURCE_AUDIO_PCM, audioData + i,
                  MIN(BLOCK_BYTES, audioDataLen - i));
    if (rc == SNSR_RC_STOP) {
      printf("Phrase spotted.\n");
      fflush(stdout);
      snsrClearRC(s);
    } else if (rc != SNSR_RC_OK) {
      fprintf(stderr, "ERROR: %s\n", snsrErrorDetail(s));
      return rc;
    }
  }

  /* Flush any remaining internally-buffered audio */
  rc = snsrStop(s);

  snsrRelease(s);

  snsrTearDown();
  return rc;
}
