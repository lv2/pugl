// Copyright 2012-2023 David Robillard <d@drobilla.net>
// Copyright 2013 Robin Gareus <robin@gareus.org>
// Copyright 2011-2012 Ben Loftis, Harrison Consoles
// SPDX-License-Identifier: ISC

#include "attributes.h"
#include "internal.h"
#include "platform.h"
#include "types.h"
#include "x11.h"

#include <pugl/pugl.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifndef USE_XSYNC
#  if __has_include(<X11/extensions/sync.h>)
#    define USE_XSYNC 1
#  else
#    define USE_XSYNC 0
#  endif
#endif

#if USE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
#endif

#include <sys/select.h>

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
#  define PUGL_INIT_STRUCT \
    {                      \
    }
#else
#  define PUGL_INIT_STRUCT {0}
#endif

static bool
initXSync(PuglWorldInternals* const impl)
{
#if USE_XSYNC
  int                 syncMajor   = 0;
  int                 syncMinor   = 0;
  int                 errorBase   = 0;
  XSyncSystemCounter* counters    = NULL;
  int                 numCounters = 0;

  if (XSyncQueryExtension(impl->display, &impl->syncEventBase, &errorBase) &&
      XSyncInitialize(impl->display, &syncMajor, &syncMinor) &&
      (counters = XSyncListSystemCounters(impl->display, &numCounters))) {
    for (int n = 0; n < numCounters; ++n) {
      if (!strcmp(counters[n].name, "SERVERTIME")) {
        impl->serverTimeCounter = counters[n].counter;
        impl->syncSupported     = true;
        break;
      }
    }

    XSyncFreeSystemCounterList(counters);
  }
#else
  (void)impl;
#endif

  return false;
}

static double
puglX11GetDisplayScaleFactor(Display* const display)
{
  double            dpi = 96.0;
  const char* const rms = XResourceManagerString(display);
  if (rms) {
    XrmDatabase db = XrmGetStringDatabase(rms);
    if (db) {
      XrmValue value = {0U, NULL};
      char*    type  = NULL;
      if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
        if (!type || !strcmp(type, "String")) {
          char*        end    = NULL;
          const double xftDpi = strtod(value.addr, &end);
          if (xftDpi > 0.0 && xftDpi < HUGE_VAL) {
            dpi = xftDpi;
          }
        }
      }

      XrmDestroyDatabase(db);
    }
  }

  return dpi / 96.0;
}

