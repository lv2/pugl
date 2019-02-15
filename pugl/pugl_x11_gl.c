/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

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

#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "pugl/pugl_x11.h"
#include "pugl/pugl_x11_gl.h"
#include "pugl/pugl_internal_types.h"

typedef struct {
	GLXContext ctx;
	int        doubleBuffered;
} PuglX11GlSurface;

static int
puglX11GlConfigure(PuglView* view)
{
	PuglInternals* const impl = view->impl;

	/** Attributes for double-buffered RGBA. */
	static int attrListDbl[] = { GLX_RGBA,
		                         GLX_DOUBLEBUFFER, True,
		                         GLX_RED_SIZE, 4,
		                         GLX_GREEN_SIZE, 4,
		                         GLX_BLUE_SIZE, 4,
		                         GLX_DEPTH_SIZE, 16,
		                         /* GLX_SAMPLE_BUFFERS  , 1, */
		                         /* GLX_SAMPLES         , 4, */
		                         None };

	/** Attributes for single-buffered RGBA. */
	static int attrListSgl[] = { GLX_RGBA,
		                         GLX_DOUBLEBUFFER, False,
		                         GLX_RED_SIZE, 4,
		                         GLX_GREEN_SIZE, 4,
		                         GLX_BLUE_SIZE, 4,
		                         GLX_DEPTH_SIZE, 16,
		                         /* GLX_SAMPLE_BUFFERS  , 1, */
		                         /* GLX_SAMPLES         , 4, */
		                         None };

	/** Null-terminated list of attributes in order of preference. */
	static int* attrLists[] = { attrListDbl, attrListSgl, NULL };

	if (view->ctx_type & PUGL_GL) {
		for (int* attr = *attrLists; !impl->vi && *attr; ++attr) {
			impl->vi = glXChooseVisual(impl->display, impl->screen, attr);
		}
	}

	return 0;
}

static int
puglX11GlCreate(PuglView* view)
{
	PuglInternals* const impl = view->impl;

	PuglX11GlSurface* surface = (PuglX11GlSurface*)calloc(1, sizeof(PuglX11GlSurface));

	impl->surface = surface;
	surface->ctx = glXCreateContext(impl->display, impl->vi, 0, GL_TRUE);
	glXGetConfig(
		impl->display, impl->vi, GLX_DOUBLEBUFFER, &surface->doubleBuffered);

	return 0;
}

static int
puglX11GlDestroy(PuglView* view)
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
	glXDestroyContext(view->impl->display, surface->ctx);
	free(surface);
	view->impl->surface = NULL;
	return 0;
}

static int
puglX11GlEnter(PuglView* view)
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
	glXMakeCurrent(view->impl->display, view->impl->win, surface->ctx);
	return 0;
}

static int
puglX11GlLeave(PuglView* view, bool flush)
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;

	if (flush) {
		glFlush();
	}

	glXMakeCurrent(view->impl->display, None, NULL);
	if (surface->doubleBuffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	}

	return 0;
}

static int
puglX11GlResize(PuglView* view, int width, int height)
{
	return 0;
}

static void*
puglX11GlGetHandle(PuglView* view)
{
	return NULL;
}

PuglDrawContext puglGetX11GlDrawContext(void)
{
	static const PuglDrawContext puglX11GlDrawContext = {
		puglX11GlConfigure,
		puglX11GlCreate,
		puglX11GlDestroy,
		puglX11GlEnter,
		puglX11GlLeave,
		puglX11GlResize,
		puglX11GlGetHandle
	};

	return puglX11GlDrawContext;
}
