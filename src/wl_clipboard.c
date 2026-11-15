// Copyright 2022-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "wl_clipboard.h"

#include "internal.h"
#include "macros.h"
#include "types.h"
#include "wl.h"

#include <pugl/pugl.h>

#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include <poll.h>
#include <sys/types.h>
#include <unistd.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_VERBOSE 1

#ifdef DEBUG_VERBOSE
#  include <stdio.h>
#  define DEBUG_LOG(m) (void)fprintf(stderr, "[wl] %s\n", m)
#  define DEBUG_LOGF(f, ...) (void)fprintf(stderr, "[wl] " f "\n", __VA_ARGS__)
#endif

struct wl_surface;

// Chunk size in bytes for reading clipboard pipes
static const unsigned chunkSize = 512U;

// Data Source

static void
onDataSourceTarget(void* const                  data,
                   struct wl_data_source* const dataSource,
                   const char* const            mimeType)
{
  (void)data;
  (void)dataSource;
  (void)mimeType;
  DEBUG_LOG("data source target");
}

static void
onDataSourceSend(void* const                  data,
                 struct wl_data_source* const dataSource,
                 const char* const            mimeType,
                 const int32_t                fd)
{
  PuglWorld* const world = (PuglWorld*)data;

  PuglWaylandClipboard* const board =
    (dataSource == world->impl->clipboard.source) ? &world->impl->clipboard
    : (dataSource == world->impl->drag.source)    ? &world->impl->drag
                                                  : NULL;

  DEBUG_LOGF("data source send size %u", board ? board->dataSize : 0U);

  if (board && board->numFormats && !strcmp(mimeType, board->formats[0])) {
    for (size_t n = 0; n < board->dataSize;) {
      const ssize_t w = write(fd, board->data + n, board->dataSize - n);
      if (w < 0) {
        break;
      }

      n += (size_t)w;
    }
  }

  close(fd);
}

static void
onDataSourceCancelled(void* const data, struct wl_data_source* const dataSource)
{
  PuglWorld* const world = (PuglWorld*)data;

  DEBUG_LOG("data source cancelled");

  PuglWaylandClipboard* const board =
    (dataSource == world->impl->clipboard.source) ? &world->impl->clipboard
    : (dataSource == world->impl->drag.source)    ? &world->impl->drag
                                                  : NULL;

  if (board) {
    wl_data_source_destroy(dataSource);
    board->source = NULL;
  }
}

static void
onDataSourceDndDropPerformed(void* const                  data,
                             struct wl_data_source* const dataSource)
{
  (void)data;
  (void)dataSource;
  DEBUG_LOG("data source dnd drop performed");
}

static void
onDataSourceDndFinished(void* const                  data,
                        struct wl_data_source* const dataSource)
{
  (void)data;
  (void)dataSource;
  DEBUG_LOG("data source dnd finished");
}

static void
onDataSourceAction(void* const                  data,
                   struct wl_data_source* const dataSource,
                   const uint32_t               dndAction)
{
  (void)data;
  (void)dataSource;
  (void)dndAction;
  DEBUG_LOG("data source action");
}

static const struct wl_data_source_listener dataSourceListener = {
  onDataSourceTarget,
  onDataSourceSend,
  onDataSourceCancelled,
  onDataSourceDndDropPerformed,
  onDataSourceDndFinished,
  onDataSourceAction,
};

// Data Offer

static void
onDataOfferOffer(void* const                 data,
                 struct wl_data_offer* const dataOffer,
                 const char* const           mimeType)
{
  (void)dataOffer;
  (void)mimeType;

  PuglWorld* const            world = (PuglWorld*)data;
  PuglWaylandClipboard* const board = &world->impl->dataOffer;

  DEBUG_LOGF("data offer offer %s", mimeType);

  if (islower(mimeType[0])) { // Hopefully an actual MIME type
    const uint32_t newNumFormats = board->numFormats + 1U;
    char** const   newFormats =
      (char**)realloc(board->formats, newNumFormats * sizeof(char*));
    if (!newFormats) {
      return;
    }

    const size_t mimeTypeLen    = strlen(mimeType);
    char* const  mimeTypeString = (char*)calloc(mimeTypeLen + 1, 1);
    memcpy(mimeTypeString, mimeType, mimeTypeLen + 1);

    board->formats                    = newFormats;
    board->formats[board->numFormats] = mimeTypeString;
    board->numFormats                 = newNumFormats;
    world->impl->dataOffer.offer      = dataOffer;
  }
}

