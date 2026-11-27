// Copyright 2012-2026 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "attributes.h"
#include "stub.h"
#include "types.h"
#include "web.h"

#include <pugl/gl.h>
#include <pugl/pugl.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <stdbool.h>
#include <stdlib.h>

// NOLINTBEGIN(misc-misplaced-const)

typedef struct {
  EGLDisplay display;
  EGLConfig  config;
  EGLSurface surface;
  EGLContext context;
} PuglWebGlSurface;

typedef int (*RankConfigFunc)(EGLDisplay display,
                              EGLConfig  config,
                              const int  hints[static PUGL_NUM_VIEW_HINTS]);

static int
getAttrib(const EGLDisplay display, const EGLConfig config, const EGLint attrib)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attrib, &value);
  return value;
}

static EGLConfig
chooseConfiguration(const EGLDisplay    eglDisplay,
                    RankConfigFunc      rankConfig,
                    int                 hints[PUGL_NUM_VIEW_HINTS],
                    const EGLint* const configAttribs)
{
  // Get the number of available configurations
  EGLint maxNumConfigs = 0;
  if (!eglGetConfigs(eglDisplay, NULL, 0, &maxNumConfigs) ||
      maxNumConfigs <= 0) {
    return NULL;
  }

  // Choose compatible configurations
  EGLint           numConfigs = 0;
  EGLConfig* const configs =
    (EGLConfig*)calloc((size_t)maxNumConfigs, sizeof(EGLConfig));
  if (!configs ||
      !eglChooseConfig(
        eglDisplay, configAttribs, configs, maxNumConfigs, &numConfigs) ||
      !numConfigs) {
    free(configs);
    return NULL;
  }

  // Let the calling code choose from those
  EGLint bestConfigIndex = -1;
  int    bestConfigRank  = 0;
  for (EGLint i = 0; i < numConfigs; ++i) {
    const int rank = rankConfig(eglDisplay, configs[i], hints);
    if (rank > bestConfigRank) {
      bestConfigIndex = i;
      bestConfigRank  = rank;
    }
  }

  const EGLConfig config =
    (bestConfigIndex < 0) ? NULL : configs[bestConfigIndex];

  free(configs);
  return config;
}

static EGLint
getConfigNumber(const EGLDisplay display,
                const EGLConfig  config,
                const EGLint     attrib)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attrib, &value);
  return value;
}

static int
rankConfig(const EGLDisplay display,
           const EGLConfig  config,
           const int        hints[static PUGL_NUM_VIEW_HINTS])
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
puglWebGlAttach(PuglView* const view)
{
  PuglWebGlSurface* const surface =
    (PuglWebGlSurface*)calloc(1, sizeof(PuglWebGlSurface));
  view->impl->surface = surface;

  int* const hints = view->hints;

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

  // Adjust hints to EGL values as necessary
  for (const PuglViewHint* h = nonNegativeHints; *h; ++h) {
    if (hints[*h] < 0) {
      hints[*h] = 0;
    }
  }

  // clang-format off
  const EGLint configAttribs[] = {
    EGL_RENDERABLE_TYPE,    EGL_OPENGL_BIT,
    EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
    EGL_TRANSPARENT_TYPE,   EGL_NONE,
    EGL_BUFFER_SIZE,        32,
    EGL_RED_SIZE,           hints[PUGL_RED_BITS],
    EGL_GREEN_SIZE,         hints[PUGL_GREEN_BITS],
    EGL_BLUE_SIZE,          hints[PUGL_BLUE_BITS],
    EGL_ALPHA_SIZE,         hints[PUGL_ALPHA_BITS],
    EGL_DEPTH_SIZE,         hints[PUGL_DEPTH_BITS],
    EGL_STENCIL_SIZE,       hints[PUGL_STENCIL_BITS],
    EGL_SAMPLE_BUFFERS,     hints[PUGL_SAMPLE_BUFFERS],
    EGL_SAMPLES,            hints[PUGL_SAMPLES],
    EGL_MIN_SWAP_INTERVAL,  hints[PUGL_SWAP_INTERVAL],
    EGL_NONE,               EGL_NONE,
  };
  // clang-format on

  // Create EGL display connection
  const EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY;
  const EGLDisplay           display       = eglGetDisplay(nativeDisplay);
  if (display == EGL_NO_DISPLAY) {
    return PUGL_CONNECT_FAILED;
  }

  // Initialize EGL
  EGLint majorVersion = 0;
  EGLint minorVersion = 0;
  if (!eglInitialize(display, &majorVersion, &minorVersion)) {
    return PUGL_CONNECT_FAILED;
  }

  // Bind requested render API
  /*
  if (!eglBindAPI(hints[PUGL_CONTEXT_API] == PUGL_OPENGL_API
                    ? EGL_OPENGL_API
                    : EGL_OPENGL_ES_API)) {
    eglTerminate(display);
    return PUGL_UNSUPPORTED;
  }
  */

  // Choose EGL configuration
  const EGLConfig config =
    chooseConfiguration(display, rankConfig, hints, configAttribs);
  if (!config) {
    eglTerminate(display);
    return PUGL_SET_FORMAT_FAILED;
  }

  // Create a window surface
  const EGLNativeWindowType nativeWindow = 0;
  const EGLSurface          eglSurface =
    eglCreateWindowSurface(display, config, nativeWindow, NULL);
  if (eglSurface == EGL_NO_SURFACE) {
    eglTerminate(display);
    return PUGL_REALIZE_FAILED;
  }

  surface->display = display;
  surface->config  = config;
  surface->surface = eglSurface;
  surface->context = NULL;

  // Update possibly ambiguous hints to reflect reality
  hints[PUGL_RED_BITS]       = getAttrib(display, config, EGL_RED_SIZE);
  hints[PUGL_GREEN_BITS]     = getAttrib(display, config, EGL_GREEN_SIZE);
  hints[PUGL_BLUE_BITS]      = getAttrib(display, config, EGL_BLUE_SIZE);
  hints[PUGL_ALPHA_BITS]     = getAttrib(display, config, EGL_ALPHA_SIZE);
  hints[PUGL_DEPTH_BITS]     = getAttrib(display, config, EGL_DEPTH_SIZE);
  hints[PUGL_STENCIL_BITS]   = getAttrib(display, config, EGL_STENCIL_SIZE);
  hints[PUGL_SAMPLE_BUFFERS] = getAttrib(display, config, EGL_SAMPLE_BUFFERS);
  hints[PUGL_SAMPLES]        = getAttrib(display, config, EGL_SAMPLES);
  hints[PUGL_DOUBLE_BUFFER]  = !getAttrib(display, config, EGL_SINGLE_BUFFER);
  hints[PUGL_SWAP_INTERVAL] = getAttrib(display, config, EGL_MIN_SWAP_INTERVAL);

  return PUGL_SUCCESS;
}

