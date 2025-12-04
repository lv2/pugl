# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

####################
# Project Metadata #
####################

if get_option('lint')
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse, args: ['--root', '..', 'lint'], suite: 'data')
  endif
endif

###################
# Header Warnings #
###################

subdir('headers')
