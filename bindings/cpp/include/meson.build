# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

cpp_headers = [
  'pugl/pugl.hpp',

  'pugl/cairo.hpp',
  'pugl/gl.hpp',
  'pugl/stub.hpp',
  'pugl/vulkan.hpp',
]

cpp_header_files = files(cpp_headers)

install_headers(cpp_headers, subdir: 'puglpp' + version_suffix / 'pugl')
