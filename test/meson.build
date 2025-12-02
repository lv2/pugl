# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Suppress some additional C warnings in tests
test_c_args = []
if get_option('warning_level') == 'everything'
  if cc.get_id() == 'clang'
    test_c_args += ['-Wno-float-equal']
  elif cc.get_id() == 'gcc'
    test_c_args += ['-Wno-float-equal']
  endif

  test_c_args = cc.get_supported_arguments(test_c_args)
endif

test_c_args += c_suppressions

# Check licensing metadata
if not meson.is_subproject() and get_option('lint')
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse, args: ['--root', pugl_src_root, 'lint'], suite: 'data')
  endif
endif

basic_exclusive_tests = []

basic_tests = [
  'bad_call',
  'cursor',
  'realize',
  'redisplay',
  'show_hide',
  'size',
  'strerror',
  'stub',
  'stub_hints',
  'update',
  'view',
  'world',
]

cairo_tests = ['cairo']

gl_tests = ['gl', 'gl_free_unrealized', 'gl_hints']

vulkan_tests = ['vulkan']

if platform != 'x11' or use_xsync
  basic_exclusive_tests += ['local_copy_paste', 'remote_copy_paste']
  basic_tests += ['timer']
endif

# Basic tests that only need a stub backend
foreach test : basic_tests
  test(
    test,
    executable(
      'test_' + test,
      'test_@0@.c'.format(test),
      c_args: test_c_args,
      dependencies: [m_dep, pugl_dep, pugl_stub_dep, puglutil_dep],
      implicit_include_directories: false,
    ),
    suite: 'unit',
  )
endforeach

# Basic tests that only need a stub backend and must be run sequentially
foreach test : basic_exclusive_tests
  test(
    test,
    executable(
      'test_' + test,
      'test_@0@.c'.format(test),
      c_args: test_c_args,
      dependencies: [pugl_dep, pugl_stub_dep, puglutil_dep],
      implicit_include_directories: false,
    ),
    is_parallel: false,
    suite: 'unit',
  )
endforeach

# Tests that need an OpenGL backend
if opengl_dep.found()
  foreach test : gl_tests
    test(
      test,
      executable(
        'test_' + test,
        'test_@0@.c'.format(test),
        c_args: test_c_args + opengl_args,
        dependencies: [pugl_dep, pugl_gl_dep, puglutil_dep],
        implicit_include_directories: false,
      ),
      suite: 'unit',
    )
  endforeach
endif

# Tests that need a Cairo backend
if cairo_dep.found()
  foreach test : cairo_tests
    test(
      test,
      executable(
        'test_' + test,
        'test_@0@.c'.format(test),
        c_args: test_c_args + cairo_args,
        dependencies: [pugl_dep, pugl_cairo_dep, puglutil_dep],
        implicit_include_directories: false,
      ),
      suite: 'unit',
    )
  endforeach
endif

# Tests that need a Vulkan backend
if vulkan_dep.found()
  foreach test : vulkan_tests
    test(
      test,
      executable(
        'test_' + test,
        'test_@0@.c'.format(test),
        c_args: test_c_args,
        dependencies: [pugl_dep, pugl_vulkan_dep, puglutil_dep],
        implicit_include_directories: false,
      ),
      suite: 'unit',
    )
  endforeach
endif

###################
# Header Warnings #
###################

subdir('headers')
