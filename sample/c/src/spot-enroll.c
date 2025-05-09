/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK keyword spotting command-line enrollment utility.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_OUT  "enrolled-sv.snsr"
#define ENROLL_TASK_VERSION "~0.10.0 || 1.0.0"

typedef struct {
  const char *enrollfile; /* current enrollment filename              */
  const char **filename;  /* enrollment filenames, for error messages */
  const char *enrolled;   /* optional enrollment context file name    */
  const char *adapted;    /* optional adapted context file name       */
  const char *model;      /* enrolled phrase spotter output file name */
  size_t fileCount;       /* number of allocated filenames            */
  int failed;             /* number of failed enrollments             */
  int verbosity;          /* incremented by the -v flag               */
} EnrollContext;


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
  return r;
}


static SnsrRC
adaptedEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;

  r = snsrSave(s, SNSR_FM_RUNTIME, snsrStreamFromFileName(e->adapted, "w"));
  if (r == SNSR_RC_OK && e->verbosity >= 1) {
    printf("Adapted enrollment context saved to \"%s\"\n", e->adapted);
    fflush(stdout);
  }
  return r;
}


static SnsrRC
enrolledEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;

  r = snsrSave(s, SNSR_FM_RUNTIME, snsrStreamFromFileName(e->enrolled, "w"));
  if (r == SNSR_RC_OK && e->verbosity >= 1) {
    printf("Enrollment context saved to \"%s\"\n", e->enrolled);
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
    fprintf(stderr, " reason: %s", reason);
    if (e->verbosity >= 2)
      fprintf(stderr, "   (%.2f, threshold is %.2f)", value, threshold);
    fprintf(stderr, "\n    fix: %s\n", guidance);
    fflush(stdout);
  }
  return snsrRC(s);
}


static SnsrRC
failEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;
  int id;

  r = snsrGetInt(s, SNSR_RES_ENROLLMENT_ID, &id);
  if (r != SNSR_RC_OK) return r;
  fprintf(stderr, "Enrollment from file \"%s\" failed:\n",
          (size_t)id < e->fileCount? e->filename[id]: e->enrollfile);
  printReason(s, key, privateData);
  if (e->verbosity >= 3) {
    fprintf(stderr, "\nAll failed checks:\n");
    snsrForEach(s, SNSR_REASON_LIST, snsrCallback(printReason, NULL, e));
  }
  fflush(stdout);
  e->failed++;
  return SNSR_RC_OK;
}


