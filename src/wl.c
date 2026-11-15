// Copyright 2022-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "wl.h"

#include "attributes.h"
#include "internal.h"
#include "macros.h"
#include "platform.h"
#include "types.h"
#include "wl_clipboard.h"
#include "xdg-shell.h"
#include "xkb.h"

#include <pugl/pugl.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-util.h>

#include <poll.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG_VERBOSE 1

#ifdef DEBUG_VERBOSE
#  include <stdio.h>
#  define DEBUG_LOG(m) (void)fprintf(stderr, "[wl] %s\n", m)
#  define DEBUG_LOGF(f, ...) (void)fprintf(stderr, "[wl] " f "\n", __VA_ARGS__)
#endif

#ifndef BTN_LEFT
#  define BTN_LEFT 0x110
#endif

// Currently limited to avoid having to dynamically allocate pollfds
#define MAX_NUM_TIMERS 8U

// Utilities

PuglView*
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
  world->impl->enteredView    = view;
  view->impl->pointerX        = surfaceX;
  view->impl->pointerY        = surfaceY;
  view->impl->lastEnterSerial = serial;

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
                                        world->impl->xkb.activeMods,
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
  world->impl->enteredView    = NULL;
  view->impl->lastEnterSerial = 0;

  const double    x     = wl_fixed_to_double(view->impl->pointerX);
  const double    y     = wl_fixed_to_double(view->impl->pointerY);
  const PuglEvent event = {.crossing = {PUGL_POINTER_OUT,
                                        0U,
                                        0.0,
                                        x,
                                        y,
                                        x,
                                        y,
                                        world->impl->xkb.activeMods,
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
                                      world->impl->xkb.activeMods}};
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
               world->impl->xkb.activeMods,
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
                                world->impl->xkb.activeMods,
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
  NULL, // frame (v5)
  NULL, // axis_source (v5)
  NULL, // axis_stop (v5)
  NULL, // axis_discrete (v5)
  NULL, // axis_value120 (v8)
  NULL, // axis_relative_direction (v9)
  NULL, // warp (v11)
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

  puglXkbClose(&impl->xkb);

  // Map the keymap file and parse it
  char* const mapString = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (mapString != MAP_FAILED) {
    puglXkbOpen(&impl->xkb, world->strings[PUGL_LOCALE], false);
    puglXkbLoadKeymapString(&impl->xkb, mapString);
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

  world->impl->focusedView = view;
  world->impl->focusSerial = serial;

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

  world->impl->focusedView = NULL;
  world->impl->focusSerial = 0U;

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

  puglXkbOnModifiers(
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
    puglXkbOnKey(&world->impl->xkb,
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
}

static void
onSeatName(void*           PUGL_UNUSED(data),
           struct wl_seat* PUGL_UNUSED(wl_seat),
           const char*     PUGL_UNUSED(name))
{}

static const struct wl_seat_listener seatListener = {onSeatCapabilities,
                                                     onSeatName};

// Output

static void
onOutputGeometry(void*             PUGL_UNUSED(data),
                 struct wl_output* PUGL_UNUSED(wlOutput),
                 const int32_t     PUGL_UNUSED(x),
                 const int32_t     PUGL_UNUSED(y),
                 const int32_t     PUGL_UNUSED(physicalWidth),
                 const int32_t     PUGL_UNUSED(physicalHeight),
                 const int32_t     PUGL_UNUSED(subpixel),
                 const char*       PUGL_UNUSED(make),
                 const char*       PUGL_UNUSED(model),
                 const int32_t     PUGL_UNUSED(transform))
{}

static void
onOutputMode(void* const       data,
             struct wl_output* PUGL_UNUSED(wlOutput),
             const uint32_t    PUGL_UNUSED(flags),
             const int32_t     PUGL_UNUSED(width),
             const int32_t     PUGL_UNUSED(height),
             const int32_t     refresh)
{
  PuglWorld* const world = (PuglWorld*)data;

  world->impl->refreshHz = refresh / 1000;
  for (size_t i = 0; i < world->numViews; ++i) {
    world->views[i]->hints[PUGL_REFRESH_RATE] = world->impl->refreshHz;
  }
}

static void
onOutputDone(void* PUGL_UNUSED(data), struct wl_output* PUGL_UNUSED(wlOutput))
{}

static void
onOutputScale(void* const       data,
              struct wl_output* PUGL_UNUSED(wlOutput),
              const int32_t     factor)
{
  ((PuglWorld*)data)->impl->scaleFactor = factor;
}

static void
onOutputName(void*             PUGL_UNUSED(data),
             struct wl_output* PUGL_UNUSED(wlOutput),
             const char*       PUGL_UNUSED(name))
{}

static void
onOutputDescription(void*             PUGL_UNUSED(data),
                    struct wl_output* PUGL_UNUSED(wlOutput),
                    const char*       PUGL_UNUSED(description))
{}

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
onRegistryGlobal(void* const               data,
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
  } else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
    impl->dataDeviceManager = (struct wl_data_device_manager*)wl_registry_bind(
      registry, name, &wl_data_device_manager_interface, 3);
  } else {
    (void)version;
    DEBUG_LOGF("Unsupported interface \"%s\" v%u", interface, version);
    return;
  }

  DEBUG_LOGF("Registering interface %u \"%s\" v%u", name, interface, version);
}

