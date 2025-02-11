// Copyright 2020-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "types.h"
#include "x11.h"

#include <pugl/pugl.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#ifndef USE_XCURSOR
#  if __has_include(<X11/Xcursor/Xcursor.h>)
#    define USE_XCURSOR 1
#  else
#    define USE_XCURSOR 0
#  endif
#endif

#if USE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#endif

#if USE_XCURSOR
static const char* const cursorNames[PUGL_NUM_CURSORS] = {
  "default",           // ARROW
  "text",              // CARET
  "crosshair",         // CROSSHAIR
  "pointer",           // HAND
  "not-allowed",       // NO
  "sb_h_double_arrow", // LEFT_RIGHT
  "sb_v_double_arrow", // UP_DOWN
  "size_fdiag",        // UP_LEFT_DOWN_RIGHT
  "size_bdiag",        // UP_RIGHT_DOWN_LEFT
  "all-scroll",        // ALL_SCROLL
};
#endif

#if USE_XCURSOR
static PuglStatus
defineCursorName(PuglView* const view, const char* const name)
{
  PuglInternals* const impl    = view->impl;
  PuglWorld* const     world   = view->world;
  Display* const       display = world->impl->display;

  // Load cursor theme
  const char* theme = XcursorGetTheme(display);
  if (!theme) {
    // If that fails (like on Linux Mint 21.1), "default" usually works
    theme = "default";
  }

  // Get the default size and cursor image from it
  const int           size  = XcursorGetDefaultSize(display);
  XcursorImage* const image = XcursorLibraryLoadImage(name, theme, size);
  if (!image) {
    return PUGL_BAD_PARAMETER;
  }

  // Load a cursor from the image
  const Cursor cur = XcursorImageLoadCursor(display, image);
  XcursorImageDestroy(image);
  if (!cur) {
    return PUGL_UNKNOWN_ERROR;
  }

  // Set the view's cursor to the new loaded one
  XDefineCursor(display, impl->win, cur);
  XFreeCursor(display, cur);
  return PUGL_SUCCESS;
}
#endif

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
#if USE_XCURSOR
  PuglInternals* const impl  = view->impl;
  const unsigned       index = (unsigned)cursor;
  const unsigned       count = sizeof(cursorNames) / sizeof(cursorNames[0]);
  if (index >= count) {
    return PUGL_BAD_PARAMETER;
  }

  const char* const name = cursorNames[index];
  if (!impl->win || impl->cursorName == name) {
    return PUGL_SUCCESS;
  }

  impl->cursorName = cursorNames[index];

  return defineCursorName(view, impl->cursorName);
#else
  (void)view;
  (void)cursor;
  return PUGL_UNSUPPORTED;
#endif
}
