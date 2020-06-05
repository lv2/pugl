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
   @file pugl_cursors_demo.c
   @brief An example of changing the mouse cursor.
*/

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/pugl.h"
#include "pugl/pugl_gl.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
	PuglTestOptions opts;
	PuglWorld*      world;
	bool            continuous;
	int             quit;
	bool            verbose;
} PuglTestApp;

enum {
	NUM_CURSORS = 13,
	ROWS        = 4,
	COLUMNS     = 4,
};

static inline void
reshape(const float width, const float height)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, (int)width, (int)height);
}

static void
onDisplay(PuglView* view)
{
	(void)view;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(0.6f, 0.6f, 0.6f);

	for (int row = 1; row < ROWS; ++row) {
		const float y = row * (2.0f / ROWS) - 1.0f;
		glBegin(GL_LINES);
		glVertex2f(-1.0f, y);
		glVertex2f(1.0f, y);
		glEnd();
	}

	for (int column = 1; column < COLUMNS; ++column) {
		const float x = column * (2.0f / COLUMNS) - 1.0f;
		glBegin(GL_LINES);
		glVertex2f(x, -1.0f);
		glVertex2f(x, 1.0f);
		glEnd();
	}
}

static void
onMotion(PuglView* view, double x, double y)
{
	const PuglRect frame  = puglGetFrame(view);
	int            row    = (int)(y * ROWS / frame.height);
	int            column = (int)(x * COLUMNS / frame.width);
	PuglCursor     cursor;

	row    = (row < 0) ? 0 : (row >= ROWS) ? (ROWS - 1) : row;
	column = (column < 0) ? 0 : (column >= COLUMNS) ? (COLUMNS - 1) : column;

	cursor = (PuglCursor)((row * COLUMNS + column) % NUM_CURSORS);
	puglSetCursor(view, cursor);
}

static void
onClose(PuglView* view)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	app->quit = 1;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	printEvent(event, "Event: ", app->opts.verbose);

	switch (event->type) {
	case PUGL_CONFIGURE:
		reshape((float)event->configure.width, (float)event->configure.height);
		break;
	case PUGL_KEY_PRESS:
		if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
			app->quit = 1;
		}
		break;
	case PUGL_MOTION:
		onMotion(view, event->motion.x, event->motion.y);
		break;
	case PUGL_UPDATE:
		if (app->opts.continuous) {
			puglPostRedisplay(view);
		}
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		onClose(view);
		break;
	default:
		break;
	}

	return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
	PuglTestApp app = {0};

	app.opts = puglParseTestOptions(&argc, &argv);
	if (app.opts.help) {
		puglPrintTestUsage(argv[0], "");
		return 1;
	}

	app.world = puglNewWorld(PUGL_PROGRAM, 0);

	puglSetWorldHandle(app.world, &app);
	puglSetClassName(app.world, "Pugl Test");

	PuglView* view = puglNewView(app.world);

	puglSetWindowTitle(view, "Pugl Window Demo");
	puglSetDefaultSize(view, 512, 512);
	puglSetMinSize(view, 128, 128);
	puglSetBackend(view, puglGlBackend());

	puglSetViewHint(view, PUGL_USE_DEBUG_CONTEXT, app.opts.errorChecking);
	puglSetViewHint(view, PUGL_RESIZABLE, app.opts.resizable);
	puglSetViewHint(view, PUGL_SAMPLES, app.opts.samples);
	puglSetViewHint(view, PUGL_DOUBLE_BUFFER, app.opts.doubleBuffer);
	puglSetViewHint(view, PUGL_SWAP_INTERVAL, app.opts.sync);
	puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, app.opts.ignoreKeyRepeat);
	puglSetHandle(view, &app);
	puglSetEventFunc(view, onEvent);

	PuglStatus st = puglRealize(view);
	if ((st = puglRealize(view))) {
		return logError("Failed to create window (%s)\n", puglStrerror(st));
	}

	puglShowWindow(view);

	PuglFpsPrinter fpsPrinter  = {puglGetTime(app.world)};
	unsigned       framesDrawn = 0;
	while (!app.quit) {
		puglUpdate(app.world, app.continuous ? 0.0 : -1.0);
		++framesDrawn;

		if (app.continuous) {
			puglPrintFps(app.world, &fpsPrinter, &framesDrawn);
		}
	}

	puglFreeView(view);

	puglFreeWorld(app.world);

	return 0;
}
