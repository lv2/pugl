# Copyright 2021-2024 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_VERSION', meson.project_version())

conf_py = configure_file(
  configuration: config,
  input: '../conf.py.in',
  output: 'conf.py',
)

configure_file(copy: true, input: '../deployment.rst', output: 'deployment.rst')
configure_file(copy: true, input: '../summary.rst', output: 'summary.rst')

cpp_rst_files = files(
  'event-loop.rst',
  'events.rst',
  'index.rst',
  'overview.rst',
  'view.rst',
  'world.rst',
)

foreach f : cpp_rst_files
  configure_file(copy: true, input: f, output: '@PLAINNAME@')
endforeach

subdir('xml')
subdir('api')

sphinx_args = ['@OUTDIR@', '@OUTDIR@', '-E', '-q']

foreach format : ['html', 'singlehtml']
  custom_target(
    'cpp_@0@'.format(format),
    build_by_default: true,
    command: [sphinx_build, ['-M', format], sphinx_args, ['-t', format]],
    input: [cpp_rst_files, cpp_pugl_rst, cpp_index_xml],
    install: true,
    install_dir: docdir / 'puglpp-0',
    output: format,
  )
endforeach
