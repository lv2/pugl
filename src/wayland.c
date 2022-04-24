// Copyright 2022-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "wayland.h"

#include "attributes.h"
#include "internal.h"
#include "platform.h"
#include "types.h"
#include "xdg-shell.h"

#include <pugl/pugl.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include <sys/mman.h>
#include <unistd.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <stdio.h>

#ifndef BTN_LEFT
#  define BTN_LEFT 0x110
#endif

struct wl_callback;

static PuglView*
findView(PuglWorld* const world, const struct wl_surface* const wlSurface)
{
  for (size_t i = 0; i < world->numViews; ++i) {
    if (world->views[i]->impl->wlSurface == wlSurface) {
      return world->views[i];
    }
  }

  return NULL;
}

// WM Base

static void
onWmBasePing(void* const               PUGL_UNUSED(data),
             struct xdg_wm_base* const xdg_wm_base,
             const uint32_t            serial)
{
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
  onWmBasePing,
};

// Pointer

static void
onPointerEnter(void* const              data,
               struct wl_pointer* const pointer,
               const uint32_t           serial,
               struct wl_surface* const wlSurface,
               const wl_fixed_t         surfaceX,
               const wl_fixed_t         surfaceY)
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = findView(world, wlSurface);
  if (!view) {
    return;
  }

  assert(!world->impl->enteredView);
  world->impl->enteredView = view;
  view->impl->pointerX     = surfaceX;
  view->impl->pointerY     = surfaceY;

  wl_pointer_set_cursor(pointer,
                        serial,
                        view->impl->cursorSurface,
                        (int32_t)view->impl->cursorImage->hotspot_x,
                        (int32_t)view->impl->cursorImage->hotspot_y);

  PuglEvent event     = {{PUGL_POINTER_IN, 0}};
  event.crossing.x    = wl_fixed_to_double(surfaceX);
  event.crossing.y    = wl_fixed_to_double(surfaceY);
  event.crossing.mode = PUGL_CROSSING_NORMAL;
  puglDispatchEvent(view, &event);
}

static void
onPointerLeave(void* const              data,
               struct wl_pointer* const PUGL_UNUSED(pointer),
               const uint32_t           PUGL_UNUSED(serial),
               struct wl_surface* const surface)
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = findView(world, surface);
  if (!view) {
    return;
  }

  assert(world->impl->enteredView);
  assert(world->impl->enteredView->impl->wlSurface == surface);
  world->impl->enteredView = NULL;

  PuglEvent event     = {{PUGL_POINTER_OUT, 0}};
  event.crossing.x    = wl_fixed_to_double(view->impl->pointerX);
  event.crossing.y    = wl_fixed_to_double(view->impl->pointerY);
  event.crossing.mode = PUGL_CROSSING_NORMAL;
  puglDispatchEvent(view, &event);
}

static void
onPointerMotion(void* const              data,
                struct wl_pointer* const PUGL_UNUSED(pointer),
                const uint32_t           time,
                const wl_fixed_t         surfaceX,
                const wl_fixed_t         surfaceY)
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = world->impl->enteredView;
  if (!view) {
    return;
  }

  view->impl->pointerX = surfaceX;
  view->impl->pointerY = surfaceY;

  PuglEvent event    = {{PUGL_MOTION, 0}};
  event.motion.time  = (double)time / 1e3;
  event.motion.x     = wl_fixed_to_double(view->impl->pointerX);
  event.motion.y     = wl_fixed_to_double(view->impl->pointerY);
  event.motion.state = world->impl->depressedMods;
  puglDispatchEvent(view, &event);
}

#if 0
static enum xdg_toplevel_resize_edge
componentEdge(const int width,
               const int height,
               const int pointerX,
               const int pointerY,
               const int margin)
{
  const bool t = pointerY < margin;
  const bool b = pointerY > (height - margin);
  const bool l = pointerX < margin;
  const bool r = pointerX > (width - margin);

  return t   ? (l   ? XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT
                : r ? XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT
                    : XDG_TOPLEVEL_RESIZE_EDGE_TOP)
         : b ? (l   ? XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT
                : r ? XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT
                    : XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM)
         : l ? XDG_TOPLEVEL_RESIZE_EDGE_LEFT
         : r ? XDG_TOPLEVEL_RESIZE_EDGE_RIGHT
             : XDG_TOPLEVEL_RESIZE_EDGE_NONE;
}
#endif

