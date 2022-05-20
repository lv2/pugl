config = configuration_data()
config.set('NAME', 'pugl_vulkan_cpp_demo')

info_plist = configure_file(configuration: config,
                            input: files('../../resources/Info.plist.in'),
                            output: 'Info.plist')

shaders = [
  'rect.frag.spv',
  'rect.vert.spv',
]

cp = find_program('../../scripts/cp.py')

custom_target('rect.vert.spv',
              input: rect_vert_spv,
              output: 'rect.vert.spv',
              command: [cp, '@INPUT@', '@OUTPUT@'],
              build_by_default: true)

custom_target('rect.frag.spv',
              input: rect_frag_spv,
              output: 'rect.frag.spv',
              command: [cp, '@INPUT@', '@OUTPUT@'],
              build_by_default: true)

subdir('MacOS')
