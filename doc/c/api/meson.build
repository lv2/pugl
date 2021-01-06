c_pugl_rst = custom_target(
  'C API ReST Documentation',
  command: [dox_to_sphinx, '-f', '@INPUT0@', 'doc/c/api'],
  input: [c_index_xml] + c_rst_files,
  output: 'pugl.rst')
