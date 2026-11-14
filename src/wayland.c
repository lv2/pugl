// Copyright 2022-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "wayland.h"

#include "attributes.h"
#include "internal.h"
#include "macros.h"
#include "platform.h"
#include "types.h"
#include "xdg-shell.h"
#include "xkb.h"

#include <pugl/pugl.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-util.h>

#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG_VERBOSE 1

#ifndef BTN_LEFT
#  define BTN_LEFT 0x110
#endif

// Utilities

#ifdef DEBUG_VERBOSE
#  include <stdio.h>
#  define DEBUG_LOG(m) (void)fprintf(stderr, "[wl] %s\n", m)
#  define DEBUG_LOGF(f, ...) (void)fprintf(stderr, "[wl] " f "\n", __VA_ARGS__)
#endif

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

static const struct xdg_wm_base_listener wmBaseListener = {
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

  const double    x     = wl_fixed_to_double(surfaceX);
  const double    y     = wl_fixed_to_double(surfaceY);
  const PuglEvent event = {.crossing = {PUGL_POINTER_IN,
                                        0U,
                                        0.0,
                                        x,
                                        y,
                                        x,
                                        y,
                                        world->impl->xkb.depressedMods,
                                        PUGL_CROSSING_NORMAL}};
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

  const double    x     = wl_fixed_to_double(view->impl->pointerX);
  const double    y     = wl_fixed_to_double(view->impl->pointerY);
  const PuglEvent event = {.crossing = {PUGL_POINTER_OUT,
                                        0U,
                                        0.0,
                                        x,
                                        y,
                                        x,
                                        y,
                                        world->impl->xkb.depressedMods,
                                        PUGL_CROSSING_NORMAL}};
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

  const double    x     = wl_fixed_to_double(surfaceX);
  const double    y     = wl_fixed_to_double(surfaceY);
  const PuglEvent event = {.motion = {PUGL_MOTION,
                                      0U,
                                      (double)time / 1e3,
                                      x,
                                      y,
                                      x,
                                      y,
                                      world->impl->xkb.depressedMods}};
  puglDispatchEvent(view, &event);
}

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

  const bool   pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
  const double x       = wl_fixed_to_double(view->impl->pointerX);
  const double y       = wl_fixed_to_double(view->impl->pointerY);

  const PuglEvent event = {
    .button = {pressed ? PUGL_BUTTON_PRESS : PUGL_BUTTON_RELEASE,
               0U,
               time / 1e3,
               x,
               y,
               x,
               y,
               world->impl->xkb.depressedMods,
               button - BTN_LEFT}};
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

  const double x = wl_fixed_to_double(view->impl->pointerX);
  const double y = wl_fixed_to_double(view->impl->pointerY);

  PuglEvent event = {.scroll = {PUGL_SCROLL,
                                0U,
                                (double)time / 1e3,
                                x,
                                y,
                                x,
                                y,
                                world->impl->xkb.depressedMods,
                                PUGL_SCROLL_DOWN,
                                0.0,
                                0.0}};

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

static const struct wl_pointer_listener pointerListener = {
  onPointerEnter,
  onPointerLeave,
  onPointerMotion,
  onPointerButton,
  onPointerAxis,
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

  xkbKeyboardClose(&impl->xkb);

  // Map the keymap file and parse it
  char* const mapString = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (mapString != MAP_FAILED) {
    xkbKeyboardOpen(&impl->xkb, world->strings[PUGL_LOCALE], mapString);
    munmap(mapString, size);
  }

  close(fd);
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

  view->impl->lastEnterSerial = 0;
  world->impl->focusedView    = NULL;

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

  xkbOnModifiers(
    &world->impl->xkb, modsDepressed, modsLatched, modsLocked, group);
}

static void
onKeyboardRepeatInfo(void* const               data,
                     struct wl_keyboard* const PUGL_UNUSED(keyboard),
                     const int32_t             rate,
                     const int32_t             delay)
{
  PuglWorld* const world = (PuglWorld*)data;

  world->impl->keyRepeatRate  = rate;
  world->impl->keyRepeatDelay = delay;
}

static void
onKeyboardKey(void* const               data,
              struct wl_keyboard* const PUGL_UNUSED(keyboard),
              const uint32_t            PUGL_UNUSED(serial),
              const uint32_t            time,
              const uint32_t            key,
              const uint32_t            state)
{
  static const uint32_t keycode_offset = 8U;

  PuglWorld* const world = (PuglWorld*)data;
  PuglView* const  view  = world->impl->focusedView;
  if (view) {
    xkbOnKey(&world->impl->xkb,
             view,
             (double)time / 1e3,
             key + keycode_offset,
             state == WL_KEYBOARD_KEY_STATE_PRESSED);
  }
}

