doxygen = find_program('doxygen')

c_doxygen_input = []
foreach h : c_headers
  c_doxygen_input += ['..' / h]
endforeach

config = configuration_data()
config.set('PUGL_HEADERS', ' '.join(c_doxygen_input))
config.set('PUGL_SRCDIR', pugl_src_root)

c_doxyfile = configure_file(configuration: config,
                            input: '../Doxyfile.in',
                            output: 'Doxyfile')

c_index_xml = custom_target('c-index.xml',
                            command: [doxygen, '@INPUT0@'],
                            input: [c_doxyfile] + c_header_files,
                            output: 'index.xml')
