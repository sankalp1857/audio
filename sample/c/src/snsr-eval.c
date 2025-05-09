/* Sensory Confidential
 *
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK model evaluation command-line utility.
 *------------------------------------------------------------------------------
 * This utility supports evaluation of many of the TrulyHandsfree/TrulyNatural
 * SDK task types. The source code is provided as a detailed example.
 *
 * For most use keyword spotting implementations the live-spot.c sample is a
 * better starting point.
 *------------------------------------------------------------------------------
 */

#ifdef _WIN32
#  include <windows.h>
#endif

#include <snsr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TASKS_SUPPORTED\
  SNSR_PHRASESPOT " ~0.5.0 || 1.0.0;"\
  SNSR_PHRASESPOT_VAD " ~0.5.0 || 1.0.0;"\
  SNSR_LVCSR " 1.0.0;"\
  SNSR_VAD " 1.0.0;" \
  SNSR_FEATURE " 1.0.0;" \
  SNSR_FEATURE_PHRASESPOT " 1.0.0;"\
  SNSR_FEATURE_LVCSR " 1.0.0;"\
  SNSR_FEATURE_VAD " 1.0.0"

#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_FRAME_SIZE_MS  15

#if defined(_MSC_VER) && (_MSC_VER < 1900)
# define snprintf _snprintf
#endif

typedef struct {
  int nBest;              /* Requested N-best results, usually 1   */
  int verbose;            /* Amount of detail resultEvent prints   */
  unsigned isPartial:1;   /* 1 if this is a preliminary result     */
  unsigned isPhrase:1;    /* 1 if this is a phrase-level iteration */
} ResultConfig;


typedef struct {
  char *filename;         /* partial output filename, e.g. out/    */
  size_t prefix;          /* filename directory path length        */
  size_t length;          /* size of the filename buffer           */
  int verbose;
} VadContext;


static SnsrRC
showAlignment(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  ResultConfig *config = (ResultConfig *)privateData;
  const char *phrase;
  const char *partial = config->isPartial? "P ": "";
  double begin, end, score = -1.0, svscore;

  snsrGetString(s, SNSR_RES_TEXT, &phrase);
  snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
  snsrGetDouble(s, SNSR_RES_END_MS, &end);
  r = snsrGetDouble(s, SNSR_RES_SCORE, &score);
  if (r == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);
  r = snsrGetDouble(s, SNSR_RES_SV_SCORE, &svscore);
  if (r != SNSR_RC_OK) return r;

  if (config->nBest > 1 && config->isPhrase && !config->isPartial) {
    int count = 1, index = 0;
    snsrGetInt(s, SNSR_RES_COUNT, &count);
    snsrGetInt(s, SNSR_RES_INDEX, &index);
    printf("%2i/%i %6.0f %6.0f %s\n", index + 1, count, begin, end, phrase);
  } else if (config->verbose <= -2) {
    if (!config->isPartial) printf("%s\n", phrase);
  } else if (config->verbose == -1) {
    printf("%s%20s", phrase, config->isPartial? "\r": "\n");
  } else if (config->verbose == 0) {
    printf("%s%6.0f %6.0f %s\n", partial, begin, end, phrase);
  } else {
    printf("%s%6.0f %6.0f (%.4f%s) %s\n",
           partial, begin, end,
           score >= 0? score: svscore,
           score >= 0? "": " sv",
           phrase);
  }
  fflush(stdout);

  return r;
}


static SnsrRC
showAvailablePoint(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  int point, *first = (int *)privateData;

  r = snsrGetInt(s, SNSR_RES_AVAILABLE_POINT, &point);
  if (r == SNSR_RC_OK) {
    if (*first) printf("Available operating points: %i", point);
    else        printf(", %i", point);
    *first = 0;
  }
  return r;
}


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


