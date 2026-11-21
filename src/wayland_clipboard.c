// Copyright 2022-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "wayland_clipboard.h"

#include "internal.h"
#include "macros.h"
#include "types.h"
#include "wayland.h"

#include <pugl/pugl.h>

#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include <poll.h>
#include <sys/types.h>
#include <unistd.h>

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_VERBOSE 1

#ifdef DEBUG_VERBOSE
#  include <stdio.h>
#  define DEBUG_LOG(m) (void)fprintf(stderr, "[wl] %s\n", m)
#  define DEBUG_LOGF(f, ...) (void)fprintf(stderr, "[wl] " f "\n", __VA_ARGS__)
#endif

struct wl_surface;

// Data Source

static void
onDataSourceTarget(void* const                  data,
                   struct wl_data_source* const wlDataSource,
                   const char* const            mimeType)
{
  (void)data;
  (void)wlDataSource;
  (void)mimeType;
  DEBUG_LOG("data source target");
}

static void
onDataSourceSend(void* const                  data,
                 struct wl_data_source* const wlDataSource,
                 const char* const            mimeType,
                 const int32_t                fd)
{
  (void)data;
  (void)wlDataSource;
  (void)mimeType;
  (void)fd;
  DEBUG_LOG("data source send");
}

static void
onDataSourceCancelled(void* const                  data,
                      struct wl_data_source* const wlDataSource)
{
  (void)data;
  (void)wlDataSource;
  DEBUG_LOG("data source cancelled");
}

static void
onDataSourceDndDropPerformed(void* const                  data,
                             struct wl_data_source* const wlDataSource)
{
  (void)data;
  (void)wlDataSource;
  DEBUG_LOG("data source dnd drop performed");
}

static void
onDataSourceDndFinished(void* const                  data,
                        struct wl_data_source* const wlDataSource)
{
  (void)data;
  (void)wlDataSource;
  DEBUG_LOG("data source dnd finished");
}

static void
onDataSourceAction(void* const                  data,
                   struct wl_data_source* const wlDataSource,
                   const uint32_t               dndAction)
{
  (void)data;
  (void)wlDataSource;
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
                 struct wl_data_offer* const wlDataOffer,
                 const char* const           mimeType)
{
  (void)wlDataOffer;
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
  }
}

static void
onDataOfferSourceActions(void* const                 data,
                         struct wl_data_offer* const wlDataOffer,
                         const uint32_t              sourceActions)
{
  (void)wlDataOffer;
  (void)sourceActions;

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
                  struct wl_data_offer* const wlDataOffer,
                  const uint32_t              dndAction)
{
  (void)data;
  (void)wlDataOffer;

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
                      struct wl_data_device* const wlDataDevice,
                      struct wl_data_offer* const  id)
{
  (void)wlDataDevice;
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
                  struct wl_data_device* const wlDataDevice,
                  const uint32_t               serial,
                  struct wl_surface* const     wlSurface,
                  const wl_fixed_t             fixedX,
                  const wl_fixed_t             fixedY,
                  struct wl_data_offer* const  id)
{
  (void)wlDataDevice;
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

  //  assert(!world->impl->dataView);
  world->impl->dataView = findView(world, wlSurface);
}

static void
onDataDeviceLeave(void* const data, struct wl_data_device* const wlDataDevice)
{
  (void)wlDataDevice;

  PuglWorld* const world = (PuglWorld*)data;

  assert(world->impl->dataView);
  // FIXME: ?
  /* world->impl->dataView = NULL; */

  DEBUG_LOG("data device leave");
}

static void
onDataDeviceMotion(void* const                  data,
                   struct wl_data_device* const wlDataDevice,
                   const uint32_t               time,
                   const wl_fixed_t             fixedX,
                   const wl_fixed_t             fixedY)
{
  (void)wlDataDevice;
  (void)time;

  PuglWorld* const world = (PuglWorld*)data;
  const double     x     = wl_fixed_to_double(fixedX);
  const double     y     = wl_fixed_to_double(fixedY);

  DEBUG_LOGF("data device drag motion %f %f", x, y);

  const PuglEvent offer = {
    .data = {
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
onDataDeviceDrop(void* const data, struct wl_data_device* const wlDataDevice)
{
  (void)data;
  (void)wlDataDevice;

  PuglWorld* const world = (PuglWorld*)data;
  assert(world->impl->dataView);

  DEBUG_LOG("data device drop");

  PuglView* const             view  = world->impl->dataView;
  PuglWaylandClipboard* const board = &view->world->impl->drag;
  fprintf(stderr, "Receiving drop...\n");
  if (board->pipeFd < 0) {
    board->pipeFd = puglWaylandClipboardRequestData(
      board->offer, board->formats[board->acceptedFormatIndex]);
  }

  wl_data_offer_finish(world->impl->dataOffer.offer);
  /* const PuglWaylandClipboard* const board = &view->world->impl->drag; */
  /* if (board->numFormats) { */
  /* const PuglEvent data = { */
  /*   .data = {PUGL_DATA_OFFER, 0U, 0.0, 0.0, 0.0, PUGL_CLIPBOARD_GENERAL}}; */
  /* puglDispatchEvent(view, &data); */
}

static void
onDataDeviceSelection(void* const                  data,
                      struct wl_data_device* const wlDataDevice,
                      struct wl_data_offer* const  id)
{
  (void)wlDataDevice;
  (void)id;

  PuglWorld* const world = (PuglWorld*)data;

  DEBUG_LOG("data device selection");

  // Data offer is the selection, move it to the general clipboard
  moveClipboardFormats(&world->impl->clipboard, &world->impl->dataOffer);
  world->impl->clipboard.offer = id;
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
puglWaylandClipboardListen(PuglWorld* const             world,
                           struct wl_data_device* const dataDevice,
                           struct wl_data_source* const dataSource)
{
  wl_data_device_add_listener(dataDevice, &dataDeviceListener, world);
  wl_data_source_add_listener(dataSource, &dataSourceListener, world);
}

void
puglWaylandClipboardUpdate(PuglWorld* const            world,
                           PuglWaylandClipboard* const board)
{
  static const unsigned chunkSize = 512U;

  if (board->pipeFd < 0) {
    return; // No data transfer in progress
  }

  const int     fd  = board->pipeFd;
  struct pollfd pfd = {fd, POLLIN, 0};
  const int     rc  = poll(&pfd, 1, 0);
  if (rc > 0 && (pfd.revents & POLLIN)) {
    while (true) {
      if (board->dataSize + chunkSize > board->dataCapacity) {
        const uint32_t newCapacity =
          MAX(chunkSize, board->dataCapacity + (board->dataCapacity / 2U));
        uint8_t* const newData = (uint8_t*)realloc(board->data, newCapacity);
        if (!newData) {
          close(fd);
          board->pipeFd = -1;
          return;
        }

        board->data         = newData;
        board->dataCapacity = newCapacity;
        memset(board->data + board->dataSize, 0, newCapacity - board->dataSize);
      }

      const ssize_t r = read(fd, board->data + board->dataSize, chunkSize);
      if (r < 0) { // Error
        close(fd);
        board->pipeFd = -1;
        return;
      }

      if (r == 0) { // EOF
        close(fd);
        board->pipeFd = -1;

        const PuglEvent data = {.data = {PUGL_DATA,
                                         0U,
                                         0.0,
                                         0.0,
                                         0.0,
                                         board->clipboard,
                                         board->acceptedFormatIndex}};
        puglDispatchEvent(world->impl->dataView, &data);
        return;
      }

      board->dataSize += r;
    }
  }
}

#undef DEBUG_LOGF
#undef DEBUG_LOG
