// Copyright 2012-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "attributes.h"
#include "stub.h"
#include "types.h"
#include "wayland.h"

#include <pugl/gl.h>
#include <pugl/pugl.h>

#include <wayland-egl-core.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <stdlib.h>

typedef struct {
  struct wl_egl_window* eglWindow;
  EGLDisplay            eglDisplay;
  EGLSurface            eglSurface;
  EGLContext            eglContext;
} PuglWaylandGlSurface;

static EGLint
puglEglHintValue(const int puglValue)
{
  return puglValue == PUGL_DONT_CARE ? EGL_DONT_CARE : puglValue;
}

static EGLint
puglEglGetConfigAttrib(const EGLDisplay display,
                       const EGLConfig  config,
                       const EGLint     attribute)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attribute, &value);
  return value;
}

static PuglArea
puglCurrentViewSize(const PuglView* const view)
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

static PuglStatus
puglWaylandGlConfigure(PuglView* view)
{
  PuglInternals* const impl = view->impl;

  // Hints where EGL expects 0 (not -1) for "don't care" values
  static const PuglViewHint nonNegativeHints[] = {
    PUGL_RED_BITS,
    PUGL_GREEN_BITS,
    PUGL_BLUE_BITS,
    PUGL_ALPHA_BITS,
    PUGL_DEPTH_BITS,
    PUGL_STENCIL_BITS,
    PUGL_SAMPLE_BUFFERS,
    PUGL_SAMPLES,
    (PuglViewHint)0,
  };

  // Adjust view hints to EGL values as necessary
  for (const PuglViewHint* h = nonNegativeHints; *h; ++h) {
    if (view->hints[*h] < 0) {
      view->hints[*h] = 0;
    }
  }

  const EGLenum api = view->hints[PUGL_CONTEXT_API] == PUGL_OPENGL_API
                        ? EGL_OPENGL_API
                        : EGL_OPENGL_ES_API;

  const EGLint profile =
    view->hints[PUGL_CONTEXT_PROFILE] == PUGL_OPENGL_COMPATIBILITY_PROFILE
      ? EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
      : EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;

  const EGLint contextAttribs[] = {
    EGL_CONTEXT_MAJOR_VERSION,
    view->hints[PUGL_CONTEXT_VERSION_MAJOR],

    EGL_CONTEXT_MINOR_VERSION,
    view->hints[PUGL_CONTEXT_VERSION_MINOR],

    EGL_CONTEXT_OPENGL_DEBUG,
    puglEglHintValue(view->hints[PUGL_CONTEXT_DEBUG]),

    api == EGL_OPENGL_API ? EGL_CONTEXT_OPENGL_PROFILE_MASK : EGL_NONE,
    api == EGL_OPENGL_API ? profile : EGL_NONE,

    EGL_NONE,
    EGL_NONE,
  };

  // clang-format off
  const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
    EGL_RED_SIZE,          view->hints[PUGL_RED_BITS],
    EGL_GREEN_SIZE,        view->hints[PUGL_GREEN_BITS],
    EGL_BLUE_SIZE,         view->hints[PUGL_BLUE_BITS],
    EGL_ALPHA_SIZE,        view->hints[PUGL_ALPHA_BITS],
    EGL_DEPTH_SIZE,        view->hints[PUGL_DEPTH_BITS],
    EGL_STENCIL_SIZE,      view->hints[PUGL_STENCIL_BITS],
    EGL_SAMPLE_BUFFERS,    view->hints[PUGL_SAMPLE_BUFFERS],
    EGL_SAMPLES,           view->hints[PUGL_SAMPLES],
    EGL_MIN_SWAP_INTERVAL, view->hints[PUGL_SWAP_INTERVAL],
    EGL_NONE,              EGL_NONE,
  };
  // clang-format on

  // Create EGL display connection
  const EGLDisplay eglDisplay = eglGetDisplay(view->world->impl->display);
  if (eglDisplay == EGL_NO_DISPLAY) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Initialize EGL
  EGLint majorVersion = 0;
  EGLint minorVersion = 0;
  if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion)) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Bind requested render API
  if (!eglBindAPI(api)) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Get configs
  EGLint numConfigs = 0;
  if (!eglGetConfigs(eglDisplay, NULL, 0, &numConfigs) || !numConfigs) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Choose config
  EGLConfig config = 0;
  if (!eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfigs) ||
      numConfigs != 1) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Create Wayland EGL window
  const PuglArea              size = puglCurrentViewSize(view);
  struct wl_egl_window* const eglWindow =
    wl_egl_window_create(impl->wlSurface, (int)size.width, (int)size.height);
  if (!eglWindow) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Create a window surface
  const EGLSurface eglSurface =
    eglCreateWindowSurface(eglDisplay, config, eglWindow, NULL);
  if (eglSurface == EGL_NO_SURFACE) {
    wl_egl_window_destroy(eglWindow);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Create a context
  const EGLContext eglContext =
    eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
  if (eglContext == EGL_NO_CONTEXT) {
    eglDestroySurface(eglDisplay, eglSurface);
    wl_egl_window_destroy(eglWindow);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  PuglWaylandGlSurface* const backendSurface =
    (PuglWaylandGlSurface*)calloc(1, sizeof(PuglWaylandGlSurface));

  backendSurface->eglWindow  = eglWindow;
  backendSurface->eglDisplay = eglDisplay;
  backendSurface->eglSurface = eglSurface;
  backendSurface->eglContext = eglContext;
  impl->backendSurface       = backendSurface;

  // Update possibly ambiguous hints to reflect reality
  view->hints[PUGL_RED_BITS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_RED_SIZE);
  view->hints[PUGL_GREEN_BITS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE);
  view->hints[PUGL_BLUE_BITS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE);
  view->hints[PUGL_ALPHA_BITS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_ALPHA_SIZE);
  view->hints[PUGL_DEPTH_BITS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_DEPTH_SIZE);
  view->hints[PUGL_STENCIL_BITS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_STENCIL_SIZE);
  view->hints[PUGL_SAMPLE_BUFFERS] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_SAMPLE_BUFFERS);
  view->hints[PUGL_SAMPLES] =
    puglEglGetConfigAttrib(eglDisplay, config, EGL_SAMPLES);
  view->hints[PUGL_DOUBLE_BUFFER] =
    !puglEglGetConfigAttrib(eglDisplay, config, EGL_SINGLE_BUFFER);
  view->hints[PUGL_SWAP_INTERVAL] =
    !puglEglGetConfigAttrib(eglDisplay, config, EGL_MIN_SWAP_INTERVAL);

  return PUGL_SUCCESS;
}

