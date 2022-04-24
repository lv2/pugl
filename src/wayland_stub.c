// Copyright 2022-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <pugl/stub.h>

#include "stub.h"
#include "types.h"
#include "wayland.h"

#include <pugl/pugl.h>

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {puglStubConfigure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}