static const struct wl_keyboard_listener keyboardListener = {
  onKeyboardKeymap,
  onKeyboardEnter,
  onKeyboardLeave,
  onKeyboardKey,
  onKeyboardModifiers,
  onKeyboardRepeatInfo,
};

// Touch

static void
onTouchDown(void*              PUGL_UNUSED(data),
            struct wl_touch*   PUGL_UNUSED(wlTouch),
            uint32_t           PUGL_UNUSED(serial),
            uint32_t           time,
            struct wl_surface* PUGL_UNUSED(surface),
            const int32_t      id,
            const wl_fixed_t   fixedX,
            const wl_fixed_t   fixedY)
{
  const double x = wl_fixed_to_double(fixedX);
  const double y = wl_fixed_to_double(fixedY);
  DEBUG_LOGF("touch down %u %d %f %f", time, id, x, y);
}

static void
onTouchUp(void*            PUGL_UNUSED(data),
          struct wl_touch* PUGL_UNUSED(wlTouch),
          uint32_t         PUGL_UNUSED(serial),
          uint32_t         time,
          int32_t          id)
{
  DEBUG_LOGF("touch up %u %d", time, id);
}

static void
onTouchMotion(void*            PUGL_UNUSED(data),
              struct wl_touch* PUGL_UNUSED(wl_touch),
              uint32_t         time,
              int32_t          id,
              wl_fixed_t       fixedX,
              wl_fixed_t       fixedY)
{
  const double x = wl_fixed_to_double(fixedX);
  const double y = wl_fixed_to_double(fixedY);
  DEBUG_LOGF("touch motion %u %d %f %f", time, id, x, y);
}

static void
onTouchFrame(void* PUGL_UNUSED(data), struct wl_touch* PUGL_UNUSED(wl_touch))
{
  DEBUG_LOG("touch frame end");
}

static void
onTouchCancel(void* PUGL_UNUSED(data), struct wl_touch* PUGL_UNUSED(wl_touch))
{
  DEBUG_LOG("touch cancel");
}

static const struct wl_touch_listener touchListener = {
  onTouchDown,
  onTouchUp,
  onTouchMotion,
  onTouchFrame,
  onTouchCancel,
};

// Seat

static void
onSeatCapabilities(void* const           data,
                   struct wl_seat* const PUGL_UNUSED(seat),
                   const uint32_t        capabilities)
{
  PuglWorld* const          world = (PuglWorld*)data;
  PuglWorldInternals* const impl  = world->impl;

  DEBUG_LOG("seat capabilities");

  const bool hasPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
  if (hasPointer && !impl->pointer) {
    impl->pointer = wl_seat_get_pointer(impl->seat);
    wl_pointer_add_listener(impl->pointer, &pointerListener, world);
  } else if (!hasPointer && impl->pointer) {
    wl_pointer_release(impl->pointer);
    impl->pointer = NULL;
  }

  const bool hasKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
  if (hasKeyboard && !impl->keyboard) {
    impl->keyboard = wl_seat_get_keyboard(impl->seat);
    wl_keyboard_add_listener(impl->keyboard, &keyboardListener, world);
  } else if (!hasKeyboard && impl->keyboard) {
    wl_keyboard_release(impl->keyboard);
    impl->keyboard = NULL;
  }

  const bool hasTouch = capabilities & WL_SEAT_CAPABILITY_TOUCH;
  if (hasTouch && !impl->touch) {
    impl->touch = wl_seat_get_touch(impl->seat);
    wl_touch_add_listener(impl->touch, &touchListener, world);
  } else if (!hasTouch && impl->touch) {
    wl_touch_release(impl->touch);
    impl->touch = NULL;
  }
}

static void
onSeatName(void* const           data,
           struct wl_seat* const wl_seat,
           const char* const     name)
{
  (void)data;
  (void)wl_seat;
  DEBUG_LOGF("seat name \"%s\"", name);
}

static const struct wl_seat_listener seatListener = {onSeatCapabilities,
                                                     onSeatName};

// Output

static void
onOutputGeometry(void* const             PUGL_UNUSED(data),
                 struct wl_output* const PUGL_UNUSED(wlOutput),
                 const int32_t           x,
                 const int32_t           y,
                 const int32_t           physicalWidth,
                 const int32_t           physicalHeight,
                 const int32_t           subpixel,
                 const char* const       make,
                 const char* const       model,
                 const int32_t           PUGL_UNUSED(transform))
{
  DEBUG_LOGF("output geometry %dx%d %dx%d %d %s %s",
             x,
             y,
             physicalWidth,
             physicalHeight,
             subpixel,
             make,
             model);
}