PuglWorldInternals*
puglInitWorldInternals(const PuglWorldType type, const PuglWorldFlags flags)
{
  if (type == PUGL_PROGRAM && (flags & PUGL_WORLD_THREADS)) {
    XInitThreads();
  }

  Display* display = XOpenDisplay(NULL);
  if (!display) {
    return NULL;
  }

  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  impl->display     = display;
  impl->scaleFactor = puglX11GetDisplayScaleFactor(display);

  // Intern the various atoms we'll need

  impl->atoms.CLIPBOARD         = XInternAtom(display, "CLIPBOARD", 0);
  impl->atoms.UTF8_STRING       = XInternAtom(display, "UTF8_STRING", 0);
  impl->atoms.WM_CLIENT_MACHINE = XInternAtom(display, "WM_CLIENT_MACHINE", 0);
  impl->atoms.WM_PROTOCOLS      = XInternAtom(display, "WM_PROTOCOLS", 0);
  impl->atoms.WM_DELETE_WINDOW  = XInternAtom(display, "WM_DELETE_WINDOW", 0);
  impl->atoms.PUGL_CLIENT_MSG   = XInternAtom(display, "_PUGL_CLIENT_MSG", 0);
  impl->atoms.NET_CLOSE_WINDOW  = XInternAtom(display, "_NET_CLOSE_WINDOW", 0);
  impl->atoms.NET_FRAME_EXTENTS = XInternAtom(display, "_NET_FRAME_EXTENTS", 0);
  impl->atoms.NET_WM_NAME       = XInternAtom(display, "_NET_WM_NAME", 0);
  impl->atoms.NET_WM_PID        = XInternAtom(display, "_NET_WM_PID", 0);
  impl->atoms.NET_WM_PING       = XInternAtom(display, "_NET_WM_PING", 0);
  impl->atoms.NET_WM_STATE      = XInternAtom(display, "_NET_WM_STATE", 0);

  impl->atoms.NET_WM_STATE_ABOVE =
    XInternAtom(display, "_NET_WM_STATE_ABOVE", 0);
  impl->atoms.NET_WM_STATE_BELOW =
    XInternAtom(display, "_NET_WM_STATE_BELOW", 0);
  impl->atoms.NET_WM_STATE_DEMANDS_ATTENTION =
    XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);
  impl->atoms.NET_WM_STATE_FULLSCREEN =
    XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);
  impl->atoms.NET_WM_STATE_HIDDEN =
    XInternAtom(display, "_NET_WM_STATE_HIDDEN", 0);
  impl->atoms.NET_WM_STATE_MAXIMIZED_HORZ =
    XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
  impl->atoms.NET_WM_STATE_MAXIMIZED_VERT =
    XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
  impl->atoms.NET_WM_STATE_MODAL =
    XInternAtom(display, "_NET_WM_STATE_MODAL", 0);
  impl->atoms.NET_WM_WINDOW_TYPE =
    XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0);
  impl->atoms.NET_WM_WINDOW_TYPE_DIALOG =
    XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
  impl->atoms.NET_WM_WINDOW_TYPE_NORMAL =
    XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", 0);
  impl->atoms.NET_WM_WINDOW_TYPE_UTILITY =
    XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", 0);

  impl->atoms.TARGETS       = XInternAtom(display, "TARGETS", 0);
  impl->atoms.text_uri_list = XInternAtom(display, "text/uri-list", 0);

  // Open input method
  XSetLocaleModifiers("");
  if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
    XSetLocaleModifiers("@im=");
    impl->xim = XOpenIM(display, NULL, NULL, NULL);
  }

  XrmInitialize();
  initXSync(impl);
  XFlush(display);

  return impl;
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  if (world->impl->xim) {
    XCloseIM(world->impl->xim);
  }
  XCloseDisplay(world->impl->display);
  free(world->impl->timers);
  free(world->impl);
}

void*
puglGetNativeWorld(PuglWorld* const world)
{
  return world->impl->display;
}

static PuglStatus
pollX11Socket(PuglWorld* const world, const double timeout)
{
  if (XPending(world->impl->display) > 0) {
    return PUGL_SUCCESS;
  }

  const int fd   = ConnectionNumber(world->impl->display);
  const int nfds = fd + 1;
  int       ret  = 0;
  fd_set    fds;
  FD_ZERO(&fds); // NOLINT
  FD_SET(fd, &fds);

  if (timeout < 0.0) {
    ret = select(nfds, &fds, NULL, NULL, NULL);
  } else {
    const long     sec  = (long)timeout;
    const long     usec = (long)((timeout - (double)sec) * 1e6);
    struct timeval tv   = {sec, usec};
    ret                 = select(nfds, &fds, NULL, NULL, &tv);
  }

  return ret < 0 ? PUGL_UNKNOWN_ERROR : PUGL_SUCCESS;
}

static PuglView*
findView(PuglWorld* const world, const Window window)
{
  for (size_t i = 0; i < world->numViews; ++i) {
    if (world->views[i]->impl->win == window) {
      return world->views[i];
    }
  }

  return NULL;
}

static PuglKey
keyInRange(const KeySym  xSym,
           const KeySym  xMin,
           const KeySym  xMax,
           const PuglKey puglMin)
{
  return (xSym >= xMin && xSym <= xMax) ? (PuglKey)(puglMin + (xSym - xMin))
                                        : PUGL_KEY_NONE;
}

