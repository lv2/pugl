data_dir = get_option('prefix') / get_option('datadir') / 'pugl-0'
example_defines = ['-DPUGL_DATA_DIR="@0@"'.format(data_dir)]

gl_examples = [
  'pugl_cpp_demo.cpp',
  'pugl_embed_demo.c',
  'pugl_print_events.c',
  'pugl_shader_demo.c',
  'pugl_window_demo.c',
]

cairo_examples = [
  'pugl_cairo_demo.c'
]

vulkan_examples = [
  'pugl_vulkan_cpp_demo.cpp',
  'pugl_vulkan_demo.c',
]

includes = include_directories(
  '..',
  '../bindings/cpp/include',
  '../include',
)

# Suppress some additional C warnings in examples
example_c_args = []
if get_option('strict')
  if cc.get_id() == 'clang'
    example_c_args += [
      '-Wno-float-equal',
      '-Wno-padded',
    ]
  elif cc.get_id() == 'gcc'
    example_c_args += [
      '-Wno-float-equal',
      '-Wno-padded',
    ]
  endif

  example_c_args = cc.get_supported_arguments(example_c_args)
endif

# Suppress some additional C++ warnings in examples
example_cpp_args = []
if is_variable('cpp')
  if cpp.get_id() == 'clang'
    example_cpp_args += [
      '-Wno-documentation', # Cairo
      '-Wno-documentation-unknown-command', # Cairo
      '-Wno-old-style-cast',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-switch-enum',
    ]
  elif cpp.get_id() == 'gcc'
    example_cpp_args += [
      '-Wno-effc++',
      '-Wno-old-style-cast',
      '-Wno-padded',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unused-const-variable',
      '-Wno-useless-cast',
    ]
  endif

  example_cpp_args = cpp.get_supported_arguments(example_cpp_args)
endif

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
               include_directories: includes,
               c_args: example_defines + example_c_args,
               cpp_args: example_defines + example_cpp_args,
               dependencies: dependencies)
  endforeach
endif

# Build Cairo examples
if cairo_dep.found()
  foreach example : cairo_examples
    target = example.split('.')[0]
    executable(target, example,
               include_directories: includes,
               c_args: example_defines + example_c_args,
               dependencies: [pugl_dep, cairo_backend_dep])
  endforeach
endif

# Build Vulkan examples
if vulkan_dep.found()
  foreach example : vulkan_examples
    source = [example]
    target = example.split('.')[0]
    dependencies = [dl_dep, vulkan_backend_dep]

    if target == 'pugl_vulkan_cpp_demo'
      source += ['file_utils.c']
    endif

    executable(target, source,
               include_directories: includes,
               c_args: example_defines + example_c_args,
               cpp_args: example_defines + example_cpp_args,
               dependencies: dependencies)
  endforeach
endif
