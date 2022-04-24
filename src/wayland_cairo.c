// Copyright 2012-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "attributes.h"
#include "stub.h"
#include "types.h"
#include "wayland.h"

#include <pugl/cairo.h>
#include <pugl/pugl.h>

#include <cairo.h>

typedef struct {
  cairo_surface_t* back;
  cairo_surface_t* front;
  cairo_t*         cr;
} PuglWaylandCairoSurface;

static PuglStatus
puglWaylandCairoEnter(PuglView*              PUGL_UNUSED(view),
                      const PuglExposeEvent* PUGL_UNUSED(expose))
{
  return PUGL_UNKNOWN_ERROR;
}

static PuglStatus
puglWaylandCairoLeave(PuglView*              PUGL_UNUSED(view),
                      const PuglExposeEvent* PUGL_UNUSED(expose))
{
  return PUGL_UNKNOWN_ERROR;
}

static void*
puglWaylandCairoGetContext(PuglView* view)
{
  PuglInternals* const           impl = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;

  return surface->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglStubConfigure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglWaylandCairoEnter,
                                      puglWaylandCairoLeave,
                                      puglWaylandCairoGetContext};

  return &backend;
}
