/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK model conversion command-line utility.
 *------------------------------------------------------------------------------
 */

#include <snsr.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMBED_TASK_VERSION "~0.6.0 || 1.0.0"

#define HEADER_NAME "-search.h"
#define SEARCH_NAME "-search.bin"
#define ACMODEL_NAME "-net.bin"

#define FILENAME_SIZE 1023
#define KEY_SIZE      64
#define TARGET_SIZE   16

#if defined(_MSC_VER) && (_MSC_VER < 1900)
# define snprintf _snprintf
#endif


typedef struct {
  const char *basename; /* output file prefix                             */
  const char *slot;     /* slot prefix                                    */
  const char *target;   /* embedded target descriptor                     */
  int fileNameInfo;     /* append version and operating point to filename */
  int outputC;          /* true to generate C output files                */
  int point;            /* operating point to convert                     */
  int verbose;          /* feedback verbosity                             */
} ConvertContext;


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


/* Concatenates head, ".", and tail into key, and returns
 * a pointer to key.
 */
const char *
slotKey(const char *head, const char *tail, char key[KEY_SIZE + 1])
{
  if (strlen(head) + strlen(tail) + 2 > KEY_SIZE)
    fatal(SNSR_RC_INVALID_ARG, "-q slotname prefix is too long.");
  strncpy(key, head, KEY_SIZE);
  strncat(key, ".", KEY_SIZE);
  strncat(key, tail, KEY_SIZE);
  key[KEY_SIZE - 1] = '\0';
  return key + (*head == '\0'); /* skip leading . if head is empty */
}


static void
writeFile(SnsrStream model, ConvertContext *ctx, const char *tag,
          const char *pre, const char *ver, const char *prod, const char *mid,
          const char *post, const char *ext, const char *mode)
{
  SnsrRC r;
  SnsrStream output;
  size_t written;
  char filename[FILENAME_SIZE + 1];

  filename[FILENAME_SIZE] = '\0';
  strncpy(filename, pre, FILENAME_SIZE);
  if (ctx->fileNameInfo) {
    if (ver) {
      strncat(filename, ver, FILENAME_SIZE);
      strncat(filename, "-", FILENAME_SIZE);
    }
    strncat(filename, mid, FILENAME_SIZE);
    strncat(filename, prod, FILENAME_SIZE);
  }
  strncat(filename, post, FILENAME_SIZE);
  strncat(filename, ext, FILENAME_SIZE);
  written = snsrStreamGetMeta(model, SNSR_ST_META_BYTES_WRITTEN);
  output = snsrStreamFromFileName(filename, mode);
  snsrRetain(output);
  snsrStreamCopy(output, model, written);
  r = snsrStreamRC(output);
  if (r != SNSR_RC_OK) fatal(r, snsrStreamErrorDetail(output));
  snsrRelease(output);
  if (ctx->verbose > 0) {
    printf("wrote %s to \"%s\"\n", tag, filename);
    fflush(stdout);
  }
}


