/*
  Copyright 2019-2020 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef EXAMPLES_FILE_UTILS_H
#define EXAMPLES_FILE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
   Return the path to a resource file.

   This takes a name like "shaders/something.glsl" and returns the actual
   path that can be used to load that resource, which may be relative to the
   current executable (for running in bundles or the build directory), or a
   shared system directory for installs.

   The returned path must be freed with free().
*/
char*
resourcePath(const char* programPath, const char* name);

#ifdef __cplusplus
}
#endif

#endif // EXAMPLES_FILE_UTILS_H
