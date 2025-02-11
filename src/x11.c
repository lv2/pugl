// Copyright 2012-2023 David Robillard <d@drobilla.net>
// Copyright 2013 Robin Gareus <robin@gareus.org>
// Copyright 2011-2012 Ben Loftis, Harrison Consoles
// SPDX-License-Identifier: ISC

#include "x11.h"

#include "internal.h"
#include "macros.h"
#include "platform.h"
#include "types.h"

#include <pugl/pugl.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef USE_XRANDR
#  if __has_include(<X11/extensions/Xrandr.h>)
#    define USE_XRANDR 1
#  else
#    define USE_XRANDR 0
#  endif
#endif

#ifndef USE_XSYNC
#  if __has_include(<X11/extensions/sync.h>)
#    define USE_XSYNC 1
#  else
#    define USE_XSYNC 0
#  endif
#endif

#if USE_XRANDR
#  include <X11/extensions/Xrandr.h>
#endif

#if USE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
#endif

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
#  define PUGL_INIT_STRUCT \
    {                      \
    }
#else
#  define PUGL_INIT_STRUCT {0}
#endif

enum WmClientStateMessageAction {
  WM_STATE_REMOVE,
  WM_STATE_ADD,
  WM_STATE_TOGGLE
};

/**
   Convert a return code from an Xlib function to a PuglStatus.

   Careful, Xlib returns are wildly inconsistent, often undocumented, and
   sometimes even use misleading types (for example, a Status that is actually
   used as a boolean).  The only way to know for a given function is to read
   the source.  For the most part, though, being a remote display protocol,
   errors are handled asynchronously and return values mean nothing.
*/
static PuglStatus
puglX11Status(const int returnCode)
{
  return returnCode ? PUGL_SUCCESS : PUGL_UNKNOWN_ERROR;
}

PuglInternals*
puglInitViewInternals(PuglWorld* const world)
{
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  impl->clipboard.selection = world->impl->atoms.CLIPBOARD;
  impl->clipboard.property  = XA_PRIMARY;
  impl->cursorName          = "default";

  return impl;
}

static Atom
styleFlagToAtom(PuglWorld* const world, const PuglViewStyleFlag flag)
{
  const PuglX11Atoms* const atoms = &world->impl->atoms;

  switch (flag) {
  case PUGL_VIEW_STYLE_MODAL:
    return atoms->NET_WM_STATE_MODAL;
  case PUGL_VIEW_STYLE_TALL:
    return atoms->NET_WM_STATE_MAXIMIZED_VERT;
  case PUGL_VIEW_STYLE_WIDE:
    return atoms->NET_WM_STATE_MAXIMIZED_HORZ;
  case PUGL_VIEW_STYLE_HIDDEN:
    return atoms->NET_WM_STATE_HIDDEN;
  case PUGL_VIEW_STYLE_FULLSCREEN:
    return atoms->NET_WM_STATE_FULLSCREEN;
  case PUGL_VIEW_STYLE_ABOVE:
    return atoms->NET_WM_STATE_ABOVE;
  case PUGL_VIEW_STYLE_BELOW:
    return atoms->NET_WM_STATE_BELOW;
  case PUGL_VIEW_STYLE_DEMANDING:
    return atoms->NET_WM_STATE_DEMANDS_ATTENTION;
  case PUGL_VIEW_STYLE_RESIZING:
  case PUGL_VIEW_STYLE_MAPPED:
    break;
  }

  return 0;
}

static Atom
viewTypeToAtom(PuglWorld* const world, const PuglViewType type)
{
  const PuglX11Atoms* const atoms = &world->impl->atoms;

  switch (type) {
  case PUGL_VIEW_TYPE_NORMAL:
    return atoms->NET_WM_WINDOW_TYPE_NORMAL;
  case PUGL_VIEW_TYPE_UTILITY:
    return atoms->NET_WM_WINDOW_TYPE_UTILITY;
  case PUGL_VIEW_TYPE_DIALOG:
    return atoms->NET_WM_WINDOW_TYPE_DIALOG;
  }

  return 0;
}

