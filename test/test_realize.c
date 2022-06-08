// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that realize sends a create event, and can safely be called twice.

  Without handling this case, an application that accidentally calls realize
  twice could end up in a very confusing situation where multiple windows have
  been allocated (and ultimately leaked) for a view.
*/

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
  puglSetWindowTitle(test.view, "Pugl Realize Test");
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  assert(puglRealize(test.view) == PUGL_BAD_BACKEND);

  puglSetBackend(test.view, puglStubBackend());
  assert(puglRealize(test.view) == PUGL_BAD_CONFIGURATION);

  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 512, 512);

  // Create initially invisible window
  assert(!puglRealize(test.view));
  assert(!puglGetVisible(test.view));
  while (test.state < CREATED) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Check that calling realize() again is okay
  assert(puglRealize(test.view) == PUGL_FAILURE);

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
