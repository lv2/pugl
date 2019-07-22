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

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool     continuous   = false;
static int      quit         = 0;
static float    xAngle       = 0.0f;
static float    yAngle       = 0.0f;
static float    dist         = 10.0f;
static double   lastMouseX   = 0.0;
static double   lastMouseY   = 0.0;
static float    lastDrawTime = 0.0;
static unsigned framesDrawn  = 0;
static bool     mouseEntered = false;

static const float cubeVertices[] = {
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,

	 1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,

	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,

	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,

	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

/** Calculate a projection matrix for a given perspective. */
static void
perspective(float* m, float fov, float aspect, float zNear, float zFar)
{
	const float h     = tanf(fov);
	const float w     = h / aspect;
	const float depth = zNear - zFar;
	const float q     = (zFar + zNear) / depth;
	const float qn    = 2 * zFar * zNear / depth;

	m[0]  = w;  m[1]  = 0;  m[2]  = 0;   m[3]  = 0;
	m[4]  = 0;  m[5]  = h;  m[6]  = 0;   m[7]  = 0;
	m[8]  = 0;  m[9]  = 0;  m[10] = q;   m[11] = -1;
	m[12] = 0;  m[13] = 0;  m[14] = qn;  m[15] = 0;
}

static void
onReshape(PuglView* view, int width, int height)
{
	(void)view;

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
	const float thisTime = (float)puglGetTime(view);
	if (continuous) {
		xAngle = fmodf(xAngle + (thisTime - lastDrawTime) * 100.0f, 360.0f);
		yAngle = fmodf(yAngle + (thisTime - lastDrawTime) * 100.0f, 360.0f);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, dist * -1);
	glRotatef(xAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(yAngle, 1.0f, 0.0f, 0.0f);

	const float bg = mouseEntered ? 0.2f : 0.0f;
	glClearColor(bg, bg, bg, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, cubeVertices);
	glColorPointer(3, GL_FLOAT, 0, cubeVertices);
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	lastDrawTime = thisTime;
	++framesDrawn;
}

static int
printModifiers(const uint32_t mods)
{
	return fprintf(stderr, "Modifiers:%s%s%s%s\n",
	               (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	               (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	               (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	               (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static int
printEvent(const PuglEvent* event, const char* prefix)
{
	switch (event->type) {
	case PUGL_KEY_PRESS:
		return fprintf(stderr, "%sKey %u (char U+%04X special U+%04X) press (%s)%s\n",
		               prefix,
		               event->key.keycode, event->key.character, event->key.special,
		               event->key.utf8, event->key.filter ? " (filtered)" : "");

	case PUGL_KEY_RELEASE:
		return fprintf(stderr, "%sKey %u (char U+%04X special U+%04X) release (%s)%s\n",
		               prefix,
		               event->key.keycode, event->key.character, event->key.special,
		               event->key.utf8, event->key.filter ? " (filtered)" : "");
	case PUGL_BUTTON_PRESS:
	case PUGL_BUTTON_RELEASE:
		return (fprintf(stderr, "%sMouse %d %s at %f,%f ",
		                prefix,
		                event->button.button,
		                (event->type == PUGL_BUTTON_PRESS) ? "down" : "up",
		                event->button.x,
		                event->button.y) +
		        printModifiers(event->scroll.state));
	case PUGL_SCROLL:
		return (fprintf(stderr, "%sScroll %f %f %f %f ",
		                prefix,
		                event->scroll.x, event->scroll.y,
		                event->scroll.dx, event->scroll.dy) +
		        printModifiers(event->scroll.state));
	case PUGL_ENTER_NOTIFY:
		return fprintf(stderr, "%sMouse enter at %f,%f\n",
		               prefix, event->crossing.x, event->crossing.y);
	case PUGL_LEAVE_NOTIFY:
		return fprintf(stderr, "%sMouse leave at %f,%f\n",
		               prefix, event->crossing.x, event->crossing.y);
	case PUGL_FOCUS_IN:
		return fprintf(stderr, "%sFocus in%s\n",
		               prefix, event->focus.grab ? " (grab)" : "");
	case PUGL_FOCUS_OUT:
		return fprintf(stderr, "%sFocus out%s\n",
		               prefix, event->focus.grab ? " (ungrab)" : "");
	default: break;
	}

	return 0;
}

static void
onEvent(PuglView* view, const PuglEvent* event)
{
	printEvent(event, "Event: ");

	switch (event->type) {
	case PUGL_CONFIGURE:
		onReshape(view, (int)event->configure.width, (int)event->configure.height);
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		quit = 1;
		break;
	case PUGL_KEY_PRESS:
		if (event->key.character == 'q' ||
		    event->key.character == 'Q' ||
		    event->key.character == PUGL_CHAR_ESCAPE) {
			quit = 1;
		}
		break;
	case PUGL_MOTION_NOTIFY:
		xAngle = fmodf(xAngle - (float)(event->motion.x - lastMouseX), 360.0f);
		yAngle = fmodf(yAngle + (float)(event->motion.y - lastMouseY), 360.0f);
		lastMouseX = event->motion.x;
		lastMouseY = event->motion.y;
		puglPostRedisplay(view);
		break;
	case PUGL_SCROLL:
		dist += (float)event->scroll.dy;
		if (dist < 10.0f) {
			dist = 10.0f;
		}
		puglPostRedisplay(view);
		break;
	case PUGL_ENTER_NOTIFY:
		mouseEntered = true;
		break;
	case PUGL_LEAVE_NOTIFY:
		mouseEntered = false;
		break;
	default:
		break;
	}
}

int
main(int argc, char** argv)
{
	int  samples         = 0;
	int  doubleBuffer    = PUGL_FALSE;
	bool ignoreKeyRepeat = false;
	bool resizable       = false;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-a")) {
			samples = 4;
		} else if (!strcmp(argv[i], "-c")) {
			continuous = true;
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

	setlocale(LC_CTYPE, "");

	PuglView* view = puglInit(NULL, NULL);
	puglInitWindowClass(view, "PuglTest");
	puglInitWindowSize(view, 512, 512);
	puglInitWindowMinSize(view, 256, 256);
	puglInitWindowAspectRatio(view, 1, 1, 1, 1);
	puglInitResizable(view, resizable);

	puglInitWindowHint(view, PUGL_SAMPLES, samples);
	puglInitWindowHint(view, PUGL_DOUBLE_BUFFER, doubleBuffer);

	puglIgnoreKeyRepeat(view, ignoreKeyRepeat);
	puglSetEventFunc(view, onEvent);

	if (puglCreateWindow(view, "Pugl Test")) {
		return 1;
	}

	puglEnterContext(view);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	puglLeaveContext(view, false);

	puglShowWindow(view);

	float lastReportTime     = (float)puglGetTime(view);
	bool  requestedAttention = false;
	while (!quit) {
		const float thisTime = (float)puglGetTime(view);

		if (continuous) {
			puglPostRedisplay(view);
		} else {
			puglWaitForEvent(view);
		}

		puglProcessEvents(view);

		if (!requestedAttention && thisTime > 5) {
			puglRequestAttention(view);
			requestedAttention = true;
		}

		if (continuous && thisTime > lastReportTime + 5) {
			const double fps = framesDrawn / (thisTime - lastReportTime);
			fprintf(stderr,
			        "%u frames in %.0f seconds = %.3f FPS\n",
			        framesDrawn, thisTime - lastReportTime, fps);

			lastReportTime = thisTime;
			framesDrawn    = 0;
		}
	}

	puglDestroy(view);
	return 0;
}
