# Copyright 2021-2024 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_CPP_SRCDIR', pugl_cpp_src_root)
config.set('DOX_OUTPUT', meson.current_build_dir() / '..')

cpp_doxyfile = configure_file(
  configuration: config,
  input: '../Doxyfile.in',
  output: 'Doxyfile',
)

cpp_index_xml = custom_target(
  'cpp_index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [cpp_doxyfile] + cpp_headers,
  output: 'index.xml',
)

