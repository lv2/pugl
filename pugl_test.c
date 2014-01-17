/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#include <stdio.h>
#include <string.h>

#include "pugl/pugl.h"

// Argh!
#ifdef __APPLE__
#    include <OpenGL/glu.h>
#else
#    include <GL/glu.h>
#endif

static int   quit   = 0;
static float xAngle = 0.0f;
static float yAngle = 0.0f;
static float dist   = 10.0f;

static void
onReshape(PuglView* view, int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);
	gluPerspective(45.0f, width/(float)height, 1.0f, 10.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void
onDisplay(PuglView* view)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, dist * -1);
	glRotatef(xAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(yAngle, 1.0f, 0.0f, 0.0f);

	glBegin(GL_QUADS);

	glColor3f(0, 0, 0); glVertex3f(-1, -1, -1);
	glColor3f(0, 0, 1); glVertex3f(-1, -1,  1);
	glColor3f(0, 1, 1); glVertex3f(-1,  1,  1);
	glColor3f(0, 1, 0); glVertex3f(-1,  1, -1);

	glColor3f(1, 0, 0); glVertex3f( 1, -1, -1);
	glColor3f(1, 0, 1); glVertex3f( 1, -1,  1);
	glColor3f(1, 1, 1); glVertex3f( 1,  1,  1);
	glColor3f(1, 1, 0); glVertex3f( 1,  1, -1);

	glColor3f(0, 0, 0); glVertex3f(-1, -1, -1);
	glColor3f(0, 0, 1); glVertex3f(-1, -1,  1);
	glColor3f(1, 0, 1); glVertex3f( 1, -1,  1);
	glColor3f(1, 0, 0); glVertex3f( 1, -1, -1);

	glColor3f(0, 1, 0); glVertex3f(-1,  1, -1);
	glColor3f(0, 1, 1); glVertex3f(-1,  1,  1);
	glColor3f(1, 1, 1); glVertex3f( 1,  1,  1);
	glColor3f(1, 1, 0); glVertex3f( 1,  1, -1);

	glColor3f(0, 0, 0); glVertex3f(-1, -1, -1);
	glColor3f(0, 1, 0); glVertex3f(-1,  1, -1);
	glColor3f(1, 1, 0); glVertex3f( 1,  1, -1);
	glColor3f(1, 0, 0); glVertex3f( 1, -1, -1);

	glColor3f(0, 0, 1); glVertex3f(-1, -1,  1);
	glColor3f(0, 1, 1); glVertex3f(-1,  1,  1);
	glColor3f(1, 1, 1); glVertex3f( 1,  1,  1);
	glColor3f(1, 0, 1); glVertex3f( 1, -1,  1);

	glEnd();
}

static void
printModifiers(PuglView* view)
{
	int mods = puglGetModifiers(view);
	fprintf(stderr, "Modifiers:%s%s%s%s\n",
	        (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	        (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	        (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	        (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static void
onKeyboard(PuglView* view, bool press, uint32_t key)
{
	fprintf(stderr, "Key %c %s ", (char)key, press ? "down" : "up");
	printModifiers(view);
	if (key == 'q' || key == 'Q' || key == PUGL_CHAR_ESCAPE ||
	    key == PUGL_CHAR_DELETE || key == PUGL_CHAR_BACKSPACE) {
		quit = 1;
	}
}

static void
onSpecial(PuglView* view, bool press, PuglKey key)
{
	fprintf(stderr, "Special key %d %s ", key, press ? "down" : "up");
	printModifiers(view);
}

static void
onMotion(PuglView* view, int x, int y)
{
	xAngle = x % 360;
	yAngle = y % 360;
	puglPostRedisplay(view);
}

static void
onMouse(PuglView* view, int button, bool press, int x, int y)
{
	fprintf(stderr, "Mouse %d %s at %d,%d ",
	        button, press ? "down" : "up", x, y);
	printModifiers(view);
}

static void
onScroll(PuglView* view, int x, int y, float dx, float dy)
{
	fprintf(stderr, "Scroll %d %d %f %f ", x, y, dx, dy);
	printModifiers(view);
	dist += dy / 4.0f;
	puglPostRedisplay(view);
}

static void
onClose(PuglView* view)
{
	quit = 1;
}

int
main(int argc, char** argv)
{
	bool ignoreKeyRepeat = false;
	bool resizable       = false;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h")) {
			printf("USAGE: %s [OPTIONS]...\n\n"
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

	PuglView* view = puglCreate(0, "Pugl Test", 512, 512, resizable, true);
	puglIgnoreKeyRepeat(view, ignoreKeyRepeat);
	puglSetKeyboardFunc(view, onKeyboard);
	puglSetMotionFunc(view, onMotion);
	puglSetMouseFunc(view, onMouse);
	puglSetScrollFunc(view, onScroll);
	puglSetSpecialFunc(view, onSpecial);
	puglSetDisplayFunc(view, onDisplay);
	puglSetReshapeFunc(view, onReshape);
	puglSetCloseFunc(view, onClose);

	while (!quit) {
		puglProcessEvents(view);
	}

	puglDestroy(view);
	return 0;
}
