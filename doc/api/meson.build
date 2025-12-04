# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

pugl_rst = custom_target(
  'pugl.rst',
  command: [sphinxygen, '-f', '@INPUT0@', '@OUTDIR@'],
  input: [index_xml] + rst_files,
  output: 'pugl.rst',
)
