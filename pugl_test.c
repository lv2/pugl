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

/*
  This program uses only the the Pugl and OpenGL APIs, but the Windows
  gl.h is broken and requires windows.h to be included first...
*/
#ifdef _WIN32
#    include <windows.h>
#endif

#include <stdio.h>

#include "GL/gl.h"
#include "pugl/pugl.h"

static int   quit   = 0;
static float xAngle = 0.0f;
static float yAngle = 0.0f;

#define KEY_ESCAPE 27

static void
onDisplay(PuglWindow* win)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glRotatef(xAngle, 1.0f, 0.0f, 0.0f);
	glRotatef(xAngle, 0.0f, 1.0f, 0.0f);

	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();
}

static void
onKeyboard(PuglWindow* win, bool press, uint32_t key)
{
	fprintf(stderr, "Key %c %s\n", (char)key, press ? "down" : "up");
	if (key == 'q' || key == 'Q' || key == KEY_ESCAPE) {
		quit = 1;
	}
}

static void
onMotion(PuglWindow* win, int x, int y)
{
	xAngle = x % 360;
	yAngle = y % 360;
	puglPostRedisplay(win);
}

static void
onMouse(PuglWindow* handle, int button, bool press, int x, int y)
{
	fprintf(stderr, "Mouse %d %s at %d,%d\n",
	        button, press ? "down" : "up", x, y);
}

static void
onClose(PuglWindow* win)
{
	quit = 1;
}

int
main(int argc, char** argv)
{
	PuglWindow* win = puglCreate(0, "Pugl Test", 512, 512);
	puglSetKeyboardFunc(win, onKeyboard);
	puglSetMotionFunc(win, onMotion);
	puglSetMouseFunc(win, onMouse);
	puglSetDisplayFunc(win, onDisplay);
	puglSetCloseFunc(win, onClose);

	while (!quit) {
		puglProcessEvents(win);
	}

	puglDestroy(win);
	return 0;
}