static PuglKey
keySymToSpecial(const KeySym sym)
{
  PuglKey key = PUGL_KEY_NONE;
  if ((key = keyInRange(sym, XK_F1, XK_F12, PUGL_KEY_F1)) ||
      (key = keyInRange(sym, XK_Page_Up, XK_End, PUGL_KEY_PAGE_UP)) ||
      (key = keyInRange(sym, XK_Home, XK_Down, PUGL_KEY_HOME)) ||
      (key = keyInRange(sym, XK_Shift_L, XK_Control_R, PUGL_KEY_SHIFT_L)) ||
      (key = keyInRange(sym, XK_Alt_L, XK_Super_R, PUGL_KEY_ALT_L)) ||
      (key = keyInRange(sym, XK_KP_Home, XK_KP_Down, PUGL_KEY_PAD_HOME)) ||
      (key = keyInRange(sym, XK_KP_0, XK_KP_9, PUGL_KEY_PAD_0)) ||
      (key = keyInRange(sym, XK_KP_Begin, XK_KP_Delete, PUGL_KEY_PAD_CLEAR)) ||
      (key = keyInRange(
         sym, XK_KP_Multiply, XK_KP_Divide, PUGL_KEY_PAD_MULTIPLY))) {
    return key;
  }

  // clang-format off
  switch (sym) {
  case XK_ISO_Level3_Shift: return PUGL_KEY_ALT_R;
  case XK_Pause:            return PUGL_KEY_PAUSE;
  case XK_Scroll_Lock:      return PUGL_KEY_SCROLL_LOCK;
  case XK_Print:            return PUGL_KEY_PRINT_SCREEN;
  case XK_Insert:           return PUGL_KEY_INSERT;
  case XK_Menu:             return PUGL_KEY_MENU;
  case XK_Num_Lock:         return PUGL_KEY_NUM_LOCK;
  case XK_KP_Enter:         return PUGL_KEY_PAD_ENTER;
  case XK_KP_Page_Up:       return PUGL_KEY_PAD_PAGE_UP;
  case XK_KP_Page_Down:     return PUGL_KEY_PAD_PAGE_DOWN;
  case XK_KP_End:           return PUGL_KEY_PAD_END;
  case XK_KP_Equal:         return PUGL_KEY_PAD_CLEAR;
  case XK_Caps_Lock:        return PUGL_KEY_CAPS_LOCK;
  default: break;
  }
  // clang-format on

  return PUGL_KEY_NONE;
}

static int
lookupString(XIC xic, XEvent* const xevent, char* const str, KeySym* const sym)
{
  Status status = 0;

#ifdef X_HAVE_UTF8_STRING
  const int n = Xutf8LookupString(xic, &xevent->xkey, str, 7, sym, &status);
#else
  const int n = XmbLookupString(xic, &xevent->xkey, str, 7, sym, &status);
#endif

  return status == XBufferOverflow ? 0 : n;
}

static void
translateKey(PuglView* const view, XEvent* const xevent, PuglEvent* const event)
{
  const unsigned state  = xevent->xkey.state;
  const bool     filter = XFilterEvent(xevent, None);

  event->key.keycode = xevent->xkey.keycode;

  // Mask off the control and shift bits to get the lowercase "main" symbol
  xevent->xkey.state =
    xevent->xkey.state & ~(unsigned)(ControlMask | ShiftMask);

  // Lookup unshifted key
  char          ustr[8] = PUGL_INIT_STRUCT;
  KeySym        sym     = 0;
  const int     ufound  = XLookupString(&xevent->xkey, ustr, 8, &sym, NULL);
  const PuglKey special = keySymToSpecial(sym);
  if (special) {
    event->key.state = puglFilterMods(event->key.state, special);
    event->key.key   = special;
  } else if (ufound > 0) {
    event->key.key = (PuglKey)puglDecodeUTF8((const uint8_t*)ustr);
  }

  if (xevent->type == KeyPress && !filter && !(special && ufound <= 0) &&
      view->impl->xic) {
    // Lookup shifted key for possible text event
    xevent->xkey.state = state;

    char      sstr[8] = PUGL_INIT_STRUCT;
    const int sfound  = lookupString(view->impl->xic, xevent, sstr, &sym);
    if (sfound > 0) {
      // Dispatch key event now
      puglDispatchEvent(view, event);

      // "Return" a text event in its place
      event->text.type      = PUGL_TEXT;
      event->text.character = puglDecodeUTF8((const uint8_t*)sstr);
      memcpy(event->text.string, sstr, sizeof(sstr));
    }
  }
}

