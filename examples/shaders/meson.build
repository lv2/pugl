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

  shaders = ['rect.vert', 'rect.frag']
  foreach shader : shaders
    source = shader.split('.')[0] + '.vulkan.' + shader.split('.')[1]
    shader_input = custom_target(source,
                                 output: source,
                                 input: ['header_420.glsl', shader],
                                 command: [cat, '@INPUT@'],
                                 build_by_default: true,
                                 capture: true)

    mytarget = custom_target(shader,
                             output: shader + '.spv',
                             input: shader_input,
                             command: [glslang, '-V', '-o', '@OUTPUT@', '@INPUT@'],
                             build_by_default: true,
                             install: false)
  endforeach
endif