static void
onPointerButton(void* const              data,
                struct wl_pointer* const PUGL_UNUSED(pointer),
                const uint32_t           PUGL_UNUSED(serial),
                const uint32_t           time,
                const uint32_t           button,
                const uint32_t           state)
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = world->impl->enteredView;
  if (!view) {
    return;
  }

  PuglEvent event = {{state == WL_POINTER_BUTTON_STATE_PRESSED
                        ? PUGL_BUTTON_PRESS
                        : PUGL_BUTTON_RELEASE,
                      0}};

  event.button.time   = (double)time / 1e3;
  event.button.x      = wl_fixed_to_double(view->impl->pointerX);
  event.button.y      = wl_fixed_to_double(view->impl->pointerY);
  event.button.state  = world->impl->depressedMods;
  event.button.button = button - BTN_LEFT;
  puglDispatchEvent(view, &event);
}

static void
onPointerAxis(void* const              data,
              struct wl_pointer* const PUGL_UNUSED(pointer),
              const uint32_t           time,
              const uint32_t           axis,
              const wl_fixed_t         value)
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = world->impl->enteredView;
  if (!view) {
    return;
  }

  PuglEvent event    = {{PUGL_SCROLL, 0}};
  event.scroll.time  = (double)time / 1e3;
  event.scroll.x     = wl_fixed_to_double(view->impl->pointerX);
  event.scroll.y     = wl_fixed_to_double(view->impl->pointerY);
  event.scroll.state = world->impl->depressedMods;

  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    event.scroll.dy = wl_fixed_to_double(value);
    event.scroll.direction =
      event.scroll.dy > 0.0 ? PUGL_SCROLL_DOWN : PUGL_SCROLL_UP;
  } else {
    event.scroll.dx = wl_fixed_to_double(value);
    event.scroll.direction =
      event.scroll.dx > 0.0 ? PUGL_SCROLL_RIGHT : PUGL_SCROLL_LEFT;
  }

  puglDispatchEvent(view, &event);
}

static void
onPointerFrame(void* const              PUGL_UNUSED(data),
               struct wl_pointer* const PUGL_UNUSED(pointer))
{}

static void
onPointerAxisSource(void* const              PUGL_UNUSED(data),
                    struct wl_pointer* const PUGL_UNUSED(pointer),
                    uint32_t const           PUGL_UNUSED(axis_source))
{}

static void
onPointerAxisStop(void* const              PUGL_UNUSED(data),
                  struct wl_pointer* const PUGL_UNUSED(pointer),
                  const uint32_t           PUGL_UNUSED(time),
                  const uint32_t           PUGL_UNUSED(axis))
{}

static void
onPointerAxisDiscrete(void* const              PUGL_UNUSED(data),
                      struct wl_pointer* const PUGL_UNUSED(pointer),
                      const uint32_t           PUGL_UNUSED(axis),
                      const int32_t            PUGL_UNUSED(discrete))
{}

static void
onPointerAxisValue120(void* const              PUGL_UNUSED(data),
                      struct wl_pointer* const PUGL_UNUSED(wl_pointer),
                      const uint32_t           PUGL_UNUSED(axis),
                      const int32_t            PUGL_UNUSED(value120))
{}

static void
onPointerAxisRelativeDirection(void* const              PUGL_UNUSED(data),
                               struct wl_pointer* const PUGL_UNUSED(wl_pointer),
                               const uint32_t           PUGL_UNUSED(axis),
                               const uint32_t           PUGL_UNUSED(direction))
{}

static const struct wl_pointer_listener wl_pointer_listener = {
  onPointerEnter,
  onPointerLeave,
  onPointerMotion,
  onPointerButton,
  onPointerAxis,
  onPointerFrame,
  onPointerAxisSource,
  onPointerAxisStop,
  onPointerAxisDiscrete,
  onPointerAxisValue120,
  onPointerAxisRelativeDirection,
};

// Keyboard

