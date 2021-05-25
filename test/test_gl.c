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

// Tests basic OpenGL support

#undef NDEBUG

#include "test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <assert.h>
#include <stdbool.h>

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  bool            exposed;
} PuglTest;

static void
onConfigure(PuglView* const view, const PuglConfigureEvent* const event)
{
  (void)view;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glViewport(0, 0, (int)event->width, (int)event->height);
}

static void
onExpose(PuglView* const view, const PuglExposeEvent* const event)
{
  (void)view;
  (void)event;

  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  switch (event->type) {
  case PUGL_CONFIGURE:
    onConfigure(view, &event->configure);
    break;

  case PUGL_EXPOSE:
    onExpose(view, &event->expose);
    test->exposed = true;
    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglWorld* const      world = puglNewWorld(PUGL_PROGRAM, 0);
  PuglView* const       view  = puglNewView(world);
  const PuglTestOptions opts  = puglParseTestOptions(&argc, &argv);
  PuglTest              test  = {world, view, opts, false};

  // Set up and show view
  puglSetClassName(test.world, "Pugl OpenGL Test");
  puglSetHandle(test.view, &test);
  puglSetBackend(test.view, puglGlBackend());
  puglSetEventFunc(test.view, onEvent);
  puglSetDefaultSize(test.view, 512, 512);
  puglShow(test.view);

  // Enter OpenGL context as if setting things up
  puglEnterContext(test.view);

  const PuglGlFunc createProgram = puglGetProcAddress("glCreateProgram");
  assert(createProgram);

  puglLeaveContext(test.view);

  // Drive event loop until the view gets exposed
  while (!test.exposed) {
    puglUpdate(test.world, -1.0);
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
