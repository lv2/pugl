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
   @file pugl_shader_demo.c
   @brief An example of drawing with OpenGL 3/4.

   This is an example of using OpenGL for pixel-perfect 2D drawing.  It uses
   pixel coordinates for positions and sizes so that things work roughly like a
   typical 2D graphics API.

   The program draws a bunch of rectangles with borders, using instancing.
   Each rectangle has origin, size, and fill color attributes, which are shared
   for all four vertices.  On each frame, a single buffer with all the
   rectangle data is sent to the GPU, and everything is drawn with a single
   draw call.

   This is not particularly realistic or optimal, but serves as a decent rough
   benchmark for how much simple geometry you can draw.  The number of
   rectangles can be given on the command line.  For reference, it begins to
   struggle to maintain 60 FPS on my machine (1950x + Vega64) with more than
   about 100000 rectangles.
*/

#include "demo_utils.h"
#include "rects.h"
#include "shader_utils.h"
#include "test/test_utils.h"

#include "glad/glad.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl_gl.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int defaultWidth  = 512;
static const int defaultHeight = 512;

typedef struct
{
	mat4 projection;
} RectUniforms;

typedef struct
{
	PuglWorld*      world;
	PuglView*       view;
	PuglTestOptions opts;
	size_t          numRects;
	Rect*           rects;
	Program         drawRect;
	GLuint          vao;
	GLuint          vbo;
	GLuint          instanceVbo;
	GLuint          ibo;
	unsigned        framesDrawn;
	int             glMajorVersion;
	int             glMinorVersion;
	int             quit;
} PuglTestApp;

static PuglStatus
setupGl(PuglTestApp* app);

static void
teardownGl(PuglTestApp* app);

static void
onConfigure(PuglView* view, double width, double height)
{
	(void)view;

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, (int)width, (int)height);
}

