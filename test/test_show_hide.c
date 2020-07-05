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
  Tests the basic sanity of view/window create, configure, map, expose, unmap,
  and destroy events.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/pugl_stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
	START,
	CREATED,
	CONFIGURED,
	MAPPED,
	EXPOSED,
	UNMAPPED,
	DESTROYED,
} State;

typedef struct {
	PuglWorld*      world;
	PuglView*       view;
	PuglTestOptions opts;
	State           state;
} PuglTest;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTest* test = (PuglTest*)puglGetHandle(view);

	if (test->opts.verbose) {
		printEvent(event, "Event: ", true);
	}

	switch (event->type) {
	case PUGL_CREATE:
		assert(test->state == START);
		test->state = CREATED;
		break;
	case PUGL_CONFIGURE:
		if (test->state == CREATED) {
			test->state = CONFIGURED;
		}
		break;
	case PUGL_MAP:
		assert(test->state == CONFIGURED || test->state == UNMAPPED);
		test->state = MAPPED;
		break;
	case PUGL_EXPOSE:
		assert(test->state == MAPPED || test->state == EXPOSED);
		test->state = EXPOSED;
		break;
	case PUGL_UNMAP:
		assert(test->state == EXPOSED);
		test->state = UNMAPPED;
		break;
	case PUGL_DESTROY:
		assert(test->state == UNMAPPED);
		test->state = DESTROYED;
		break;
	default:
		break;
	}

	return PUGL_SUCCESS;
}

static void
tick(PuglWorld* world)
{
#ifdef __APPLE__
	// FIXME: Expose events are not events on MacOS, so we can't block
	// indefinitely here since it will block forever
	assert(!puglUpdate(world, 1 / 30.0));
#else
	assert(!puglUpdate(world, -1));
#endif
}

int
main(int argc, char** argv)
{
	PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
	                 NULL,
	                 puglParseTestOptions(&argc, &argv),
	                 START};

	// Set up view
	test.view = puglNewView(test.world);
	puglSetClassName(test.world, "Pugl Test");
	puglSetBackend(test.view, puglStubBackend());
	puglSetHandle(test.view, &test);
	puglSetEventFunc(test.view, onEvent);
	puglSetDefaultSize(test.view, 512, 512);

	// Create initially invisible window
	assert(!puglRealize(test.view));
	assert(!puglGetVisible(test.view));
	while (test.state < CREATED) {
		tick(test.world);
	}

	// Show and hide window a couple of times
	for (unsigned i = 0u; i < 2u; ++i) {
		assert(!puglShowWindow(test.view));
		while (test.state != EXPOSED) {
			tick(test.world);
		}

		assert(puglGetVisible(test.view));
		assert(!puglHideWindow(test.view));
		while (test.state != UNMAPPED) {
			tick(test.world);
		}
	}

	// Tear down
	assert(!puglGetVisible(test.view));
	puglFreeView(test.view);
	assert(test.state == DESTROYED);
	puglFreeWorld(test.world);

	return 0;
}
