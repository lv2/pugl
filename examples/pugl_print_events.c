/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

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

/**
   @file pugl_print_events.c
   @brief A utility program that prints view events.
*/

#include "test/test_utils.h"

#include "pugl/pugl.h"
#include "pugl/pugl_stub.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct
{
	PuglWorld* world;
	PuglView*  view;
	int        quit;
} PuglPrintEventsApp;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglPrintEventsApp* app = (PuglPrintEventsApp*)puglGetHandle(view);

	printEvent(event, "Event: ", true);

	switch (event->type) {
	case PUGL_CLOSE: app->quit = 1; break;
	default: break;
	}

	return PUGL_SUCCESS;
}

int
main(void)
{
	PuglPrintEventsApp app = {NULL, NULL, 0};

	app.world = puglNewWorld(PUGL_PROGRAM, 0);
	app.view  = puglNewView(app.world);

	puglSetClassName(app.world, "Pugl Print Events");
	puglSetWindowTitle(app.view, "Pugl Event Printer");
	puglSetBackend(app.view, puglStubBackend());
	puglSetHandle(app.view, &app);
	puglSetEventFunc(app.view, onEvent);

	if (puglRealize(app.view)) {
		return logError("Failed to create window\n");
	}

	puglShowWindow(app.view);

	while (!app.quit) {
		puglUpdate(app.world, -1.0);
	}

	puglFreeView(app.view);
	puglFreeWorld(app.world);

	return 0;
}
