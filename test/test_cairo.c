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

// Tests that creating a view with a Cairo backend works

#undef NDEBUG

#include "test_utils.h"

#include "pugl/cairo.h"
#include "pugl/pugl.h"

#include <cairo.h>

#include <assert.h>
#include <stdbool.h>

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  bool            exposed;
} PuglTest;

static void
onExpose(PuglView* const view, const PuglExposeEvent* const event)
{
  cairo_t* const cr = (cairo_t*)puglGetContext(view);

  assert(cr);

  cairo_rectangle(cr, event->x, event->y, event->width, event->height);
  cairo_set_source_rgb(cr, 0, 1, 0);
  cairo_fill(cr);
}

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  if (event->type == PUGL_EXPOSE) {
    onExpose(view, &event->expose);
    test->exposed = true;
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
  puglSetClassName(test.world, "Pugl Test");
  puglSetWindowTitle(test.view, "Pugl Cairo Test");
  puglSetHandle(test.view, &test);
  puglSetBackend(test.view, puglCairoBackend());
  puglSetEventFunc(test.view, onEvent);
  puglSetDefaultSize(test.view, 512, 512);
  puglShow(test.view);

  // Drive event loop until the view gets exposed
  while (!test.exposed) {
    puglUpdate(test.world, -1.0);
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