static void
onDataOfferSourceActions(void* const                 data,
                         struct wl_data_offer* const dataOffer,
                         const uint32_t              sourceActions)
{
  (void)dataOffer;

  PuglWorld* const world = (PuglWorld*)data;

  DEBUG_LOGF("data offer source actions %u", sourceActions);

  world->impl->dataOffer.sourceActions = sourceActions;

  if (sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) {
    DEBUG_LOG("data offer copy action");
  }
  if (sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) {
    DEBUG_LOG("data offer move action");
  }
}

static void
onDataOfferAction(void* const                 data,
                  struct wl_data_offer* const dataOffer,
                  const uint32_t              dndAction)
{
  (void)data;
  (void)dataOffer;

  DEBUG_LOGF("data offer action %u", dndAction);
}

static const struct wl_data_offer_listener dataOfferListener = {
  onDataOfferOffer,
  onDataOfferSourceActions,
  onDataOfferAction,
};

// Data Device

static void
onDataDeviceDataOffer(void* const                  data,
                      struct wl_data_device* const dataDevice,
                      struct wl_data_offer* const  id)
{
  (void)dataDevice;
  (void)id;

  PuglWorld* const world = (PuglWorld*)data;

  DEBUG_LOG("data device data offer");

  free(world->impl->dataOffer.formats);
  if (world->impl->dataOffer.offer) {
    wl_data_offer_destroy(world->impl->dataOffer.offer);
  }

  world->impl->dataOffer.offer      = id;
  world->impl->dataOffer.formats    = NULL;
  world->impl->dataOffer.numFormats = 0U;

  wl_data_offer_add_listener(id, &dataOfferListener, data);
}

static void
moveClipboardFormats(PuglWaylandClipboard* const dst,
                     PuglWaylandClipboard* const src)
{
  free(dst->formats);
  dst->formats    = src->formats;
  dst->numFormats = src->numFormats;
  src->formats    = NULL;
  src->numFormats = 0U;
}

static void
onDataDeviceEnter(void* const                  data,
                  struct wl_data_device* const dataDevice,
                  const uint32_t               serial,
                  struct wl_surface* const     wlSurface,
                  const wl_fixed_t             fixedX,
                  const wl_fixed_t             fixedY,
                  struct wl_data_offer* const  id)
{
  (void)dataDevice;
  (void)serial;
  (void)id;

  PuglWorld* const world = (PuglWorld*)data;
  const double     x     = wl_fixed_to_double(fixedX);
  const double     y     = wl_fixed_to_double(fixedY);

  DEBUG_LOGF("data device enter %f %f", x, y);

  // Data offer is a drag, move it to the drag clipboard
  moveClipboardFormats(&world->impl->drag, &world->impl->dataOffer);
  world->impl->drag.offer = id;
  world->impl->dragSerial = serial;
  world->impl->dataView   = findView(world, wlSurface);
}

static void
onDataDeviceLeave(void* const data, struct wl_data_device* const dataDevice)
{
  (void)dataDevice;

  PuglWorld* const world = (PuglWorld*)data;

  assert(world->impl->dataView);
  world->impl->dataView = NULL;

  DEBUG_LOG("data device leave");
}

static void
onDataDeviceMotion(void* const                  data,
                   struct wl_data_device* const dataDevice,
                   const uint32_t               time,
                   const wl_fixed_t             fixedX,
                   const wl_fixed_t             fixedY)
{
  (void)dataDevice;
  (void)time;

  PuglWorld* const world = (PuglWorld*)data;
  const double     x     = wl_fixed_to_double(fixedX);
  const double     y     = wl_fixed_to_double(fixedY);

  DEBUG_LOGF("data device drag motion %f %f", x, y);

  const PuglEvent offer = {
    .offer = {
      PUGL_DATA_OFFER, 0U, puglGetTime(world), x, y, PUGL_CLIPBOARD_DRAG}};
  puglDispatchEvent(world->impl->dataView, &offer);
}