static uint32_t
translateModifiers(const unsigned xstate)
{
  return (((xstate & ShiftMask) ? (uint32_t)PUGL_MOD_SHIFT : 0U) |
          ((xstate & ControlMask) ? (uint32_t)PUGL_MOD_CTRL : 0U) |
          ((xstate & Mod1Mask) ? (uint32_t)PUGL_MOD_ALT : 0U) |
          ((xstate & Mod4Mask) ? (uint32_t)PUGL_MOD_SUPER : 0U) |
          ((xstate & Mod2Mask) ? (uint32_t)PUGL_MOD_NUM_LOCK : 0U) |
          ((xstate & Mod3Mask) ? (uint32_t)PUGL_MOD_SCROLL_LOCK : 0U) |
          ((xstate & LockMask) ? (uint32_t)PUGL_MOD_CAPS_LOCK : 0U));
}

static PuglEvent
translateClientMessage(PuglView* const view, XClientMessageEvent message)
{
  Display* const            display = view->world->impl->display;
  const PuglX11Atoms* const atoms   = &view->world->impl->atoms;
  PuglEvent                 event   = {{PUGL_NOTHING, 0U}};

  if (message.message_type == atoms->WM_PROTOCOLS) {
    const Atom protocol = (Atom)message.data.l[0];
    if (protocol == atoms->WM_DELETE_WINDOW) {
      event.type = PUGL_CLOSE;
    } else if (protocol == atoms->NET_WM_PING) {
      const Window root    = RootWindow(display, view->impl->screen);
      XEvent       reply   = {ClientMessage};
      reply.xclient        = message;
      reply.xclient.window = root;
      XSendEvent(display,
                 root,
                 False,
                 SubstructureNotifyMask | SubstructureRedirectMask,
                 &reply);
    }
  } else if (message.message_type == atoms->PUGL_CLIENT_MSG) {
    event.type         = PUGL_CLIENT;
    event.client.data1 = (uintptr_t)message.data.l[0];
    event.client.data2 = (uintptr_t)message.data.l[1];
  }

  return event;
}

static PuglViewStyleFlags
getCurrentViewStyleFlags(PuglView* const view)
{
  const PuglX11Atoms* const atoms = &view->world->impl->atoms;

  unsigned long      numHints = 0;
  Atom*              hints    = NULL;
  PuglViewStyleFlags state    = 0U;
  if (!getAtomProperty(
        view, view->impl->win, atoms->NET_WM_STATE, &numHints, &hints)) {
    for (unsigned long i = 0; i < numHints; ++i) {
      if (hints[i] == atoms->NET_WM_STATE_MAXIMIZED_VERT) {
        state |= PUGL_VIEW_STYLE_TALL;
      } else if (hints[i] == atoms->NET_WM_STATE_MAXIMIZED_HORZ) {
        state |= PUGL_VIEW_STYLE_WIDE;
      } else if (hints[i] == atoms->NET_WM_STATE_HIDDEN) {
        state |= PUGL_VIEW_STYLE_HIDDEN;
      } else if (hints[i] == atoms->NET_WM_STATE_FULLSCREEN) {
        state |= PUGL_VIEW_STYLE_FULLSCREEN;
      } else if (hints[i] == atoms->NET_WM_STATE_MODAL) {
        state |= PUGL_VIEW_STYLE_MODAL;
      } else if (hints[i] == atoms->NET_WM_STATE_ABOVE) {
        state |= PUGL_VIEW_STYLE_ABOVE;
      } else if (hints[i] == atoms->NET_WM_STATE_BELOW) {
        state |= PUGL_VIEW_STYLE_BELOW;
      } else if (hints[i] == atoms->NET_WM_STATE_DEMANDS_ATTENTION) {
        state |= PUGL_VIEW_STYLE_DEMANDING;
      }
    }
  }

  if (view->impl->mapped) {
    state |= PUGL_VIEW_STYLE_MAPPED;
  }

  return state;
}

