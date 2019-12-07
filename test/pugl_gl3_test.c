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
   @file pugl_gl3_test.c A simple test of OpenGL 3 with Pugl.

   This is an example of using OpenGL for pixel-perfect 2D drawing.  It uses
   pixel coordinates for positions and sizes so that things work roughly like a
   typical 2D graphics API.

   The program draws a bunch of rectangles with borders, with one draw call per
   rectangle (the shader draws the borders).  Rectangle attributes are
   controlled via uniform variables.  This is certainly not the fastest way to
   do this: it is probably CPU and/or I/O bound, but serves as a decent very
   rough benchmark for how many draw calls you can get away with.

   A better (if slightly more GPU memory intensive) way to do this would be to
   put everything in vertex attributes, jam all the rectangle data into a
   single buffer, and draw the whole thing with a single draw call.  That way
   would probably be GPU bound instead, and show a difference between alpha
   blending and depth testing for many overlapped rectangles.
*/

#define GL_SILENCE_DEPRECATION 1

#include "shader_utils.h"
#include "test_utils.h"

#include "glad/glad.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl_gl.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int defaultWidth  = 512;
static const int defaultHeight = 512;

typedef struct
{
	float pos[2];
	float size[2];
	float fillColor[4];
} Rect;

// clang-format off
static const GLfloat rectVertices[] = {
	0.0f, 0.0f, // TL
	1.0f, 0.0f, // TR
	0.0f, 1.0f, // BL
	1.0f, 1.0f, // BR
};
// clang-format on

static const GLuint rectIndices[4] = {0, 1, 2, 3};

typedef struct
{
	PuglTestOptions opts;
	PuglWorld*      world;
	PuglView*       view;
	size_t          numRects;
	Rect*           rects;
	Program         drawRect;
	GLuint          vao;
	GLuint          vbo;
	GLuint          ibo;
	GLint           u_MVP;
	GLint           u_size;
	GLint           u_fillColor;
	unsigned        framesDrawn;
	int             quit;
} PuglTestApp;

static void
onConfigure(PuglView* view, double width, double height)
{
	(void)view;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, (int)width, (int)height);
}

static void
drawRect(const PuglTestApp* app, const Rect* rect, mat4 projection)
{
	/* The vertex data is always the same: a normalized rectangle from (0, 0)
	   to (1, 1).  We use the MVP matrix to scale and translate this to the
	   desired screen coordinates. */

	// Construct model matrix to scale/translate to screen coordinates
	mat4 m;
	mat4Identity(m);
	mat4Translate(m, rect->pos[0], rect->pos[1], 0);
	m[0][0] = rect->size[0];
	m[1][1] = rect->size[1];

	// Combine them into the final MVP matrix and set uniform
	mat4 mvp;
	mat4Mul(mvp, projection, m);
	glUniformMatrix4fv(app->u_MVP, 1, GL_FALSE, (const GLfloat*)&mvp);

	// Set uniforms for the various rectangle attributes
	glUniform2fv(app->u_size, 1, rect->size);
	glUniform4fv(app->u_fillColor, 1, rect->fillColor);

	// Draw
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);
}

static void
onExpose(PuglView* view)
{
	PuglTestApp*   app   = (PuglTestApp*)puglGetHandle(view);
	const PuglRect frame = puglGetFrame(view);
	const double   time  = puglGetTime(puglGetWorld(view));

	// Construct projection matrix for 2D window surface (in pixels)
	mat4 proj;
	mat4Ortho(proj,
	          0.0f,
	          (float)frame.width,
	          0.0f,
	          (float)frame.height,
	          -1.0f,
	          1.0f);

	// Clear and bind everything that is the same for every rect
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(app->drawRect.program);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->ibo);

	for (size_t i = 0; i < app->numRects; ++i) {
		Rect* rect = &app->rects[i];

		// Move rect around in an arbitrary way that looks cool
		rect->pos[0] = (float)(frame.width - rect->size[0]) *
		               (sinf((float)time * rect->size[0] / 64.0f) + 1.0f) /
		               2.0f;
		rect->pos[1] = (float)(frame.height - rect->size[1]) *
		               (cosf((float)time * rect->size[1] / 64.0f) + 1.0f) /
		               2.0f;

		drawRect(app, rect, proj);
	}

	++app->framesDrawn;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	printEvent(event, "Event: ", app->opts.verbose);

	switch (event->type) {
	case PUGL_CONFIGURE:
		onConfigure(view, event->configure.width, event->configure.height);
		break;
	case PUGL_EXPOSE: onExpose(view); break;
	case PUGL_CLOSE: app->quit = 1; break;
	case PUGL_KEY_PRESS:
		if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
			app->quit = 1;
		}
		break;
	default: break;
	}

	return PUGL_SUCCESS;
}

static Rect*
makeRects(const size_t numRects)
{
	const float minSize  = (float)defaultWidth / 64.0f;
	const float maxSize  = (float)defaultWidth / 6.0f;
	const float boxAlpha = 0.25f;

	Rect* rects = (Rect*)calloc(numRects, sizeof(Rect));
	for (size_t i = 0; i < numRects; ++i) {
		const float s = (sinf(i) / 2.0f + 0.5f);
		const float c = (cosf(i) / 2.0f + 0.5f);

		rects[i].size[0]      = minSize + s * maxSize;
		rects[i].size[1]      = minSize + c * maxSize;
		rects[i].fillColor[1] = s / 2.0f + 0.25f;
		rects[i].fillColor[2] = c / 2.0f + 0.25f;
		rects[i].fillColor[3] = boxAlpha;
	}

	return rects;
}