static SnsrRC
entityIterator(SnsrSession s, const char *key, void *privateData)
{
  const char *entity, *value;
  double score = 0;

  snsrGetDouble(s, SNSR_RES_NLU_ENTITY_SCORE, &score);
  snsrClearRC(s); /* Not all models support NLU scores, ignore errors */
  snsrGetString(s, SNSR_RES_NLU_ENTITY_NAME,  &entity);
  snsrGetString(s, SNSR_RES_NLU_ENTITY_VALUE, &value);
  printf("NLU entity:   %s (%.4f) = %s\n", entity, score, value);
  return snsrRC(s);
}


static SnsrRC
intentEvent(SnsrSession s, const char *key, void *privateData)
{
  const char *intent, *value;
  double score = 0;

  snsrGetDouble(s, SNSR_RES_NLU_INTENT_SCORE, &score);
  snsrClearRC(s); /* Not all models support NLU scores, ignore errors */
  snsrGetString(s, SNSR_RES_NLU_INTENT_NAME, &intent);
  snsrGetString(s, SNSR_RES_NLU_INTENT_VALUE, &value);
  printf("NLU intent: %s (%.4f) = %s\n", intent, score, value);
  return snsrForEach(s, SNSR_NLU_ENTITY_LIST,
                     snsrCallback(entityIterator, NULL, privateData));
}


static SnsrRC
nluEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  const char *name, *parentPath = (char *)privateData, *value;
  char *path;
  double score = 0;
  size_t pathLen = 0;
  int nluMax = 1, nBest = 1;

  snsrGetDouble(s, SNSR_RES_NLU_SLOT_SCORE, &score);
  snsrClearRC(s); /* Not all models support NLU scores, ignore errors */

  snsrGetString(s, SNSR_RES_NLU_SLOT_NAME, &name);
  r = snsrGetString(s, SNSR_RES_NLU_SLOT_VALUE, &value);
  if (r != SNSR_RC_OK) return r;

  pathLen = strlen(SNSR_RES_NLU_SLOT_VALUE) + strlen(name) + 1;
  if (parentPath) pathLen += strlen(parentPath) + 1;
  path = malloc(pathLen);
  if (!path) return SNSR_RC_NO_MEMORY;
  if (!parentPath) {
    strcpy(path, SNSR_RES_NLU_SLOT_VALUE);
    strcat(path, name);
  } else {
    strcpy(path, parentPath);
    strcat(path, ".");
    strcat(path, name);
  }

  /* SNSR_NLU_RES_MAX introduced in 6.16.0, missing from older models */
  r = snsrGetInt(s, SNSR_NLU_RES_MAX, &nluMax);
  if (r != SNSR_RC_OK) snsrClearRC(s);

  /* SNSR_RESULT_MAX introduced in 6.17.0, missing from older models */
  r = snsrGetInt(s, SNSR_RESULT_MAX, &nBest);
  if (r != SNSR_RC_OK) snsrClearRC(s);

  if (nBest > 1 || nluMax > 1) {
    int nluIndex = 0, nluCount = 1, recIndex = 0, recCount = 0;
    snsrGetInt(s, SNSR_RES_COUNT, &recCount);
    snsrGetInt(s, SNSR_RES_INDEX, &recIndex);
    snsrClearRC(s); /* SNSR_RES_{COUNT,INDEX} not available before 6.17.0 */
    snsrGetInt(s, SNSR_RES_NLU_COUNT, &nluCount);
    snsrGetInt(s, SNSR_RES_NLU_INDEX, &nluIndex);
    if (recCount > 1) {
      printf("%2i/%i NLU %2i/%i %s (%.4f) = %s\n", recIndex + 1, recCount,
             nluIndex + 1, nluCount, path, score, value);
    } else {
      printf("NLU %2i/%i %s (%.4f) = %s\n",
             nluIndex + 1, nluCount, path, score, value);
    }

  } else {
    printf("NLU %s (%.4f) = %s\n", path, score, value);
  }

  r = snsrForEach(s, SNSR_NLU_SLOT_LIST, snsrCallback(nluEvent, NULL, path));
  free(path);
  return r;
}


