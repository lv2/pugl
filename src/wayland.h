// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WAYLAND_H
#define PUGL_SRC_WAYLAND_H

#include "types.h"

// #include "xdg-shell.h"

#include <pugl/pugl.h>

/* #include <wayland-client-core.h> */
#include <wayland-util.h>
/* #include <xkbcommon/xkbcommon.h> */

#include <stdint.h>

/* struct wl_compositor; */
/* struct wl_cursor_image; */
/* struct wl_cursor_theme; */
/* struct wl_display; */
/* struct wl_keyboard; */
/* struct wl_pointer; */
/* struct wl_registry; */
/* struct wl_seat; */
/* struct wl_shm; */
/* struct wl_surface; */
/* struct xdg_surface; */
/* struct xdg_toplevel; */
/* struct xdg_wm_base; */
/* struct xkb_context; */
/* struct xkb_keymap; */
/* struct xkb_state; */

struct PuglWorldInternalsImpl {
  struct wl_display*      display;
  struct wl_registry*     registry;
  struct wl_shm*          shm;
  struct wl_compositor*   compositor;
  struct xdg_wm_base*     wmBase;
  struct wl_seat*         seat;
  struct wl_pointer*      pointer;
  struct wl_cursor_theme* cursorTheme;
  struct wl_keyboard*     keyboard;
  struct xkb_state*       xkbState;
  struct xkb_context*     xkbContext;
  struct xkb_keymap*      xkbKeymap;
  uint32_t                shiftModMask;
  uint32_t                ctrlModMask;
  uint32_t                altModMask;
  uint32_t                superModMask;
  PuglMods                depressedMods;
  PuglView*               enteredView;
  PuglView*               focusedView;
};

struct PuglInternalsImpl {
  struct wl_surface*      wlSurface;
  struct xdg_surface*     xdgSurface;
  struct xdg_toplevel*    toplevel;
  struct wl_surface*      cursorSurface;
  struct wl_cursor_image* cursorImage;
  PuglSurface*            backendSurface;
  uint32_t                lastEnterSerial;
  wl_fixed_t              pointerX;
  wl_fixed_t              pointerY;
};

#endif // PUGL_SRC_WAYLAND_H
