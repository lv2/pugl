// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGLUTIL_DEMO_UTILS_H
#define PUGLUTIL_DEMO_UTILS_H

#include <stdio.h>

typedef struct {
  double lastReportTime;
} PuglFpsPrinter;

static inline void
puglPrintFps(const double    thisTime,
             PuglFpsPrinter* printer,
             unsigned* const framesDrawn)
{
  if (thisTime > printer->lastReportTime + 5.0) {
    const double elapsed = (thisTime - printer->lastReportTime);
    const double fps     = *framesDrawn / elapsed;
    fprintf(stderr,
            "FPS: %.2f (%u frames / %.2f seconds)\n",
            fps,
            *framesDrawn,
            elapsed);

    printer->lastReportTime = thisTime;
    *framesDrawn            = 0;
  }
}

#endif // PUGLUTIL_DEMO_UTILS_H