static void
onOutputMode(void* const             data,
             struct wl_output* const PUGL_UNUSED(wlOutput),
             const uint32_t          PUGL_UNUSED(flags),
             const int32_t           width,
             const int32_t           height,
             const int32_t           refresh)
{
  PuglWorld* const world = (PuglWorld*)data;

  DEBUG_LOGF("output mode %dx%d @ %d mHz", width, height, refresh);

  world->sizeHints[PUGL_CURRENT_SIZE].width  = (PuglSpan)width;
  world->sizeHints[PUGL_CURRENT_SIZE].height = (PuglSpan)height;
  world->impl->refreshHz                     = refresh / 1000;

  for (size_t i = 0; i < world->numViews; ++i) {
    world->views[i]->hints[PUGL_REFRESH_RATE] = world->impl->refreshHz;
  }
}

static void
onOutputDone(void* const             PUGL_UNUSED(data),
             struct wl_output* const PUGL_UNUSED(wlOutput))
{
  DEBUG_LOG("output done");
}

static void
onOutputScale(void* const             data,
              struct wl_output* const PUGL_UNUSED(wlOutput),
              const int32_t           factor)
{
  DEBUG_LOGF("output scale %d", factor);
  ((PuglWorld*)data)->impl->scaleFactor = factor;
}

static void
onOutputName(void* const             data,
             struct wl_output* const PUGL_UNUSED(wlOutput),
             const char* const       name)
{
  PuglWorld* const world = (PuglWorld*)data;

  puglSetString(&world->strings[PUGL_OUTPUT_NAME], name);
}

static void
onOutputDescription(void* const             data,
                    struct wl_output* const PUGL_UNUSED(wlOutput),
                    const char* const       description)
{
  PuglWorld* const world = (PuglWorld*)data;

  puglSetString(&world->strings[PUGL_OUTPUT_DESCRIPTION], description);
}

static const struct wl_output_listener outputListener = {
  onOutputGeometry,
  onOutputMode,
  onOutputDone,
  onOutputScale,
  onOutputName,
  onOutputDescription,
};

// Registry

static void
registry_global(void* const               data,
                struct wl_registry* const registry,
                const uint32_t            name,
                const char* const         interface,
                const uint32_t            version)
{
  PuglWorld* const          world = (PuglWorld*)data;
  PuglWorldInternals* const impl  = world->impl;

  if (strcmp(interface, wl_shm_interface.name) == 0) {
    impl->shm =
      (struct wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
    impl->compositor = (struct wl_compositor*)wl_registry_bind(
      registry, name, &wl_compositor_interface, 6);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    impl->wmBase = (struct xdg_wm_base*)wl_registry_bind(
      registry, name, &xdg_wm_base_interface, 5);
    xdg_wm_base_add_listener(impl->wmBase, &wmBaseListener, world);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    impl->seat =
      (struct wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 4);
    wl_seat_add_listener(impl->seat, &seatListener, world);
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    impl->output = (struct wl_output*)wl_registry_bind(
      registry, name, &wl_output_interface, 4);
    wl_output_add_listener(impl->output, &outputListener, world);
  } else {
    (void)version;
    DEBUG_LOGF("Unsupported interface \"%s\" v%u", interface, version);
    return;
  }

  DEBUG_LOGF("Registering interface \"%s\" v%u", interface, version);
}

static void
registry_global_remove(void* const               PUGL_UNUSED(data),
                       struct wl_registry* const PUGL_UNUSED(registry),
                       const uint32_t            PUGL_UNUSED(name))
{}

static const struct wl_registry_listener registryListener = {
  registry_global,
  registry_global_remove,
};

// Toplevel

static PuglEvent
getCurrentConfiguration(const PuglView* const view)
{
  const PuglPoint pos  = puglGetPositionHint(view, PUGL_CURRENT_POSITION);
  const PuglArea  size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);

  PuglEvent event = {.configure = {PUGL_CONFIGURE,
                                   0U,
                                   pos.x,
                                   pos.y,
                                   size.width,
                                   size.height,
                                   view->lastConfigure.style}};

  if (view->impl->toplevel) {
    event.configure.style |= PUGL_VIEW_STYLE_MAPPED;
  } else {
    event.configure.style &= ~(PuglViewStyleFlags)PUGL_VIEW_STYLE_MAPPED;
  }

  return event;
}

