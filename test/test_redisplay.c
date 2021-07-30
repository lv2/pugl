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
  Tests that redisplays posted in the event handler are dispatched at the end
  of the same event loop iteration.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __APPLE__
static const double timeout = 1 / 60.0;
#else
static const double timeout = -1.0;
#endif

typedef enum {
  START,
  EXPOSED,
  SHOULD_REDISPLAY,
  POSTED_REDISPLAY,
  REDISPLAYED,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  State           state;
} PuglTest;

static const PuglRect  redisplayRect   = {2, 4, 8, 16};
static const uintptr_t postRedisplayId = 42;

static bool
rectContains(PuglRect outer, PuglRect inner)
{
  return outer.x <= inner.x && outer.y <= inner.y &&
         inner.x + inner.width <= outer.x + outer.width &&
         inner.y + inner.height <= outer.y + outer.height;
}

static PuglRect
getExposeRect(PuglExposeEvent event)
{
  PuglRect result = {
    .x = event.x, .y = event.y, .width = event.width, .height = event.height};
  return result;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTest* test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  switch (event->type) {
  case PUGL_UPDATE:
    if (test->state == SHOULD_REDISPLAY) {
      puglPostRedisplayRect(view, redisplayRect);
      test->state = POSTED_REDISPLAY;
    }
    break;

  case PUGL_EXPOSE:
    if (test->state == START) {
      test->state = EXPOSED;
    } else if (test->state == POSTED_REDISPLAY &&
               rectContains(getExposeRect(event->expose), redisplayRect)) {
      test->state = REDISPLAYED;
    }
    break;

  case PUGL_CLIENT:
    if (event->client.data1 == postRedisplayId) {
      test->state = SHOULD_REDISPLAY;
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
  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   START};

  // Set up view
  test.view = puglNewView(test.world);
  puglSetClassName(test.world, "Pugl Test");
  puglSetWindowTitle(test.view, "Pugl Redisplay Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetDefaultSize(test.view, 512, 512);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view));
  while (test.state != EXPOSED) {
    assert(!puglUpdate(test.world, timeout));
  }

  // Send a custom event to trigger a redisplay in the event loop
  PuglEvent client_event    = {{PUGL_CLIENT, 0}};
  client_event.client.data1 = postRedisplayId;
  client_event.client.data2 = 0;
  assert(!puglSendEvent(test.view, &client_event));

  // Loop until an expose happens in the same iteration as the redisplay
  test.state = SHOULD_REDISPLAY;
  while (test.state != REDISPLAYED) {
    assert(!puglUpdate(test.world, timeout));
    assert(test.state != POSTED_REDISPLAY);
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