static SnsrRC
resultEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrCallback c;
  ResultConfig *config = (ResultConfig *)privateData;
  const char *partial = config->isPartial? "P ": "";

  /* Skip empty (LVCSR) results. */
  if (config->nBest > 1) {
    const char *phrase = NULL;
    snsrGetString(s, SNSR_RES_TEXT, &phrase);
    if (phrase && !phrase[0]) return snsrRC(s);
  }

  c = snsrCallback(showAlignment, NULL, privateData);
  snsrRetain(c);

  if (config->verbose > 1) printf("%sphrase:\n", partial);
  config->isPhrase = 1;
  snsrForEach(s, SNSR_PHRASE_LIST, c);
  config->isPhrase = 0;

  if (config->verbose > 1) {
    printf("%swords:\n", partial);
    snsrForEach(s, SNSR_WORD_LIST, c);
  }
  if (config->verbose > 2) {
    printf("%sphonemes:\n", partial);
    snsrForEach(s, SNSR_PHONE_LIST, c);
  }
  if (config->verbose > 1) {
    printf("\n");
    fflush(stdout);
  }
  snsrRelease(c);
  return snsrRC(s);
}


/* The SNSR_ADAPT_STARTED_EVENT is called from a worker thread with
 * the SnsrSession argument set to NULL.
 */
static SnsrRC
adaptStartedEvent(SnsrSession s, const char *key, void *privateData)
{
  printf("       [%s] on worker thread\n", key);
  fflush(stdout);
  return SNSR_RC_OK;
}


/* Display events with sample time-stamps */
static SnsrRC
showEvent(SnsrSession s, const char *key, void *privateData)
{
  SnsrRC r;
  double samples, timestamp = 0;
  int rate = DEFAULT_SAMPLE_RATE;

  r = snsrGetInt(s, SNSR_SAMPLE_RATE, &rate);
  /* VAD task types do not include SNSR_SAMPLE_RATE support, use default */
  if (r == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);

  r = snsrGetDouble(s, SNSR_RES_SAMPLES, &samples);
  if (r == SNSR_RC_OK) {
    timestamp = samples / rate * 1000;
  } else if (r == SNSR_RC_SETTING_NOT_FOUND) {
    double frames = 0;
    snsrClearRC(s);
    r = snsrGetDouble(s, SNSR_RES_FRAMES, &frames);
    timestamp = frames * DEFAULT_FRAME_SIZE_MS;
  }

  if (privateData) {
    const char *user = "(unknown)";
    snsrGetString(s, SNSR_USER, &user);
    printf("%6.0f [%s] %s\n", timestamp, key, user);
  } else {
    printf("%6.0f [%s]\n", timestamp, key);
  }
  fflush(stdout);
  return snsrRC(s);
}


/* VAD start point detected */
static SnsrRC
vadBeginEvent(SnsrSession s, const char *key, void *privateData)
{
  VadContext *c = (VadContext *)privateData;

  if (c->verbose > 1) showEvent(s, key, NULL);
  if (c->filename) {
    SnsrStream out;
    double begin = 0;
    snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
    snprintf(c->filename + c->prefix, c->length - c->prefix, "%.0f.wav", begin);
    out = snsrStreamFromAudioFile(c->filename, "w", SNSR_ST_AF_DEFAULT);
    snsrSetStream(s, SNSR_SINK_AUDIO_PCM, out);
    snsrSetInt(s, SNSR_PASS_THROUGH, 1);
    if (c->verbose > 0) {
      printf("Saving VAD audio to \"%s\".\n", c->filename);
      fflush(stdout);
    }
  }
  return snsrRC(s);
}