static void
onXdgToplevelConfigure(void* const                data,
                       struct xdg_toplevel* const PUGL_UNUSED(toplevel),
                       const int32_t              width,
                       const int32_t              height,
                       struct wl_array* const     statesArray)
{
  DEBUG_LOGF("toplevel configure %d %d", width, height);

  PuglView* const view = (PuglView*)data;

  assert(width >= 0 && width <= UINT16_MAX);
  assert(height >= 0 && height <= UINT16_MAX);

  const uint32_t* const states = (const uint32_t*)statesArray->data;
  PuglEvent event = {.configure = {PUGL_CONFIGURE, 0U, 0, 0, 0U, 0U, 0U}};

  event.configure.style = view->impl->toplevel ? PUGL_VIEW_STYLE_MAPPED : 0U;
  for (unsigned i = 0U; i < statesArray->size; ++i) {
    if (states[i] == XDG_TOPLEVEL_STATE_MAXIMIZED) {
      event.configure.style |= PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE;
    }
    if (states[i] == XDG_TOPLEVEL_STATE_FULLSCREEN) {
      event.configure.style |= PUGL_VIEW_STYLE_FULLSCREEN;
    }
    if (states[i] == XDG_TOPLEVEL_STATE_RESIZING) {
      event.configure.style |= PUGL_VIEW_STYLE_RESIZING;
    }
  }

  // TODO: ACTIVATED, TILED_*, SUSPENDED, CONSTRAINED_*
  // No Wayland support for MODAL, ABOVE, BELOW, HIDDEN, or DEMANDING

  if (width && height) { // Use given size
    event.configure.width  = (PuglSpan)width;
    event.configure.height = (PuglSpan)height;
  } else { // Defer to our preferred size
    event.configure.width  = view->sizeHints[PUGL_DEFAULT_SIZE].width;
    event.configure.height = view->sizeHints[PUGL_DEFAULT_SIZE].height;
  }

  puglDispatchEvent(view, &event);
}

static void
onXdgToplevelClose(void* const                data,
                   struct xdg_toplevel* const PUGL_UNUSED(toplevel))
{
  puglDispatchSimpleEvent((PuglView*)data, PUGL_CLOSE);
}

static void
onXdgToplevelConfigureBounds(void* const                PUGL_UNUSED(data),
                             struct xdg_toplevel* const PUGL_UNUSED(toplevel),
                             const int32_t              width,
                             const int32_t              height)
{
  DEBUG_LOGF("toplevel bounds %d %d", width, height);
}

static void
onXdgToplevelWmCapabilities(
  void* const                PUGL_UNUSED(data),
  struct xdg_toplevel* const PUGL_UNUSED(xdg_toplevel),
  struct wl_array* const     capabilitiesArray)
{
  const uint32_t* const capabilities = (const uint32_t*)capabilitiesArray->data;

  DEBUG_LOG("toplevel wm capabilities");
  if (capabilities[XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU]) {
    DEBUG_LOG("WM has WINDOW_MENU");
  }
  if (capabilities[XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE]) {
    DEBUG_LOG("WM has MAXIMIZE");
  }
  if (capabilities[XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN]) {
    DEBUG_LOG("WM has FULLSCREEN");
  }
  if (capabilities[XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE]) {
    DEBUG_LOG("WM has MINIMIZE");
  }
}

static const struct xdg_toplevel_listener toplevelListener = {
  onXdgToplevelConfigure,
  onXdgToplevelClose,
  onXdgToplevelConfigureBounds,
  onXdgToplevelWmCapabilities,
};

// Wayland Surface

static void
onWlSurfaceEnter(void*              PUGL_UNUSED(data),
                 struct wl_surface* PUGL_UNUSED(wlSurface),
                 struct wl_output*  PUGL_UNUSED(output))
{
  DEBUG_LOG("wl_surface enter");
}

static void
onWlSurfaceLeave(void*              PUGL_UNUSED(data),
                 struct wl_surface* PUGL_UNUSED(wlSurface),
                 struct wl_output*  PUGL_UNUSED(output))
{
  DEBUG_LOG("wl_surface leave");
}

static void
onWlSurfacePreferredBufferScale(void*              PUGL_UNUSED(data),
                                struct wl_surface* PUGL_UNUSED(wlSurface),
                                int32_t            factor)
{
  DEBUG_LOGF("wl_surface preferred buffer scale %d", factor);
}

static void
onWlSurfacePreferredBufferTransform(void*              PUGL_UNUSED(data),
                                    struct wl_surface* PUGL_UNUSED(wlSurface),
                                    uint32_t           transform)
{
  DEBUG_LOGF("wl_surface preferred buffer transform %u", transform);
}

