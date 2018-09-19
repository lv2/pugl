/*
  Copyright 2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef PUGL_HAVE_GL
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#ifdef PUGL_HAVE_CAIRO
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#endif

#include "pugl/cairo_gl.h"
#include "pugl/pugl_internal.h"

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

// debug-print dnd info if our window is the dnd source/target?
// #define DND_PRINT_SOURCE
// #define DND_PRINT_TARGET

#ifdef PUGL_HAVE_GL

/** Attributes for double-buffered RGBA. */
static int attrListDbl[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER    , True,
	GLX_RED_SIZE        , 4,
	GLX_GREEN_SIZE      , 4,
	GLX_BLUE_SIZE       , 4,
	GLX_ALPHA_SIZE      , 4,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
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
	GLX_ALPHA_SIZE      , 4,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
	/* GLX_SAMPLE_BUFFERS  , 1, */
	/* GLX_SAMPLES         , 4, */
	None
};

/** Null-terminated list of attributes in order of preference. */
static int* attrLists[] = { attrListDbl, attrListSgl, NULL };

#endif  // PUGL_HAVE_GL

enum {
	XdndAware,
	XdndEnter,
	XdndLeave,
	XdndDrop,
	XdndPosition,
	XdndStatus,
	XdndSelection,
	XdndFinished,
	XdndActionCopy,
	XdndActionMove,
	XdndActionLink,
	XdndProxy,
	WM_PROTOCOLS,
	/* insert more here and keep in sync with atom_names! */
	ATOM_COUNT
};

char *atom_names[ATOM_COUNT] = {
	"XdndAware",
	"XdndEnter",
	"XdndLeave",
	"XdndDrop",
	"XdndPosition",
	"XdndStatus",
	"XdndSelection",
	"XdndFinished",
	"XdndActionCopy",
	"XdndActionMove",
	"XdndActionLink",
	"XdndProxy",
	"WM_PROTOCOLS",
	/* insert more here and keep in sync with above enum! */
};

//! we don't support typelists yet, but the rest is version 5
const int dnd_protocol_version = 5;

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
	Atom atoms[ATOM_COUNT]; //!< useful atoms, see atom_names above
	Time time; //!< ~ the last time an event happened, not always accurate
	struct timeval last_process_events; //!< last time puglProcessEvents was called

	// the dnd_target and dnd_source variables:
	// target means that pugl is the target, source means pugl is the source

	Window dnd_target_last_source;
	int dnd_target_x; //!< current pointer x (relative to pugl)
	int dnd_target_y; //!< current pointer y (relative to pugl)
	int dnd_target_root_x; //!< current pointer x (global)
	int dnd_target_root_y; //!< current pointer y (global)

	//! last action given by XdndPosition
	//! this must usually be the action to reply, and pugl will reply that way
	Atom dnd_target_last_action;

	Window dnd_source_last_target;
	// last rect returned by status
	int dnd_source_last_rect_x0, dnd_source_last_rect_x1,
		dnd_source_last_rect_y0, dnd_source_last_rect_y1;
	int dnd_target_max_types; //!< size of dnd_target_offered_types
	Atom* dnd_target_offered_types; //!< offered mimetypes, size: dnd_target_max_types
};

PuglInternals*
puglInitInternals(void)
{
	PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));
	gettimeofday(&impl->last_process_events, NULL);
	impl->dnd_target_max_types = 3; // WARNING: if >3, you need to implement support for X11 typelists
	impl->dnd_target_offered_types = malloc(impl->dnd_target_max_types * sizeof(Atom));
	for(int i = 0; i < impl->dnd_target_max_types; ++i)
		impl->dnd_target_offered_types[i] = None;
	impl->dnd_source_last_target = None;
	impl->dnd_target_last_source = None;
	impl->time = CurrentTime;
	impl->dnd_target_last_action = None;
	impl->dnd_source_last_rect_x0 = impl->dnd_source_last_rect_x1 =
		impl->dnd_source_last_rect_y0 = impl->dnd_source_last_rect_y1 = -1;
	return impl;
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
	if(!impl->display) {
		fputs("Could not open display, aborting.\n", stderr);
		exit(1);
	}
	impl->screen  = DefaultScreen(impl->display);

	{
		int success = XInternAtoms(impl->display, atom_names, ATOM_COUNT, False, impl->atoms);
		if(!success)
		 exit(1);
	}

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

	unsigned char version = dnd_protocol_version;
	XChangeProperty(impl->display, impl->win, impl->atoms[XdndAware], XA_ATOM,
		32, PropModeReplace, &version, 1);

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
		free(view->impl->dnd_target_offered_types);
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
			//ASCII for control characters
			if(sym == (sym&0x7f)) {
				event->key.character = sym;
				event->key.utf8[0] = sym;
			}
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
			//ASCII for control characters
			if(sym == (sym&0x7f)) {
				event->key.character = sym;
				event->key.utf8[0] = sym;
			}
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

