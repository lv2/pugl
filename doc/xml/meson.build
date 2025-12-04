# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_SRCDIR', pugl_src_root)
config.set('DOX_OUTPUT', meson.current_build_dir() / '..')

doxyfile = configure_file(
  configuration: config,
  input: '../Doxyfile.in',
  output: 'Doxyfile',
)

index_xml = custom_target(
  'index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [doxyfile] + c_headers,
  output: 'index.xml',
)