static const struct wl_surface_listener wlSurfaceListener = {
  onWlSurfaceEnter,
  onWlSurfaceLeave,
  onWlSurfacePreferredBufferScale,
  onWlSurfacePreferredBufferTransform,
};

// XDG Surface

static void
onXdgSurfaceConfigure(void* const               data,
                      struct xdg_surface* const xdgSurface,
                      const uint32_t            serial)
{
  PuglView* const view = (PuglView*)data;

  DEBUG_LOG("xdg_surface configure");
  xdg_surface_ack_configure(xdgSurface, serial);

  // Dispatch current configuration
  PuglEvent event = getCurrentConfiguration(view);
  puglDispatchEvent(view, &event);

  // Set entire surface to opaque
  const PuglArea          size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
  struct wl_region* const region =
    wl_compositor_create_region(view->world->impl->compositor);
  wl_region_add(region, 0, 0, size.width, size.height);
  wl_surface_set_opaque_region(view->impl->wlSurface, region);
  wl_region_destroy(region);
  wl_surface_commit(view->impl->wlSurface);
}

static const struct xdg_surface_listener xdgSurfaceListener = {
  onXdgSurfaceConfigure,
};

// World

PuglWorldInternals*
puglAllocWorldInternals(void)
{
  return (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));
}

PuglStatus
puglInitWorldInternals(PuglWorld* const     world,
                       const PuglWorldFlags PUGL_UNUSED(flags))
{
  struct wl_display* const display = wl_display_connect(NULL);
  if (!display) {
    return PUGL_CONNECT_FAILED;
  }

  PuglWorldInternals* const impl = world->impl;

  impl->display     = display;
  impl->scaleFactor = 1;

  // Get the registry and our list of globals from it
  impl->registry = wl_display_get_registry(display);
  wl_registry_add_listener(impl->registry, &registryListener, world);
  if (wl_display_roundtrip(display) < 0) {
    return PUGL_UNKNOWN_ERROR;
  }

  if (!impl->compositor || !impl->wmBase || !impl->seat) {
    world->impl = NULL;
    free(impl);
    return PUGL_UNSUPPORTED;
  }

  wl_display_roundtrip(display);
  if (world->impl->shm) {
    impl->cursorTheme = wl_cursor_theme_load(NULL, 24, world->impl->shm);
  }

  assert(!impl->enteredView);
  assert(!impl->focusedView);
  return PUGL_SUCCESS;
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  xkbKeyboardClose(&world->impl->xkb);
  wl_display_disconnect(world->impl->display);
  free(world->impl);
}

void*
puglGetNativeWorld(PuglWorld* const world)
{
  return world->impl->display;
}

double
puglGetWorldScaleFactor(const PuglWorld* const world,
                        const PuglNativeView   parent,
                        const PuglPoint        position)
{
  (void)parent;
  (void)position;

  return world->impl->scaleFactor;
}

double
puglGetTime(const PuglWorld* const world)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
    return 0.0;
  }

  return ((double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0)) -
         world->startTime;
}

static int
pollDisplay(struct wl_display* const display,
            const short              events,
            const double             timeout)
{
  const int timeout_ms = (timeout < 0.0)    ? -1
                         : (timeout > 60.0) ? 60000
                                            : (int)(timeout * 1000.0);

  struct pollfd pfd = {wl_display_get_fd(display), events, 0};
  const int     rc  = poll(&pfd, 1, timeout_ms);
  return (rc <= 0) ? rc : ((pfd.revents & events) ? rc : -1);
}

/// Flush pending configure and expose events for all views
PUGL_WARN_UNUSED_RESULT static PuglStatus
flushExposures(PuglWorld* const world)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;
  PuglStatus st2 = PUGL_SUCCESS;

  // Dispatch update events and obscure any necessary redraws
  for (size_t i = 0; i < world->numViews; ++i) {
    PuglView* const view = world->views[i];
    puglDispatchSimpleEvent(view, PUGL_UPDATE);
    if (puglGetVisible(view) && view->hints[PUGL_CONTINUOUS]) {
      puglObscureView(view);
    }
  }

  // Expose any obscured views
  const PuglWorldState oldState = world->state;
  world->state                  = PUGL_WORLD_EXPOSING;
  for (size_t i = 0; i < world->numViews; ++i) {
    PuglView* const view = world->views[i];
    if (puglGetVisible(view)) {
      if (view->hints[PUGL_CONTINUOUS]) {
        puglObscureView(view);
      }

      if (view->impl->damage.type &&
          !(st0 = puglDispatchSimpleEvent(view, PUGL_BLANK))) {
        const PuglEvent damageEvent = {.damage = view->impl->damage};
        const PuglEvent exposeEvent = {.expose = {PUGL_EXPOSE,
                                                  0U,
                                                  view->impl->damage.x,
                                                  view->impl->damage.y,
                                                  view->impl->damage.width,
                                                  view->impl->damage.height}};

        st0 = puglDispatchEvent(view, &damageEvent);
        st1 = puglDispatchEvent(view, &exposeEvent);
        st2 = puglDispatchSimpleEvent(view, PUGL_FLUSH);

        // Inform the compositor about surface damage
        wl_surface_damage_buffer(view->impl->wlSurface,
                                 damageEvent.damage.x,
                                 damageEvent.damage.y,
                                 damageEvent.damage.width,
                                 damageEvent.damage.height);
        wl_surface_commit(view->impl->wlSurface);
        view->impl->damage.type = PUGL_NOTHING;
      }
    }
  }

  world->state = oldState;
  return st0 ? st0 : st1 ? st1 : st2;
}