/*
 * debug print helpers
 */
static void
dumpAtom(Display* disp, Atom atm, const char* description)
{
	if(atm)
	{
		char* atomname = XGetAtomName(disp, atm);
		if(atomname) {
			printf("  %s: %s\n", description, atomname);
			XFree(atomname);
		} else {
			printf(" %s: <undefined??>\n", atomname);
		}
	}
}

static void
dumpWindow(Window me, Window window, const char* desc)
{
	if(window == me)
		printf("  %s: <me>\n", desc);
	else
		printf("  %s: %lx\n", desc, (unsigned long) window);
}

static void
dumpMsgHeader(Window me, const XClientMessageEvent* m, const char* name, bool incoming)
{
	printf("<me> %s %s:\n", incoming ? "<-" : "->", name);
	printf("  (type: %s, serial: %lu, send_event: %s, display: %p)\n",
		(m->type == ClientMessage) ? "ClientMessage" : "<unknown>",
		m->serial,
		(m->send_event == True) ? "true" : "false",
		m->display);
	dumpWindow(me, m->window, "target window");
}

static void
dumpTime(Time time, const char* desc)
{
	if(time == CurrentTime)
		printf("  %s: now\n", desc);
	else
		printf("  %s: %.1fs\n", desc, time / 1000.0f);
}

static void
dumpMsg(PuglInternals* impl, const XClientMessageEvent* m, bool incoming)
{
	char* name = XGetAtomName(impl->display, m->message_type);
	dumpMsgHeader(impl->win, m, name, incoming);

	if(m->message_type == impl->atoms[XdndPosition])
	{
		printf("  position: %d %d\n", (int)(m->data.l[2] >> 16), (int)(m->data.l[2] & 0xffff));
		dumpTime(m->data.l[3], "position time");
		dumpAtom(impl->display, m->data.l[4], "position action");
	}
	else if(m->message_type == impl->atoms[XdndEnter])
	{
		printf("  enter protocol version: %lu\n", (unsigned long) (m->data.l[1]>>24));
		for(int i = 2; i < 5; ++i)
		 if(m->data.l[i])
		  dumpAtom(impl->display, m->data.l[i], "enter mimetype");
	}
	else if(m->message_type == impl->atoms[XdndStatus])
	{
		Window expected = incoming ? impl->dnd_source_last_target : impl->win;
		if(m->data.l[0] == expected)
		{
			long flags = m->data.l[1];
			int	x0 = (int)(m->data.l[2] >> 16),
				y0 = (int)(m->data.l[2] & 0xffff),
				x1 = x0 + (int)(m->data.l[3] >> 16),
				y1 = y0 + (int)(m->data.l[3] & 0xffff);


			printf("  status will accept drop: %s\n", (flags & 1) ? "true" : "false");
			printf("  status keep sending in rect: %s\n", (flags & 2) ? "true" : "false");
			if(x0 == x1 || y0 == y1)
				puts("  status send immediatelly if moved");
			else
				printf("  status don't send until leaving "
					"(x0 y0 x1 y1): %d %d %d %d\n",
					x0, y0, x1, y1);
			if(flags & 1)
				dumpAtom(impl->display, m->data.l[4], "status action");
		}
		else
		{
			puts("  !! Bad status message (sender too old):");
			dumpWindow(impl->win, m->data.l[0], "   -> sender");
			dumpWindow(impl->win, expected, "   -> expected");
		}
	}
	else if(m->message_type == impl->atoms[XdndFinished])
	{
		int flags = m->data.l[1];
		printf("  finished target did accept: %s\n", (flags & 1) ? "yes" : "no");
		if(flags & 1)
		 dumpAtom(impl->display, m->data.l[2], "finished action");

		// Probably not necessary. Qt doesn't seem to do it.
		// Enable if you think you need it
		// XSetSelectionOwner(impl->display, impl->atoms[XdndSelection],
		//			None, CurrentTime);
	}
	else if(m->message_type == impl->atoms[XdndDrop])
	{
		dumpTime(m->data.l[2], "drop_time");
	}
	else if(m->message_type == impl->atoms[XdndLeave])
	{
	}
	else {
		printf("!! Unknown message type: %s.\n", name);
	}
	XFree(name);
}

