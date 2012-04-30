/*
  Copyright 2012 David Robillard <http://drobilla.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "pugl_internal.h"

struct PuglPlatformDataImpl {
	Display*   display;
	int        screen;
	Window     win;
	GLXContext ctx;
	Bool       doubleBuffered;
};

/**
   Attributes for single-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListSgl[] = {
	GLX_RGBA,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	None
};

/**
   Attributes for double-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListDbl[] = {
	GLX_RGBA, GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	None
};

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));

	view->impl = (PuglPlatformData*)calloc(1, sizeof(PuglPlatformData));

	PuglPlatformData* impl = view->impl;

	view->width         = width;
	view->height        = height;
	impl->display = XOpenDisplay(0);
	impl->screen  = DefaultScreen(impl->display);

	XVisualInfo* vi = glXChooseVisual(impl->display, impl->screen, attrListDbl);
	if (vi == NULL) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListSgl);
		impl->doubleBuffered = False;
		printf("singlebuffered rendering will be used, no doublebuffering available\n");
	} else {
		impl->doubleBuffered = True;
		printf("doublebuffered rendering available\n");
	}

	int glxMajor, glxMinor;
	glXQueryVersion(impl->display, &glxMajor, &glxMinor);
	printf("GLX-Version %d.%d\n", glxMajor, glxMinor);

	impl->ctx = glXCreateContext(impl->display, vi, 0, GL_TRUE);

	Window xParent = parent
		? (Window)parent
		: RootWindow(impl->display, impl->screen);

	Colormap cmap = XCreateColormap(
		impl->display, xParent, vi->visual, AllocNone);

	XSetWindowAttributes attr;
	memset(&attr, 0, sizeof(XSetWindowAttributes));
	attr.colormap     = cmap;
	attr.border_pixel = 0;

	attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask
		| PointerMotionMask | StructureNotifyMask;

	impl->win = XCreateWindow(
		impl->display, xParent,
		0, 0, view->width, view->height, 0, vi->depth, InputOutput, vi->visual,
		CWBorderPixel | CWColormap | CWEventMask, &attr);

	XSizeHints sizeHints;
	memset(&sizeHints, 0, sizeof(sizeHints));
	if (!resizable) {
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = width;
		sizeHints.min_height = height;
		sizeHints.max_width  = width;
		sizeHints.max_height = height;
		XSetNormalHints(impl->display, impl->win, &sizeHints);
	}

	if (title) {
		XStoreName(impl->display, impl->win, title);
	}

	if (!parent) {
		Atom wmDelete = XInternAtom(impl->display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(impl->display, impl->win, &wmDelete, 1);
	}

	XMapRaised(impl->display, impl->win);

	if (glXIsDirect(impl->display, impl->ctx)) {
		printf("DRI enabled\n");
	} else {
		printf("no DRI available\n");
	}

	return view;
}

void
puglDestroy(PuglView* view)
{
	if (view->impl->ctx) {
		if (!glXMakeCurrent(view->impl->display, None, NULL)) {
			printf("Could not release drawing context.\n");
		}
		/* destroy the context */
		glXDestroyContext(view->impl->display, view->impl->ctx);
		view->impl->ctx = NULL;
	}

	XCloseDisplay(view->impl->display);
	free(view);
}

void
puglReshape(PuglView* view, int width, int height)
{
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);

	if (view->reshapeFunc) {
		// User provided a reshape function, defer to that
		view->reshapeFunc(view, width, height);
	} else {
		// No custom reshape function, do something reasonable
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, width/(float)height, 1.0f, 10.0f);
		glViewport(0, 0, width, height);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	view->width     = width;
	view->height    = height;
	view->redisplay = true;
}

void
puglDisplay(PuglView* view)
{
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	if (view->impl->doubleBuffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	}

	view->redisplay = false;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	XEvent event;

	/* handle the events in the queue */
	while (XPending(view->impl->display) > 0) {
		XNextEvent(view->impl->display, &event);
		switch (event.type) {
		case MapNotify:
			puglReshape(view, view->width, view->height);
			break;
		case ConfigureNotify:
			if ((event.xconfigure.width != view->width) ||
			    (event.xconfigure.height != view->height)) {
				puglReshape(view,
				            event.xconfigure.width,
				            event.xconfigure.height);
			}
			break;
		case Expose:
			if (event.xexpose.count != 0) {
				break;
			}
			puglDisplay(view);
			view->redisplay = false;
			break;
		case MotionNotify:
			if (view->motionFunc) {
				view->motionFunc(view, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ButtonPress:
			if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
				if (view->scrollFunc) {
					float dx = 0, dy = 0;
					switch (event.xbutton.button) {
					case 4: dy =  1.0f; break;
					case 5: dy = -1.0f; break;
					case 6: dx = -1.0f; break;
					case 7: dx =  1.0f; break;
					}
					view->scrollFunc(view, dx, dy);
				}
				break;
			}
			// nobreak
		case ButtonRelease:
			if (view->mouseFunc &&
			    (event.xbutton.button < 4 || event.xbutton.button > 7)) {
				view->mouseFunc(view,
				               event.xbutton.button, event.type == ButtonPress,
				               event.xbutton.x, event.xbutton.y);
			}
			break;
		case KeyPress:
			if (view->keyboardFunc) {
				KeySym sym = XKeycodeToKeysym(
					view->impl->display, event.xkey.keycode, 0);
				view->keyboardFunc(view, event.type == KeyPress, sym);
			}
			break;
		case KeyRelease: {
			bool retriggered = false;
			if (XEventsQueued(view->impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(view->impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == event.xkey.time &&
				    next.xkey.keycode == event.xkey.keycode) {
					// Key repeat, ignore fake KeyPress event
					XNextEvent(view->impl->display, &event);
					retriggered = true;
				}
			}

			if (!retriggered && view->keyboardFunc) {
				KeySym sym = XKeycodeToKeysym(
					view->impl->display, event.xkey.keycode, 0);
				view->keyboardFunc(view, false, sym);
			}
		}
		case ClientMessage:
			if (!strcmp(XGetAtomName(view->impl->display,
			                         event.xclient.message_type),
			            "WM_PROTOCOLS")) {
				if (view->closeFunc) {
					view->closeFunc(view);
				}
			}
			break;
		default:
			break;
		}
	}

	if (view->redisplay) {
		puglDisplay(view);
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
