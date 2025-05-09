/* Sensory Confidential
 * Copyright (C)2017-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK keyword spotter, runs trailing audio through
 * a voice activity detector and saves it to file.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <stdlib.h>

/* Set INCLUDE_SPOT to 1 to include the trigger phrase in the audio output */
#define INCLUDE_SPOT    0

/* Output filename */
#define VAD_AUDIO_FILE  "vad-audio.wav"


/* Print an error message and exit.
 */
static void
fatalError(int rc, const char *msg)
{
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(rc);
}


/* Result callback function, see snsrSetHandler() in main() below.
 */
static SnsrRC
resultEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  const char *phrase;

  r = snsrGetString(s, SNSR_RES_TEXT, &phrase);
  if (r == SNSR_RC_OK) {
    printf("Spotted \"%s\", listening...\n", phrase);
    fflush(stdout);
  }
  return r;
}


/* VAD segmentation callback - speech endpoint detected
 */
static SnsrRC
endpointEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  double begin, end;

  snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
  r = snsrGetDouble(s, SNSR_RES_END_MS, &end);
  if (r != SNSR_RC_OK) return r;
  printf("VAD detected speech from %.0f ms to %.f ms.\n", begin, end);
  return SNSR_RC_STOP;
}


int
main(int argc, char *argv[])
{
  SnsrRC r;
  SnsrSession s;
  SnsrStream audio, out;
  const char *spotModel, *tmplModel;
  int testing = 0;

  if (argc < 3 || argc > 4) {
    fprintf(stderr, "usage: %s tmpl-vad-model spot-model [--test]\n", argv[0]);
    exit(1);
  }
  tmplModel = argv[1];
  spotModel = argv[2];
  testing  = argc == 4;

  /* Create a new session handle. */
  snsrNew(&s);

  /* Load and validate the spotter-vad template task file. */
  snsrLoad(s, snsrStreamFromFileName(tmplModel, "r"));
  snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT_VAD);

  /* Load the spotter into template slot 0. */
  snsrSetStream(s, SNSR_SLOT_0, snsrStreamFromFileName(spotModel, "r"));

  /* If requested, include the trigger phrase in the audio output. */
  snsrSetInt(s, SNSR_INCLUDE_LEADING_SILENCE, INCLUDE_SPOT);

  /* Register VAD endpoint callbacks. */
  snsrSetHandler(s, SNSR_END_EVENT, snsrCallback(endpointEvent, NULL, NULL));
  snsrSetHandler(s, SNSR_LIMIT_EVENT, snsrCallback(endpointEvent, NULL, NULL));

  /* Register a result callback. Private data handle is used as a flag. */
  snsrSetHandler(s, SNSR_RESULT_EVENT, snsrCallback(resultEvent, NULL, NULL));

  /* Create an audio stream instance and attach it to the session. */
  if (testing) {
    /* Read from stdin for testing. */
    audio = snsrStreamFromFILE(stdin, SNSR_ST_MODE_READ);
    /* Reduce the trailing silence time-out, as test recordings have less than
     * 1000 ms of silence at the end */
    snsrSetInt(s, SNSR_TRAILING_SILENCE, 500);
    /* Reduce VAD margins to the absolute minimum for testing only. This could
     * lead to small portions of the beginning and end of the audio being lost.
     * The recommendation is to use default values for production code.
     */
    snsrSetInt(s, SNSR_BACKOFF, 0);
    snsrSetInt(s, SNSR_HOLD_OVER, 0);
  } else {
    /* live audio */
    audio = snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT);
  }
  snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, audio);

  /* Set up the output stream. Speech-detected audio will be written to
   * this file. */
  out = snsrStreamFromFileName(VAD_AUDIO_FILE, "w");
  out = snsrStreamFromAudioStream(out, SNSR_ST_AF_DEFAULT);
  snsrSetStream(s, SNSR_SINK_AUDIO_PCM, out);

  printf("Say <trigger phrase> will it rain in Portland tomorrow?\n");

  /* Main recognition loop. The endpoint handler will cause snsrRun() to
   * return SNSR_RC_STOP. Other return codes indicate an unexpected error.
   * Session errors remain until explicitly cleared: Any errors that occured
   * earlier will also be reported here.
   */
  r = snsrRun(s);
  if (r != SNSR_RC_STOP) fatalError(r, snsrErrorDetail(s));

  snsrRelease(s);
  printf("Wrote recording to \"%s\".\n", VAD_AUDIO_FILE);
  return 0;
}
