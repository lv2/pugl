# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

c_headers = [
  'pugl/pugl.h',

  'pugl/cairo.h',
  'pugl/gl.h',
  'pugl/stub.h',
  'pugl/vulkan.h',
]

c_header_files = files(c_headers)

install_headers(c_headers, subdir: versioned_name / 'pugl')