static PuglEvent
getCurrentConfiguration(PuglView* const view)
{
  Display* const display = view->world->impl->display;

  // Get window size from attributes
  XWindowAttributes attrs;
  XGetWindowAttributes(display, view->impl->win, &attrs);

  // Get window position (relative to the root window if not a child)
  Window ignoredChild = 0;
  int    x            = attrs.x;
  int    y            = attrs.y;
  if (!view->parent) {
    XTranslateCoordinates(
      display, view->impl->win, attrs.root, 0, 0, &x, &y, &ignoredChild);
  }

  // Build a configure event based on the current window configuration
  PuglEvent configureEvent        = {{PUGL_CONFIGURE, 0U}};
  configureEvent.configure.x      = (PuglCoord)x;
  configureEvent.configure.y      = (PuglCoord)y;
  configureEvent.configure.width  = (PuglSpan)attrs.width;
  configureEvent.configure.height = (PuglSpan)attrs.height;
  configureEvent.configure.style  = getCurrentViewStyleFlags(view);

  return configureEvent;
}

static PuglEvent
makeConfigureEvent(PuglView* const view)
{
  PuglEvent event = view->impl->pendingConfigure;

  if (event.type != PUGL_CONFIGURE) {
    event = getCurrentConfiguration(view);
  } else if (view->impl->mapped) {
    event.configure.style |= PUGL_VIEW_STYLE_MAPPED;
  } else {
    event.configure.style &= ~(PuglViewStyleFlags)PUGL_VIEW_STYLE_MAPPED;
  }

  return event;
}

static PuglEvent
translatePropertyNotify(PuglView* const view, XPropertyEvent message)
{
  const PuglInternals* const impl  = view->impl;
  const PuglX11Atoms* const  atoms = &view->world->impl->atoms;
  PuglEvent                  event = {{PUGL_NOTHING, 0U}};

  if (message.atom == atoms->NET_WM_STATE) {
    // Get all the current states set in the window hints
    unsigned long numHints = 0;
    Atom*         hints    = NULL;
    if (getAtomProperty(view, impl->win, message.atom, &numHints, &hints)) {
      return event;
    }

    // Make a configure event based on the current configuration to update
    event                 = makeConfigureEvent(view);
    event.configure.style = getCurrentViewStyleFlags(view); // FIXME: necessary?

    XFree(hints);
  } else if (message.atom == atoms->NET_FRAME_EXTENTS) {
    Atom          actualType     = 0;
    int           actualFormat   = 0;
    unsigned long actualNumItems = 0U;
    unsigned long bytesAfter     = 0;
    long*         extents        = NULL;
    XGetWindowProperty(view->world->impl->display,
                       impl->win,
                       message.atom,
                       0,
                       LONG_MAX,
                       False,
                       XA_CARDINAL,
                       &actualType,
                       &actualFormat,
                       &actualNumItems,
                       &bytesAfter,
                       (uint8_t**)&extents);

    if (actualNumItems == 4) {
      view->impl->frameExtentLeft = extents[0];
      view->impl->frameExtentTop  = extents[2];
    }

    if (extents) {
      XFree(extents);
    }
  }

  return event;
}

