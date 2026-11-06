# Copyright 2026 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

cpp = meson.get_compiler('cpp')

header_cpp_suppressions = []
if get_option('warning_level') == 'everything'
  if cpp.get_id() == 'clang'
    header_cpp_suppressions += [
      '-Wno-c++98-compat',
      '-Wno-lifetime-safety-intra-tu-constructor-suggestions',
    ]

    if not meson.is_cross_build()
      header_cpp_suppressions += ['-Wno-poison-system-directories']
    endif
  elif cc.get_id() == 'msvc'
    header_cpp_suppressions += [
      '/wd4514',  # unreferenced inline function has been removed
    ]
  endif
endif

header_cpp_suppressions = cpp.get_supported_arguments(header_cpp_suppressions)

test(
  'cpp_header',
  executable(
    'test_headers_cpp',
    files('test_headers.cpp'),
    cpp_args: header_cpp_suppressions,
    dependencies: [traje_dep],
    implicit_include_directories: false,
  ),
  suite: 'unit',
)
