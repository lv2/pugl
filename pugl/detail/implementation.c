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
   @file implementation.c Platform-independent implementation.
*/

#include "pugl/detail/implementation.h"
#include "pugl/pugl.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void
puglSetDefaultHints(PuglHints hints)
{
	hints[PUGL_USE_COMPAT_PROFILE]    = PUGL_TRUE;
	hints[PUGL_CONTEXT_VERSION_MAJOR] = 2;
	hints[PUGL_CONTEXT_VERSION_MINOR] = 0;
	hints[PUGL_RED_BITS]              = 4;
	hints[PUGL_GREEN_BITS]            = 4;
	hints[PUGL_BLUE_BITS]             = 4;
	hints[PUGL_ALPHA_BITS]            = 4;
	hints[PUGL_DEPTH_BITS]            = 24;
	hints[PUGL_STENCIL_BITS]          = 8;
	hints[PUGL_SAMPLES]               = 0;
	hints[PUGL_DOUBLE_BUFFER]         = PUGL_FALSE;
	hints[PUGL_RESIZABLE]             = PUGL_FALSE;
	hints[PUGL_IGNORE_KEY_REPEAT]     = PUGL_FALSE;
}

PuglView*
puglInit(int* PUGL_UNUSED(pargc), char** PUGL_UNUSED(argv))
{
	return puglNewView(puglNewWorld());
}

void
puglDestroy(PuglView* const view)
{
	PuglWorld* const world = view->world;

	puglFreeView(view);
	puglFreeWorld(world);
}

PuglWorld*
puglNewWorld(void)
{
	PuglWorld* world = (PuglWorld*)calloc(1, sizeof(PuglWorld));
	if (!world || !(world->impl = puglInitWorldInternals())) {
		free(world);
		return NULL;
	}

	world->startTime = puglGetTime(world);

	return world;
}

void
puglFreeWorld(PuglWorld* const world)
{
	puglFreeWorldInternals(world);
	free(world->views);
	free(world);
}

PuglView*
puglNewView(PuglWorld* const world)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
	if (!view || !(view->impl = puglInitViewInternals())) {
		free(view);
		return NULL;
	}

	view->world      = world;
	view->width      = 640;
	view->height     = 480;

	puglSetDefaultHints(view->hints);

	// Add to world view list
	++world->numViews;
	world->views = (PuglView**)realloc(world->views,
	                                   world->numViews * sizeof(PuglView*));
	world->views[world->numViews - 1] = view;

	return view;
}

void
puglFreeView(PuglView* view)
{
	// Remove from world view list
	PuglWorld* world = view->world;
	for (size_t i = 0; i < world->numViews; ++i) {
		if (world->views[i] == view) {
			if (i == world->numViews - 1) {
				world->views[i] = NULL;
			} else {
				memmove(world->views + i, world->views + i + 1,
				        sizeof(PuglView*) * (world->numViews - i - 1));
				world->views[world->numViews - 1] = NULL;
			}
			--world->numViews;
		}
	}

	puglFreeViewInternals(view);
	free(view->windowClass);
	free(view);
}

void
puglInitWindowHint(PuglView* view, PuglWindowHint hint, int value)
{
	if (hint < PUGL_NUM_WINDOW_HINTS) {
		view->hints[hint] = value;
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
	view->hints[PUGL_RESIZABLE] = resizable;
}

void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
	view->transient_parent = parent;
}

int
puglInitBackend(PuglView* view, const PuglBackend* backend)
{
	view->backend = backend;
	return 0;
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

void*
puglGetContext(PuglView* view)
{
	return view->backend->getContext(view);
}

void
puglEnterContext(PuglView* view, bool drawing)
{
	view->backend->enter(view, drawing);
}

void
puglLeaveContext(PuglView* view, bool drawing)
{
	view->backend->leave(view, drawing);
}

void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	puglInitWindowHint(view, PUGL_IGNORE_KEY_REPEAT, ignore);
}

void
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc)
{
	view->eventFunc = eventFunc;
}

/** Return the code point for buf, or the replacement character on error. */
uint32_t
puglDecodeUTF8(const uint8_t* buf)
{
#define FAIL_IF(cond) do { if (cond) return 0xFFFD; } while (0)

	// http://en.wikipedia.org/wiki/UTF-8

	if (buf[0] < 0x80) {
		return buf[0];
	} else if (buf[0] < 0xC2) {
		return 0xFFFD;
	} else if (buf[0] < 0xE0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		return (buf[0] << 6) + buf[1] - 0x3080u;
	} else if (buf[0] < 0xF0) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xE0 && buf[1] < 0xA0);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		return (buf[0] << 12) + (buf[1] << 6) + buf[2] - 0xE2080u;
	} else if (buf[0] < 0xF5) {
		FAIL_IF((buf[1] & 0xC0) != 0x80);
		FAIL_IF(buf[0] == 0xF0 && buf[1] < 0x90);
		FAIL_IF(buf[0] == 0xF4 && buf[1] >= 0x90);
		FAIL_IF((buf[2] & 0xC0) != 0x80);
		FAIL_IF((buf[3] & 0xC0) != 0x80);
		return ((buf[0] << 18) +
		        (buf[1] << 12) +
		        (buf[2] << 6) +
		        buf[3] - 0x3C82080u);
	}
	return 0xFFFD;
}

void
puglDispatchEvent(PuglView* view, const PuglEvent* event)
{
	switch (event->type) {
	case PUGL_NOTHING:
		break;
	case PUGL_CONFIGURE:
		view->width  = (int)event->configure.width;
		view->height = (int)event->configure.height;
		puglEnterContext(view, false);
		view->eventFunc(view, event);
		puglLeaveContext(view, false);
		break;
	case PUGL_EXPOSE:
		if (event->expose.count == 0) {
			puglEnterContext(view, true);
			view->eventFunc(view, event);
			puglLeaveContext(view, true);
		}
		break;
	default:
		view->eventFunc(view, event);
	}
}
