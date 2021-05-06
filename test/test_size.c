/*
  Copyright 2021 David Robillard <d@drobilla.net>

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

// Tests basic view setup

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  START,
  CREATED,
  CONFIGURED,
  MAPPED,
  DESTROYED,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  State           state;
  PuglRect        configuredRect;
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
    test->configuredRect.x      = event->configure.x;
    test->configuredRect.y      = event->configure.y;
    test->configuredRect.width  = event->configure.width;
    test->configuredRect.height = event->configure.height;
    break;
  case PUGL_MAP:
    test->state = MAPPED;
    break;
  case PUGL_DESTROY:
    test->state = DESTROYED;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  static const int minSize     = 256;
  static const int defaultSize = 512;
  static const int maxSize     = 1024;

  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   START,
                   {0.0, 0.0, 0.0, 0.0}};

  // Set up view with size bounds and an aspect ratio
  test.view = puglNewView(test.world);
  puglSetClassName(test.world, "Pugl Test");
  puglSetWindowTitle(test.view, "Pugl Size Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetViewHint(test.view, PUGL_RESIZABLE, PUGL_TRUE);
  puglSetMinSize(test.view, minSize, minSize);
  puglSetDefaultSize(test.view, defaultSize, defaultSize);
  puglSetMaxSize(test.view, maxSize, maxSize);
  puglSetAspectRatio(test.view, 1, 1, 1, 1);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view));
  while (test.state < MAPPED) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Check that the frame matches the last configure event
  const PuglRect frame = puglGetFrame(test.view);
  assert(frame.x == test.configuredRect.x);
  assert(frame.y == test.configuredRect.y);
  assert(frame.width == test.configuredRect.width);
  assert(frame.height == test.configuredRect.height);

#if defined(_WIN32) || defined(__APPLE__)
  /* Some window managers on Linux (particularly tiling ones) just disregard
     these hints entirely, so we only check that the size is in bounds on MacOS
     and Windows where this is more or less universally supported. */

  assert(frame.width >= minSize);
  assert(frame.height >= minSize);
  assert(frame.width <= maxSize);
  assert(frame.height <= maxSize);
#endif

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
