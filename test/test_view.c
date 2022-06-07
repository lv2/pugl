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
  MAPPED,
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
  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   START};

  // Set up view
  test.view = puglNewView(test.world);
  puglSetClassName(test.world, "PuglTest");
  puglSetWindowTitle(test.view, "Pugl View Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 512, 512);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view));
  while (test.state < MAPPED) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Check that puglGetNativeView() returns something
  assert(puglGetNativeView(test.view));

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
