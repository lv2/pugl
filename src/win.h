/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef PUGL_DETAIL_WIN_H
#define PUGL_DETAIL_WIN_H

#include "implementation.h"

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

static inline PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints hints)
{
  const int rgbBits = (hints[PUGL_RED_BITS] +   //
                       hints[PUGL_GREEN_BITS] + //
                       hints[PUGL_BLUE_BITS]);

  const DWORD dwFlags = hints[PUGL_DOUBLE_BUFFER] ? PFD_DOUBLEBUFFER : 0u;

  PuglWinPFD pfd;
  ZeroMemory(&pfd, sizeof(pfd));
  pfd.nSize        = sizeof(pfd);
  pfd.nVersion     = 1;
  pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | dwFlags;
  pfd.iPixelType   = PFD_TYPE_RGBA;
  pfd.cColorBits   = (BYTE)rgbBits;
  pfd.cRedBits     = (BYTE)hints[PUGL_RED_BITS];
  pfd.cGreenBits   = (BYTE)hints[PUGL_GREEN_BITS];
  pfd.cBlueBits    = (BYTE)hints[PUGL_BLUE_BITS];
  pfd.cAlphaBits   = (BYTE)hints[PUGL_ALPHA_BITS];
  pfd.cDepthBits   = (BYTE)hints[PUGL_DEPTH_BITS];
  pfd.cStencilBits = (BYTE)hints[PUGL_STENCIL_BITS];
  pfd.iLayerType   = PFD_MAIN_PLANE;
  return pfd;
}

static inline unsigned
puglWinGetWindowFlags(const PuglView* const view)
{
  const bool     resizable = view->hints[PUGL_RESIZABLE];
  const unsigned sizeFlags = resizable ? (WS_SIZEBOX | WS_MAXIMIZEBOX) : 0u;

  return (WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
          (view->parent
             ? WS_CHILD
             : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | sizeFlags)));
}

static inline unsigned
puglWinGetWindowExFlags(const PuglView* const view)
{
  return WS_EX_NOINHERITLAYOUT | (view->parent ? 0u : WS_EX_APPWINDOW);
}

static inline PuglStatus
puglWinCreateWindow(PuglView* const   view,
                    const char* const title,
                    HWND* const       hwnd,
                    HDC* const        hdc)
{
  const char*    className  = (const char*)view->world->className;
  const unsigned winFlags   = puglWinGetWindowFlags(view);
  const unsigned winExFlags = puglWinGetWindowExFlags(view);

  if (view->frame.width == 0.0 && view->frame.height == 0.0) {
    if (view->defaultWidth == 0.0 && view->defaultHeight == 0.0) {
      return PUGL_BAD_CONFIGURATION;
    }

    RECT desktopRect;
    GetClientRect(GetDesktopWindow(), &desktopRect);

    const int screenWidth  = desktopRect.right - desktopRect.left;
    const int screenHeight = desktopRect.bottom - desktopRect.top;

    view->frame.width  = view->defaultWidth;
    view->frame.height = view->defaultHeight;
    view->frame.x      = screenWidth / 2.0 - view->frame.width / 2.0;
    view->frame.y      = screenHeight / 2.0 - view->frame.height / 2.0;
  }

  // The meaning of "parent" depends on the window type (WS_CHILD)
  PuglNativeView parent = view->parent ? view->parent : view->transientParent;

  // Calculate total window size to accommodate requested view size
  RECT wr = {(long)view->frame.x,
             (long)view->frame.y,
             (long)view->frame.width,
             (long)view->frame.height};
  AdjustWindowRectEx(&wr, winFlags, FALSE, winExFlags);

  // Create window and get drawing context
  if (!(*hwnd = CreateWindowEx(winExFlags,
                               className,
                               title,
                               winFlags,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               wr.right - wr.left,
                               wr.bottom - wr.top,
                               (HWND)parent,
                               NULL,
                               NULL,
                               NULL))) {
    return PUGL_REALIZE_FAILED;
  } else if (!(*hdc = GetDC(*hwnd))) {
    DestroyWindow(*hwnd);
    *hwnd = NULL;
    return PUGL_REALIZE_FAILED;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglWinStubConfigure(PuglView* view);

PuglStatus
puglWinStubEnter(PuglView* view, const PuglEventExpose* expose);

PuglStatus
puglWinStubLeave(PuglView* view, const PuglEventExpose* expose);

#endif // PUGL_DETAIL_WIN_H
