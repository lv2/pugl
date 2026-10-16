# Copyright 2025-2026 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'puglutil',
  ['c'],
  license: 'ISC',
  meson_version: '>= 0.54.0',
  version: '0.0.0',
)

#######################
# Compilers and Flags #
#######################

# Required tools
cc = meson.get_compiler('c')

###########
# Library #
###########

includes = include_directories(['include'])

puglutil_dep = declare_dependency(
  include_directories: includes,
)

meson.override_dependency('puglutil', puglutil_dep)
