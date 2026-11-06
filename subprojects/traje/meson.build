# Copyright 2026 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'traje',
  ['c'],
  default_options: [
    'b_ndebug=if-release',
    'buildtype=release',
    'c_std=c99',
    'c_winlibs=',
    'cpp_std=c++17',
  ],
  license: 'ISC',
  meson_version: '>= 0.54.0',
  version: '0.0.0',
)

#######################
# Compilers and Flags #
#######################

# Required tools
cc = meson.get_compiler('c')

# Warning suppressions
warning_level = get_option('warning_level')
c_suppressions = []
if cc.get_id() == 'clang'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-declaration-after-statement',
      '-Wno-unsafe-buffer-usage',
      '-Wno-switch-default',
    ]

    if not meson.is_cross_build()
      c_suppressions += ['-Wno-poison-system-directories']
    endif

    if host_machine.system() == 'freebsd'
      c_suppressions += ['-Wno-c11-extensions']
    endif
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += ['-Wno-switch-default']
  endif

elif cc.get_id() == 'msvc'
  c_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    c_suppressions += [
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

##########################
# Platform Configuration #
##########################

platform_args = []
if host_machine.system() != 'windows'
  platform_args = ['-D_POSIX_C_SOURCE=200809L', '-D_GNU_SOURCE']
endif

gettid_code = '''#include <unistd.h>
int main(void) { return gettid(); }'''

platform_args += '-DHAVE_GETTID=@0@'.format(
  cc.compiles(gettid_code, args: platform_args, name: 'gettid').to_int(),
)

###########
# Library #
###########

includes = include_directories(['include'])

traje_stub_dep = declare_dependency(
  include_directories: includes,
)

if not get_option('tracing').disabled()
  libtraje = static_library(
    'traje',
    files('src/traje.c'),
    c_args: c_suppressions + platform_args + ['-DTRAJE_ENABLED'],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: includes,
    install: false,
  )

  traje_dep = declare_dependency(
    include_directories: includes,
    link_with: libtraje,
  )

  meson.override_dependency('traje', traje_dep)
else
  traje_dep = traje_stub_dep
  meson.override_dependency('traje', traje_stub_dep)
endif

#########
# Tests #
#########

if not get_option('tests').disabled()
  subdir('test/c_header')
  if add_languages('cpp', native: false, required: false)
    subdir('test/cpp_header')
  endif
endif