PuglStatus
puglUpdate(PuglWorld* const world, const double timeout)
{
  PuglStatus               st         = PUGL_SUCCESS;
  const double             startTime  = puglGetTime(world);
  const PuglWorldState     startState = world->state;
  struct wl_display* const display    = world->impl->display;
  if (startState == PUGL_WORLD_IDLE) {
    world->state = PUGL_WORLD_UPDATING;
  } else if (startState != PUGL_WORLD_RECURSING) {
    return PUGL_BAD_CALL;
  }

  // See wl_display_prepare_read documentation for notes on correct use
  // Prepare to read events, emptying the queue if necessary until success
  while (wl_display_prepare_read(display)) {
    wl_display_dispatch_pending(display);
  }

  // Flush any pending requests before potentially waiting for events
  for (int f = wl_display_flush(display); f < 0 && errno == EAGAIN;) {
    f = wl_display_flush(display);
  }

  // Poll for events and read/dispatch them for the timeout period
  if (timeout <= 0.0) {
    if (pollDisplay(display, POLLIN, timeout) > 0) {
      wl_display_read_events(display);
      wl_display_dispatch_pending(display);
    } else {
      wl_display_cancel_read(display);
    }
  } else {
    const double endTime = startTime + timeout;
    double       t       = startTime;
    int          rc      = 0;
    while (rc >= 0 && t < endTime) {
      if ((rc = pollDisplay(display, POLLIN, endTime - t)) > 0) {
        wl_display_read_events(display);
        while (wl_display_prepare_read(display)) {
          wl_display_dispatch_pending(display);
        }
      }

      t = puglGetTime(world);
    }
    wl_display_cancel_read(display);
  }

  st = flushExposures(world);

  world->state = startState;
  return st;
}

// View

PuglInternals*
puglInitViewInternals(PuglWorld* const world)
{
  (void)world;

  PuglInternals* const impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  return impl;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    puglUnrealize(view);
    free(view->impl);
  }
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return view->world->impl->scaleFactor;
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
puglSetTransientParent(PuglView* const      PUGL_UNUSED(view),
                       const PuglNativeView PUGL_UNUSED(parent))
{
  return PUGL_UNKNOWN_ERROR;
}

PuglStatus
puglSetViewStyle(PuglView* const view, const PuglViewStyleFlags flags)
{
  PuglWorld* const         world    = view->world;
  PuglInternals* const     impl     = view->impl;
  const PuglViewStyleFlags oldFlags = puglGetViewStyle(view);

  const bool isMaximized =
    (flags & PUGL_VIEW_STYLE_TALL) && (flags & PUGL_VIEW_STYLE_WIDE);
  const bool wasMaximized =
    (oldFlags & PUGL_VIEW_STYLE_TALL) && (oldFlags & PUGL_VIEW_STYLE_WIDE);
  if (isMaximized && !wasMaximized) {
    xdg_toplevel_set_maximized(impl->toplevel);
  } else if (!isMaximized && wasMaximized) {
    xdg_toplevel_unset_maximized(impl->toplevel);
  }

  const bool isFullscreen  = (flags & PUGL_VIEW_STYLE_FULLSCREEN);
  const bool wasFullscreen = (oldFlags & PUGL_VIEW_STYLE_FULLSCREEN);
  if (isFullscreen && !wasFullscreen) {
    xdg_toplevel_set_fullscreen(impl->toplevel, world->impl->output);
  } else if (!isFullscreen && wasFullscreen) {
    xdg_toplevel_unset_fullscreen(impl->toplevel);
  }

  return PUGL_SUCCESS;
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

  // Create Wayland surface
  impl->wlSurface = wl_compositor_create_surface(world->impl->compositor);
  wl_surface_add_listener(impl->wlSurface, &wlSurfaceListener, NULL);
  wl_surface_commit(impl->wlSurface);

  // Attach backend and create context
  if ((st = view->backend->attach(view)) ||
      (st = view->backend->create(view))) {
    return st;
  }

  // Set default depth hints if the user or backend hasn't given any
  puglEnsureHint(view, PUGL_RED_BITS, 8);
  puglEnsureHint(view, PUGL_GREEN_BITS, 8);
  puglEnsureHint(view, PUGL_BLUE_BITS, 8);
  puglEnsureHint(view, PUGL_ALPHA_BITS, 0);

  view->hints[PUGL_REFRESH_RATE] = world->impl->refreshHz;

  if (!(st = puglDispatchSimpleEvent(view, PUGL_REALIZE))) {
    puglObscureView(view);
  }

  return st;
}

