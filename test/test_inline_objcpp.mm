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

// Tests that the implementation compiles as included ObjC++

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#include "../src/implementation.c" // IWYU pragma: keep
#include "../src/mac.h"            // IWYU pragma: keep
#include "../src/mac.m"            // IWYU pragma: keep
#include "../src/mac_stub.m"       // IWYU pragma: keep

#if defined(WITH_CAIRO)
#  include "../src/mac_cairo.m" // IWYU pragma: keep
#endif

#if defined(WITH_OPENGL)
#  include "../src/mac_gl.m" // IWYU pragma: keep
#endif

#if defined(WITH_VULKAN)
#  include "../src/mac_vulkan.m" // IWYU pragma: keep
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

int
main(void)
{
  return 0;
}