static void
puglWebGlDetach(PuglView* const view)
{
  free(view->impl->surface);
  view->impl->surface = NULL;
}

static PuglStatus
puglWebGlEnter(PuglView* const view, bool PUGL_UNUSED(expose))
{
  PuglWebGlSurface* const surface = (PuglWebGlSurface*)view->impl->surface;

  return (surface && surface->context &&
          eglMakeCurrent(surface->display,
                         surface->surface,
                         surface->surface,
                         surface->context))
           ? PUGL_SUCCESS
           : PUGL_FAILURE;
}

static PuglStatus
puglWebGlLeave(PuglView* const view, const bool expose)
{
  PuglWebGlSurface* const surface = (PuglWebGlSurface*)view->impl->surface;

  if (expose) {
    eglSwapBuffers(surface->display, surface->surface);
  }

  return eglMakeCurrent(
           surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)
           ? PUGL_SUCCESS
           : PUGL_FAILURE;
}

static PuglStatus
puglWebGlCreate(PuglView* const view)
{
  PuglWebGlSurface* const surface = (PuglWebGlSurface*)view->impl->surface;
  const EGLDisplay        display = surface->display;
  const EGLConfig         config  = surface->config;

  /*
  const EGLenum api = view->hints[PUGL_CONTEXT_API] == PUGL_OPENGL_API
                        ? EGL_OPENGL_API
                        : EGL_OPENGL_ES_API;

  const EGLint profile =
    view->hints[PUGL_CONTEXT_PROFILE] == PUGL_OPENGL_COMPATIBILITY_PROFILE
      ? EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
      : EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;
  */

  const EGLint attrs[] = {
    EGL_CONTEXT_MAJOR_VERSION,
    view->hints[PUGL_CONTEXT_VERSION_MAJOR],

    // EGL_CONTEXT_MINOR_VERSION,
    // view->hints[PUGL_CONTEXT_VERSION_MINOR],

    /* api == EGL_OPENGL_API ? EGL_CONTEXT_OPENGL_PROFILE_MASK : EGL_NONE, */
    /* api == EGL_OPENGL_API ? profile : EGL_NONE, */

    EGL_NONE};

  surface->context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs);
  if (surface->context == EGL_NO_CONTEXT) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  surface->surface = eglCreateWindowSurface(display, config, 0, NULL);
  if (surface->surface == EGL_NO_SURFACE) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static void
puglWebGlDestroy(PuglView* const view)
{
  PuglWebGlSurface* const surface = (PuglWebGlSurface*)view->impl->surface;
  if (surface) {
    const EGLDisplay display = surface->display;
    if (surface->surface) {
      eglDestroySurface(display, surface->surface);
    }
    if (surface->context) {
      eglDestroyContext(display, surface->context);
    }
    eglTerminate(display);
    free(surface);
    view->impl->surface = NULL;
  }
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
  return eglGetProcAddress(name);
}

PuglStatus
puglEnterContext(PuglView* const view)
{
  return view->backend->enter(view, false);
}

PuglStatus
puglLeaveContext(PuglView* const view)
{
  return view->backend->leave(view, false);
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglWebGlAttach,
                                      puglWebGlDetach,
                                      puglWebGlCreate,
                                      puglWebGlDestroy,
                                      puglWebGlEnter,
                                      puglStubDamage,
                                      puglWebGlLeave,
                                      puglStubGetContext};
  return &backend;
}

// NOLINTEND(misc-misplaced-const)
