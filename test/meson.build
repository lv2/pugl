basic_tests = [
  'local_copy_paste',
  'realize',
  'redisplay',
  'remote_copy_paste',
  'show_hide',
  'size',
  'strerror',
  'stub_hints',
  'timer',
  'update',
  'view',
  'world',
]

gl_tests = [
  'gl_hints'
]

cairo_tests = [
  'cairo'
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
