// Copyright 2019-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <puglutil/file_utils.h>

#ifdef _WIN32
#  include <io.h>
#  include <windows.h>
#else
#  include <libgen.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE*
resourceFile(const char* const programPath,
             const char* const dataDir,
             const char* const name)
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

  FILE* file = fopen(programRelative, "rb");
  free(programRelative);
  free(binary);
  if (file) {
    return file;
  }

  const size_t sysPathLen = strlen(dataDir) + nameLen + 4;
  char* const  sysPath    = (char*)calloc(sysPathLen, 1);
  snprintf(sysPath, sysPathLen, "%s/%s", dataDir, name);
  file = fopen(sysPath, "rb");
  free(sysPath);
  return file;
}
