#!/usr/bin/env python

import os
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

def options(ctx):
    ctx.load('compiler_c')

    opts = ctx.configuration_options()
    opts.add_option('--target', default=None, dest='target',
                   help='target platform (e.g. "win32" or "darwin")')

    ctx.add_flags(
        opts,
        {'no-gl':      'do not build OpenGL support',
         'no-cairo':   'do not build Cairo support',
         'static':     'build static library',
         'log':        'print GL information to console',
         'grab-focus': 'work around keyboard issues by grabbing focus'})

def configure(conf):
    conf.env.TARGET_PLATFORM = Options.options.target or sys.platform
    conf.load('compiler_c', cache=True)
    conf.load('autowaf', cache=True)

    if conf.env.TARGET_PLATFORM == 'win32':
        if conf.env.MSVC_COMPILER:
            conf.env.append_unique('CFLAGS', ['/wd4191'])
    elif conf.env.TARGET_PLATFORM == 'darwin':
        conf.env.append_unique('CFLAGS', ['-Wno-deprecated-declarations'])

    if Options.options.strict and not conf.env.MSVC_COMPILER:
        conf.env.append_value('CFLAGS', ['-Wunused-parameter'])
        conf.env.append_value('CXXFLAGS', ['-Wunused-parameter'])

    autowaf.set_c_lang(conf, 'c99')

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
    conf.env.update({
        'BUILD_SHARED': conf.env.TARGET_PLATFORM != 'win32',
        'BUILD_STATIC': conf.env['BUILD_TESTS'] or Options.options.static})

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
        lib_source = ['pugl/pugl_win.c', 'pugl/pugl_win_gl.c']
        libs       = ['opengl32', 'gdi32', 'user32']
    elif bld.env.TARGET_PLATFORM == 'darwin':
        lib_source = ['pugl/pugl_osx.m']
        framework  = ['Cocoa', 'OpenGL']
    else:
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
        bld(features = 'c cshlib',
            name     = 'libpugl',
            cflags   = libflags + ['-DPUGL_SHARED', '-DPUGL_INTERNAL'],
            **lib_common)

    # Static library
    if bld.env['BUILD_STATIC']:
        bld(features = 'c cstlib',
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
            if bld.env.TARGET_PLATFORM == 'darwin':
                target = '{0}.app/Contents/MacOS/{0}'.format(prog)

                bld(features     = 'subst',
                    source       = 'resources/Info.plist.in',
                    target       = '{}.app/Contents/Info.plist'.format(prog),
                    install_path = '',
                    NAME         = prog)
            else:
                target = prog

            bld(features     = 'c cprogram',
                source       = 'test/%s.c' % prog,
                use          = 'libpugl_static',
                lib          = test_libs,
                target       = target,
                install_path = '',
                cflags       = test_cflags,
                **common)

    if bld.env['DOCS']:
        bld(features     = 'subst',
            source       = 'Doxyfile.in',
            target       = 'Doxyfile',
            install_path = '',
            name         = 'Doxyfile',
            PUGL_VERSION = PUGL_VERSION,
            PUGL_SRCDIR  = os.path.abspath(bld.path.srcpath()))

        bld(features = 'doxygen',
            doxyfile = 'Doxyfile')

def test(tst):
    pass

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