static PuglEvent
translateEvent(PuglView* const view, XEvent xevent)
{
  PuglEvent event = {{PUGL_NOTHING, 0U}};
  event.any.flags = xevent.xany.send_event ? PUGL_IS_SEND_EVENT : 0;

  switch (xevent.type) {
  case ClientMessage:
    event = translateClientMessage(view, xevent.xclient);
    break;
  case PropertyNotify:
    event = translatePropertyNotify(view, xevent.xproperty);
    break;
  case VisibilityNotify:
    event = makeConfigureEvent(view);
    break;
  case MapNotify:
    view->impl->mapped = true;
    event              = makeConfigureEvent(view);
    break;
  case UnmapNotify:
    view->impl->mapped = false;
    event              = makeConfigureEvent(view);
    break;
  case ConfigureNotify:
    event                  = makeConfigureEvent(view);
    event.configure.width  = (PuglSpan)xevent.xconfigure.width;
    event.configure.height = (PuglSpan)xevent.xconfigure.height;
    if (view->parent) {
      // Use window position relative to parent
      event.configure.x = (PuglCoord)xevent.xconfigure.x;
      event.configure.y = (PuglCoord)xevent.xconfigure.y;
    } else {
      // Get window position relative to the root
      Display* const display = view->world->impl->display;
      const Window   win     = view->impl->win;
      const Window   root    = RootWindow(display, view->impl->screen);
      int            x       = 0;
      int            y       = 0;
      Window         ignored = 0;
      if (XTranslateCoordinates(display, win, root, 0, 0, &x, &y, &ignored)) {
        event.configure.x = (PuglCoord)x;
        event.configure.y = (PuglCoord)y;
      }
    }
    break;
  case Expose:
    event.type          = PUGL_EXPOSE;
    event.expose.x      = (PuglCoord)xevent.xexpose.x;
    event.expose.y      = (PuglCoord)xevent.xexpose.y;
    event.expose.width  = (PuglSpan)xevent.xexpose.width;
    event.expose.height = (PuglSpan)xevent.xexpose.height;
    break;
  case MotionNotify:
    event.type         = PUGL_MOTION;
    event.motion.time  = (double)xevent.xmotion.time / 1e3;
    event.motion.x     = xevent.xmotion.x;
    event.motion.y     = xevent.xmotion.y;
    event.motion.xRoot = xevent.xmotion.x_root;
    event.motion.yRoot = xevent.xmotion.y_root;
    event.motion.state = translateModifiers(xevent.xmotion.state);
    if (xevent.xmotion.is_hint == NotifyHint) {
      event.motion.flags |= PUGL_IS_HINT;
    }
    break;
  case ButtonPress:
  case ButtonRelease:
    if (xevent.type == ButtonPress && xevent.xbutton.button >= 4 &&
        xevent.xbutton.button <= 7) {
      event.type         = PUGL_SCROLL;
      event.scroll.time  = (double)xevent.xbutton.time / 1e3;
      event.scroll.x     = xevent.xbutton.x;
      event.scroll.y     = xevent.xbutton.y;
      event.scroll.xRoot = xevent.xbutton.x_root;
      event.scroll.yRoot = xevent.xbutton.y_root;
      event.scroll.state = translateModifiers(xevent.xbutton.state);
      event.scroll.dx    = 0.0;
      event.scroll.dy    = 0.0;
      switch (xevent.xbutton.button) {
      case 4:
        event.scroll.dy        = 1.0;
        event.scroll.direction = PUGL_SCROLL_UP;
        break;
      case 5:
        event.scroll.dy        = -1.0;
        event.scroll.direction = PUGL_SCROLL_DOWN;
        break;
      case 6:
        event.scroll.dx        = -1.0;
        event.scroll.direction = PUGL_SCROLL_LEFT;
        break;
      case 7:
        event.scroll.dx        = 1.0;
        event.scroll.direction = PUGL_SCROLL_RIGHT;
        break;
      }
    } else if (xevent.xbutton.button < 4 || xevent.xbutton.button > 7) {
      event.button.type   = ((xevent.type == ButtonPress) ? PUGL_BUTTON_PRESS
                                                          : PUGL_BUTTON_RELEASE);
      event.button.time   = (double)xevent.xbutton.time / 1e3;
      event.button.x      = xevent.xbutton.x;
      event.button.y      = xevent.xbutton.y;
      event.button.xRoot  = xevent.xbutton.x_root;
      event.button.yRoot  = xevent.xbutton.y_root;
      event.button.state  = translateModifiers(xevent.xbutton.state);
      event.button.button = xevent.xbutton.button - 1;
      if (event.button.button == 1) {
        event.button.button = 2;
      } else if (event.button.button == 2) {
        event.button.button = 1;
      } else if (event.button.button >= 7) {
        event.button.button -= 4;
      }
    }
    break;
  case KeyPress:
  case KeyRelease:
    event.type =
      ((xevent.type == KeyPress) ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE);
    event.key.time  = (double)xevent.xkey.time / 1e3;
    event.key.x     = xevent.xkey.x;
    event.key.y     = xevent.xkey.y;
    event.key.xRoot = xevent.xkey.x_root;
    event.key.yRoot = xevent.xkey.y_root;
    event.key.state = translateModifiers(xevent.xkey.state);
    translateKey(view, &xevent, &event);
    break;
  case EnterNotify:
  case LeaveNotify:
    event.type =
      ((xevent.type == EnterNotify) ? PUGL_POINTER_IN : PUGL_POINTER_OUT);
    event.crossing.time  = (double)xevent.xcrossing.time / 1e3;
    event.crossing.x     = xevent.xcrossing.x;
    event.crossing.y     = xevent.xcrossing.y;
    event.crossing.xRoot = xevent.xcrossing.x_root;
    event.crossing.yRoot = xevent.xcrossing.y_root;
    event.crossing.state = translateModifiers(xevent.xcrossing.state);
    event.crossing.mode  = PUGL_CROSSING_NORMAL;
    if (xevent.xcrossing.mode == NotifyGrab) {
      event.crossing.mode = PUGL_CROSSING_GRAB;
    } else if (xevent.xcrossing.mode == NotifyUngrab) {
      event.crossing.mode = PUGL_CROSSING_UNGRAB;
    }
    break;

  case FocusIn:
  case FocusOut:
    event.type = (xevent.type == FocusIn) ? PUGL_FOCUS_IN : PUGL_FOCUS_OUT;
    event.focus.mode = PUGL_CROSSING_NORMAL;
    if (xevent.xfocus.mode == NotifyGrab) {
      event.focus.mode = PUGL_CROSSING_GRAB;
    } else if (xevent.xfocus.mode == NotifyUngrab) {
      event.focus.mode = PUGL_CROSSING_UNGRAB;
    }
    break;

  default:
    break;
  }

  return event;
}