static void
onRegistryGlobalRemove(void*               PUGL_UNUSED(data),
                       struct wl_registry* PUGL_UNUSED(registry),
                       const uint32_t      name)
{
  DEBUG_LOGF("Remove global %u", name);
}

static const struct wl_registry_listener registryListener = {
  onRegistryGlobal,
  onRegistryGlobalRemove,
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
                                   view->world->impl->scaleFactor,
                                   view->lastConfigure.style}};

  if (view->impl->toplevel) {
    event.configure.style |= PUGL_VIEW_STYLE_MAPPED;
  } else {
    event.configure.style &= ~(PuglViewStyleFlags)PUGL_VIEW_STYLE_MAPPED;
  }

  return event;
}

static void
onXdgToplevelConfigure(void* const            data,
                       struct xdg_toplevel*   PUGL_UNUSED(toplevel),
                       const int32_t          width,
                       const int32_t          height,
                       struct wl_array* const statesArray)
{
  DEBUG_LOGF("toplevel configure %d %d", width, height);

  PuglView* const view = (PuglView*)data;

  assert(width >= 0 && width <= UINT16_MAX);
  assert(height >= 0 && height <= UINT16_MAX);

  const size_t          numStates = statesArray->size / sizeof(uint32_t);
  const uint32_t* const states    = (const uint32_t*)statesArray->data;

  PuglEvent event = {.configure = {PUGL_CONFIGURE, 0U, 0, 0, 0U, 0U, 1.0, 0U}};
  event.configure.style = view->impl->toplevel ? PUGL_VIEW_STYLE_MAPPED : 0U;

  for (unsigned i = 0U; i < numStates; ++i) {
    if (states[i] == XDG_TOPLEVEL_STATE_MAXIMIZED) {
      event.configure.style |= PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE;
    }
    if (states[i] == XDG_TOPLEVEL_STATE_FULLSCREEN) {
      event.configure.style |= PUGL_VIEW_STYLE_FULLSCREEN;
    }
    if (states[i] == XDG_TOPLEVEL_STATE_RESIZING) {
      event.configure.style |= PUGL_VIEW_STYLE_RESIZING;
    }
    if (states[i] == XDG_TOPLEVEL_STATE_ACTIVATED) {
      event.configure.style |= PUGL_VIEW_STYLE_ACTIVE;
    }
    if (states[i] == XDG_TOPLEVEL_STATE_SUSPENDED) {
      event.configure.style |= PUGL_VIEW_STYLE_HIDDEN;
    }
  }

  // No Pugl support for TILED_* or CONSTRAINED_*
  // No Wayland support for MODAL, ABOVE, BELOW, or DEMANDING

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
onXdgToplevelClose(void* const data, struct xdg_toplevel* PUGL_UNUSED(toplevel))
{
  puglDispatchSimpleEvent((PuglView*)data, PUGL_CLOSE);
}

static void
onXdgToplevelConfigureBounds(void*                PUGL_UNUSED(data),
                             struct xdg_toplevel* PUGL_UNUSED(toplevel),
                             const int32_t        width,
                             const int32_t        height)
{
  DEBUG_LOGF("toplevel bounds %d %d", width, height);
}

static void
onXdgToplevelWmCapabilities(void*                PUGL_UNUSED(data),
                            struct xdg_toplevel* PUGL_UNUSED(xdg_toplevel),
                            struct wl_array*     PUGL_UNUSED(capabilitiesArray))
{
  DEBUG_LOG("toplevel wm capabilities");
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
                                const int32_t      factor)
{
  DEBUG_LOGF("wl_surface preferred buffer scale %d", factor);
}

static void
onWlSurfacePreferredBufferTransform(void*              PUGL_UNUSED(data),
                                    struct wl_surface* PUGL_UNUSED(wlSurface),
                                    const uint32_t     transform)
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

