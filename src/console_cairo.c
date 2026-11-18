// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "console.h"
#include "stub.h"
#include "types.h"

#include <pugl/cairo.h>
#include <pugl/pugl.h>

#include <cairo.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int                      fd;      // Framebuffer file descriptor
  struct fb_fix_screeninfo finfo;   // Fixed screen information
  struct fb_var_screeninfo vinfo;   // Variable screen information
  uint32_t*                pixels;  // Memory-mapped pixel buffer
  cairo_surface_t*         surface; // Cairo surface
  cairo_t*                 cr;      // Cairo context
  cairo_region_t*          region;  // Damage region
} PuglConsoleCairoSurface;

static PuglStatus
puglConsoleCairoAttach(PuglView* const view)
{
  const int fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
  if (fd < 0) {
    return PUGL_BACKEND_FAILED;
  }

  struct fb_var_screeninfo vinfo = {0};
  struct fb_fix_screeninfo finfo = {0};
  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1 ||
      ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
    close(fd);
    return PUGL_BACKEND_FAILED;
  }

  // const uint32_t width  = surface->vinfo.xres;
  // const uint32_t height = surface->vinfo.yres;
  const uint32_t size = finfo.smem_len;

  // Map framebuffer to memory
  void* const pixels =
    mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (pixels == MAP_FAILED) {
    close(fd);
    return PUGL_BACKEND_FAILED;
  }

  PuglConsoleCairoSurface* surface =
    (PuglConsoleCairoSurface*)calloc(1, sizeof(PuglConsoleCairoSurface));
  if (!surface) {
    return PUGL_NO_MEMORY;
  }

  surface->fd         = fd;
  surface->finfo      = finfo;
  surface->vinfo      = vinfo;
  surface->pixels     = (uint32_t*)pixels;
  view->impl->surface = surface;
  return PUGL_SUCCESS;
}

static void
puglConsoleCairoDetach(PuglView* const view)
{
  free(view->impl->surface);
  view->impl->surface = NULL;
}

static void
puglConsoleCairoClose(PuglView* const view)
{
  PuglInternals* const           impl = view->impl;
  PuglConsoleCairoSurface* const surface =
    (PuglConsoleCairoSurface*)impl->surface;

  cairo_region_destroy(surface->region);
  surface->region = NULL;

  cairo_destroy(surface->cr);
  surface->cr = NULL;

  cairo_surface_destroy(surface->surface);
  surface->surface = NULL;
}

static PuglStatus
puglConsoleCairoOpen(PuglView* const view)
{
  PuglInternals* const           impl = view->impl;
  PuglConsoleCairoSurface* const surface =
    (PuglConsoleCairoSurface*)impl->surface;

  if (!(surface->surface =
          cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                     (int)surface->vinfo.xres,
                                     (int)surface->vinfo.yres)) ||
      cairo_surface_status(surface->surface) ||
      !(surface->cr = cairo_create(surface->surface)) ||
      cairo_status(surface->cr) || !(surface->region = cairo_region_create())) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static void
puglConsoleCairoDestroy(PuglView* const view)
{
  puglConsoleCairoClose(view);
}

static PuglStatus
puglConsoleCairoEnter(PuglView* const view, const bool expose)
{
  return expose ? puglConsoleCairoOpen(view) : PUGL_SUCCESS;
}

static PuglStatus
puglConsoleCairoDamage(PuglView* const              view,
                       const PuglDamageEvent* const damage)
{
  PuglInternals* const           impl = view->impl;
  PuglConsoleCairoSurface* const surface =
    (PuglConsoleCairoSurface*)impl->surface;

  if (surface->region) {
    const cairo_rectangle_int_t rect = {
      damage->x, damage->y, damage->width, damage->height};

    cairo_region_union_rectangle(surface->region, &rect);
    cairo_reset_clip(surface->cr);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglConsoleCairoLeave(PuglView* const view, const bool expose)
{
  PuglInternals* const           impl = view->impl;
  PuglConsoleCairoSurface* const surface =
    (PuglConsoleCairoSurface*)impl->surface;

  if (expose) {
    // Flush surface to ensure all drawing is finished
    cairo_surface_flush(surface->surface);

    // Blit each exposed rectangle to the window drawing context
    const int            nRects = cairo_region_num_rectangles(surface->region);
    const uint8_t* const data = cairo_image_surface_get_data(surface->surface);
    for (int i = 0; i < nRects; ++i) {
      cairo_rectangle_int_t rect = {0, 0, 0, 0};
      cairo_region_get_rectangle(surface->region, i, &rect);

      for (unsigned y = (unsigned)rect.y; y < (unsigned)rect.height; ++y) {
        const unsigned start = (y * surface->vinfo.xres) + (unsigned)rect.x;
        memcpy(&surface->pixels[start],
               data + (sizeof(uint32_t) * start),
               sizeof(uint32_t) * (unsigned)rect.width);
      }
    }

    // Destroy Cairo context/surface/region, and drawing context
    puglConsoleCairoClose(view);
  }

  return PUGL_SUCCESS;
}

static void*
puglConsoleCairoGetContext(PuglView* const view)
{
  return ((PuglConsoleCairoSurface*)view->impl->surface)->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglConsoleCairoAttach,
                                      puglConsoleCairoDetach,
                                      puglStubCreate,
                                      puglConsoleCairoDestroy,
                                      puglConsoleCairoEnter,
                                      puglConsoleCairoDamage,
                                      puglConsoleCairoLeave,
                                      puglConsoleCairoGetContext};

  return &backend;
}
