// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_CONSOLE_H
#define PUGL_SRC_CONSOLE_H

#include "types.h"
#include "xkb.h"

struct PuglWorldInternalsImpl {
  struct udev*     udev;
  struct libinput* libinput;
  PuglXkb          xkb;
  double           mouseX;
  double           mouseY;
};

struct PuglInternalsImpl {
  PuglSurface* surface;
};

#endif // PUGL_SRC_CONSOLE_H
