# Copyright 2021-2026 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'pugl',
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
  version: '0.5.7',
)

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

# Enable Objective C support if we're building for MacOS
if host_machine.system() == 'darwin'
  add_languages(['objc'], native: false)
  objcc = meson.get_compiler('objc')
endif

# Set global warning suppressions

warning_level = get_option('warning_level')

# C
c_suppressions = []
if cc.get_id() == 'clang'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-cast-function-type-strict',
      '-Wno-declaration-after-statement',
      '-Wno-padded',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsafe-buffer-usage',
    ]

    if not meson.is_cross_build()
      c_suppressions += ['-Wno-poison-system-directories']
    endif

    if host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-cast-function-type',
        '-Wno-covered-switch-default',
        '-Wno-deprecated-declarations',
        '-Wno-format-nonliteral',
        '-Wno-nonportable-system-include-path',
        '-Wno-unused-macros',
      ]
    endif
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=pure',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsuffixed-float-constants',
    ]

    if host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-cast-function-type',
        '-Wno-float-equal',
        '-Wno-pedantic',
        '-Wno-suggest-attribute=format',
      ]
    endif
  endif

elif cc.get_id() == 'msvc'
  c_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    c_suppressions += [
      '/wd4061',  # enumerator in switch is not explicitly handled
      '/wd4191',  # unsafe conversion from type to type
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4820',  # padding added after construct
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '/wd4100',  # unreferenced formal parameter
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

# Objective C
objc_suppressions = []
if is_variable('objcc')
  if warning_level == 'everything'
    objc_suppressions = [
      '-Wno-switch-enum',
      '-Wno-bad-function-cast',
      '-Wno-declaration-after-statement',
      '-Wno-direct-ivar-access',
      '-Wno-objc-messaging-id',
      '-Wno-padded',
      '-Wno-pedantic',
      '-Wno-poison-system-directories',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-undeclared-selector',
    ]

    objc_suppressions = objcc.get_supported_arguments(objc_suppressions)
    objc_suppressions += c_suppressions
  endif
endif

# Set additional suppressions for OpenGL code
opengl_args = []
if host_machine.system() == 'darwin'
  opengl_args += ['-DGL_SILENCE_DEPRECATION', '-Wno-deprecated-declarations']
endif

#############
# Platforms #
#############

dl_dep = cc.find_library('dl', required: false)
m_dep = cc.find_library('m', required: false)

platform_link_args = []

if host_machine.system() == 'darwin'
  # MacOS

  framework_deps = dependency(
    'appleframeworks',
    modules: ['appkit', 'foundation', 'corevideo'],
    required: false,
  )

  platform = 'mac'
  platform_sources = files('src/mac.m')
  platform_link_args = ['-Wl,-framework,Cocoa']
  core_deps = [framework_deps]
  backend_extension = '.m'
  soversion = meson.project_version().split('.')[0]

  platform_args = ['-DGL_SILENCE_DEPRECATION', '-Wno-deprecated-declarations']

elif host_machine.system() == 'windows'
  # Windows

  if get_option('default_library') == 'both'
    error('default_library=both is not supported on Windows')
  endif

  platform_args = [
    # Set minimum target version
    '-DWINVER=0x0500',  # Windows 2000
    '-D_WIN32_WINNT=0x0500',  # Windows 2000

    # Disable as many things from windows.h as possible
    '-DWIN32_LEAN_AND_MEAN',
    '-DNOGDICAPMASKS',  # CC_*, LC_*, PC_*, CP_*, TC_*, RC_
    '-DNOSYSMETRICS',  # SM_*
    '-DNOKEYSTATES',  # MK_*
    '-DOEMRESOURCE',  # OEM Resource values
    '-DNOATOM',  # Atom Manager routines
    '-DNOCOLOR',  # Screen colors
    '-DNODRAWTEXT',  # DrawText() and DT_*
    '-DNOKERNEL',  # All KERNEL defines and routines
    '-DNOMB',  # MB_* and MessageBox()
    '-DNOMEMMGR',  # GMEM_*, LMEM_*, GHND, LHND, associated routines
    '-DNOMETAFILE',  # typedef METAFILEPICT
    '-DNOMINMAX',  # Macros min(a,b) and max(a,b)
    '-DNOOPENFILE',  # OpenFile(), OemToAnsi, AnsiToOem, and OF_*
    '-DNOSCROLL',  # SB_* and scrolling routines
    '-DNOSERVICE',  # All Service Controller routines, SERVICE_ equates, etc.
    '-DNOSOUND',  # Sound driver routines
    '-DNOWH',  # SetWindowsHook and WH_*
    '-DNOCOMM',  # COMM driver routines
    '-DNOKANJI',  # Kanji support stuff
    '-DNOHELP',  # Help engine interface
    '-DNOPROFILER',  # Profiler interface
    '-DNODEFERWINDOWPOS',  # DeferWindowPos routines
    '-DNOMCX',  # Modem Configuration Extensions

    # Needed only for Cairo
    # '-DNORASTEROPS', # Binary and Tertiary raster ops

    # Needed by Pugl
    # '-DNOVIRTUALKEYCODES', # VK_*
    # '-DNOWINMESSAGES', # WM_*, EM_*, LB_*, CB_*
    # '-DNOWINSTYLES', # WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
    # '-DNOMENUS', # MF_*
    # '-DNOICONS', # IDI_*
    # '-DNOSYSCOMMANDS', # SC_*
    # '-DNOSHOWWINDOW', # SW_*
    # '-DNOCLIPBOARD', # Clipboard routines
    # '-DNOCTLMGR', # Control and Dialog routines
    # '-DNOGDI', # All GDI defines and routines
    # '-DNOUSER', # All USER defines and routines
    # '-DNONLS', # All NLS defines and routines
    # '-DNOMSG', # typedef MSG and associated routines
    # '-DNOWINOFFSETS', # GWL_*, GCL_*, associated routines
  ]

  if cc.get_id() == 'msvc'
    platform_args += [
      '-DNOTEXTMETRIC',  # typedef TEXTMETRIC and associated routines
    ]
  endif

  if not get_option('win_wchar').disabled()
    platform_args += ['-DUNICODE', '-D_UNICODE']
  endif

  dwmapi_dep = cc.find_library('dwmapi')
  gdi32_dep = cc.find_library('gdi32')
  shell32_dep = cc.find_library('shell32')
  shlwapi_dep = cc.find_library('shlwapi')
  user32_dep = cc.find_library('user32')

  platform = 'win'
  platform_sources = files('src/win.c')
  core_deps = [dwmapi_dep, gdi32_dep, shell32_dep, shlwapi_dep, user32_dep]
  backend_extension = '.c'
  soversion = ''  # Avoid redundant version in DLL name

else
  # X11
  x11_dep = cc.find_library('X11')
  platform_args = ['-D_POSIX_C_SOURCE=200809L']

  xcursor_dep = cc.find_library('Xcursor', required: get_option('xcursor'))
  platform_args += ['-DUSE_XCURSOR=@0@'.format(xcursor_dep.found().to_int())]

  xrandr_dep = cc.find_library('Xrandr', required: get_option('xrandr'))
  platform_args += ['-DUSE_XRANDR=@0@'.format(xrandr_dep.found().to_int())]

  use_xsync = false
  xext_dep = cc.find_library('Xext', required: get_option('xsync'))
  if xext_dep.found()
    xsync_fragment = '''#include <X11/Xlib.h>
      #include <X11/extensions/sync.h>
      int main(void) { XSyncQueryExtension(0, 0, 0); return 0; }'''
    use_xsync = cc.compiles(xsync_fragment, name: 'Xsync')
    if (not use_xsync) and (get_option('xsync') == 'enabled')
      error('C header <X11/extensions/sync.h> not found')
    endif
  endif
  platform_args += ['-DUSE_XSYNC=@0@'.format(use_xsync.to_int())]

  x11_suppressions = []
  if cc.get_id() == 'clang'
    x11_suppressions += [
      '-Wno-reserved-identifier',  # FD_ZERO
    ]
  endif

  platform = 'x11'
  platform_sources = files('src/x11.c')
  platform_args += cc.get_supported_arguments(x11_suppressions)
  core_deps = [x11_dep, xcursor_dep, xext_dep, xrandr_dep]
  backend_extension = '.c'
  soversion = meson.project_version().split('.')[0]
endif

################
# Core Library #
################

core_name = 'pugl_@0@@1@'.format(platform, version_suffix)
includes = include_directories(['include'])

# Determine if project should be installed
default_library = get_option('default_library')
install = (
  get_option('install').enabled()
  or (
    get_option('install').auto()
    and not (meson.is_subproject() and default_library == 'static')
  )
)

# "Usage" arguments for libraries and their dependants
usage_args = []
if default_library == 'static'
  usage_args += ['-DPUGL_STATIC']
endif

# Arguments used when building libraries
library_args = usage_args + platform_args
library_args += ['-DPUGL_INTERNAL']

common_sources = files('src/common.c', 'src/internal.c')

libpugl = library(
  core_name,
  common_sources + platform_sources,
  c_args: c_suppressions + library_args,
  dependencies: core_deps,
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  include_directories: includes,
  install: install,
  link_args: platform_link_args,
  objc_args: objc_suppressions + library_args,
  soversion: soversion,
  version: meson.project_version(),
)

pugl_dep = declare_dependency(
  compile_args: usage_args,
  dependencies: core_deps,
  include_directories: includes,
  link_with: libpugl,
)

if install
  pkg.generate(
    libpugl,
    description: 'Pugl GUI library core',
    filebase: versioned_name,
    name: 'Pugl',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )
endif

meson.override_dependency(versioned_name, pugl_dep)

############
# Backends #
############

# Stub Backend #

if get_option('stub')
  stub_backend = library(
    'pugl_@0@_stub@1@'.format(platform, version_suffix),
    files('src' / (platform + '_stub' + backend_extension)),
    c_args: c_suppressions + library_args,
    dependencies: [pugl_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: includes,
    install: install,
    link_args: platform_link_args,
    objc_args: objc_suppressions + library_args,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_stub_dep = declare_dependency(
    compile_args: usage_args,
    link_with: stub_backend,
    dependencies: [pugl_dep],
  )

  stub_pkg_name = 'pugl-stub-@0@'.format(major_version)
  meson.override_dependency(stub_pkg_name, pugl_stub_dep)

  if install
    pkg.generate(
      stub_backend,
      description: 'Pugl GUI library with stub (platform-specific) backend',
      filebase: stub_pkg_name,
      name: 'Pugl Stub',
      subdirs: [versioned_name],
      version: meson.project_version(),
    )
  endif
endif

# OpenGL Backend #

opengl_dep = dependency(
  'GL',
  include_type: 'system',
  required: get_option('opengl'),
)
if opengl_dep.found()
  gl_backend = library(
    'pugl_@0@_gl@1@'.format(platform, version_suffix),
    files('src' / (platform + '_gl' + backend_extension)),
    c_args: c_suppressions + library_args,
    dependencies: [pugl_dep, opengl_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: includes,
    install: install,
    link_args: platform_link_args,
    objc_args: objc_suppressions + library_args,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_gl_dep = declare_dependency(
    compile_args: usage_args,
    link_with: gl_backend,
    dependencies: [pugl_dep, opengl_dep],
  )

  gl_pkg_name = 'pugl-gl-@0@'.format(major_version)
  meson.override_dependency(gl_pkg_name, pugl_gl_dep)

  if install
    pkg.generate(
      gl_backend,
      description: 'Pugl GUI library with OpenGL backend',
      filebase: gl_pkg_name,
      name: 'Pugl OpenGL',
      subdirs: [versioned_name],
      version: meson.project_version(),
    )
  endif
endif

# Cairo Backend #

cairo_dep = dependency(
  'cairo',
  include_type: 'system',
  required: get_option('cairo'),
)
if cairo_dep.found()
  cairo_args = []
  if cc.get_id() == 'clang'
    cairo_args += [
      '-Wno-documentation',
      '-Wno-documentation-unknown-command',
      '-Wno-reserved-identifier',
    ]
  endif

  cairo_args = cc.get_supported_arguments(cairo_args)
  cairo_args += library_args

  cairo_framework_deps = []
  if host_machine.system() == 'darwin'
    cairo_framework_deps = dependency(
      'appleframeworks',
      modules: ['coregraphics'],
      required: false,
    )
  endif

  cairo_backend = library(
    'pugl_@0@_cairo@1@'.format(platform, version_suffix),
    files('src' / (platform + '_cairo' + backend_extension)),
    c_args: c_suppressions + cairo_args,
    dependencies: [pugl_dep, cairo_dep, cairo_framework_deps],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: includes,
    install: install,
    link_args: platform_link_args,
    objc_args: objc_suppressions + cairo_args,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_cairo_dep = declare_dependency(
    compile_args: usage_args,
    dependencies: [pugl_dep, cairo_dep, cairo_framework_deps],
    link_with: cairo_backend,
  )

  cairo_pkg_name = 'pugl-cairo-@0@'.format(major_version)
  meson.override_dependency(cairo_pkg_name, pugl_cairo_dep)

  if install
    pkg.generate(
      cairo_backend,
      description: 'Pugl GUI library with Cairo backend',
      filebase: cairo_pkg_name,
      name: 'Pugl Cairo',
      subdirs: [versioned_name],
      version: meson.project_version(),
    )
  endif
endif

# Vulkan
vulkan_dep = dependency(
  'vulkan',
  include_type: 'system',
  required: get_option('vulkan'),
)
if vulkan_dep.found()
  thread_dep = dependency('threads', include_type: 'system')

  vulkan_deps = [pugl_dep, vulkan_dep, dl_dep]
  if platform == 'mac'
    vulkan_deps += [
      dependency('Metal', include_type: 'system', modules: 'foundation'),
      dependency('QuartzCore', include_type: 'system', modules: 'foundation'),
    ]
  endif

  vulkan_args = []
  if cc.get_id() == 'gcc' and warning_level in ['everything', '3']
    vulkan_args += ['-Wno-pedantic']
  endif

  vulkan_backend = library(
    'pugl_@0@_vulkan@1@'.format(platform, version_suffix),
    files('src' / (platform + '_vulkan' + backend_extension)),
    c_args: c_suppressions + library_args + vulkan_args,
    dependencies: vulkan_deps,
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: includes,
    install: install,
    link_args: platform_link_args,
    objc_args: objc_suppressions + library_args,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_vulkan_dep = declare_dependency(
    compile_args: usage_args,
    dependencies: [pugl_dep, vulkan_dep, thread_dep],
    link_with: vulkan_backend,
  )

  vulkan_pkg_name = 'pugl-vulkan-@0@'.format(major_version)
  meson.override_dependency(vulkan_pkg_name, pugl_vulkan_dep)

  if install
    pkg.generate(
      vulkan_backend,
      description: 'Pugl GUI library with Vulkan backend',
      filebase: vulkan_pkg_name,
      name: 'Pugl Vulkan',
      subdirs: [versioned_name],
      version: meson.project_version(),
    )
  endif
endif

##################
# Public Headers #
##################

subdir('include')

######################
# Tests and Examples #
######################

if not get_option('tests').disabled() or not get_option('examples').disabled()
  puglutil = subproject('puglutil')
  puglutil_dep = puglutil.get_variable('puglutil_dep')
endif

if not get_option('tests').disabled()
  subdir('test')
endif

if not get_option('examples').disabled()
  subdir('examples')
endif

################
# C++ Bindings #
################

subproject('pugl-cpp', required: get_option('bindings_cpp'))

#################
# Documentation #
#################

if not get_option('docs').disabled()
  subdir('doc')
else
  build_docs = false
endif

if not meson.is_subproject()
  summary({'Platform': platform}, section: 'Configuration')

  if platform == 'x11'
    summary(
      {
        'Cursor support (XCursor)': xcursor_dep.found(),
        'Refresh rate support (XRandR)': xrandr_dep.found(),
        'Timer support (XSync)': use_xsync,
      },
      bool_yn: true,
      section: 'Configuration',
    )
  endif

  summary(
    {
      'Cairo': cairo_dep.found(),
      'OpenGL': opengl_dep.found(),
      'Stub': get_option('stub'),
      'Vulkan': vulkan_dep.found(),
    },
    bool_yn: true,
    section: 'Backends',
  )

  summary(
    {
      'Tests': not get_option('tests').disabled(),
      'Examples': not get_option('examples').disabled(),
      'Documentation': build_docs,
      'C++ bindings': not get_option('bindings_cpp').disabled(),
    },
    bool_yn: true,
    section: 'Components',
  )

  summary(
    {
      'Install prefix': get_option('prefix'),
      'Headers': get_option('prefix') / get_option('includedir'),
      'Libraries': get_option('prefix') / get_option('libdir'),
    },
    section: 'Directories',
  )

  if not get_option('examples').disabled()
    summary(
      {'Executables': get_option('prefix') / get_option('bindir')},
      section: 'Directories',
    )
  endif
endif
