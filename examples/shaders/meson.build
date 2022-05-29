# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

shader_files = [
  'header_330.glsl',
  'header_420.glsl',
  'rect.frag',
  'rect.vert',
]

# Copy shader sources for GL examples
foreach shader_file : shader_files
  configure_file(copy: true, input: shader_file, output: shader_file)
endforeach

# Build SPV shader binaries for Vulkan examples
if vulkan_dep.found()
  cat = find_program('../../scripts/cat.py')
  glslang = find_program('glslangValidator')

  rect_vulkan_vert = custom_target('rect.vulkan.vert',
                                   output: 'rect.vulkan.vert',
                                   input: ['header_420.glsl', 'rect.vert'],
                                   command: [cat, '@INPUT@'],
                                   build_by_default: true,
                                   capture: true)

  rect_vulkan_frag = custom_target('rect.vulkan.frag',
                                   output: 'rect.vulkan.frag',
                                   input: ['header_420.glsl', 'rect.frag'],
                                   command: [cat, '@INPUT@'],
                                   build_by_default: true,
                                   capture: true)

  glslang_command = [glslang, '-V', '-o', '@OUTPUT@', '@INPUT@']

  rect_vert_spv = custom_target('rect.vert.spv',
                                output: 'rect.vert.spv',
                                input: rect_vulkan_vert,
                                command: glslang_command,
                                build_by_default: true,
                                install: false)

  rect_frag_spv = custom_target('rect.frag.spv',
                                output: 'rect.frag.spv',
                                input: rect_vulkan_frag,
                                command: glslang_command,
                                build_by_default: true,
                                install: false)
endif
