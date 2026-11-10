// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "types.h"
#include "win.h"

#include <pugl/stub.h>

static PuglStatus
puglWinStubAttach(PuglView* view)
{
  return puglWinAttach(view);
}

static void
puglWinStubDetach(PuglView* view)
{
  puglWinDetach(view);
}

static PuglStatus
puglWinStubEnter(PuglView* view, const bool expose)
{
  return puglWinEnter(view, expose);
}

static PuglStatus
puglWinStubLeave(PuglView* view, const bool expose)
{
  return puglWinLeave(view, expose);
}

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {puglWinStubAttach,
                                      puglWinStubDetach,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglWinStubEnter,
                                      puglStubDamage,
                                      puglWinStubLeave,
                                      puglStubGetContext};

  return &backend;
}
