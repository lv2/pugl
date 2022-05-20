executable(
  'pugl_vulkan_cpp_demo',
  [
    '../../pugl_vulkan_cpp_demo.cpp',
    '../../file_utils.c',
  ],
  include_directories: include_directories('../../../../include', '../../..'),
  c_args: example_defines + example_c_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [puglpp_dep, vulkan_backend_dep])
