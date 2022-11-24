# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project('pugl', ['c'],
        version: '0.4.0',
        license: 'ISC',
        meson_version: '>= 0.54.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
          'cpp_std=c++14',
        ])

pugl_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'pugl' + version_suffix

#######################
# Compilers and Flags #
#######################

# Required tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Enable C++ support if we're building the examples
if not get_option('examples').disabled() or not get_option('tests').disabled()
  if add_languages(['cpp'], native: false, required: false)
    cpp = meson.get_compiler('cpp')
  endif
endif

# Enable Objective C support if we're building for MacOS
if host_machine.system() == 'darwin'
  add_languages(['objc'], native: false)
  objcc = meson.get_compiler('objc')
endif

# Set global ultra-strict warnings for developers if enabled
if get_option('strict')
  subdir('meson/warnings')
  subdir('meson/suppressions')
endif

# Disable deprecated API which is not used by tests or examples
add_project_arguments(['-DPUGL_DISABLE_DEPRECATED'],
                      language: ['c', 'cpp', 'objc'])

#############
# Platforms #
#############

core_args = []
dl_dep = cc.find_library('dl', required: false)

# MacOS
if host_machine.system() == 'darwin'
  cocoa_dep = dependency(
    'Cocoa',
    include_type: 'system',
    modules: 'foundation',
    required: false,
  )

  corevideo_dep = dependency(
    'CoreVideo',
    include_type: 'system',
    required: false,
  )

  platform = 'mac'
  platform_sources = files('src/mac.m')
  core_deps = [cocoa_dep, corevideo_dep]
  extension = '.m'

  platform_suppressions = [
    '-DGL_SILENCE_DEPRECATION',
    '-Wno-deprecated-declarations',
  ]

  add_project_arguments(platform_suppressions, language: ['c', 'objc'])
  add_project_link_arguments(['-Wl,-framework,Cocoa'], language: ['c', 'objc'])

# Windows
elif host_machine.system() == 'windows'
  if cc.get_id() == 'msvc'
    msvc_args = [
      '/TP',
      '/experimental:external',
      '/external:W0',
      '/external:anglebrackets',
    ]

    add_project_arguments(msvc_args, language: ['c', 'cpp'])
  endif

  win_args = [
    '-DWIN32_LEAN_AND_MEAN',
    '-D_CRT_SECURE_NO_WARNINGS',
  ]

  add_project_arguments(win_args, language: ['c', 'cpp'])

  user32_dep = cc.find_library('user32')
  shlwapi_dep = cc.find_library('shlwapi')

  platform = 'win'
  platform_sources = files('src/win.c')
  core_deps = [user32_dep, shlwapi_dep]
  extension = '.c'

else # X11
  x11_dep = cc.find_library('X11')
  core_args += '-D_POSIX_C_SOURCE=200809L'

  xcursor_dep = cc.find_library('Xcursor', required: false)
  if xcursor_dep.found()
    core_args += ['-DHAVE_XCURSOR']
  endif

  xrandr_dep = cc.find_library('Xrandr', required: false)
  if xrandr_dep.found()
    core_args += ['-DHAVE_XRANDR']
  endif

  xext_dep = cc.find_library('Xext', required: false)
  if xext_dep.found()
    xsync_fragment = '''#include <X11/Xlib.h>
      #include <X11/extensions/sync.h>
      int main(void) { XSyncQueryExtension(0, 0, 0); return 0; }'''
    if cc.compiles(xsync_fragment, name: 'Xsync')
      core_args += ['-DHAVE_XSYNC']
    endif
  endif

  x11_args = []
  if cc.get_id() == 'clang'
    x11_args += [
      '-Wno-reserved-identifier', # FD_ZERO
    ]
  endif

  core_args += cc.get_supported_arguments(x11_args)

  platform = 'x11'
  platform_sources = files('src/x11.c')
  core_deps = [x11_dep, xcursor_dep, xrandr_dep, xext_dep]
  extension = '.c'
endif

################
# Core Library #
################

core_deps += [cc.find_library('m', required: false)]
core_name = 'pugl_@0@@1@'.format(platform, version_suffix)
includes = include_directories(['include'])
library_args = ['-DPUGL_INTERNAL']

subdir('meson/library')
if library_type == 'static_library'
  add_project_arguments(['-DPUGL_STATIC'], language: ['c', 'cpp', 'objc'])
endif

common_sources = files(
  'src/common.c',
  'src/internal.c',
)

libpugl = build_target(
  core_name,
  common_sources + platform_sources,
  c_args: library_args + core_args,
  dependencies: core_deps,
  gnu_symbol_visibility: 'hidden',
  include_directories: includes,
  install: true,
  target_type: library_type,
  version: meson.project_version(),
)

pugl_dep = declare_dependency(
  dependencies: core_deps,
  include_directories: ['include'],
  link_with: libpugl,
)

pkg.generate(
  libpugl,
  description: 'Pugl GUI library core',
  filebase: versioned_name,
  name: 'Pugl',
  subdirs: [versioned_name],
  version: meson.project_version(),
)

meson.override_dependency(versioned_name, pugl_dep)

############
# Backends #
############

