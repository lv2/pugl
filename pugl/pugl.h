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

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

typedef struct PuglWindowImpl PuglWindow;

/**
   A native window handle.

   For X11, this is a Window.
*/
typedef intptr_t PuglNativeWindow;

typedef enum {
	PUGL_SUCCESS = 0
} PuglStatus;

/**
   Handle for opaque user data.
*/
typedef void* PuglHandle;

typedef void (*PuglCloseFunc)(PuglWindow* handle);
typedef void (*PuglDisplayFunc)(PuglWindow* handle);
typedef void (*PuglKeyboardFunc)(PuglWindow* handle, bool press, uint32_t key);
typedef void (*PuglMotionFunc)(PuglWindow* handle, int x, int y);
typedef void (*PuglMouseFunc)(PuglWindow* handle,
                              int button, bool down,
                              int x, int y);
typedef void (*PuglReshapeFunc)(PuglWindow* handle, int width, int height);
typedef void (*PuglScrollFunc)(PuglWindow* handle, float dx, float dy);

/**
   Create a new GL window.
   @param parent Parent window, or 0 for top level.
   @param title Window title, or NULL.
   @param width Window width in pixels.
   @param height Window height in pixels.
   @param resizable Whether window should be user resizable.
*/
PuglWindow*
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
void
puglSetHandle(PuglWindow* window, PuglHandle handle);

/**
   Get the handle to be passed to all callbacks.
*/
PuglHandle
puglGetHandle(PuglWindow* window);

/**
   Set the function to call when the window is closed.
*/
void
puglSetCloseFunc(PuglWindow* window, PuglCloseFunc closeFunc);

/**
   Set the display function which should draw the UI using GL.
*/
void
puglSetDisplayFunc(PuglWindow* window, PuglDisplayFunc displayFunc);

/**
   Set the function to call on keyboard events.
*/
void
puglSetKeyboardFunc(PuglWindow* window, PuglKeyboardFunc keyboardFunc);

/**
   Set the function to call on mouse motion.
*/
void
puglSetMotionFunc(PuglWindow* window, PuglMotionFunc motionFunc);

/**
   Set the function to call on mouse button events.
*/
void
puglSetMouseFunc(PuglWindow* window, PuglMouseFunc mouseFunc);

/**
   Set the function to call on scroll events.
*/
void
puglSetScrollFunc(PuglWindow* window, PuglScrollFunc scrollFunc);

/**
   Set the function to call when the window size changes.
*/
void
puglSetReshapeFunc(PuglWindow* window, PuglReshapeFunc reshapeFunc);

/**
   Return the native window handle.
*/
PuglNativeWindow
puglGetNativeWindow(PuglWindow* win);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.
*/
PuglStatus
puglProcessEvents(PuglWindow* win);

/**
   Request a redisplay on the next call to puglProcessEvents().
*/
void
puglPostRedisplay(PuglWindow* win);

/**
   Destroy a GL window.
*/
void
puglDestroy(PuglWindow* win);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_H_INCLUDED */
