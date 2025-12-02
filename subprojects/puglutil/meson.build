# Copyright 2025 David Robillard <d@drobilla.net>
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

# Warning suppressions
warning_level = get_option('warning_level')
c_suppressions = []
if cc.get_id() == 'clang'
  if warning_level == 'everything'
    c_suppressions += ['-Wno-declaration-after-statement']

    if not meson.is_cross_build()
      c_suppressions += ['-Wno-poison-system-directories']
    endif
  endif
elif cc.get_id() == 'gcc'
  c_suppressions += ['-Wno-suggest-attribute=malloc']
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

#############
# Platform #
#############

platform_args = []
if host_machine.system() in ['gnu', 'linux']
  platform_args = ['-D_POSIX_C_SOURCE=200809L']
endif

###########
# Library #
###########

includes = include_directories(['include'])

sources = files('src/file_utils.c')

libpuglutil = static_library(
  'puglutil',
  sources,
  c_args: c_suppressions + platform_args,
  implicit_include_directories: false,
  include_directories: includes,
  install: false,
)

puglutil_dep = declare_dependency(
  include_directories: includes,
  link_with: libpuglutil,
)

meson.override_dependency('puglutil', puglutil_dep)
