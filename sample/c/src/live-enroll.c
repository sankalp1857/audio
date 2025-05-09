/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK keyword spotting command-line enrollment utility,
 * using live audio from the default capture source.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_OUT  "enrolled-sv.snsr"
#define ENROLL_TASK_VERSION "~0.8.0 || 1.0.0"

#ifdef _MSC_VER
#  define strdup _strdup
#  if _MSC_VER < 1900
#    define snprintf _snprintf
#  endif
#endif


typedef struct {
  const char *enroll;   /* optional enrollment context file name    */
  const char *model;    /* enrolled phrase spotter output file name */
  const char *prefix;   /* audio capture file name prefix           */
  const char **user;    /* pointer to users in argv[]               */
  char *phrase;         /* enrollment phrase                        */
  SnsrStream audio;     /* audio stream handle                      */
  int verbosity;        /* incremented by the -v flag               */
  int userCount;        /* number of users to enroll                */
  int currentUser;      /* current user index                       */
  int failCount;        /* number of failed enrollment attempts     */
} EnrollContext;


static SnsrRC
saveEnrollmentAudio(SnsrSession s, EnrollContext *e, const char *tag, int id)
{
  SnsrStream out, enrollment;
  SnsrRC r;
  const char *dash, *user = NULL;
  const char *format = "%s%s%s-%s-%i.wav";
  char *tmp;
  int len;

  dash = *e->prefix? "-": "";
  snsrGetString(s, SNSR_USER, &user);
  r = snsrGetStream(s, SNSR_AUDIO_STREAM, &enrollment);
  if (r != SNSR_RC_OK) return r;
  if (snsrStreamRC(enrollment) != SNSR_RC_OK) return SNSR_RC_OK;

  len = snprintf(NULL, 0, format, e->prefix, dash, user, tag, id);
  if (len < 0) {
    snsrDescribeError(s, "snprintf() failed, rc = %i\n", len);
    return SNSR_RC_ERROR;
  }
  tmp = malloc(++len);
  if (!tmp) return SNSR_RC_NO_MEMORY;
  snprintf(tmp, len, format, e->prefix, dash, user, tag, id);
  out = snsrStreamFromAudioFile(tmp, "w", SNSR_ST_AF_DEFAULT);
  snsrRetain(out);

  snsrStreamCopy(out, enrollment, SIZE_MAX);
  if ((r = snsrStreamRC(out)) == SNSR_RC_EOF) r = SNSR_RC_OK;
  if (r != SNSR_RC_OK) snsrDescribeError(s, "%s", snsrStreamErrorDetail(out));
  else if (e->verbosity > 0) {
    printf("Saved enrollment audio to %s\n", tmp);
    fflush(stdout);
  }

  snsrRelease(out);
  free(tmp);
  return r;
}


static SnsrRC
nextEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  const char *tag;

  if (e->currentUser >= e->userCount) return SNSR_RC_OK;
  tag = e->user[e->currentUser++] + 1;
  return snsrSetString(s, SNSR_USER, tag);
}


static SnsrRC
passEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  int id = 0;

  if (e->verbosity >= 1) {
    printf("Preliminary enrollment checks passed.\n");
    fflush(stdout);
  }
  if (!e->prefix) return SNSR_RC_OK;
  snsrGetInt(s, SNSR_RES_ENROLLMENT_ID, &id);
  return saveEnrollmentAudio(s, e, "pass", id);
}


static SnsrRC
pauseEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  snsrStreamClose(e->audio);
  printf("\n");
  fflush(stdout);
  return SNSR_RC_OK;
}


static SnsrRC
resumeEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;
  const char *tag;
  int count, target;
  int ctx;

  snsrStreamOpen(e->audio);
  snsrGetInt(s, SNSR_ENROLLMENT_TARGET, &target);
  snsrGetInt(s, SNSR_RES_ENROLLMENT_COUNT, &count);
  snsrGetInt(s, SNSR_ADD_CONTEXT, &ctx);
  r = snsrGetString(s, SNSR_USER, &tag);
  if (r == SNSR_RC_OK) {
    printf("\nSay %s (%i/%i) for \"%s\"", e->phrase, count + 1, target, tag);
    if (ctx) printf(" with context,\n  for example: "
                    "\"<phrase> will it rain tomorrow?\"");
    printf("\n");
    fflush(stdout);
  }
  return r;
}


static SnsrRC
samplesEvent(SnsrSession s, const char *key, void *privateData)
{
  double count;
  snsrGetDouble(s, SNSR_RES_SAMPLES, &count);
  printf("Recording: %6.2f s\r", count / 16000.0);
  fflush(stdout);
  return SNSR_RC_OK;
}


static SnsrRC
doneEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;
  SnsrStream model = NULL, out;
  size_t written;

  r = snsrGetStream(s, SNSR_MODEL_STREAM, &model);
  if (r != SNSR_RC_OK) return r;
  written = snsrStreamGetMeta(model, SNSR_ST_META_BYTES_WRITTEN);
  out = snsrStreamFromFileName(e->model, "w");
  snsrStreamCopy(out, model, written);
  r = snsrStreamRC(out);
  if (r != SNSR_RC_OK) snsrDescribeError(s, "%s", snsrStreamErrorDetail(out));
  snsrRelease(out);
  if (r == SNSR_RC_OK && e->verbosity >= 1) {
    printf("Enrolled model saved to \"%s\"\n", e->model);
    fflush(stdout);
  }
  if (r != SNSR_RC_OK) return r;
  return SNSR_RC_STOP;
}