PuglStatus
puglSetViewStyle(PuglView* const view, const PuglViewStyleFlags flags)
{
  PuglWorld* const          world    = view->world;
  PuglInternals* const      impl     = view->impl;
  Display* const            display  = view->world->impl->display;
  const PuglX11Atoms* const atoms    = &view->world->impl->atoms;
  const PuglViewStyleFlags  oldFlags = puglGetViewStyle(view);

  for (uint32_t mask = 1U; mask <= PUGL_MAX_VIEW_STYLE_FLAG; mask <<= 1U) {
    const Atom stateAtom = styleFlagToAtom(world, (PuglViewStyleFlag)mask);
    const bool oldValue  = oldFlags & mask;
    const bool newValue  = flags & mask;
    if (!stateAtom || oldValue == newValue) {
      continue;
    }

    if (stateAtom == atoms->NET_WM_STATE_HIDDEN) {
      // KDE annoyingly doesn't support clients setting the hidden hint
      XIconifyWindow(display, impl->win, impl->screen);
    } else {
      XEvent event               = {ClientMessage};
      event.xclient.window       = impl->win;
      event.xclient.format       = 32;
      event.xclient.message_type = atoms->NET_WM_STATE;
      event.xclient.data.l[0]    = newValue ? WM_STATE_ADD : WM_STATE_REMOVE;
      event.xclient.data.l[1]    = (long)stateAtom;
      event.xclient.data.l[2]    = 0;
      event.xclient.data.l[3]    = 1;
      event.xclient.data.l[4]    = 0;

      if (!XSendEvent(display,
                      RootWindow(display, impl->screen),
                      False,
                      SubstructureNotifyMask | SubstructureRedirectMask,
                      &event)) {
        return PUGL_UNKNOWN_ERROR;
      }
    }
  }

  return PUGL_SUCCESS;
}

static PuglStatus
updateSizeHints(const PuglView* const view)
{
  if (!view->impl->win) {
    return PUGL_SUCCESS;
  }

  Display*   display   = view->world->impl->display;
  XSizeHints sizeHints = PUGL_INIT_STRUCT;

  if (!view->hints[PUGL_RESIZABLE]) {
    PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    if (!puglIsValidSize(size.width, size.height)) {
      size = puglGetSizeHint(view, PUGL_DEFAULT_SIZE);
    }

    sizeHints.flags       = PBaseSize | PMinSize | PMaxSize;
    sizeHints.base_width  = (int)size.width;
    sizeHints.base_height = (int)size.height;
    sizeHints.min_width   = (int)size.width;
    sizeHints.min_height  = (int)size.height;
    sizeHints.max_width   = (int)size.width;
    sizeHints.max_height  = (int)size.height;
  } else {
    // Avoid setting PBaseSize for top level views to avoid window manager bugs
    const PuglArea defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
    if (puglIsValidArea(defaultSize) && view->parent) {
      sizeHints.flags |= PBaseSize;
      sizeHints.base_width  = defaultSize.width;
      sizeHints.base_height = defaultSize.height;
    }

    const PuglArea minSize = view->sizeHints[PUGL_MIN_SIZE];
    if (puglIsValidArea(minSize)) {
      sizeHints.flags |= PMinSize;
      sizeHints.min_width  = minSize.width;
      sizeHints.min_height = minSize.height;
    }

    const PuglArea maxSize = view->sizeHints[PUGL_MAX_SIZE];
    if (puglIsValidArea(maxSize)) {
      sizeHints.flags |= PMaxSize;
      sizeHints.max_width  = maxSize.width;
      sizeHints.max_height = maxSize.height;
    }

    const PuglArea minAspect = view->sizeHints[PUGL_MIN_ASPECT];
    const PuglArea maxAspect = view->sizeHints[PUGL_MAX_ASPECT];
    if (puglIsValidArea(minAspect) && puglIsValidArea(maxAspect)) {
      sizeHints.flags |= PAspect;
      sizeHints.min_aspect.x = minAspect.width;
      sizeHints.min_aspect.y = minAspect.height;
      sizeHints.max_aspect.x = maxAspect.width;
      sizeHints.max_aspect.y = maxAspect.height;
    }

    const PuglArea fixedAspect = view->sizeHints[PUGL_FIXED_ASPECT];
    if (puglIsValidArea(fixedAspect)) {
      sizeHints.flags |= PAspect;
      sizeHints.min_aspect.x = fixedAspect.width;
      sizeHints.min_aspect.y = fixedAspect.height;
      sizeHints.max_aspect.x = fixedAspect.width;
      sizeHints.max_aspect.y = fixedAspect.height;
    }
  }

  XSetWMNormalHints(display, view->impl->win, &sizeHints);
  return PUGL_SUCCESS;
}

