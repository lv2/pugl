shaders = [
  'header_330.glsl',
  'header_420.glsl',
  'rect.vert',
  'rect.frag',
]

foreach shader : shaders
  configure_file(input: files('..' / '..' / 'shaders' / shader),
                 output: shader,
                 copy: true)
endforeach
