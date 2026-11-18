// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "console.h"

#include "attributes.h"
#include "internal.h"
#include "platform.h"
#include "types.h"

#include <pugl/pugl.h>

#include <libinput.h>
#include <libudev.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/time.h> // IWYU pragma: keep
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct libinput_event_keyboard;
struct libinput_event_pointer;

#define DEBUG_VERBOSE 1

#ifdef DEBUG_VERBOSE
#  include <stdio.h>
#  define DEBUG_LOG(m) (void)fprintf(stderr, "[console] %s\n", m)
#  define DEBUG_LOGF(f, ...) \
    (void)fprintf(stderr, "[console] " f "\n", __VA_ARGS__)
#else
#  define DEBUG_LOG(m)
#  define DEBUG_LOGF(f, ...)
#endif

static int
openRestricted(const char* const path,
               const int         flags,
               void*             PUGL_UNUSED(userData))
{
  return open(path, flags);
}

static void
closeRestricted(const int fd, void* PUGL_UNUSED(userData))
{
  close(fd);
}

static const struct libinput_interface inputInterface = {
  .open_restricted  = openRestricted,
  .close_restricted = closeRestricted,
};

// World

PuglWorldInternals*
puglAllocWorldInternals(void)
{
  return (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));
}

PuglStatus
puglInitWorldInternals(PuglWorld*           world,
                       const PuglWorldFlags PUGL_UNUSED(flags))
{
  struct udev* const udev = udev_new();
  if (!udev) {
    DEBUG_LOG("error: Failed to open udev");
    return PUGL_CONNECT_FAILED;
  }

  struct libinput* const libinput =
    libinput_udev_create_context(&inputInterface, NULL, udev);
  if (!libinput) {
    DEBUG_LOG("error: Failed to open libinput");
    udev_unref(udev);
    return PUGL_UNKNOWN_ERROR;
  }

  if (libinput_udev_assign_seat(libinput, "seat0")) {
    libinput_unref(libinput);
    udev_unref(udev);
    return PUGL_REGISTRATION_FAILED;
  }

  PuglStatus st =
    puglXkbOpen(&world->impl->xkb, world->strings[PUGL_LOCALE], true);
  if (!st) {
    st = puglXkbLoadDefaultKeymap(&world->impl->xkb);
    if (st) {
      DEBUG_LOGF("error: Failed to load keymap (%s)", puglStrerror(st));
    }
  } else {
    DEBUG_LOGF("error: Failed to open keyboard (%s)", puglStrerror(st));
  }

  world->impl->udev     = udev;
  world->impl->libinput = libinput;
  return PUGL_SUCCESS;
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  if (world->impl->libinput) {
    libinput_unref(world->impl->libinput);
  }
  if (world->impl->udev) {
    udev_unref(world->impl->udev);
  }
  free(world->impl);
}

PuglNativeWorld
puglGetNativeWorld(PuglWorld* PUGL_UNUSED(world))
{
  return NULL;
}

double
puglGetWorldScaleFactor(const PuglWorld* const PUGL_UNUSED(world),
                        const PuglNativeView   PUGL_UNUSED(parent),
                        const PuglPoint        PUGL_UNUSED(position))
{
  return 1.0;
}

static PuglStatus
handleKeyboard(PuglWorld* const                      world,
               struct libinput_event_keyboard* const kev)
{
  static const uint32_t keycodeOffset = 8U;

  PuglView* const view    = world->views[0];
  const uint32_t  key     = libinput_event_keyboard_get_key(kev);
  const bool      pressed = libinput_event_keyboard_get_key_state(kev);
  const uint64_t  uTime   = libinput_event_keyboard_get_time_usec(kev);
  const double    time    = (double)uTime / 1e6;

  puglXkbOnKey(&world->impl->xkb, view, time, key + keycodeOffset, pressed);
  return PUGL_SUCCESS;
}