PuglPoint
puglGetAncestorCenter(const PuglView* const view)
{
  Display* const    display       = view->world->impl->display;
  const int         screen        = view->impl->screen;
  XWindowAttributes ancestorAttrs = PUGL_INIT_STRUCT;
  XGetWindowAttributes(display,
                       view->transientParent ? (Window)view->transientParent
                                             : RootWindow(display, screen),
                       &ancestorAttrs);

  const PuglPoint center = {
    (PuglCoord)(ancestorAttrs.x + (ancestorAttrs.width / 2)),
    (PuglCoord)(ancestorAttrs.y + (ancestorAttrs.height / 2))};
  return center;
}

PuglStatus
puglRealize(PuglView* const view)
{
  PuglInternals* const impl    = view->impl;
  PuglWorld* const     world   = view->world;
  PuglX11Atoms* const  atoms   = &view->world->impl->atoms;
  Display* const       display = world->impl->display;
  const int            screen  = DefaultScreen(display);
  const Window         root    = RootWindow(display, screen);
  const Window         parent  = view->parent ? (Window)view->parent : root;
  XSetWindowAttributes attr    = PUGL_INIT_STRUCT;
  PuglStatus           st      = PUGL_SUCCESS;

  // Ensure that we're unrealized
  if (impl->win) {
    return PUGL_FAILURE;
  }

  // Check that the basic required configuration has been done
  if ((st = puglPreRealize(view))) {
    return st;
  }

  // Ensure hints that will be used are set to sensible values
  puglEnsureHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_FALSE);
  puglEnsureHint(view, PUGL_RESIZABLE, PUGL_TRUE);
  puglEnsureHint(view, PUGL_VIEW_TYPE, PUGL_VIEW_TYPE_NORMAL);

  // Configure the backend to get the visual info
  impl->screen = screen;
  if ((st = view->backend->configure(view)) || !impl->vi) {
    view->backend->destroy(view);
    return st ? st : PUGL_BACKEND_FAILED;
  }

  // Create a colormap based on the visual info from the backend
  attr.colormap = XCreateColormap(display, parent, impl->vi->visual, AllocNone);

  // Set the event mask to request all of the event types we react to
  attr.event_mask |= ButtonPressMask;
  attr.event_mask |= ButtonReleaseMask;
  attr.event_mask |= EnterWindowMask;
  attr.event_mask |= ExposureMask;
  attr.event_mask |= FocusChangeMask;
  attr.event_mask |= KeyPressMask;
  attr.event_mask |= KeyReleaseMask;
  attr.event_mask |= LeaveWindowMask;
  attr.event_mask |= PointerMotionMask;
  attr.event_mask |= PropertyChangeMask;
  attr.event_mask |= StructureNotifyMask;
  attr.event_mask |= VisibilityChangeMask;

  // Calculate the initial window frame
  const PuglArea  initialSize = puglGetInitialSize(view);
  const PuglPoint initialPos  = puglGetInitialPosition(view, initialSize);

  // Create the window
  impl->win = XCreateWindow(display,
                            parent,
                            initialPos.x,
                            initialPos.y,
                            initialSize.width,
                            initialSize.height,
                            0,
                            impl->vi->depth,
                            InputOutput,
                            impl->vi->visual,
                            CWColormap | CWEventMask,
                            &attr);

  // Create the backend drawing context/surface
  if ((st = view->backend->create(view))) {
    return st;
  }

  // Set window type
  if (view->hints[PUGL_VIEW_TYPE] != PUGL_DONT_CARE) {
    const PuglViewType viewType   = (PuglViewType)view->hints[PUGL_VIEW_TYPE];
    const Atom         windowType = viewTypeToAtom(world, viewType);
    XChangeProperty(display,
                    impl->win,
                    atoms->NET_WM_WINDOW_TYPE,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (const unsigned char*)&windowType,
                    1);
  }