/*
 * misc dnd helpers
 */

static void
setDndSourceStatus(PuglView* view, PuglDndSourceStatus new_status)
{
	if(view->dnd_source_status == new_status)
	{
		printf("!!! set dnd source status to %d, but it already is... "
			"internal logic error?\n", new_status);
	}
	else
	{
		view->dnd_source_status = new_status;
		view->dndSourceStatusFunc(view, new_status);
	}
}

static void
setDndTargetStatus(PuglView* view, PuglDndTargetStatus new_status)
{
	if(view->dnd_target_status == new_status)
	{
		printf("!!! set dnd target status to %d, but it already is... "
			"internal logic error?\n", new_status);
	}
	else
	{
		view->dnd_target_status = new_status;
		view->dndTargetStatusFunc(view, new_status);
	}
}

//! @param win The window that is "most useful to toolkit dispatchers" ...
static void
dndInitMessage(PuglInternals* impl, XClientMessageEvent* msg, Atom dnd_type, Window win)
{
	msg->type = ClientMessage;
	msg->serial = 0;
	msg->send_event = True;
	msg->display = impl->display;
	msg->window = win;
	msg->message_type = impl->atoms[dnd_type];

	memset((char*)&msg->data, 0, sizeof(msg->data));
	msg->format = 32; /* interpret as 5 ints */
	// data.l[0] is always the window sending the message
	msg->data.l[0] = impl->win;
}

typedef enum
{
	toSource,
	toTarget
} DndSendDirection;

static Status
dndSendEvent(PuglInternals* impl, XEvent* event, DndSendDirection sd)
{
	return XSendEvent(impl->display,
			(sd == toSource) ? impl->dnd_target_last_source
					: impl->dnd_source_last_target,
			False, NoEventMask, event);
}

static Status
dndSendMessage(PuglInternals* impl, XClientMessageEvent* event,
			DndSendDirection sd)
{
	return dndSendEvent(impl, (XEvent*)event, sd);
}

static void
dndConvertDesiredSelections(PuglView* view)
{
	int types = view->dndTargetChooseTypesToLookupFunc(view);
	PuglInternals* impl = view->impl;
	Atom* atoms = impl->atoms;
	for(int i = 0; i < impl->dnd_target_max_types; ++i)
	 if((types & (1 << i)) && (impl->dnd_target_offered_types[i] != None))
	  XConvertSelection(impl->display, atoms[XdndSelection], impl->dnd_target_offered_types[i],
				atoms[XdndSelection], impl->win, impl->time);
}

static
Atom translateToX11Action(const PuglView* view, PuglDndAction pugl_action)
{
	const Atom* atoms = view->impl->atoms;
	switch(pugl_action)
	{
		case PuglDndActionCopy: return atoms[XdndActionCopy];
		case PuglDndActionMove: return atoms[XdndActionMove];
		case PuglDndActionLink: return atoms[XdndActionLink];
		default: exit(1);
	}
}

static
PuglDndAction translateToPuglAction(const PuglView* view, Atom x11_action)
{
	const Atom* atoms = view->impl->atoms;
	if(x11_action == atoms[XdndActionCopy])
		return PuglDndActionCopy;
	else if(x11_action == atoms[XdndActionMove])
		return PuglDndActionMove;
	else if(x11_action == atoms[XdndActionLink])
		return PuglDndActionLink;
	else
		exit(1);
}