static SnsrRC
enrolledEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;

  r = snsrSave(s, SNSR_FM_RUNTIME, snsrStreamFromFileName(e->enroll, "w"));
  if (r == SNSR_RC_OK && e->verbosity >= 1) {
    printf("Enrollment context saved to \"%s\"\n", e->enroll);
    fflush(stdout);
  }
  return r;
}


static SnsrRC
printReason(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  const char *guidance, *reason;
  int pass = 0;
  double value = 0.0, threshold = 0.0;

  snsrGetInt(s, SNSR_RES_REASON_PASS, &pass);
  if (pass) return snsrRC(s);
  snsrGetString(s, SNSR_RES_REASON, &reason);
  snsrGetString(s, SNSR_RES_GUIDANCE, &guidance);
  snsrGetDouble(s, SNSR_RES_REASON_VALUE, &value);
  snsrGetDouble(s, SNSR_RES_REASON_THRESHOLD, &threshold);
  if (snsrRC(s) == SNSR_RC_OK) {
    fprintf(stderr, "This enrollment recording is not usable.\n");
    fprintf(stderr, " Reason: %s", reason);
    if (e->verbosity >= 2)
      fprintf(stderr, "   (%.2f, threshold is %.2f)", value, threshold);
    fprintf(stderr, "\n    Fix: %s\n", guidance);
    fflush(stdout);
  }
  return snsrRC(s);
}


static SnsrRC
failEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;

  printReason(s, key, privateData);
  if (e->verbosity >= 3) {
    fprintf(stderr, "\nAll failed checks:\n");
    fflush(stdout);
    snsrForEach(s, SNSR_REASON_LIST, snsrCallback(printReason, NULL, e));
  }
  if (!e->prefix) return SNSR_RC_OK;
  return saveEnrollmentAudio(s, e, "fail", e->failCount++);
}


static SnsrRC
progEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r = SNSR_RC_OK;
  EnrollContext *e = (EnrollContext *)privateData;

  if (e->verbosity >= 1) {
    double progress;
    r = snsrGetDouble(s, SNSR_RES_PERCENT_DONE, &progress);
    if (r == SNSR_RC_OK) {
      printf("\rAdapting: %3.0f%% complete.", progress);
      if (progress >= 100) printf("\n");
      fflush(stdout);
    }
  }
  return r;
}


static void
fatal(int rc, const char *format, ...)
{
  va_list a;
  fprintf(stderr, "ERROR: ");
  va_start(a, format);
  vfprintf(stderr, format, a);
  va_end(a);
  fprintf(stderr, "\n");
  exit(rc);
}


static void
usage(const char *name)
{
  SnsrSession s;
  const char *libInfo;

  fprintf(stderr,
          "usage: %s -t task [options] +user1 [+user2 ...] [file ...]\n"
          " options:\n"
          "  -e enrollments   : enrollment context output filename\n"
          "  -o out           : enrolled model output filename (default: "
          DEFAULT_OUT ")\n"
          "  -p prefix        : capture each enrollment to file as\n"
          "                     <prefix>-<user>-{pass,fail}-<index>.wav\n"
          "  -s setting=value : override a task setting\n"
          "  -t task          : specify task filename (required)\n"
          "  -v [-v [-v]]     : increase verbosity\n", name);
  fprintf(stderr,
          "\nEnrollment audio is captured from the default microphone, unless\n"
          "the optional [file ...] arguments are supplied.\n");

  snsrNew(&s);
  snsrGetString(s, SNSR_LIBRARY_INFO, &libInfo);
  fprintf(stderr, "\n%s\n", libInfo);
  snsrRelease(s);
  exit(199);
}


/* Report model license keys.
 */
static void
reportModelLicense(SnsrSession s, const char *modelfile, int verbose)
{
  const char *msg = NULL;

  if (verbose > 1) {
    snsrGetString(s, SNSR_MODEL_LICENSE_EXPIRES, &msg);
    if (msg)
      fprintf(stderr, "\"%s\": %s.\n", modelfile, msg);
  }
  msg = NULL;
  snsrGetString(s, SNSR_MODEL_LICENSE_WARNING, &msg);
  if (msg)
    fprintf(stderr, "WARNING for model \"%s\": %s.\n", modelfile, msg);
}


/* Store the first enrollment phrase in e.phrase.
 */
static SnsrRC
getVocab(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;
  const char *vocab;

  r = snsrGetString(s, SNSR_RES_TEXT, &vocab);
  if (r != SNSR_RC_OK) return r;
  free(e->phrase);
  e->phrase = strdup(vocab);
  return r;
}


