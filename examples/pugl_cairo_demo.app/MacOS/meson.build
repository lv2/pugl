executable(
  'pugl_cairo_demo',
  ['../../pugl_cairo_demo.c'],
  include_directories: include_directories('../../../../include', '../../..'),
  c_args: example_defines + example_c_args + cairo_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [pugl_dep, cairo_backend_dep])
