# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

config = configuration_data()
config.set('PUGL_VERSION', meson.project_version())

conf_py = configure_file(configuration: config,
                         input: '../conf.py.in',
                         output: 'conf.py')

configure_file(copy: true, input: '../deployment.rst', output: 'deployment.rst')
configure_file(copy: true, input: '../summary.rst', output: 'summary.rst')

cpp_rst_files = files(
  'index.rst',
  'overview.rst',
  'world.rst',
  'view.rst',
  'events.rst',
  'event-loop.rst',
)

foreach f : cpp_rst_files
  configure_file(copy: true, input: f, output: '@PLAINNAME@')
endforeach

subdir('xml')
subdir('api')

docs = custom_target(
  'singlehtml',
  command: [sphinx_build, '-M', 'singlehtml', '@OUTDIR@', '@OUTDIR@',
            '-E', '-q', '-t', 'singlehtml'],
  input: [cpp_rst_files, cpp_pugl_rst, cpp_index_xml],
  output: 'singlehtml',
  build_by_default: true,
  install: true,
  install_dir: docdir / 'puglpp-0')

docs = custom_target(
  'html',
  command: [sphinx_build, '-M', 'html', '@OUTDIR@', '@OUTDIR@',
            '-E', '-q', '-t', 'html'],
  input: [cpp_rst_files, cpp_pugl_rst, cpp_index_xml],
  output: 'html',
  build_by_default: true,
  install: true,
  install_dir: docdir / 'puglpp-0')
