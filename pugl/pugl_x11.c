/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>
  Copyright 2013 Robin Gareus <robin@gareus.org>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles

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

/**
   @file pugl_x11.c X11 implementation.
*/

#define _POSIX_C_SOURCE 199309L

#include "pugl/pugl_internal.h"
#include "pugl/pugl_x11.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

enum WmClientStateMessageAction {
	WM_STATE_REMOVE,
	WM_STATE_ADD,
	WM_STATE_TOGGLE
};

static const long eventMask =
	(ExposureMask | StructureNotifyMask | FocusChangeMask |
	 EnterWindowMask | LeaveWindowMask | PointerMotionMask |
	 ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* const impl    = view->impl;
	Display* const       display = XOpenDisplay(0);

	impl->display = display;
	impl->screen  = DefaultScreen(display);

	// Intern the various atoms we will need
	impl->atoms.WM_PROTOCOLS     = XInternAtom(display, "WM_PROTOCOLS", 0);
	impl->atoms.WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	impl->atoms.NET_WM_STATE     = XInternAtom(display, "_NET_WM_STATE", 0);
	impl->atoms.NET_WM_STATE_DEMANDS_ATTENTION =
		XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);

	if (!view->backend || !view->backend->configure) {
		return 1;
	} else if (view->backend->configure(view) || !impl->vi) {
		view->backend->destroy(view);
		return 2;
	}

	Window xParent = view->parent ? (Window)view->parent
	                              : RootWindow(display, impl->screen);

	Colormap cmap = XCreateColormap(
		display, xParent, impl->vi->visual, AllocNone);

	XSetWindowAttributes attr = {0};
	attr.colormap   = cmap;
	attr.event_mask = eventMask;

	const Window win = impl->win = XCreateWindow(
		display, xParent,
		0, 0, view->width, view->height, 0, impl->vi->depth, InputOutput,
		impl->vi->visual, CWColormap | CWEventMask, &attr);

	if (view->backend->create(view)) {
		return 3;
	}

	XSizeHints sizeHints = {0};
	if (!view->hints.resizable) {
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = view->width;
		sizeHints.min_height = view->height;
		sizeHints.max_width  = view->width;
		sizeHints.max_height = view->height;
	} else {
		if (view->min_width || view->min_height) {
			sizeHints.flags      = PMinSize;
			sizeHints.min_width  = view->min_width;
			sizeHints.min_height = view->min_height;
		}
		if (view->min_aspect_x) {
			sizeHints.flags        |= PAspect;
			sizeHints.min_aspect.x  = view->min_aspect_x;
			sizeHints.min_aspect.y  = view->min_aspect_y;
			sizeHints.max_aspect.x  = view->max_aspect_x;
			sizeHints.max_aspect.y  = view->max_aspect_y;
		}
	}
	XSetNormalHints(display, win, &sizeHints);

	if (title) {
		XStoreName(display, win, title);
	}

	if (!view->parent) {
		XSetWMProtocols(display, win, &view->impl->atoms.WM_DELETE_WINDOW, 1);
	}

	if (view->transient_parent) {
		XSetTransientForHint(display, win, (Window)(view->transient_parent));
	}

	XSetLocaleModifiers("");
	if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
		XSetLocaleModifiers("@im=");
		if (!(impl->xim = XOpenIM(display, NULL, NULL, NULL))) {
			fprintf(stderr, "warning: XOpenIM failed\n");
		}
	}

	const XIMStyle im_style = XIMPreeditNothing | XIMStatusNothing;
	if (!(impl->xic = XCreateIC(impl->xim,
	                            XNInputStyle,   im_style,
	                            XNClientWindow, win,
	                            XNFocusWindow,  win,
	                            NULL))) {
		fprintf(stderr, "warning: XCreateIC failed\n");
	}

	return 0;
}

void
puglShowWindow(PuglView* view)
{
	XMapRaised(view->impl->display, view->impl->win);
	view->visible = true;
}

void
puglHideWindow(PuglView* view)
{
	XUnmapWindow(view->impl->display, view->impl->win);
	view->visible = false;
}

void
puglDestroy(PuglView* view)
{
	if (view) {
		if (view->impl->xic) {
			XDestroyIC(view->impl->xic);
		}
		if (view->impl->xim) {
			XCloseIM(view->impl->xim);
		}
		view->backend->destroy(view);
		XDestroyWindow(view->impl->display, view->impl->win);
		XCloseDisplay(view->impl->display);
		XFree(view->impl->vi);
		free(view->windowClass);
		free(view->impl);
		free(view);
	}
}

