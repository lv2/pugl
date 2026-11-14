// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WAYLAND_H
#define PUGL_SRC_WAYLAND_H

#include "types.h"
#include "xkb.h"

#include <pugl/pugl.h>

#include <stdint.h>

typedef int32_t wl_fixed_t;

struct PuglWorldInternalsImpl {
  struct wl_display*      display;
  struct wl_registry*     registry;
  struct wl_shm*          shm;
  struct wl_compositor*   compositor;
  struct wl_output*       output;
  struct wl_callback*     updateCallback;
  struct xdg_wm_base*     wmBase;
  struct wl_seat*         seat;
  struct wl_pointer*      pointer;
  struct wl_touch*        touch;
  struct wl_cursor_theme* cursorTheme;
  struct wl_keyboard*     keyboard;
  XkbKeyboard             xkb;
  int32_t                 keyRepeatRate;
  int32_t                 keyRepeatDelay;
  int32_t                 scaleFactor;
  int32_t                 refreshHz;
  PuglView*               enteredView;
  PuglView*               focusedView;
};

struct PuglInternalsImpl {
  struct wl_surface*      wlSurface;
  struct xdg_surface*     xdgSurface;
  struct xdg_toplevel*    toplevel;
  struct wl_surface*      cursorSurface;
  struct wl_cursor_image* cursorImage;
  PuglDamageEvent         damage;
  PuglSurface*            backendSurface;
  uint32_t                lastEnterSerial;
  int32_t                 pointerX;
  int32_t                 pointerY;
};

#endif // PUGL_SRC_WAYLAND_H
