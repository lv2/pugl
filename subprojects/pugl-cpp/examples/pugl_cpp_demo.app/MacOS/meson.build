# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_cpp_demo',
  '../../pugl_cpp_demo.cpp',
  cpp_args: example_defines + example_cpp_args + opengl_args,
  dependencies: [pugl_cpp_dep, pugl_gl_dep, puglutil_dep],
  implicit_include_directories: false,
)
