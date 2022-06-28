# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

# General code to enable approximately all warnings in GCC 12, clang, and MSVC.
#
# This is trivial for clang and MSVC, but GCC doesn't have an "everything"
# option, so we need to enable everything we want explicitly.  Wall is assumed,
# but Wextra is not, for stability.
#
# These are collected from common.opt and c.opt in the GCC source, and manually
# curated with the help of the GCC documentation.  Warnings that are
# application-specific, historical, or about compatibility between specific
# language revisions are omitted.  The intent here is to have roughly the same
# meaning as clang's Weverything: extremely strict, but general.  Specifically
# omitted are:
#
# General:
#
# Wabi=
# Waggregate-return
# Walloc-size-larger-than=BYTES
# Walloca-larger-than=BYTES
# Wframe-larger-than=BYTES
# Wlarger-than=BYTES
# Wstack-usage=BYTES
# Wsystem-headers
# Wtraditional
# Wtraditional-conversion
# Wtrampolines
# Wvla-larger-than=BYTES
#
# Build specific:
#
# Wpoison-system-directories
#
# C Specific:
#
# Wc11-c2x-compat
# Wc90-c99-compat
# Wc99-c11-compat
# Wdeclaration-after-statement
# Wtraditional
# Wtraditional-conversion
#
# C++ Specific:
#
# Wc++0x-compat
# Wc++1z-compat
# Wc++2a-compat
# Wctad-maybe-unsupported
# Wnamespaces
# Wtemplates

gcc_common_warnings = [
  '-Walloc-zero',
  '-Walloca',
  '-Wanalyzer-too-complex',
  '-Warith-conversion',
  '-Warray-bounds=2',
  '-Wattribute-alias=2',
  '-Wbidi-chars=ucn',
  '-Wcast-align=strict',
  '-Wcast-function-type',
  '-Wcast-qual',
  '-Wclobbered',
  '-Wconversion',
  '-Wdate-time',
  '-Wdisabled-optimization',
  '-Wdouble-promotion',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wempty-body',
  '-Wendif-labels',
  '-Wfloat-equal',
  '-Wformat-overflow=2',
  '-Wformat-signedness',
  '-Wformat-truncation=2',
  '-Wformat=2',
  '-Wignored-qualifiers',
  '-Wimplicit-fallthrough=3',
  '-Winit-self',
  '-Winline',
  '-Winvalid-pch',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-field-initializers',
  '-Wmissing-include-dirs',
  '-Wmultichar',
  '-Wnormalized=nfc',
  '-Wnull-dereference',
  '-Wopenacc-parallelism',
  '-Woverlength-strings',
  '-Wpacked',
  '-Wpacked-bitfield-compat',
  '-Wpadded',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wshift-negative-value',
  '-Wshift-overflow=2',
  '-Wstack-protector',
  '-Wstrict-aliasing=3',
  '-Wstrict-overflow=5',
  '-Wstring-compare',
  '-Wstringop-overflow=3',
  '-Wsuggest-attribute=cold',
  '-Wsuggest-attribute=const',
  '-Wsuggest-attribute=format',
  '-Wsuggest-attribute=malloc',
  '-Wsuggest-attribute=noreturn',
  '-Wsuggest-attribute=pure',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wtrampolines',
  '-Wtrivial-auto-var-init',
  '-Wtype-limits',
  '-Wundef',
  '-Wuninitialized',
  '-Wunsafe-loop-optimizations',
  '-Wunused',
  '-Wunused-const-variable=2',
  '-Wunused-macros',
  '-Wvector-operation-performance',
  '-Wvla',
  '-Wwrite-strings',
]

#####
# C #
#####

if is_variable('cc') and not is_variable('all_c_warnings')
  all_c_warnings = []

  if cc.get_id() == 'clang'
    all_c_warnings += ['-Weverything']

  elif cc.get_id() == 'gcc'
    all_c_warnings += gcc_common_warnings + [
      '-Wabsolute-value',
      '-Wbad-function-cast',
      '-Wc++-compat',
      '-Wenum-conversion',
      '-Wjump-misses-init',
      '-Wmissing-parameter-type',
      '-Wmissing-prototypes',
      '-Wnested-externs',
      '-Wold-style-declaration',
      '-Wold-style-definition',
      '-Woverride-init',
      '-Wsign-compare',
      '-Wstrict-prototypes',
      '-Wunsuffixed-float-constants',
    ]

  elif cc.get_id() == 'msvc'
    all_c_warnings += ['/Wall']
  endif

  all_c_warnings = cc.get_supported_arguments(all_c_warnings)
endif

#######
# C++ #
#######

if is_variable('cpp')
  all_cpp_warnings = []

  if cpp.get_id() == 'clang'
    all_cpp_warnings += [
      '-Weverything',
      '-Wno-c++98-compat',
      '-Wno-c++98-compat-pedantic'
    ]

  elif cpp.get_id() == 'gcc'
    all_cpp_warnings += gcc_common_warnings + [
      '-Wabi-tag',
      '-Waligned-new=all',
      '-Wcatch-value=3',
      '-Wcomma-subscript',
      '-Wconditionally-supported',
      '-Wctor-dtor-privacy',
      '-Wdelete-non-virtual-dtor',
      '-Wdeprecated',
      '-Wdeprecated-copy',
      '-Wdeprecated-copy-dtor',
      '-Wdeprecated-enum-enum-conversion',
      '-Wdeprecated-enum-float-conversion',
      '-Weffc++',
      '-Wexpansion-to-defined',
      '-Wextra-semi',
      '-Wimport',
      '-Winvalid-imported-macros',
      '-Wmismatched-tags',
      '-Wmultiple-inheritance',
      '-Wnoexcept',
      '-Wnoexcept-type',
      '-Wnon-virtual-dtor',
      '-Wold-style-cast',
      '-Woverloaded-virtual',
      '-Wplacement-new=2',
      '-Wredundant-move',
      '-Wredundant-tags',
      '-Wregister',
      '-Wsign-compare',
      '-Wsign-promo',
      '-Wsized-deallocation',
      '-Wstrict-null-sentinel',
      '-Wsuggest-final-methods',
      '-Wsuggest-final-types',
      '-Wsuggest-override',
      '-Wuseless-cast',
      '-Wvirtual-inheritance',
      '-Wvolatile',
      '-Wzero-as-null-pointer-constant',
    ]

  elif cpp.get_id() == 'msvc'
    all_cpp_warnings += ['/Wall']
  endif

  all_cpp_warnings = cpp.get_supported_arguments(all_cpp_warnings)
endif

# Set all_objc_warnings for the current Objective C compiler
if is_variable('objcc')
  all_objc_warnings = []

  if objcc.get_id() == 'clang'
    all_objc_warnings += ['-Weverything']

  elif objc.get_id() == 'gcc'
    all_objc_warnings += gcc_common_warnings + [
      '-Wno-direct-ivar-access',
    ]
  endif

  all_objc_warnings = objcc.get_supported_arguments(all_objc_warnings)
endif