static PuglStatus
handlePointer(PuglWorld* const world, struct libinput_event_pointer* const pev)
{
  const enum libinput_event_type type =
    libinput_event_get_type(libinput_event_pointer_get_base_event(pev));

  const uint64_t uTime = libinput_event_pointer_get_time_usec(pev);
  const double   time  = (double)uTime / 1e6;

  if (type == LIBINPUT_EVENT_POINTER_MOTION) {
    const double dx = libinput_event_pointer_get_dx(pev);
    const double dy = libinput_event_pointer_get_dy(pev);
    world->impl->mouseX += dx;
    world->impl->mouseY += dy;
    const PuglEvent event = {.motion = {PUGL_MOTION,
                                        0U,
                                        time,
                                        world->impl->mouseX,
                                        world->impl->mouseY,
                                        world->impl->mouseX,
                                        world->impl->mouseY,
                                        0U}};
    return puglDispatchEvent(world->views[0], &event);
  }

  if (type == LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE) {
    const double    x     = libinput_event_pointer_get_absolute_x(pev);
    const double    y     = libinput_event_pointer_get_absolute_y(pev);
    const PuglEvent event = {.motion = {PUGL_MOTION, 0U, time, x, y, x, y, 0U}};
    world->impl->mouseX   = x;
    world->impl->mouseY   = y;
    return puglDispatchEvent(world->views[0], &event);
  }

  if (type == LIBINPUT_EVENT_POINTER_BUTTON) {
    const uint32_t  button  = libinput_event_pointer_get_button(pev);
    const bool      pressed = libinput_event_pointer_get_button_state(pev);
    const PuglEvent event   = {
      .button = {pressed ? PUGL_BUTTON_PRESS : PUGL_BUTTON_RELEASE,
                 0U,
                 time,
                 0.0,
                 0.0,
                 0.0,
                 0.0,
                 0U,
                 button}};
    return puglDispatchEvent(world->views[0], &event);
  }

  /* if (type == LIBINPUT_EVENT_POINTER_SCROLL_WHEEL) { */
  /* } */
  /* if (type == LIBINPUT_EVENT_POINTER_SCROLL_FINGER) { */
  /* } */
  /* if (type == LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS) { */
  /* } */

  return PUGL_FAILURE;
}

PuglStatus
puglUpdateWorld(PuglWorld* const world, const double PUGL_UNUSED(timeout))
{
  PuglStatus             st       = PUGL_SUCCESS;
  struct libinput* const libinput = world->impl->libinput;
  struct pollfd          pfd      = {libinput_get_fd(libinput), POLLIN, 0};

  const int rc = poll(&pfd, 1, 0);
  if (rc > -1) {
    libinput_dispatch(libinput);
    struct libinput_event* ev = NULL;
    while ((ev = libinput_get_event(libinput))) {
      const enum libinput_event_type type = libinput_event_get_type(ev);

      if (type == LIBINPUT_EVENT_KEYBOARD_KEY) {
        handleKeyboard(world, libinput_event_get_keyboard_event(ev));
      } else if (type >= LIBINPUT_EVENT_POINTER_MOTION &&
                 type <= LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS) {
        handlePointer(world, libinput_event_get_pointer_event(ev));
      }

      libinput_event_destroy(ev);
    }

    PuglView* const view = world->views[0];
    const PuglArea  size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);

    // Send update events so the application can trigger redraws
    for (size_t i = 0; i < world->numViews; ++i) {
      puglDispatchSimpleEvent(world->views[i], PUGL_UPDATE);
    }

    puglDispatchSimpleEvent(view, PUGL_BLANK);

    const PuglEvent damage = {
      .damage = {PUGL_DAMAGE, 0U, 0, 0, size.width, size.height}};
    puglDispatchEvent(view, &damage);

    const PuglEvent event = {
      .expose = {PUGL_EXPOSE, 0U, 0, 0, size.width, size.height}};
    puglDispatchEvent(view, &event);

    st = puglDispatchSimpleEvent(view, PUGL_FLUSH);
  }

  return st;
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

// View

PuglInternals*
puglInitViewInternals(PuglWorld* PUGL_UNUSED(world))
{
  PuglInternals* const impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  return impl;
}

PuglStatus
puglSetViewStyle(PuglView*                PUGL_UNUSED(view),
                 const PuglViewStyleFlags PUGL_UNUSED(flags))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglApplySizeHint(PuglView*          PUGL_UNUSED(view),
                  const PuglSizeHint PUGL_UNUSED(hint))
{
  return PUGL_UNSUPPORTED;
}

int
puglFetchViewHint(const PuglView*    PUGL_UNUSED(view),
                  const PuglViewHint PUGL_UNUSED(hint))
{
  return PUGL_DONT_CARE;
}

PuglStatus
puglReconfigure(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
}

PuglPoint
puglGetAncestorCenter(const PuglView* PUGL_UNUSED(view))
{
  const PuglPoint origin = {0, 0};
  return origin;
}

PuglStatus
puglRealize(PuglView* const view)
{
  PuglStatus     st   = PUGL_SUCCESS;
  const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);

  // Attach the backend to get the visual info
  if ((st = view->backend->attach(view))) {
    view->backend->detach(view);
    return st;
  }

  // Create the backend drawing context/surface
  if ((st = view->backend->create(view))) {
    return st;
  }

  puglDispatchSimpleEvent(view, PUGL_REALIZE);

  // FIXME
  const PuglEvent configure = {
    .configure = {PUGL_CONFIGURE, 0U, 0, 0, size.width, size.height, 1.0, 0U}};
  puglDispatchEvent(view, &configure);

  return st;
}

