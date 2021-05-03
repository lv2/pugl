/*
  Copyright 2021 David Robillard <d@drobilla.net>

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

/*
  Tests basic functionality of, and access to, the world.
*/

#undef NDEBUG

#include "pugl/pugl.h"

#include <assert.h>
#include <stdint.h>

int
main(void)
{
  PuglWorld* const world = puglNewWorld(PUGL_PROGRAM, 0);
  PuglView* const  view  = puglNewView(world);

  // Check that the world can be accessed from the view
  assert(puglGetWorld(view) == world);

  // Check that puglGetNativeWorld() returns something
  assert(puglGetNativeWorld(world));

  // Set and get world handle
  uintptr_t data = 1234;
  puglSetWorldHandle(world, &data);
  assert(puglGetWorldHandle(world) == &data);

  // Tear down
  puglFreeView(view);
  puglFreeWorld(world);

  return 0;
}
