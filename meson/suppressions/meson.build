# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

# Project-specific warning suppressions.
#
# This should be used in conjunction with the generic "warnings" sibling that
# enables all reasonable warnings for the compiler.  It lives here just to keep
# the top-level meson.build more readable.

#####
# C #
#####

c_warnings = all_c_warnings
if cc.get_id() == 'clang'
  c_warnings += [
    '-Wno-bad-function-cast',
    '-Wno-padded',
    '-Wno-switch-default',
    '-Wno-switch-enum',
  ]

  if host_machine.system() == 'darwin'
    c_warnings += [
      '-Wno-poison-system-directories',
    ]
  elif host_machine.system() == 'windows'
    c_warnings += [
      '-Wno-deprecated-declarations',
      '-Wno-format-nonliteral',
      '-Wno-nonportable-system-include-path',
      '-Wno-unused-macros',
    ]
  endif
elif cc.get_id() == 'gcc'
  c_warnings += [
    '-Wno-bad-function-cast',
    '-Wno-inline',
    '-Wno-padded',
    '-Wno-pedantic',
    '-Wno-suggest-attribute=const',
    '-Wno-suggest-attribute=malloc',
    '-Wno-suggest-attribute=pure',
    '-Wno-switch-default',
    '-Wno-switch-enum',
    '-Wno-unsuffixed-float-constants',
  ]

  if host_machine.system() == 'windows'
    c_warnings += [
      '-Wno-cast-function-type',
      '-Wno-float-equal',
      '-Wno-suggest-attribute=format',
    ]
  endif
elif cc.get_id() == 'msvc'
  c_warnings += [
    '/wd4061', # enumerator in switch is not explicitly handled
    '/wd4191', # unsafe conversion from type to type
    '/wd4514', # unreferenced inline function has been removed
    '/wd4706', # assignment within conditional expression
    '/wd4710', # function not inlined
    '/wd4711', # function selected for automatic inline expansion
    '/wd4820', # padding added after construct
    '/wd4996', # POSIX name for this item is deprecated
    '/wd5045', # will insert Spectre mitigation for memory load
    '/wd5246', # subobject initialization should be wrapped in braces
  ]
endif

add_project_arguments(cc.get_supported_arguments(c_warnings),
                      language: ['c'])

#######
# C++ #
#######

if is_variable('cpp')
  cpp_warnings = all_cpp_warnings

  if cpp.get_id() == 'clang'
    cpp_warnings += [
      '-Wno-inline',
      '-Wno-padded',
    ]

    if host_machine.system() == 'darwin'
      cpp_warnings += [
        '-Wno-poison-system-directories',
      ]
    endif

  elif cpp.get_id() == 'gcc'
    cpp_warnings += [
      '-Wno-inline',
      '-Wno-padded',
    ]
  elif cpp.get_id() == 'msvc'
    cpp_warnings += [
      '/wd4061', # enumerator in switch is not explicitly handled
      '/wd4191', # unsafe conversion from type to type
      '/wd4514', # unreferenced inline function has been removed
      '/wd4625', # copy constructor implicitly deleted
      '/wd4626', # copy assignment operator implicitly deleted
      '/wd4706', # assignment within conditional expression
      '/wd4710', # function not inlined
      '/wd4711', # function selected for automatic inline expansion
      '/wd4800', # implicit conversion to bool
      '/wd4820', # padding added after construct
      '/wd5026', # move constructor implicitly deleted
      '/wd5027', # move assignment operator implicitly deleted
      '/wd5039', # pointer to potentially throwing function passed to C
      '/wd5045', # will insert Spectre mitigation for memory load
    ]
  endif

  add_project_arguments(cpp.get_supported_arguments(cpp_warnings),
                        language: ['cpp'])
endif

###############
# Objective C #
###############

if is_variable('objcc')
  objc_warnings = all_objc_warnings +  [
    '-Wno-bad-function-cast',
    '-Wno-direct-ivar-access',
    '-Wno-padded',
    '-Wno-pedantic',
    '-Wno-poison-system-directories',
  ]

  add_project_arguments(objcc.get_supported_arguments(objc_warnings),
                        language: ['objc'])
endif
