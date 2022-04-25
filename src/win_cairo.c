// Copyright 2012-2022 David Robillard <d@drobilla.net>
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
puglWinCairoConfigure(PuglView* view)
{
  const PuglStatus st = puglWinConfigure(view);

  if (!st) {
    view->impl->surface =
      (PuglWinCairoSurface*)calloc(1, sizeof(PuglWinCairoSurface));
  }

  return st;
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
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  puglWinCairoClose(view);
  puglWinCairoDestroyDrawContext(view);
  free(surface);
  impl->surface = NULL;
}

static PuglStatus
puglWinCairoEnter(PuglView* view, const bool drawing)
{
  PuglStatus st = PUGL_SUCCESS;

  if (drawing) {
    if (!(st = puglWinCairoCreateDrawContext(view))) {
      st = puglWinCairoOpen(view);
    }
  }

  return st ? st : PUGL_IGNORE;
}

static PuglStatus
puglWinCairoExpose(PuglView* view, const PuglExposeEvent* expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (surface->region) {
    const cairo_rectangle_int_t rect = {
      expose->x, expose->y, expose->width, expose->height};

    cairo_region_union_rectangle(surface->region, &rect);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoHandle(PuglView* view, const PuglEvent* const event)
{
  return event->type == PUGL_EXPOSE ? puglWinCairoExpose(view, &event->expose)
                                    : PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoLeave(PuglView* view, const bool drawing)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (drawing && surface->region) {
    cairo_surface_flush(surface->surface);

    // Paint exposed region(s) from the Cairo surface to the window
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
             0,
             0,
             SRCCOPY);
    }

    puglWinCairoClose(view);
    puglWinCairoDestroyDrawContext(view);
  }

  return PUGL_IGNORE;
}

static void*
puglWinCairoGetContext(PuglView* view)
{
  return ((PuglWinCairoSurface*)view->impl->surface)->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglWinCairoConfigure,
                                      puglStubCreate,
                                      puglWinCairoDestroy,
                                      puglWinCairoEnter,
                                      puglWinCairoLeave,
                                      puglWinCairoHandle,
                                      puglWinCairoGetContext};

  return &backend;
}
