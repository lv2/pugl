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

#include "pugl/pugl_internal_types.h"
#include "pugl/pugl_x11.h"
#include "pugl/pugl_x11_gl.h"

#include <GL/gl.h>
#include <GL/glx.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct {
	GLXFBConfig fb_config;
	GLXContext  ctx;
	int         double_buffered;
} PuglX11GlSurface;

static int
puglX11GlHintValue(const int value)
{
	return value == PUGL_DONT_CARE ? (int)GLX_DONT_CARE : value;
}

static int
puglX11GlGetAttrib(Display* const    display,
                   const GLXFBConfig fb_config,
                   const int         attrib)
{
	int value = 0;
	glXGetFBConfigAttrib(display, fb_config, attrib, &value);
	return value;
}

static int
puglX11GlConfigure(PuglView* view)
{
	PuglInternals* const impl    = view->impl;
	const int            screen  = impl->screen;
	Display* const       display = impl->display;

	PuglX11GlSurface* const surface =
		(PuglX11GlSurface*)calloc(1, sizeof(PuglX11GlSurface));
	impl->surface = surface;

	const int attrs[] = {
		GLX_X_RENDERABLE,  True,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_SAMPLES,       view->hints.samples,
		GLX_RED_SIZE,      puglX11GlHintValue(view->hints.red_bits),
		GLX_GREEN_SIZE,    puglX11GlHintValue(view->hints.green_bits),
		GLX_BLUE_SIZE,     puglX11GlHintValue(view->hints.blue_bits),
		GLX_ALPHA_SIZE,    puglX11GlHintValue(view->hints.alpha_bits),
		GLX_DEPTH_SIZE,    puglX11GlHintValue(view->hints.depth_bits),
		GLX_STENCIL_SIZE,  puglX11GlHintValue(view->hints.stencil_bits),
		GLX_DOUBLEBUFFER,  puglX11GlHintValue(view->hints.double_buffer),
		None
	};

	int          n_fbc = 0;
	GLXFBConfig* fbc   = glXChooseFBConfig(display, screen, attrs, &n_fbc);
	if (n_fbc <= 0) {
		fprintf(stderr, "error: Failed to create GL context\n");
		return 1;
	}

	surface->fb_config = fbc[0];
	impl->vi           = glXGetVisualFromFBConfig(impl->display, fbc[0]);

	printf("Using visual 0x%lX: R=%d G=%d B=%d A=%d D=%d"
	       " DOUBLE=%d SAMPLES=%d\n",
	       impl->vi->visualid,
	       puglX11GlGetAttrib(display, fbc[0], GLX_RED_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_GREEN_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_BLUE_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_ALPHA_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_DEPTH_SIZE),
	       puglX11GlGetAttrib(display, fbc[0], GLX_DOUBLEBUFFER),
	       puglX11GlGetAttrib(display, fbc[0], GLX_SAMPLES));

	XFree(fbc);

	return 0;
}

static int
puglX11GlCreate(PuglView* view)
{
	PuglInternals* const    impl      = view->impl;
	PuglX11GlSurface* const surface   = (PuglX11GlSurface*)impl->surface;
	Display* const          display   = impl->display;
	const GLXFBConfig       fb_config = surface->fb_config;

	const int ctx_attrs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, view->hints.context_version_major,
		GLX_CONTEXT_MINOR_VERSION_ARB, view->hints.context_version_minor,
		GLX_CONTEXT_PROFILE_MASK_ARB, (view->hints.use_compat_profile
		                               ? GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
		                               : GLX_CONTEXT_CORE_PROFILE_BIT_ARB),
		0};

	typedef GLXContext (*CreateContextAttribs)(
		Display*, GLXFBConfig, GLXContext, Bool, const int*);

	CreateContextAttribs create_context =
		(CreateContextAttribs)glXGetProcAddress(
			(const GLubyte*)"glXCreateContextAttribsARB");

	impl->surface = surface;
	surface->ctx  = create_context(display, fb_config, 0, GL_TRUE, ctx_attrs);
	if (!surface->ctx) {
		surface->ctx =
			glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, 0, True);
	}

	glXGetConfig(impl->display,
	             impl->vi,
	             GLX_DOUBLEBUFFER,
	             &surface->double_buffered);

	return 0;
}

static int
puglX11GlDestroy(PuglView* view)
{
	PuglX11GlSurface* surface = (PuglX11GlSurface*)view->impl->surface;
	if (surface) {
		glXDestroyContext(view->impl->display, surface->ctx);
		free(surface);
		view->impl->surface = NULL;
	}
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

	if (flush && surface->double_buffered) {
		glXSwapBuffers(view->impl->display, view->impl->win);
	} else if (flush) {
		glFlush();
	}

	glXMakeCurrent(view->impl->display, None, NULL);

	return 0;
}

static int
puglX11GlResize(PuglView* view, int width, int height)
{
	(void)view;
	(void)width;
	(void)height;
	return 0;
}

static void*
puglX11GlGetHandle(PuglView* view)
{
	(void)view;
	return NULL;
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
	return glXGetProcAddress((const GLubyte*)name);
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
