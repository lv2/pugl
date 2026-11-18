// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "egl_surface.h"

#include <pugl/pugl.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(misc-misplaced-const)

static EGLint
hintValue(const int puglValue)
{
  return puglValue == PUGL_DONT_CARE ? EGL_DONT_CARE : puglValue;
}

static EGLint
getConfigAttrib(const EGLDisplay display,
                const EGLConfig  config,
                const EGLint     attribute)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attribute, &value);
  return value;
}

static EGLConfig
chooseConfiguration(const EGLDisplay    eglDisplay,
                    RankConfigFunc      rankConfig,
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
    const int rank = rankConfig(eglDisplay, configs[i]);
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

PuglStatus
puglEglAttach(PuglEglSurface* const      surface,
              const EGLNativeDisplayType nativeDisplay,
              const EGLNativeWindowType  nativeWindow,
              RankConfigFunc             rankConfig,
              int                        hints[PUGL_NUM_VIEW_HINTS])
{
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
  const EGLDisplay eglDisplay = eglGetDisplay(nativeDisplay);
  if (eglDisplay == EGL_NO_DISPLAY) {
    return PUGL_CONNECT_FAILED;
  }

  // Initialize EGL
  EGLint majorVersion = 0;
  EGLint minorVersion = 0;
  if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion)) {
    return PUGL_CONNECT_FAILED;
  }

  // Bind requested render API
  if (!eglBindAPI(hints[PUGL_CONTEXT_API] == PUGL_OPENGL_API
                    ? EGL_OPENGL_API
                    : EGL_OPENGL_ES_API)) {
    eglTerminate(eglDisplay);
    return PUGL_BIND_FAILED;
  }

  // Choose EGL configuration
  const EGLConfig config =
    chooseConfiguration(eglDisplay, rankConfig, configAttribs);
  if (!config) {
    eglTerminate(eglDisplay);
    return PUGL_BAD_CONFIGURATION;
  }

  // Create a window surface
  const EGLSurface eglSurface =
    eglCreateWindowSurface(eglDisplay, config, nativeWindow, NULL);
  if (eglSurface == EGL_NO_SURFACE) {
    eglTerminate(eglDisplay);
    return PUGL_REALIZE_FAILED;
  }

  surface->display = eglDisplay;
  surface->config  = config;
  surface->surface = eglSurface;
  surface->context = NULL;

  // Update possibly ambiguous hints to reflect reality
  hints[PUGL_RED_BITS]   = getConfigAttrib(eglDisplay, config, EGL_RED_SIZE);
  hints[PUGL_GREEN_BITS] = getConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE);
  hints[PUGL_BLUE_BITS]  = getConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE);
  hints[PUGL_ALPHA_BITS] = getConfigAttrib(eglDisplay, config, EGL_ALPHA_SIZE);
  hints[PUGL_DEPTH_BITS] = getConfigAttrib(eglDisplay, config, EGL_DEPTH_SIZE);
  hints[PUGL_STENCIL_BITS] =
    getConfigAttrib(eglDisplay, config, EGL_STENCIL_SIZE);
  hints[PUGL_SAMPLE_BUFFERS] =
    getConfigAttrib(eglDisplay, config, EGL_SAMPLE_BUFFERS);
  hints[PUGL_SAMPLES] = getConfigAttrib(eglDisplay, config, EGL_SAMPLES);
  hints[PUGL_DOUBLE_BUFFER] =
    !getConfigAttrib(eglDisplay, config, EGL_SINGLE_BUFFER);
  hints[PUGL_SWAP_INTERVAL] =
    !getConfigAttrib(eglDisplay, config, EGL_MIN_SWAP_INTERVAL);

  return PUGL_SUCCESS;
}

void
puglEglDetach(PuglEglSurface* const surface)
{
  if (surface) {
    eglDestroySurface(surface->display, surface->surface);
    eglTerminate(surface->display);
    memset(surface, 0, sizeof(*surface));
  }
}

PuglStatus
puglEglCreate(PuglEglSurface* const surface,
              const int             hints[PUGL_NUM_VIEW_HINTS])
{
  const EGLenum api = hints[PUGL_CONTEXT_API] == PUGL_OPENGL_API
                        ? EGL_OPENGL_API
                        : EGL_OPENGL_ES_API;

  const EGLint profile =
    hints[PUGL_CONTEXT_PROFILE] == PUGL_OPENGL_COMPATIBILITY_PROFILE
      ? EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
      : EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;

  const EGLint contextAttribs[] = {
    // clang-format off
    EGL_CONTEXT_MAJOR_VERSION, hints[PUGL_CONTEXT_VERSION_MAJOR],
    EGL_CONTEXT_MINOR_VERSION, hints[PUGL_CONTEXT_VERSION_MINOR],
    EGL_CONTEXT_OPENGL_DEBUG,  hintValue(hints[PUGL_CONTEXT_DEBUG]),

    api == EGL_OPENGL_API ? EGL_CONTEXT_OPENGL_PROFILE_MASK : EGL_NONE,
    api == EGL_OPENGL_API ? profile                         : EGL_NONE,
    // clang-format on

    EGL_NONE,
    EGL_NONE,
  };

  // Create a context
  const EGLContext eglContext = eglCreateContext(
    surface->display, surface->config, EGL_NO_CONTEXT, contextAttribs);
  if (eglContext == EGL_NO_CONTEXT) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  surface->context = eglContext;
  return PUGL_SUCCESS;
}

void
puglEglDestroy(PuglEglSurface* const surface)
{
  if (surface) {
    eglDestroyContext(surface->display, surface->context);
    surface->context = NULL;
  }
}

PuglStatus
puglEglEnter(PuglEglSurface* const surface, const bool expose)
{
  (void)expose;

  return surface && eglMakeCurrent(surface->display,
                                   surface->surface,
                                   surface->surface,
                                   surface->context)
           ? PUGL_SUCCESS
           : PUGL_FAILURE;
}

PuglStatus
puglEglLeave(PuglEglSurface* const surface, const bool expose)
{
  if (expose) {
    eglSwapBuffers(surface->display, surface->surface);
  }

  return eglMakeCurrent(
           surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)
           ? PUGL_SUCCESS
           : PUGL_UNKNOWN_ERROR;
}

// NOLINTEND(misc-misplaced-const)
