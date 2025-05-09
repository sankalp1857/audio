/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK command-line model editing utility.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TASK_VERSION "~0.8.0 || 1.0.0"

#define DEFAULT_INIT_FILENAME  "snsr-custom-init.c";
#define DEFAULT_MODEL_FILENAME "edited-model.snsr"


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
          "usage: %s -t task [options]\n"
          " options:\n"
          "  -C tag-identifier   : emit C source to load model into RAM\n"
          "  -c tag-identifier   : emit C source to run model from code space\n"
          "  -e setting filename : extract task setting/slot into filename\n"
          "  -f setting filename : load filename into task setting/slot\n"
          "  -g setting value    : load string into task setting\n"
          "  -i                  : emit custom initialization code\n"
          "  -o out              : output filename\n"
          "  -p                  : prune unused settings to reduce model size\n"
          "  -q setting          : query a task setting\n"
          "  -s setting=value    : override a task setting\n"
          "  -t task             : specify task filename (required)\n"
          "  -v [-v [-v]]        : increase verbosity\n", name);

  snsrNew(&s);
  snsrGetString(s, SNSR_LIBRARY_INFO, &libInfo);
  fprintf(stderr, "\n%s\n", libInfo);
  snsrRelease(s);
  exit(199);
}


/* Report command-line argument errors.
 */
