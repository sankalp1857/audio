/* Sensory Confidential
 * Copyright (C)2018-2025 Sensory, Inc. https://sensory.com/
 *
 * TrulyHandsfree SDK example of a custom stream, such as an audio stream
 * to get input from a custom audio driver (in a RTOS for example).
 * Similar streams are in samples wmme-stream.c and alsa-stream.c.
 *
 * NOTE: Normally it's best to use snsrStreamFromMemory(..) for a
 * stream of data, although this example would also work.
 *-----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>

#include <snsr.h>

typedef struct {
  char *data;
  size_t dataSize;
  size_t index;
} ProviderData;


static SnsrRC
streamOpen(SnsrStream stream)
{
  ProviderData *instanceData = snsrStream_getData(stream);
  if (!instanceData->data) return SNSR_RC_ERROR;
  instanceData->index = 0;
  return SNSR_RC_OK;
}


static SnsrRC
streamClose(SnsrStream stream)
{
  ProviderData *instanceData = snsrStream_getData(stream);
  instanceData->index = 0;
  return SNSR_RC_OK;
}


static void
streamRelease(SnsrStream stream)
{
  ProviderData *instanceData = snsrStream_getData(stream);
  free(instanceData);
}


static size_t
streamRead(SnsrStream stream, void *buffer, size_t readSize)
{
  /* NOTE: For a live audio stream, if there is no data available,
   * this call should block until there is more data available.
   */
  ProviderData *d = snsrStream_getData(stream);
  size_t available, read;

  read = readSize;
  available = d->dataSize - d->index;
  if (read > available) {
    read = available;
    /* Session will end with SNSR_RC_STREAM_END */
    snsrStream_setRC(stream, SNSR_RC_EOF);
  }
  if (read) memcpy(buffer, d->data + d->index, read);
  d->index += read;
  return read;
}


static size_t
streamWrite(SnsrStream stream, const void *buffer, size_t writeSize)
{
  /* NOTE: For a live audio stream, implementing a streamWrite
   * would make no sense and this function should be removed.
   */
  ProviderData *d = snsrStream_getData(stream);
  size_t available, written;

  written = writeSize;
  available = d->dataSize - d->index;
  if (written > available) {
    written = available;
    /* Session will end with SNSR_RC_STREAM_END */
    snsrStream_setRC(stream, SNSR_RC_EOF);
  }
  if (written) memcpy(d->data + d->index, buffer, written);
  d->index += written;
  return written;
}


/* This is the interface any SnsrStream has to provide
 * (Virtual Method Table)
 */
static SnsrStream_Vmt methods = {
  "data", &streamOpen, &streamClose, &streamRelease, &streamRead, &streamWrite
};


/* This is the 'constructor' for this kind of stream */
SnsrStream
streamFromData(void *data, size_t dataSize, SnsrStreamMode mode)
{
  SnsrStream dataStream;
  int readable = (mode == SNSR_ST_MODE_READ);
  int writeable = (mode == SNSR_ST_MODE_WRITE);
  /* The stream object has instance data (particular to this instance)
   * and virtual method pointers (particular to this type)
   * just like it would in C++
   */
  ProviderData *instanceData = malloc(sizeof(*instanceData));
  memset(instanceData, 0, sizeof(*instanceData));
  instanceData->data = data;
  instanceData->dataSize = dataSize;

  dataStream = snsrStream_alloc(&methods, instanceData, readable, writeable);
  if (data == NULL) snsrStream_setRC(dataStream, SNSR_RC_INVALID_ARG);
  return dataStream;
}
