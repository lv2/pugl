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

#if !defined(__APPLE__) && !defined(_GNU_SOURCE)
#	define _GNU_SOURCE
#endif

#include "file_utils.h"

#ifdef _WIN32
#	include <io.h>
#	include <windows.h>
#	define F_OK 0
#else
#	include <libgen.h>
#	include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char*
resourcePath(const char* const programPath, const char* const name)
{
	char* const binary = strdup(programPath);

#ifdef _WIN32
	char programDir[_MAX_DIR];
	_splitpath(binary, programDir, NULL, NULL, NULL);
	_splitpath(binary, NULL, programDir + strlen(programDir), NULL, NULL);
	programDir[strlen(programDir) - 1] = '\0';
#else
	char* const programDir = dirname(binary);
#endif

	const size_t programDirLen = strlen(programDir);
	const size_t nameLen       = strlen(name);
	const size_t totalLen      = programDirLen + nameLen + 4;

	char* const programRelative = (char*)calloc(totalLen, 1);
	snprintf(programRelative, totalLen, "%s/%s", programDir, name);
	if (!access(programRelative, F_OK)) {
		free(binary);
		return programRelative;
	}

	free(programRelative);
	free(binary);

	const size_t sysPathLen = strlen(PUGL_DATA_DIR) + nameLen + 4;
	char* const  sysPath    = (char*)calloc(sysPathLen, 1);
	snprintf(sysPath, sysPathLen, "%s/%s", PUGL_DATA_DIR, name);
	return sysPath;
}
