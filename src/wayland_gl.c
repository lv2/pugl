// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "egl_surface.h"
#include "stub.h"
#include "types.h"
#include "wayland.h"

#include <pugl/gl.h>
#include <pugl/pugl.h>

#include <wayland-egl-core.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  struct wl_egl_window* eglWindow;
  PuglEglSurface        egl;
} PuglWaylandGlSurface;

static EGLint
getConfigNumber(EGLDisplay display, EGLConfig config, const EGLint attrib)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attrib, &value);
  return value;
}

static int
rankConfig(EGLDisplay display,
           EGLConfig  config,
           const int  hints[static PUGL_NUM_VIEW_HINTS])
{
  const EGLint bufferSize  = getConfigNumber(display, config, EGL_BUFFER_SIZE);
  const EGLint redSize     = getConfigNumber(display, config, EGL_RED_SIZE);
  const EGLint greenSize   = getConfigNumber(display, config, EGL_GREEN_SIZE);
  const EGLint blueSize    = getConfigNumber(display, config, EGL_BLUE_SIZE);
  const EGLint alphaSize   = getConfigNumber(display, config, EGL_ALPHA_SIZE);
  const EGLint depthSize   = getConfigNumber(display, config, EGL_DEPTH_SIZE);
  const EGLint stencilSize = getConfigNumber(display, config, EGL_STENCIL_SIZE);

  return ((bufferSize == 32) + (redSize == hints[PUGL_RED_BITS]) +
          (greenSize == hints[PUGL_GREEN_BITS]) +
          (blueSize == hints[PUGL_BLUE_BITS]) +
          (alphaSize == hints[PUGL_ALPHA_BITS]) +
          (depthSize == hints[PUGL_DEPTH_BITS]) +
          (stencilSize == hints[PUGL_STENCIL_BITS]));
}

static PuglStatus
puglWaylandGlAttach(PuglView* view)
{
  PuglStatus           st   = PUGL_SUCCESS;
  PuglInternals* const impl = view->impl;

  PuglWaylandGlSurface* const backendSurface =
    (PuglWaylandGlSurface*)calloc(1, sizeof(PuglWaylandGlSurface));

  // Create Wayland EGL window
  backendSurface->eglWindow =
    wl_egl_window_create(impl->wlSurface,
                         (int)view->lastConfigure.width,
                         (int)view->lastConfigure.height);
  if (!backendSurface->eglWindow) {
    free(backendSurface);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Create an EGL surface for the window
  if ((st = puglEglAttach(&backendSurface->egl,
                          view->world->impl->display,
                          (EGLNativeWindowType)backendSurface->eglWindow,
                          rankConfig,
                          view->hints))) {
    wl_egl_window_destroy(backendSurface->eglWindow);
    free(backendSurface);
    return st;
  }

  impl->backendSurface = backendSurface;
  return st;
}

static void
puglWaylandGlDetach(PuglView* const view)
{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;

  if (surface) {
    puglEglDetach(&surface->egl);
    wl_egl_window_destroy(surface->eglWindow);

    free(view->impl->backendSurface);
    view->impl->backendSurface = NULL;
  }
}

static PuglStatus
puglWaylandGlCreate(PuglView* const view)
{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;

  return puglEglCreate(&surface->egl, view->hints);
}

static void
puglWaylandGlDestroy(PuglView* const view)
{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;

  puglEglDestroy(&surface->egl);
}

static PuglStatus
puglWaylandGlEnter(PuglView* view, const bool expose)
{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;
  if (!surface) {
    return PUGL_FAILURE;
  }

  // Resize the EGL window if necessary
  const PuglSpan width          = view->lastConfigure.width;
  const PuglSpan height         = view->lastConfigure.height;
  int            attachedWidth  = 0;
  int            attachedHeight = 0;
  wl_egl_window_get_attached_size(
    surface->eglWindow, &attachedWidth, &attachedHeight);
  if (attachedWidth != width || attachedHeight != height) {
    wl_egl_window_resize(surface->eglWindow, width, height, 0, 0);
  }

  return puglEglEnter(&surface->egl, expose);
}

static PuglStatus
puglWaylandGlLeave(PuglView* view, const bool expose)
{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;

  return surface ? puglEglLeave(&surface->egl, expose) : PUGL_FAILURE;
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
  return eglGetProcAddress(name);
}

PuglStatus
puglEnterContext(PuglView* view)
{
  return view->backend->enter(view, NULL);
}

PuglStatus
puglLeaveContext(PuglView* view)
{
  return view->backend->leave(view, NULL);
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglWaylandGlAttach,
                                      puglWaylandGlDetach,
                                      puglWaylandGlCreate,
                                      puglWaylandGlDestroy,
                                      puglWaylandGlEnter,
                                      puglStubDamage,
                                      puglWaylandGlLeave,
                                      puglStubGetContext};

  return &backend;
}
