// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_STUB_H
#define PUGL_SRC_STUB_H

#include <pugl/attributes.h>
#include <pugl/pugl.h>

#include <stdbool.h>
#include <stddef.h>

PUGL_BEGIN_DECLS

static inline PuglStatus
puglStubConfigure(PuglView* const view)
{
  (void)view;
  return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubCreate(PuglView* const view)
{
  (void)view;
  return PUGL_SUCCESS;
}

static inline void
puglStubDestroy(PuglView* const view)
{
  (void)view;
}

static inline PuglStatus
puglStubEnter(PuglView* const view, const bool drawing)
{
  (void)view;
  (void)drawing;
  return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubLeave(PuglView* const view, const bool drawing)
{
  (void)view;
  (void)drawing;
  return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubHandle(PuglView* const view, const PuglEvent* const event)
{
  (void)view;
  (void)event;
  return PUGL_SUCCESS;
}

static inline PuglStatus
puglStubExpose(PuglView* const view, const PuglExposeEvent* const expose)
{
  (void)view;
  (void)expose;
  return PUGL_SUCCESS;
}

static inline void*
puglStubGetContext(PuglView* const view)
{
  (void)view;
  return NULL;
}

PUGL_END_DECLS

#endif // PUGL_SRC_STUB_H
