// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Common implementations of public API functions in the core library

#include "internal.h"
#include "platform.h"
#include "types.h"

#include <pugl/pugl.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char*
puglStrerror(const PuglStatus status)
{
  // clang-format off
  switch (status) {
  case PUGL_SUCCESS:               return "Success";
  case PUGL_FAILURE:               return "Non-fatal failure";
  case PUGL_UNKNOWN_ERROR:         return "Unknown system error";
  case PUGL_BAD_BACKEND:           return "Invalid or missing backend";
  case PUGL_BAD_CONFIGURATION:     return "Invalid view configuration";
  case PUGL_BAD_PARAMETER:         return "Invalid parameter";
  case PUGL_BACKEND_FAILED:        return "Backend initialisation failed";
  case PUGL_REGISTRATION_FAILED:   return "Class registration failed";
  case PUGL_REALIZE_FAILED:        return "View creation failed";
  case PUGL_SET_FORMAT_FAILED:     return "Failed to set pixel format";
  case PUGL_CREATE_CONTEXT_FAILED: return "Failed to create drawing context";
  case PUGL_UNSUPPORTED:           return "Unsupported operation";
  case PUGL_NO_MEMORY:             return "Failed to allocate memory";
  }
  // clang-format on

  return "Unknown error";
}

PuglWorld*
puglNewWorld(PuglWorldType type, PuglWorldFlags flags)
{
  PuglWorld* world = (PuglWorld*)calloc(1, sizeof(PuglWorld));
  if (!world || !(world->impl = puglInitWorldInternals(type, flags))) {
    free(world);
    return NULL;
  }

  world->startTime = puglGetTime(world);
  world->type      = type;

  puglSetString(&world->strings[PUGL_CLASS_NAME], "Pugl");

  return world;
}

void
puglFreeWorld(PuglWorld* const world)
{
  puglFreeWorldInternals(world);

  for (size_t i = 0; i < PUGL_NUM_STRING_HINTS; ++i) {
    free(world->strings[i]);
  }

  free(world->views);
  free(world);
}

void
puglSetWorldHandle(PuglWorld* world, PuglWorldHandle handle)
{
  world->handle = handle;
}

PuglWorldHandle
puglGetWorldHandle(PuglWorld* world)
{
  return world->handle;
}

PuglStatus
puglSetWorldString(PuglWorld* const     world,
                   const PuglStringHint key,
                   const char* const    value)
{
  if ((unsigned)key >= PUGL_NUM_STRING_HINTS) {
    return PUGL_BAD_PARAMETER;
  }

  puglSetString(&world->strings[key], value);
  return PUGL_SUCCESS;
}

const char*
puglGetWorldString(const PuglWorld* const world, const PuglStringHint key)
{
  if ((unsigned)key >= PUGL_NUM_STRING_HINTS) {
    return NULL;
  }

  return world->strings[key];
}

static void
puglSetDefaultHints(PuglView* const view)
{
  view->hints[PUGL_CONTEXT_API]           = PUGL_OPENGL_API;
  view->hints[PUGL_CONTEXT_VERSION_MAJOR] = 2;
  view->hints[PUGL_CONTEXT_VERSION_MINOR] = 0;
  view->hints[PUGL_CONTEXT_PROFILE]       = PUGL_OPENGL_CORE_PROFILE;
  view->hints[PUGL_CONTEXT_DEBUG]         = PUGL_FALSE;
  view->hints[PUGL_RED_BITS]              = 8;
  view->hints[PUGL_GREEN_BITS]            = 8;
  view->hints[PUGL_BLUE_BITS]             = 8;
  view->hints[PUGL_ALPHA_BITS]            = 8;
  view->hints[PUGL_DEPTH_BITS]            = 0;
  view->hints[PUGL_STENCIL_BITS]          = 0;
  view->hints[PUGL_SAMPLE_BUFFERS]        = PUGL_DONT_CARE;
  view->hints[PUGL_SAMPLES]               = 0;
  view->hints[PUGL_DOUBLE_BUFFER]         = PUGL_TRUE;
  view->hints[PUGL_SWAP_INTERVAL]         = PUGL_DONT_CARE;
  view->hints[PUGL_RESIZABLE]             = PUGL_FALSE;
  view->hints[PUGL_IGNORE_KEY_REPEAT]     = PUGL_FALSE;
  view->hints[PUGL_REFRESH_RATE]          = PUGL_DONT_CARE;
  view->hints[PUGL_VIEW_TYPE]             = PUGL_DONT_CARE;

  for (unsigned i = 0U; i < PUGL_NUM_POSITION_HINTS; ++i) {
    view->positionHints[i].x = INT16_MIN;
    view->positionHints[i].y = INT16_MIN;
  }

  for (unsigned i = 0U; i < PUGL_NUM_SIZE_HINTS; ++i) {
    view->sizeHints[i].width  = 0U;
    view->sizeHints[i].height = 0U;
  }
}

