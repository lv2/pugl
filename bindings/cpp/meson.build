# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

subdir('include')

puglpp_dep = declare_dependency(
  include_directories: include_directories('include'),
  link_with: libpugl,
  dependencies: core_deps + [pugl_dep])

pkg.generate(name: 'Pugl++',
             filebase: 'puglpp-@0@'.format(major_version),
             subdirs: ['puglpp-@0@'.format(major_version)],
             version: meson.project_version(),
             description: 'Pugl GUI library C++ bindings')
