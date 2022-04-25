// Copyright 2012-2023 David Robillard <d@drobilla.net>
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

static PuglArea
puglX11CairoGetViewSize(const PuglView* const view)
{
  PuglArea size = {0U, 0U};

  if (view->lastConfigure.type == PUGL_CONFIGURE) {
    // Use the size of the last configured frame
    size.width  = view->lastConfigure.width;
    size.height = view->lastConfigure.height;
  } else {
    // Use the default size
    size.width  = view->sizeHints[PUGL_DEFAULT_SIZE].width;
    size.height = view->sizeHints[PUGL_DEFAULT_SIZE].height;
  }

  return size;
}

static void
puglX11CairoClose(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

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
      cairo_surface_status(surface->front)) {
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
puglX11CairoEnter(PuglView* view, const bool drawing)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;
  PuglStatus                 st      = PUGL_SUCCESS;
  if (!drawing) {
    return st;
  }

  const PuglArea viewSize = puglX11CairoGetViewSize(view);
  if ((st = puglX11CairoOpen(view, viewSize.width, viewSize.height))) {
    return st;
  }

  surface->cr     = cairo_create(surface->front);
  surface->region = cairo_region_create();
  if (cairo_status(surface->cr) || cairo_region_status(surface->region)) {
    cairo_region_destroy(surface->region);
    cairo_destroy(surface->cr);
    surface->region = NULL;
    surface->cr     = NULL;
    puglX11CairoClose(view);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return st ? st : PUGL_IGNORE;
}

static PuglStatus
puglX11CairoLeave(PuglView* view, const bool drawing)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  if (!drawing || !surface->region) {
    return PUGL_IGNORE;
  }

  // Destroy front context and create a new one for drawing to the back
  cairo_destroy(surface->cr);
  surface->cr = cairo_create(surface->back);

  // Paint exposed region(s) from the front to the back
  const int n_rects = cairo_region_num_rectangles(surface->region);
  for (int i = 0; i < n_rects; ++i) {
    cairo_rectangle_int_t rect = {0, 0, 0, 0};
    cairo_region_get_rectangle(surface->region, i, &rect);

    cairo_save(surface->cr);

    // Clip to exposed region
    cairo_rectangle(surface->cr, rect.x, rect.y, rect.width, rect.height);
    cairo_clip(surface->cr);

    // Paint front onto back
    cairo_set_source_surface(surface->cr, surface->front, 0, 0);
    cairo_paint(surface->cr);

    cairo_restore(surface->cr);
  }

  // Flush to X and close everything
  cairo_destroy(surface->cr);
  cairo_region_destroy(surface->region);
  cairo_surface_flush(surface->back);
  surface->region = NULL;
  surface->cr     = NULL;
  puglX11CairoClose(view);
  return PUGL_IGNORE;
}

static PuglStatus
puglX11CairoExpose(PuglView* view, const PuglExposeEvent* expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  if (surface->region) {
    const cairo_rectangle_int_t rect = {
      expose->x, expose->y, expose->width, expose->height};

    cairo_region_union_rectangle(surface->region, &rect);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoHandle(PuglView* view, const PuglEvent* event)
{
  return (event->type == PUGL_EXPOSE) ? puglX11CairoExpose(view, &event->expose)
                                      : PUGL_SUCCESS;
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
  static const PuglBackend backend = {puglX11Configure,
                                      puglX11CairoCreate,
                                      puglX11CairoDestroy,
                                      puglX11CairoEnter,
                                      puglX11CairoLeave,
                                      puglX11CairoHandle,
                                      puglX11CairoGetContext};

  return &backend;
}