/*
 * dnd events
 */

// handle incoming messages from source or target
static PuglEvent
handleDndMessages(PuglView* view, const XClientMessageEvent* xclient)
{
	PuglEvent event;
	memset(&event, 0, sizeof(event));
	event.any.view = view;
	// TODO: not sure:
	// the X11 messages do come from another thread, but the events that
	// we send here are not directly translated, but only inferred
	// event.any.flags |= PUGL_IS_SEND_EVENT;

	const Atom* const atoms = view->impl->atoms;
	Atom msg_type = xclient->message_type;
/*	{ char* type = XGetAtomName(view->impl->display,
				  xclient->message_type);
	printf("type: %s\n", type);
	XFree(type); } */

	if(msg_type == atoms[XdndPosition]) {
#ifdef DND_PRINT_TARGET
		dumpMsg(view->impl, xclient, true);
#endif

		int global_x = (xclient->data.l[2]>>16) & 0xFFFF;
		int global_y = (xclient->data.l[2]>>00) & 0xFFFF;

		int win_x = 0, win_y = 0;
		Window child_unused;
		Window root_win = RootWindow(view->impl->display, view->impl->screen);
		XTranslateCoordinates(view->impl->display, root_win,
			view->impl->win, global_x, global_y, &win_x, &win_y, &child_unused);
		int drop_ok = view->dndTargetInformPositionFunc(view, win_x, win_y,
								  translateToPuglAction(view, xclient->data.l[4]));

		view->impl->dnd_target_x = win_x;
		view->impl->dnd_target_y = win_y;
		view->impl->dnd_target_root_x = global_x;
		view->impl->dnd_target_root_y = global_y;
		// printf("xdnd at  <%d %d %d %d>\n", global_x, global_y, win_x, win_y);

		// Moving the DnD pointer over zest, some X events are suppressed.
		// The following is required to still update pugl.
		// That way, tooltips etc can still be shown.
		{
			event.type = PUGL_MOTION_NOTIFY;
			event.motion.time    = 0;
			event.motion.x       = win_x;
			event.motion.y       = win_y;
			event.motion.x_root  = global_x;
			event.motion.y_root  = global_y;
			event.motion.state   = 0;
			event.motion.is_hint = 1;
		}



		PuglInternals* impl = view->impl;
		impl->dnd_target_last_source = xclient->data.l[0];
		impl->time = xclient->data.l[3];
		impl->dnd_target_last_action = xclient->data.l[4];

		dndConvertDesiredSelections(view);

		XClientMessageEvent status;
		dndInitMessage(impl, &status, XdndStatus, impl->dnd_target_last_source);
		status.data.l[1] = (drop_ok) ? 1 : 0; /* accept drop, don't send more */

		int rx, ry, rw, rh;
		int in_use = view->dndTargetNoPositionIn(view, &rx, &ry, &rw, &rh);
		if(in_use)
		{
			int rx_global=0, ry_global=0;
			// not tested yet:
			XTranslateCoordinates(view->impl->display,
				view->impl->win, root_win, rx, ry, &rx_global, &ry_global, &child_unused);
			status.data.l[2] = (rx_global << 16 | ry_global);
			status.data.l[3] = (rw << 16 | rh);
		}
		else
		{
			status.data.l[2] =
			status.data.l[3] = 0;
		}
		status.data.l[4] = impl->dnd_target_last_action;
#ifdef DND_PRINT_TARGET
		dumpMsg(impl, &status, false);
#endif
		dndSendMessage(impl, &status, toSource);

	} else if(msg_type == atoms[XdndEnter]) {

		PuglInternals* impl = view->impl;
#ifdef DND_PRINT_TARGET
		dumpMsg(impl, xclient, true);
#endif
		setDndTargetStatus(view, PuglDndTargetDragged);
		for(int i = 0; i < impl->dnd_target_max_types; ++i)
		{
			Atom a = xclient->data.l[2+i];
			impl->dnd_target_offered_types[i] = a;
			char* atomname = (a == None)
				? ""
				: XGetAtomName(impl->display, a);

			view->dndTargetOfferTypeFunc(view, i, atomname);

			if(*atomname)
				XFree(atomname);
		}

	} else if(msg_type == atoms[XdndLeave]) {
#ifdef DND_PRINT_TARGET
		dumpMsg(view->impl, xclient, true);
#endif
		// clean up
		view->impl->dnd_target_last_source = None;
		view->impl->dnd_target_last_action = None;
		view->dndTargetLeaveFunc(view);
		setDndTargetStatus(view, PuglNotDndTarget);

	} else if(msg_type == atoms[XdndStatus]) {

#ifdef DND_PRINT_SOURCE
		dumpMsg(view->impl, xclient, true);
#endif
		PuglInternals* impl = view->impl;
		if(xclient->data.l[0] == impl->dnd_source_last_target)
		{
			long flags = xclient->data.l[1];
			if(flags & 1)
			{
				// TODO: change cursor? - not implemented yet

				impl->dnd_source_last_rect_x0 = (int)(xclient->data.l[2] >> 16);
				impl->dnd_source_last_rect_y0 = (int)(xclient->data.l[2] & 0xffff);
				impl->dnd_source_last_rect_x1 = impl->dnd_source_last_rect_x0 + (int)(xclient->data.l[3] >> 16);
				impl->dnd_source_last_rect_y1 = impl->dnd_source_last_rect_y0 + (int)(xclient->data.l[3] & 0xffff);
			}
			else
			{
				// drop denied - wait, maybe it will be OK soon
			}
		}
		else
		{
#ifdef DND_PRINT_SOURCE
			puts("  Sorry, denying this status message (sender too old)...");
#endif
		}
	} else if(msg_type == atoms[XdndFinished]) {
#ifdef DND_PRINT_SOURCE
		dumpMsg(view->impl, xclient, true);
#endif
		PuglInternals* impl = view->impl;
		if(xclient->data.l[0] == impl->dnd_source_last_target)
		{
			long flags = xclient->data.l[1];
			view->dndSourceFinishedFunc(view, flags & 1);

			setDndSourceStatus(view, PuglNotDndSource);
			impl->dnd_source_last_target = None;
		}

	} else if(msg_type == atoms[XdndDrop]) {
#ifdef DND_PRINT_TARGET
		dumpMsg(view->impl, xclient, true);
#endif
		setDndTargetStatus(view, PuglDndTargetDropped);

		PuglInternals* impl = view->impl;
		impl->time = xclient->data.l[2];

		view->dndTargetDropFunc(view);

		dndConvertDesiredSelections(view);

		// for justification, see the comment for
		// the motion event when receiving XdndPosition
		{
			event.button.type   = PUGL_BUTTON_PRESS;
			event.button.time   = 0;
			event.button.x      = impl->dnd_target_x;
			event.button.y      = impl->dnd_target_y;
			event.button.x_root = impl->dnd_target_root_x;
			event.button.y_root = impl->dnd_target_root_y;
			event.button.state  = 0;
			event.button.button = 4;
		}

		XClientMessageEvent finished;
		dndInitMessage(impl, &finished, XdndFinished, impl->dnd_target_last_source);

		int acc = view->dndTargetAcceptDropFunc(view);
		finished.data.l[1] = acc ? 1 : 0; /* drop accepted */
		finished.data.l[2] = acc
				? impl->dnd_target_last_action
				: None;
#ifdef DND_PRINT_SOURCE
		dumpMsg(impl, &finished, false);
#endif
		dndSendMessage(impl, &finished, toSource);

		// clean up
		setDndTargetStatus(view, PuglNotDndTarget);
		impl->dnd_target_last_source = None;
		impl->dnd_target_last_action = None;
	}

	// continue; // TODO
	return event;
}

