# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_cursor_demo',
  '../../pugl_cursor_demo.c',
  c_args: example_defines + example_c_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [pugl_dep, pugl_gl_dep, puglutil_dep],
  implicit_include_directories: false,
)
