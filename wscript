#!/usr/bin/env python

import sys
from waflib import Options, TaskGen
from waflib.extras import autowaf

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
         'grab-focus': 'work around keyboard issues by grabbing focus'})

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
        conf.define('PUGL_HAVE_GL', 1)

    if not Options.options.no_cairo:
        autowaf.check_pkg(conf, 'cairo',
                          uselib_store    = 'CAIRO',
                          atleast_version = '1.0.0',
                          mandatory       = False)
        if conf.is_defined('HAVE_CAIRO'):
            conf.define('PUGL_HAVE_CAIRO', 1)

    if Options.options.log:
        conf.define('PUGL_VERBOSE', 1)

    # Shared library building is broken on win32 for some reason
    conf.env['BUILD_TESTS']  = Options.options.test
    conf.env['BUILD_SHARED'] = conf.env.TARGET_PLATFORM != 'win32'
    conf.env['BUILD_STATIC'] = (Options.options.test or Options.options.static)

    autowaf.set_lib_env(conf, 'pugl', PUGL_VERSION)
    conf.write_config_header('pugl_config.h', remove=False)

    autowaf.display_summary(
        conf,
        {"Build static library":   bool(conf.env['BUILD_STATIC']),
         "Build shared library":   bool(conf.env['BUILD_SHARED']),
         "OpenGL support":         conf.is_defined('HAVE_GL'),
         "Cairo support":          conf.is_defined('HAVE_CAIRO'),
         "Verbose console output": conf.is_defined('PUGL_VERBOSE')})

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/pugl-%s/pugl' % PUGL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.hpp'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'PUGL', PUGL_VERSION, PUGL_MAJOR_VERSION, [],
                     {'PUGL_MAJOR_VERSION': PUGL_MAJOR_VERSION})

    libflags  = ['-fvisibility=hidden']
    framework = []
    libs      = []
    if bld.env.TARGET_PLATFORM == 'win32':
        lang       = 'cxx'
        lib_source = ['pugl/pugl_win.cpp']
        libs       = ['opengl32', 'gdi32', 'user32']
    elif bld.env.TARGET_PLATFORM == 'darwin':
        lang       = 'c'  # Objective C, actually
        lib_source = ['pugl/pugl_osx.m']
        framework  = ['Cocoa', 'OpenGL']
    else:
        lang       = 'c'
        lib_source = ['pugl/pugl_x11.c']
        libs       = ['X11']
        if bld.is_defined('HAVE_GL'):
            lib_source += ['pugl/pugl_x11_gl.c']
            libs       += ['GL']
        if bld.is_defined('HAVE_CAIRO'):
            lib_source += ['pugl/pugl_x11_cairo.c']
    if bld.env['MSVC_COMPILER']:
        libflags = []
    else:
        libs += ['m']

    common = {
        'framework': framework,
        'includes':  ['.', './src'],
        'uselib':    ['CAIRO'],
    }

    lib_common = common.copy()
    lib_common.update({
        'export_includes': ['.'],
        'install_path':    '${LIBDIR}',
        'lib':             libs,
        'source':          lib_source,
        'target':          'pugl-%s' % PUGL_MAJOR_VERSION,
        'vnum':            PUGL_VERSION,
    })

    # Shared Library
    if bld.env['BUILD_SHARED']:
        bld(features = '%s %sshlib' % (lang, lang),
            name     = 'libpugl',
            cflags   = libflags + ['-DPUGL_SHARED', '-DPUGL_INTERNAL'],
            **lib_common)

    # Static library
    if bld.env['BUILD_STATIC']:
        bld(features = '%s %sstlib' % (lang, lang),
            name     = 'libpugl_static',
            cflags   = ['-DPUGL_INTERNAL'],
            **lib_common)


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
            bld(features     = 'c cprogram',
                source       = '%s.c' % prog,
                use          = 'libpugl_static',
                lib          = test_libs,
                target       = prog,
                install_path = '',
                cflags       = test_cflags,
                **common)


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
    import subprocess

    subprocess.call("flake8 wscript --ignore E221,W504,E302,E251,E241",
                    shell=True)

    cmd = ("clang-tidy -p=. -header-filter=.* -checks=\"*," +
           "-clang-analyzer-alpha.*," +
           "-google-readability-todo," +
           "-llvm-header-guard," +
           "-misc-unused-parameters," +
           "-hicpp-signed-bitwise," +  # FIXME?
           "-readability-else-after-return\" " +
           "../pugl/*.c ../*.c")
    subprocess.call(cmd, cwd='build', shell=True)

# Alias .m files to be compiled like .c files, gcc will do the right thing.
@TaskGen.extension('.m')
def m_hook(self, node):
    return self.create_compiled_task('c', node)
