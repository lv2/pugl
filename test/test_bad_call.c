// Copyright 2021-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests graceful handling of bad calls

#undef NDEBUG

#include <puglutil/test_utils.h>

#include <pugl/pugl.h>
#include <pugl/stub.h>

#include <assert.h>
#include <stdbool.h>

typedef struct {
  PuglTestOptions opts;
  PuglWorld*      world;
  PuglView*       view;
  bool            exposed;
} PuglTest;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  static const PuglEvent close_event  = {{PUGL_CLOSE, 0U}};
  const PuglEvent        expose_event = {{PUGL_EXPOSE, 0U}};

  PuglTest* test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  if (event->type == PUGL_UPDATE) {
    assert(puglUpdate(test->world, -1.0) == PUGL_BAD_CALL);
  }

  if (event->type == PUGL_EXPOSE) {
    /* The commented checks are for functions that don't bother to check
       because they obviously shouldn't be called when exposing */

    assert(puglSetPositionHint(view, PUGL_CURRENT_POSITION, 1, 1) ==
           PUGL_BAD_CALL);
    assert(puglSetSizeHint(view, PUGL_CURRENT_SIZE, 1U, 1U) == PUGL_BAD_CALL);
    // assert(puglSetParent(view, 0U) == PUGL_BAD_CALL);
    // assert(puglSetTransientParent(view, 0U) == PUGL_BAD_CALL);
    // assert(puglRealize(view) == PUGL_BAD_CALL);
    // assert(puglUnrealize(view) == PUGL_BAD_CALL);
    // assert(puglShow(view, PUGL_SHOW_RAISE) == PUGL_BAD_CALL);
    // assert(puglHide(view) == PUGL_BAD_CALL);
    // assert(puglSetViewStyle(view, 0U) == PUGL_BAD_CALL);
    assert(puglObscureView(view) == PUGL_BAD_CALL);
    assert(puglObscureRegion(view, 0, 0, 32U, 32U) == PUGL_BAD_CALL);
    // assert(puglGrabFocus(view) == PUGL_BAD_CALL);
    // assert(puglPaste(view) == PUGL_BAD_CALL);
    // assert(puglAcceptOffer(view, NULL, 0U) == PUGL_BAD_CALL);
    // assert(puglSetClipboard(view, NULL, NULL, 0U) == PUGL_BAD_CALL);
    // assert(puglSetCursor(view, PUGL_CURSOR_ARROW) == PUGL_BAD_CALL);
    assert(puglSendEvent(view, &expose_event) == PUGL_FAILURE);
    assert(puglSendEvent(view, &close_event) == PUGL_FAILURE);
    test->exposed = true;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglWorld* const world = puglNewWorld(PUGL_PROGRAM, 0U);
  assert(world);

  PuglTest test = {
    puglParseTestOptions(&argc, &argv), world, puglNewView(world), false};
  assert(test.view);

  // Set up view
  puglSetWorldString(test.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Bad Call Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPositionHint(test.view, PUGL_DEFAULT_POSITION, 0, 0);

  // Realize, show, then update until the view is exposed
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view, PUGL_SHOW_RAISE));
  while (!test.exposed) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