  impl->display          = display;
  impl->scaleFactor      = 1;
  impl->drag.clipboard   = PUGL_CLIPBOARD_GENERAL;
  impl->drag.clipboard   = PUGL_CLIPBOARD_DRAG;
  impl->clipboard.pipeFd = -1;
  impl->drag.pipeFd      = -1;

  // Get the registry and our list of globals from it
  impl->registry = wl_display_get_registry(display);
  wl_registry_add_listener(impl->registry, &registryListener, world);
  if (wl_display_roundtrip(display) < 0) {
    return PUGL_UNKNOWN_ERROR;
  }

  // Ensure required globals exist
  if (!impl->compositor || !impl->wmBase || !impl->seat) {
    world->impl = NULL;
    free(impl);
    return PUGL_UNSUPPORTED;
  }

  if (impl->dataDeviceManager) {
    impl->dataDevice = wl_data_device_manager_get_data_device(
      impl->dataDeviceManager, impl->seat);

    puglWaylandClipboardDeviceListen(world, impl->dataDevice);
  }

  wl_display_roundtrip(display);
  if (world->impl->shm) {
    impl->cursorTheme = wl_cursor_theme_load(NULL, 24, world->impl->shm);
  }

  assert(!impl->enteredView);
  assert(!impl->focusedView);
  return PUGL_SUCCESS;
}

