cpp_pugl_rst = custom_target(
  'C++ API ReST Documentation',
  command: [dox_to_sphinx, '-l', 'cpp', '-f', '@INPUT@', meson.current_build_dir()],
  input: cpp_index_xml,
  output: 'pugl.rst')
