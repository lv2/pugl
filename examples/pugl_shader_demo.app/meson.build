config = configuration_data()
config.set('NAME', 'pugl_shader_demo')

info_plist = configure_file(configuration: config,
                            input: files('../../resources/Info.plist.in'),
                            output: 'Info.plist')

shaders = [
  'header_330.glsl',
  'header_420.glsl',
  'rect.vert',
  'rect.frag',
]

foreach shader : shaders
  configure_file(input: files('..' / 'shaders' / shader),
                 output: shader,
                 copy: true)
endforeach

subdir('MacOS')
