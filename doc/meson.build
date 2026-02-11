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
  subdir('_static')

  config = configuration_data()
  config.set('PUGL_VERSION', meson.project_version())

  conf_py = configure_file(
    configuration: config,
    input: files('conf.py.in'),
    output: 'conf.py',
  )

  rst_files_src = files(
    'clipboards.rst',
    'deployment.rst',
    'event-loop.rst',
    'events.rst',
    'index.rst',
    'overview.rst',
    'shutting-down.rst',
    'summary.rst',
    'view.rst',
    'world.rst',
  )

  rst_files = []
  foreach f : rst_files_src
    rst_files += [configure_file(copy: true, input: f, output: '@PLAINNAME@')]
  endforeach

  subdir('xml')
  subdir('api')

  sphinx_args = ['@OUTDIR@', '@OUTDIR@', '-E', '-q']

  foreach format : ['html', 'singlehtml']
    custom_target(
      format,
      build_by_default: true,
      command: [sphinx_build, ['-M', format], sphinx_args, ['-t', format]],
      input: [conf_py, rst_files, pugl_rst, index_xml],
      install: true,
      install_dir: docdir / 'pugl-0',
      output: format,
    )
  endforeach
endif
