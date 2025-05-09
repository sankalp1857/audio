/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK keyword spotting minimal example.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <stdlib.h>


/* Result callback function, see snsrSetHandler() in main() below.
 * Print the result text and the start time of the first spotted phrase.
 */
static SnsrRC
resultEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  const char *phrase;
  double begin;

  /* Retrieve the phrase text and start time from the session handle. */
  snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
  r = snsrGetString(s, SNSR_RES_TEXT, &phrase);

  /* Quit early if an error occurred. */
  if (r != SNSR_RC_OK) return r;
  printf("Spotted \"%s\" at %.2f seconds.\n", phrase, begin/1000.0);

  /* Returning a code other than SNSR_RC_OK instructs snsrRun() to return it. */
  return SNSR_RC_STOP;
}


int
main(int argc, char *argv[])
{
  SnsrRC r;
  SnsrSession s;

  if (argc != 2) {
    fprintf(stderr, "usage: %s spotter-model\n", argv[0]);
    exit(1);
  }

  /* Create a new session handle. */
  snsrNew(&s);

  /* Load and validate the spotter model task file. */
  snsrLoad(s, snsrStreamFromFileName(argv[1], "r"));
  snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT);

  /* Create a live audio stream instance and attach it to the session. */
  snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM,
                snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT));

  /* Register a result callback. Private data handle is not used */
  snsrSetHandler(s, SNSR_RESULT_EVENT, snsrCallback(resultEvent, NULL, NULL));

  /* Main recognition loop. The result handler will cause snsrRun() to
   * return SNSR_RC_STOP. Other return codes indicate an unexpected error.
   * Session errors remain until explicitly cleared: Any errors that occured
   * earlier will also be reported here.
   */
  r = snsrRun(s);
  if (r != SNSR_RC_STOP)
    fprintf(stderr, "ERROR: %s\n", snsrErrorDetail(s));

  /* Release the session. This will also release the model and audio streams,
   * and the callback handler. No other references to these handles are held,
   * so their memory will be reclaimed.
   */
  snsrRelease(s);
  return r;
}