static void
onKeyboardKeymap(void* const               data,
                 struct wl_keyboard* const PUGL_UNUSED(keyboard),
                 const uint32_t            format,
                 const int32_t             fd,
                 const uint32_t            size)
{
  PuglWorld* const          world = (PuglWorld*)data;
  PuglWorldInternals* const impl  = world->impl;

  assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
  (void)format;

  if (impl->xkbState) {
    xkb_state_unref(impl->xkbState);
    impl->xkbState = NULL;
  }

  if (impl->xkbKeymap) {
    xkb_keymap_unref(impl->xkbKeymap);
    impl->xkbKeymap = NULL;
  }

  // Map the keymap file and parse it
  char* const mapString = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (mapString != MAP_FAILED) {
    impl->xkbKeymap = xkb_keymap_new_from_string(impl->xkbContext,
                                                 mapString,
                                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                                 XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(mapString, size);
  }

  close(fd);

  if (impl->xkbKeymap) {
    impl->xkbState = xkb_state_new(impl->xkbKeymap);

    // Build masks for each modifier to quickly build PuglMods on key presses
    const xkb_mod_index_t numMods = xkb_keymap_num_mods(impl->xkbKeymap);
    for (xkb_mod_index_t i = 0; i < numMods; ++i) {
      const char* modName = xkb_keymap_mod_get_name(impl->xkbKeymap, i);
      if (!strcmp(modName, "Shift")) {
        impl->shiftModMask |= (1U << i);
      } else if (!strcmp(modName, "Control") || !strcmp(modName, "LControl") ||
                 !strcmp(modName, "RControl")) {
        impl->ctrlModMask |= (1U << i);
      } else if (!strcmp(modName, "Alt") || !strcmp(modName, "LAlt") ||
                 !strcmp(modName, "RAlt") || !strcmp(modName, "Meta")) {
        impl->altModMask |= (1U << i);
      } else if (!strcmp(modName, "Super")) {
        impl->superModMask |= (1U << i);
      }
    }
  }
}

static void
onKeyboardEnter(void* const               data,
                struct wl_keyboard* const PUGL_UNUSED(keyboard),
                const uint32_t            serial,
                struct wl_surface* const  wlSurface,
                struct wl_array* const    PUGL_UNUSED(keys))
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = findView(world, wlSurface);
  if (!view) {
    return;
  }

  assert(!world->impl->focusedView);
  world->impl->focusedView    = view;
  view->impl->lastEnterSerial = serial;

  const PuglEvent event = {.focus = {PUGL_FOCUS_IN, 0U, PUGL_CROSSING_NORMAL}};
  puglDispatchEvent(view, &event);
}

static void
onKeyboardLeave(void* const               data,
                struct wl_keyboard* const PUGL_UNUSED(keyboard),
                const uint32_t            PUGL_UNUSED(serial),
                struct wl_surface* const  wlSurface)
{
  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = findView(world, wlSurface);
  if (!view) {
    return;
  }

  assert(world->impl->focusedView);
  assert(world->impl->focusedView->impl->wlSurface == wlSurface);
  world->impl->focusedView    = NULL;
  view->impl->lastEnterSerial = 0;

  const PuglEvent event = {.focus = {PUGL_FOCUS_OUT, 0U, PUGL_CROSSING_NORMAL}};
  puglDispatchEvent(view, &event);
}

static void
onKeyboardModifiers(void* const               data,
                    struct wl_keyboard* const PUGL_UNUSED(keyboard),
                    const uint32_t            PUGL_UNUSED(serial),
                    const uint32_t            modsDepressed,
                    const uint32_t            modsLatched,
                    const uint32_t            modsLocked,
                    const uint32_t            group)
{
  PuglWorld* const world = (PuglWorld*)data;

  xkb_state_update_mask(
    world->impl->xkbState, modsDepressed, modsLatched, modsLocked, 0, 0, group);

  world->impl->depressedMods =
    ((modsDepressed & world->impl->shiftModMask) ? PUGL_MOD_SHIFT : 0U) |
    ((modsDepressed & world->impl->ctrlModMask) ? PUGL_MOD_CTRL : 0U) |
    ((modsDepressed & world->impl->altModMask) ? PUGL_MOD_ALT : 0U) |
    ((modsDepressed & world->impl->superModMask) ? PUGL_MOD_SUPER : 0U);
}

static void
onKeyboardRepeatInfo(void* const               PUGL_UNUSED(data),
                     struct wl_keyboard* const PUGL_UNUSED(keyboard),
                     const int32_t             PUGL_UNUSED(rate),
                     const int32_t             PUGL_UNUSED(delay))
{
  fprintf(stderr, "[wl] keyboard repeat info\n");
}

