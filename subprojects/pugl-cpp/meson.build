# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'pugl-cpp',
  ['cpp'],
  default_options: ['b_ndebug=if-release', 'buildtype=release', 'cpp_std=c++17'],
  license: 'ISC',
  meson_version: '>= 0.54.0',
  version: '0.5.7',
)

pugl_cpp_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
versioned_name = 'pugl-cpp-@0@'.format(major_version)

#######################
# Compilers and Flags #
#######################

# Required tools
pkg = import('pkgconfig')
cpp = meson.get_compiler('cpp')

# Strict warning flags for development
cpp_suppressions = []
warning_level = get_option('warning_level')
if cpp.get_id() == 'clang'
  if warning_level == 'everything'
    cpp_suppressions += [
      '-Wno-c++98-compat',
      '-Wno-c++98-compat-pedantic',
      '-Wno-cast-function-type-strict',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-sign-conversion',
      '-Wno-switch-default',
      '-Wno-unsafe-buffer-usage',
    ]

    if not meson.is_cross_build()
      cpp_suppressions += ['-Wno-poison-system-directories']
    endif
  endif

elif cpp.get_id() == 'gcc'
  if warning_level == 'everything'
    cpp_suppressions += [
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=pure',
    ]

    if host_machine.system() == 'windows'
      cpp_suppressions += [
        '-Wno-cast-function-type',
        '-Wno-suggest-attribute=format',
      ]
    endif
  endif

elif cpp.get_id() == 'msvc'
  cpp_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    cpp_suppressions += [
      '/wd4061',  # enumerator in switch is not explicitly handled
      '/wd4191',  # unsafe conversion from type to type
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4625',  # copy constructor implicitly deleted
      '/wd4626',  # copy assignment operator implicitly deleted
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4800',  # implicit conversion to bool
      '/wd4820',  # padding added after construct
      '/wd5026',  # move constructor implicitly deleted
      '/wd5027',  # move assignment operator implicitly deleted
      '/wd5039',  # pointer to potentially throwing function passed to C
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  endif
endif

cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)

################
# Dependencies #
################

dl_dep = cpp.find_library('dl', required: false)
pugl_dep = dependency('pugl-0', version: '>= 0.5.3')

vulkan_dep = dependency(
  'vulkan',
  include_type: 'system',
  required: get_option('vulkan'),
)

##################
# Public Headers #
##################

cpp_headers = files(
  'include/pugl/cairo.hpp',
  'include/pugl/gl.hpp',
  'include/pugl/pugl.hpp',
  'include/pugl/stub.hpp',
  'include/pugl/vulkan.hpp',
)

install_headers(cpp_headers, subdir: versioned_name / 'pugl')

###########
# Library #
###########

# Declare dependency for internal meson dependants
pugl_cpp_dep = declare_dependency(
  dependencies: [pugl_dep],
  include_directories: include_directories('include'),
)

# Generage pkg-config file for external dependants
pkg.generate(
  description: 'Pugl GUI library - C++ bindings',
  filebase: versioned_name,
  name: 'Pugl C++',
  subdirs: [versioned_name],
  version: meson.project_version(),
)

# Override pkg-config dependency for internal meson dependants
meson.override_dependency(versioned_name, pugl_cpp_dep)

#################
# Documentation #
#################

if not get_option('docs').disabled()
  subdir('doc')
endif

######################
# Tests and Examples #
######################

if not get_option('tests').disabled()
  subdir('test')
endif

if not get_option('examples').disabled()
  subdir('examples')
endif
