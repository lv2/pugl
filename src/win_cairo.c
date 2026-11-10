// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "types.h"
#include "win.h"

#include <pugl/cairo.h>

#include <cairo-win32.h>
#include <cairo.h>

#include <stdlib.h>

typedef struct {
  cairo_surface_t* surface;
  cairo_t*         cr;
  cairo_region_t*  region;
  HDC              drawDc;
  HBITMAP          drawBitmap;
} PuglWinCairoSurface;

static PuglStatus
puglWinCairoCreateDrawContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  surface->drawDc     = CreateCompatibleDC(impl->hdc);
  surface->drawBitmap = CreateCompatibleBitmap(
    impl->hdc, view->lastConfigure.width, view->lastConfigure.height);

  DeleteObject(SelectObject(surface->drawDc, surface->drawBitmap));

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoDestroyDrawContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  DeleteDC(surface->drawDc);
  DeleteObject(surface->drawBitmap);

  surface->drawDc     = NULL;
  surface->drawBitmap = NULL;

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoAttach(PuglView* view)
{
  const PuglStatus st = puglWinAttach(view);

  if (!st) {
    view->impl->surface =
      (PuglWinCairoSurface*)calloc(1, sizeof(PuglWinCairoSurface));
  }

  return st;
}

static void
puglWinCairoDetach(PuglView* view)
{
  free(view->impl->surface);
  view->impl->surface = NULL;
  puglWinDetach(view);
}

static void
puglWinCairoClose(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  cairo_region_destroy(surface->region);
  surface->region = NULL;

  cairo_destroy(surface->cr);
  surface->cr = NULL;

  cairo_surface_destroy(surface->surface);
  surface->surface = NULL;
}

static PuglStatus
puglWinCairoOpen(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (!(surface->surface = cairo_win32_surface_create(surface->drawDc)) ||
      cairo_surface_status(surface->surface) ||
      !(surface->cr = cairo_create(surface->surface)) ||
      cairo_status(surface->cr) || !(surface->region = cairo_region_create())) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static void
puglWinCairoDestroy(PuglView* view)
{
  puglWinCairoClose(view);
  puglWinCairoDestroyDrawContext(view);
}

static PuglStatus
puglWinCairoEnter(PuglView* view, const bool expose)
{
  PuglStatus st = PUGL_SUCCESS;

  if (expose && !(st = puglWinCairoCreateDrawContext(view))) {
    st = puglWinCairoOpen(view);
  }

  return st ? st : puglWinEnter(view, expose);
}

static PuglStatus
puglWinCairoDamage(PuglView* view, const PuglDamageEvent* damage)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (surface->region) {
    const cairo_rectangle_int_t rect = {
      damage->x, damage->y, damage->width, damage->height};

    cairo_region_union_rectangle(surface->region, &rect);
    cairo_reset_clip(surface->cr);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoLeave(PuglView* view, const bool expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (expose) {
    // Flush surface to ensure all drawing is finished
    cairo_surface_flush(surface->surface);

    // Blit each exposed rectangle to the window drawing context
    const int n_rects = cairo_region_num_rectangles(surface->region);
    for (int i = 0; i < n_rects; ++i) {
      cairo_rectangle_int_t rect = {0, 0, 0, 0};
      cairo_region_get_rectangle(surface->region, i, &rect);

      BitBlt(impl->hdc,
             rect.x,
             rect.y,
             rect.width,
             rect.height,
             surface->drawDc,
             rect.x,
             rect.y,
             SRCCOPY);
    }

    // Destroy Cairo context/surface/region, and drawing context
    puglWinCairoClose(view);
    puglWinCairoDestroyDrawContext(view);
  }

  return puglWinLeave(view, expose);
}

static void*
puglWinCairoGetContext(PuglView* view)
{
  return ((PuglWinCairoSurface*)view->impl->surface)->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglWinCairoAttach,
                                      puglWinCairoDetach,
                                      puglStubCreate,
                                      puglWinCairoDestroy,
                                      puglWinCairoEnter,
                                      puglWinCairoDamage,
                                      puglWinCairoLeave,
                                      puglWinCairoGetContext};

  return &backend;
}