static PuglKey
keysymToSpecial(const xkb_keysym_t sym)
{
  // clang-format off
  switch (sym) {
  case XKB_KEY_F1:               return PUGL_KEY_F1;
  case XKB_KEY_F2:               return PUGL_KEY_F2;
  case XKB_KEY_F3:               return PUGL_KEY_F3;
  case XKB_KEY_F4:               return PUGL_KEY_F4;
  case XKB_KEY_F5:               return PUGL_KEY_F5;
  case XKB_KEY_F6:               return PUGL_KEY_F6;
  case XKB_KEY_F7:               return PUGL_KEY_F7;
  case XKB_KEY_F8:               return PUGL_KEY_F8;
  case XKB_KEY_F9:               return PUGL_KEY_F9;
  case XKB_KEY_F10:              return PUGL_KEY_F10;
  case XKB_KEY_F11:              return PUGL_KEY_F11;
  case XKB_KEY_F12:              return PUGL_KEY_F12;
  case XKB_KEY_Left:             return PUGL_KEY_LEFT;
  case XKB_KEY_Up:               return PUGL_KEY_UP;
  case XKB_KEY_Right:            return PUGL_KEY_RIGHT;
  case XKB_KEY_Down:             return PUGL_KEY_DOWN;
  case XKB_KEY_Page_Up:          return PUGL_KEY_PAGE_UP;
  case XKB_KEY_Page_Down:        return PUGL_KEY_PAGE_DOWN;
  case XKB_KEY_Home:             return PUGL_KEY_HOME;
  case XKB_KEY_End:              return PUGL_KEY_END;
  case XKB_KEY_Insert:           return PUGL_KEY_INSERT;
  case XKB_KEY_Shift_L:          return PUGL_KEY_SHIFT_L;
  case XKB_KEY_Shift_R:          return PUGL_KEY_SHIFT_R;
  case XKB_KEY_Control_L:        return PUGL_KEY_CTRL_L;
  case XKB_KEY_Control_R:        return PUGL_KEY_CTRL_R;
  case XKB_KEY_Alt_L:            return PUGL_KEY_ALT_L;
  case XKB_KEY_ISO_Level3_Shift:
  case XKB_KEY_Alt_R:            return PUGL_KEY_ALT_R;
  case XKB_KEY_Super_L:          return PUGL_KEY_SUPER_L;
  case XKB_KEY_Super_R:          return PUGL_KEY_SUPER_R;
  case XKB_KEY_Menu:             return PUGL_KEY_MENU;
  case XKB_KEY_Caps_Lock:        return PUGL_KEY_CAPS_LOCK;
  case XKB_KEY_Scroll_Lock:      return PUGL_KEY_SCROLL_LOCK;
  case XKB_KEY_Num_Lock:         return PUGL_KEY_NUM_LOCK;
  case XKB_KEY_Print:            return PUGL_KEY_PRINT_SCREEN;
  case XKB_KEY_Pause:            return PUGL_KEY_PAUSE;
  default:                       break;
  }
  // clang-format on

  return (PuglKey)0;
}

static void
onKeyboardKey(void* const               data,
              struct wl_keyboard* const PUGL_UNUSED(keyboard),
              const uint32_t            PUGL_UNUSED(serial),
              const uint32_t            time,
              const uint32_t            key,
              const uint32_t            state)
{
  PuglWorld* const        world    = (PuglWorld*)data;
  struct xkb_state* const xkbState = world->impl->xkbState;
  PuglView* const         view     = world->impl->focusedView;
  if (!view) {
    return;
  }

  const uint32_t     keycode = key + 8U;
  const xkb_keysym_t sym     = xkb_state_key_get_one_sym(xkbState, keycode);
  const PuglKey      special = keysymToSpecial(sym);
  const bool         isPress = state == WL_KEYBOARD_KEY_STATE_PRESSED;

  const PuglEvent keyEvent = {
    .key = {isPress ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE,
            0U,
            (double)time / 1e3,
            0.0,
            0.0,
            0.0,
            0.0,
            world->impl->depressedMods,
            special ? special : keycode,
            xkb_state_key_get_utf32(xkbState, keycode)}};

  puglDispatchEvent(view, &keyEvent);

  if (isPress && xkb_state_key_get_utf8(xkbState, keycode, NULL, 0)) {
    PuglEvent textEvent      = {{PUGL_TEXT, 0U}};
    textEvent.text.keycode   = keycode;
    textEvent.text.character = xkb_state_key_get_utf32(xkbState, keycode);

    xkb_state_key_get_utf8(
      xkbState, keycode, textEvent.text.string, sizeof(textEvent.text.string));

    puglDispatchEvent(view, &textEvent);
  }
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
  onKeyboardKeymap,
  onKeyboardEnter,
  onKeyboardLeave,
  onKeyboardKey,
  onKeyboardModifiers,
  onKeyboardRepeatInfo,
};

