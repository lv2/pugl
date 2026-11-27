# Copyright 2026 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'caiwo',
  ['c'],
  default_options: [
    'b_ndebug=if-release',
    'buildtype=release',
    'c_std=gnu99',
    'c_winlibs=',
  ],
  license: 'ISC',
  meson_version: '>= 0.54.0',
  version: '0.0.0',
)

caiwo_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'caiwo' + version_suffix

#######################
# Compilers and Flags #
#######################

pkg = import('pkgconfig')
cc = meson.get_compiler('c')
if cc.get_id() != 'emscripten'
  error('This library can only be used in emscripten')
endif

c_suppressions = []
if get_option('warning_level') == 'everything'
  c_suppressions += [
    '-Wno-c11-extensions',
    '-Wno-disabled-macro-expansion',
    '-Wno-dollar-in-identifier-extension',
    '-Wno-padded',
    '-Wno-unsafe-buffer-usage',
    '-Wno-unused-parameter',
    '-Wno-variadic-macro-arguments-omitted',
  ]
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

###########
# Library #
###########

includes = include_directories(['include'])
library_c_args = c_suppressions
library_sources = files('src/cairo.c')
library_link_args = []

libcaiwo = library(
  'caiwo',
  library_sources,
  c_args: library_c_args,
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  include_directories: includes,
  install: false,
  link_args: library_link_args,
  version: meson.project_version(),
)

caiwo_dep = declare_dependency(
  include_directories: includes,
  link_with: libcaiwo,
)

meson.override_dependency(versioned_name, caiwo_dep)

