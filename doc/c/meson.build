# Copyright 2021-2024 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_VERSION', meson.project_version())

conf_py = configure_file(
  configuration: config,
  input: '../conf.py.in',
  output: 'conf.py',
)

deployment_rst = configure_file(
  copy: true,
  input: '../deployment.rst',
  output: 'deployment.rst',
)

summary_rst = configure_file(
  copy: true,
  input: '../summary.rst',
  output: 'summary.rst',
)

copied_rst_files = [deployment_rst, summary_rst]

c_rst_files = files(
  'clipboards.rst',
  'event-loop.rst',
  'events.rst',
  'index.rst',
  'overview.rst',
  'shutting-down.rst',
  'view.rst',
  'world.rst',
)

foreach f : c_rst_files
  configure_file(copy: true, input: f, output: '@PLAINNAME@')
endforeach

subdir('xml')
subdir('api')

docs = custom_target(
  'c_singlehtml',
  build_by_default: true,
  command: [
    sphinx_build,
    '-M', 'singlehtml',
    '@OUTDIR@',
    '@OUTDIR@',
    '-E',
    '-q',
    '-t', 'singlehtml',
  ],
  input: [c_rst_files, copied_rst_files, c_pugl_rst, c_index_xml],
  install: true,
  install_dir: docdir / 'pugl-0',
  output: 'singlehtml',
)

docs = custom_target(
  'c_html',
  build_by_default: true,
  command: [
    sphinx_build,
    '-M', 'html',
    '@OUTDIR@',
    '@OUTDIR@',
    '-E',
    '-q',
    '-t', 'html',
  ],
  input: [c_rst_files, copied_rst_files, c_pugl_rst, c_index_xml],
  install: true,
  install_dir: docdir / 'pugl-0',
  output: 'html',
)