static void
freeClipboard(PuglWaylandClipboard* const board)
{
  for (unsigned long i = 0; i < board->numFormats; ++i) {
    free(board->formats[i]);
  }
  free(board->formats);
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  freeClipboard(&world->impl->clipboard);
  freeClipboard(&world->impl->drag);
  freeClipboard(&world->impl->dataOffer);

  wl_cursor_theme_destroy(world->impl->cursorTheme);
  wl_data_device_destroy(world->impl->dataDevice);
  puglXkbClose(&world->impl->xkb);

  wl_data_device_manager_release(world->impl->dataDeviceManager);
  wl_output_release(world->impl->output);
  xdg_wm_base_destroy(world->impl->wmBase);
  wl_compositor_release(world->impl->compositor);
  wl_shm_release(world->impl->shm);

  wl_keyboard_release(world->impl->keyboard);
  wl_pointer_release(world->impl->pointer);
  wl_seat_release(world->impl->seat);

  wl_registry_destroy(world->impl->registry);
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

PuglNanoseconds
puglGetNanoseconds(const PuglWorld* PUGL_UNUSED(world))
{
  struct timespec ts = {0, 0};
  return clock_gettime(CLOCK_MONOTONIC, &ts)
           ? 0U
           : ((1000000000L * ts.tv_sec) + ts.tv_nsec);
}

double
puglGetTime(const PuglWorld* const world)
{
  const PuglNanoseconds nanoseconds = puglGetNanoseconds(world);

  return (double)nanoseconds / 1e9;
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

// Read available display events and leave the display prepared to read again
static void
readDisplayEvents(struct wl_display* const display)
{
  wl_display_read_events(display);
  while (wl_display_prepare_read(display)) {
    wl_display_dispatch_pending(display);
  }
}

static void
readTimerEvent(PuglWorld* const world, const int fd, const short revents)
{
  for (uint32_t t = 0U; t < world->impl->numTimers; ++t) {
    const PuglTimer* const timer = &world->impl->timers[t];
    if (fd == timer->fd && (revents & POLLIN)) {
      uint64_t numExpirations = 0U;
      read(fd, &numExpirations, sizeof(numExpirations));
      const PuglEvent event = {.timer = {PUGL_TIMER, 0U, timer->id}};
      puglDispatchEvent(timer->view, &event);
    }
  }
}

static struct pollfd
makePollfd(const int fd, const short events)
{
  const struct pollfd ret = {fd, events, 0};
  return ret;
}

static void
readEvents(PuglWorld* const           world,
           const unsigned             numPfds,
           const struct pollfd* const pfds)
{
  if ((pfds[0].revents & POLLIN)) {
    readDisplayEvents(world->impl->display);
  }
  for (unsigned i = 1U; i < numPfds; ++i) {
    if (pfds[i].revents) {
      if (pfds[i].fd == world->impl->clipboard.pipeFd) {
        puglWaylandClipboardRead(&world->impl->clipboard, pfds[i].revents);
      } else if (pfds[i].fd == world->impl->drag.pipeFd) {
        puglWaylandClipboardRead(&world->impl->drag, pfds[i].revents);
      } else {
        readTimerEvent(world, pfds[i].fd, pfds[i].revents);
      }
    }
  }
}

PuglStatus
puglUpdateWorld(PuglWorld* const world, const double timeout)
{
  const PuglNanoseconds    startNs   = puglGetNanoseconds(world);
  const PuglNanoseconds    timeoutNs = (PuglNanoseconds)(timeout * 1e9);
  struct wl_display* const display   = world->impl->display;

  // See wl_display_prepare_read documentation for notes on correct use
  // Prepare to read events, emptying the queue if necessary until success
  while (wl_display_prepare_read(display)) {
    wl_display_dispatch_pending(display);
  }

  // Flush any pending requests before potentially waiting for events
  for (int f = wl_display_flush(display); f < 0 && errno == EAGAIN;) {
    f = wl_display_flush(display);
  }

  // Always poll the display
  unsigned      numPfds                  = 1U;
  struct pollfd pfds[3 + MAX_NUM_TIMERS] = {
    {wl_display_get_fd(display), POLLIN, 0},
  };

  // Poll any incoming clipboard pipes
  if (world->impl->clipboard.pipeFd >= 0) {
    pfds[numPfds++] = makePollfd(world->impl->clipboard.pipeFd, POLLIN);
  }
  if (world->impl->drag.pipeFd >= 0) {
    pfds[numPfds++] = makePollfd(world->impl->drag.pipeFd, POLLIN);
  }

  // Poll all active timers
  for (uint32_t t = 0U; t < world->impl->numTimers; ++t) {
    if (world->impl->timers[t].fd) {
      pfds[numPfds++] = makePollfd(world->impl->timers[t].fd, POLLIN);
    }
  }

  // Poll for events and read/dispatch them for the timeout period
  if (timeoutNs <= 0) {
    if (poll(pfds, numPfds, (int)(timeoutNs / 1000000)) > 0) {
      readEvents(world, numPfds, pfds);
    }
  } else {
    const PuglNanoseconds endNs = startNs + timeoutNs;
    int                   rc    = 0;
    for (int64_t t = startNs; rc >= 0 && t < endNs;
         t         = puglGetNanoseconds(world)) {
      if ((rc = poll(pfds, numPfds, (int)((endNs - t) / 1000000)) > 0)) {
        readEvents(world, numPfds, pfds);
      }
    }
  }

  // Cancel the last prepared read and flush exposures
  wl_display_cancel_read(display);
  return flushExposures(world);
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
    if (view->impl->cursorSurface) {
      wl_surface_destroy(view->impl->cursorSurface);
    }
    free(view->impl);
  }
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return view->world->impl->scaleFactor;
}

PuglStatus
puglSetWindowPosition(PuglView* PUGL_UNUSED(view),
                      const int PUGL_UNUSED(x),
                      const int PUGL_UNUSED(y))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglSetWindowSize(PuglView*      PUGL_UNUSED(view),
                  const unsigned PUGL_UNUSED(width),
                  const unsigned PUGL_UNUSED(height))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglApplySizeHint(PuglView*          PUGL_UNUSED(view),
                  const PuglSizeHint PUGL_UNUSED(hint))
{
  return PUGL_SUCCESS;
}

int
puglFetchViewHint(const PuglView*    PUGL_UNUSED(view),
                  const PuglViewHint PUGL_UNUSED(hint))
{
  return PUGL_DONT_CARE;
}

PuglStatus
puglSetTransientParent(PuglView*            PUGL_UNUSED(view),
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
puglReconfigure(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
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
onNoopDone(void*               PUGL_UNUSED(data),
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
    const int x1 = MIN(impl->damage.x, x);
    const int y1 = MIN(impl->damage.y, y);
    const int x2 = MAX(impl->damage.x + impl->damage.width, x + (int)width);
    const int y2 = MAX(impl->damage.y + impl->damage.height, y + (int)height);

    impl->damage.x      = (PuglCoord)x1;
    impl->damage.y      = (PuglCoord)y1;
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
puglGrabFocus(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
}

bool
puglHasFocus(const PuglView* const view)
{
  return view == view->world->impl->focusedView;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const offer,
                const uint32_t                  typeIndex,
                PuglDataAction                  PUGL_UNUSED(action),
                const int                       PUGL_UNUSED(regionX),
                const int                       PUGL_UNUSED(regionY),
                const unsigned                  PUGL_UNUSED(regionWidth),
                const unsigned                  PUGL_UNUSED(regionHeight))
{
  PuglWorld* const world = view->world;

  if (offer->clipboard == PUGL_CLIPBOARD_GENERAL) {
    // Request data to be sent through a pipe which will be read next update
    PuglWaylandClipboard* const board = &world->impl->clipboard;

    if (board->source && board->data && typeIndex < board->numFormats &&
        typeIndex == board->acceptedFormatIndex) {
      const PuglEvent data = {.data = {PUGL_DATA,
                                       0U,
                                       0.0,
                                       0.0,
                                       0.0,
                                       board->clipboard,
                                       board->acceptedFormatIndex}};
      return puglDispatchEvent(view, &data);
    }

    if (board->pipeFd >= 0 || !board->offer) {
      return PUGL_FAILURE;
    }

    board->pipeFd = puglWaylandClipboardRequestData(
      board->offer, board->formats[board->acceptedFormatIndex]);

    board->receiver             = view;
    board->acceptedFormatIndex  = typeIndex;
    view->world->impl->dataView = view;
  } else {
    PuglWaylandClipboard* const board = &world->impl->drag;
    wl_data_offer_accept(
      board->offer, world->impl->dragSerial, board->formats[typeIndex]);
    wl_data_offer_set_actions(board->offer,
                              WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY,
                              WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
    board->acceptedFormatIndex = typeIndex;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglRejectOffer(PuglView*                       view,
                const PuglDataOfferEvent* const offer,
                const int                       PUGL_UNUSED(regionX),
                const int                       PUGL_UNUSED(regionY),
                const unsigned                  PUGL_UNUSED(regionWidth),
                const unsigned                  PUGL_UNUSED(regionHeight))
{
  if (offer->clipboard == PUGL_CLIPBOARD_DRAG) {
    PuglWaylandClipboard* const board = &view->world->impl->drag;
    wl_data_offer_accept(board->offer, view->world->impl->dragSerial, NULL);
  }
  return PUGL_SUCCESS;
}

PuglStatus
puglPaste(PuglView* const view)
{
  const PuglWaylandClipboard* const board = &view->world->impl->clipboard;
  if (!board->numFormats) {
    return PUGL_FAILURE;
  }

  const PuglEvent data = {
    .offer = {PUGL_DATA_OFFER, 0U, 0.0, 0.0, 0.0, PUGL_CLIPBOARD_GENERAL}};
  puglDispatchEvent(view, &data);

  return PUGL_SUCCESS;
}

PuglStatus
puglRegisterDropType(PuglView* PUGL_UNUSED(view), const char* PUGL_UNUSED(type))
{
  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const view,
                         const PuglClipboard   clipboard)
{
  const PuglWaylandClipboard* const board =
    (clipboard == PUGL_CLIPBOARD_GENERAL) ? &view->world->impl->clipboard
                                          : &view->world->impl->drag;

  return board->numFormats;
}

const char*
puglGetClipboardType(const PuglView* const view,
                     const PuglClipboard   clipboard,
                     const uint32_t        typeIndex)
{
  const PuglWaylandClipboard* const board =
    (clipboard == PUGL_CLIPBOARD_GENERAL) ? &view->world->impl->clipboard
                                          : &view->world->impl->drag;

  return typeIndex < board->numFormats ? board->formats[typeIndex] : NULL;
}

PuglStatus
puglSetClipboard(PuglView* const     view,
                 const PuglClipboard clipboard,
                 const char* const   type,
                 const void* const   data,
                 const size_t        len)
{
  PuglWorldInternals* const   impl = view->world->impl;
  PuglWaylandClipboard* const board =
    (clipboard == PUGL_CLIPBOARD_GENERAL) ? &impl->clipboard : &impl->drag;

  if (board->dataCapacity <= len) {
    const size_t newDataSize = len + 1U;
    void* const  newData     = realloc(board->data, newDataSize);
    if (!newData) {
      return PUGL_NO_MEMORY;
    }

    board->data         = (uint8_t*)newData;
    board->dataCapacity = (uint32_t)newDataSize;
    board->numFormats   = 1U;
  }

  const size_t typeLen = strlen(type);
  free(board->formats);
  board->formats    = (char**)malloc(sizeof(char*));
  board->formats[0] = (char*)malloc(typeLen + 1U);
  memcpy(board->formats[0], type, typeLen + 1U);

  // Copy data to the clipboard so we can send it later
  memcpy(board->data, data, len);
  board->data[len] = '\0';
  board->dataSize  = (uint32_t)len;

  // Create a data source and notify the compositor about it
  board->source =
    wl_data_device_manager_create_data_source(impl->dataDeviceManager);
  puglWaylandClipboardSourceListen(view->world, board->source);
  wl_data_source_offer(board->source, type);
  wl_data_device_set_selection(
    impl->dataDevice, board->source, impl->focusSerial);
  return PUGL_SUCCESS;
}

const void*
puglGetClipboard(PuglView* const     view,
                 const PuglClipboard clipboard,
                 const uint32_t      PUGL_UNUSED(typeIndex),
                 size_t*             PUGL_UNUSED(len))
{
  const PuglWaylandClipboard* const board =
    (clipboard == PUGL_CLIPBOARD_GENERAL) ? &view->world->impl->clipboard
                                          : &view->world->impl->drag;

  return board->data;
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

  if (view->impl->cursorSurface) {
    wl_surface_destroy(view->impl->cursorSurface);
  }

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
  PuglWorldInternals* const impl = view->world->impl;
  if (impl->numTimers >= MAX_NUM_TIMERS) {
    return PUGL_NO_MEMORY;
  }

  // Search for an existing timer with this id to replace
  PuglTimer* timer = NULL;
  for (uint32_t t = 0U; t < impl->numTimers; ++t) {
    if (impl->timers[t].view == view && impl->timers[t].id == id) {
      timer = &impl->timers[t];
      break;
    }
  }

  if (!timer) {
    // Expand the world timers array
    const uint32_t   newNumTimers = impl->numTimers + 1U;
    const size_t     newSize      = newNumTimers * sizeof(PuglTimer);
    PuglTimer* const newTimers    = (PuglTimer*)realloc(impl->timers, newSize);
    if (!newTimers) {
      return PUGL_NO_MEMORY;
    }

    // Create a timerfd for this timer
    const int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (fd < 0) {
      free(newTimers);
      return PUGL_UNKNOWN_ERROR;
    }

    // Add new timer
    timer           = &newTimers[impl->numTimers];
    timer->view     = view;
    timer->id       = id;
    timer->fd       = fd;
    impl->timers    = newTimers;
    impl->numTimers = newNumTimers;
  }

  // Set timer to fire 1 timeout from now and every timeout after that
  const double      seconds     = trunc(timeout);
  const int64_t     nanoseconds = (int64_t)((timeout - seconds) * 1e9);
  struct timespec   period      = {(time_t)seconds, nanoseconds};
  struct itimerspec timerspec   = {period, period};
  timerfd_settime(timer->fd, 0, &timerspec, NULL);
  return PUGL_SUCCESS;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
  PuglWorldInternals* const impl = view->world->impl;

  for (uint32_t i = 0; i < impl->numTimers; ++i) {
    if (impl->timers[i].view == view && impl->timers[i].id == id) {
      close(impl->timers[i].fd);

      if (i != impl->numTimers - 1) {
        memmove(impl->timers + i,
                impl->timers + i + 1,
                sizeof(PuglTimer) * (impl->numTimers - i - 1));
      }

      memset(&impl->timers[impl->numTimers - 1], 0, sizeof(PuglTimer));
      --impl->numTimers;
      return PUGL_SUCCESS;
    }
  }

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
