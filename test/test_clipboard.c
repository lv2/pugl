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
  Tests basic clipboard copy/paste functionality between two views.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
	PuglWorld*      world;
	PuglView*       views[2];
	PuglTestOptions opts;
	bool            exposed;
} PuglTest;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTest* test = (PuglTest*)puglGetHandle(view);

	if (event->type == PUGL_EXPOSE) {
		test->exposed = true;
	}

	if (test->opts.verbose) {
		printEvent(event, "Event: ", true);
	}

	return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
	PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
	                 {NULL, NULL},
	                 puglParseTestOptions(&argc, &argv),
	                 false};

	puglSetClassName(test.world, "Pugl Test");

	// Set up views
	for (unsigned i = 0u; i < 2; ++i) {
		test.views[i] = puglNewView(test.world);
		puglSetBackend(test.views[i], puglStubBackend());
		puglSetHandle(test.views[i], &test);
		puglSetEventFunc(test.views[i], onEvent);
		puglSetDefaultSize(test.views[i], 512, 512);

		assert(!puglShow(test.views[i]));
	}

	// Update until view is exposed
	while (!test.exposed) {
		assert(!puglUpdate(test.world, 0.0));
	}

	// Set clipboard text via the first view
	puglSetClipboard(test.views[0], NULL, "Text", 5);

	// Get clipboard contents via the second view
	const char*       type     = NULL;
	size_t            len      = 0;
	const void* const contents = puglGetClipboard(test.views[1], &type, &len);

	// Check that the data made it over
	assert(!strcmp(type, "text/plain"));
	assert(len == 5);
	assert(contents);
	assert(!strcmp((const char*)contents, "Text"));

	// Try setting the clipboard to an unsupported type
	assert(puglSetClipboard(test.views[0], "text/csv", "a,b,c", 6));

	// Tear down
	puglFreeView(test.views[0]);
	puglFreeView(test.views[1]);
	puglFreeWorld(test.world);

	return 0;
}
