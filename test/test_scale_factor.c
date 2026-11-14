// Copyright 2021-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Test that the world scale factor is accurate to what the view actually gets

#undef NDEBUG

#include <pugl/pugl.h>
#include <pugl/stub.h>

#include <assert.h>

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  (void)view;
  (void)event;
  return PUGL_SUCCESS;
}

int
main(void)
{
  PuglWorld* const world = puglNewWorld(PUGL_PROGRAM, 0);

  puglSetWorldString(world, PUGL_CLASS_NAME, "PuglTest");

  // Get the world scale for our position
  const PuglPoint pos        = {64, 64};
  const double    worldScale = puglGetWorldScaleFactor(world, 0U, pos);

  // Create the view at that position and run an update
  PuglView* const view = puglNewView(world);
  puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl Scale Factor Test");
  puglSetBackend(view, puglStubBackend());
  puglSetEventFunc(view, onEvent);
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPositionHint(view, PUGL_DEFAULT_POSITION, pos.x, pos.y);
  assert(!puglShow(view, PUGL_SHOW_RAISE));
  assert(!puglUpdate(world, -1.0));

  // Check that the view scale factor is as expected
  const double viewScale = puglGetScaleFactor(view);
  assert(viewScale == worldScale);

  // Tear down
  puglFreeView(view);
  puglFreeWorld(world);

  return 0;
}
