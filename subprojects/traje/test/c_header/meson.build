# Copyright 2026 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

header_c_suppressions = []
if get_option('warning_level') == 'everything'
  if cc.get_id() == 'clang' and not meson.is_cross_build()
    header_c_suppressions += ['-Wno-poison-system-directories']
  endif
endif

header_c_suppressions = cc.get_supported_arguments(header_c_suppressions)

test(
  'c_header',
  executable(
    'test_headers_c',
    files('test_headers.c'),
    c_args: header_c_suppressions,
    dependencies: [traje_dep],
    implicit_include_directories: false,
  ),
  suite: 'unit',
)
