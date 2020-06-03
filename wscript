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
    ctx.load('compiler_cxx')

    opts = ctx.configuration_options()
    opts.add_option('--target', default=None, dest='target',
                    help='target platform (e.g. "win32" or "darwin")')

    ctx.add_flags(
        opts,
        {'all-headers': 'install complete header implementation',
         'no-gl':       'do not build OpenGL support',
         'no-cairo':    'do not build Cairo support',
         'no-static':   'do not build static library',
         'no-shared':   'do not build shared library'})

    ctx.get_option_group('Test options').add_option(
        '--gui-tests', action='store_true', help='Run GUI tests')


def configure(conf):
    conf.load('compiler_c', cache=True)
    try:
        conf.load('compiler_cxx', cache=True)
    except Exception:
        pass

    conf.load('autowaf', cache=True)
    autowaf.set_c_lang(conf, 'c99')
    if 'COMPILER_CXX' in conf.env:
        autowaf.set_cxx_lang(conf, 'c++11')

    conf.env.ALL_HEADERS     = Options.options.all_headers
    conf.env.TARGET_PLATFORM = Options.options.target or sys.platform
    platform                 = conf.env.TARGET_PLATFORM

    def append_cflags(flags):
        conf.env.append_value('CFLAGS', flags)
        conf.env.append_value('CXXFLAGS', flags)

    if conf.env.MSVC_COMPILER:
        append_cflags(['/wd4191', '/wd4355'])
    else:
        if Options.options.ultra_strict:
            append_cflags(['-Wunused-parameter', '-Wno-pedantic'])

    if conf.env.TARGET_PLATFORM == 'darwin':
        append_cflags(['-DGL_SILENCE_DEPRECATION',
                       '-Wno-deprecated-declarations'])

    if Options.options.ultra_strict and 'clang' in conf.env.CC:
        for var in ['CFLAGS', 'CXXFLAGS']:
            flags = conf.env[var]
            conf.env[var] = [f for f in flags if not f.startswith('-W')]
            conf.env.append_value(var, [
                '-Weverything',
                '-Wno-bad-function-cast',
                '-Wno-float-equal',
                '-Wno-format-nonliteral',
                '-Wno-padded',
                '-Wno-reserved-id-macro',
                '-Wno-switch-enum',
            ])

        conf.env.append_value('CXXFLAGS', [
            '-Wno-c++98-compat',
            '-Wno-c++98-compat-pedantic',
            '-Wno-documentation-unknown-command',
            '-Wno-old-style-cast',
        ])

    conf.check_cc(lib='m', uselib_store='M', mandatory=False)
    conf.check_cc(lib='dl', uselib_store='DL', mandatory=False)

    # Check for "native" platform dependencies
    conf.env.HAVE_GL = False
    if platform == 'darwin':
        conf.check_cc(framework_name='Cocoa', framework='Cocoa',
                      uselib_store='COCOA')
        if not Options.options.no_gl:
            conf.check_cc(framework_name='OpenGL', uselib_store='GL',
                          mandatory=False)
            conf.env.HAVE_GL = conf.env.FRAMEWORK_GL

    elif platform == 'win32':
        conf.check_cc(lib='gdi32', uselib_store='GDI32')
        conf.check_cc(lib='user32', uselib_store='USER32')
        if not Options.options.no_gl:
            conf.check_cc(lib='opengl32', uselib_store='GL', mandatory=False)
            conf.env.HAVE_GL = conf.env.LIB_GL

    else:
        conf.check_cc(lib='X11', uselib_store='X11')

        xsync_fragment = """#include <X11/Xlib.h>
            #include <X11/extensions/sync.h>
            int main(void) { XSyncQueryExtension(0, 0, 0); return 0; }"""
        if conf.check_cc(fragment=xsync_fragment,
                         uselib_store='XSYNC',
                         lib='Xext',
                         mandatory=False,
                         msg='Checking for function XSyncQueryExtension'):
            conf.define('HAVE_XSYNC', 1)

        if conf.check_cc(lib='Xcursor', uselib_store='XCURSOR'):
            conf.define('HAVE_XCURSOR', 1)

        if not Options.options.no_gl:
            glx_fragment = """#include <GL/glx.h>
                int main(void) { glXSwapBuffers(0, 0); return 0; }"""

            conf.check_cc(lib='GLX', uselib_store='GLX', mandatory=False)
            conf.check_cc(lib='GL', uselib_store='GL', mandatory=False)
            conf.check_cc(fragment=glx_fragment,
                          lib='GLX' if conf.env.LIB_GLX else 'GL',
                          mandatory=False,
                          msg='Checking for GLX')
            conf.env.HAVE_GL = conf.env.LIB_GL or conf.env.LIB_GLX

    # Check for Cairo via pkg-config
    if not Options.options.no_cairo:
        autowaf.check_pkg(conf, 'cairo',
                          uselib_store    = 'CAIRO',
                          atleast_version = '1.0.0',
                          system          = True,
                          mandatory       = False)

    conf.env.update({
        'BUILD_SHARED': not Options.options.no_shared,
        'BUILD_STATIC': conf.env.BUILD_TESTS or not Options.options.no_static})

    if conf.env.TARGET_PLATFORM == 'win32':
        conf.env.PUGL_PLATFORM = 'win'
    elif conf.env.TARGET_PLATFORM == 'darwin':
        conf.env.PUGL_PLATFORM = 'mac'
    else:
        conf.env.PUGL_PLATFORM = 'x11'

    autowaf.set_lib_env(conf, 'pugl', PUGL_VERSION,
                        lib='pugl_' + conf.env.PUGL_PLATFORM)

    autowaf.set_lib_env(conf, 'pugl_gl', PUGL_VERSION,
                        lib='pugl_%s_gl' % conf.env.PUGL_PLATFORM)

    autowaf.display_summary(
        conf,
        {"Build static library":   bool(conf.env.BUILD_STATIC),
         "Build shared library":   bool(conf.env.BUILD_SHARED),
         "OpenGL support":         bool(conf.env.HAVE_GL),
         "Cairo support":          bool(conf.env.HAVE_CAIRO)})


