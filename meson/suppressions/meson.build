# Copyright 2020-2024 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions

warning_level = get_option('warning_level')

#####
# C #
#####

c_suppressions = []
if cc.get_id() == 'clang'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-cast-function-type-strict',
      '-Wno-declaration-after-statement',
      '-Wno-padded',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsafe-buffer-usage',
    ]

    if not meson.is_cross_build()
      c_suppressions += ['-Wno-poison-system-directories']
    endif

    if host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-cast-function-type',
        '-Wno-deprecated-declarations',
        '-Wno-format-nonliteral',
        '-Wno-nonportable-system-include-path',
        '-Wno-unused-macros',
      ]
    endif
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=pure',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsuffixed-float-constants',
    ]

    if host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-cast-function-type',
        '-Wno-float-equal',
        '-Wno-pedantic',
        '-Wno-suggest-attribute=format',
      ]
    endif
  endif

elif cc.get_id() == 'msvc'
  c_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    c_suppressions += [
      '/wd4061',  # enumerator in switch is not explicitly handled
      '/wd4191',  # unsafe conversion from type to type
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4710',  # function not inlined
      '/wd4711',  # function selected for automatic inline expansion
      '/wd4820',  # padding added after construct
      '/wd5045',  # will insert Spectre mitigation for memory load
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '/wd4100',  # unreferenced formal parameter
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

###############
# Objective C #
###############

objc_suppressions = []
if is_variable('objcc')
  if warning_level == 'everything'
    objc_suppressions = [
      '-Wno-switch-enum',
      '-Wno-bad-function-cast',
      '-Wno-declaration-after-statement',
      '-Wno-direct-ivar-access',
      '-Wno-objc-messaging-id',
      '-Wno-padded',
      '-Wno-pedantic',
      '-Wno-poison-system-directories',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-undeclared-selector',
    ]

    objc_suppressions = objcc.get_supported_arguments(objc_suppressions)
    objc_suppressions += c_suppressions
  endif
endif