#if USE_XRANDR
  int ignored = 0;
  if (XRRQueryExtension(display, &ignored, &ignored)) {
    // Set refresh rate hint to the real refresh rate
    XRRScreenConfiguration* conf         = XRRGetScreenInfo(display, parent);
    const short             current_rate = XRRConfigCurrentRate(conf);

    view->hints[PUGL_REFRESH_RATE] = current_rate;
    XRRFreeScreenConfigInfo(conf);
  }
#endif

  // Set basic window hints and attributes
  char* const className = world->strings[PUGL_CLASS_NAME];
  XClassHint  classHint = {className, className};
  XSetClassHint(display, impl->win, &classHint);
  puglSetViewString(view, PUGL_WINDOW_TITLE, view->strings[PUGL_WINDOW_TITLE]);
  puglSetTransientParent(view, view->transientParent);
  updateSizeHints(view);

  // Set PID and hostname so the window manager can access our process
  char       hostname[256] = PUGL_INIT_STRUCT;
  const long pid           = (long)getpid();
  if (pid > 0 && !gethostname(hostname, sizeof(hostname))) {
    hostname[sizeof(hostname) - 1] = '\0';
    XChangeProperty(display,
                    impl->win,
                    atoms->WM_CLIENT_MACHINE,
                    XA_STRING,
                    8,
                    PropModeReplace,
                    (const uint8_t*)hostname,
                    (int)strlen(hostname));

    XChangeProperty(display,
                    impl->win,
                    atoms->NET_WM_PID,
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (const uint8_t*)&pid,
                    1);
  }

  // Set supported WM protocols
  Atom protocols[] = {atoms->NET_WM_PING, atoms->WM_DELETE_WINDOW};
  XSetWMProtocols(display, impl->win, protocols, (parent == root) ? 2 : 1);

  // Create input context
  if (world->impl->xim) {
    impl->xic = XCreateIC(world->impl->xim,
                          XNInputStyle,
                          XIMPreeditNothing | XIMStatusNothing,
                          XNClientWindow,
                          impl->win,
                          XNFocusWindow,
                          impl->win,
                          (XIM)0);
  }

  st = puglDispatchSimpleEvent(view, PUGL_REALIZE);

  /* Flush before returning for two reasons: so that hints are available to the
     view's parent via the X server during embedding, and so that the X server
     has a chance to create the window (and make its actual position and size
     known) before any children are created.  Potential bugs aside, this
     increases the chances that an application will be cleanly configured once
     on startup with the correct position and size. */
  XFlush(display);
  return st;
}

PuglStatus
puglUnrealize(PuglView* const view)
{
  PuglInternals* const impl = view->impl;
  if (!impl || !impl->win) {
    return PUGL_FAILURE;
  }

  puglDispatchSimpleEvent(view, PUGL_UNREALIZE);
  clearX11Clipboard(&impl->clipboard);

  if (impl->xic) {
    XDestroyIC(impl->xic);
    impl->xic = None;
  }

  if (view->backend) {
    view->backend->destroy(view);
  }

  if (view->world->impl->display && impl->win) {
    XDestroyWindow(view->world->impl->display, impl->win);
    impl->win = None;
  }

  XFree(impl->vi);
  impl->vi = NULL;

  memset(&view->lastConfigure, 0, sizeof(PuglConfigureEvent));
  memset(&view->impl->pendingConfigure, 0, sizeof(PuglEvent));
  memset(&view->impl->pendingExpose, 0, sizeof(PuglEvent));

  if (impl->mapped) {
    view->impl->pendingConfigure.configure.style |= PUGL_VIEW_STYLE_MAPPED;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view, const PuglShowCommand command)
{
  PuglStatus st = view->impl->win ? PUGL_SUCCESS : puglRealize(view);

  if (!st) {
    switch (command) {
    case PUGL_SHOW_PASSIVE:
      XMapWindow(view->world->impl->display, view->impl->win);
      break;
    case PUGL_SHOW_RAISE:
    case PUGL_SHOW_FORCE_RAISE:
      XMapRaised(view->world->impl->display, view->impl->win);
      break;
    }

    if (view->stage == PUGL_VIEW_STAGE_CONFIGURED) {
      st = puglObscureView(view);
    }
  }

  return st;
}

PuglStatus
puglHide(PuglView* const view)
{
  XUnmapWindow(view->world->impl->display, view->impl->win);
  return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    puglUnrealize(view);
    free(view->impl->clipboard.data.data);
    free(view->impl->clipboard.formats);
    free(view->impl->clipboard.formatStrings);
    free(view->impl);
  }
}

