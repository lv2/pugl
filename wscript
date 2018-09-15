#!/usr/bin/env python
import subprocess
import sys
import waflib.Options as Options
import waflib.extras.autowaf as autowaf

# Library and package version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
PUGL_VERSION       = '0.2.0'
PUGL_MAJOR_VERSION = '0'

# Mandatory waf variables
APPNAME = 'pugl'        # Package name for waf dist
VERSION = PUGL_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    autowaf.set_options(opt)

    opt = opt.get_option_group('Configuration options')
    opt.add_option('--target', default=None, dest='target',
                   help='target platform (e.g. "win32" or "darwin")')

    autowaf.add_flags(
        opt,
        {'no-gl':      'do not build OpenGL support',
         'no-cairo':   'do not build Cairo support',
         'static':     'build static library',
         'test':       'build test programs',
         'log':        'print GL information to console',
         'grab-focus': 'work around reparent keyboard issues by grabbing focus'})

def configure(conf):
    conf.env.TARGET_PLATFORM = Options.options.target or sys.platform
    conf.load('compiler_c')
    if conf.env.TARGET_PLATFORM == 'win32':
        conf.load('compiler_cxx')

    autowaf.configure(conf)
    autowaf.set_c_lang(conf, 'c99')
    autowaf.display_header('Pugl Configuration')

    if not Options.options.no_gl:
        # TODO: Portable check for OpenGL
        conf.define('HAVE_GL', 1)
        autowaf.define(conf, 'PUGL_HAVE_GL', 1)

    if not Options.options.no_cairo:
        autowaf.check_pkg(conf, 'cairo',
                          uselib_store    = 'CAIRO',
                          atleast_version = '1.0.0',
                          mandatory       = False)
        if conf.is_defined('HAVE_CAIRO'):
            autowaf.define(conf, 'PUGL_HAVE_CAIRO', 1)

    if Options.options.log:
        autowaf.define(conf, 'PUGL_VERBOSE', 1)

    # Shared library building is broken on win32 for some reason
    conf.env['BUILD_TESTS']  = Options.options.test
    conf.env['BUILD_SHARED'] = conf.env.TARGET_PLATFORM != 'win32'
    conf.env['BUILD_STATIC'] = (Options.options.test or Options.options.static)

    autowaf.define(conf, 'PUGL_VERSION', PUGL_VERSION)
    conf.write_config_header('pugl_config.h', remove=False)

    conf.env['INCLUDES_PUGL'] = ['%s/pugl-%s' % (conf.env['INCLUDEDIR'],
                                                 PUGL_MAJOR_VERSION)]
    conf.env['LIBPATH_PUGL'] = [conf.env['LIBDIR']]
    conf.env['LIB_PUGL'] = ['pugl-%s' % PUGL_MAJOR_VERSION];

    autowaf.display_msg(conf, "OpenGL support", conf.is_defined('HAVE_GL'))
    autowaf.display_msg(conf, "Cairo support", conf.is_defined('HAVE_CAIRO'))
    autowaf.display_msg(conf, "Verbose console output", conf.is_defined('PUGL_VERBOSE'))
    autowaf.display_msg(conf, "Static library", str(conf.env['BUILD_STATIC']))
    autowaf.display_msg(conf, "Unit tests", str(conf.env['BUILD_TESTS']))
    print('')

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/pugl-%s/pugl' % PUGL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.hpp'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'PUGL', PUGL_VERSION, PUGL_MAJOR_VERSION, [],
                     {'PUGL_MAJOR_VERSION' : PUGL_MAJOR_VERSION})

    libflags  = [ '-fvisibility=hidden' ]
    framework = []
    libs      = []
    if bld.env.TARGET_PLATFORM == 'win32':
        lang       = 'cxx'
        lib_source = ['pugl/pugl_win.cpp']
        libs       = ['opengl32', 'gdi32', 'user32']
        defines    = []
    elif bld.env.TARGET_PLATFORM == 'darwin':
        lang       = 'c'  # Objective C, actually
        lib_source = ['pugl/pugl_osx.m']
        framework  = ['Cocoa', 'OpenGL']
        defines    = []
    else:
        lang       = 'c'
        lib_source = ['pugl/pugl_x11.c']
        libs       = ['X11']
        defines    = []
        if bld.is_defined('HAVE_GL'):
            libs += ['GL']
    if bld.env['MSVC_COMPILER']:
        libflags = []
    else:
        libs += ['m']

    # Shared Library
    if bld.env['BUILD_SHARED']:
        obj = bld(features        = '%s %sshlib' % (lang, lang),
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  lib             = libs,
                  uselib          = ['CAIRO'],
                  framework       = framework,
                  name            = 'libpugl',
                  target          = 'pugl-%s' % PUGL_MAJOR_VERSION,
                  vnum            = PUGL_VERSION,
                  install_path    = '${LIBDIR}',
                  defines         = defines,
                  cflags          = libflags + [ '-DPUGL_SHARED',
                                                 '-DPUGL_INTERNAL' ])

    # Static library
    if bld.env['BUILD_STATIC']:
        obj = bld(features        = '%s %sstlib' % (lang, lang),
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  lib             = libs,
                  uselib          = ['CAIRO'],
                  framework       = framework,
                  name            = 'libpugl_static',
                  target          = 'pugl-%s' % PUGL_MAJOR_VERSION,
                  vnum            = PUGL_VERSION,
                  install_path    = '${LIBDIR}',
                  defines         = defines,
                  cflags          = ['-DPUGL_INTERNAL'])

    if bld.env['BUILD_TESTS']:
        test_libs   = libs
        test_cflags = ['']

        # Test programs
        progs = []
        if bld.is_defined('HAVE_GL'):
            progs += ['pugl_test']
        if bld.is_defined('HAVE_CAIRO'):
            progs += ['pugl_cairo_test']

        for prog in progs:
            obj = bld(features     = 'c cprogram',
                      source       = '%s.c' % prog,
                      includes     = ['.', './src'],
                      use          = 'libpugl_static',
                      lib          = test_libs,
                      uselib       = ['CAIRO'],
                      framework    = framework,
                      target       = prog,
                      install_path = '',
                      defines      = defines,
                      cflags       = test_cflags)

    if bld.env['DOCS']:
        bld(features     = 'subst',
            source       = 'Doxyfile.in',
            target       = 'Doxyfile',
            install_path = '',
            name         = 'Doxyfile',
            PUGL_VERSION = PUGL_VERSION)

        bld(features = 'doxygen',
            doxyfile = 'Doxyfile')

def lint(ctx):
    "checks code for style issues"
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include src/* pugl/*', shell=True)

# Alias .m files to be compiled the same as .c files, gcc will do the right thing.
from waflib import TaskGen
@TaskGen.extension('.m')
def m_hook(self, node):
    return self.create_compiled_task('c', node)
