# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

######################################
# C++ Bindings Package (Header-Only) #
######################################

subdir('include')

pugl_cpp_dep = declare_dependency(
  dependencies: [pugl_dep],
  include_directories: include_directories('include'),
  link_with: libpugl,
)

pugl_cpp_versioned_name = 'pugl-cpp-@0@'.format(major_version)

pkg.generate(
  description: 'Pugl GUI library C++ bindings',
  filebase: pugl_cpp_versioned_name,
  name: 'Pugl++',
  subdirs: [pugl_cpp_versioned_name],
  version: meson.project_version(),
)

meson.override_dependency(pugl_cpp_versioned_name, pugl_cpp_dep)

############################
# C++ Header Warning Tests #
############################

subdir('test/headers')
