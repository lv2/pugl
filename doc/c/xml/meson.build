# Copyright 2021-2024 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_SRCDIR', pugl_src_root)
config.set('DOX_OUTPUT', meson.current_build_dir() / '..')

c_doxyfile = configure_file(
  configuration: config,
  input: '../Doxyfile.in',
  output: 'Doxyfile',
)

c_index_xml = custom_target(
  'c_index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [c_doxyfile] + c_headers,
  output: 'index.xml',
)