static PuglStatus
puglWaylandGlCreate(PuglView* PUGL_UNUSED(view))
{
  return PUGL_SUCCESS;
}

static void
puglWaylandGlDestroy(PuglView* PUGL_UNUSED(view))
{}

static PuglStatus
puglWaylandGlEnter(PuglView* view, const PuglExposeEvent* PUGL_UNUSED(expose))
{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;

  return surface && eglMakeCurrent(surface->eglDisplay,
                                   surface->eglSurface,
                                   surface->eglSurface,
                                   surface->eglContext)
           ? PUGL_SUCCESS
           : PUGL_FAILURE;
}

static PuglStatus
puglWaylandGlLeave(PuglView* view, const PuglExposeEvent* expose)

{
  PuglWaylandGlSurface* const surface =
    (PuglWaylandGlSurface*)view->impl->backendSurface;
  if (!surface) {
    return PUGL_FAILURE;
  }

  if (expose) {
    eglSwapBuffers(surface->eglDisplay, surface->eglSurface);
  }

  if (!eglMakeCurrent(
        surface->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
    return PUGL_UNKNOWN_ERROR;
  }

  return PUGL_SUCCESS;
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
  static const PuglBackend backend = {puglWaylandGlConfigure,
                                      puglWaylandGlCreate,
                                      puglWaylandGlDestroy,
                                      puglWaylandGlEnter,
                                      puglWaylandGlLeave,
                                      puglStubGetContext};

  return &backend;
}
