project('pugl', ['c'],
        version: '0.3.0',
        license: 'ISC',
        meson_version: '>= 0.49.2',
        default_options: [
          'c_std=c99',
          'cpp_std=c++11',
          'default_library=shared'
        ])

pugl_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'pugl' + version_suffix

# Load build tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Enable C++ support if we're building the examples
if get_option('examples')
  add_languages(['cpp'])
  cpp = meson.get_compiler('cpp')
endif

# Enable Objective C support if we're building for MacOS
if host_machine.system() == 'darwin'
  add_languages(['objc'])
  objcc = meson.get_compiler('objc')
endif

# Set ultra strict warnings for developers, if requested
if get_option('strict')
  subdir('meson')

  # C warnings
  c_warnings = all_c_warnings
  if cc.get_id() == 'clang'
    c_warnings += [
      '-Wno-bad-function-cast',
      '-Wno-documentation', # Cairo
      '-Wno-documentation-unknown-command', # Cairo
      '-Wno-float-equal',
      '-Wno-implicit-fallthrough',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unused-macros', # Mac
    ]
  elif cc.get_id() == 'gcc'
    c_warnings += [
      '-Wno-bad-function-cast',
      '-Wno-float-equal',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-pedantic',
      '-Wno-suggest-attribute=const',
      '-Wno-suggest-attribute=malloc',
      '-Wno-suggest-attribute=pure',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsuffixed-float-constants',
    ]
  elif cc.get_id() == 'msvc'
    c_warnings += [
      '/wd4028',  # formal parameter different from declaration
      '/wd4061',  # enumerator in switch is not explicitly handled
      '/wd4191',  # unsafe conversion from type to type
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4706',  # assignment within conditional expression
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4800',  # implicit conversion from int to bool
      '/wd4820',  # padding added after construct
      '/wd4996',  # function or variable may be unsafe
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  endif

  add_project_arguments(cc.get_supported_arguments(c_warnings),
                        language: ['c', 'objc'])

  # C++ warnings
  cpp_warnings = all_cpp_warnings
  if is_variable('cpp')
    if cpp.get_id() == 'clang'
      cpp_warnings += [
        '-Wno-documentation-unknown-command', # Cairo
        '-Wno-old-style-cast',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-switch-enum',
        '-Wno-unused-macros', # Mac
      ]
    elif cpp.get_id() == 'gcc'
      cpp_warnings += [
        '-Wno-effc++',
        '-Wno-inline',
        '-Wno-old-style-cast',
        '-Wno-padded',
        '-Wno-suggest-attribute=const',
        '-Wno-suggest-attribute=malloc',
        '-Wno-suggest-attribute=pure',
        '-Wno-suggest-final-methods',
        '-Wno-switch-default',
        '-Wno-switch-enum',
        '-Wno-unused-const-variable',
        '-Wno-useless-cast',
      ]
    elif cpp.get_id() == 'msvc'
      cpp_warnings += [
        '/wd4061',  # enumerator in switch is not explicitly handled
        '/wd4191',  # unsafe conversion from type to type
        '/wd4355',  # 'this' used in base member initializer list
        '/wd4514',  # unreferenced inline function has been removed
        '/wd4571',  # structured exceptions (SEH) are no longer caught
        '/wd4625',  # copy constructor implicitly deleted
        '/wd4626',  # assignment operator implicitly deleted
        '/wd4706',  # assignment within conditional expression
        '/wd4710',  # function not inlined
        '/wd4711',  # function selected for automatic inline expansion
        '/wd4800',  # implicit conversion from int to bool
        '/wd4820',  # padding added after construct
        '/wd4868',  # compiler may not enforce left-to-right evaluation order
        '/wd4996',  # function or variable may be unsafe
        '/wd5026',  # move constructor implicitly deleted
        '/wd5027',  # move assignment operator implicitly deleted
        '/wd5039',  # potentially throwing function passed to C
        '/wd5045',  # will insert Spectre mitigation for memory load
      ]
    endif

    add_project_arguments(cpp.get_supported_arguments(cpp_warnings),
                          language: ['cpp'])
  endif

  # Objective C warnings
  if is_variable('objcc')
    add_project_arguments(objcc.get_supported_arguments(all_objc_warnings),
                          language: ['objc'])
  endif
endif

# Disable deprecated API which is not used by tests or examples
add_project_arguments(['-DPUGL_DISABLE_DEPRECATED'],
                      language: ['c', 'cpp', 'objc'])

c_headers = [
  'include/pugl/pugl.h',

  'include/pugl/cairo.h',
  'include/pugl/gl.h',
  'include/pugl/stub.h',
  'include/pugl/vulkan.h',
]

c_header_files = files(c_headers)

cpp_headers = [
  'bindings/cxx/include/pugl/pugl.hpp',

  'bindings/cxx/include/pugl/cairo.hpp',
  'bindings/cxx/include/pugl/gl.hpp',
  'bindings/cxx/include/pugl/stub.hpp',
  'bindings/cxx/include/pugl/vulkan.hpp',
]

cpp_header_files = files(cpp_headers)

core_sources = [
  'src/implementation.c'
]

# System libraries
m_dep = cc.find_library('m', required: false)
dl_dep = cc.find_library('dl', required: false)
thread_dep = dependency('threads')

# Cairo (optional backend)
cairo_dep = dependency('cairo',
                       required: get_option('cairo'))

# OpenGL (optional backend)
opengl_dep = dependency('GL',
                        required: get_option('opengl'))

# Vulkan (optional backend)
vulkan_dep = dependency('vulkan',
                        required: get_option('vulkan'))

core_args = []

# MacOS
if host_machine.system() == 'darwin'
  cocoa_dep = dependency('Cocoa', required: false, modules: 'foundation')
  corevideo_dep = dependency('CoreVideo', required: false)

  platform = 'mac'
  platform_sources = ['src/mac.m', 'src/mac_stub.m']
  core_deps = [cocoa_dep, corevideo_dep]
  extension = '.m'

  add_project_arguments(['-Wno-deprecated-declarations'], language: ['objc'])
  add_project_arguments(['-Wno-direct-ivar-access'], language: ['objc'])

  add_project_arguments(['-DGL_SILENCE_DEPRECATION'],
                        language: ['c', 'objc'])

  add_project_link_arguments(['-Wl,-framework,Cocoa'],
                             language: ['c', 'objc'])

# Windows
elif host_machine.system() == 'windows'
  if cpp.get_id() == 'msvc'
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

  platform = 'win'
  platform_sources = ['src/win.c']
  core_deps = []
  extension = '.c'

else # X11
  x11_dep = cc.find_library('X11')

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

  platform = 'x11'
  platform_sources = ['src/x11.c']
  core_deps = [x11_dep, xcursor_dep, xrandr_dep, xext_dep]
  extension = '.c'
endif

# Build core library

core_deps += [m_dep]
core_sources += platform_sources
core_name = 'pugl_@0@@1@'.format(platform, version_suffix)

library_args = ['-DPUGL_INTERNAL']
if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif

  library_type = 'both_libraries'
elif get_option('default_library') == 'shared'
  library_type = 'shared_library'
else
  library_type = 'static_library'
  add_project_arguments(['-DPUGL_STATIC'], language: ['c', 'cpp', 'objc'])
endif

libpugl = build_target(
  core_name, core_sources,
  version: meson.project_version(),
  include_directories: include_directories(['include']),
  c_args: library_args + core_args,
  dependencies: core_deps,
  gnu_symbol_visibility: 'hidden',
  install: true,
  target_type: library_type)

pugl_dep = declare_dependency(link_with: libpugl, dependencies: core_deps)

pkg.generate(libpugl,
             name: 'Pugl',
             filebase: versioned_name,
             subdirs: [versioned_name],
             version: meson.project_version(),
             description: 'Pugl GUI library core')

# Build stub backend

name = 'pugl_' + platform + '_stub' + version_suffix
sources = 'src/' + platform + '_stub' + extension

stub_backend = build_target(
  name, sources,
  version: meson.project_version(),
  include_directories: include_directories(['include']),
  c_args: library_args,
  dependencies: [pugl_dep],
  gnu_symbol_visibility: 'hidden',
  install: true,
  target_type: library_type)

stub_backend_dep = declare_dependency(link_with: stub_backend)

pkg.generate(stub_backend,
             name: 'Pugl Stub',
             filebase: 'pugl-stub-@0@'.format(major_version),
             subdirs: [name],
             version: meson.project_version(),
             description: 'Native window pugl graphics backend')

# Build GL backend
if opengl_dep.found()
  name = 'pugl_' + platform + '_gl' + version_suffix
  sources = 'src/' + platform + '_gl' + extension

  gl_backend = build_target(
    name, sources,
    version: meson.project_version(),
    include_directories: include_directories(['include']),
    c_args: library_args,
    dependencies: [pugl_dep, opengl_dep],
    gnu_symbol_visibility: 'hidden',
    install: true,
    target_type: library_type)

  gl_backend_dep = declare_dependency(link_with: gl_backend,
                                      dependencies: [pugl_dep, opengl_dep])

  pkg.generate(gl_backend,
               name: 'Pugl OpenGL',
               filebase: 'pugl-gl-@0@'.format(major_version),
               subdirs: [name],
               version: meson.project_version(),
               description: 'Pugl GUI library with OpenGL backend')
endif

# Build Cairo backend
if cairo_dep.found()
  name = 'pugl_' + platform + '_cairo' + version_suffix
  sources = ['src/' + platform + '_cairo' + extension,
             'src/' + platform + '_stub' + extension]

  cairo_backend = build_target(
    name, sources,
    version: meson.project_version(),
    include_directories: include_directories(['include']),
    c_args: library_args,
    dependencies: [pugl_dep, cairo_dep, stub_backend_dep],
    gnu_symbol_visibility: 'hidden',
    install: true,
    target_type: library_type)

  cairo_backend_dep = declare_dependency(
    link_with: cairo_backend,
    dependencies: [pugl_dep, cairo_dep, stub_backend_dep])

  pkg.generate(cairo_backend,
               name: 'Pugl Cairo',
               filebase: 'pugl-cairo-@0@'.format(major_version),
               subdirs: [name],
               version: meson.project_version(),
               description: 'Pugl GUI library with Cairo backend')
endif

# Build Vulkan backend
if vulkan_dep.found()
  name = 'pugl_' + platform + '_vulkan' + version_suffix
  sources = ['src/' + platform + '_vulkan' + extension,
             'src/' + platform + '_stub' + extension]

  vulkan_deps = [pugl_dep, vulkan_dep, dl_dep]
  vulkan_c_args = library_args
  vulkan_link_args = []
  if platform == 'mac'
    metal_dep = dependency('Metal', modules: 'foundation')
    quartzcore_dep = dependency('QuartzCore', modules: 'foundation')

    vulkan_deps += [metal_dep, quartzcore_dep]
  endif

  vulkan_backend = build_target(
    name, sources,
    version: meson.project_version(),
    include_directories: include_directories(['include']),
    c_args: library_args,
    dependencies: vulkan_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    target_type: library_type)

  vulkan_backend_dep = declare_dependency(
    link_with: vulkan_backend,
    dependencies: [pugl_dep, vulkan_dep, thread_dep])

  pkg.generate(vulkan_backend,
               name: 'Pugl Vulkan',
               filebase: 'pugl-vulkan-@0@'.format(major_version),
               subdirs: [name],
               version: meson.project_version(),
               description: 'Pugl GUI library with Vulkan backend')
endif

install_headers(c_headers, subdir: versioned_name / 'pugl')
install_headers(cpp_headers, subdir: 'puglxx' + version_suffix)

if not get_option('docs').disabled()
  subdir('doc')
else
  build_docs = false
endif

if get_option('examples')
  subdir('examples')
endif

if get_option('tests')
  subdir('test')
endif

if meson.version().version_compare('>=0.53.0')
  summary('Platform', platform)
  summary('Cairo backend', cairo_dep.found(), bool_yn: true)
  summary('OpenGL backend', opengl_dep.found(), bool_yn: true)
  summary('Vulkan backend', vulkan_dep.found(), bool_yn: true)
  summary('Tests', get_option('tests'), bool_yn: true)
  summary('Examples', get_option('examples'), bool_yn: true)
  summary('Documentation', build_docs, bool_yn: true)

  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))

  if get_option('examples')
    summary('Executables', get_option('prefix') / get_option('bindir'))
  endif
endif
