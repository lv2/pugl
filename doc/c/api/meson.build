# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

c_pugl_rst = custom_target(
  'C API ReST Documentation',
  command: [dox_to_sphinx, '-f', '@INPUT0@', 'doc/c/api'],
  input: [c_index_xml] + c_rst_files,
  output: 'pugl.rst')