static PuglKey
keySymToSpecial(KeySym sym)
{
	switch (sym) {
	case XK_F1:          return PUGL_KEY_F1;
	case XK_F2:          return PUGL_KEY_F2;
	case XK_F3:          return PUGL_KEY_F3;
	case XK_F4:          return PUGL_KEY_F4;
	case XK_F5:          return PUGL_KEY_F5;
	case XK_F6:          return PUGL_KEY_F6;
	case XK_F7:          return PUGL_KEY_F7;
	case XK_F8:          return PUGL_KEY_F8;
	case XK_F9:          return PUGL_KEY_F9;
	case XK_F10:         return PUGL_KEY_F10;
	case XK_F11:         return PUGL_KEY_F11;
	case XK_F12:         return PUGL_KEY_F12;
	case XK_Left:        return PUGL_KEY_LEFT;
	case XK_Up:          return PUGL_KEY_UP;
	case XK_Right:       return PUGL_KEY_RIGHT;
	case XK_Down:        return PUGL_KEY_DOWN;
	case XK_Page_Up:     return PUGL_KEY_PAGE_UP;
	case XK_Page_Down:   return PUGL_KEY_PAGE_DOWN;
	case XK_Home:        return PUGL_KEY_HOME;
	case XK_End:         return PUGL_KEY_END;
	case XK_Insert:      return PUGL_KEY_INSERT;
	case XK_Shift_L:     return PUGL_KEY_SHIFT_L;
	case XK_Shift_R:     return PUGL_KEY_SHIFT_R;
	case XK_Control_L:   return PUGL_KEY_CTRL_L;
	case XK_Control_R:   return PUGL_KEY_CTRL_R;
	case XK_Alt_L:       return PUGL_KEY_ALT_L;
	case XK_ISO_Level3_Shift:
	case XK_Alt_R:       return PUGL_KEY_ALT_R;
	case XK_Super_L:     return PUGL_KEY_SUPER_L;
	case XK_Super_R:     return PUGL_KEY_SUPER_R;
	case XK_Menu:        return PUGL_KEY_MENU;
	case XK_Caps_Lock:   return PUGL_KEY_CAPS_LOCK;
	case XK_Scroll_Lock: return PUGL_KEY_SCROLL_LOCK;
	case XK_Num_Lock:    return PUGL_KEY_NUM_LOCK;
	case XK_Print:       return PUGL_KEY_PRINT_SCREEN;
	case XK_Pause:       return PUGL_KEY_PAUSE;
	default: break;
	}
	return (PuglKey)0;
}

