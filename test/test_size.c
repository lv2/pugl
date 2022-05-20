// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

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
  PuglRect        configuredFrame;
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
    test->configuredFrame.x      = event->configure.x;
    test->configuredFrame.y      = event->configure.y;
    test->configuredFrame.width  = event->configure.width;
    test->configuredFrame.height = event->configure.height;
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
  static const PuglSpan minSize     = 256;
  static const PuglSpan defaultSize = 512;
  static const PuglSpan maxSize     = 1024;

  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   START,
                   {0, 0, 0u, 0u}};

  // Set up view with size bounds and an aspect ratio
  test.view = puglNewView(test.world);
  puglSetClassName(test.world, "PuglTest");
  puglSetWindowTitle(test.view, "Pugl Size Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetViewHint(test.view, PUGL_RESIZABLE, PUGL_TRUE);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, defaultSize, defaultSize);
  puglSetSizeHint(test.view, PUGL_MIN_SIZE, minSize, minSize);
  puglSetSizeHint(test.view, PUGL_MAX_SIZE, maxSize, maxSize);
  puglSetSizeHint(test.view, PUGL_MIN_ASPECT, 1, 1);
  puglSetSizeHint(test.view, PUGL_MAX_ASPECT, 1, 1);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view));
  while (test.state < MAPPED) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Check that the frame matches the last configure event
  const PuglRect frame = puglGetFrame(test.view);
  assert(frame.x == test.configuredFrame.x);
  assert(frame.y == test.configuredFrame.y);
  assert(frame.width == test.configuredFrame.width);
  assert(frame.height == test.configuredFrame.height);

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
