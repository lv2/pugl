/*
  Copyright 2012-2016 David Robillard <http://drobilla.net>
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
   @file pugl_x11.c X11 Pugl Implementation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef PUGL_HAVE_GL
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#ifdef PUGL_HAVE_CAIRO
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#endif

#include "pugl/cairo_gl.h"
#include "pugl/pugl_internal.h"

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifdef PUGL_HAVE_GL

/** Attributes for double-buffered RGBA. */
static int attrListDbl[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER    , True,
	GLX_RED_SIZE        , 4,
	GLX_GREEN_SIZE      , 4,
	GLX_BLUE_SIZE       , 4,
	GLX_DEPTH_SIZE      , 16,
	/* GLX_SAMPLE_BUFFERS  , 1, */
	/* GLX_SAMPLES         , 4, */
	None
};

/** Attributes for single-buffered RGBA. */
static int attrListSgl[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER    , False,
	GLX_RED_SIZE        , 4,
	GLX_GREEN_SIZE      , 4,
	GLX_BLUE_SIZE       , 4,
	GLX_DEPTH_SIZE      , 16,
	/* GLX_SAMPLE_BUFFERS  , 1, */
	/* GLX_SAMPLES         , 4, */
	None
};

/** Null-terminated list of attributes in order of preference. */
static int* attrLists[] = { attrListDbl, attrListSgl, NULL };

#endif  // PUGL_HAVE_GL

struct PuglInternalsImpl {
	Display*         display;
	int              screen;
	Window           win;
	XIM              xim;
	XIC              xic;
#ifdef PUGL_HAVE_CAIRO
	cairo_surface_t* surface;
	cairo_t*         cr;
#endif
#ifdef PUGL_HAVE_GL
	GLXContext       ctx;
	int              doubleBuffered;
#endif
#if defined(PUGL_HAVE_CAIRO) && defined(PUGL_HAVE_GL)
	PuglCairoGL      cairo_gl;
#endif
};

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

static XVisualInfo*
getVisual(PuglView* view)
{
	PuglInternals* const impl = view->impl;
	XVisualInfo*         vi   = NULL;

#ifdef PUGL_HAVE_GL
	if (view->ctx_type & PUGL_GL) {
		for (int* attr = *attrLists; !vi && *attr; ++attr) {
			vi = glXChooseVisual(impl->display, impl->screen, attr);
		}
	}
#endif
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type == PUGL_CAIRO) {
		XVisualInfo pat;
		int         n;
		pat.screen = impl->screen;
		vi         = XGetVisualInfo(impl->display, VisualScreenMask, &pat, &n);
	}
#endif

	return vi;
}

#ifdef PUGL_HAVE_CAIRO
static int
createCairoContext(PuglView* view)
{
	PuglInternals* const impl = view->impl;

	if (impl->cr) {
		cairo_destroy(impl->cr);
	}

	impl->cr = cairo_create(impl->surface);
	return cairo_status(impl->cr);
}
#endif

static bool
createContext(PuglView* view, XVisualInfo* vi)
{
	PuglInternals* const impl = view->impl;

#ifdef PUGL_HAVE_GL
	if (view->ctx_type & PUGL_GL) {
		impl->ctx = glXCreateContext(impl->display, vi, 0, GL_TRUE);
		glXGetConfig(impl->display, vi, GLX_DOUBLEBUFFER, &impl->doubleBuffered);
	}
#endif
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type == PUGL_CAIRO) {
		impl->surface = cairo_xlib_surface_create(
			impl->display, impl->win, vi->visual, view->width, view->height);
	}
#endif
#if defined(PUGL_HAVE_GL) && defined(PUGL_HAVE_CAIRO)
	if (view->ctx_type == PUGL_CAIRO_GL) {
		impl->surface = pugl_cairo_gl_create(
			&impl->cairo_gl, view->width, view->height, 4);
	}
#endif

#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		if (cairo_surface_status(impl->surface) != CAIRO_STATUS_SUCCESS) {
			fprintf(stderr, "error: failed to create cairo surface\n");
			return false;
		}

		if (createCairoContext(view) != CAIRO_STATUS_SUCCESS) {
			cairo_surface_destroy(impl->surface);
			fprintf(stderr, "error: failed to create cairo context\n");
			return false;
		}
	}
#endif

	return true;
}

