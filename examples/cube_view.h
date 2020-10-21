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

#ifndef EXAMPLES_CUBE_VIEW_H
#define EXAMPLES_CUBE_VIEW_H

#define GL_SILENCE_DEPRECATION 1

#include "demo_utils.h"

#include "pugl/gl.h"

// clang-format off

static const float cubeStripVertices[] = {
	-1.0f,  1.0f,  1.0f, // Front top left
	 1.0f,  1.0f,  1.0f, // Front top right
	-1.0f, -1.0f,  1.0f, // Front bottom left
	 1.0f, -1.0f,  1.0f, // Front bottom right
	 1.0f, -1.0f, -1.0f, // Back bottom right
	 1.0f,  1.0f,  1.0f, // Front top right
	 1.0f,  1.0f, -1.0f, // Back top right
	-1.0f,  1.0f,  1.0f, // Front top left
	-1.0f,  1.0f, -1.0f, // Back top left
	-1.0f, -1.0f,  1.0f, // Front bottom left
	-1.0f, -1.0f, -1.0f, // Back bottom left
	 1.0f, -1.0f, -1.0f, // Back bottom right
	-1.0f,  1.0f, -1.0f, // Back top left
	 1.0f,  1.0f, -1.0f  // Back top right
};

static const float cubeFrontLineLoop[] = {
	-1.0f,  1.0f,  1.0f, // Front top left
	 1.0f,  1.0f,  1.0f, // Front top right
	 1.0f, -1.0f,  1.0f, // Front bottom right
	-1.0f, -1.0f,  1.0f, // Front bottom left
};

static const float cubeBackLineLoop[] = {
	-1.0f,  1.0f, -1.0f, // Back top left
	 1.0f,  1.0f, -1.0f, // Back top right
	 1.0f, -1.0f, -1.0f, // Back bottom right
	-1.0f, -1.0f, -1.0f, // Back bottom left
};

static const float cubeSideLines[] = {
	-1.0f,  1.0f,  1.0f, // Front top left
	-1.0f,  1.0f, -1.0f, // Back top left

	-1.0f, -1.0f,  1.0f, // Front bottom left
	-1.0f, -1.0f, -1.0f, // Back bottom left

	 1.0f,  1.0f,  1.0f, // Front top right
	 1.0f,  1.0f, -1.0f, // Back top right

	 1.0f, -1.0f,  1.0f, // Front bottom right
	 1.0f, -1.0f, -1.0f, // Back bottom right
};

// clang-format on

static inline void
reshapeCube(const float width, const float height)
{
	const float aspect = width / height;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, (int)width, (int)height);

	float projection[16];
	perspective(projection, 1.8f, aspect, 1.0f, 100.0f);
	glLoadMatrixf(projection);
}

static inline void
displayCube(PuglView* const view,
            const float     distance,
            const float     xAngle,
            const float     yAngle,
            const bool      entered)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, distance * -1.0f);
	glRotatef(xAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(yAngle, 1.0f, 0.0f, 0.0f);

	const float bg = entered ? 0.2f : 0.0f;
	glClearColor(bg, bg, bg, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (puglHasFocus(view)) {
		// Draw cube surfaces
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, cubeStripVertices);
		glColorPointer(3, GL_FLOAT, 0, cubeStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		glColor3f(0.0f, 0.0f, 0.0f);
	} else {
		glColor3f(1.0f, 1.0f, 1.0f);
	}

	// Draw cube wireframe
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, cubeFrontLineLoop);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glVertexPointer(3, GL_FLOAT, 0, cubeBackLineLoop);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glVertexPointer(3, GL_FLOAT, 0, cubeSideLines);
	glDrawArrays(GL_LINES, 0, 8);
	glDisableClientState(GL_VERTEX_ARRAY);
}

#endif // EXAMPLES_CUBE_VIEW_H
