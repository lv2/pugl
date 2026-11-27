// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <pugl/cairo.h>

#include "attributes.h"
#include "stub.h"
#include "types.h"
#include "web.h"

#include <cairo.h> // actually caiwo

#include <pugl/pugl.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  cairo_t* cr;
} PuglWebCairoSurface;

static const char*
getCanvasId(const PuglView* const view)
{
  const char* const string = view->strings[PUGL_CANVAS_ID];
  return string ? string : "canvas";
}

static PuglStatus
puglWebCairoCreate(PuglView* view)
{
  PuglInternals* const impl = view->impl;

  cairo_t* const cr = cairo_create_for_canvas(getCanvasId(view));
  if (!cr) {
    return PUGL_NO_MEMORY;
  }

  PuglWebCairoSurface* const surface =
    (PuglWebCairoSurface*)calloc(1, sizeof(PuglWebCairoSurface));
  if (!surface) {
    cairo_destroy(cr);
    return PUGL_NO_MEMORY;
  }

  surface->cr   = cr;
  impl->surface = surface;
  return PUGL_SUCCESS;
}

static void
puglWebCairoDestroy(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWebCairoSurface* const surface = (PuglWebCairoSurface*)impl->surface;

  cairo_destroy(surface->cr);
  free(surface);
}

static PuglStatus
puglWebCairoEnter(PuglView* view, bool PUGL_UNUSED(expose))
{
  PuglInternals* const       impl    = view->impl;
  PuglWebCairoSurface* const surface = (PuglWebCairoSurface*)impl->surface;

  cairo_save(surface->cr);
  return PUGL_SUCCESS;
}

static PuglStatus
puglWebCairoLeave(PuglView* view, bool PUGL_UNUSED(expose))
{
  PuglInternals* const       impl    = view->impl;
  PuglWebCairoSurface* const surface = (PuglWebCairoSurface*)impl->surface;

  cairo_restore(surface->cr);
  return PUGL_SUCCESS;
}

static void*
puglWebCairoGetContext(PuglView* const view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWebCairoSurface* const surface = (PuglWebCairoSurface*)impl->surface;

  return surface->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {
    puglStubAttach,
    puglStubDetach,
    puglWebCairoCreate,
    puglWebCairoDestroy,
    puglWebCairoEnter,
    puglStubDamage,
    puglWebCairoLeave,
    puglWebCairoGetContext,
  };

  return &backend;
}
