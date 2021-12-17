# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

docdir = get_option('datadir') / 'doc'

doxygen = find_program('doxygen', required: get_option('docs'))
dox_to_sphinx = find_program('../scripts/dox_to_sphinx.py')
sphinx_build = find_program('sphinx-build', required: get_option('docs'))

build_docs = doxygen.found() and sphinx_build.found()

if build_docs
  subdir('_static')
  subdir('c')
  subdir('cpp')
endif