// Seat

static void
onSeatCapabilities(void* const           data,
                   struct wl_seat* const PUGL_UNUSED(seat),
                   const uint32_t        capabilities)
{
  PuglWorld* const          world = (PuglWorld*)data;
  PuglWorldInternals* const impl  = world->impl;

  const bool hasPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
  if (hasPointer && !impl->pointer) {
    impl->pointer = wl_seat_get_pointer(impl->seat);
    wl_pointer_add_listener(impl->pointer, &wl_pointer_listener, world);
  } else if (!hasPointer && impl->pointer) {
    wl_pointer_release(impl->pointer);
    impl->pointer = NULL;
  }

  const bool hasKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
  if (hasKeyboard && !impl->keyboard) {
    impl->keyboard = wl_seat_get_keyboard(impl->seat);
    wl_keyboard_add_listener(impl->keyboard, &wl_keyboard_listener, world);
  } else if (!hasKeyboard && impl->keyboard) {
    wl_keyboard_release(impl->keyboard);
    impl->keyboard = NULL;
  }
}

static void
onSeatName(void* const           PUGL_UNUSED(data),
           struct wl_seat* const PUGL_UNUSED(wl_seat),
           const char* const     PUGL_UNUSED(name))
{}

static const struct wl_seat_listener wl_seat_listener = {
  onSeatCapabilities,
  onSeatName,
};

// Registry

static void
registry_global(void* const               data,
                struct wl_registry* const registry,
                const uint32_t            name,
                const char* const         interface,
                const uint32_t            PUGL_UNUSED(version))
{
  PuglWorld* const          world = (PuglWorld*)data;
  PuglWorldInternals* const impl  = world->impl;

  if (strcmp(interface, wl_shm_interface.name) == 0) {
    impl->shm =
      (struct wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
    impl->compositor = (struct wl_compositor*)wl_registry_bind(
      registry, name, &wl_compositor_interface, 4);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    impl->wmBase = (struct xdg_wm_base*)wl_registry_bind(
      registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(impl->wmBase, &xdg_wm_base_listener, world);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    impl->seat =
      (struct wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 5);
    wl_seat_add_listener(impl->seat, &wl_seat_listener, world);
  } else {
    fprintf(stderr, "Unknown global interface \"%s\"\n", interface);
  }
}

static void
registry_global_remove(void* const               PUGL_UNUSED(data),
                       struct wl_registry* const PUGL_UNUSED(registry),
                       const uint32_t            PUGL_UNUSED(name))
{}

static const struct wl_registry_listener wl_registry_listener = {
  registry_global,
  registry_global_remove,
};

// World

PuglWorldInternals*
puglInitWorldInternals(PuglWorld* const     world,
                       const PuglWorldType  PUGL_UNUSED(type),
                       const PuglWorldFlags PUGL_UNUSED(flags))
{
  struct wl_display* display = wl_display_connect(NULL);
  if (!display) {
    return NULL;
  }

  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  world->impl      = impl;
  impl->display    = display;
  impl->xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  // Get the registry and our list of globals from it
  impl->registry = wl_display_get_registry(display);
  wl_registry_add_listener(impl->registry, &wl_registry_listener, world);
  if (wl_display_roundtrip(display) < 0 || !impl->compositor || !impl->wmBase ||
      !impl->seat) {
    world->impl = NULL;
    free(impl);
    return NULL;
  }

  if (world->impl->shm) {
    impl->cursorTheme = wl_cursor_theme_load(NULL, 24, world->impl->shm);
  }

  assert(!impl->enteredView);
  assert(!impl->focusedView);
  return impl;
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  free(world->impl);
}

void*
puglGetNativeWorld(PuglWorld* const world)
{
  return world->impl->display;
}

double
puglGetTime(const PuglWorld* const world)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
    return 0.0;
  }

  return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0) -
         world->startTime;
}

PuglStatus
puglUpdate(PuglWorld* const world, const double PUGL_UNUSED(timeout))
{
  wl_display_dispatch(world->impl->display);
  return PUGL_SUCCESS;
}

