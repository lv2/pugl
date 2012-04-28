#!/usr/bin/env python
import glob
import os
import sys

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs, waflib.Options as Options

# Version of this package (even if built as a child)
PUGL_VERSION       = '0.0.0'
PUGL_MAJOR_VERSION = '0'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Pugl uses the same version number for both library and package
PUGL_LIB_VERSION = PUGL_VERSION

# Variables for 'waf dist'
APPNAME = 'pugl'
VERSION = PUGL_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')
    autowaf.set_options(opt)
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--static', action='store_true', default=False, dest='static',
                   help="Build static library")

def configure(conf):
    conf.load('compiler_c')
    autowaf.configure(conf)
    autowaf.display_header('Pugl Configuration')

    if conf.env['MSVC_COMPILER']:
        conf.env.append_unique('CFLAGS', ['-TP', '-MD'])
    else:
        conf.env.append_unique('CFLAGS', '-std=c99')

    conf.env['BUILD_TESTS']  = Options.options.build_tests
    conf.env['BUILD_STATIC'] = (Options.options.build_tests or
                                Options.options.static)

    autowaf.define(conf, 'PUGL_VERSION', PUGL_VERSION)
    conf.write_config_header('pugl_config.h', remove=False)

    conf.env['INCLUDES_PUGL'] = ['%s/pugl-%s' % (conf.env['INCLUDEDIR'],
                                                 PUGL_MAJOR_VERSION)]
    conf.env['LIBPATH_PUGL'] = [conf.env['LIBDIR']]
    conf.env['LIB_PUGL'] = ['pugl-%s' % PUGL_MAJOR_VERSION];

    autowaf.display_msg(conf, "Static library", str(conf.env['BUILD_STATIC']))
    autowaf.display_msg(conf, "Unit tests", str(conf.env['BUILD_TESTS']))
    print('')

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/pugl-%s/pugl' % PUGL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'PUGL', PUGL_VERSION, PUGL_MAJOR_VERSION, [],
                     {'PUGL_MAJOR_VERSION' : PUGL_MAJOR_VERSION})

    libflags = [ '-fvisibility=hidden' ]
    libs     = [ 'X11', 'GL' ]
    defines  = []
    if bld.env['MSVC_COMPILER']:
        libflags = []
        libs     = []
        defines  = ['snprintf=_snprintf']

    lib_source = ['./pugl/pugl_x11.c']

    # Shared Library
    obj = bld(features        = 'c cshlib',
              export_includes = ['.'],
              source          = lib_source,
              includes        = ['.', './src'],
              lib             = libs,
              name            = 'libpugl',
              target          = 'pugl-%s' % PUGL_MAJOR_VERSION,
              vnum            = PUGL_LIB_VERSION,
              install_path    = '${LIBDIR}',
              defines         = defines,
              cflags          = libflags + [ '-DPUGL_SHARED',
                                             '-DPUGL_INTERNAL' ])

    # Static library
    if bld.env['BUILD_STATIC']:
        obj = bld(features        = 'c cstlib',
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  lib             = libs,
                  name            = 'libpugl_static',
                  target          = 'pugl-%s' % PUGL_MAJOR_VERSION,
                  vnum            = PUGL_LIB_VERSION,
                  install_path    = '${LIBDIR}',
                  defines         = defines,
                  cflags          = ['-DPUGL_INTERNAL'])

    if bld.env['BUILD_TESTS']:
        test_libs   = libs
        test_cflags = ['']

        # Unit test program
        obj = bld(features     = 'c cprogram',
                  source       = 'pugl_test.c',
                  includes     = ['.', './src'],
                  use          = 'libpugl_static',
                  lib          = test_libs,
                  target       = 'pugl_test',
                  install_path = '',
                  defines      = defines,
                  cflags       = test_cflags)

def lint(ctx):
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include src/* pugl/*', shell=True)