int
main(int argc, char *argv[])
{
  EnrollContext e;
  SnsrRC r;
  SnsrSession s;
  int i, o;
  const char *msg = NULL;
  extern char *optarg;
  extern int optind;
#ifdef SNSR_USE_SECURITY_CHIP
  uint32_t *securityChipComms(uint32_t *in);
  snsrConfig(SNSR_CONFIG_SECURITY_CHIP, securityChipComms);
#endif

  if (argc == 1) usage(argv[0]);
  r = snsrNew(&s);
  if (r != SNSR_RC_OK) fatal(r, s? snsrErrorDetail(s): snsrRCMessage(r));
  e.currentUser = 0;
  e.enroll = NULL;
  e.phrase = strdup("the enrollment phrase");
  e.prefix = NULL;
  e.model  = DEFAULT_OUT;
  e.failCount = 0;
  e.userCount = 0;
  e.verbosity = 0;

  while ((o = getopt(argc, argv, "e:o:p:s:t:v?")) >= 0) {
    switch (o) {
    case 'e':
      e.enroll = optarg;
      break;
    case 'o':
      e.model = optarg;
      break;
    case 'p':
      e.prefix = optarg;
      r = snsrSetInt(s, SNSR_SAVE_ENROLLMENT_AUDIO, 1);
      if (r == SNSR_RC_NO_MODEL)
        fatal(r, "set -t task before -p prefix");
      break;
    case 's':
      r = snsrSet(s, optarg);
      if (r == SNSR_RC_NO_MODEL)
        fatal(r, "set -t task before -s setting=value");
      else if (r != SNSR_RC_OK)
        fatal(r, snsrErrorDetail(s));
      break;
    case 't':
      snsrLoad(s, snsrStreamFromFileName(optarg, "r"));
      snsrRequire(s, SNSR_TASK_TYPE, SNSR_ENROLL);
      r = snsrRequire(s, SNSR_TASK_VERSION, ENROLL_TASK_VERSION);
      if (r != SNSR_RC_OK) fatal(r, snsrErrorDetail(s));
      reportModelLicense(s, optarg, e.verbosity);
      break;
    case 'v': e.verbosity++; break;
    case '?':
    default:  usage(argv[0]);
    }
  }

  if (optind == argc || argv[optind][0] != '+') usage(argv[0]);

  /* Report application license status */
  if (e.verbosity > 1) {
    snsrGetString(s, SNSR_LICENSE_EXPIRES, &msg);
    if (msg) fprintf(stderr, "\"%s\": %s.\n", argv[0], msg);
  }
  msg = NULL;
  snsrGetString(s, SNSR_LICENSE_WARNING, &msg);
  if (msg) fprintf(stderr, "WARNING for \"%s\": %s.\n", argv[0], msg);

  r = snsrSetInt(s, SNSR_INTERACTIVE_MODE, 1);
  if (r  == SNSR_RC_NO_MODEL) usage(argv[0]);
  snsrSetHandler(s, SNSR_NEXT_EVENT, snsrCallback(nextEvent, NULL, &e));
  snsrSetHandler(s, SNSR_DONE_EVENT, snsrCallback(doneEvent, NULL, &e));
  snsrSetHandler(s, SNSR_FAIL_EVENT, snsrCallback(failEvent, NULL, &e));
  snsrSetHandler(s, SNSR_PASS_EVENT, snsrCallback(passEvent, NULL, &e));
  snsrSetHandler(s, SNSR_PROG_EVENT, snsrCallback(progEvent, NULL, &e));
  snsrSetHandler(s, SNSR_PAUSE_EVENT, snsrCallback(pauseEvent, NULL, &e));
  snsrSetHandler(s, SNSR_RESUME_EVENT, snsrCallback(resumeEvent, NULL, &e));
  snsrSetHandler(s, SNSR_SAMPLES_EVENT, snsrCallback(samplesEvent, NULL, &e));
  if (e.enroll) snsrSetHandler(s, SNSR_ENROLLED_EVENT,
                               snsrCallback(enrolledEvent, NULL, &e));

  /* SNSR_VOCAB_LIST is supported for a subset of models only, ignore errors */
  if (snsrRC(s) == SNSR_RC_OK) {
    snsrForEach(s, SNSR_VOCAB_LIST, snsrCallback(getVocab, NULL, &e));
    snsrClearRC(s);
  }

  for (i = optind; i < argc && argv[i][0] == '+'; i++)
    ;
  e.user = (const char **)argv + optind;
  e.userCount = i - optind;
  if (i == argc) {
    e.audio = snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT);
  } else {
    SnsrStream tmp;
    e.audio = snsrStreamFromString("");
    for (; i < argc; i++) {
      tmp = snsrStreamFromFileName(argv[i], "r");
      tmp = snsrStreamFromAudioStream(tmp, SNSR_ST_AF_DEFAULT);
      e.audio = snsrStreamFromStreams(e.audio, tmp);
    }
  }
  snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, e.audio);
  r = snsrRun(s);
  if (r != SNSR_RC_OK && r != SNSR_RC_STOP)
    fatal(snsrRC(s), snsrErrorDetail(s));
  free(e.phrase);
  snsrRelease(s);
  snsrTearDown();
  return 0;
}
