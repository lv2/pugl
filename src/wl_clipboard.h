// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_WL_CLIPBOARD_H
#define PUGL_WL_CLIPBOARD_H

#include <pugl/attributes.h>
#include <pugl/pugl.h>

#include <stdint.h>

PUGL_BEGIN_DECLS

struct wl_data_device;
struct wl_data_offer;
struct wl_data_source;

typedef struct {
  struct wl_data_source* source;
  struct wl_data_offer*  offer;
  PuglView*              receiver;
  PuglClipboard          clipboard;
  uint32_t               sourceActions;
  uint32_t               acceptedFormatIndex;
  uint32_t               numFormats;
  char**                 formats;
  uint32_t               dataSize;
  uint32_t               dataCapacity;
  uint8_t*               data;
  int                    pipeFd;
} PuglWaylandClipboard;

// Register listeners to act on incoming data (device) events
void
puglWaylandClipboardDeviceListen(PuglWorld*             world,
                                 struct wl_data_device* dataDevice);

// Register listeners to act on outgoing data (source) events
void
puglWaylandClipboardSourceListen(PuglWorld*             world,
                                 struct wl_data_source* dataSource);

// Request offered data to be sent in a particular format
// Returns a file descriptor for the read end of a pipe
int
puglWaylandClipboardRequestData(struct wl_data_offer* offer,
                                const char*           mimeType);

// Read available data and update and/or finish any pending data transfers
PuglStatus
puglWaylandClipboardRead(PuglWaylandClipboard* board, short revents);

PUGL_END_DECLS

#endif // PUGL_WL_CLIPBOARD_H