static void
handleDndButtonEvents(PuglView* view, const XButtonEvent* event, bool pressed)
{
	if(pressed)
	{
		if(event->button == Button1 && view->dnd_source_status == PuglDndSourceReady)
		{
			int drag_ok = view->dndSourceDragFunc(view, event->x, event->y);
			if(drag_ok)
			{
				const Atom* const atoms = view->impl->atoms;
				XSetSelectionOwner(view->impl->display, atoms[XdndSelection],
					view->impl->win, CurrentTime);
				Window new_owner = XGetSelectionOwner(view->impl->display, atoms[XdndSelection]);
				if(new_owner == view->impl->win)
				  setDndSourceStatus(view, PuglDndSourceDragged);
			}
		}
	} else { // released
		if(view->dnd_source_status == PuglDndSourceDragged)
		{
			setDndSourceStatus(view, PuglDndSourceDropped);
			PuglInternals* impl = view->impl;
			XClientMessageEvent drop;
			dndInitMessage(impl, &drop, XdndDrop, impl->dnd_source_last_target);
			drop.data.l[2] = CurrentTime;

#ifdef DND_PRINT_SOURCE
			dumpMsg(view->impl, &drop, false);
#endif

			if( 0 == dndSendMessage(impl, &drop, toTarget) )
			{
				impl->dnd_source_last_target = None;
			}
		}
	}
}

