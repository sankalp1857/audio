/* Sensory Confidential
 * Copyright (C)2017-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK recognition from file example, where audio processing
 * is driven by the application, also known as push mode processing.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <stdlib.h>

/* Ten second output ring buffer for optional VAD */
#define RING_BUFFER_SIZE 320000

/* Process 15 ms of 16-bit audio sampled at 16 kHz */
#define CHUNK_SIZE 480

#define TASKS_SUPPORTED\
  SNSR_PHRASESPOT " 1.0.0;"\
  SNSR_PHRASESPOT_VAD " 1.0.0;"\
  SNSR_LVCSR " 1.0.0;"\
  SNSR_VAD " 1.0.0"


/* VAD endpoint event callback function.
 * Print the segmentation found, and return SNSR_RC_STOP to exit the main loop.
 */
static SnsrRC
endEvent(SnsrSession s, const char *key, void *privateData)
{
  double begin, end;
  snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
  snsrGetDouble(s, SNSR_RES_END_MS, &end);
  printf("VAD found audio from %.0f ms to %.0f ms.\n", begin, end);
  return SNSR_RC_STOP;
}


/* VAD detected silence, print a message and continue.
 */
static SnsrRC
silenceEvent(SnsrSession s, const char *key, void *privateData)
{
  printf("VAD detected silence. Listening for trigger.\n");
  return SNSR_RC_OK;
}


/* Result callback function, see snsrSetHandler() in main() below.
 * Print the result hypothesis and the start and end sample indices
 * for this phrase.
 */
static SnsrRC
resultEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  const char *phrase;
  double begin, end;

  /* Retrieve the phrase text and alignments from the session handle */
  snsrGetDouble(s, SNSR_RES_BEGIN_SAMPLE, &begin);
  snsrGetDouble(s, SNSR_RES_END_SAMPLE, &end);
  r = snsrGetString(s, SNSR_RES_TEXT, &phrase);

  /* Quit early if an error occurred. */
  if (r != SNSR_RC_OK) return r;
  printf("Recognized \"%s\" from sample %.0f to sample %.0f.\n",
         phrase, begin, end);

  return SNSR_RC_OK;
}


/* Print error message and exit */
static void
fatal(int rc, const char *format, ...)
{
  va_list a;
  va_start(a, format);
  vfprintf(stderr, format, a);
  va_end(a);
  fprintf(stderr, "\n");
  exit(rc);
}


int
main(int argc, char *argv[])
{
  SnsrRC r;
  SnsrSession s;
  SnsrStream a, out = NULL;
  char buffer[CHUNK_SIZE];
  size_t read;

  if (argc != 2 && argc != 3) fatal(255, "usage: %s model [wavefile]", argv[0]);

  /* Create a new session handle. */
  snsrNew(&s);

  /* Load the model task file. */
  snsrLoad(s, snsrStreamFromFileName(argv[1], "r"));

  /* Validate model types this application supports. */
  r = snsrRequire(s, SNSR_TASK_TYPE_AND_VERSION_LIST, TASKS_SUPPORTED);
  if (r != SNSR_RC_OK) fatal(r, "ERROR: %s", snsrErrorDetail(s));

  /* Check whether an output audio channel exists, wire it in if it does */
  r = snsrSetStream(s, SNSR_SINK_AUDIO_PCM, NULL);
  if (r == SNSR_RC_DST_CHANNEL_NOT_FOUND) {
    snsrClearRC(s);
  } else {
    out = snsrStreamFromBuffer(RING_BUFFER_SIZE, RING_BUFFER_SIZE);
    snsrRetain(out);
    snsrSetStream(s, SNSR_SINK_AUDIO_PCM, out);
    /* Register VAD endpoint callbacks. */
    snsrSetHandler(s, SNSR_END_EVENT, snsrCallback(endEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_LIMIT_EVENT, snsrCallback(endEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_SILENCE_EVENT,
                   snsrCallback(silenceEvent, NULL, NULL));
  }

  if (argc == 2) {
    /* Open a stream handle on the default microphone for live audio. */
    a = snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT);
  } else {
    /* Open a stream handle on the audio file. */
    a = snsrStreamFromAudioFile(argv[2], "r", SNSR_ST_AF_DEFAULT);
  }

  /* Register a result callback. Private data handle is not used. */
  r = snsrSetHandler(s, SNSR_RESULT_EVENT,
                     snsrCallback(resultEvent, NULL, NULL));

  /* Pure VAD models do support SNSR_RESULT_EVENTS, ignore the error */
  if (r == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);

  /* Main recognition loop. */
  do {
    /* Read from the audio stream into the temporary workspace. */
    read = snsrStreamRead(a, buffer, sizeof(*buffer), CHUNK_SIZE);
    if (snsrStreamRC(a) != SNSR_RC_OK && snsrStreamRC(a) != SNSR_RC_EOF)
      fatal(snsrStreamRC(a), "ERROR: %s", snsrStreamErrorDetail(a));

    /* Process one block of audio. */
    r = snsrPush(s, SNSR_SOURCE_AUDIO_PCM, buffer, read);

    /* The VAD endpoint callback returns SNSR_RC_STOP. */
    if (r == SNSR_RC_STOP) break;
    if (r != SNSR_RC_OK) fatal(r, "ERROR: %s", snsrErrorDetail(s));

    /* If this is pipeline includes a voice activity detector,
     * read from the ring buffer stream, then process that output
     */
    if (out) {
#define VAD_CHUNK_SIZE 2400
      short samples[VAD_CHUNK_SIZE];
      size_t read =
        snsrStreamRead(out, samples, sizeof(*samples), VAD_CHUNK_SIZE);
      if (read > 0) {
        /* samples[] now contains read VAD audio samples. */
        printf("Read %u samples from VAD stream.\n", (unsigned)read);
      }
    }
  } while (!snsrStreamAtEnd(a));

  /* Flush internal audio ring buffer, stop any session threads */
  r = snsrStop(s);

  /* Release the session. */
  snsrRelease(s);
  /* Release the audio stream. */
  snsrRelease(a);
  /* Release the ring buffer (VAD output) stream */
  snsrRelease(out);

  /* POSIX process return code. */
  return r == SNSR_RC_OK || r == SNSR_RC_STOP? 0: r;
}
