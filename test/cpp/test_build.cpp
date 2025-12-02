// Copyright 2020-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests that C++ headers compile without any warnings

#include <pugl/cairo.hpp> // IWYU pragma: keep
#include <pugl/gl.hpp>    // IWYU pragma: keep
#include <pugl/pugl.hpp>  // IWYU pragma: keep
#include <pugl/stub.hpp>  // IWYU pragma: keep

#ifdef PUGL_TEST_VULKAN
#  include <pugl/vulkan.hpp> // IWYU pragma: keep
#endif

int
main()
{
  return 0;
}