static char*
loadShader(const char* const path)
{
	FILE* const file = fopen(path, "r");
	if (!file) {
		logError("Failed to open '%s'\n", path);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	const long fileSize = ftell(file);

	fseek(file, 0, SEEK_SET);
	char* source = (char*)calloc(1, fileSize + 1);

	fread(source, 1, fileSize, file);
	fclose(file);

	return source;
}

int
main(int argc, char** argv)
{
	PuglTestApp app;
	memset(&app, 0, sizeof(app));

	const PuglRect frame = {0, 0, defaultWidth, defaultHeight};

	// Parse command line options
	app.numRects = 1024;
	app.opts     = puglParseTestOptions(&argc, &argv);
	if (app.opts.help) {
		puglPrintTestUsage("pugl_gl3_test", "[NUM_RECTS]");
		return 1;
	}

	// Parse number of rectangles argument, if given
	if (argc == 1) {
		char* endptr = NULL;
		app.numRects = (size_t)strtol(argv[0], &endptr, 10);
		if (endptr != argv[0] + strlen(argv[0])) {
			puglPrintTestUsage("pugl_gl3_test", "[NUM_RECTS]");
			return 1;
		}
	}

	// Create world, view, and rect data
	app.world = puglNewWorld();
	app.view  = puglNewView(app.world);
	app.rects = makeRects(app.numRects);

	// Set up world and view
	puglSetClassName(app.world, "PuglGL3Test");
	puglSetFrame(app.view, frame);
	puglSetMinSize(app.view, defaultWidth / 4, defaultHeight / 4);
	puglSetAspectRatio(app.view, 1, 1, 16, 9);
	puglSetBackend(app.view, puglGlBackend());
	puglSetViewHint(app.view, PUGL_USE_COMPAT_PROFILE, PUGL_FALSE);
	puglSetViewHint(app.view, PUGL_USE_DEBUG_CONTEXT, app.opts.errorChecking);
	puglSetViewHint(app.view, PUGL_CONTEXT_VERSION_MAJOR, 3);
	puglSetViewHint(app.view, PUGL_CONTEXT_VERSION_MINOR, 3);
	puglSetViewHint(app.view, PUGL_RESIZABLE, app.opts.resizable);
	puglSetViewHint(app.view, PUGL_SAMPLES, app.opts.samples);
	puglSetViewHint(app.view, PUGL_DOUBLE_BUFFER, app.opts.doubleBuffer);
	puglSetViewHint(app.view, PUGL_SWAP_INTERVAL, app.opts.doubleBuffer);
	puglSetViewHint(app.view, PUGL_IGNORE_KEY_REPEAT, PUGL_TRUE);
	puglSetHandle(app.view, &app);
	puglSetEventFunc(app.view, onEvent);

	const PuglStatus st = puglCreateWindow(app.view, "Pugl OpenGL 3");
	if (st) {
		return logError("Failed to create window (%s)\n", puglStrerror(st));
	}

	// Enter context to set up GL stuff
	puglEnterContext(app.view, false);

	// Load GL functions via GLAD
	if (!gladLoadGLLoader((GLADloadproc)&puglGetProcAddress)) {
		logError("Failed to load GL\n");
		puglFreeView(app.view);
		puglFreeWorld(app.world);
		return 1;
	}

	// Load shader sources
	char* const vertexSource   = loadShader("shaders/rect.vert");
	char* const fragmentSource = loadShader("shaders/rect.frag");
	if (!vertexSource || !fragmentSource) {
		logError("Failed to load shader sources\n");
		puglFreeView(app.view);
		puglFreeWorld(app.world);
		return 1;
	}

	// Compile rectangle shaders and program
	app.drawRect = compileProgram(vertexSource, fragmentSource);
	free(fragmentSource);
	free(vertexSource);
	if (!app.drawRect.program) {
		puglFreeView(app.view);
		puglFreeWorld(app.world);
		return 1;
	}

	// Get location of rectangle shader uniforms
	app.u_MVP       = glGetUniformLocation(app.drawRect.program, "MVP");
	app.u_size      = glGetUniformLocation(app.drawRect.program, "u_size");
	app.u_fillColor = glGetUniformLocation(app.drawRect.program, "u_fillColor");

	// Generate/bind a VAO to track state
	glGenVertexArrays(1, &app.vao);
	glBindVertexArray(app.vao);

	// Generate/bind a VBO to store vertex position data
	glGenBuffers(1, &app.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, app.vbo);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(rectVertices),
	             rectVertices,
	             GL_STATIC_DRAW);

	// Set up the first/only attribute, position, as 2 floats from the VBO
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

	// Set up the IBO to index into the VBO
	glGenBuffers(1, &app.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             sizeof(rectIndices),
	             rectIndices,
	             GL_STATIC_DRAW);

	// Finally ready to go, leave GL context and show the window
	puglLeaveContext(app.view, false);
	puglShowWindow(app.view);

	// Grind away, drawing continuously
	PuglFpsPrinter fpsPrinter = {puglGetTime(app.world)};
	while (!app.quit) {
		puglPostRedisplay(app.view);
		puglDispatchEvents(app.world);
		puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
	}

	// Delete GL stuff
	puglEnterContext(app.view, false);
	glDeleteBuffers(1, &app.ibo);
	glDeleteBuffers(1, &app.vbo);
	glDeleteVertexArrays(1, &app.vao);
	deleteProgram(app.drawRect);
	puglLeaveContext(app.view, false);

	// Tear down view and world
	puglFreeView(app.view);
	puglFreeWorld(app.world);
	return 0;
}
