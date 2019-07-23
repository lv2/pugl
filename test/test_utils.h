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

#include "pugl/pugl.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
	double lastReportTime;
} PuglFpsPrinter;

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

/** Calculate a projection matrix for a given perspective. */
static inline void
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

static inline int
printModifiers(const uint32_t mods)
{
	return fprintf(stderr, "Modifiers:%s%s%s%s\n",
	               (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	               (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	               (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	               (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static inline int
printEvent(const PuglEvent* event, const char* prefix)
{
	switch (event->type) {
	case PUGL_KEY_PRESS:
		return fprintf(stderr, "%sKey press   code %3u key  U+%04X\n",
		               prefix, event->key.keycode, event->key.key);
	case PUGL_KEY_RELEASE:
		return fprintf(stderr, "%sKey release code %3u key  U+%04X\n",
		               prefix, event->key.keycode, event->key.key);
	case PUGL_TEXT:
		return fprintf(stderr, "%sText entry  code %3u char U+%04X (%s)\n",
		               prefix, event->text.keycode,
		               event->text.character, event->text.string);
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

static inline void
puglPrintFps(const PuglWorld* world,
             PuglFpsPrinter*  printer,
             unsigned* const  framesDrawn)
{
	const double thisTime = puglGetTime(world);
	if (thisTime > printer->lastReportTime + 5) {
		const double fps = *framesDrawn / (thisTime - printer->lastReportTime);
		fprintf(stderr,
		        "%u frames in %.0f seconds = %.3f FPS\n",
		        *framesDrawn,
		        thisTime - printer->lastReportTime,
		        fps);

		printer->lastReportTime = thisTime;
		*framesDrawn            = 0;
	}
}