static SnsrRC
passEvent(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;
  int id;

  r = snsrGetInt(s, SNSR_RES_ENROLLMENT_ID, &id);
  if (r != SNSR_RC_OK) return r;
  if ((size_t)id >= e->fileCount) {
    e->fileCount++;
    e->filename = (const char **)realloc((char **)e->filename,
                                         sizeof(*e->filename) * e->fileCount);
  }
  e->filename[id] = e->enrollfile;
  return SNSR_RC_OK;
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


static SnsrRC
userIterator(SnsrSession s, const char *key, void *privateData)
{
  EnrollContext *e = (EnrollContext *)privateData;
  SnsrRC r;
  int count, recommended;
  const char *user;

  snsrGetString(s, SNSR_USER, &user);
  snsrGetInt(s, SNSR_ENROLLMENT_TARGET, &recommended);
  r = snsrGetInt(s, SNSR_RES_ENROLLMENT_COUNT, &count);
  if (r == SNSR_RC_OK) {
    if (e->verbosity >= 2)
      printf("%16s: %u enrollment%s.\n", user, count, count == 1? "": "s");
    if (count != recommended)
      fprintf(stderr, "WARNING: \"%s\" has %i enrollment%s, task recommends "
              "%i for optimal performance.\n",
              user, count, count == 1? "": "s", recommended);
    fflush(stdout);
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
          "usage: %s -t task [options] "
          "[+user1 file1 [-c] file2 ...] [+user2 ...]\n"
          " options:\n"
          "  -a adaptedfile   : adapted enrollment context output filename\n"
          "  -c file          : recording contains trailing context\n"
          "  -e enrolledfile  : enrollment context output filename\n"
          "  -o out           : enrolled model output filename (default: "
          DEFAULT_OUT ")\n"
          "  -s setting=value : override a task setting\n"
          "  -t task          : specify task filename (required)\n"
          "  -v [-v [-v]]     : increase verbosity\n", name);

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


/* List enrollment phrases and IDs, where available
 */
static SnsrRC
showVocab(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  const char *text = NULL;
  int id = -1, *first = (int *)privateData;

  snsrGetInt(s, SNSR_RES_ID, &id);
  r = snsrGetString(s, SNSR_RES_TEXT, &text);
  if (r != SNSR_RC_OK) return r;
  if (*first) printf("Available vocabulary:\n");
  printf(" %2i: \"%s\"\n", id, text);
  *first = 0;
  return r;
}


int
main(int argc, char *argv[])
{
  EnrollContext e;
  SnsrRC r;
  SnsrSession s;
  int i, o, rejected = 0;
  const char *msg = NULL;
  extern char *optarg;
  extern int optind;
  const char *u = NULL;
#ifdef SNSR_USE_SECURITY_CHIP
  uint32_t *securityChipComms(uint32_t *in);
  snsrConfig(SNSR_CONFIG_SECURITY_CHIP, securityChipComms);
#endif

  if (argc == 1) usage(argv[0]);
  r = snsrNew(&s);
  if (r != SNSR_RC_OK) fatal(r, s? snsrErrorDetail(s): snsrRCMessage(r));
  e.failed = 0;
  e.verbosity = 0;
  e.enrolled = NULL;
  e.adapted = NULL;
  e.model  = DEFAULT_OUT;
  e.fileCount = 0;
  e.filename = NULL;

  while ((o = getopt(argc, argv, "+a:e:o:s:t:v?")) >= 0) {
    switch (o) {
    case 'a':
      e.adapted = optarg;
      break;
    case 'e':
      e.enrolled = optarg;
      break;
    case 'o':
      e.model = optarg;
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
    default: usage(argv[0]);
    }
  }

  r = snsrSetInt(s, SNSR_INTERACTIVE_MODE, 0);
  if (r  == SNSR_RC_NO_MODEL) usage(argv[0]);

  /* Report application license status */
  if (e.verbosity > 1) {
    snsrGetString(s, SNSR_LICENSE_EXPIRES, &msg);
    if (msg) fprintf(stderr, "\"%s\": %s.\n", argv[0], msg);
  }
  msg = NULL;
  snsrGetString(s, SNSR_LICENSE_WARNING, &msg);
  if (msg) fprintf(stderr, "WARNING for \"%s\": %s.\n", argv[0], msg);

  snsrSetHandler(s, SNSR_DONE_EVENT, snsrCallback(doneEvent, NULL, &e));
  snsrSetHandler(s, SNSR_FAIL_EVENT, snsrCallback(failEvent, NULL, &e));
  snsrSetHandler(s, SNSR_PASS_EVENT, snsrCallback(passEvent, NULL, &e));
  snsrSetHandler(s, SNSR_PROG_EVENT, snsrCallback(progEvent, NULL, &e));
  if (e.enrolled)  snsrSetHandler(s, SNSR_ENROLLED_EVENT,
                                  snsrCallback(enrolledEvent, NULL, &e));
  if (e.adapted)  snsrSetHandler(s, SNSR_ADAPTED_EVENT,
                                 snsrCallback(adaptedEvent, NULL, &e));

  /* SNSR_VOCAB_LIST is supported for a subset of models only, ignore errors */
  if (e.verbosity > 2 && snsrRC(s) == SNSR_RC_OK) {
    int first = 1;
    snsrForEach(s, SNSR_VOCAB_LIST, snsrCallback(showVocab, NULL, &first));
    snsrClearRC(s);
  }

  if (optind + 1 < argc) {
    int enrollmentIndex = 0, idx = -1, errors;
    if (argv[optind][0] != '+') usage(argv[0]);
    for (i = optind; i < argc; i++) {
      if (argv[i][0] == '+') {
        u = argv[i] + 1;
        snsrSetString(s, SNSR_USER, u);
      } else {
        SnsrStream a;
        int hasContext;
        hasContext = !strcmp("-c", argv[i]);
        if (hasContext && ++i >= argc) usage(argv[0]);
        a = snsrStreamFromFileName(argv[i], "r");
        e.enrollfile = argv[i];
        a = snsrStreamFromAudioStream(a, SNSR_ST_AF_DEFAULT);
        snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, a);
        snsrSetInt(s, SNSR_ADD_CONTEXT, hasContext);
        if (e.verbosity >= 2) {
          printf("Enrolling user \"%s\"%s from file \"%s\".\n",
                 u, hasContext? " with context": "", argv[i]);
          fflush(stdout);
        }
        errors = e.failed;
        if (snsrRun(s) == SNSR_RC_STREAM_END) snsrClearRC(s);
        snsrGetInt(s, SNSR_RES_ENROLLMENT_COUNT, &idx);
        if (idx == enrollmentIndex && errors == e.failed) {
          fprintf(stderr, "Enrollment skipped for \"%s\", amplitude too low?\n",
                  e.enrollfile);
          e.failed++;
        }
        if (e.failed > errors) rejected++;
        enrollmentIndex = idx;
      }
      if (snsrRC(s) != SNSR_RC_OK) fatal(snsrRC(s), snsrErrorDetail(s));
    }
  }
  if (rejected) fatal(100,"%u enrollment %s rejected.",
                      rejected, rejected == 1? "file was": "files were");
  snsrForEach(s, SNSR_USER_LIST, snsrCallback(userIterator, NULL, &e));
  snsrSetString(s, SNSR_USER, NULL); /* end-of-enrollment marker */
  snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, snsrStreamFromString(""));
  if (snsrRun(s) == SNSR_RC_STREAM_END) snsrClearRC(s);
  if (snsrRC(s) != SNSR_RC_OK) fatal(snsrRC(s), snsrErrorDetail(s));
  snsrRelease(s);
  snsrTearDown();
  free((char **)e.filename);

  return e.failed;
}
