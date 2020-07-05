/*
  Copyright 2020 David Robillard <d@drobilla.net>

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

/*
  Tests that update events are received and than redisplays they trigger happen
  immediately in the same event loop iteration.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/pugl_stub.h"

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

#ifdef _WIN32
// Windows SetTimer has a maximum resolution of 10ms
static const double tolerance = 0.011;
#else
static const double tolerance = 0.002;
#endif

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
	State           state;
} PuglTest;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
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
		assert(event->timer.id == timerId);
		++test->numAlarms;
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
	PuglTest app = {puglNewWorld(PUGL_PROGRAM, 0),
	                NULL,
	                puglParseTestOptions(&argc, &argv),
	                0,
	                START};

	// Set up view
	app.view = puglNewView(app.world);
	puglSetClassName(app.world, "Pugl Test");
	puglSetBackend(app.view, puglStubBackend());
	puglSetHandle(app.view, &app);
	puglSetEventFunc(app.view, onEvent);
	puglSetDefaultSize(app.view, 512, 512);

	// Create and show window
	assert(!puglRealize(app.view));
	assert(!puglShowWindow(app.view));
	while (app.state != EXPOSED) {
		assert(!puglUpdate(app.world, timeout));
	}

	// Register a timer with a longer period first
	assert(!puglStartTimer(app.view, timerId, timerPeriod * 2.0));

	// Replace it with the one we want (to ensure timers are replaced)
	assert(!puglStartTimer(app.view, timerId, timerPeriod));

	const double startTime = puglGetTime(app.world);

	puglUpdate(app.world, 1.0);

	// Calculate the actual period of the timer
	const double endTime        = puglGetTime(app.world);
	const double duration       = endTime - startTime;
	const double expectedPeriod = roundPeriod(timerPeriod);
	const double actualPeriod   = roundPeriod(duration / (double)app.numAlarms);
	const double difference     = fabs(actualPeriod - expectedPeriod);

	if (difference > tolerance) {
		fprintf(stderr,
		        "error: Period not within %f of %f\n",
		        tolerance,
		        expectedPeriod);
		fprintf(stderr, "note: Actual period %f\n", actualPeriod);
	}

	assert(difference <= tolerance);

	// Deregister timer and tick once to synchronize
	assert(!puglStopTimer(app.view, timerId));
	puglUpdate(app.world, 0.0);

	// Update for a half second and check that we receive no more alarms
	app.numAlarms = 0;
	puglUpdate(app.world, 0.5);
	assert(app.numAlarms == 0);

	puglFreeView(app.view);
	puglFreeWorld(app.world);

	return 0;
}