static void
quitOnError(SnsrSession s)
{
  SnsrRC r = snsrRC(s);
  if (r == SNSR_RC_NO_MODEL)
    fatal(r, "set -t task before -f, -q, or -s options");
  if (r != SNSR_RC_OK) fatal(r, "%s", snsrErrorDetail(s));
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


int
main(int argc, char *argv[])
{
  SnsrRC r;
  SnsrSession s;
  int customInit = 0, o, prune = 0, verbose = 0, useRAM = 0;
  const char *msg = NULL, *out = NULL, *task = NULL, *tag = NULL;
  char *outPath = NULL;
  extern char *optarg;
  extern int optind;
#ifdef SNSR_USE_SECURITY_CHIP
  uint32_t *securityChipComms(uint32_t *in);
  snsrConfig(SNSR_CONFIG_SECURITY_CHIP, securityChipComms);
#endif

  if (argc == 1) usage(argv[0]);
  r = snsrNew(&s);
  if (r != SNSR_RC_OK) fatal(r, "%s", s? snsrErrorDetail(s): snsrRCMessage(r));
  snsrSetString(s, SNSR_PREPARE_SUBSET_INIT, NULL);

  while ((o = getopt(argc, argv, "C:c:e:f:g:io:pq:s:t:v?")) >= 0) {
    switch (o) {
    case 'C':
      useRAM = 1;
    case 'c':
      tag = optarg;
      break;
    case 'e':
      if (optind >= argc) usage(argv[0]);
      {
        SnsrStream slot, out;
        snsrGetStream(s, optarg, &slot);
        quitOnError(s);
        out = snsrStreamFromFileName(argv[optind], "w");
        snsrRetain(out);
        if (verbose > 1)
          printf("Saving setting \"%s\" into \"%s\".\n", optarg, argv[optind]);
        snsrStreamCopy(out, slot, SIZE_MAX);
        if (snsrStreamRC(out) != SNSR_RC_EOF)
          fatal(snsrStreamRC(out), "%s", snsrStreamErrorDetail(out));
        snsrRelease(out);
        optind++;
      }
      break;
    case 'f':
      if (optind >= argc) usage(argv[0]);
      if (verbose > 1)
        printf("Loading \"%s\" into setting \"%s\".\n", argv[optind], optarg);
      snsrSetStream(s, optarg, snsrStreamFromFileName(argv[optind++], "r"));
      quitOnError(s);
      reportModelLicense(s, argv[optind - 1], verbose);
      if (!out) out = DEFAULT_MODEL_FILENAME;
      break;
    case 'g':
      if (optind >= argc) usage(argv[0]);
      if (verbose > 1)
        printf("Loading \"%s\" into setting \"%s\".\n", argv[optind], optarg);
      snsrSetStream(s, optarg, snsrStreamFromString(argv[optind++]));
      quitOnError(s);
      if (!out) out = DEFAULT_MODEL_FILENAME;
      break;
    case 'i':
      customInit = 1;
      if (!out) out = DEFAULT_INIT_FILENAME;
      break;
    case 'o':
      out = optarg;
      break;
    case 'p':
      prune = 1;
      break;
    case 'q':
      {
        double dblVal;
        const char *strVal = NULL;
        int intVal;
        if (snsrGetDouble(s, optarg, &dblVal) == SNSR_RC_OK) {
          printf("%s=%g\n", optarg, dblVal);
          break;
        }
        if (snsrRC(s) == SNSR_RC_INCORRECT_SETTING_TYPE) snsrClearRC(s);
        if (snsrGetInt(s, optarg, &intVal) == SNSR_RC_OK) {
          printf("%s=%i\n", optarg, intVal);
          break;
        }
        if (snsrRC(s) == SNSR_RC_INCORRECT_SETTING_TYPE) snsrClearRC(s);
        if (snsrGetString(s, optarg, &strVal) == SNSR_RC_OK) {
          printf("%s=\"%s\"\n", optarg, strVal);
          break;
        }
      }
      quitOnError(s);
      break;
    case 's':
      if (verbose > 2)
        printf("Applying setting \"%s\".\n", optarg);
      snsrSet(s, optarg);
      quitOnError(s);
      if (!out) out = DEFAULT_MODEL_FILENAME;
      break;
    case 't':
      if (verbose > 1)
        printf("Loading \"%s\" as the template model.\n", optarg);
      snsrLoad(s, snsrStreamFromFileName(optarg, "r"));
      if (!task) task = optarg;
      quitOnError(s);
      reportModelLicense(s, optarg, verbose);
      break;
    case 'v': verbose++; break;
    case '?':
    default:  usage(argv[0]);
    }
  }
  r = snsrRequire(s, SNSR_TASK_VERSION, TASK_VERSION);
  if (r == SNSR_RC_NO_MODEL || optind != argc) usage(argv[0]);

  /* Report application license status */
  if (verbose > 1) {
    snsrGetString(s, SNSR_LICENSE_EXPIRES, &msg);
    if (msg) fprintf(stderr, "\"%s\": %s.\n", argv[0], msg);
  }
  msg = NULL;
  snsrGetString(s, SNSR_LICENSE_WARNING, &msg);
  if (msg) fprintf(stderr, "WARNING for \"%s\": %s.\n", argv[0], msg);

  snsrSetString(s, SNSR_MODEL_NAME, task);

  if (tag) {
    SnsrDataFormat fmt = SNSR_FM_SOURCE;
    if (!out || !strcmp(out, DEFAULT_MODEL_FILENAME)) {
      char *t;
      if (!(out = outPath = malloc(strlen(task) + 3)))
        fatal(SNSR_RC_NO_MEMORY, "%s", snsrRCMessage(SNSR_RC_NO_MEMORY));
      strcpy(outPath, task);
      if ((t = strrchr(outPath, '.'))) *t = '\0';
      strcat(outPath, ".c");
      if ((t = strrchr(outPath, '/')) || (t = strrchr(outPath, '\\'))) {
        *t = '\0';
        out = t + 1;
      }
    }
    snsrSetString(s, SNSR_TAG_IDENTIFIER, tag);
    if (useRAM)     fmt = SNSR_FM_SOURCE_RAM;
    else if (prune) fmt = SNSR_FM_SOURCE_PRUNED;
    r = snsrSave(s, fmt, snsrStreamFromFileName(out, "w"));

  } else if (customInit) {
    r = snsrSave(s, SNSR_FM_SUBSET_INIT, snsrStreamFromFileName(out, "w"));

  } else if (out) {
    r = snsrSave(s, prune? SNSR_FM_CONFIG_PRUNED: SNSR_FM_CONFIG,
                 snsrStreamFromFileName(out, "w"));
  }

  if (r != SNSR_RC_OK) fatal(r, "%s", snsrErrorDetail(s));
  if (out && verbose > 0)
    printf("Output written to \"%s\".\n", out);
  free(outPath);
  snsrRelease(s);
  snsrTearDown();
  return 0;
}