// View

PuglInternals*
puglInitViewInternals(PuglWorld* const world)
{
  (void)world;

  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  return impl;
}

void
puglFreeViewInternals(PuglView* const view)
{
  free(view->impl);
}

double
puglGetScaleFactor(const PuglView* const view)
{
  (void)view;
  return 1.0;
}

PuglStatus
puglSetWindowPosition(PuglView* const PUGL_UNUSED(view),
                      const int       PUGL_UNUSED(x),
                      const int       PUGL_UNUSED(y))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglSetWindowSize(PuglView* const PUGL_UNUSED(view),
                  const unsigned  PUGL_UNUSED(width),
                  const unsigned  PUGL_UNUSED(height))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglApplySizeHint(PuglView* const    PUGL_UNUSED(view),
                  const PuglSizeHint PUGL_UNUSED(hint))
{
  return PUGL_SUCCESS;
}

PuglStatus
puglUpdateSizeHints(PuglView* const PUGL_UNUSED(view))
{
  return PUGL_SUCCESS;
}

PuglStatus
puglSetTransientParent(PuglView* const      PUGL_UNUSED(view),
                       const PuglNativeView PUGL_UNUSED(parent))
{
  return PUGL_UNKNOWN_ERROR;
}

PuglStatus
puglSetViewStyle(PuglView* const          PUGL_UNUSED(view),
                 const PuglViewStyleFlags PUGL_UNUSED(flags))
{
  return PUGL_UNSUPPORTED;
}

static void
onXdgToplevelConfigure(void* const                data,
                       struct xdg_toplevel* const PUGL_UNUSED(toplevel),
                       const int32_t              width,
                       const int32_t              height,
                       struct wl_array* const     PUGL_UNUSED(states))
{
  fprintf(stderr, "[wl] toplevel configure %d %d\n", width, height);

  PuglView* const view = (PuglView*)data;

  assert(width >= 0 && width <= UINT16_MAX);
  assert(height >= 0 && height <= UINT16_MAX);

  PuglConfigureEvent configureEvent = {
    PUGL_CONFIGURE, 0U, 0, 0, (PuglSpan)width, (PuglSpan)height, 0U};

  if (width <= 0 || height <= 0) {
    // Compositor is deferring to our preferred size
    configureEvent.width  = view->sizeHints[PUGL_DEFAULT_SIZE].width;
    configureEvent.height = view->sizeHints[PUGL_DEFAULT_SIZE].height;
  }

  PuglEvent event;
  event.configure     = configureEvent;
  const PuglStatus st = puglDispatchEvent(view, &event);
  if (st) {
    fprintf(stderr, "\tconfigure error: %s\n", puglStrerror(st));
  }
}

static void
onXdgToplevelClose(void* const                data,
                   struct xdg_toplevel* const PUGL_UNUSED(toplevel))
{
  PuglView* const view = (PuglView*)data;

  puglDispatchSimpleEvent(view, PUGL_CLOSE);
}

static void
onXdgToplevelConfigureBounds(void* const                PUGL_UNUSED(data),
                             struct xdg_toplevel* const PUGL_UNUSED(toplevel),
                             const int32_t              width,
                             const int32_t              height)
{
  fprintf(stderr, "[wl] toplevel bounds %d %d\n", width, height);
}

static void
onXdgToplevelWmCapabilities(
  void* const                PUGL_UNUSED(data),
  struct xdg_toplevel* const PUGL_UNUSED(xdg_toplevel),
  struct wl_array* const     PUGL_UNUSED(capabilities))
{
  fprintf(stderr, "[wl] toplevel wm capabilities\n");
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
  onXdgToplevelConfigure,
  onXdgToplevelClose,
  onXdgToplevelConfigureBounds,
  onXdgToplevelWmCapabilities,
};

