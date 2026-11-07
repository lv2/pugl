// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGLUTIL_DEMO_UTILS_H
#define PUGLUTIL_DEMO_UTILS_H

#include <stdio.h>

#include <pugl/pugl.h>

typedef struct {
  double   lastReportTime;
  unsigned numCalls;
} PuglFpsPrinter;

static inline void
puglPrintFps(const double    thisTime,
             PuglFpsPrinter* printer,
             unsigned* const framesDrawn)
{
  ++printer->numCalls;

  if (thisTime > printer->lastReportTime + 5.0) {
    const double elapsed = (thisTime - printer->lastReportTime);
    fprintf(stderr,
            "FPS: %.2f (%u frames) TPS: %.2f (%u ticks) / %.2f s\n",
            *framesDrawn / elapsed,
            *framesDrawn,
            (double)printer->numCalls / elapsed,
            printer->numCalls,
            elapsed);

    printer->lastReportTime = thisTime;
    printer->numCalls       = 0;
    *framesDrawn            = 0;
  }
}

#endif // PUGLUTIL_DEMO_UTILS_H
