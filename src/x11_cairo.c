// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "types.h"
#include "x11.h"

#include <pugl/cairo.h>
#include <pugl/pugl.h>

#include <cairo-xlib.h>
#include <cairo.h>

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  cairo_surface_t* back;
  cairo_surface_t* front;
  cairo_t*         cr;
  cairo_region_t*  region;
} PuglX11CairoSurface;

static void
puglX11CairoClose(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  cairo_region_destroy(surface->region);
  surface->region = NULL;

  cairo_surface_destroy(surface->front);
  cairo_surface_destroy(surface->back);
  surface->front = surface->back = NULL;
}

static PuglStatus
puglX11CairoOpen(PuglView* view, const PuglSpan width, const PuglSpan height)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  surface->back = cairo_xlib_surface_create(
    view->world->impl->display, impl->win, impl->vi->visual, width, height);

  surface->front = cairo_surface_create_similar(
    surface->back, cairo_surface_get_content(surface->back), width, height);

  if (cairo_surface_status(surface->back) ||
      cairo_surface_status(surface->front) ||
      !(surface->region = cairo_region_create())) {
    puglX11CairoClose(view);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoCreate(PuglView* view)
{
  PuglInternals* const impl = view->impl;

  impl->surface = (cairo_surface_t*)calloc(1, sizeof(PuglX11CairoSurface));

  return PUGL_SUCCESS;
}

static void
puglX11CairoDestroy(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  puglX11CairoClose(view);
  free(surface);
}

static PuglStatus
puglX11CairoEnter(PuglView* view, const bool expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;
  PuglStatus                 st      = PUGL_SUCCESS;

  if (expose) {
    const PuglArea viewSize = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    if (!(st = puglX11CairoOpen(view, viewSize.width, viewSize.height))) {
      surface->cr = cairo_create(surface->front);
      if (cairo_status(surface->cr)) {
        cairo_destroy(surface->cr);
        surface->cr = NULL;
        st          = PUGL_CREATE_CONTEXT_FAILED;
      }
    }
  }

  return st;
}

static PuglStatus
puglX11CairoDamage(PuglView* view, const PuglDamageEvent* const damage)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;
  if (surface->region) {
    const cairo_rectangle_int_t rect = {
      damage->x, damage->y, damage->width, damage->height};

    cairo_region_union_rectangle(surface->region, &rect);
    cairo_reset_clip(surface->cr);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoLeave(PuglView* view, const bool expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  if (expose) {
    // Destroy front context and create a new one for drawing to the back
    cairo_destroy(surface->cr);
    surface->cr        = NULL;
    cairo_t* const bcr = cairo_create(surface->back);
    cairo_set_source_surface(bcr, surface->front, 0, 0);

    const int n_rects = cairo_region_num_rectangles(surface->region);
    for (int i = 0; i < n_rects; ++i) {
      cairo_rectangle_int_t rect = {0, 0, 0, 0};
      cairo_region_get_rectangle(surface->region, i, &rect);

      // Clip to expose region
      cairo_rectangle(bcr, rect.x, rect.y, rect.width, rect.height);
      cairo_clip(bcr);

      // Paint front onto back
      cairo_paint(bcr);
      cairo_reset_clip(bcr);
    }

    // Flush to X and close everything
    cairo_destroy(bcr);
    cairo_surface_flush(surface->back);
    puglX11CairoClose(view);
  }

  return PUGL_SUCCESS;
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
  static const PuglBackend backend = {puglX11Attach,
                                      puglX11Detach,
                                      puglX11CairoCreate,
                                      puglX11CairoDestroy,
                                      puglX11CairoEnter,
                                      puglX11CairoDamage,
                                      puglX11CairoLeave,
                                      puglX11CairoGetContext};

  return &backend;
}
