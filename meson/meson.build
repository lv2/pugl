# General code to enable approximately all warnings.
#
# This is trivial for clang and MSVC, but GCC does not have such an option, and
# has several esoteric warnings, so we need to enable everything we want
# explicitly.  We enable everything that does not require a value argument,
# except for warnings that are only relevant for very old languages (earlier
# than C99 or C++11) or non-standard extensions.
#
# Omitted common warnings:
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
# Omitted C warnings:
#
# Wc90-c99-compat
# Wdeclaration-after-statement
# Wtraditional
# Wtraditional-conversion
#
# Omitted C++ warnings:
#
# Wnamespaces
# Wtemplates

gcc_common_warnings = [
  '-Walloc-zero',
  '-Walloca',
  '-Wanalyzer-too-complex',
  '-Warith-conversion',
  '-Warray-bounds=2',
  '-Wattribute-alias=2',
  '-Wcast-align=strict',
  '-Wcast-qual',
  '-Wconversion',
  '-Wdate-time',
  '-Wdisabled-optimization',
  '-Wdouble-promotion',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wfloat-equal',
  '-Wformat-overflow=2',
  '-Wformat-signedness',
  '-Wformat-truncation=2',
  '-Wformat=2',
  '-Wimplicit-fallthrough=2',
  '-Winit-self',
  '-Winline',
  '-Winvalid-pch',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-include-dirs',
  '-Wmultichar',
  '-Wnormalized=nfc',
  '-Wnull-dereference',
  '-Wpacked',
  '-Wpadded',
  '-Wredundant-decls',
  '-Wscalar-storage-order',
  '-Wshadow',
  '-Wshift-overflow=2',
  '-Wsizeof-array-argument',
  '-Wstack-protector',
  '-Wstrict-aliasing=3',
  '-Wstrict-overflow=5',
  '-Wstringop-overflow=3',
  '-Wsuggest-attribute=cold',
  '-Wsuggest-attribute=const',
  '-Wsuggest-attribute=format',
  '-Wsuggest-attribute=malloc',
  '-Wsuggest-attribute=noreturn',
  '-Wsuggest-attribute=pure',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wsync-nand',
  '-Wundef',
  '-Wunused-const-variable=2',
  '-Wunused-macros',
  '-Wvarargs',
  '-Wvector-operation-performance',
  '-Wvla',
  '-Wwrite-strings',
]

gcc_c_warnings = [
  '-Wbad-function-cast',
  '-Wc++-compat',
  '-Wc99-c11-compat',
  '-Wdesignated-init',
  '-Wdiscarded-array-qualifiers',
  '-Wdiscarded-qualifiers',
  '-Wincompatible-pointer-types',
  '-Wjump-misses-init',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Wold-style-definition',
  '-Wstrict-prototypes',
  '-Wunsuffixed-float-constants',
]

# Set all_c_warnings for the current C compiler
if is_variable('cc')
  if cc.get_id() == 'clang'
    all_c_warnings = ['-Weverything']
  elif cc.get_id() == 'gcc'
    all_c_warnings = gcc_common_warnings + [
      '-Wbad-function-cast',
      '-Wc++-compat',
      '-Wc99-c11-compat',
      '-Wdesignated-init',
      '-Wdiscarded-array-qualifiers',
      '-Wdiscarded-qualifiers',
      '-Wincompatible-pointer-types',
      '-Wjump-misses-init',
      '-Wmissing-prototypes',
      '-Wnested-externs',
      '-Wold-style-definition',
      '-Wstrict-prototypes',
      '-Wunsuffixed-float-constants',
    ]
  elif cc.get_id() == 'msvc'
    all_c_warnings = ['/Wall']
  else
    all_c_warnings = []
  endif
endif

# Set all_cpp_warnings for the current C++ compiler
if is_variable('cpp')
  if cpp.get_id() == 'clang'
    all_cpp_warnings = [
      '-Weverything',
      '-Wno-c++98-compat',
      '-Wno-c++98-compat-pedantic'
    ]
  elif cpp.get_id() == 'gcc'
    all_cpp_warnings = gcc_common_warnings + [
      '-Wabi-tag',
      '-Waligned-new=all',
      '-Wcatch-value=3',
      '-Wcomma-subscript',
      '-Wconditionally-supported',
      '-Wctor-dtor-privacy',
      '-Wdeprecated-copy-dtor',
      '-Weffc++',
      '-Wextra-semi',
      '-Wmismatched-tags',
      '-Wmultiple-inheritance',
      '-Wnoexcept',
      '-Wnoexcept-type',
      '-Wnon-virtual-dtor',
      '-Wold-style-cast',
      '-Woverloaded-virtual',
      '-Wplacement-new=2',
      '-Wredundant-tags',
      '-Wregister',
      '-Wsign-promo',
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
    all_cpp_warnings = ['/Wall']
  else
    all_cpp_warnings = []
  endif
endif

# Set all_objc_warnings for the current Objective C compiler
if is_variable('objcc')
  all_objc_warnings = []
  if objcc.get_id() == 'clang'
    all_objc_warnings = ['-Weverything']
  elif objc.get_id() == 'gcc'
    all_objc_warnings = gcc_common_warnings + [
      '-Wno-direct-ivar-access',
    ]
  else
    all_objc_warnings = []
  endif
endif