static SnsrRC
writeEmbeddedFiles(SnsrSession s, ConvertContext *ctx, const char *slot)
{
  SnsrRC r;
  SnsrStream net = NULL, sch = NULL, hdr = NULL;
#define OP_SIZE 6
  char op[OP_SIZE];
  char kb[KEY_SIZE + 1];
  char srcTarget[TARGET_SIZE + 1];
  const char *tSliceVersion = NULL;
  int prodReady = 0;
  const char *key, *prod = "prod-";

  r = snsrSetString(s, slotKey(slot, SNSR_EMBEDDED_TARGET, kb), ctx->target);
  if (r == SNSR_RC_SETTING_NOT_FOUND)
    fatal(snsrRC(s), "This model cannot be converted to DSP format."
          " (%s)", snsrErrorDetail(s));

  snsrGetStream(s, slotKey(slot, SNSR_EMBEDDED_ACMODEL_STREAM, kb), &net);
  if (net) snsrRetain(net);
  snsrGetStream(s, slotKey(slot, SNSR_EMBEDDED_SEARCH_STREAM, kb), &sch);
  if (sch) snsrRetain(sch);
  snsrGetStream(s, slotKey(slot, SNSR_EMBEDDED_HEADER_STREAM, kb), &hdr);
  if (hdr) snsrRetain(hdr);
  key = slotKey(slot, SNSR_RES_MIN_EMBEDDED_VERSION, kb);
  snsrGetString(s, key, &tSliceVersion);
  if (tSliceVersion) snsrRetain(tSliceVersion);
  key = slotKey(slot, SNSR_RES_EMBEDDED_MODEL_PRODUCTION_READY, kb);
  r = snsrGetInt(s, key, &prodReady);
  if (r != SNSR_RC_OK) fatal(snsrRC(s), "%s", snsrErrorDetail(s));

  snprintf(op, OP_SIZE, "op%02i-", ctx->point);
  if (ctx->verbose > 0) {
    printf("operating-point: %i\n", ctx->point);
    fflush(stdout);
  }
  if (!prodReady) prod = "dev-";
  if (ctx->verbose > 0) {
    printf("production-ready: %s\n", prodReady? "yes": "no");
    fflush(stdout);
  }

  writeFile(net, ctx, "acoustic model (bin)",
            ctx->basename, tSliceVersion, prod,  op, "net", ".bin", "w");
  snsrRelease(net);
  writeFile(sch, ctx, "search model (bin)",
            ctx->basename, tSliceVersion, prod,  op, "search", ".bin", "w");
  snsrRelease(sch);
  writeFile(hdr, ctx, "search header",
            ctx->basename, tSliceVersion, prod,  op, "search", ".h", "wt");
  snsrRelease(hdr);

  if (ctx->outputC) {
    memset(srcTarget, 0, TARGET_SIZE + 1);
    strncpy(srcTarget, "src:", TARGET_SIZE);
    strncat(srcTarget, ctx->target, TARGET_SIZE);
    snsrSetString(s, slotKey(slot, SNSR_EMBEDDED_TARGET, kb), srcTarget);
    snsrGetStream(s, slotKey(slot, SNSR_EMBEDDED_ACMODEL_STREAM, kb), &net);
    writeFile(net, ctx, "acoustic model (C)",
              ctx->basename, tSliceVersion, prod,  op, "net", ".c", "wt");
    snsrGetStream(s, slotKey(slot, SNSR_EMBEDDED_SEARCH_STREAM, kb), &sch);
    writeFile(sch, ctx, "search model (C)",
              ctx->basename, tSliceVersion, prod, op, "search", ".c", "wt");
  }
  snsrRelease(tSliceVersion);
  return snsrRC(s);
}


static SnsrRC
convertAllPoints(SnsrSession s, const char *key, void *data)
{
  ConvertContext *c = (ConvertContext *)data;
  const char *slot = c->slot;
  char keyBuff[KEY_SIZE + 1];
  SnsrRC r;

  snsrGetInt(s, slotKey(slot, SNSR_RES_AVAILABLE_POINT, keyBuff), &c->point);
  r = snsrSetInt(s, slotKey(slot, SNSR_OPERATING_POINT, keyBuff), c->point);
  if (r != SNSR_RC_OK) return r;
  return writeEmbeddedFiles(s, c, slot);
}


static void
usage(const char *name)
{
  SnsrSession s;
  const char *libInfo;

  fprintf(stderr,
          "usage: %s -t task [options] target\n"
          " options:\n"
          "  -a               : convert all operating-points\n"
          "  -c               : create .c output (in addition to .bin)\n"
          "  -o output        : full prefix for output filenames\n"
          "  -p output-prefix : prefix for output filenames "
          "(default: task-target-)\n"
          "  -q slotname      : model slot prefix\n"
          "  -s setting=value : override a task setting\n"
          "  -t task          : set a task filename (required)\n"
          "  -v [-v [-v]]     : increase verbosity\n", name);

  fprintf(stderr,
          "\n"
          "Output filenames are determined by the model parameters:\n"
          "  $(prefix) [-] [slot$(slotname)-] $(target)- $(version)-\n"
          "    op$(operating-point)- {dev,prod}- {net,search}.{bin,c,h}\n"
          "where:\n"
          "  prefix specified by the -p option, or taken from the filename\n"
          "         of the task if -p isn't used.\n"
          "  version is the oldest DSP library that can run this model.\n"
          "  -dev-  models are limited in runtime or number of recognition\n"
          "         events and should not be used in products.\n"
          "  -prod- models are not limited and ready for production use.\n"
          "\n"
          "Use the -o option to override this filename pattern to:\n"
          "  $(prefix)[-]{net,search}.{bin,c,h}\n"
          "\n"
          "The -o and -a options are mutually exclusive.\n"
          "\n"
          "Output filenames are constrained to never start with \"-\"\n");

  snsrNew(&s);
  snsrGetString(s, SNSR_LIBRARY_INFO, &libInfo);
  fprintf(stderr, "\n%s\n", libInfo);
  snsrRelease(s);
  exit(199);
}


/* Report model license keys that have imminent expiration dates.
 */
static void
reportExpiringModelLicense(SnsrSession s, const char *modelfile)
{
  const char *expWarning = NULL;
  snsrGetString(s, SNSR_MODEL_LICENSE_WARNING, &expWarning);
  if (expWarning)
    fprintf(stderr, "WARNING for model \"%s\": %s.\n", modelfile, expWarning);
}