/// Flush pending configure and expose events for all views
PUGL_WARN_UNUSED_RESULT static PuglStatus
flushExposures(PuglWorld* const world)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;
  PuglStatus st2 = PUGL_SUCCESS;

  for (size_t i = 0; i < world->numViews; ++i) {
    PuglView* const view = world->views[i];

    // Send update event so the application can trigger redraws
    if (puglGetVisible(view)) {
      puglDispatchSimpleEvent(view, PUGL_UPDATE);
    }

    // Copy and reset pending events (in case their handlers write new ones)
    const PuglEvent configure = view->impl->pendingConfigure;
    const PuglEvent expose    = view->impl->pendingExpose;

    view->impl->pendingConfigure.type = PUGL_NOTHING;
    view->impl->pendingExpose.type    = PUGL_NOTHING;

    if (expose.type || configure.type) {
      const PuglExposeEvent* const exposeEvent =
        expose.type ? &expose.expose : NULL;

      if (!(st0 = view->backend->enter(view, exposeEvent))) {
        if (configure.type) {
          st0 = puglConfigure(view, &configure);
        }

        if (expose.type) {
          st1 = view->eventFunc(view, &expose);
        }
      }

      st2 = view->backend->leave(view, exposeEvent);
    }
  }

  return st0 ? st0 : st1 ? st1 : st2;
}

