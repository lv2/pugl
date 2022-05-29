# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

data_dir = get_option('prefix') / get_option('datadir') / 'pugl-0'
example_defines = ['-DPUGL_DATA_DIR="@0@"'.format(data_dir)]

stub_examples = [
  'pugl_print_events.c',
]

gl_examples = [
  'pugl_clipboard_demo.c',
  'pugl_cpp_demo.cpp',
  'pugl_cursor_demo.c',
  'pugl_embed_demo.c',
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
      '-Wno-reserved-id-macro',
      '-Wno-reserved-identifier',
    ]
  elif cc.get_id() == 'gcc'
    example_c_args += [
      '-Wno-float-equal',
    ]
  endif

  example_c_args = cc.get_supported_arguments(example_c_args)
endif

# Suppress some additional C++ warnings in examples
example_cpp_args = []
if get_option('strict') and is_variable('cpp')
  if cpp.get_id() == 'clang'
    example_cpp_args += [
      '-Wno-documentation', # Cairo
      '-Wno-documentation-unknown-command', # Cairo
      '-Wno-old-style-cast',
      '-Wno-switch-enum',
    ]
  elif cpp.get_id() == 'gcc'
    example_cpp_args += [
      '-Wno-effc++',
      '-Wno-old-style-cast',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unused-const-variable',
      '-Wno-useless-cast',
    ]
  elif cpp.get_id() == 'msvc'
    example_cpp_args += [
        '/wd4355', # 'this' used in base member initializer list
        '/wd4868', # may not enforce left-to-right evaluation order
        '/wd5246', # subobject initialization should be wrapped in braces
    ]
  endif

  example_cpp_args = cpp.get_supported_arguments(example_cpp_args)
endif

subdir('shaders')

if host_machine.system() == 'darwin'
  # On Darwin, build examples as application bundles (required to work properly)

  subdir('pugl_clipboard_demo.app')

  if cairo_dep.found()
    subdir('pugl_cairo_demo.app')
  endif

  if opengl_dep.found()
    subdir('pugl_cpp_demo.app')
    subdir('pugl_cursor_demo.app')
    subdir('pugl_embed_demo.app')
    subdir('pugl_shader_demo.app')
    subdir('pugl_window_demo.app')
  endif

  if vulkan_dep.found()
    subdir('pugl_vulkan_cpp_demo.app')
    subdir('pugl_vulkan_demo.app')
  endif

else
  # On all other platforms, build examples as simple programs

  # Build stub examples
  foreach example : stub_examples
    source = [example]
    target = example.split('.')[0]
    dependencies = [pugl_dep, stub_backend_dep]
    defines = []

    executable(target, source,
               include_directories: includes,
               c_args: example_defines + example_c_args + defines,
               cpp_args: example_defines + example_cpp_args + defines,
               dependencies: dependencies)
  endforeach

  # Build GL examples
  if opengl_dep.found()
    foreach example : gl_examples
      source = [example]
      target = example.split('.')[0]
      dependencies = [pugl_dep, gl_backend_dep]
      defines = []

      if target == 'pugl_shader_demo'
        source += ['file_utils.c', 'glad/glad.c']
        dependencies += [dl_dep]
        defines += ['-D_POSIX_C_SOURCE=200809L']
      elif target == 'pugl_print_events'
        dependencies += [stub_backend_dep]
      elif target == 'pugl_cpp_demo'
        dependencies += [puglpp_dep]
      endif

      executable(target, source,
                 include_directories: includes,
                 c_args: example_defines + example_c_args + defines,
                 cpp_args: example_defines + example_cpp_args + defines,
                 dependencies: dependencies)
    endforeach
  endif

  # Build Cairo examples
  if cairo_dep.found()
    foreach example : cairo_examples
      target = example.split('.')[0]
      executable(target, example,
                 include_directories: includes,
                 c_args: example_defines + example_c_args + cairo_args,
                 dependencies: [pugl_dep, cairo_backend_dep])
    endforeach
  endif

  # Build Vulkan examples
  if vulkan_dep.found()
    foreach example : vulkan_examples
      source = [example]
      target = example.split('.')[0]
      dependencies = [dl_dep, vulkan_backend_dep]
      defines = []

      if target == 'pugl_vulkan_cpp_demo'
        source += ['file_utils.c']
        defines += ['-D_POSIX_C_SOURCE=200809L']
      endif

      executable(target, source,
                 include_directories: includes,
                 c_args: example_defines + example_c_args + defines,
                 cpp_args: example_defines + example_cpp_args + defines,
                 dependencies: dependencies)
    endforeach
  endif
endif