static void
destroyContext(PuglView* view)
{
#if defined(PUGL_HAVE_CAIRO) && defined(PUGL_HAVE_GL)
	if (view->ctx_type == PUGL_CAIRO_GL) {
		pugl_cairo_gl_free(&view->impl->cairo_gl);
	}
#endif
#ifdef PUGL_HAVE_GL
	if (view->ctx_type & PUGL_GL) {
		glXDestroyContext(view->impl->display, view->impl->ctx);
	}
#endif
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		cairo_destroy(view->impl->cr);
		cairo_surface_destroy(view->impl->surface);
	}
#endif
}

void
puglEnterContext(PuglView* view)
{
#ifdef PUGL_HAVE_GL
	if (view->ctx_type & PUGL_GL) {
		glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);
	}
#endif
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		cairo_set_source_rgb(view->impl->cr, 0, 0, 0);
		cairo_paint(view->impl->cr);
	}
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
#ifdef PUGL_HAVE_GL
	if (flush && view->ctx_type & PUGL_GL) {
#ifdef PUGL_HAVE_CAIRO
		if (view->ctx_type == PUGL_CAIRO_GL) {
			pugl_cairo_gl_draw(&view->impl->cairo_gl, view->width, view->height);
		}
#endif

		glFlush();
		if (view->impl->doubleBuffered) {
			glXSwapBuffers(view->impl->display, view->impl->win);
		}
	}

	glXMakeCurrent(view->impl->display, None, NULL);
#endif
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* const impl = view->impl;

	impl->display = XOpenDisplay(0);
	impl->screen  = DefaultScreen(impl->display);

	XVisualInfo* const vi = getVisual(view);
	if (!vi) {
		return 1;
	}

	Window xParent = view->parent
		? (Window)view->parent
		: RootWindow(impl->display, impl->screen);

	Colormap cmap = XCreateColormap(
		impl->display, xParent, vi->visual, AllocNone);

	XSetWindowAttributes attr;
	memset(&attr, 0, sizeof(XSetWindowAttributes));
	attr.colormap         = cmap;
	attr.event_mask       = (ExposureMask | StructureNotifyMask |
	                         EnterWindowMask | LeaveWindowMask |
	                         KeyPressMask | KeyReleaseMask |
	                         ButtonPressMask | ButtonReleaseMask |
	                         PointerMotionMask | FocusChangeMask);

	impl->win = XCreateWindow(
		impl->display, xParent,
		0, 0, view->width, view->height, 0, vi->depth, InputOutput, vi->visual,
		CWColormap | CWEventMask, &attr);

	if (!createContext(view, vi)) {
		return 2;
	}

	XSizeHints sizeHints;
	memset(&sizeHints, 0, sizeof(sizeHints));
	if (!view->resizable) {
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = view->width;
		sizeHints.min_height = view->height;
		sizeHints.max_width  = view->width;
		sizeHints.max_height = view->height;
		XSetNormalHints(impl->display, impl->win, &sizeHints);
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

		XSetNormalHints(impl->display, impl->win, &sizeHints);
	}

	if (title) {
		XStoreName(impl->display, impl->win, title);
	}

	if (!view->parent) {
		Atom wmDelete = XInternAtom(impl->display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(impl->display, impl->win, &wmDelete, 1);
	}

	if (view->transient_parent) {
		XSetTransientForHint(impl->display, impl->win,
		                     (Window)(view->transient_parent));
	}

	XSetLocaleModifiers("");
	if (!(impl->xim = XOpenIM(impl->display, NULL, NULL, NULL))) {
		XSetLocaleModifiers("@im=");
		if (!(impl->xim = XOpenIM(impl->display, NULL, NULL, NULL))) {
			fprintf(stderr, "warning: XOpenIM failed\n");
		}
	}

	const XIMStyle im_style = XIMPreeditNothing | XIMStatusNothing;
	if (!(impl->xic = XCreateIC(impl->xim,
	                            XNInputStyle,   im_style,
	                            XNClientWindow, impl->win,
	                            XNFocusWindow,  impl->win,
	                            NULL))) {
		fprintf(stderr, "warning: XCreateIC failed\n");
	}

	XFree(vi);

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
		destroyContext(view);
		XDestroyWindow(view->impl->display, view->impl->win);
		XCloseDisplay(view->impl->display);
		free(view->windowClass);
		free(view->impl);
		free(view);
	}
}