PuglStatus
getAtomProperty(PuglView* const view,
                const Window    window,
                const Atom      property,
                unsigned long*  numValues,
                Atom**          values)
{
  Atom          actualType   = 0;
  int           actualFormat = 0;
  unsigned long bytesAfter   = 0;

  return (XGetWindowProperty(view->world->impl->display,
                             window,
                             property,
                             0,
                             LONG_MAX,
                             False,
                             XA_ATOM,
                             &actualType,
                             &actualFormat,
                             numValues,
                             &bytesAfter,
                             (unsigned char**)values) == Success)
           ? PUGL_SUCCESS
           : PUGL_FAILURE;
}

PuglStatus
puglGrabFocus(PuglView* const view)
{
  PuglInternals* const impl    = view->impl;
  Display* const       display = view->world->impl->display;
  XWindowAttributes    attrs   = PUGL_INIT_STRUCT;

  if (!impl->win || !XGetWindowAttributes(display, impl->win, &attrs)) {
    return PUGL_UNKNOWN_ERROR;
  }

  if (attrs.map_state != IsViewable) {
    return PUGL_FAILURE;
  }

  XSetInputFocus(display, impl->win, RevertToNone, CurrentTime);
  return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* const view)
{
  int    revertTo      = 0;
  Window focusedWindow = 0;
  XGetInputFocus(view->world->impl->display, &focusedWindow, &revertTo);
  return focusedWindow == view->impl->win;
}

PuglStatus
puglStartTimer(PuglView* const view, const uintptr_t id, const double timeout)
{
#if USE_XSYNC
  if (view->world->impl->syncSupported) {
    XSyncValue value;
    XSyncIntToValue(&value, (int)floor(timeout * 1000.0));

    PuglWorldInternals*  w       = view->world->impl;
    Display* const       display = w->display;
    const XSyncCounter   counter = w->serverTimeCounter;
    const XSyncTestType  type    = XSyncPositiveTransition;
    const XSyncTrigger   trigger = {counter, XSyncRelative, value, type};
    XSyncAlarmAttributes attr    = {trigger, value, True, XSyncAlarmActive};
    const XSyncAlarm     alarm   = XSyncCreateAlarm(display, 0x17, &attr);
    const PuglTimer      timer   = {alarm, view, id};

    if (alarm != None) {
      for (size_t i = 0; i < w->numTimers; ++i) {
        if (w->timers[i].view == view && w->timers[i].id == id) {
          // Replace existing timer
          XSyncDestroyAlarm(w->display, w->timers[i].alarm);
          w->timers[i] = timer;
          return PUGL_SUCCESS;
        }
      }

      // Add new timer
      const size_t     size      = ++w->numTimers * sizeof(timer);
      PuglTimer* const newTimers = (PuglTimer*)realloc(w->timers, size);
      if (newTimers) {
        w->timers                   = newTimers;
        w->timers[w->numTimers - 1] = timer;
        return PUGL_SUCCESS;
      }
    }
  }
#else
  (void)view;
  (void)id;
  (void)timeout;
#endif

  return PUGL_FAILURE;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
#if USE_XSYNC
  PuglWorldInternals* w = view->world->impl;

  for (size_t i = 0; i < w->numTimers; ++i) {
    if (w->timers[i].view == view && w->timers[i].id == id) {
      XSyncDestroyAlarm(w->display, w->timers[i].alarm);

      if (i != w->numTimers - 1) {
        memmove(w->timers + i,
                w->timers + i + 1,
                sizeof(PuglTimer) * (w->numTimers - i - 1));
      }

      memset(&w->timers[w->numTimers - 1], 0, sizeof(PuglTimer));

      --w->numTimers;
      return PUGL_SUCCESS;
    }
  }
#else
  (void)view;
  (void)id;
#endif

  return PUGL_FAILURE;
}

