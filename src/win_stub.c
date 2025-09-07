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

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {0U,
                                      puglWinStubConfigure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubHandle,
                                      puglStubGetContext};

  return &backend;
}