static int
lookupString(XIC xic, XEvent* xevent, char* str, KeySym* sym)
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
translateKey(PuglView* view, XEvent* xevent, PuglEvent* event)
{
	const unsigned state  = xevent->xkey.state;
	const bool     filter = XFilterEvent(xevent, None);

	event->key.keycode = xevent->xkey.keycode;
	xevent->xkey.state = 0;

	// Lookup unshifted key
	char          ustr[8] = {0};
	KeySym        sym     = 0;
	const int     ufound  = XLookupString(&xevent->xkey, ustr, 8, &sym, NULL);
	const PuglKey special = keySymToSpecial(sym);

	event->key.key = ((special || ufound <= 0)
	                  ? special
	                  : puglDecodeUTF8((const uint8_t*)ustr));

	if (xevent->type == KeyPress && !filter && !special) {
		// Lookup shifted key for possible text event
		xevent->xkey.state = state;

		char      sstr[8] = {0};
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
	return (((xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0) |
	        ((xstate & ControlMask) ? PUGL_MOD_CTRL   : 0) |
	        ((xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0) |
	        ((xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0));
}

static PuglEvent
translateEvent(PuglView* view, XEvent xevent)
{
	PuglEvent event = {0};
	event.any.flags = xevent.xany.send_event ? PUGL_IS_SEND_EVENT : 0;

	switch (xevent.type) {
	case ClientMessage:
		if (xevent.xclient.message_type == view->impl->atoms.WM_PROTOCOLS) {
			const Atom protocol = (Atom)xevent.xclient.data.l[0];
			if (protocol == view->impl->atoms.WM_DELETE_WINDOW) {
				event.type = PUGL_CLOSE;
			}
		}
		break;
	case MapNotify: {
		XWindowAttributes attrs = {0};
		XGetWindowAttributes(view->impl->display, view->impl->win, &attrs);
		event.type             = PUGL_CONFIGURE;
		event.configure.x      = attrs.x;
		event.configure.y      = attrs.y;
		event.configure.width  = attrs.width;
		event.configure.height = attrs.height;
		break;
	}
	case ConfigureNotify:
		event.type             = PUGL_CONFIGURE;
		event.configure.x      = xevent.xconfigure.x;
		event.configure.y      = xevent.xconfigure.y;
		event.configure.width  = xevent.xconfigure.width;
		event.configure.height = xevent.xconfigure.height;
		break;
	case Expose:
		event.type          = PUGL_EXPOSE;
		event.expose.x      = xevent.xexpose.x;
		event.expose.y      = xevent.xexpose.y;
		event.expose.width  = xevent.xexpose.width;
		event.expose.height = xevent.xexpose.height;
		event.expose.count  = xevent.xexpose.count;
		break;
	case MotionNotify:
		event.type           = PUGL_MOTION_NOTIFY;
		event.motion.time    = xevent.xmotion.time / 1e3;
		event.motion.x       = xevent.xmotion.x;
		event.motion.y       = xevent.xmotion.y;
		event.motion.x_root  = xevent.xmotion.x_root;
		event.motion.y_root  = xevent.xmotion.y_root;
		event.motion.state   = translateModifiers(xevent.xmotion.state);
		event.motion.is_hint = (xevent.xmotion.is_hint == NotifyHint);
		break;
	case ButtonPress:
		if (xevent.xbutton.button >= 4 && xevent.xbutton.button <= 7) {
			event.type           = PUGL_SCROLL;
			event.scroll.time    = xevent.xbutton.time / 1e3;
			event.scroll.x       = xevent.xbutton.x;
			event.scroll.y       = xevent.xbutton.y;
			event.scroll.x_root  = xevent.xbutton.x_root;
			event.scroll.y_root  = xevent.xbutton.y_root;
			event.scroll.state   = translateModifiers(xevent.xbutton.state);
			event.scroll.dx      = 0.0;
			event.scroll.dy      = 0.0;
			switch (xevent.xbutton.button) {
			case 4: event.scroll.dy =  1.0; break;
			case 5: event.scroll.dy = -1.0; break;
			case 6: event.scroll.dx = -1.0; break;
			case 7: event.scroll.dx =  1.0; break;
			}
			// fallthru
		}
		// fallthru
	case ButtonRelease:
		if (xevent.xbutton.button < 4 || xevent.xbutton.button > 7) {
			event.button.type   = ((xevent.type == ButtonPress)
			                       ? PUGL_BUTTON_PRESS
			                       : PUGL_BUTTON_RELEASE);
			event.button.time   = xevent.xbutton.time / 1e3;
			event.button.x      = xevent.xbutton.x;
			event.button.y      = xevent.xbutton.y;
			event.button.x_root = xevent.xbutton.x_root;
			event.button.y_root = xevent.xbutton.y_root;
			event.button.state  = translateModifiers(xevent.xbutton.state);
			event.button.button = xevent.xbutton.button;
		}
		break;
	case KeyPress:
	case KeyRelease:
		event.type       = ((xevent.type == KeyPress)
		                    ? PUGL_KEY_PRESS
		                    : PUGL_KEY_RELEASE);
		event.key.time   = xevent.xkey.time / 1e3;
		event.key.x      = xevent.xkey.x;
		event.key.y      = xevent.xkey.y;
		event.key.x_root = xevent.xkey.x_root;
		event.key.y_root = xevent.xkey.y_root;
		event.key.state  = translateModifiers(xevent.xkey.state);
		translateKey(view, &xevent, &event);
		break;
	case EnterNotify:
	case LeaveNotify:
		event.type            = ((xevent.type == EnterNotify)
		                         ? PUGL_ENTER_NOTIFY
		                         : PUGL_LEAVE_NOTIFY);
		event.crossing.time   = xevent.xcrossing.time / 1e3;
		event.crossing.x      = xevent.xcrossing.x;
		event.crossing.y      = xevent.xcrossing.y;
		event.crossing.x_root = xevent.xcrossing.x_root;
		event.crossing.y_root = xevent.xcrossing.y_root;
		event.crossing.state  = translateModifiers(xevent.xcrossing.state);
		event.crossing.mode   = PUGL_CROSSING_NORMAL;
		if (xevent.xcrossing.mode == NotifyGrab) {
			event.crossing.mode = PUGL_CROSSING_GRAB;
		} else if (xevent.xcrossing.mode == NotifyUngrab) {
			event.crossing.mode = PUGL_CROSSING_UNGRAB;
		}
		break;

	case FocusIn:
	case FocusOut:
		event.type = (xevent.type == FocusIn) ? PUGL_FOCUS_IN : PUGL_FOCUS_OUT;
		event.focus.grab = (xevent.xfocus.mode != NotifyNormal);
		break;

	default:
		break;
	}

	return event;
}

void
puglGrabFocus(PuglView* view)
{
	XSetInputFocus(
		view->impl->display, view->impl->win, RevertToPointerRoot, CurrentTime);
}

void
puglRequestAttention(PuglView* view)
{
	PuglInternals* const impl  = view->impl;
	XEvent               event = {0};
	event.type                 = ClientMessage;
	event.xclient.window       = impl->win;
	event.xclient.format       = 32;
	event.xclient.message_type = impl->atoms.NET_WM_STATE;
	event.xclient.data.l[0]    = WM_STATE_ADD;
	event.xclient.data.l[1]    = impl->atoms.NET_WM_STATE_DEMANDS_ATTENTION;
	event.xclient.data.l[2]    = 0;
	event.xclient.data.l[3]    = 1;
	event.xclient.data.l[4]    = 0;

	const Window root = RootWindow(impl->display, impl->screen);
	XSendEvent(impl->display,
	           root,
	           False,
	           SubstructureNotifyMask | SubstructureRedirectMask,
	           (XEvent*)&event);
}

PuglStatus
puglWaitForEvent(PuglView* view)
{
	XEvent xevent;
	XPeekEvent(view->impl->display, &xevent);
	return PUGL_SUCCESS;
}

static void
merge_expose_events(PuglEvent* dst, const PuglEvent* src)
{
	if (!dst->type) {
		*dst = *src;
	} else {
		const double max_x = MAX(dst->expose.x + dst->expose.width,
		                         src->expose.x + src->expose.width);
		const double max_y = MAX(dst->expose.y + dst->expose.height,
		                         src->expose.y + src->expose.height);

		dst->expose.x      = MIN(dst->expose.x, src->expose.x);
		dst->expose.y      = MIN(dst->expose.y, src->expose.y);
		dst->expose.width  = max_x - dst->expose.x;
		dst->expose.height = max_y - dst->expose.y;
		dst->expose.count  = MIN(dst->expose.count, src->expose.count);
	}
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	/* Maintain a single expose/configure event to execute after all pending
	   events.  This avoids redundant drawing/configuration which prevents a
	   series of window resizes in the same loop from being laggy. */
	PuglInternals* const impl         = view->impl;
	PuglEvent            expose_event = { 0 };
	PuglEvent            config_event = { 0 };
	XEvent               xevent;
	while (XPending(impl->display) > 0) {
		XNextEvent(impl->display, &xevent);
		if (xevent.type == KeyRelease) {
			// Ignore key repeat if necessary
			if (view->ignoreKeyRepeat &&
			    XEventsQueued(impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == xevent.xkey.time &&
				    next.xkey.keycode == xevent.xkey.keycode) {
					XNextEvent(impl->display, &xevent);
					continue;
				}
			}
		} else if (xevent.type == FocusIn) {
			XSetICFocus(impl->xic);
		} else if (xevent.type == FocusOut) {
			XUnsetICFocus(impl->xic);
		}

		// Translate X11 event to Pugl event
		const PuglEvent event = translateEvent(view, xevent);

		if (event.type == PUGL_EXPOSE) {
			// Expand expose event to be dispatched after loop
			merge_expose_events(&expose_event, &event);
		} else if (event.type == PUGL_CONFIGURE) {
			// Expand configure event to be dispatched after loop
			config_event = event;
		} else {
			// Dispatch event to application immediately
			puglDispatchEvent(view, &event);
		}
	}

	if (config_event.type || expose_event.type) {
		const bool draw = expose_event.type && expose_event.expose.count == 0;

		puglEnterContext(view, draw);

		if (config_event.type) {
			view->width  = (int)config_event.configure.width;
			view->height = (int)config_event.configure.height;
			view->backend->resize(view, view->width, view->height);
			view->eventFunc(view, (const PuglEvent*)&config_event);
		}

		if (draw) {
			view->eventFunc(view, (const PuglEvent*)&expose_event);
		}

		puglLeaveContext(view, draw);
	}

	return PUGL_SUCCESS;
}

double
puglGetTime(PuglView* view)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((double)ts.tv_sec + ts.tv_nsec / 1000000000.0) - view->start_time;
}

void
puglPostRedisplay(PuglView* view)
{
	XExposeEvent ev = {Expose, 0, True,
	                   view->impl->display, view->impl->win,
	                   0, 0,
	                   view->width, view->height,
	                   0};

	XSendEvent(view->impl->display, view->impl->win, False, 0, (XEvent*)&ev);
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->win;
}
