// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "console.h"
#include "stub.h"
#include "types.h"

typedef struct {
  int pad;
} PuglConsoleVulkanSurface;

static PuglStatus
puglConsoleVulkanEnter(PuglView* PUGL_UNUSED(view), bool PUGL_UNUSED(expose))
{
  return PUGL_UNKNOWN_ERROR;
}

static PuglStatus
puglConsoleVulkanDamage(PuglView*              PUGL_UNUSED(view),
                        const PuglDamageEvent* PUGL_UNUSED(damage))
{
  return PUGL_UNKNOWN_ERROR;
}

static PuglStatus
puglConsoleVulkanLeave(PuglView* PUGL_UNUSED(view), bool PUGL_UNUSED(expose))
{
  return PUGL_UNKNOWN_ERROR;
}

static void*
puglConsoleVulkanGetContext(PuglView* PUGL_UNUSED(view))
{
  return NULL;
}

const PuglBackend*
puglVulkanBackend(void)
{
  static const PuglBackend backend = {puglConsoleVulkanAttach,
                                      puglConsoleVulkanDetach,
                                      puglStubCreate,
                                      puglConsoleVulkanDestroy,
                                      puglConsoleVulkanEnter,
                                      puglConsoleVulkanDamage,
                                      puglConsoleVulkanLeave,
                                      puglConsoleVulkanGetContext};

  return &backend;
}