// an app (likely another app) called ConvertSelection,
// asking the source (pugl) to change a property
static void
handleDndSelectionRequest(PuglView* view, const XSelectionRequestEvent* ev)
{
	const Atom* const atoms = view->impl->atoms;

#ifdef DND_PRINT_SOURCE
	printf("selection request\n");
#endif
	PuglInternals* impl = view->impl;

	if(ev->selection == atoms[XdndSelection]
			&& ev->property == atoms[XdndSelection])
	{
		int type_offered = -1;
		for(int i = 0; (type_offered == -1) && i < impl->dnd_target_max_types; ++i)
		 if(ev->target == impl->dnd_target_offered_types[i])
		  type_offered = i;

		if(type_offered != -1)
		{
			char buffer[4096];
			int used_size = view->dndSourceProvideDataFunc(view, type_offered, sizeof(buffer), buffer);
			if(used_size > sizeof(buffer))
				exit(1); // library user did not check for overflows - bad

#ifdef DND_PRINT_SOURCE
			dumpAtom(impl->display, ev->selection, "SR: selection");
			dumpAtom(impl->display, ev->target, "SR: selection target");
			dumpAtom(impl->display, ev->property, "SR: selection property");
#endif
			XChangeProperty(impl->display, ev->requestor,
					ev->property, ev->target,
					8, PropModeReplace,
					buffer, used_size);

			XEvent reply;
			reply.xselection.display = impl->display;
			reply.xselection.property = ev->property;
			reply.xselection.requestor = ev->requestor;
			reply.xselection.selection = ev->selection;
			reply.xselection.send_event = True;
			reply.xselection.serial = 0;
			reply.xselection.target = ev->target;
			reply.xselection.time = ev->time;
			reply.xselection.type = SelectionNotify;
			dndSendEvent(impl, &reply, toSource);
		}
	}
	else
	{
#ifdef DND_PRINT_SOURCE
		puts("Ignoring selection request");
#endif
	}
}

// the target (pugl) gets notified that it can now get the
// property that belongs to the requested mimetype
static void
handleDndSelectionNotify(PuglView* view)
{
	const Atom* const atoms = view->impl->atoms;

	Atom type_return;
	int format_return;
	PuglInternals* impl = view->impl;


	unsigned char* prop_return;
	unsigned long n, a;
	int maxsize = XMaxRequestSize(impl->display);
	int res = XGetWindowProperty(impl->display, impl->win, atoms[XdndSelection], 0, maxsize, False,
					AnyPropertyType, &type_return, &format_return,
					&n, &a, &prop_return);
	if(res != Success)
	{
		puts("!!! bad SelectionNotify");
	}
	else
	{
		// finally, here the target receives its data (the "property")
		// computation:
		// n = i + l + a where
		//   i is the index where the property starts
		//   l is the actual length
		//   a is the number of bytes after the property
		// we have set
		//   i = 4*0 (4th param to XGetWindowProperty)
		//   n, a (after return of the function)
		//   wrong formulated from X11: l = min (n, 4*maxsize)
		// l = n - a

#ifdef DND_PRINT_TARGET
		dumpAtom(impl->display, type_return, "SelectionNotify type");
		printf("type: %s, format: %d\n",XGetAtomName(impl->display, type_return), format_return);
		printf("property: %s\n", prop_return);
#endif
		int slot = 0;
		for(; slot < impl->dnd_target_max_types &&
			impl->dnd_target_offered_types[slot] != type_return; ++slot) ;
		if(slot != impl->dnd_target_max_types)
		{
			view->dndTargetReceiveDataFunc(view, slot, n-a, prop_return);
		}
		XFree(prop_return);
	}
}

