/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

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
   @file pugl_internal_types.h Private platform-independent type definitions.
*/

#ifndef PUGL_INTERNAL_TYPES_H
#define PUGL_INTERNAL_TYPES_H

#include "pugl/pugl.h"

#include <stdbool.h>
#include <stdint.h>

// Unused parameter macro to suppresses warnings and make it impossible to use
#if defined(__cplusplus) || defined(_MSC_VER)
#   define PUGL_UNUSED(name)
#elif defined(__GNUC__)
#   define PUGL_UNUSED(name) name##_unused __attribute__((__unused__))
#else
#   define PUGL_UNUSED(name)
#endif

/** Platform-specific internals. */
typedef struct PuglInternalsImpl PuglInternals;

typedef struct {
	int  context_version_major;
	int  context_version_minor;
	int  red_bits;
	int  green_bits;
	int  blue_bits;
	int  alpha_bits;
	int  depth_bits;
	int  stencil_bits;
	int  samples;
	int  double_buffer;
	bool use_compat_profile;
	bool resizable;
} PuglHints;

/** Cross-platform view definition. */
struct PuglViewImpl {
	PuglInternals*   impl;
	PuglHandle       handle;
	PuglEventFunc    eventFunc;
	char*            windowClass;
	PuglNativeWindow parent;
	double           start_time;
	uintptr_t        transient_parent;
	PuglContextType  ctx_type;
	PuglHints        hints;
	int              width;
	int              height;
	int              min_width;
	int              min_height;
	int              min_aspect_x;
	int              min_aspect_y;
	int              max_aspect_x;
	int              max_aspect_y;
	bool             ignoreKeyRepeat;
	bool             visible;
};

/** Opaque surface used by draw context. */
typedef void PuglSurface;

/** Graphics backend interface. */
typedef struct {
	/** Get visual information from display and setup view as necessary. */
	int (*configure)(PuglView*);

	/** Create surface and drawing context. */
	int (*create)(PuglView*);

	/** Destroy surface and drawing context. */
	int (*destroy)(PuglView*);

	/** Enter drawing context, for drawing if parameter is true. */
	int (*enter)(PuglView*, bool);

	/** Leave drawing context, after drawing if parameter is true. */
	int (*leave)(PuglView*, bool);

	/** Resize drawing context to the given width and height. */
	int (*resize)(PuglView*, int, int);

	/** Return the puglGetContext() handle for the application, if any. */
	void* (*getContext)(PuglView*);
} PuglBackend;

#endif // PUGL_INTERNAL_TYPES_H