PuglView*
puglNewView(PuglWorld* const world)
{
  PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
  if (!view || !(view->impl = puglInitViewInternals(world))) {
    free(view);
    return NULL;
  }

  view->world = world;
  puglSetDefaultHints(view);

  // Enlarge world view list
  const size_t     newNumViews = world->numViews + 1U;
  PuglView** const views =
    (PuglView**)realloc(world->views, newNumViews * sizeof(PuglView*));

  if (!views) {
    free(view);
    return NULL;
  }

  // Add to world view list
  world->views                  = views;
  world->views[world->numViews] = view;
  world->numViews               = newNumViews;

  return view;
}

void
puglFreeView(PuglView* view)
{
  // Remove from world view list
  PuglWorld* world = view->world;
  for (size_t i = 0; i < world->numViews; ++i) {
    if (world->views[i] == view) {
      if (i == world->numViews - 1) {
        world->views[i] = NULL;
      } else {
        memmove(world->views + i,
                world->views + i + 1,
                sizeof(PuglView*) * (world->numViews - i - 1));
        world->views[world->numViews - 1] = NULL;
      }
      --world->numViews;
    }
  }

  for (size_t i = 0; i < PUGL_NUM_STRING_HINTS; ++i) {
    free(view->strings[i]);
  }

  puglFreeViewInternals(view);
  free(view);
}

PuglWorld*
puglGetWorld(PuglView* view)
{
  return view->world;
}

void
puglSetHandle(PuglView* view, PuglHandle handle)
{
  view->handle = handle;
}

PuglHandle
puglGetHandle(PuglView* view)
{
  return view->handle;
}

PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend)
{
  view->backend = backend;
  return PUGL_SUCCESS;
}

const PuglBackend*
puglGetBackend(const PuglView* view)
{
  return view->backend;
}

PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc)
{
  view->eventFunc = eventFunc;
  return PUGL_SUCCESS;
}

PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value)
{
  if (value == PUGL_DONT_CARE) {
    switch (hint) {
    case PUGL_CONTEXT_API:
    case PUGL_CONTEXT_VERSION_MAJOR:
    case PUGL_CONTEXT_VERSION_MINOR:
    case PUGL_CONTEXT_PROFILE:
    case PUGL_CONTEXT_DEBUG:
    case PUGL_SWAP_INTERVAL:
      return PUGL_BAD_PARAMETER;
    default:
      break;
    }
  }

  if ((unsigned)hint < PUGL_NUM_VIEW_HINTS) {
    view->hints[hint] = value;
    return PUGL_SUCCESS;
  }

  return PUGL_BAD_PARAMETER;
}

int
puglGetViewHint(const PuglView* view, PuglViewHint hint)
{
  if ((unsigned)hint < PUGL_NUM_VIEW_HINTS) {
    return view->hints[hint];
  }

  return PUGL_DONT_CARE;
}

PuglStatus
puglSetViewString(PuglView* const      view,
                  const PuglStringHint key,
                  const char* const    value)
{
  if ((unsigned)key >= PUGL_NUM_STRING_HINTS) {
    return PUGL_BAD_PARAMETER;
  }

  puglSetString(&view->strings[key], value);
  return puglViewStringChanged(view, key, view->strings[key]);
}

const char*
puglGetViewString(const PuglView* const view, const PuglStringHint key)
{
  if ((unsigned)key >= PUGL_NUM_STRING_HINTS) {
    return NULL;
  }

  return view->strings[key];
}

PuglPoint
puglGetPositionHint(const PuglView* const view, const PuglPositionHint hint)
{
  if (hint == PUGL_CURRENT_POSITION) {
    PuglPoint pos = {0, 0};
    if (view->lastConfigure.type == PUGL_CONFIGURE) {
      pos.x = view->lastConfigure.x;
      pos.y = view->lastConfigure.y;
    } else {
      const PuglPoint defaultPos = view->positionHints[PUGL_DEFAULT_POSITION];
      if (puglIsValidPosition(defaultPos.x, defaultPos.y)) {
        pos.x = defaultPos.x;
        pos.y = defaultPos.y;
      }
    }
    return pos;
  }

  return view->positionHints[hint];
}

PuglArea
puglGetSizeHint(const PuglView* const view, const PuglSizeHint hint)
{
  if (hint == PUGL_CURRENT_SIZE && view->lastConfigure.type == PUGL_CONFIGURE) {
    const PuglArea area = {view->lastConfigure.width,
                           view->lastConfigure.height};
    return area;
  }

  return view->sizeHints[hint];
}

PuglStatus
puglSetParent(PuglView* view, PuglNativeView parent)
{
  view->parent = parent;
  return PUGL_SUCCESS;
}

PuglNativeView
puglGetParent(const PuglView* const view)
{
  return view->parent;
}

PuglNativeView
puglGetTransientParent(const PuglView* const view)
{
  return view->transientParent;
}

bool
puglGetVisible(const PuglView* view)
{
  return (view->lastConfigure.style & PUGL_VIEW_STYLE_MAPPED) &&
         !(view->lastConfigure.style & PUGL_VIEW_STYLE_HIDDEN);
}

void*
puglGetContext(PuglView* const view)
{
  return view->backend->getContext(view);
}

PuglViewStyleFlags
puglGetViewStyle(const PuglView* const view)
{
  return view->lastConfigure.style;
}
