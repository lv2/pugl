# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

# General code to determine the library type to build.
#
# Unfortunately, meson's default_library option does not handle real-world
# situations out of the box.  In particular, it is usually necessary to specify
# different flags for static and shared builds of C libraries so that symbols
# can be exported.  To work around this, we do not support default_library=both
# on Windows.  On other platforms with GCC-like compilers, we can support both
# because symbols can safely be exported in the same way (giving them default
# visibility) in both static and shared builds.

if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif

  library_type = 'both_libraries'
elif get_option('default_library') == 'shared'
  library_type = 'shared_library'
else
  library_type = 'static_library'
  add_project_arguments(['-DPUGL_STATIC'], language: ['c', 'cpp', 'objc'])
endif
