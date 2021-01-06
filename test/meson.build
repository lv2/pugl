basic_tests = [
  'realize',
  'redisplay',
  'show_hide',
  'stub_hints',
  'timer',
  'update',
]

gl_tests = [
  'gl_hints'
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