static XEvent
eventToX(PuglView* const view, const PuglEvent* const event)
{
  XEvent xev          = PUGL_INIT_STRUCT;
  xev.xany.send_event = True;

  switch (event->type) {
  case PUGL_EXPOSE: {
    const double x = floor(event->expose.x);
    const double y = floor(event->expose.y);
    const double w = ceil(event->expose.x + event->expose.width) - x;
    const double h = ceil(event->expose.y + event->expose.height) - y;

    xev.xexpose.type    = Expose;
    xev.xexpose.serial  = 0;
    xev.xexpose.display = view->world->impl->display;
    xev.xexpose.window  = view->impl->win;
    xev.xexpose.x       = (int)x;
    xev.xexpose.y       = (int)y;
    xev.xexpose.width   = (int)w;
    xev.xexpose.height  = (int)h;
    break;
  }

  case PUGL_CLIENT:
    xev.xclient.type         = ClientMessage;
    xev.xclient.serial       = 0;
    xev.xclient.send_event   = True;
    xev.xclient.display      = view->world->impl->display;
    xev.xclient.window       = view->impl->win;
    xev.xclient.message_type = view->world->impl->atoms.PUGL_CLIENT_MSG;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = (long)event->client.data1;
    xev.xclient.data.l[1]    = (long)event->client.data2;
    break;

  default:
    break;
  }

  return xev;
}

PuglStatus
puglSendEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglInternals* const impl    = view->impl;
  Display* const       display = view->world->impl->display;
  XEvent               xev     = PUGL_INIT_STRUCT;
  if (!impl->win) {
    return PUGL_FAILURE;
  }

  if (event->type == PUGL_CLOSE) {
    xev.xclient.type         = ClientMessage;
    xev.xclient.serial       = 0;
    xev.xclient.send_event   = True;
    xev.xclient.display      = display;
    xev.xclient.window       = impl->win;
    xev.xclient.message_type = view->world->impl->atoms.NET_CLOSE_WINDOW;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = CurrentTime;
    xev.xclient.data.l[1]    = 1;

    return puglX11Status(
      XSendEvent(display,
                 RootWindow(display, impl->screen),
                 False,
                 SubstructureNotifyMask | SubstructureRedirectMask,
                 &xev));
  }

  xev = eventToX(view, event);
  if (xev.type) {
    return puglX11Status(XSendEvent(display, impl->win, False, 0, &xev));
  }

  return PUGL_UNSUPPORTED;
}

void
mergeExposeEvents(PuglExposeEvent* const dst, const PuglExposeEvent* const src)
{
  if (!dst->type) {
    if (src->width && src->height) {
      *dst = *src;
    }
  } else {
    const int dst_r = dst->x + dst->width;
    const int src_r = src->x + src->width;
    const int max_x = MAX(dst_r, src_r);
    const int dst_b = dst->y + dst->height;
    const int src_b = src->y + src->height;
    const int max_y = MAX(dst_b, src_b);

    dst->x      = (PuglCoord)MIN(dst->x, src->x);
    dst->y      = (PuglCoord)MIN(dst->y, src->y);
    dst->width  = (PuglSpan)(max_x - dst->x);
    dst->height = (PuglSpan)(max_y - dst->y);
  }
}

PuglStatus
puglObscureView(PuglView* const view)
{
  return puglObscureRegion(
    view, 0, 0, view->lastConfigure.width, view->lastConfigure.height);
}

PuglStatus
puglObscureRegion(PuglView* const view,
                  const int       x,
                  const int       y,
                  const unsigned  width,
                  const unsigned  height)
{
  if (!puglIsValidPosition(x, y) || !puglIsValidSize(width, height)) {
    return PUGL_BAD_PARAMETER;
  }

  const PuglCoord cx = MAX((PuglCoord)0, (PuglCoord)x);
  const PuglCoord cy = MAX((PuglCoord)0, (PuglCoord)y);
  const PuglSpan  cw = MIN(view->lastConfigure.width, (PuglSpan)width);
  const PuglSpan  ch = MIN(view->lastConfigure.height, (PuglSpan)height);

  const PuglExposeEvent event = {PUGL_EXPOSE, 0U, cx, cy, cw, ch};

  if (view->world->impl->dispatchingEvents) {
    // Currently dispatching events, add/expand expose for the loop end
    mergeExposeEvents(&view->impl->pendingExpose.expose, &event);
  } else if (view->impl->win) {
    // Not dispatching events, send an X expose so we wake up next time
    PuglEvent exposeEvent = {{PUGL_EXPOSE, 0U}};
    exposeEvent.expose    = event;
    return puglSendEvent(view, &exposeEvent);
  }

  return PUGL_SUCCESS;
}

