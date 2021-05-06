# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

executable(
  'pugl_clipboard_demo',
  '../../pugl_clipboard_demo.c',
  include_directories: include_directories('../../../../include', '../../..'),
  c_args: example_defines + example_c_args,
  dependencies: [pugl_dep, gl_backend_dep])
