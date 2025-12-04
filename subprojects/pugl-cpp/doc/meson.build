# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

docdir = get_option('datadir') / 'doc'

# Find required external programs
doxygen = find_program('doxygen', required: get_option('docs'))
sphinx_build = find_program('sphinx-build', required: get_option('docs'))

# Find sphinxygen or fall back to subproject
if doxygen.found() and sphinx_build.found()
  sphinxygen = find_program('sphinxygen', required: false)
  if not sphinxygen.found()
    subproject('sphinxygen')
    sphinxygen = find_program('sphinxygen', required: get_option('docs'))
  endif
else
  sphinxygen = disabler()
endif

# Build documentation if all required tools are found
build_docs = doxygen.found() and sphinxygen.found() and sphinx_build.found()
if build_docs
  config = configuration_data()
  config.set('PUGL_VERSION', meson.project_version())

  conf_py = configure_file(
    configuration: config,
    input: files('conf.py.in'),
    output: 'conf.py',
  )

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
      install_dir: docdir / 'pugl-cpp-0',
      output: format,
    )
  endforeach
endif
