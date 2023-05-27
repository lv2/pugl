# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

c_headers = [
  'pugl/attributes.h',
  'pugl/pugl.h',

  'pugl/cairo.h',
  'pugl/gl.h',
  'pugl/glu.h',
  'pugl/stub.h',
  'pugl/vulkan.h',
]

c_header_files = files(c_headers)

install_headers(c_headers, subdir: versioned_name / 'pugl')
