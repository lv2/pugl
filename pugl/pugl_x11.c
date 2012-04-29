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

PuglWindow*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable)
{
	PuglWindow* win = (PuglWindow*)calloc(1, sizeof(PuglWindow));

	win->impl = (PuglPlatformData*)calloc(1, sizeof(PuglPlatformData));

	PuglPlatformData* impl = win->impl;

	win->width         = width;
	win->height        = height;
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
		0, 0, win->width, win->height, 0, vi->depth, InputOutput, vi->visual,
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

	return win;
}

void
puglDestroy(PuglWindow* win)
{
	if (win->impl->ctx) {
		if (!glXMakeCurrent(win->impl->display, None, NULL)) {
			printf("Could not release drawing context.\n");
		}
		/* destroy the context */
		glXDestroyContext(win->impl->display, win->impl->ctx);
		win->impl->ctx = NULL;
	}

	XCloseDisplay(win->impl->display);
	free(win);
}

void
puglReshape(PuglWindow* win, int width, int height)
{
	glXMakeCurrent(win->impl->display, win->impl->win, win->impl->ctx);

	if (win->reshapeFunc) {
		// User provided a reshape function, defer to that
		win->reshapeFunc(win, width, height);
	} else {
		// No custom reshape function, do something reasonable
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, win->width/(float)win->height, 1.0f, 10.0f);
		glViewport(0, 0, win->width, win->height);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	win->width     = width;
	win->height    = height;
	win->redisplay = true;
}

void
puglDisplay(PuglWindow* win)
{
	glXMakeCurrent(win->impl->display, win->impl->win, win->impl->ctx);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (win->displayFunc) {
		win->displayFunc(win);
	}

	glFlush();
	if (win->impl->doubleBuffered) {
		glXSwapBuffers(win->impl->display, win->impl->win);
	}
}

PuglStatus
puglProcessEvents(PuglWindow* win)
{
	XEvent event;

	/* handle the events in the queue */
	while (XPending(win->impl->display) > 0) {
		XNextEvent(win->impl->display, &event);
		switch (event.type) {
		case MapNotify:
			puglReshape(win, win->width, win->height);
			break;
		case ConfigureNotify:
			if ((event.xconfigure.width != win->width) ||
			    (event.xconfigure.height != win->height)) {
				puglReshape(win,
				            event.xconfigure.width,
				            event.xconfigure.height);
			}
			break;
		case Expose:
			if (event.xexpose.count != 0) {
				break;
			}
			puglDisplay(win);
			win->redisplay = false;
			break;
		case MotionNotify:
			if (win->motionFunc) {
				win->motionFunc(win, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ButtonPress:
			if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
				if (win->scrollFunc) {
					float dx = 0, dy = 0;
					switch (event.xbutton.button) {
					case 4: dy =  1.0f; break;
					case 5: dy = -1.0f; break;
					case 6: dx = -1.0f; break;
					case 7: dx =  1.0f; break;
					}
					win->scrollFunc(win, dx, dy);
				}
				break;
			}
			// nobreak
		case ButtonRelease:
			if (win->mouseFunc &&
			    (event.xbutton.button < 4 || event.xbutton.button > 7)) {
				win->mouseFunc(win,
				               event.xbutton.button, event.type == ButtonPress,
				               event.xbutton.x, event.xbutton.y);
			}
			break;
		case KeyPress:
			if (win->keyboardFunc) {
				KeySym sym = XKeycodeToKeysym(
					win->impl->display, event.xkey.keycode, 0);
				win->keyboardFunc(win, event.type == KeyPress, sym);
			}
			break;
		case KeyRelease: {
			bool retriggered = false;
			if (XEventsQueued(win->impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(win->impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == event.xkey.time &&
				    next.xkey.keycode == event.xkey.keycode) {
					// Key repeat, ignore fake KeyPress event
					XNextEvent(win->impl->display, &event);
					retriggered = true;
				}
			}

			if (!retriggered && win->keyboardFunc) {
				KeySym sym = XKeycodeToKeysym(
					win->impl->display, event.xkey.keycode, 0);
				win->keyboardFunc(win, false, sym);
			}
		}
		case ClientMessage:
			if (!strcmp(XGetAtomName(win->impl->display,
			                         event.xclient.message_type),
			            "WM_PROTOCOLS")) {
				if (win->closeFunc) {
					win->closeFunc(win);
				}
			}
			break;
		default:
			break;
		}
	}

	if (win->redisplay) {
		puglDisplay(win);
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglWindow* win)
{
	win->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglWindow* win)
{
	return win->impl->win;
}
