data_dir = get_option('prefix') / get_option('datadir') / 'pugl-0'
example_args = ['-DPUGL_DATA_DIR="@0@"'.format(data_dir)]

gl_examples = [
  'pugl_cxx_demo.cpp',
  'pugl_embed_demo.c',
  'pugl_print_events.c',
  'pugl_shader_demo.c',
  'pugl_window_demo.c',
]

cairo_examples = [
  'pugl_cairo_demo.c'
]

vulkan_examples = [
  'pugl_vulkan_cxx_demo.cpp',
  'pugl_vulkan_demo.c',
]

includes = [
  '.',
  '..',
  '../bindings/cxx/include',
  '../include',
]

subdir('shaders')

# Build GL examples
if opengl_dep.found()
  foreach example : gl_examples
    source = [example]
    target = example.split('.')[0]
    dependencies = [gl_backend_dep]

    if target == 'pugl_shader_demo'
      source += ['file_utils.c', 'glad/glad.c']
      dependencies += [dl_dep]
    elif target == 'pugl_print_events'
      dependencies += [stub_backend_dep]
    endif

    executable(target, source,
               include_directories: include_directories(includes),
               c_args: example_args,
               cpp_args: example_args,
               dependencies: dependencies)
  endforeach
endif

# Build Cairo examples
if cairo_dep.found()
  foreach example : cairo_examples
    target = example.split('.')[0]
    executable(target, example,
               include_directories: include_directories(includes),
               c_args: example_args,
               dependencies: [pugl_dep, cairo_backend_dep])
  endforeach
endif

# Build Vulkan examples
if vulkan_dep.found()
  foreach example : vulkan_examples
    source = [example]
    target = example.split('.')[0]
    dependencies = [dl_dep, vulkan_backend_dep]

    if target == 'pugl_vulkan_cxx_demo'
      source += ['file_utils.c']
    endif

    executable(target, source,
               include_directories: include_directories(includes),
               c_args: example_args,
               cpp_args: example_args,
               dependencies: dependencies)
  endforeach
endif