static void
onXdgSurfaceConfigure(void* const               data,
                      struct xdg_surface* const xdgSurface,
                      const uint32_t            serial)
{
  PuglView* const view        = (PuglView*)data;
  const PuglArea  initialSize = puglGetInitialSize(view);
  const PuglPoint initialPos  = puglGetInitialPosition(view, initialSize);

  fprintf(stderr,
          "[wl] xdg_surface_configure %u %u\n",
          initialSize.width,
          initialSize.height);

  xdg_surface_ack_configure(xdgSurface, serial);

  PuglEvent event = {{PUGL_NOTHING, 0U}};

  const PuglConfigureEvent configure = {PUGL_CONFIGURE,
                                        0U,
                                        initialPos.x,
                                        initialPos.y,
                                        initialSize.width,
                                        initialSize.height,
                                        0U};

  event.configure = configure;
  puglDispatchEvent(view, &event);

  const PuglExposeEvent expose = {
    PUGL_EXPOSE, 0U, 0, 0, initialSize.width, initialSize.height};
  event.expose = expose;
  puglDispatchEvent(view, &event);

  wl_surface_commit(view->impl->wlSurface);

  /* const PuglConfigureEvent configure = */
  /* { PUGL_CONFIGURE, */
  /*   0U, */
  /*   /\* struct wl_buffer* buffer = draw_frame(state); *\/ */
  /*   /\* wl_surface_attach(view->impl->wlSurface, buffer, 0, 0); *\/ */
  /*   /\* wl_surface_commit(view->impl->wlSurface); *\/ */
}

static const struct xdg_surface_listener xdg_surface_listener = {
  onXdgSurfaceConfigure,
};

// Frame

static void
onSurfaceFrameDone(void* data, struct wl_callback* cb, uint32_t time);

static const struct wl_callback_listener wl_surface_frame_listener = {
  onSurfaceFrameDone,
};

static void
onSurfaceFrameDone(void* const         data,
                   struct wl_callback* cb,
                   const uint32_t      PUGL_UNUSED(time))
{
  PuglView* const view = (PuglView*)data;

  // Destroy this callback and request another frame
  wl_callback_destroy(cb);
  cb = wl_surface_frame(view->impl->wlSurface);
  wl_callback_add_listener(cb, &wl_surface_frame_listener, view);

  const PuglExposeEvent expose = {PUGL_EXPOSE,
                                  0U,
                                  0,
                                  0,
                                  view->lastConfigure.width,
                                  view->lastConfigure.height};

  PuglEvent event;
  event.expose = expose;
  puglDispatchEvent(view, &event);
}

PuglPoint
puglGetAncestorCenter(const PuglView* const view)
{
  (void)view;
  const PuglPoint point = {0, 0};
  return point;
}

PuglStatus
puglRealize(PuglView* const view)
{
  PuglWorld* const     world = view->world;
  PuglInternals* const impl  = view->impl;
  PuglStatus           st    = PUGL_SUCCESS;

  // Ensure that we're unrealized
  if (impl->wlSurface) {
    return PUGL_FAILURE;
  }

  // Check that the basic required configuration has been done
  if ((st = puglPreRealize(view))) {
    return st;
  }

  impl->wlSurface = wl_compositor_create_surface(world->impl->compositor);
  assert(impl->wlSurface);

  impl->xdgSurface =
    xdg_wm_base_get_xdg_surface(world->impl->wmBase, impl->wlSurface);

  assert(impl->xdgSurface);

  xdg_surface_add_listener(impl->xdgSurface, &xdg_surface_listener, view);

  impl->toplevel = xdg_surface_get_toplevel(impl->xdgSurface);
  assert(impl->toplevel);

  xdg_toplevel_add_listener(impl->toplevel, &xdg_toplevel_listener, view);

  char* const title = view->strings[PUGL_WINDOW_TITLE];
  if (title) {
    xdg_toplevel_set_title(impl->toplevel, title);
  }

  puglSetCursor(view, PUGL_CURSOR_ARROW);

  // Configure the backend
  if ((st = view->backend->configure(view))) {
    view->backend->destroy(view);
    return st ? st : PUGL_BACKEND_FAILED;
  }

  // Create the backend drawing context/surface
  if ((st = view->backend->create(view))) {
    return st;
  }

  st = puglDispatchSimpleEvent(view, PUGL_REALIZE);

  struct wl_callback* cb = wl_surface_frame(view->impl->wlSurface);
  wl_callback_add_listener(cb, &wl_surface_frame_listener, view);

  wl_surface_damage_buffer(impl->wlSurface, 0, 0, INT32_MAX, INT32_MAX);
  wl_surface_commit(view->impl->wlSurface);

  return st;
}

PuglStatus
puglUnrealize(PuglView* const PUGL_UNUSED(view))
{
  return PUGL_UNKNOWN_ERROR;
}

PuglStatus
puglShow(PuglView* const view, const PuglShowCommand PUGL_UNUSED(command))
{
  // FIXME
  return puglRealize(view);
}