PuglStatus
puglUnrealize(PuglView* const view)
{
  return puglDispatchSimpleEvent(view, PUGL_UNREALIZE);
}

PuglStatus
puglShow(PuglView*             PUGL_UNUSED(view),
         const PuglShowCommand PUGL_UNUSED(command))
{
  return PUGL_FAILURE;
}

PuglStatus
puglHide(PuglView* PUGL_UNUSED(view))
{
  return PUGL_FAILURE;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    free(view->impl);
  }
}

PuglStatus
puglGrabFocus(PuglView* PUGL_UNUSED(view))
{
  return PUGL_FAILURE;
}

bool
puglHasFocus(const PuglView* PUGL_UNUSED(view))
{
  return true;
}

PuglStatus
puglStartTimer(PuglView*       PUGL_UNUSED(view),
               const uintptr_t PUGL_UNUSED(id),
               const double    PUGL_UNUSED(timeout))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglStopTimer(PuglView* PUGL_UNUSED(view), const uintptr_t PUGL_UNUSED(id))
{
  return PUGL_FAILURE;
}

PuglStatus
puglSendEvent(PuglView* PUGL_UNUSED(view), const PuglEvent* PUGL_UNUSED(event))
{
  return PUGL_FAILURE;
}

PuglStatus
puglObscureView(PuglView* PUGL_UNUSED(view))
{
  return PUGL_FAILURE;
}

PuglStatus
puglObscureRegion(PuglView*      PUGL_UNUSED(view),
                  const int      PUGL_UNUSED(x),
                  const int      PUGL_UNUSED(y),
                  const unsigned PUGL_UNUSED(width),
                  const unsigned PUGL_UNUSED(height))
{
  return PUGL_FAILURE;
}

PuglNativeView
puglGetNativeView(const PuglView* PUGL_UNUSED(view))
{
  return 0U;
}

PuglStatus
puglApplyViewString(PuglView*            PUGL_UNUSED(view),
                    const PuglStringHint PUGL_UNUSED(key),
                    const char*          PUGL_UNUSED(value))
{
  return PUGL_FAILURE;
}

double
puglGetScaleFactor(const PuglView* PUGL_UNUSED(view))
{
  return 1.0;
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
puglSetTransientParent(PuglView*            PUGL_UNUSED(view),
                       const PuglNativeView PUGL_UNUSED(parent))
{
  return PUGL_UNSUPPORTED;
}

const void*
puglGetClipboard(PuglView*           PUGL_UNUSED(view),
                 const PuglClipboard PUGL_UNUSED(clipboard),
                 const uint32_t      PUGL_UNUSED(typeIndex),
                 size_t*             PUGL_UNUSED(len))
{
  return NULL;
}

PuglStatus
puglAcceptOffer(PuglView*                 PUGL_UNUSED(view),
                const PuglDataOfferEvent* PUGL_UNUSED(offer),
                const uint32_t            PUGL_UNUSED(typeIndex),
                PuglDataAction            PUGL_UNUSED(action),
                const int                 PUGL_UNUSED(regionX),
                const int                 PUGL_UNUSED(regionY),
                const unsigned            PUGL_UNUSED(regionWidth),
                const unsigned            PUGL_UNUSED(regionHeight))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglRejectOffer(PuglView*                 PUGL_UNUSED(view),
                const PuglDataOfferEvent* PUGL_UNUSED(offer),
                const int                 PUGL_UNUSED(regionX),
                const int                 PUGL_UNUSED(regionY),
                const unsigned            PUGL_UNUSED(regionWidth),
                const unsigned            PUGL_UNUSED(regionHeight))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglPaste(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglRegisterDropType(PuglView* PUGL_UNUSED(view), const char* PUGL_UNUSED(type))
{
  return PUGL_UNSUPPORTED;
}

uint32_t
puglGetNumClipboardTypes(const PuglView*     PUGL_UNUSED(view),
                         const PuglClipboard PUGL_UNUSED(clipboard))
{
  return 0U;
}

const char*
puglGetClipboardType(const PuglView*     PUGL_UNUSED(view),
                     const PuglClipboard PUGL_UNUSED(clipboard),
                     const uint32_t      PUGL_UNUSED(typeIndex))
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

PuglStatus
puglSetCursor(PuglView* const  PUGL_UNUSED(view),
              const PuglCursor PUGL_UNUSED(cursor))
{
  return PUGL_UNSUPPORTED;
}
