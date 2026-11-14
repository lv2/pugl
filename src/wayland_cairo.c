// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "types.h"
#include "wayland.h"

#include <pugl/cairo.h>
#include <pugl/pugl.h>

#include <cairo.h>
#include <wayland-client-protocol.h>

#include <sys/mman.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
  cairo_surface_t*  surface;
  struct wl_buffer* buffer;
} PuglWaylandCairoBuffer;

typedef struct {
  PuglSpan               width;
  PuglSpan               height;
  void*                  data;
  PuglWaylandCairoBuffer buffers[2];
  cairo_t*               cr;
  cairo_region_t*        region;
  bool                   phase;
} PuglWaylandCairoSurface;

static void
puglWaylandCairoClose(PuglView* const view)
{
  PuglInternals* const           impl = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;

  cairo_region_destroy(surface->region);
  surface->region = NULL;

  for (unsigned i = 0U; i < 2U; ++i) {
    cairo_surface_destroy(surface->buffers[i].surface);
    surface->buffers[i].surface = NULL;
  }

  munmap(surface->data, (size_t)4U * surface->width * surface->height);
  surface->data = NULL;
}

static PuglStatus
puglWaylandCairoOpen(PuglView* const view,
                     const PuglSpan  width,
                     const PuglSpan  height)
{
  PuglWorld* const               world = view->world;
  PuglInternals* const           impl  = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;

  // Create shm file
  const int stride   = 4 * width;       // 4 bytes per pixesl
  const int size     = stride * height; // Size of a single buffer
  const int dataSize = 2 * size;        // Size of both buffers
  const int fd       = memfd_create("pugl-wl-shm", MFD_CLOEXEC);
  if (fd >= 0 && ftruncate(fd, dataSize) < 0) {
    perror("ftruncate");
    close(fd);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  if (fd < 0) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Map buffers data into memory
  surface->data =
    mmap(NULL, (size_t)dataSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (surface->data == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Create shm pool and create two buffers from it
  struct wl_shm_pool* const pool =
    wl_shm_create_pool(world->impl->shm, fd, dataSize);

  for (unsigned i = 0U; i < 2U; ++i) {
    surface->buffers[i].buffer = wl_shm_pool_create_buffer(
      pool, (int)i * size, width, height, stride, WL_SHM_FORMAT_XRGB8888);
  }

  // Destroy pool and close original fd (the buffers hold up references)
  wl_shm_pool_destroy(pool);
  close(fd);

  // Create an image surface for each buffer
  for (unsigned i = 0U; i < 2U; ++i) {
    const size_t offset = i * (size_t)size;
    surface->buffers[i].surface =
      cairo_image_surface_create_for_data((uint8_t*)surface->data + offset,
                                          CAIRO_FORMAT_RGB24,
                                          width,
                                          height,
                                          stride);
    if (cairo_surface_status(surface->buffers[i].surface)) {
      puglWaylandCairoClose(view);
      return PUGL_CREATE_CONTEXT_FAILED;
    }
  }

  if (!(surface->region = cairo_region_create())) {
    puglWaylandCairoClose(view);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  surface->width  = width;
  surface->height = height;
  return PUGL_SUCCESS;
}

static PuglStatus
puglWaylandCairoCreate(PuglView* view)
{
  PuglInternals* const impl = view->impl;

  impl->backendSurface =
    (PuglWaylandCairoSurface*)calloc(1, sizeof(PuglWaylandCairoSurface));

  return PUGL_SUCCESS;
}

static void
puglWaylandCairoDestroy(PuglView* view)
{
  PuglInternals* const           impl = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;

  puglWaylandCairoClose(view);
  free(surface);
}

static PuglStatus
puglWaylandCairoEnter(PuglView* const view, const bool expose)
{
  PuglStatus                     st   = PUGL_SUCCESS;
  PuglInternals* const           impl = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;
  if (expose) {
    const PuglArea viewSize = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    if (surface->width != viewSize.width ||
        surface->height != viewSize.height) {
      puglWaylandCairoClose(view);
      if ((st = puglWaylandCairoOpen(view, viewSize.width, viewSize.height))) {
        return st;
      }
    }

    surface->cr = cairo_create(surface->buffers[surface->phase].surface);
    if (cairo_status(surface->cr)) {
      cairo_destroy(surface->cr);
      surface->cr = NULL;
      st          = PUGL_CREATE_CONTEXT_FAILED;
    }
  }

  return st;
}

static PuglStatus
puglWaylandCairoDamage(PuglView* view, const PuglDamageEvent* const damage)
{
  PuglInternals* const           impl = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;
  if (surface->region) {
    const cairo_rectangle_int_t rect = {
      damage->x, damage->y, damage->width, damage->height};

    cairo_region_union_rectangle(surface->region, &rect);
    cairo_reset_clip(surface->cr);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglWaylandCairoLeave(PuglView* const view, const bool expose)
{
  PuglInternals* const           impl = view->impl;
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)impl->backendSurface;

  if (expose) {
    cairo_destroy(surface->cr);
    surface->cr = NULL;

    cairo_surface_flush(surface->buffers[surface->phase].surface);
    wl_surface_attach(
      view->impl->wlSurface, surface->buffers[surface->phase].buffer, 0, 0);

    surface->phase = !surface->phase;
  }

  return PUGL_SUCCESS;
}

static void*
puglWaylandCairoGetContext(PuglView* view)
{
  PuglWaylandCairoSurface* const surface =
    (PuglWaylandCairoSurface*)view->impl->backendSurface;

  return surface->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglStubAttach,
                                      puglStubDetach,
                                      puglWaylandCairoCreate,
                                      puglWaylandCairoDestroy,
                                      puglWaylandCairoEnter,
                                      puglWaylandCairoDamage,
                                      puglWaylandCairoLeave,
                                      puglWaylandCairoGetContext};

  return &backend;
}
