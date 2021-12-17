// Copyright 2012-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_DETAIL_WIN_H
#define PUGL_DETAIL_WIN_H

#include "implementation.h"

#include "pugl/pugl.h"

#include <windows.h>

#include <stdbool.h>

typedef PIXELFORMATDESCRIPTOR PuglWinPFD;

struct PuglWorldInternalsImpl {
  double timerFrequency;
};

struct PuglInternalsImpl {
  PuglWinPFD   pfd;
  int          pfId;
  HWND         hwnd;
  HCURSOR      cursor;
  HDC          hdc;
  PuglSurface* surface;
  bool         flashing;
  bool         mouseTracked;
};

PUGL_API
PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints hints);

PUGL_API
PuglStatus
puglWinCreateWindow(PuglView* const   view,
                    const char* const title,
                    HWND* const       hwnd,
                    HDC* const        hdc);

PUGL_API
PuglStatus
puglWinConfigure(PuglView* view);

PUGL_API
PuglStatus
puglWinEnter(PuglView* view, const PuglExposeEvent* expose);

PUGL_API
PuglStatus
puglWinLeave(PuglView* view, const PuglExposeEvent* expose);

#endif // PUGL_DETAIL_WIN_H
