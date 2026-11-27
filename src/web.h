// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WEB_H
#define PUGL_SRC_WEB_H

#include "pugl/pugl.h"
#include "types.h"

typedef struct {
  PuglView* view;
  uintptr_t id;
} PuglTimer;

struct PuglWorldInternalsImpl {
  double scaleFactor;
};

typedef struct {
  double x;
  double y;
  double xRoot;
  double yRoot;
} LastMotionValues;

struct PuglInternalsImpl {
  PuglSurface*      surface;
  uint32_t          numTimers;
  LastMotionValues  lastMotion;
  PuglEvent         nextButtonEvent;
  PuglViewHintValue supportsClipboardRead;
  PuglViewHintValue supportsClipboardWrite;
  char*             clipboardData;
  PuglTimer*        timers;
  bool              isFullscreen;
  bool              needsRepaint;
  bool              pointerLocked;
  bool              created;
  bool              visible;
  bool              focused;
};

#endif // PUGL_SRC_WEB_H