PuglStatus
puglUnrealize(PuglView* const view)
{
  if (!view->impl->wlSurface) {
    return PUGL_FAILURE;
  }

  puglHide(view);
  puglDispatchSimpleEvent(view, PUGL_UNREALIZE);

  if (view->backend) {
    view->backend->destroy(view);
    view->backend->detach(view);
  }

  wl_surface_destroy(view->impl->wlSurface);
  view->impl->wlSurface = NULL;
  memset(&view->lastConfigure, 0, sizeof(PuglConfigureEvent));
  memset(&view->impl->damage, 0, sizeof(PuglDamageEvent));
  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view, const PuglShowCommand PUGL_UNUSED(command))
{
  PuglStatus           st    = PUGL_SUCCESS;
  PuglWorld* const     world = view->world;
  PuglInternals* const impl  = view->impl;

  if (view->impl->xdgSurface) {
    return PUGL_FAILURE;
  }

  if ((st = view->impl->wlSurface ? PUGL_SUCCESS : puglRealize(view))) {
    return st;
  }

  impl->xdgSurface =
    xdg_wm_base_get_xdg_surface(world->impl->wmBase, impl->wlSurface);

  impl->toplevel = xdg_surface_get_toplevel(impl->xdgSurface);
  xdg_surface_add_listener(impl->xdgSurface, &xdgSurfaceListener, view);
  xdg_toplevel_add_listener(impl->toplevel, &toplevelListener, view);
  wl_surface_commit(view->impl->wlSurface);

  puglSetCursor(view, PUGL_CURSOR_ARROW);
  if (view->strings[PUGL_WINDOW_TITLE]) {
    xdg_toplevel_set_title(impl->toplevel, view->strings[PUGL_WINDOW_TITLE]);
  }

  if (puglIsValidArea(view->sizeHints[PUGL_MIN_SIZE])) {
    xdg_toplevel_set_min_size(impl->toplevel,
                              view->sizeHints[PUGL_MIN_SIZE].width,
                              view->sizeHints[PUGL_MIN_SIZE].height);
  }

  if (puglIsValidArea(view->sizeHints[PUGL_MAX_SIZE])) {
    xdg_toplevel_set_max_size(impl->toplevel,
                              view->sizeHints[PUGL_MAX_SIZE].width,
                              view->sizeHints[PUGL_MAX_SIZE].height);
  }

  puglObscureView(view);

  return st;
}

PuglStatus
puglHide(PuglView* const view)
{
  if (view->world->state == PUGL_WORLD_EXPOSING) {
    return PUGL_BAD_CALL;
  }

  if (!view->impl->xdgSurface) {
    return PUGL_FAILURE;
  }

  xdg_toplevel_destroy(view->impl->toplevel);
  xdg_surface_destroy(view->impl->xdgSurface);
  wl_surface_attach(view->impl->wlSurface, NULL, 0, 0);
  wl_surface_commit(view->impl->wlSurface);
  view->impl->toplevel   = NULL;
  view->impl->xdgSurface = NULL;

  const PuglEvent configureEvent = getCurrentConfiguration(view);
  return puglDispatchEvent(view, &configureEvent);
}

PuglNativeView
puglGetNativeView(const PuglView* const view)
{
  return (PuglNativeView)view->impl->wlSurface;
}

PuglStatus
puglApplyViewString(PuglView* const      view,
                    const PuglStringHint key,
                    const char* const    value)
{
  if (!view->impl->wlSurface) {
    return PUGL_SUCCESS;
  }

  if (key == PUGL_WINDOW_TITLE && value) {
    xdg_toplevel_set_title(view->impl->toplevel, value);
  }

  return PUGL_SUCCESS;
}