# Stub
if get_option('stub')
  name = 'pugl_' + platform + '_stub' + version_suffix
  sources = files('src/' + platform + '_stub' + extension)

  stub_backend = build_target(
    name,
    sources,
    c_args: library_args,
    dependencies: [pugl_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    target_type: library_type,
    version: meson.project_version(),
  )

  stub_backend_dep = declare_dependency(
    link_with: stub_backend,
    dependencies: [pugl_dep],
  )

  stub_pkg_name = 'pugl-stub-@0@'.format(major_version)

  pkg.generate(
    stub_backend,
    description: 'Native window pugl graphics backend',
    filebase: stub_pkg_name,
    name: 'Pugl Stub',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(stub_pkg_name, stub_backend_dep)
endif

# OpenGL
opengl_dep = dependency(
  'GL',
  include_type: 'system',
  required: get_option('opengl'),
)
if opengl_dep.found()
  name = 'pugl_' + platform + '_gl' + version_suffix
  sources = files('src/' + platform + '_gl' + extension)

  gl_backend = build_target(
    name,
    sources,
    c_args: library_args,
    dependencies: [pugl_dep, opengl_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    target_type: library_type,
    version: meson.project_version(),
  )

  gl_backend_dep = declare_dependency(
    link_with: gl_backend,
    dependencies: [pugl_dep, opengl_dep],
  )

  gl_pkg_name = 'pugl-gl-@0@'.format(major_version)

  pkg.generate(
    gl_backend,
    description: 'Pugl GUI library with OpenGL backend',
    filebase: gl_pkg_name,
    name: 'Pugl OpenGL',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(gl_pkg_name, gl_backend_dep)
endif

# Cairo
cairo_dep = dependency(
  'cairo',
  include_type: 'system',
  required: get_option('cairo'),
)
if cairo_dep.found()
  name = 'pugl_' + platform + '_cairo' + version_suffix
  sources = files('src/' + platform + '_cairo' + extension)

  cairo_args = []
  if cc.get_id() == 'clang'
    cairo_args += [
      '-Wno-documentation',
      '-Wno-documentation-unknown-command',
      '-Wno-reserved-identifier',
    ]
  endif

  cairo_args = cc.get_supported_arguments(cairo_args)

  cairo_backend = build_target(
    name,
    sources,
    c_args: library_args + cairo_args,
    dependencies: [pugl_dep, cairo_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    objc_args: library_args + cairo_args,
    target_type: library_type,
    version: meson.project_version(),
  )

  cairo_backend_dep = declare_dependency(
    dependencies: [pugl_dep, cairo_dep],
    link_with: cairo_backend,
  )

  cairo_pkg_name = 'pugl-cairo-@0@'.format(major_version)

  pkg.generate(
    cairo_backend,
    description: 'Pugl GUI library with Cairo backend',
    filebase: cairo_pkg_name,
    name: 'Pugl Cairo',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(cairo_pkg_name, cairo_backend_dep)
endif

# Vulkan
vulkan_dep = dependency(
  'vulkan',
  include_type: 'system',
  required: get_option('vulkan'),
)
if vulkan_dep.found()
  name = 'pugl_' + platform + '_vulkan' + version_suffix
  sources = files('src/' + platform + '_vulkan' + extension)

  thread_dep = dependency('threads', include_type: 'system')

  vulkan_deps = [pugl_dep, vulkan_dep, dl_dep]
  vulkan_c_args = library_args
  vulkan_link_args = []
  if platform == 'mac'
    vulkan_deps += [
      dependency('Metal', include_type: 'system', modules: 'foundation'),
      dependency('QuartzCore', include_type: 'system', modules: 'foundation'),
    ]
  endif

  vulkan_backend = build_target(
    name,
    sources,
    c_args: library_args,
    dependencies: vulkan_deps,
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    target_type: library_type,
    version: meson.project_version(),
  )

  vulkan_backend_dep = declare_dependency(
    dependencies: [pugl_dep, vulkan_dep, thread_dep],
    link_with: vulkan_backend,
  )

  vulkan_pkg_name = 'pugl-vulkan-@0@'.format(major_version)

  pkg.generate(
    vulkan_backend,
    description: 'Pugl GUI library with Vulkan backend',
    filebase: vulkan_pkg_name,
    name: 'Pugl Vulkan',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(vulkan_pkg_name, vulkan_backend_dep)
endif

###############################
# Public Headers and Bindings #
###############################

subdir('include')
subdir('bindings/cpp')

######################
# Tests and Examples #
######################

if not get_option('tests').disabled()
  subdir('test')
endif

if not get_option('examples').disabled()
  subdir('examples')
endif

#################
# Documentation #
#################

if not get_option('docs').disabled()
  subdir('doc')
else
  build_docs = false
endif

if meson.version().version_compare('>=0.53.0')
  summary('Platform', platform)

  summary('Cairo', cairo_dep.found(), section: 'Backends', bool_yn: true)
  summary('OpenGL', opengl_dep.found(), section: 'Backends', bool_yn: true)
  summary('Stub', get_option('stub'), section: 'Backends', bool_yn: true)
  summary('Vulkan', vulkan_dep.found(), section: 'Backends', bool_yn: true)

  summary('Tests', not get_option('tests').disabled(), bool_yn: true)
  summary('Examples', not get_option('examples').disabled(), bool_yn: true)
  summary('Documentation', build_docs, bool_yn: true)

  summary('Prefix', get_option('prefix'), section: 'Paths')
  summary('Headers', get_option('prefix') / get_option('includedir'), section: 'Paths')
  summary('Libraries', get_option('prefix') / get_option('libdir'), section: 'Paths')

  if not get_option('examples').disabled()
    summary('Executables', get_option('prefix') / get_option('bindir'), section: 'Paths')
  endif
endif
