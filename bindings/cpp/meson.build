# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

subdir('include')

puglpp_dep = declare_dependency(
  dependencies: core_deps + [pugl_dep],
  include_directories: include_directories('include'),
  link_with: libpugl,
)

pkg.generate(
  description: 'Pugl GUI library C++ bindings',
  filebase: 'puglpp-@0@'.format(major_version),
  name: 'Pugl++',
  subdirs: ['puglpp-@0@'.format(major_version)],
  version: meson.project_version(),
)
