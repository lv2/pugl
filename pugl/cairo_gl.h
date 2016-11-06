/*
  Copyright 2016 David Robillard <http://drobilla.net>

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

#if defined(PUGL_HAVE_GL) && defined(PUGL_HAVE_CAIRO)

#include <cairo/cairo.h>
#include <stdint.h>

#include "pugl/gl.h"

typedef struct {
	unsigned texture_id;
	uint8_t* buffer;
} PuglCairoGL;

static cairo_surface_t*
pugl_cairo_gl_create(PuglCairoGL* ctx, int width, int height, int bpp)
{
	free(ctx->buffer);
	ctx->buffer = (uint8_t*)calloc(bpp * width * height, sizeof(uint8_t));
	if (!ctx->buffer) {
		fprintf(stderr, "failed to allocate surface buffer\n");
		return NULL;
	}

	return cairo_image_surface_create_for_data(
		ctx->buffer, CAIRO_FORMAT_ARGB32, width, height, bpp * width);
}

static void
pugl_cairo_gl_free(PuglCairoGL* ctx)
{
	free(ctx->buffer);
	ctx->buffer = NULL;
}

static void
pugl_cairo_gl_configure(PuglCairoGL* ctx, int width, int height)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT);

	glDeleteTextures(1, &ctx->texture_id);
	glGenTextures(1, &ctx->texture_id);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, ctx->texture_id);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

static void
pugl_cairo_gl_draw(PuglCairoGL* ctx, int width, int height)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glEnable(GL_TEXTURE_2D);

	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
	             width, height, 0,
	             GL_BGRA, GL_UNSIGNED_BYTE, ctx->buffer);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, (GLfloat)height);
	glVertex2f(-1.0f, -1.0f);

	glTexCoord2f((GLfloat)width, (GLfloat)height);
	glVertex2f(1.0f, -1.0f);

	glTexCoord2f((GLfloat)width, 0.0f);
	glVertex2f(1.0f, 1.0f);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glPopMatrix();
}

#endif
