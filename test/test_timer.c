// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that update events are received and than redisplays they trigger happen
  immediately in the same event loop iteration.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __APPLE__
static const double timeout = 1 / 60.0;
#else
static const double timeout = -1.0;
#endif

// Windows SetTimer has a maximum resolution of 10ms
static const double tolerance = 0.012;

static const uintptr_t timerId     = 1u;
static const double    timerPeriod = 1 / 60.0;

typedef enum {
  START,
  EXPOSED,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  size_t          numAlarms;
  double          firstAlarmTime;
  double          lastAlarmTime;
  State           state;
} PuglTest;

static void
onTimer(PuglView* const view, const PuglTimerEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);
  const double    time = puglGetTime(puglGetWorld(view));

  assert(event->id == timerId);

  if (test->numAlarms++ == 0) {
    test->firstAlarmTime = time;
  }

  test->lastAlarmTime = time;
}

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    test->state = EXPOSED;
    break;

  case PUGL_TIMER:
    onTimer(view, &event->timer);
    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

static double
roundPeriod(const double period)
{
  return floor(period * 1000.0) / 1000.0; // Round down to milliseconds
}

int
main(int argc, char** argv)
{
  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   0,
                   0.0,
                   0.0,
                   START};

  // Set up view
  test.view = puglNewView(test.world);
  puglSetClassName(test.world, "PuglTest");
  puglSetWindowTitle(test.view, "Pugl Timer Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 512, 512);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view));
  while (test.state != EXPOSED) {
    assert(!puglUpdate(test.world, timeout));
  }

  // Register a timer with a longer period first
  assert(!puglStartTimer(test.view, timerId, timerPeriod * 2.0));

  // Replace it with the one we want (to ensure timers are replaced)
  assert(!puglStartTimer(test.view, timerId, timerPeriod));

  puglUpdate(test.world, timerPeriod * 90.0);
  assert(test.numAlarms > 0);

  // Calculate the actual period of the timer
  const double duration = test.lastAlarmTime - test.firstAlarmTime;
  const double expected = roundPeriod(timerPeriod);
  const double actual   = roundPeriod(duration / (double)(test.numAlarms - 1));
  const double difference = fabs(actual - expected);

  if (difference > tolerance) {
    fprintf(stderr, "error: Period not within %f of %f\n", tolerance, expected);
    fprintf(stderr, "note: Actual period %f\n", actual);
  }

  assert(difference <= tolerance);

  // Deregister timer and tick once to synchronize
  assert(!puglStopTimer(test.view, timerId));
  puglUpdate(test.world, 0.0);

  // Update for a half second and check that we receive no more alarms
  test.numAlarms = 0;
  puglUpdate(test.world, 0.5);
  assert(test.numAlarms == 0);

  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