static void
onExpose(PuglView* view)
{
	PuglTestApp*   app    = (PuglTestApp*)puglGetHandle(view);
	const PuglRect frame  = puglGetFrame(view);
	const float    width  = (float)frame.width;
	const float    height = (float)frame.height;
	const double   time   = puglGetTime(puglGetWorld(view));

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

	for (size_t i = 0; i < app->numRects; ++i) {
		moveRect(&app->rects[i], i, app->numRects, width, height, time);
	}

	glBufferData(GL_UNIFORM_BUFFER, sizeof(proj), &proj, GL_STREAM_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER,
	                0,
	                (GLsizeiptr)(app->numRects * sizeof(Rect)),
	                app->rects);

	glDrawElementsInstanced(GL_TRIANGLE_STRIP,
	                        4,
	                        GL_UNSIGNED_INT,
	                        NULL,
	                        (GLsizei)(app->numRects * 4));

	++app->framesDrawn;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

	printEvent(event, "Event: ", app->opts.verbose);

	switch (event->type) {
	case PUGL_CREATE:
		setupGl(app);
		break;
	case PUGL_DESTROY:
		teardownGl(app);
		break;
	case PUGL_CONFIGURE:
		onConfigure(view, event->configure.width, event->configure.height);
		break;
	case PUGL_UPDATE:
		puglPostRedisplay(view);
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
	Rect* rects = (Rect*)calloc(numRects, sizeof(Rect));
	for (size_t i = 0; i < numRects; ++i) {
		rects[i] = makeRect(i, (float)defaultWidth);
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
	const size_t fileSize = (size_t)ftell(file);

	fseek(file, 0, SEEK_SET);
	char* source = (char*)calloc(1, fileSize + 1u);

	fread(source, 1, fileSize, file);
	fclose(file);

	return source;
}

static int
parseOptions(PuglTestApp* app, int argc, char** argv)
{
	char* endptr = NULL;

	// Parse command line options
	app->numRects = 1024;
	app->opts     = puglParseTestOptions(&argc, &argv);
	if (app->opts.help) {
		return 1;
	}

	// Parse number of rectangles argument, if given
	if (argc >= 1) {
		app->numRects = (size_t)strtol(argv[0], &endptr, 10);
		if (endptr != argv[0] + strlen(argv[0])) {
			logError("Invalid number of rectangles: %s\n", argv[0]);
			return 1;
		}
	}

	// Parse OpenGL major version argument, if given
	if (argc >= 2) {
		app->glMajorVersion = (int)strtol(argv[1], &endptr, 10);
		if (endptr != argv[1] + strlen(argv[1])) {
			logError("Invalid GL major version: %s\n", argv[1]);
			return 1;
		} else if (app->glMajorVersion == 4) {
			app->glMinorVersion = 2;
		} else if (app->glMajorVersion != 3) {
			logError("Unsupported GL major version %d\n", app->glMajorVersion);
			return 1;
		}
	}

	return 0;
}

static void
setupPugl(PuglTestApp* app)
{
	// Create world, view, and rect data
	app->world = puglNewWorld(PUGL_PROGRAM, 0);
	app->view  = puglNewView(app->world);
	app->rects = makeRects(app->numRects);

	// Set up world and view
	puglSetClassName(app->world, "PuglGL3Demo");
	puglSetWindowTitle(app->view, "Pugl OpenGL 3");
	puglSetDefaultSize(app->view, defaultWidth, defaultHeight);
	puglSetMinSize(app->view, defaultWidth / 4, defaultHeight / 4);
	puglSetAspectRatio(app->view, 1, 1, 16, 9);
	puglSetBackend(app->view, puglGlBackend());
	puglSetViewHint(app->view, PUGL_USE_COMPAT_PROFILE, PUGL_FALSE);
	puglSetViewHint(app->view, PUGL_USE_DEBUG_CONTEXT, app->opts.errorChecking);
	puglSetViewHint(app->view, PUGL_CONTEXT_VERSION_MAJOR, app->glMajorVersion);
	puglSetViewHint(app->view, PUGL_CONTEXT_VERSION_MINOR, app->glMinorVersion);
	puglSetViewHint(app->view, PUGL_RESIZABLE, app->opts.resizable);
	puglSetViewHint(app->view, PUGL_SAMPLES, app->opts.samples);
	puglSetViewHint(app->view, PUGL_DOUBLE_BUFFER, app->opts.doubleBuffer);
	puglSetViewHint(app->view, PUGL_SWAP_INTERVAL, app->opts.sync);
	puglSetViewHint(app->view, PUGL_IGNORE_KEY_REPEAT, PUGL_TRUE);
	puglSetHandle(app->view, app);
	puglSetEventFunc(app->view, onEvent);
}

static PuglStatus
setupGl(PuglTestApp* app)
{
	// Load GL functions via GLAD
	if (!gladLoadGLLoader((GLADloadproc)&puglGetProcAddress)) {
		logError("Failed to load GL\n");
		return PUGL_FAILURE;
	}

	const char* const headerFile = (app->glMajorVersion == 3
	                                ? "shaders/header_330.glsl"
	                                : "shaders/header_420.glsl");

	// Load shader sources
	char* const headerSource   = loadShader(headerFile);
	char* const vertexSource   = loadShader("shaders/rect.vert");
	char* const fragmentSource = loadShader("shaders/rect.frag");
	if (!vertexSource || !fragmentSource) {
		logError("Failed to load shader sources\n");
		return PUGL_FAILURE;
	}

	// Compile rectangle shaders and program
	app->drawRect = compileProgram(headerSource, vertexSource, fragmentSource);
	free(fragmentSource);
	free(vertexSource);
	free(headerSource);
	if (!app->drawRect.program) {
		return PUGL_FAILURE;
	}

	// Get location of rectangle shader uniform block
	const GLuint globalsIndex = glGetUniformBlockIndex(app->drawRect.program,
	                                                   "UniformBufferObject");

	// Generate/bind a uniform buffer for setting rectangle properties
	GLuint uboHandle = 0;
	glGenBuffers(1, &uboHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, uboHandle);
	glBindBufferBase(GL_UNIFORM_BUFFER, globalsIndex, uboHandle);

	// Generate/bind a VAO to track state
	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);

	// Generate/bind a VBO to store vertex position data
	glGenBuffers(1, &app->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, app->vbo);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(rectVertices),
	             rectVertices,
	             GL_STATIC_DRAW);

	// Attribute 0 is position, 2 floats from the VBO
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

	// Generate/bind a VBO to store instance attribute data
	glGenBuffers(1, &app->instanceVbo);
	glBindBuffer(GL_ARRAY_BUFFER, app->instanceVbo);
	glBufferData(GL_ARRAY_BUFFER,
	             (GLsizeiptr)(app->numRects * sizeof(Rect)),
	             app->rects,
	             GL_STREAM_DRAW);

	// Attribute 1 is Rect::position
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 4);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Rect), NULL);

	// Attribute 2 is Rect::size
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 4);
	glVertexAttribPointer(2,
	                      2,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      sizeof(Rect),
	                      (const void*)offsetof(Rect, size));

	// Attribute 3 is Rect::fillColor
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 4);
	glVertexAttribPointer(3,
	                      4,
	                      GL_FLOAT,
	                      GL_FALSE,
	                      sizeof(Rect),
	                      (const void*)offsetof(Rect, fillColor));

	// Set up the IBO to index into the VBO
	glGenBuffers(1, &app->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             sizeof(rectIndices),
	             rectIndices,
	             GL_STATIC_DRAW);

	return PUGL_SUCCESS;
}

static void
teardownGl(PuglTestApp* app)
{
	glDeleteBuffers(1, &app->ibo);
	glDeleteBuffers(1, &app->vbo);
	glDeleteBuffers(1, &app->instanceVbo);
	glDeleteVertexArrays(1, &app->vao);
	deleteProgram(app->drawRect);
}

int
main(int argc, char** argv)
{
	PuglTestApp app = {0};

	app.glMajorVersion = 3;
	app.glMinorVersion = 3;

	// Parse command line options
	if (parseOptions(&app, argc, argv)) {
		puglPrintTestUsage("pugl_shader_demo", "[NUM_RECTS] [GL_MAJOR]");
		return 1;
	}

	// Create and configure world and view
	setupPugl(&app);

	// Create window (which will send a PUGL_CREATE event)
	const PuglStatus st = puglRealize(app.view);
	if (st) {
		return logError("Failed to create window (%s)\n", puglStrerror(st));
	}

	// Show window
	puglShowWindow(app.view);

	// Grind away, drawing continuously
	PuglFpsPrinter fpsPrinter = {puglGetTime(app.world)};
	while (!app.quit) {
		puglUpdate(app.world, 0.0);
		puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
	}

	// Destroy window (which will send a PUGL_DESTROY event)
	puglFreeView(app.view);

	// Free everything else
	puglFreeWorld(app.world);
	free(app.rects);

	return 0;
}
