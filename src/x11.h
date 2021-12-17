// Copyright 2012-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_DETAIL_X11_H
#define PUGL_DETAIL_X11_H

#include "types.h"

#include "pugl/pugl.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  Atom CLIPBOARD;
  Atom UTF8_STRING;
  Atom WM_PROTOCOLS;
  Atom WM_DELETE_WINDOW;
  Atom PUGL_CLIENT_MSG;
  Atom NET_WM_NAME;
  Atom NET_WM_STATE;
  Atom NET_WM_STATE_DEMANDS_ATTENTION;
  Atom NET_WM_STATE_HIDDEN;
} PuglX11Atoms;

typedef struct {
  XID       alarm;
  PuglView* view;
  uintptr_t id;
} PuglTimer;

struct PuglWorldInternalsImpl {
  Display*     display;
  PuglX11Atoms atoms;
  XIM          xim;
  PuglTimer*   timers;
  size_t       numTimers;
  XID          serverTimeCounter;
  int          syncEventBase;
  bool         syncSupported;
  bool         dispatchingEvents;
};

struct PuglInternalsImpl {
  Display*     display;
  XVisualInfo* vi;
  Window       win;
  XIC          xic;
  PuglSurface* surface;
  PuglEvent    pendingConfigure;
  PuglEvent    pendingExpose;
  int          screen;
#ifdef HAVE_XCURSOR
  unsigned cursorShape;
#endif
};

PUGL_API
PuglStatus
puglX11Configure(PuglView* view);

#endif // PUGL_DETAIL_X11_H
