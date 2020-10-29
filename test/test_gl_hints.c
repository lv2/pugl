/*
  Copyright 2020 David Robillard <d@drobilla.net>

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

/*
  Tests that all hints are set to real values after a view is realized.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <assert.h>

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	(void)view;
	(void)event;

	return PUGL_SUCCESS;
}

int
main(void)
{
	PuglWorld* const world = puglNewWorld(PUGL_PROGRAM, 0);
	PuglView* const  view  = puglNewView(world);

	// Set up view
	puglSetClassName(world, "Pugl Test");
	puglSetBackend(view, puglGlBackend());
	puglSetEventFunc(view, onEvent);
	puglSetDefaultSize(view, 512, 512);

	// Set all hints that support it to PUGL_DONT_CARE
	assert(!puglSetViewHint(view, PUGL_RED_BITS, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_GREEN_BITS, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_BLUE_BITS, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_ALPHA_BITS, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_DEPTH_BITS, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_STENCIL_BITS, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_SAMPLES, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_DOUBLE_BUFFER, PUGL_DONT_CARE));
	assert(!puglSetViewHint(view, PUGL_REFRESH_RATE, PUGL_DONT_CARE));

	// Realize view and print all hints for debugging convenience
	assert(!puglRealize(view));
	printViewHints(view);

	// Check that no hints are set to PUGL_DONT_CARE
	assert(puglGetViewHint(view, PUGL_USE_COMPAT_PROFILE) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_USE_DEBUG_CONTEXT) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_CONTEXT_VERSION_MINOR) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_RED_BITS) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_GREEN_BITS) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_BLUE_BITS) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_ALPHA_BITS) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_DEPTH_BITS) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_STENCIL_BITS) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_SAMPLES) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_DOUBLE_BUFFER) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_SWAP_INTERVAL) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_RESIZABLE) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_IGNORE_KEY_REPEAT) != PUGL_DONT_CARE);
	assert(puglGetViewHint(view, PUGL_REFRESH_RATE) != PUGL_DONT_CARE);

	// Tear down
	puglFreeView(view);
	puglFreeWorld(world);

	return 0;
}