PuglStatus
puglHide(PuglView* const PUGL_UNUSED(view))
{
  return PUGL_SUCCESS;
  // FIXME
  // return PUGL_UNKNOWN_ERROR;
}

PuglNativeView
puglGetNativeView(const PuglView* const view)
{
  return (PuglNativeView)view->impl->wlSurface;
}

PuglStatus
puglViewStringChanged(PuglView* const      view,
                      const PuglStringHint key,
                      const char* const    value)
{
  if (!view->impl->wlSurface) {
    return PUGL_SUCCESS;
  }

  switch (key) {
  case PUGL_CLASS_NAME:
    break;

  case PUGL_WINDOW_TITLE:
    if (value) {
      xdg_toplevel_set_title(view->impl->toplevel, value);
    }
    break;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglObscureView(PuglView* const view)
{
  wl_surface_damage_buffer(view->impl->wlSurface, 0, 0, INT32_MAX, INT32_MAX);
  return PUGL_SUCCESS;
}

PuglStatus
puglObscureRegion(PuglView* const view,
                  const int       x,
                  const int       y,
                  const unsigned  width,
                  const unsigned  height)
{
  wl_surface_damage_buffer(
    view->impl->wlSurface, x, y, (int32_t)width, (int32_t)height);

  return PUGL_SUCCESS;
}

PuglStatus
puglGrabFocus(PuglView* const PUGL_UNUSED(view))
{
  return PUGL_UNKNOWN_ERROR;
}

bool
puglHasFocus(const PuglView* const PUGL_UNUSED(view))
{
  return false;
}

PuglStatus
puglAcceptOffer(PuglView* const                 PUGL_UNUSED(view),
                const PuglDataOfferEvent* const PUGL_UNUSED(offer),
                const uint32_t                  PUGL_UNUSED(typeIndex))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglPaste(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* PUGL_UNUSED(view))
{
  return 0U;
}

const char*
puglGetClipboardType(const PuglView* const PUGL_UNUSED(view),
                     const uint32_t        PUGL_UNUSED(typeIndex))
{
  return NULL;
}

PuglStatus
puglSetClipboard(PuglView* const   PUGL_UNUSED(view),
                 const char* const PUGL_UNUSED(type),
                 const void* const PUGL_UNUSED(data),
                 const size_t      PUGL_UNUSED(len))
{
  return PUGL_UNSUPPORTED;
}

const void*
puglGetClipboard(PuglView* const PUGL_UNUSED(view),
                 const uint32_t  PUGL_UNUSED(typeIndex),
                 size_t* const   PUGL_UNUSED(len))
{
  return NULL;
}

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
  PuglWorld* const world = view->world;
  if (!world->impl->cursorTheme) {
    return PUGL_FAILURE;
  }

  static const char* const cursor_names[] = {
    "left_ptr",
    "text",
    "crosshair",
    "hand2",
    "not-allowed",
    "h_double_arrow",
    "v_double_arrow",
    "size_fdiag",
    "size_bdiag",
    "all-scroll",
  };

  struct wl_cursor* const wlCursor =
    wl_cursor_theme_get_cursor(world->impl->cursorTheme, cursor_names[cursor]);

  view->impl->cursorImage = wlCursor->images[0];
  view->impl->cursorSurface =
    wl_compositor_create_surface(world->impl->compositor);

  struct wl_buffer* const cursorBuffer =
    wl_cursor_image_get_buffer(view->impl->cursorImage);

  wl_surface_attach(view->impl->cursorSurface, cursorBuffer, 0, 0);
  wl_surface_commit(view->impl->cursorSurface);

  if (view == world->impl->enteredView) {
    assert(view->impl->lastEnterSerial);
    wl_pointer_set_cursor(world->impl->pointer,
                          view->impl->lastEnterSerial,
                          view->impl->cursorSurface,
                          (int32_t)view->impl->cursorImage->hotspot_x,
                          (int32_t)view->impl->cursorImage->hotspot_y);
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* const view, const uintptr_t id, const double timeout)
{
  (void)view;
  (void)id;
  (void)timeout;
  return PUGL_FAILURE;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
  (void)view;
  (void)id;
  return PUGL_FAILURE;
}

PuglStatus
puglSendEvent(PuglView* const view, const PuglEvent* const event)
{
  (void)view;
  (void)event;
  return PUGL_UNSUPPORTED;
}
