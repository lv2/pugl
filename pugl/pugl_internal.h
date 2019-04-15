/*
  Copyright 2012-2016 David Robillard <http://drobilla.net>

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
   @file pugl_internal.h Private platform-independent definitions.

   Note this file contains function definitions, so it must be compiled into
   the final binary exactly once.  Each platform specific implementation file
   including it once should achieve this.

   If you are copying the pugl code into your source tree, the following
   symbols can be defined to tweak pugl behaviour:

   PUGL_HAVE_CAIRO: Include Cairo support code.
   PUGL_HAVE_GL:    Include OpenGL support code.
*/

#include "pugl/pugl.h"
#include "pugl/pugl_internal_types.h"

#include <stdlib.h>
#include <string.h>

PuglInternals* puglInitInternals(void);

static PuglHints
puglDefaultHints()
{
	static const PuglHints hints = {
		2, 0, 4, 4, 4, 4, 24, 8, 0, true, true, false
	};
	return hints;
}

PuglView*
puglInit(int* pargc, char** argv)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
	if (!view) {
		return NULL;
	}

	view->hints = puglDefaultHints();

	PuglInternals* impl = puglInitInternals();
	if (!impl) {
		return NULL;
	}

	view->ctx_type = PUGL_GL;
	view->impl     = impl;
	view->width    = 640;
	view->height   = 480;

	return view;
}

void
puglInitWindowHint(PuglView* view, PuglWindowHint hint, int value)
{
	switch (hint) {
	case PUGL_USE_COMPAT_PROFILE:
		view->hints.use_compat_profile = value;
		break;
	case PUGL_CONTEXT_VERSION_MAJOR:
		view->hints.context_version_major = value;
		break;
	case PUGL_CONTEXT_VERSION_MINOR:
		view->hints.context_version_minor = value;
		break;
	case PUGL_RED_BITS:
		view->hints.red_bits = value;
		break;
	case PUGL_GREEN_BITS:
		view->hints.green_bits = value;
		break;
	case PUGL_BLUE_BITS:
		view->hints.blue_bits = value;
		break;
	case PUGL_ALPHA_BITS:
		view->hints.alpha_bits = value;
		break;
	case PUGL_DEPTH_BITS:
		view->hints.depth_bits = value;
		break;
	case PUGL_STENCIL_BITS:
		view->hints.stencil_bits = value;
		break;
	case PUGL_SAMPLES:
		view->hints.samples = value;
		break;
	case PUGL_DOUBLE_BUFFER:
		view->hints.double_buffer = value;
		break;
	case PUGL_RESIZABLE:
		view->hints.resizable = value;
		break;
	}
}

void
puglInitWindowSize(PuglView* view, int width, int height)
{
	view->width  = width;
	view->height = height;
}

void
puglInitWindowMinSize(PuglView* view, int width, int height)
{
	view->min_width  = width;
	view->min_height = height;
}

void
puglInitWindowAspectRatio(PuglView* view,
                          int       min_x,
                          int       min_y,
                          int       max_x,
                          int       max_y)
{
	view->min_aspect_x = min_x;
	view->min_aspect_y = min_y;
	view->max_aspect_x = max_x;
	view->max_aspect_y = max_y;
}

void
puglInitWindowClass(PuglView* view, const char* name)
{
	const size_t len = strlen(name);

	free(view->windowClass);
	view->windowClass = (char*)calloc(1, len + 1);
	memcpy(view->windowClass, name, len);
}

void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
	view->parent = parent;
}

void
puglInitResizable(PuglView* view, bool resizable)
{
	view->hints.resizable = resizable;
}

void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
	view->transient_parent = parent;
}

void
puglInitContextType(PuglView* view, PuglContextType type)
{
	view->ctx_type = type;
}

void
puglSetHandle(PuglView* view, PuglHandle handle)
{
	view->handle = handle;
}

PuglHandle
puglGetHandle(PuglView* view)
{
	return view->handle;
}

bool
puglGetVisible(PuglView* view)
{
	return view->visible;
}

void
puglGetSize(PuglView* view, int* width, int* height)
{
	*width  = view->width;
	*height = view->height;
}

void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	view->ignoreKeyRepeat = ignore;
}

void
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc)
{
	view->eventFunc = eventFunc;
}

/** Return the code point for buf, or the replacement character on error. */
static uint32_t
puglDecodeUTF8(const uint8_t* buf)
{
#define FAIL_IF(cond) { if (cond) return 0xFFFD; }

	// http://en.wikipedia.org/wiki/UTF-8

	if (buf[0] < 0x80) {
		return buf[0];
	} else if (buf[0] < 0xC2) {
		return 0xFFFD;
	} else if (buf[0] < 0xE0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		return (buf[0] << 6) + buf[1] - 0x3080;
	} else if (buf[0] < 0xF0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xE0 && buf[1] < 0xA0);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		return (buf[0] << 12) + (buf[1] << 6) + buf[2] - 0xE2080;
	} else if (buf[0] < 0xF5) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xF0 && buf[1] < 0x90);
		FAIL_IF(buf[0] == 0xF4 && buf[1] >= 0x90);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		FAIL_IF((buf[3] & 0xC0) != 0x80);
		return ((buf[0] << 18) +
		        (buf[1] << 12) +
		        (buf[2] << 6) +
		        buf[3] - 0x3C82080);
	}
	return 0xFFFD;
}

static void
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
	switch (event->type) {
	case PUGL_NOTHING:
		break;
	case PUGL_CONFIGURE:
		view->width  = (int)event->configure.width;
		view->height = (int)event->configure.height;
		puglEnterContext(view);
		view->eventFunc(view, event);
		puglLeaveContext(view, false);
		break;
	case PUGL_EXPOSE:
		if (event->expose.count == 0) {
			puglEnterContext(view);
			view->eventFunc(view, event);
			puglLeaveContext(view, true);
		}
		break;
	default:
		view->eventFunc(view, event);
	}
}
