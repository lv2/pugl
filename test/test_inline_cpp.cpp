// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

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
