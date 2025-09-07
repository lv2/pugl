// Copyright 2012-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "types.h"
#include "win.h"

#include <pugl/stub.h>
#include <stdbool.h>

static PuglStatus
puglWinStubConfigure(PuglView* view)
{
  return puglWinConfigure(view);
}

static PuglStatus
puglWinStubEnter(PuglView* view, const bool drawing)
{
  return puglWinEnter(view, drawing);
}

static PuglStatus
puglWinStubLeave(PuglView* view, const bool drawing)
{
  return puglWinLeave(view, drawing);
}

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {puglWinStubConfigure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglWinStubEnter,
                                      puglWinStubLeave,
                                      puglStubExpose,
                                      puglStubGetContext};

  return &backend;
}