// jobs that need to be done after a certain timeout
static void
handleTimeoutEvents(PuglView* view)
{
	const Atom* const atoms = view->impl->atoms;

	struct timeval curProcessEvents;
	gettimeofday(&curProcessEvents, NULL);

	struct PuglInternalsImpl* const impl = view->impl;

	// 200 ms passed
	if(    view->dnd_source_status == PuglDndSourceDragged
	   && (   curProcessEvents.tv_sec > impl->last_process_events.tv_sec /* seconds wrapped? */
	       || (curProcessEvents.tv_usec - impl->last_process_events.tv_usec) > 200000 /* 0.2 s passed? */ ))
	{
		// TODO: if we don't get any XdndStatus after, let's say, 1 second,
		// we should send XdndLeave
		int rootx, rooty;
		Window dnd_cur_root = None;

		{
			Window root_return, child_return;
			int winx, winy;
			unsigned int mask;

			Bool res = XQueryPointer(impl->display,
				RootWindow(impl->display, impl->screen),
				&root_return, &child_return,
				&rootx, &rooty, &winx, &winy, &mask);

			if(res != False && child_return != None)
			{
				dnd_cur_root = root_return;
				//printf("win => (%d, %d), root: %lx (%d, %d), child: %lx \n", winx, winy, (unsigned long)root_return, rootx, rooty, (unsigned long)child_return);
			}
			else
			{
				puts(res == False ? "!!! no pointer" : "!!! no child");
			}
		}

		if(dnd_cur_root)
		{
			// make dnd_cur_root the root of the dnd target
			Bool abort = False;

			int targetx = rootx, targety = rooty;
			do
			{
				Atom type_return;
				int format_return;
				unsigned long n, a;
				unsigned char * prop_return = NULL;
				int res = XGetWindowProperty(impl->display, dnd_cur_root,
					atoms[XdndAware], 0, 1, 0,
					AnyPropertyType, &type_return, &format_return,
					&n, &a, &prop_return);


				int version = -1;
				if(res != Success || type_return == None)
				{
					if(res != Success)
						puts("!!! XGetWindowProperty failure");
				}
				else {
					version = *(int*)prop_return;

					if(version < 5)
					{
						/* < 2003 => not supported */
						printf("!!! bad Xdnd version; %d\n", version);
						dnd_cur_root = None;
						abort = True;
					}
					else
					{
						// success, target is XdndAware
						abort = True;
					}
				}
				if(prop_return)
					 XFree(prop_return);

				int unused1, unused2;
				Window ch = None;
				// printf("XTranslateCoordinates:\ndnd_cur_root: %lx, root: %d, %d\n", dnd_cur_root, rootx, rooty);

				// get the child first
				Bool trans_res = XTranslateCoordinates(impl->display,
							dnd_cur_root, dnd_cur_root,
							targetx, targety, &unused1, &unused2, &ch);
				if(trans_res == False)
				{
					puts("!! DnD to different screen. Not supported");
					abort = True;
				}
				else if (ch == None)
				{
					// no more child windows - we can leave the loop
					abort = True;
				}
				else
				{
					Window ch_unused;
					int chx, chy;
					// get the coords relative to the child
					if(XTranslateCoordinates(impl->display,
						dnd_cur_root, ch,
						targetx, targety, &chx, &chy, &ch_unused))
					{
						//printf("recursing: => %lx (%d, %d) => %lx (%d, %d) \n", dnd_cur_root, rootx, rooty, ch, chx, chy);
						dnd_cur_root = ch;
						targetx = chx;
						targety = chy;
					}
					else
					{
						puts("!!! XTranslateCoordinates failure");
						dnd_cur_root = None;
						abort = True;
					}
				}
			} while (abort == False);
		}

		Window proxy = None;

		{
			Atom type_return; int format_return;
			unsigned long n, a;
			unsigned char* prop_return = NULL;
			int res = XGetWindowProperty(impl->display, dnd_cur_root,
					atoms[XdndProxy], 0, 1, 0,
					XA_WINDOW, &type_return, &format_return,
					&n, &a, &prop_return);
			if(res == Success && type_return != None)
			{
				proxy = *(Window*)prop_return;
				printf("!! found proxy %lx (NOT TESTED YET)\n",
					(unsigned long)proxy);
			}
			else if (res != Success) {
				puts("!! XGetWindowProperty failure");
			}

			if(prop_return)
				XFree(prop_return);
		}

		if(proxy == None)
			proxy = dnd_cur_root;
		/*
		 * note: below this line, proxy is the proxy, or the current dnd target
		 * note: proxies are untested yet
		 */

		if(proxy != impl->dnd_source_last_target)
		{
			if(impl->dnd_source_last_target)
			{
				XClientMessageEvent leave;
				dndInitMessage(impl, &leave, XdndLeave, impl->dnd_source_last_target);
				dndSendMessage(impl, &leave, toTarget);
			}

			impl->dnd_source_last_target = proxy;

			XClientMessageEvent enter;
			dndInitMessage(impl, &enter, XdndEnter, dnd_cur_root);
			enter.data.l[1] = dnd_protocol_version << 24;

			for(int i = 0; i < impl->dnd_target_max_types; ++i)
			{
				const char* next_mimetype = view->dndSourceOfferTypeFunc(view, rootx, rooty, i);
				if(!next_mimetype || !*next_mimetype)
					i = impl->dnd_target_max_types;
				else
					enter.data.l[2+i] = XInternAtom(impl->display, next_mimetype, True);
			}

			if( 0 == dndSendMessage(impl, &enter, toTarget) )
			{
				puts("!!! XSendEvent failed");
				proxy = None;
			}
			else
			{
#ifdef DND_PRINT_SOURCE
				dumpMsg(impl, &enter, false);
#endif
			}
		}

		if(proxy)
		{
			// TODO: this should only be sent again if the position
			// is somehow outside of the dnd_source_last_rect_... variables
			// By now, we had no way to test this
			// Note: There is no requirement to not keep sending, so
			//       this implementation is already legal

			XClientMessageEvent pos;
			dndInitMessage(impl, &pos, XdndPosition, dnd_cur_root);
			pos.data.l[2] = rootx << 16 | rooty;
			pos.data.l[3] = CurrentTime;
			pos.data.l[4] = translateToX11Action(view, view->dndSourceActionFunc(view, rootx, rooty));

			if( 0 == dndSendMessage(impl, &pos, toTarget) )
			{
				puts("!!! XSendEvent failed");
				proxy = None;
			}
#ifdef DND_PRINT_SOURCE
			else {
				dumpMsg(impl, &pos, false);
			}
#endif
		}

		view->impl->last_process_events.tv_usec = curProcessEvents.tv_usec;
		view->impl->last_process_events.tv_sec = curProcessEvents.tv_sec;
	}
}

/*
 * event handling
 */
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
	case ClientMessage:
		if (xevent.xclient.message_type == view->impl->atoms[WM_PROTOCOLS]) {
			event.type = PUGL_CLOSE;
		}
		else {
			// overwrite event
			event = handleDndMessages(view, &xevent.xclient);
		}
		break;
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
			// fallthru
		}
		// fallthru
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
		} else if (xevent.type == ButtonPress || xevent.type == ButtonRelease) {
			handleDndButtonEvents(view, &xevent.xbutton, xevent.type == ButtonPress);
		} else if (xevent.type == SelectionRequest) {
			handleDndSelectionRequest(view, &xevent.xselectionrequest);
		} else if (xevent.type == SelectionNotify) {
			handleDndSelectionNotify(view);
		}

		handleTimeoutEvents(view);

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
