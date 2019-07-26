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

#include "pugl/pugl_cairo_backend.h"
#include "pugl/pugl_internal_types.h"
#include "pugl/pugl_x11.h"

#include <X11/Xutil.h>
#include <cairo-xlib.h>
#include <cairo.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct  {
	cairo_surface_t* surface;
	cairo_t*         cr;
} PuglX11CairoSurface;

static int
puglX11CairoConfigure(PuglView* view)
{
	PuglInternals* const impl = view->impl;

	XVisualInfo pat;
	int         n;
	pat.screen = impl->screen;
	impl->vi = XGetVisualInfo(impl->display, VisualScreenMask, &pat, &n);

	return 0;
}

static int
puglX11CairoCreate(PuglView* view)
{
	PuglInternals* const       impl = view->impl;
	PuglX11CairoSurface* const surface =
		(PuglX11CairoSurface*)calloc(1, sizeof(PuglX11CairoSurface));

	impl->surface = surface;

	surface->surface = cairo_xlib_surface_create(
		impl->display, impl->win, impl->vi->visual, view->width, view->height);

	if (!surface->surface) {
		return 1;
	}

	cairo_status_t st = cairo_surface_status(surface->surface);
	if (st) {
		fprintf(stderr, "error: failed to create cairo surface (%s)\n",
		        cairo_status_to_string(st));
	} else if (!(surface->cr = cairo_create(surface->surface))) {
		fprintf(stderr, "error: failed to create cairo context\n");
	} else if ((st = cairo_status(surface->cr))) {
		cairo_surface_destroy(surface->surface);
		fprintf(stderr, "error: cairo context is invalid (%s)\n",
		        cairo_status_to_string(st));
	}
	return (int)st;
}

static int
puglX11CairoDestroy(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	cairo_destroy(surface->cr);
	cairo_surface_destroy(surface->surface);
	free(surface);
	impl->surface = NULL;
	return 0;
}

static int
puglX11CairoEnter(PuglView* PUGL_UNUSED(view), bool PUGL_UNUSED(drawing))
{
	return 0;
}

static int
puglX11CairoLeave(PuglView* PUGL_UNUSED(view), bool PUGL_UNUSED(drawing))
{
	return 0;
}

static int
puglX11CairoResize(PuglView* view, int width, int height)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	cairo_xlib_surface_set_size(surface->surface, width, height);

	return 0;
}

static void*
puglX11CairoGetContext(PuglView* view)
{
	PuglInternals* const       impl    = view->impl;
	PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

	return surface->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
	static const PuglBackend backend = {
		puglX11CairoConfigure,
		puglX11CairoCreate,
		puglX11CairoDestroy,
		puglX11CairoEnter,
		puglX11CairoLeave,
		puglX11CairoResize,
		puglX11CairoGetContext
	};

	return &backend;
}
