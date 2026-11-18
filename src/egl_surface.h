// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_EGL_H
#define PUGL_SRC_EGL_H

#include <pugl/pugl.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <stdbool.h>

typedef struct {
  EGLDisplay display;
  EGLConfig  config;
  EGLSurface surface;
  EGLContext context;
} PuglEglSurface;

typedef int (*RankConfigFunc)(EGLDisplay display, EGLConfig config);

PuglStatus
puglEglAttach(PuglEglSurface*      surface,
              EGLNativeDisplayType nativeDisplay,
              EGLNativeWindowType  nativeWindow,
              RankConfigFunc       rankConfig,
              int                  hints[PUGL_NUM_VIEW_HINTS]);

void
puglEglDetach(PuglEglSurface* surface);

PuglStatus
puglEglCreate(PuglEglSurface* surface, const int hints[PUGL_NUM_VIEW_HINTS]);

void
puglEglDestroy(PuglEglSurface* surface);

PuglStatus
puglEglEnter(PuglEglSurface* surface, bool expose);

PuglStatus
puglEglLeave(PuglEglSurface* surface, bool expose);

#endif // PUGL_SRC_EGL_H
