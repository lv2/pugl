cpp_pugl_rst = custom_target(
  'C++ API ReST Documentation',
  command: [dox_to_sphinx, '-l', 'cpp', '-f', '@INPUT@', 'doc/cpp/api'],
  input: cpp_index_xml,
  output: 'pugl.rst')
