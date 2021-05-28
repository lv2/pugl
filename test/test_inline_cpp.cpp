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

// Tests that the implementation compiles as included C++

#define PUGL_API

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "../src/implementation.c" // IWYU pragma: keep

#if defined(_WIN32)
#  include "../src/win.c"      // IWYU pragma: keep
#  include "../src/win.h"      // IWYU pragma: keep
#  include "../src/win_stub.c" // IWYU pragma: keep
#  if defined(WITH_CAIRO)
#    include "../src/win_cairo.c" // IWYU pragma: keep
#  endif
#  if defined(WITH_OPENGL)
#    include "../src/win_gl.c" // IWYU pragma: keep
#  endif
#  if defined(WITH_VULKAN)
#    include "../src/win_vulkan.c" // IWYU pragma: keep
#  endif

#else
#  include "../src/x11.c"      // IWYU pragma: keep
#  include "../src/x11_stub.c" // IWYU pragma: keep
#  if defined(WITH_CAIRO)
#    include "../src/x11_cairo.c" // IWYU pragma: keep
#  endif
#  if defined(WITH_OPENGL)
#    include "../src/x11_gl.c" // IWYU pragma: keep
#  endif
#  if defined(WITH_VULKAN)
#    include "../src/x11_vulkan.c" // IWYU pragma: keep
#  endif
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

int
main()
{
  return 0;
}
