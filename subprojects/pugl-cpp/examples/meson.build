# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

data_dir = get_option('prefix') / get_option('datadir') / 'pugl-0'
example_defines = ['-DPUGL_DATA_DIR="@0@"'.format(data_dir)]

puglutil_dep = dependency('puglutil')

pugl_gl_dep = dependency('pugl-gl-0', required: false, version: '>= 0.5.3')

pugl_vulkan_dep = dependency(
  'pugl-vulkan-0',
  required: false,
  version: '>= 0.5.3',
)

gl_examples = ['pugl_cpp_demo.cpp']

vulkan_examples = ['pugl_vulkan_cpp_demo.cpp']

# Suppress some additional C++ warnings in examples
example_cpp_args = []
if get_option('warning_level') == 'everything' and is_variable('cpp')
  if cpp.get_id() == 'clang'
    example_cpp_args += ['-Wno-old-style-cast', '-Wno-switch-enum']
    if host_machine.system() == 'windows'
      example_cpp_args += [
        '-Wno-deprecated-declarations',
        '-Wno-format-nonliteral',
        '-Wno-nonportable-system-include-path',
      ]
    endif
  elif cpp.get_id() == 'gcc'
    example_cpp_args += [
      '-Wno-effc++',
      '-Wno-old-style-cast',
      '-Wno-strict-overflow',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unused-const-variable',
      '-Wno-useless-cast',
    ]
  elif cpp.get_id() == 'msvc'
    example_cpp_args += [
      '/wd4355',  # 'this' used in base member initializer list
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4868',  # may not enforce left-to-right evaluation order
      '/wd5246',  # subobject initialization should be wrapped in braces
    ]
  endif
  example_cpp_args = cpp.get_supported_arguments(example_cpp_args)
endif
example_cpp_args += cpp_suppressions

subdir('shaders')

if host_machine.system() == 'darwin'
  # On Darwin, build examples as application bundles (required to work properly)

  if pugl_gl_dep.found()
    subdir('pugl_cpp_demo.app')
  endif

  if pugl_vulkan_dep.found()
    subdir('pugl_vulkan_cpp_demo.app')
  endif

else
  # On all other platforms, build examples as simple programs

  # Build GL examples
  if pugl_gl_dep.found()
    foreach example : gl_examples
      source = [example]
      target = example.split('.')[0]
      dependencies = [dl_dep, pugl_cpp_dep, pugl_gl_dep, puglutil_dep]
      defines = []

      executable(
        target,
        source,
        cpp_args: example_defines + example_cpp_args + defines,
        dependencies: dependencies,
      )
    endforeach
  endif

  # Build Vulkan examples
  if pugl_vulkan_dep.found()
    foreach example : vulkan_examples
      source = [example]
      target = example.split('.')[0]
      dependencies = [dl_dep, pugl_cpp_dep, pugl_vulkan_dep, puglutil_dep]
      defines = ['-D_POSIX_C_SOURCE=200809L']

      executable(
        target,
        source,
        cpp_args: example_defines + example_cpp_args + defines,
        dependencies: dependencies,
      )
    endforeach
  endif
endif