int
main(int argc, char *argv[])
{
  SnsrRC r;
  SnsrSession s;
  char basename[FILENAME_SIZE + 1];
  char keyBuff[KEY_SIZE + 1];
  int allPoints = 0, o;
  const char *prefix = NULL, *task = NULL;
  ConvertContext ctx;
  extern char *optarg;
  extern int optind;
#ifdef SNSR_USE_SECURITY_CHIP
  uint32_t *securityChipComms(uint32_t *in);
  snsrConfig(SNSR_CONFIG_SECURITY_CHIP, securityChipComms);
#endif

  if (argc == 1) usage(argv[0]);

  ctx.basename = basename;
  ctx.slot = "";
  ctx.target = NULL;
  ctx.outputC = 0;
  ctx.point = 0;
  ctx.fileNameInfo = 1;
  ctx.verbose = 0;

  r = snsrNew(&s);
  if (r != SNSR_RC_OK) fatal(r, s? snsrErrorDetail(s): snsrRCMessage(r));

  while ((o = getopt(argc, argv, "aco:p:q:s:t:v?")) >= 0) {
    switch (o) {
    case 'a':
      allPoints = 1;
      break;
    case 'c':
      ctx.outputC = 1;
      break;
    case 'o':
      prefix = optarg;
      ctx.fileNameInfo = 0;
      break;
    case 'p':
      prefix = optarg;
      break;
    case 'q':
      ctx.slot = optarg;
      break;
    case 's':
      r = snsrSet(s, optarg);
      if (r == SNSR_RC_NO_MODEL)
        fatal(r, "Set -t task before -s setting=value");
      else if (r != SNSR_RC_OK)
        fatal(r, snsrErrorDetail(s));
      break;
    case 't':
      snsrLoad(s, snsrStreamFromFileName(optarg, "r"));
      snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT);
      r = snsrRequire(s, SNSR_TASK_VERSION, EMBED_TASK_VERSION);
      if (r != SNSR_RC_OK) fatal(r, snsrErrorDetail(s));
      task = optarg;
      reportExpiringModelLicense(s, optarg);
      break;
    case 'v': ctx.verbose++; break;
    case '?':
    default:  usage(argv[0]);
    }
  }
  if (optind != argc - 1) usage(argv[0]);
  ctx.target = argv[optind];
  if (allPoints && !ctx.fileNameInfo)
    fatal(SNSR_RC_INVALID_ARG, "The -a and -o options are mutually exclusive.");
  r = snsrRequire(s, SNSR_TASK_TYPE, SNSR_PHRASESPOT);
  if (r == SNSR_RC_NO_MODEL) usage(argv[0]);

  /* We'll include the source filename in the header output */
  r = snsrSetString(s, SNSR_MODEL_NAME, task);
  if (r != SNSR_RC_OK) fatal(r, snsrErrorDetail(s));

  /* Output filename prefix buffer */
  basename[FILENAME_SIZE] = '\0';
  if (prefix) strncpy(basename, prefix, FILENAME_SIZE);
  else {
    char *e;
    assert(task);
    if ((e = (char *)strrchr(task, '/')))
      strncpy(basename, e + 1, FILENAME_SIZE);
    else strncpy(basename, task, FILENAME_SIZE);
    if ((e = strrchr(basename, '.'))) *e = '\0';
  }
  if (*basename) strncat(basename, "-", FILENAME_SIZE);
  if (ctx.fileNameInfo) {
    char *e;
    size_t len;
    if (*ctx.slot) {
      len = strlen(basename);
      snprintf(basename + len, FILENAME_SIZE - len, "slot%s-", ctx.slot);
    }
    len = strlen(basename);
    snprintf(basename + len, FILENAME_SIZE - len, "%s-", ctx.target);
    if ((e = strchr(basename + len, ':'))) *e = '_';
  }
  if (ctx.verbose > 1) {
    printf("target: %s\n", ctx.target);
    printf("basename: %s\n", ctx.basename);
    fflush(stdout);
  }

  if (allPoints) {
    snsrForEach(s, slotKey(ctx.slot, SNSR_OPERATING_POINT_LIST, keyBuff),
                snsrCallback(convertAllPoints, NULL, &ctx));
  } else {
    /* Very old models do not have support for operating points */
    if (snsrRC(s) != SNSR_RC_OK) fatal(snsrRC(s), snsrErrorDetail(s));
    snsrGetInt(s, slotKey(ctx.slot, SNSR_OPERATING_POINT, keyBuff), &ctx.point);
    snsrClearRC(s);
    writeEmbeddedFiles(s, &ctx, ctx.slot);
  }

  if (snsrRC(s) != SNSR_RC_OK) fatal(snsrRC(s), snsrErrorDetail(s));
  snsrRelease(s);
  snsrTearDown();
  return 0;
}
