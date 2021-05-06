/*
  Copyright 2020-2021 David Robillard <d@drobilla.net>

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

// Tests copy and paste from one view to another

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uintptr_t copierTimerId = 1u;
static const uintptr_t pasterTimerId = 2u;

typedef enum {
  START,
  EXPOSED,
  COPIED,
  FINISHED,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       copierView;
  PuglView*       pasterView;
  PuglTestOptions opts;
  State           state;
  bool            copierStarted;
  bool            pasterStarted;
} PuglTest;

static PuglStatus
onCopierEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Copier Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    if (!test->copierStarted) {
      // Start timer on first expose
      assert(!puglStartTimer(view, copierTimerId, 1 / 15.0));
      test->copierStarted = true;
    }
    break;

  case PUGL_TIMER:
    assert(event->timer.id == copierTimerId);

    if (test->state < COPIED) {
      puglSetClipboard(
        view, "text/plain", "Copied Text", strlen("Copied Text") + 1);
      test->state = COPIED;
    }

    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onPasterEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Paster Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    if (!test->pasterStarted) {
      // Start timer on first expose
      assert(!puglStartTimer(view, pasterTimerId, 1 / 60.0));
      test->pasterStarted = true;
    }
    break;

  case PUGL_TIMER:
    assert(event->timer.id == pasterTimerId);

    if (test->state == COPIED) {
      const char* type = NULL;
      size_t      len  = 0;
      const char* text = (const char*)puglGetClipboard(view, &type, &len);

      assert(!strcmp(type, "text/plain"));
      assert(!strcmp(text, "Copied Text"));

      test->state = FINISHED;
    }

    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTest app = {puglNewWorld(PUGL_PROGRAM, 0),
                  NULL,
                  NULL,
                  puglParseTestOptions(&argc, &argv),
                  START,
                  false,
                  false};

  // Set up copier view
  app.copierView = puglNewView(app.world);
  puglSetClassName(app.world, "Pugl Test Copier");
  puglSetBackend(app.copierView, puglStubBackend());
  puglSetHandle(app.copierView, &app);
  puglSetEventFunc(app.copierView, onCopierEvent);
  puglSetDefaultSize(app.copierView, 256, 256);

  // Set up paster view
  app.pasterView = puglNewView(app.world);
  puglSetClassName(app.world, "Pugl Test Paster");
  puglSetBackend(app.pasterView, puglStubBackend());
  puglSetHandle(app.pasterView, &app);
  puglSetEventFunc(app.pasterView, onPasterEvent);
  puglSetDefaultSize(app.pasterView, 256, 256);

  // Create and show both views
  assert(!puglShow(app.copierView));
  assert(!puglShow(app.pasterView));

  // Run until the test is finished
  while (app.state != FINISHED) {
    assert(!puglUpdate(app.world, 1 / 60.0));
  }

  puglFreeView(app.copierView);
  puglFreeView(app.pasterView);
  puglFreeWorld(app.world);

  return 0;
}
