/*
  Copyright 2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl.h API for Pugl, a portable micro-framework for GL UIs.
*/

#ifndef PUGL_H_INCLUDED
#define PUGL_H_INCLUDED

#include <stdint.h>

/*
  This API is pure portable C and contains no platform specific elements, or
  even a GL dependency.  However, unfortunately GL includes vary across
  platforms so they are included here to allow for pure portable programs.
*/
#ifdef __APPLE__
#    include "OpenGL/gl.h"
#    include "OpenGL/glu.h"
#else
#    ifdef _WIN32
#        include <windows.h>  /* Broken Windows GL headers require this */
#    endif
#    include "GL/gl.h"
#    include "GL/glu.h"
#endif

#ifdef PUGL_SHARED
#    ifdef _WIN32
#        define PUGL_LIB_IMPORT __declspec(dllimport)
#        define PUGL_LIB_EXPORT __declspec(dllexport)
#    else
#        define PUGL_LIB_IMPORT __attribute__((visibility("default")))
#        define PUGL_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef PUGL_INTERNAL
#        define PUGL_API PUGL_LIB_EXPORT
#    else
#        define PUGL_API PUGL_LIB_IMPORT
#    endif
#else
#    define PUGL_API
#endif

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

typedef struct PuglWindowImpl PuglWindow;

/**
   A native window handle.

   On X11, this is a Window.
   On OSX, this is an NSView*.
   On Windows, this is a HWND.
*/
typedef intptr_t PuglNativeWindow;

typedef enum {
	PUGL_SUCCESS = 0
} PuglStatus;

/**
   Handle for opaque user data.
*/
typedef void* PuglHandle;

typedef void (*PuglCloseFunc)(PuglWindow* win);
typedef void (*PuglDisplayFunc)(PuglWindow* win);
typedef void (*PuglKeyboardFunc)(PuglWindow* win, bool press, uint32_t key);
typedef void (*PuglMotionFunc)(PuglWindow* win, int x, int y);
typedef void (*PuglMouseFunc)(PuglWindow* win,
                              int button, bool down, int x, int y);
typedef void (*PuglReshapeFunc)(PuglWindow* win, int width, int height);
typedef void (*PuglScrollFunc)(PuglWindow* win, float dx, float dy);

/**
   Create a new GL window.
   @param parent Parent window, or 0 for top level.
   @param title Window title, or NULL.
   @param width Window width in pixels.
   @param height Window height in pixels.
   @param resizable Whether window should be user resizable.
*/
PUGL_API PuglWindow*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable);

/**
   Set the handle to be passed to all callbacks.

   This is generally a pointer to a struct which contains all necessary state.
   Everything needed in callbacks should be here, not in static variables.

   Note the lack of this facility makes GLUT unsuitable for plugins or
   non-trivial programs; this mistake is largely why Pugl exists.
*/
PUGL_API void
puglSetHandle(PuglWindow* window, PuglHandle handle);

/**
   Get the handle to be passed to all callbacks.
*/
PUGL_API PuglHandle
puglGetHandle(PuglWindow* window);

/**
   Set the function to call when the window is closed.
*/
PUGL_API void
puglSetCloseFunc(PuglWindow* window, PuglCloseFunc closeFunc);

/**
   Set the display function which should draw the UI using GL.
*/
PUGL_API void
puglSetDisplayFunc(PuglWindow* window, PuglDisplayFunc displayFunc);

/**
   Set the function to call on keyboard events.
*/
PUGL_API void
puglSetKeyboardFunc(PuglWindow* window, PuglKeyboardFunc keyboardFunc);

/**
   Set the function to call on mouse motion.
*/
PUGL_API void
puglSetMotionFunc(PuglWindow* window, PuglMotionFunc motionFunc);

/**
   Set the function to call on mouse button events.
*/
PUGL_API void
puglSetMouseFunc(PuglWindow* window, PuglMouseFunc mouseFunc);

/**
   Set the function to call on scroll events.
*/
PUGL_API void
puglSetScrollFunc(PuglWindow* window, PuglScrollFunc scrollFunc);

/**
   Set the function to call when the window size changes.
*/
PUGL_API void
puglSetReshapeFunc(PuglWindow* window, PuglReshapeFunc reshapeFunc);

/**
   Return the native window handle.
*/
PUGL_API PuglNativeWindow
puglGetNativeWindow(PuglWindow* win);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.
*/
PUGL_API PuglStatus
puglProcessEvents(PuglWindow* win);

/**
   Request a redisplay on the next call to puglProcessEvents().
*/
PUGL_API void
puglPostRedisplay(PuglWindow* win);

/**
   Destroy a GL window.
*/
PUGL_API void
puglDestroy(PuglWindow* win);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_H_INCLUDED */
