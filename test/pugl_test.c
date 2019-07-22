/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

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
   @file pugl_test.c A simple Pugl test that creates a top-level window.
*/

#define GL_SILENCE_DEPRECATION 1

#include "test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl_gl_backend.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
	PuglWorld* world;
	bool       continuous;
	int        quit;
	float      xAngle;
	float      yAngle;
	float      dist;
	double     lastMouseX;
	double     lastMouseY;
	double     lastDrawTime;
	unsigned   framesDrawn;
	bool       mouseEntered;
} PuglTestApp;

static void
onReshape(PuglView* view, int width, int height)
{
	(void)view;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);

	float projection[16];
	perspective(projection, 1.8f, width / (float)height, 1.0, 100.0f);
	glLoadMatrixf(projection);
}

static void
onDisplay(PuglView* view)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	const double thisTime = puglGetTime(app->world);
	if (app->continuous) {
		const double dTime = thisTime - app->lastDrawTime;
		app->xAngle = fmodf((float)(app->xAngle + dTime * 100.0f), 360.0f);
		app->yAngle = fmodf((float)(app->yAngle + dTime * 100.0f), 360.0f);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, app->dist * -1);
	glRotatef(app->xAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(app->yAngle, 1.0f, 0.0f, 0.0f);

	const float bg = app->mouseEntered ? 0.2f : 0.0f;
	glClearColor(bg, bg, bg, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, cubeVertices);
	glColorPointer(3, GL_FLOAT, 0, cubeVertices);
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	app->lastDrawTime = thisTime;
	++app->framesDrawn;
}

static void
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	printEvent(event, "Event: ");

	switch (event->type) {
	case PUGL_CONFIGURE:
		onReshape(view, (int)event->configure.width, (int)event->configure.height);
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		app->quit = 1;
		break;
	case PUGL_KEY_PRESS:
		if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
			app->quit = 1;
		}
		break;
	case PUGL_MOTION_NOTIFY:
		app->xAngle = fmodf(app->xAngle - (float)(event->motion.x - app->lastMouseX), 360.0f);
		app->yAngle = fmodf(app->yAngle + (float)(event->motion.y - app->lastMouseY), 360.0f);
		app->lastMouseX = event->motion.x;
		app->lastMouseY = event->motion.y;
		puglPostRedisplay(view);
		break;
	case PUGL_SCROLL:
		app->dist = fmaxf(10.0f, app->dist + (float)event->scroll.dy);
		puglPostRedisplay(view);
		break;
	case PUGL_ENTER_NOTIFY:
		app->mouseEntered = true;
		break;
	case PUGL_LEAVE_NOTIFY:
		app->mouseEntered = false;
		break;
	default:
		break;
	}
}

int
main(int argc, char** argv)
{
	PuglTestApp app = {0};
	app.dist = 10;

	int  samples         = 0;
	int  doubleBuffer    = PUGL_FALSE;
	bool ignoreKeyRepeat = false;
	bool resizable       = false;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-a")) {
			samples = 4;
		} else if (!strcmp(argv[i], "-c")) {
			app.continuous = true;
		} else if (!strcmp(argv[i], "-d")) {
			doubleBuffer = PUGL_TRUE;
		} else if (!strcmp(argv[i], "-h")) {
			printf("USAGE: %s [OPTIONS]...\n\n"
			       "  -a  Enable anti-aliasing\n"
			       "  -c  Continuously animate and draw\n"
			       "  -d  Enable double-buffering\n"
			       "  -h  Display this help\n"
			       "  -i  Ignore key repeat\n"
			       "  -r  Resizable window\n", argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "-i")) {
			ignoreKeyRepeat = true;
		} else if (!strcmp(argv[i], "-r")) {
			resizable = true;
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
		}
	}

	app.world = puglNewWorld();

	PuglView* view = puglNewView(app.world);
	puglInitWindowClass(view, "PuglTest");
	puglInitWindowSize(view, 512, 512);
	puglInitWindowMinSize(view, 256, 256);
	puglInitWindowAspectRatio(view, 1, 1, 16, 9);
	puglInitBackend(view, puglGlBackend());

	puglInitWindowHint(view, PUGL_RESIZABLE, resizable);
	puglInitWindowHint(view, PUGL_SAMPLES, samples);
	puglInitWindowHint(view, PUGL_DOUBLE_BUFFER, doubleBuffer);

	puglInitWindowHint(view, PUGL_IGNORE_KEY_REPEAT, ignoreKeyRepeat);
	puglSetEventFunc(view, onEvent);
	puglSetHandle(view, &app);

	const uint8_t title[] = { 'P', 'u', 'g', 'l', ' ',
	                          'P', 'r', 0xC3, 0xBC, 'f', 'u', 'n', 'g', 0 };
	if (puglCreateWindow(view, (const char*)title)) {
		return 1;
	}

	puglShowWindow(view);

	PuglFpsPrinter fpsPrinter         = { puglGetTime(app.world) };
	bool           requestedAttention = false;
	while (!app.quit) {
		const double thisTime = puglGetTime(app.world);

		if (app.continuous) {
			puglPostRedisplay(view);
		} else {
			puglPollEvents(app.world, -1);
		}

		puglProcessEvents(view);

		if (!requestedAttention && thisTime > 5.0) {
			puglRequestAttention(view);
			requestedAttention = true;
		}

		if (app.continuous) {
			puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
		}
	}

	puglFreeView(view);
	puglFreeWorld(app.world);

	return 0;
}