static PuglKey
keySymToSpecial(KeySym sym)
{
	switch (sym) {
	case XK_F1:        return PUGL_KEY_F1;
	case XK_F2:        return PUGL_KEY_F2;
	case XK_F3:        return PUGL_KEY_F3;
	case XK_F4:        return PUGL_KEY_F4;
	case XK_F5:        return PUGL_KEY_F5;
	case XK_F6:        return PUGL_KEY_F6;
	case XK_F7:        return PUGL_KEY_F7;
	case XK_F8:        return PUGL_KEY_F8;
	case XK_F9:        return PUGL_KEY_F9;
	case XK_F10:       return PUGL_KEY_F10;
	case XK_F11:       return PUGL_KEY_F11;
	case XK_F12:       return PUGL_KEY_F12;
	case XK_Left:      return PUGL_KEY_LEFT;
	case XK_Up:        return PUGL_KEY_UP;
	case XK_Right:     return PUGL_KEY_RIGHT;
	case XK_Down:      return PUGL_KEY_DOWN;
	case XK_Page_Up:   return PUGL_KEY_PAGE_UP;
	case XK_Page_Down: return PUGL_KEY_PAGE_DOWN;
	case XK_Home:      return PUGL_KEY_HOME;
	case XK_End:       return PUGL_KEY_END;
	case XK_Insert:    return PUGL_KEY_INSERT;
	case XK_Shift_L:   return PUGL_KEY_SHIFT;
	case XK_Shift_R:   return PUGL_KEY_SHIFT;
	case XK_Control_L: return PUGL_KEY_CTRL;
	case XK_Control_R: return PUGL_KEY_CTRL;
	case XK_Alt_L:     return PUGL_KEY_ALT;
	case XK_Alt_R:     return PUGL_KEY_ALT;
	case XK_Super_L:   return PUGL_KEY_SUPER;
	case XK_Super_R:   return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static void
translateKey(PuglView* view, XEvent* xevent, PuglEvent* event)
{
	KeySym sym = 0;
	char*  str = (char*)event->key.utf8;
	memset(str, 0, 8);
	event->key.filter = XFilterEvent(xevent, None);
	if (xevent->type == KeyRelease || event->key.filter || !view->impl->xic) {
		if (XLookupString(&xevent->xkey, str, 7, &sym, NULL) == 1) {
			event->key.character = str[0];
		}
	} else {
		/* TODO: Not sure about this.  On my system, some characters work with
		   Xutf8LookupString but not with XmbLookupString, and some are the
		   opposite. */
		Status status = 0;
#ifdef X_HAVE_UTF8_STRING
		const int n = Xutf8LookupString(
			view->impl->xic, &xevent->xkey, str, 7, &sym, &status);
#else
		const int n = XmbLookupString(
			view->impl->xic, &xevent->xkey, str, 7, &sym, &status);
#endif
		if (n > 0) {
			event->key.character = puglDecodeUTF8((const uint8_t*)str);
		}
	}
	event->key.special = keySymToSpecial(sym);
	event->key.keycode = xevent->xkey.keycode;
}

static unsigned
translateModifiers(unsigned xstate)
{
	unsigned state = 0;
	state |= (xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0;
	state |= (xstate & ControlMask) ? PUGL_MOD_CTRL   : 0;
	state |= (xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0;
	state |= (xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0;
	return state;
}

static PuglEvent
translateEvent(PuglView* view, XEvent xevent)
{
	PuglEvent event;
	memset(&event, 0, sizeof(event));

	event.any.view = view;
	if (xevent.xany.send_event) {
		event.any.flags |= PUGL_IS_SEND_EVENT;
	}

	switch (xevent.type) {
	case ClientMessage: {
		char* type = XGetAtomName(view->impl->display,
		                          xevent.xclient.message_type);
		if (!strcmp(type, "WM_PROTOCOLS")) {
			event.type = PUGL_CLOSE;
		}
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
		event.motion.time    = xevent.xmotion.time;
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
			event.scroll.time    = xevent.xbutton.time;
			event.scroll.x       = xevent.xbutton.x;
			event.scroll.y       = xevent.xbutton.y;
			event.scroll.x_root  = xevent.xbutton.x_root;
			event.scroll.y_root  = xevent.xbutton.y_root;
			event.scroll.state   = translateModifiers(xevent.xbutton.state);
			event.scroll.dx      = 0.0;
			event.scroll.dy      = 0.0;
			switch (xevent.xbutton.button) {
			case 4: event.scroll.dy =  1.0f; break;
			case 5: event.scroll.dy = -1.0f; break;
			case 6: event.scroll.dx = -1.0f; break;
			case 7: event.scroll.dx =  1.0f; break;
			}
		}
		// nobreak
	case ButtonRelease:
		if (xevent.xbutton.button < 4 || xevent.xbutton.button > 7) {
			event.button.type   = ((xevent.type == ButtonPress)
			                       ? PUGL_BUTTON_PRESS
			                       : PUGL_BUTTON_RELEASE);
			event.button.time   = xevent.xbutton.time;
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
		event.key.time   = xevent.xkey.time;
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
		event.crossing.time   = xevent.xcrossing.time;
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
		event.type = ((xevent.type == FocusIn)
		              ? PUGL_FOCUS_IN
		              : PUGL_FOCUS_OUT);
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

PuglStatus
puglWaitForEvent(PuglView* view)
{
	XEvent xevent;
	XPeekEvent(view->impl->display, &xevent);
	return PUGL_SUCCESS;
}

static void
merge_draw_events(PuglEvent* dst, const PuglEvent* src)
{
	if (!dst->type) {
		*dst = *src;
	} else {
		dst->expose.x      = MIN(dst->expose.x,      src->expose.x);
		dst->expose.y      = MIN(dst->expose.y,      src->expose.y);
		dst->expose.width  = MAX(dst->expose.width,  src->expose.width);
		dst->expose.height = MAX(dst->expose.height, src->expose.height);
	}
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	/* Maintain a single expose/configure event to execute after all pending
	   events.  This avoids redundant drawing/configuration which prevents a
	   series of window resizes in the same loop from being laggy. */
	PuglEvent expose_event = { 0 };
	PuglEvent config_event = { 0 };
	XEvent    xevent;
	while (XPending(view->impl->display) > 0) {
		XNextEvent(view->impl->display, &xevent);
		if (xevent.type == KeyRelease) {
			// Ignore key repeat if necessary
			if (view->ignoreKeyRepeat &&
			    XEventsQueued(view->impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(view->impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == xevent.xkey.time &&
				    next.xkey.keycode == xevent.xkey.keycode) {
					XNextEvent(view->impl->display, &xevent);
					continue;
				}
			}
		} else if (xevent.type == FocusIn) {
			XSetICFocus(view->impl->xic);
		} else if (xevent.type == FocusOut) {
			XUnsetICFocus(view->impl->xic);
		}

		// Translate X11 event to Pugl event
		const PuglEvent event = translateEvent(view, xevent);

		if (event.type == PUGL_EXPOSE) {
			// Expand expose event to be dispatched after loop
			merge_draw_events(&expose_event, &event);
		} else if (event.type == PUGL_CONFIGURE) {
			// Expand configure event to be dispatched after loop
			merge_draw_events(&config_event, &event);
		} else {
			// Dispatch event to application immediately
			puglDispatchEvent(view, &event);
		}
	}

	if (config_event.type) {
#ifdef PUGL_HAVE_CAIRO
		if (view->ctx_type == PUGL_CAIRO) {
			// Resize surfaces/contexts before dispatching
			view->redisplay = true;
			cairo_xlib_surface_set_size(view->impl->surface,
			                            config_event.configure.width,
			                            config_event.configure.height);
		}
#ifdef PUGL_HAVE_GL
		if (view->ctx_type == PUGL_CAIRO_GL) {
			view->redisplay = true;
			cairo_surface_destroy(view->impl->surface);
			view->impl->surface = pugl_cairo_gl_create(
				&view->impl->cairo_gl,
				config_event.configure.width,
				config_event.configure.height,
				4);
			pugl_cairo_gl_configure(&view->impl->cairo_gl,
			                        config_event.configure.width,
			                        config_event.configure.height);
			createCairoContext(view);
		}
#endif
#endif
		puglDispatchEvent(view, (const PuglEvent*)&config_event);
	}

	if (view->redisplay) {
		expose_event.expose.type       = PUGL_EXPOSE;
		expose_event.expose.view       = view;
		expose_event.expose.x          = 0;
		expose_event.expose.y          = 0;
		expose_event.expose.width      = view->width;
		expose_event.expose.height     = view->height;
		view->redisplay                = false;
	}

	if (expose_event.type) {
		puglDispatchEvent(view, (const PuglEvent*)&expose_event);
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return view->impl->win;
}

void*
puglGetContext(PuglView* view)
{
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		return view->impl->cr;
	}
#endif
	return NULL;
}
