doxygen = find_program('doxygen')

cpp_doxygen_input = []
foreach h : c_headers + cpp_headers
  cpp_doxygen_input += ['..' / h]
endforeach

config = configuration_data()
config.set('PUGL_HEADERS', ' '.join(cpp_doxygen_input))
config.set('PUGL_SRCDIR', pugl_src_root)

cpp_doxyfile = configure_file(configuration: config,
                              input: '../Doxyfile.in',
                              output: 'Doxyfile')

cpp_index_xml = custom_target(
  'cpp-index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [cpp_doxyfile] + c_header_files + cpp_header_files,
  output: 'index.xml')

