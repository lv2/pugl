# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

shader_files = files('rect.frag', 'rect.vert')

# Copy shader sources for GL examples
foreach shader_file : shader_files
  configure_file(copy: true, input: shader_file, output: '@PLAINNAME@')
endforeach

# Build SPV shader binaries for Vulkan examples
if vulkan_dep.found()
  glslang = find_program('glslangValidator')
  glslang_command = [glslang, '-V', '-o', '@OUTPUT@', '@INPUT@']

  rect_vert_spv = custom_target(
    'rect.vert.spv',
    build_by_default: true,
    command: glslang_command,
    input: files('rect.vert'),
    install: false,
    output: 'rect.vert.spv',
  )

  rect_frag_spv = custom_target(
    'rect.frag.spv',
    build_by_default: true,
    command: glslang_command,
    input: files('rect.frag'),
    install: false,
    output: 'rect.frag.spv',
  )
endif