static bool
handleTimerEvent(PuglWorld* const world, const XEvent xevent)
{
#if USE_XSYNC
  if (xevent.type == world->impl->syncEventBase + XSyncAlarmNotify) {
    const XSyncAlarmNotifyEvent* const notify =
      ((const XSyncAlarmNotifyEvent*)&xevent);

    for (size_t i = 0; i < world->impl->numTimers; ++i) {
      if (world->impl->timers[i].alarm == notify->alarm) {
        PuglEvent event = {{PUGL_TIMER, 0U}};
        event.timer.id  = world->impl->timers[i].id;
        puglDispatchEvent(world->impl->timers[i].view, &event);
      }
    }

    return true;
  }
#else
  (void)world;
  (void)xevent;
#endif

  return false;
}

static PuglStatus
dispatchX11Events(PuglWorld* const world)
{
  PuglStatus st = PUGL_SUCCESS;

  // Flush output to the server once at the start
  Display* display = world->impl->display;
  XFlush(display);

  // Process all queued events (without further flushing)
  while (XEventsQueued(display, QueuedAfterReading) > 0) {
    XEvent xevent;
    XNextEvent(display, &xevent);

    if (handleTimerEvent(world, xevent)) {
      continue;
    }

    PuglView* const view = findView(world, xevent.xany.window);
    if (!view) {
      continue;
    }

    // Handle special events
    PuglInternals* const impl = view->impl;
    if (xevent.type == KeyRelease && view->hints[PUGL_IGNORE_KEY_REPEAT]) {
      XEvent next;
      if (XCheckTypedWindowEvent(display, impl->win, KeyPress, &next) &&
          next.type == KeyPress && next.xkey.time == xevent.xkey.time &&
          next.xkey.keycode == xevent.xkey.keycode) {
        continue;
      }
    } else if (xevent.type == SelectionClear) {
      st = handleSelectionClear(view, &xevent.xselectionclear);
    } else if (xevent.type == SelectionNotify) {
      st = handleSelectionNotify(world, view, &xevent.xselection);
    } else if (xevent.type == SelectionRequest) {
      st = handleSelectionRequest(world, view, &xevent.xselectionrequest);
    }

    if (st) {
      break;
    }

    // Translate X11 event to Pugl event
    const PuglEvent event = translateEvent(view, xevent);

    switch (event.type) {
    case PUGL_CONFIGURE:
      // Update configure event to be dispatched after loop
      view->impl->pendingConfigure = event;
      break;
    case PUGL_EXPOSE:
      // Expand expose event to be dispatched after loop
      mergeExposeEvents(&view->impl->pendingExpose.expose, &event.expose);
      break;
    case PUGL_FOCUS_IN:
      // Set the input context focus
      if (view->impl->xic) {
        XSetICFocus(view->impl->xic);
      }
      break;
    case PUGL_FOCUS_OUT:
      // Unset the input context focus
      if (view->impl->xic) {
        XUnsetICFocus(view->impl->xic);
      }
      break;
    default:
      // Dispatch event to application immediately
      st = puglDispatchEvent(view, &event);
      break;
    }
  }

  return st;
}

PuglStatus
puglUpdate(PuglWorld* const world, const double timeout)
{
  const double startTime = puglGetTime(world);
  PuglStatus   st0       = PUGL_SUCCESS;
  PuglStatus   st1       = PUGL_SUCCESS;

  world->impl->dispatchingEvents = true;

  if (timeout < 0.0) {
    if (!(st0 = pollX11Socket(world, timeout))) {
      st0 = dispatchX11Events(world);
    }
  } else if (timeout <= 0.001) {
    st0 = dispatchX11Events(world);
  } else {
    const double endTime = startTime + timeout - 0.001;
    double       t       = startTime;
    while (!st0 && t < endTime) {
      if (!(st0 = pollX11Socket(world, endTime - t))) {
        st0 = dispatchX11Events(world);
      }

      t = puglGetTime(world);
    }
  }

  st1 = flushExposures(world);

  world->impl->dispatchingEvents = false;

  return st0 ? st0 : st1;
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
