/* Sensory Confidential
 * Copyright (C)2018-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK example illustrating the use of the
 * sample custom data-stream (see data-stream.c)  This should
 * be easily adaptable to a custom live-audio stream
 * (in case of a custom audio driver for RTOS for example.)
 *
 * The spotter model is loaded from code space. On platforms where code is
 * read directly from ROM, this will reduce heap requirements.
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include <snsr.h>

#include "data-stream.h"


/* See spot-hbg-enUS-1.4.0-m.c */
extern SnsrCodeModel spot_hbg_enUS_1_3_0_m;

/* NOTE: extern char * foo is NOT always the same as extern char foo[] */
extern unsigned char audioData[];
extern unsigned int audioDataLen;


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
  printf("\nSpotted \"%s\" from sample %d to sample %d\n",
         phrase, (int)begin, (int)end);
  /* This return code from the event handler sets the
   * return code in the SnsrSession and causes the session
   * to stop
   */
  return SNSR_RC_STOP;
}


int
main(int argc, char **argv)
{
  SnsrRC rc;
  SnsrSession s = NULL;
  SnsrStream audioStream = NULL;

  rc = snsrNew(&s);
  if (rc != SNSR_RC_OK) {
    const char *err = s ? snsrErrorDetail(s) : snsrRCMessage(rc);
    fprintf(stderr, "Error on init: %d - %s\n", rc, err);
    return rc;
  }

  /* Load and validate the spotter model task from code space */
  snsrLoad(s, snsrStreamFromCode(spot_hbg_enUS_1_3_0_m));
  if (snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT) != SNSR_RC_OK) {
    fprintf(stderr, "Error loading spotter: %s\n", snsrErrorDetail(s));
    return rc;
  }

  /* Register a result callback. Private data handle is not used. */
  snsrSetHandler(s, SNSR_RESULT_EVENT, snsrCallback(resultEvent, NULL, NULL));

  /* NOTE: Audio stream should be 16 KHz, 16 bits/sample, mono */

  /* NOTE: Directly casting char to short works on little-endian only */
  /* ARM and x86 are little-endian, MIPS may not be */
  audioStream = streamFromData(audioData, audioDataLen, SNSR_ST_MODE_READ);
  snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, audioStream);

  /* snsrRun won't return until stopped or interrupted or end of data */
  rc = snsrRun(s);

  switch (rc) {
    case SNSR_RC_OK:
      printf("Done, no error but no phrase.\n");
      break;
    case SNSR_RC_STOP:
      printf("Done, found phrase.\n");
      break;
    case SNSR_RC_STREAM_END:
      printf("Reached end of stream.\n");
      break;
    default:
      printf("Unexpected return: %d\n", rc);
      return rc;
  }
  printf("\n");
  if (s) snsrRelease(s);
  /* audioStream has already been released because session had
   * the only reference to it - so don't snsrRelease it again.
   */

  return 0;
}