/* VAD silence detected */
static SnsrRC
vadSilenceEvent(SnsrSession s, const char *key, void *privateData)
{
  VadContext *c = (VadContext *)privateData;

  if (c->verbose > 1) showEvent(s, key, NULL);
  return snsrRC(s);
}


/* Vad endpoint event */
static SnsrRC
vadEndEvent(SnsrSession s, const char *key, void *privateData)
{
  double begin, end;
  VadContext *c = (VadContext *)privateData;

  if (c->verbose > 0) {
    snsrGetDouble(s, SNSR_RES_BEGIN_MS, &begin);
    snsrGetDouble(s, SNSR_RES_END_MS, &end);
    printf("%6.0f %6.0f [%s] VAD speech region.\n", begin, end, key);
    fflush(stdout);
  }
  return snsrRC(s);
}

/* Optional SLM processing is about to start.
 */
static SnsrRC
slmStartEvent(SnsrSession s, const char *key, void *privateData)
{
  printf("SLM: ");
  fflush(stdout);
  return SNSR_RC_OK;
}


/* SLM partial result event, SNSR_RES_TEXT is the current next word prediction
 */
static SnsrRC
slmPartialResultEvent(SnsrSession s, const char *key, void *privateData)
{
  const char *txt = NULL;
  SnsrRC r = snsrGetString(s, SNSR_RES_TEXT, &txt);
  if (r != SNSR_RC_OK) return r;
  printf("%s", txt);
  fflush(stdout);
  return SNSR_RC_OK;
}

/* SLM final result event, SNSR_RES_TEXT is the entire result
 */
static SnsrRC
slmResultEvent(SnsrSession s, const char *key, void *privateData)
{
  printf("\n");
  fflush(stdout);
  return SNSR_RC_OK;
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
          "usage: %s -t task [options] [wavefile ...]\n"
          " options:\n"
          "  -d directory        : VAD audio output directory\n"
          "  -f setting filename : load filename into task setting\n"
          "  -g setting value    : load string into task setting\n"
          "  -l [-l [-l]]        : reduce verbosity\n"
          "  -o out              : VAD audio output filename\n"
          "  -p [-p]             : Enable pipeline profiling (experimental)\n"
          "  -s setting=value    : override a task setting\n"
          "  -t task             : specify task filename (required)\n"
          "  -v [-v [-v]]        : increase verbosity\n", name);
  fprintf(stderr, "\nUse a filename of - to read\n"
          "headerless linear 16-bit PCM little-endian audio from stdin.\n");
  fprintf(stderr,
          "If no wave files are specified, live audio captured from\n"
          "the default audio device is used.\n");
  fprintf(stderr, "\nThe -d and -o options are multually exclusive. "
          "The output directory\n"
          "must be writable. Audio files created by VAD segmentation are "
          "named\n  <directory>/<start-time-in-ms>.wav\n");

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
  if (r == SNSR_RC_NO_MODEL) fatal(r, "set -t task before -f or -s options");
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


/* Show CPU required to run the model in real time. This uses timing
 * information gathered while running model inference.
 */
static void
showRealTimeFactor(SnsrSession s)
{
  double cpuSeconds, rtf, samplesProcessed;
  int sampleRate;
  SnsrRC r;

  snsrClearRC(s);
  snsrGetDouble(s, SNSR_RES_CPU_SECONDS_USED, &cpuSeconds);
  snsrGetDouble(s, SNSR_RES_SAMPLES, &samplesProcessed);
  r = snsrGetInt(s, SNSR_SAMPLE_RATE, &sampleRate);
  if (r != SNSR_RC_OK) fatal(r, "%s", snsrErrorDetail(s));
  rtf = cpuSeconds / samplesProcessed * sampleRate;
  printf("CPU required for real-time recognition: %.2f%%\n", rtf * 100);
}


