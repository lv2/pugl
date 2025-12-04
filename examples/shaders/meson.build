# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

shader_files = files(
  'header_320_es.glsl',
  'header_330.glsl',
  'header_420.glsl',
  'rect.frag',
  'rect.vert',
)

# Copy shader sources for GL examples
foreach shader_file : shader_files
  configure_file(copy: true, input: shader_file, output: '@PLAINNAME@')
endforeach