def _build_pc_file(bld, name, desc, target, libname, deps={}, requires=[]):
    "Builds a pkg-config file for a library"
    env = bld.env
    prefix = env.PREFIX
    xprefix = os.path.dirname(env.LIBDIR)

    uselib   = deps.get('uselib', [])
    pkg_deps = [l for l in uselib if 'PKG_' + l.lower() in env]
    lib_deps = [l for l in uselib if 'PKG_' + l.lower() not in env]

    link_flags = [env.LIB_ST % l for l in (deps.get('lib', []) + [libname])]
    for l in lib_deps:
        link_flags += [env.LIB_ST % l for l in env['LIB_' + l]]
    for f in deps.get('framework', []):
        link_flags += ['-framework', f]

    bld(features='subst',
        source='pugl.pc.in',
        target='%s-%s.pc' % (target, PUGL_MAJOR_VERSION),
        install_path=os.path.join(env.LIBDIR, 'pkgconfig'),

        PREFIX=prefix,
        EXEC_PREFIX='${prefix}' if xprefix == prefix else xprefix,
        LIBDIR='${exec_prefix}/' + os.path.basename(env.LIBDIR),
        INCLUDEDIR=env.INCLUDEDIR.replace(prefix, '${prefix}', 1),

        NAME=name,
        DESCRIPTION=desc,
        PUGL_MAJOR_VERSION=PUGL_MAJOR_VERSION,
        REQUIRES=' '.join(requires + [p.lower() for p in pkg_deps]),
        LIBS=' '.join(link_flags))


tests = ['redisplay', 'show_hide', 'update', 'timer']


