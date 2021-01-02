/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "pugl/stub.h"

#include "stub.h"
#include "types.h"
#include "x11.h"

#include "pugl/pugl.h"

#include <X11/Xutil.h>

PuglStatus
puglX11StubConfigure(PuglView* view)
{
  PuglInternals* const impl = view->impl;
  XVisualInfo          pat  = {0};
  int                  n    = 0;

  pat.screen = impl->screen;
  impl->vi   = XGetVisualInfo(impl->display, VisualScreenMask, &pat, &n);

  view->hints[PUGL_RED_BITS]   = impl->vi->bits_per_rgb;
  view->hints[PUGL_GREEN_BITS] = impl->vi->bits_per_rgb;
  view->hints[PUGL_BLUE_BITS]  = impl->vi->bits_per_rgb;
  view->hints[PUGL_ALPHA_BITS] = 0;

  return PUGL_SUCCESS;
}

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {
    puglX11StubConfigure,
    puglStubCreate,
    puglStubDestroy,
    puglStubEnter,
    puglStubLeave,
    puglStubGetContext,
  };

  return &backend;
}