static void
onNoopDone(void* const         PUGL_UNUSED(data),
           struct wl_callback* callback,
           uint32_t            PUGL_UNUSED(callbackData))
{
  wl_callback_destroy(callback);
}

static const struct wl_callback_listener noopListener = {onNoopDone};

PuglStatus
puglObscureView(PuglView* const view)
{
  return puglObscureRegion(view, 0, 0, INT16_MAX, INT16_MAX);
}

PuglStatus
puglObscureRegion(PuglView* const view,
                  const int       x,
                  const int       y,
                  const unsigned  width,
                  const unsigned  height)
{
  PuglStatus st = puglPreObscureRegion(view, x, y, width, height);
  if (st) {
    return st;
  }

  // Expand damage to be exposed at the next loop iteration end
  PuglInternals* const impl = view->impl;
  if (!impl->damage.type) {
    impl->damage.type   = PUGL_DAMAGE;
    impl->damage.x      = (PuglCoord)x;
    impl->damage.y      = (PuglCoord)y;
    impl->damage.width  = (PuglSpan)width;
    impl->damage.height = (PuglSpan)height;
  } else {
    const PuglCoord x1 = MIN(impl->damage.x, (PuglCoord)x);
    const PuglCoord y1 = MIN(impl->damage.y, (PuglCoord)y);
    const PuglCoord x2 =
      MAX((PuglCoord)(impl->damage.x + (PuglCoord)impl->damage.width),
          (PuglCoord)(x + (PuglCoord)width));
    const PuglCoord y2 =
      MAX((PuglCoord)(impl->damage.y + (PuglCoord)impl->damage.height),
          (PuglCoord)(y + (PuglCoord)height));
    impl->damage.x      = x1;
    impl->damage.y      = y1;
    impl->damage.width  = (PuglSpan)(x2 - x1);
    impl->damage.height = (PuglSpan)(y2 - y1);
  }

  if (impl->wlSurface && view->world->state != PUGL_WORLD_UPDATING) {
    // Not dispatching events, send an empty event so we wake up next time
    struct wl_callback* callback = wl_display_sync(view->world->impl->display);
    wl_callback_add_listener(callback, &noopListener, NULL);
  }

  return st;
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
                const uint32_t                  PUGL_UNUSED(typeIndex),
                PuglDataAction                  PUGL_UNUSED(action),
                const int                       PUGL_UNUSED(regionX),
                const int                       PUGL_UNUSED(regionY),
                const unsigned                  PUGL_UNUSED(regionWidth),
                const unsigned                  PUGL_UNUSED(regionHeight))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglPaste(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglRegisterDropType(PuglView* const   PUGL_UNUSED(view),
                     const char* const PUGL_UNUSED(type))
{
  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView*     PUGL_UNUSED(view),
                         const PuglClipboard PUGL_UNUSED(clipboard))
{
  return 0U;
}

const char*
puglGetClipboardType(const PuglView* const PUGL_UNUSED(view),
                     const PuglClipboard   PUGL_UNUSED(clipboard),
                     const uint32_t        PUGL_UNUSED(typeIndex))
{
  return NULL;
}

PuglStatus
puglSetClipboard(PuglView* const     PUGL_UNUSED(view),
                 const PuglClipboard PUGL_UNUSED(clipboard),
                 const char* const   PUGL_UNUSED(type),
                 const void* const   PUGL_UNUSED(data),
                 const size_t        PUGL_UNUSED(len))
{
  return PUGL_UNSUPPORTED;
}

const void*
puglGetClipboard(PuglView* const     PUGL_UNUSED(view),
                 const PuglClipboard PUGL_UNUSED(clipboard),
                 const uint32_t      PUGL_UNUSED(typeIndex),
                 size_t* const       PUGL_UNUSED(len))
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

  static const char* const cursorNames[] = {
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

  const unsigned index = (unsigned)cursor;
  const unsigned count = sizeof(cursorNames) / sizeof(cursorNames[0]);
  if (index >= count) {
    return PUGL_BAD_PARAMETER;
  }

  struct wl_cursor* const wlCursor =
    wl_cursor_theme_get_cursor(world->impl->cursorTheme, cursorNames[cursor]);

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
  if (view->world->state == PUGL_WORLD_EXPOSING) {
    return PUGL_FAILURE;
  }

  if (event->type == PUGL_EXPOSE) {
    return puglObscureRegion(view,
                             event->expose.x,
                             event->expose.y,
                             event->expose.width,
                             event->expose.height);
  }

  return PUGL_UNSUPPORTED;
}

#undef DEBUG_LOGF
#undef DEBUG_LOG
