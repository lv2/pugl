/*
  Copyright 2019 David Robillard <http://drobilla.net>

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
   @file pugl_stub.h Stub backend functions and accessor declaration.
*/

#ifndef PUGL_PUGL_STUB_H
#define PUGL_PUGL_STUB_H

#include "pugl/pugl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   Stub graphics backend.

   This backend just creates a simple native window without setting up any
   portable graphics API.
*/
PUGL_API
const PuglBackend*
puglStubBackend(void);

/**
   @name Stub backend functions

   Implementations of stub backend functions which do nothing and always return
   success.  These do not make for a usable backend on their own since the
   platform implementation would fail to create a window, but are useful for
   other backends to reuse since not all need non-trivial implementations of
   every backend function.

   @{
*/

static inline PuglStatus
puglStubConfigure(PuglView* view)
{
	(void)view;
	return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubCreate(PuglView* view)
{
	(void)view;
	return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubDestroy(PuglView* view)
{
	(void)view;
	return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubEnter(PuglView* view, bool drawing)
{
	(void)view;
	(void)drawing;
	return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubLeave(PuglView* view, bool drawing)
{
	(void)view;
	(void)drawing;
	return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubResize(PuglView* view, int width, int height)
{
	(void)view;
	(void)width;
	(void)height;
	return PUGL_SUCCESS;
}

static inline void*
puglStubGetContext(PuglView* view)
{
	(void)view;
	return NULL;
}

/**
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // PUGL_PUGL_STUB_H