PuglNativeView
puglGetNativeView(PuglView* const view)
{
  return (PuglNativeView)view->impl->win;
}

PuglStatus
puglViewStringChanged(PuglView* const      view,
                      const PuglStringHint key,
                      const char* const    value)
{
  Display* const      display = view->world->impl->display;
  const PuglX11Atoms* atoms   = &view->world->impl->atoms;

  if (!view->impl->win) {
    return PUGL_SUCCESS;
  }

  switch (key) {
  case PUGL_CLASS_NAME:
    break;

  case PUGL_WINDOW_TITLE:
    if (value) {
      XStoreName(display, view->impl->win, value);
      XChangeProperty(display,
                      view->impl->win,
                      atoms->NET_WM_NAME,
                      atoms->UTF8_STRING,
                      8,
                      PropModeReplace,
                      (const uint8_t*)value,
                      (int)strlen(value));
    }
    break;
  }

  return PUGL_SUCCESS;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return view->world->impl->scaleFactor;
}

static PuglStatus
puglSetWindowPosition(PuglView* const view, const int x, const int y)
{
  return puglX11Status(XMoveWindow(view->world->impl->display,
                                   view->impl->win,
                                   (int)(x - view->impl->frameExtentLeft),
                                   (int)(y - view->impl->frameExtentTop)));
}

static PuglStatus
puglSetWindowSize(PuglView* const view,
                  const unsigned  width,
                  const unsigned  height)
{
  return !view->impl->win
           ? PUGL_SUCCESS
           : puglX11Status(XResizeWindow(
               view->world->impl->display, view->impl->win, width, height));
}

PuglStatus
puglSetPositionHint(PuglView* const        view,
                    const PuglPositionHint hint,
                    const int              x,
                    const int              y)
{
  if (x <= INT16_MIN || x > INT16_MAX || y <= INT16_MIN || y > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  view->positionHints[hint].x = (PuglCoord)x;
  view->positionHints[hint].y = (PuglCoord)y;

  return (hint == PUGL_CURRENT_POSITION) ? puglSetWindowPosition(view, x, y)
                                         : PUGL_SUCCESS;
}

PuglStatus
puglSetSizeHint(PuglView* const    view,
                const PuglSizeHint hint,
                const unsigned     width,
                const unsigned     height)
{
  const PuglStatus st = puglStoreSizeHint(view, hint, width, height);

  return st                            ? st
         : (hint == PUGL_CURRENT_SIZE) ? puglSetWindowSize(view, width, height)
                                       : updateSizeHints(view);
}

PuglStatus
puglSetTransientParent(PuglView* const view, const PuglNativeView parent)
{
  Display* display = view->world->impl->display;

  if (view->parent) {
    return PUGL_FAILURE;
  }

  view->transientParent = parent;

  if (view->impl->win && view->transientParent) {
    XSetTransientForHint(
      display, view->impl->win, (Window)view->transientParent);
  }

  return PUGL_SUCCESS;
}

// Semi-public platform API used by backends

PuglStatus
puglX11Configure(PuglView* view)
{
  PuglInternals* const impl    = view->impl;
  Display* const       display = view->world->impl->display;
  XVisualInfo          pat     = PUGL_INIT_STRUCT;
  int                  n       = 0;

  pat.screen = impl->screen;
  if (!(impl->vi = XGetVisualInfo(display, VisualScreenMask, &pat, &n))) {
    return PUGL_BAD_CONFIGURATION;
  }

  view->hints[PUGL_RED_BITS]   = impl->vi->bits_per_rgb;
  view->hints[PUGL_GREEN_BITS] = impl->vi->bits_per_rgb;
  view->hints[PUGL_BLUE_BITS]  = impl->vi->bits_per_rgb;
  view->hints[PUGL_ALPHA_BITS] = 0;

  return PUGL_SUCCESS;
}
