executable(
  'pugl_vulkan_demo',
  [
    '../../pugl_vulkan_demo.c',
    '../../file_utils.c',
  ],
  include_directories: include_directories('../../../../include', '../../..'),
  c_args: example_defines + example_c_args,
  dependencies: [pugl_dep, vulkan_backend_dep])