int
main(int argc, char *argv[])
{
  SnsrRC r;
  SnsrSession s;
  SnsrStream tmp, audio = NULL;
  int i, o, profile = 0;
  int verbose = 0;
  const char *dir = NULL, *msg = NULL, *out = NULL;
  extern char *optarg;
  extern int optind;
  ResultConfig full = {1, 0, 0, 0}, partial = {1, 0, 1, 0};
  VadContext vadContext = {NULL, 0, 0};
#ifdef SNSR_USE_SECURITY_CHIP
  uint32_t *securityChipComms(uint32_t *in);
  snsrConfig(SNSR_CONFIG_SECURITY_CHIP, securityChipComms);
#endif

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif

  if (argc == 1) usage(argv[0]);
  r = snsrNew(&s);
  if (r != SNSR_RC_OK) fatal(r, "%s", s? snsrErrorDetail(s): snsrRCMessage(r));

  while ((o = getopt(argc, argv, "d:f:g:lo:ps:t:v?")) >= 0) {
    switch (o) {
    case 'd':
      dir = optarg;
      break;
    case 'f':
      if (optind >= argc) usage(argv[0]);
      snsrSetStream(s, optarg, snsrStreamFromFileName(argv[optind++], "r"));
      quitOnError(s);
      reportModelLicense(s, argv[optind - 1], verbose);
      break;
    case 'g':
      if (optind >= argc) usage(argv[0]);
      snsrSetStream(s, optarg, snsrStreamFromString(argv[optind++]));
      quitOnError(s);
      break;
    case 'l':
      verbose--;
      break;
    case 'o':
      out = optarg;
      break;
    case 'p':
      profile++;
      break;
    case 's':
      snsrSet(s, optarg);
      quitOnError(s);
      break;
    case 't':
      snsrLoad(s, snsrStreamFromFileName(optarg, "r"));
      quitOnError(s);
      reportModelLicense(s, optarg, verbose);
      break;
    case 'v': verbose++;
      break;
    case '?':
    default:  usage(argv[0]);
    }
  }

  r = snsrRequire(s, SNSR_TASK_TYPE_AND_VERSION_LIST, TASKS_SUPPORTED);
  if (r == SNSR_RC_NO_MODEL) usage(argv[0]);
  else if (r != SNSR_RC_OK) fatal(r, "%s", snsrErrorDetail(s));

  if (out && dir) fatal(SNSR_RC_INVALID_ARG,
                        "The -d and -o options are multually exclusive.\n");

  /* Report application license status */
  if (verbose > 1) {
    snsrGetString(s, SNSR_LICENSE_EXPIRES, &msg);
    if (msg) fprintf(stderr, "\"%s\": %s.\n", argv[0], msg);
  }
  msg = NULL;
  snsrGetString(s, SNSR_LICENSE_WARNING, &msg);
  if (msg) fprintf(stderr, "WARNING for \"%s\": %s.\n", argv[0], msg);

  r = snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, NULL);
  snsrClearRC(s);
  if (r == SNSR_RC_OK) {
    /* No audio files provided, use live audio from the
     * default capture device
     */
    if (optind == argc) {
      audio = snsrStreamFromAudioDevice(SNSR_ST_AF_DEFAULT);
      if (verbose > 0) {
        printf("Using live audio from default capture device. ^C to stop.\n");
        fflush(stdout);
      }
    } else {
      /* Create stream concatenation of all the audio files */
      audio = snsrStreamFromString("");
      for (i = optind; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '\0') {
          tmp = snsrStreamFromFILE(stdin, SNSR_ST_MODE_READ);
        } else {
          tmp = snsrStreamFromAudioFile(argv[i], "r", SNSR_ST_AF_DEFAULT);
        }
        audio = snsrStreamFromStreams(audio, tmp);
      }
    }

    /* Wire up the audio input stream. */
    snsrSetStream(s, SNSR_SOURCE_AUDIO_PCM, audio);

  } else {
    /* SNSR_SOURCE_AUDIO_PCM not found, try feature-stream */
    r = snsrSetStream(s, SNSR_SOURCE_FEATURE, NULL);
    snsrClearRC(s);
    if (r == SNSR_RC_OK) {
      SnsrStream feature;
      feature = snsrStreamFromString("");
      for (i = optind; i < argc; i++) {
        SnsrStream tmp = snsrStreamFromFileName(argv[i], "r");
        feature = snsrStreamFromStreams(feature, tmp);
      }
      r = snsrSetStream(s, SNSR_SOURCE_FEATURE, feature);
    } else r = SNSR_RC_OK;
  }

  /* The SNSR_OPERATING_POINT setting was introduced with 5.0.0-beta.10 */
  if (verbose > 1 && snsrRC(s) == SNSR_RC_OK) {
    int first = 1, point = 0;
    r = snsrGetInt(s, SNSR_OPERATING_POINT, &point);
    if (r == SNSR_RC_SETTING_NOT_FOUND || r == SNSR_RC_VALUE_NOT_SET) {
      snsrClearRC(s);
    } else {
      printf("Using operating point %i.\n", point);
      snsrForEach(s, SNSR_OPERATING_POINT_LIST,
                  snsrCallback(showAvailablePoint, NULL, &first));
      printf(".\n");
      fflush(stdout);
    }
  }

  /* The SNSR_VOCAB_LIST setting was introduced with 6.7.0. */
  if (verbose > 1 && snsrRC(s) == SNSR_RC_OK) {
    int first = 1;
    snsrForEach(s, SNSR_VOCAB_LIST, snsrCallback(showVocab, NULL, &first));
    snsrClearRC(s);
  }


  /* Wire up the optional audio output stream. */
  r = snsrSetStream(s, SNSR_SINK_AUDIO_PCM, NULL);
  if (r == SNSR_RC_DST_CHANNEL_NOT_FOUND) {
    r = SNSR_RC_OK;
    snsrClearRC(s);
  } else if (out) {
    r = snsrSetStream(s, SNSR_SINK_AUDIO_PCM,
                      snsrStreamFromAudioFile(out, "w", SNSR_ST_AF_DEFAULT));
  } else {
    /* No file specified, turn off VAD audio output. */
    r = snsrSetInt(s, SNSR_PASS_THROUGH, 0);
  }

  /* Wire up the optional feature output stream. */
  if (r == SNSR_RC_OK) {
    r = snsrSetStream(s, SNSR_SINK_FEATURE, NULL);
    if (r == SNSR_RC_DST_CHANNEL_NOT_FOUND) {
      r = SNSR_RC_OK;
      snsrClearRC(s);
    } else if (out) {
      r = snsrSetStream(s, SNSR_SINK_FEATURE, snsrStreamFromFileName(out, "w"));
    } else {
      /* No file specified, turn off VAD feature output. */
      r = snsrSetInt(s, SNSR_PASS_THROUGH, 0);
    }
  }

  if (r != SNSR_RC_OK) fatal(r, "%s", snsrErrorDetail(s));

  /* SNSR_RESULT_MAX introduced in 6.17.0, missing from older models */
  r = snsrGetInt(s, SNSR_RESULT_MAX, &full.nBest);
  if (r != SNSR_RC_OK) snsrClearRC(s);

  /* Handle recognition results. */
  full.verbose = verbose;
  r = snsrSetHandler(s, SNSR_RESULT_EVENT,
                     snsrCallback(resultEvent, NULL, &full));
  /* VAD task types do not include SNSR_RESULT_EVENT support */
  if (r == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);
  else if (r != SNSR_RC_OK) fatal(r, "%s", snsrErrorDetail(s));

  /* Partial results might not be available, ignore handler setup errors. */
  partial.verbose = verbose;
  r = snsrSetHandler(s, SNSR_PARTIAL_RESULT_EVENT,
                     snsrCallback(resultEvent, NULL, &partial));
  if (r == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);

  /* VAD callback handlers. These are not supported for all task types. */
  vadContext.verbose = verbose;
  if (dir) {
    size_t dirLen = strlen(dir);
    vadContext.length = dirLen + 32;
    vadContext.filename = malloc(vadContext.length);
    if (!vadContext.filename)
      fatal(SNSR_RC_NO_MEMORY, "Could not allocate output filename buffer");
    strcpy(vadContext.filename, dir);
    if (!dirLen) strcat(vadContext.filename, "./");
    else if (dir[dirLen - 1] != '/') strcat(vadContext.filename, "/");
    vadContext.prefix = strlen(vadContext.filename);
  }
  snsrSetHandler(s, SNSR_BEGIN_EVENT,
                 snsrCallback(vadBeginEvent, NULL, &vadContext));
  snsrSetHandler(s, SNSR_SILENCE_EVENT,
                 snsrCallback(vadSilenceEvent, NULL, &vadContext));
  snsrSetHandler(s, SNSR_END_EVENT,
                 snsrCallback(vadEndEvent, NULL, &vadContext));
  snsrSetHandler(s, SNSR_LIMIT_EVENT,
                 snsrCallback(vadEndEvent, NULL, &vadContext));
  /* Ignore not-found errors for VAD handlers */
  if (snsrRC(s) == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);

  /* Prefer NLU intent events added in TrulyNatural 7.1.0 */
  if (verbose > -3) {
    r = snsrSetHandler(s, SNSR_NLU_INTENT_EVENT,
                       snsrCallback(intentEvent, NULL, NULL));
    if (r == SNSR_RC_SETTING_NOT_FOUND || verbose > 1) {
      snsrClearRC(s);
      /* NLU slot events were added in TrulyNatural 6.13.0. */
      r = snsrSetHandler(s, SNSR_NLU_SLOT_EVENT,
                         snsrCallback(nluEvent, NULL, NULL));
      if (r == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);
    }
  }

  /* Events introdcued by model version 0.13.0. */
  if (verbose > 1) {
    snsrSetHandler(s, SNSR_LISTEN_BEGIN_EVENT,
                   snsrCallback(showEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_LISTEN_END_EVENT,
                   snsrCallback(showEvent, NULL, NULL));
    /* Treat these as optional, for compatibility with older spotter models. */
    if (snsrRC(s) == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);
  }

  /* Continuous Adaptation spotters provide additional events. */
  if (verbose > 0) {
    snsrSetHandler(s, SNSR_ADAPT_STARTED_EVENT,
                   snsrCallback(adaptStartedEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_ADAPTED_EVENT,
                   snsrCallback(showEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_NEW_USER_EVENT,
                   snsrCallback(showEvent, NULL, (void *)1));
    /* Treat these as optional as only CA spotters support them */
    if (snsrRC(s) == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);
  }

  /* SLM events are optional */
  if (verbose > -3) {
    snsrSetHandler(s, SNSR_SLM_START_EVENT,
                   snsrCallback(slmStartEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_SLM_PARTIAL_RESULT_EVENT,
                   snsrCallback(slmPartialResultEvent, NULL, NULL));
    snsrSetHandler(s, SNSR_SLM_RESULT_EVENT,
                   snsrCallback(slmResultEvent, NULL, NULL));
    if (snsrRC(s) == SNSR_RC_SETTING_NOT_FOUND) snsrClearRC(s);
  }

  r = snsrRun(s);
  if (r != SNSR_RC_OK && r != SNSR_RC_STREAM_END)
    fatal(r, "%s", snsrErrorDetail(s));

  free(vadContext.filename);

  if (profile == 1) showRealTimeFactor(s);
  else if (profile > 1)
    snsrProfile(s, snsrStreamFromFILE(stdout, SNSR_ST_MODE_WRITE));

  snsrRelease(s);
  snsrTearDown();

  if (out && verbose > 0) printf("VAD audio saved to \"%s\".\n", out);
  return 0;
}
