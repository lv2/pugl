#!/usr/bin/env python

import os
import sys

from waflib import Logs, Options, TaskGen
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
        {'all-headers': 'install complete header implementation',
         'no-gl':       'do not build OpenGL support',
         'no-cairo':    'do not build Cairo support',
         'no-static':   'do not build static library',
         'no-shared':   'do not build shared library',
         'log':         'print GL information to console',
         'grab-focus':  'work around keyboard issues by grabbing focus'})


def configure(conf):
    conf.env.ALL_HEADERS     = Options.options.all_headers
    conf.env.TARGET_PLATFORM = Options.options.target or sys.platform
    conf.load('compiler_c', cache=True)
    conf.load('autowaf', cache=True)

    if conf.env.TARGET_PLATFORM == 'win32':
        if conf.env.MSVC_COMPILER:
            conf.env.append_unique('CFLAGS', ['/wd4191'])
    elif conf.env.TARGET_PLATFORM == 'darwin':
        conf.env.append_unique('CFLAGS', ['-Wno-deprecated-declarations'])

    if not conf.env.MSVC_COMPILER:
        conf.env.append_value('LINKFLAGS', ['-fvisibility=hidden'])
        for f in ('CFLAGS', 'CXXFLAGS'):
            conf.env.append_value(f, ['-fvisibility=hidden'])
            if Options.options.strict:
                conf.env.append_value(f, ['-Wunused-parameter'])

    autowaf.set_c_lang(conf, 'c99')

    if not Options.options.no_gl:
        # TODO: Portable check for OpenGL
        conf.define('HAVE_GL', 1)
        conf.define('PUGL_HAVE_GL', 1)

    conf.check(features='c cshlib', lib='m', uselib_store='M', mandatory=False)

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
        'BUILD_SHARED': not Options.options.no_shared,
        'BUILD_STATIC': conf.env.BUILD_TESTS or not Options.options.no_static})

    autowaf.set_lib_env(conf, 'pugl', PUGL_VERSION)
    conf.write_config_header('pugl_config.h', remove=False)

    autowaf.display_summary(
        conf,
        {"Build static library":   bool(conf.env.BUILD_STATIC),
         "Build shared library":   bool(conf.env.BUILD_SHARED),
         "OpenGL support":         conf.is_defined('HAVE_GL'),
         "Cairo support":          conf.is_defined('HAVE_CAIRO'),
         "Verbose console output": conf.is_defined('PUGL_VERBOSE')})


def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/pugl-%s/pugl' % PUGL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.hpp'))
    if bld.env.ALL_HEADERS:
        detaildir = os.path.join(includedir, 'detail')
        bld.install_files(detaildir, bld.path.ant_glob('pugl/detail/*.h'))
        bld.install_files(detaildir, bld.path.ant_glob('pugl/detail/*.c'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'PUGL', PUGL_VERSION, PUGL_MAJOR_VERSION, [],
                     {'PUGL_MAJOR_VERSION': PUGL_MAJOR_VERSION})

    libflags   = []
    framework  = []
    libs       = []
    lib_source = ['pugl/detail/implementation.c']
    if bld.env.TARGET_PLATFORM == 'win32':
        lib_source += ['pugl/detail/win.c']
        libs        = ['gdi32', 'user32']
        if bld.is_defined('HAVE_GL'):
            lib_source += ['pugl/detail/win_gl.c']
            libs       += ['opengl32']
        if bld.is_defined('HAVE_CAIRO'):
            lib_source += ['pugl/detail/win_cairo.c']
            libs       += ['cairo']
    elif bld.env.TARGET_PLATFORM == 'darwin':
        lib_source += ['pugl/detail/mac.m']
        framework   = ['Cocoa']
        if bld.is_defined('HAVE_GL'):
            lib_source += ['pugl/detail/mac_gl.m']
            framework  += ['OpenGL']
        if bld.is_defined('HAVE_CAIRO'):
            lib_source += ['pugl/detail/mac_cairo.m']
    else:
        lib_source += ['pugl/detail/x11.c']
        libs        = ['X11']
        if bld.is_defined('HAVE_GL'):
            lib_source += ['pugl/detail/x11_gl.c']
            libs       += ['GL']
        if bld.is_defined('HAVE_CAIRO'):
            lib_source += ['pugl/detail/x11_cairo.c']

    common = {
        'framework': framework,
        'includes':  ['.'],
    }

    lib_common = common.copy()
    lib_common.update({
        'export_includes': ['.'],
        'install_path':    '${LIBDIR}',
        'lib':             libs,
        'uselib':          ['CAIRO'],
        'source':          lib_source,
        'target':          'pugl-%s' % PUGL_MAJOR_VERSION,
        'vnum':            PUGL_VERSION,
    })

    # Shared Library
    if bld.env.BUILD_SHARED:
        bld(features = 'c cshlib',
            name     = 'libpugl',
            cflags   = libflags + ['-DPUGL_SHARED', '-DPUGL_INTERNAL'],
            **lib_common)

    # Static library
    if bld.env.BUILD_STATIC:
        bld(features = 'c cstlib',
            name     = 'libpugl_static',
            cflags   = ['-DPUGL_INTERNAL'],
            **lib_common)

    if bld.env.BUILD_TESTS:
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
                uselib       = ['CAIRO', 'M'],
                target       = target,
                install_path = '',
                cflags       = test_cflags,
                **common)

    if bld.env.DOCS:
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

    subprocess.call("flake8 wscript --ignore E221,W504,E251,E241",
                    shell=True)

    cmd = ("clang-tidy -p=. -header-filter=.* -checks=\"*," +
           "-bugprone-suspicious-string-compare," +
           "-clang-analyzer-alpha.*," +
           "-cppcoreguidelines-avoid-magic-numbers," +
           "-google-readability-todo," +
           "-hicpp-multiway-paths-covered," +
           "-hicpp-signed-bitwise," +
           "-hicpp-uppercase-literal-suffix," +
           "-llvm-header-guard," +
           "-misc-misplaced-const," +
           "-misc-unused-parameters," +
           "-readability-else-after-return," +
           "-readability-magic-numbers," +
           "-readability-uppercase-literal-suffix\" " +
           "../pugl/detail/*.c")

    subprocess.call(cmd, cwd='build', shell=True)

    try:
        subprocess.call(['iwyu_tool.py', '-o', 'clang', '-p', 'build'])
    except Exception:
        Logs.warn('Failed to call iwyu_tool.py')

# Alias .m files to be compiled like .c files, gcc will do the right thing.
@TaskGen.extension('.m')
def m_hook(self, node):
    return self.create_compiled_task('c', node)
