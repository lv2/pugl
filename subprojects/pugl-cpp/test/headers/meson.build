# Copyright 2020-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

header_cpp_suppressions = []
if get_option('warning_level') == 'everything'
  if cpp.get_id() == 'clang'
    header_cpp_suppressions += [
      '-Wno-c++98-compat-pedantic',
      '-Wno-padded',
      '-Wno-switch-default',
      '-Wno-unsafe-buffer-usage',
    ]

    if not meson.is_cross_build()
      header_cpp_suppressions += ['-Wno-poison-system-directories']
    endif
  elif cpp.get_id() == 'gcc'
    header_cpp_suppressions += ['-Wno-padded']
  elif cpp.get_id() == 'msvc'
    header_cpp_suppressions += [
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4625',  # copy constructor implicitly deleted
      '/wd4626',  # copy assignment operator implicitly deleted
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4820',  # padding added after construct
      '/wd5027',  # move assignment operator implicitly deleted
      '/wd5039',  # pointer to potentially throwing function passed to C
    ]
  endif
endif

header_cpp_suppressions = cpp.get_supported_arguments(header_cpp_suppressions)

# Test build of pugl/gl.hpp etc., without including OpenGL headers
test_headers_cpp_args = header_cpp_suppressions
test_headers_cpp_args += ['-DPUGL_NO_INCLUDE_GL_H', '-DPUGL_NO_INCLUDE_GLU_H']

# Only test build of pugl/vulkan.hpp if Vulkan is available
if vulkan_dep.found()
  test_headers_cpp_args += ['-DPUGL_TEST_VULKAN']
endif

test(
  'headers',
  executable(
    'test_headers_cpp',
    files('test_headers.cpp'),
    cpp_args: test_headers_cpp_args,
    dependencies: [pugl_cpp_dep, vulkan_dep],
    implicit_include_directories: false,
  ),
  suite: 'unit',
)
