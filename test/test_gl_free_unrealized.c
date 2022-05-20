// Copyright 2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that deleting an unrealized view works properly.

  This was a crash bug with OpenGL backends.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
} PuglTest;

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTest test = {
    puglNewWorld(PUGL_PROGRAM, 0), NULL, puglParseTestOptions(&argc, &argv)};

  // Set up view
  test.view = puglNewView(test.world);
  puglSetClassName(test.world, "PuglTest");
  puglSetWindowTitle(test.view, "Pugl OpenGL Free Test");
  puglSetBackend(test.view, puglGlBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 512, 512);

  assert(!puglGetVisible(test.view));

  // Tear everything down without ever realizing the view
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
