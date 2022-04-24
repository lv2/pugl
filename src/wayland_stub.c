// Copyright 2022-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <pugl/stub.h>

#include "stub.h"
#include "types.h"

#include <pugl/pugl.h>

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {puglStubAttach,
                                      puglStubDetach,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}