def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/pugl-%s/pugl' % PUGL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.hpp'))
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.ipp'))
    if bld.env.ALL_HEADERS:
        detaildir = os.path.join(includedir, 'detail')
        bld.install_files(detaildir, bld.path.ant_glob('pugl/detail/*.h'))
        bld.install_files(detaildir, bld.path.ant_glob('pugl/detail/*.c'))

    # Library dependencies of pugl libraries (for building examples)
    deps = {}

    def build_pugl_lib(name, **kwargs):
        deps[name] = {}
        for k in ('lib', 'framework', 'uselib'):
            deps[name][k] = kwargs.get(k, [])

        args = kwargs.copy()
        args.update({'includes':        ['.'],
                     'export_includes': ['.'],
                     'install_path':    '${LIBDIR}',
                     'vnum':            PUGL_VERSION})

        flags = [] if bld.env.MSVC_COMPILER else ['-fPIC', '-fvisibility=hidden']

        if bld.env.BUILD_SHARED:
            bld(features  = 'c cshlib',
                name      = name,
                target    = 'pugl_%s-%s' % (name, PUGL_MAJOR_VERSION),
                defines   = ['PUGL_INTERNAL', 'PUGL_SHARED'],
                cflags    = flags,
                linkflags = flags,
                **args)

        if bld.env.BUILD_STATIC:
            bld(features  = 'c cstlib',
                name      = 'pugl_%s_static' % name,
                target    = 'pugl_%s-%s' % (name, PUGL_MAJOR_VERSION),
                defines   = ['PUGL_INTERNAL', 'PUGL_DISABLE_DEPRECATED'],
                cflags    = flags,
                linkflags = flags,
                **args)

    def build_platform(platform, **kwargs):
        build_pugl_lib(platform, **kwargs)
        _build_pc_file(bld, 'Pugl', 'Pugl GUI library core',
                       'pugl', 'pugl_%s' % platform,
                       deps=kwargs)

    def build_backend(platform, backend, **kwargs):
        name  = '%s_%s' % (platform, backend)
        label = 'OpenGL' if backend == 'gl' else backend.title()
        build_pugl_lib(name, **kwargs)
        _build_pc_file(bld, 'Pugl %s' % label,
                       'Pugl GUI library with %s backend' % label,
                       'pugl-%s' % backend, 'pugl_' + name,
                       deps=kwargs,
                       requires=['pugl-%s' % PUGL_MAJOR_VERSION])

    lib_source = ['pugl/detail/implementation.c']
    if bld.env.TARGET_PLATFORM == 'win32':
        platform = 'win'
        build_platform('win',
                       uselib=['GDI32', 'USER32'],
                       source=lib_source + ['pugl/detail/win.c'])

        if bld.env.HAVE_GL:
            build_backend('win', 'gl',
                          uselib=['GDI32', 'USER32', 'GL'],
                          source=['pugl/detail/win_gl.c'])

        if bld.env.HAVE_CAIRO:
            build_backend('win', 'cairo',
                          uselib=['CAIRO', 'GDI32', 'USER32'],
                          source=['pugl/detail/win_cairo.c'])

    elif bld.env.TARGET_PLATFORM == 'darwin':
        platform = 'mac'
        build_platform('mac',
                       framework=['Cocoa'],
                       source=lib_source + ['pugl/detail/mac.m'])

        build_backend('mac', 'stub',
                      framework=['Cocoa'],
                      source=['pugl/detail/mac_stub.m'])

        if bld.env.HAVE_GL:
            build_backend('mac', 'gl',
                          framework=['Cocoa', 'OpenGL'],
                          source=['pugl/detail/mac_gl.m'])

        if bld.env.HAVE_CAIRO:
            build_backend('mac', 'cairo',
                          framework=['Cocoa'],
                          uselib=['CAIRO'],
                          source=['pugl/detail/mac_cairo.m'])
    else:
        platform = 'x11'
        build_platform('x11',
                       uselib=['M', 'X11', 'XSYNC', 'XCURSOR'],
                       source=lib_source + ['pugl/detail/x11.c'])

        if bld.env.HAVE_GL:
            glx_lib = 'GLX' if bld.env.LIB_GLX else 'GL'
            build_backend('x11', 'gl',
                          uselib=[glx_lib, 'X11'],
                          source=['pugl/detail/x11_gl.c'])

        if bld.env.HAVE_CAIRO:
            build_backend('x11', 'cairo',
                          uselib=['CAIRO', 'X11'],
                          source=['pugl/detail/x11_cairo.c'])

    def build_example(prog, source, platform, backend, **kwargs):
        lang = 'cxx' if source[0].endswith('.cpp') else 'c'

        use = ['pugl_%s_static' % platform,
               'pugl_%s_%s_static' % (platform, backend)]

        target = prog
        if bld.env.TARGET_PLATFORM == 'darwin':
            target = '{0}.app/Contents/MacOS/{0}'.format(prog)

            bld(features     = 'subst',
                source       = 'resources/Info.plist.in',
                target       = '{}.app/Contents/Info.plist'.format(prog),
                install_path = '',
                NAME         = prog)

        backend_lib = platform + '_' + backend
        for k in ('lib', 'framework', 'uselib'):
            kwargs.update({k: (kwargs.get(k, []) +
                               deps.get(platform, {}).get(k, []) +
                               deps.get(backend_lib, {}).get(k, []))})

        bld(features     = '%s %sprogram' % (lang, lang),
            source       = source,
            target       = target,
            use          = use,
            install_path = '',
            **kwargs)

    if bld.env.BUILD_TESTS:
        for s in ('rect.vert', 'rect.frag'):
            # Copy shaders to build directory for example programs
            bld(features = 'subst',
                is_copy  = True,
                source   = 'shaders/%s' % s,
                target   = 'shaders/%s' % s)

        if bld.env.HAVE_GL:
            glad_cflags = ['-Wno-pedantic'] if not bld.env.MSVC_COMPILER else []
            build_example('pugl_embed_demo', ['examples/pugl_embed_demo.c'],
                          platform, 'gl', uselib=['GL', 'M'])
            build_example('pugl_window_demo', ['examples/pugl_window_demo.c'],
                          platform, 'gl', uselib=['GL', 'M'])
            build_example('pugl_cursors_demo', ['examples/pugl_cursors_demo.c'],
                          platform, 'gl', uselib=['GL', 'M'])
            build_example('pugl_print_events',
                          ['examples/pugl_print_events.c'],
                          platform, 'stub')
            build_example('pugl_shader_demo',
                          ['examples/pugl_shader_demo.c',
                           'examples/glad/glad.c'],
                          platform, 'gl',
                          cflags=glad_cflags,
                          uselib=['DL', 'GL', 'M'])

        if bld.env.HAVE_CAIRO:
            build_example('pugl_cairo_demo', ['examples/pugl_cairo_demo.c'],
                          platform, 'cairo',
                          uselib=['M', 'CAIRO'])

        for test in tests:
            bld(features     = 'c cprogram',
                source       = 'test/test_%s.c' % test,
                target       = 'test/test_%s' % test,
                install_path = '',
                use          = ['pugl_%s_static' % platform,
                                'pugl_%s_stub_static' % platform],
                uselib       = deps[platform]['uselib'] + ['CAIRO'])

        if bld.env.CXX and bld.env.HAVE_GL:
            build_example('pugl_cxx_demo', ['examples/pugl_cxx_demo.cpp'],
                          platform, 'gl',
                          defines=['PUGL_DISABLE_DEPRECATED'],
                          uselib=['GL', 'M'])

    if bld.env.DOCS:
        autowaf.build_dox(bld, 'PUGL', PUGL_VERSION, top, out)


def test(tst):
    if tst.options.gui_tests:
        with tst.group('gui') as check:
            for test in tests:
                check(['test/test_%s' % test])


def lint(ctx):
    "checks code for style issues"
    import json
    import subprocess

    subprocess.call("flake8 wscript --ignore E221,W504,E251,E241",
                    shell=True)

    with open('build/compile_commands.json', 'r') as db:
        commands = json.load(db)
        files = [c['file'] for c in commands
                 if os.path.basename(c['file']) != 'glad.c']

    c_files = [f for f in files if f.endswith('.c')]
    cpp_files = [f for f in files if f.endswith('.cpp')]

    subprocess.call(['clang-tidy'] + c_files, cwd='build')

    subprocess.call(['clang-tidy', '--header-filter=".*\\.hpp'] + cpp_files,
                    cwd='build')

    try:
        subprocess.call(['iwyu_tool.py', '-o', 'clang', '-p', 'build'])
    except Exception:
        Logs.warn('Failed to call iwyu_tool.py')

# Alias .m files to be compiled like .c files, gcc will do the right thing.
@TaskGen.extension('.m')
def m_hook(self, node):
    return self.create_compiled_task('c', node)