int
puglWaylandClipboardRequestData(struct wl_data_offer* const offer,
                                const char* const           mimeType)
{
  int fds[2] = {0, 0};
  pipe(fds);
  wl_data_offer_receive(offer, mimeType, fds[1]);
  close(fds[1]);
  return fds[0];
}

static void
onDataDeviceDrop(void* const data, struct wl_data_device* const dataDevice)
{
  (void)data;
  (void)dataDevice;

  PuglWorld* const world = (PuglWorld*)data;
  assert(world->impl->dataView);

  DEBUG_LOG("data device drop");

  PuglView* const             view  = world->impl->dataView;
  PuglWaylandClipboard* const board = &view->world->impl->drag;
  if (board->pipeFd < 0) {
    board->receiver = view;
    board->pipeFd   = puglWaylandClipboardRequestData(
      board->offer, board->formats[board->acceptedFormatIndex]);
  }
}

static void
onDataDeviceSelection(void* const                  data,
                      struct wl_data_device* const dataDevice,
                      struct wl_data_offer* const  id)
{
  (void)dataDevice;
  (void)id;

  PuglWorld* const world = (PuglWorld*)data;

  DEBUG_LOGF("data device selection, offer %p, existing %p",
             (void*)id,
             (void*)world->impl->clipboard.offer);

  if (id != world->impl->clipboard.offer && !world->impl->clipboard.source) {
    // Data offer is the selection, move it to the general clipboard
    moveClipboardFormats(&world->impl->clipboard, &world->impl->dataOffer);
    world->impl->clipboard.offer    = id;
    world->impl->clipboard.receiver = world->impl->dataView;
    world->impl->clipboard.dataSize = 0U;
  }
}

static const struct wl_data_device_listener dataDeviceListener = {
  onDataDeviceDataOffer,
  onDataDeviceEnter,
  onDataDeviceLeave,
  onDataDeviceMotion,
  onDataDeviceDrop,
  onDataDeviceSelection,
};

void
puglWaylandClipboardDeviceListen(PuglWorld* const             world,
                                 struct wl_data_device* const dataDevice)
{
  wl_data_device_add_listener(dataDevice, &dataDeviceListener, world);
}

void
puglWaylandClipboardSourceListen(PuglWorld* const             world,
                                 struct wl_data_source* const dataSource)
{
  wl_data_source_add_listener(dataSource, &dataSourceListener, world);
}

static void
closeBoardPipe(PuglWaylandClipboard* const board)
{
  if (board->pipeFd >= 0) {
    close(board->pipeFd);
    board->pipeFd = -1;
  }
}

static ssize_t
readChunk(PuglWaylandClipboard* const board)
{
  if (board->dataSize + chunkSize > board->dataCapacity) {
    const uint32_t newCapacity =
      MAX(chunkSize, board->dataCapacity + (board->dataCapacity / 2U));
    void* const newData = realloc(board->data, newCapacity);
    if (!newData) {
      return -1;
    }

    board->data         = (uint8_t*)newData;
    board->dataCapacity = newCapacity;
    memset(board->data + board->dataSize, 0, newCapacity - board->dataSize);
  }

  return read(board->pipeFd, board->data + board->dataSize, chunkSize);
}

PuglStatus
puglWaylandClipboardRead(PuglWaylandClipboard* const board, const short revents)
{
  if (revents & POLLIN) {
    const ssize_t r = readChunk(board);
    if (r < 0) { // Error
      closeBoardPipe(board);
      return PUGL_UNKNOWN_ERROR;
    }

    board->dataSize += r;
  }

  if (revents & POLLHUP) {
    closeBoardPipe(board);

    const PuglEvent data = {.data = {PUGL_DATA,
                                     0U,
                                     0.0,
                                     0.0,
                                     0.0,
                                     board->clipboard,
                                     board->acceptedFormatIndex}};
    return puglDispatchEvent(board->receiver, &data);
  }

  return PUGL_SUCCESS;
}

#undef DEBUG_LOGF
#undef DEBUG_LOG
