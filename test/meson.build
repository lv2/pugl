basic_tests = [
  'local_copy_paste',
  'realize',
  'redisplay',
  'remote_copy_paste',
  'show_hide',
  'size',
  'strerror',
  'stub',
  'stub_hints',
  'timer',
  'update',
  'view',
  'world',
]

cairo_tests = [
  'cairo'
]

gl_tests = [
  'gl',
  'gl_hints'
]

vulkan_tests = [
  'vulkan'
]

includes = [
  '.',
  '../include',
]

foreach test : basic_tests
  test(test,
       executable('test_' + test, 'test_@0@.c'.format(test),
                  include_directories: include_directories(includes),
                  dependencies: [pugl_dep, stub_backend_dep]))
endforeach

if opengl_dep.found()
  foreach test : gl_tests
    test(test,
         executable('test_' + test, 'test_@0@.c'.format(test),
                    include_directories: include_directories(includes),
                    dependencies: [pugl_dep, gl_backend_dep]))
  endforeach
endif

if cairo_dep.found()
  foreach test : cairo_tests
    test(test,
         executable('test_' + test, 'test_@0@.c'.format(test),
                    include_directories: include_directories(includes),
                    dependencies: [pugl_dep, cairo_backend_dep]))
  endforeach
endif

if vulkan_dep.found()
  foreach test : vulkan_tests
    test(test,
         executable('test_' + test, 'test_@0@.c'.format(test),
                    include_directories: include_directories(includes),
                    dependencies: [pugl_dep, vulkan_backend_dep]))
  endforeach
endif

unified_args = []
unified_deps = [core_deps]
if cairo_dep.found()
  unified_args += ['-DWITH_CAIRO']
  unified_deps += [cairo_dep]
endif

if opengl_dep.found()
  unified_args += ['-DWITH_OPENGL']
  unified_deps += [opengl_dep]
endif

if vulkan_dep.found()
  unified_args += ['-DWITH_VULKAN']
  unified_deps += [vulkan_deps]
endif

if host_machine.system() == 'darwin'
  add_languages(['objcpp'])

  objcpp = meson.get_compiler('objcpp')
  unified_args += objcpp.get_supported_arguments(
    c_warnings + cpp_warnings + objc_warnings)

  executable('inline_objcpp', 'test_inline_objcpp.mm',
             include_directories: include_directories(includes),
             dependencies: unified_deps,
             objcpp_args: unified_args)
elif is_variable('cpp')
  if cpp.get_id() == 'msvc'
    unified_args += [
      '/wd4464' # relative include path contains '..'
    ]
  endif

  executable('inline_cpp', 'test_inline_cpp.cpp',
             include_directories: include_directories(includes),
             dependencies: unified_deps,
             cpp_args: unified_args)
endif
