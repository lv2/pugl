/*
  Copyright 2012 David Robillard <http://drobilla.net>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles

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
   @file pugl_internal.h Private platform-independent definitions.

   Note this file contains function definitions, so it must be compiled into
   the final binary exactly once.  Each platform specific implementation file
   including it once should achieve this.
*/

#include "pugl.h"

typedef struct PuglPlatformDataImpl PuglPlatformData;

struct PuglWindowImpl {
	PuglHandle       handle;
	PuglCloseFunc    closeFunc;
	PuglDisplayFunc  displayFunc;
	PuglKeyboardFunc keyboardFunc;
	PuglMotionFunc   motionFunc;
	PuglMouseFunc    mouseFunc;
	PuglReshapeFunc  reshapeFunc;

	PuglPlatformData* impl;

	int  width;
	int  height;
	bool redisplay;
};

void
puglSetHandle(PuglWindow* window, PuglHandle handle)
{
	window->handle = handle;
}

PuglHandle
puglGetHandle(PuglWindow* window)
{
	return window->handle;
}

void
puglSetCloseFunc(PuglWindow* window, PuglCloseFunc closeFunc)
{
	window->closeFunc = closeFunc;
}

void
puglSetDisplayFunc(PuglWindow* window, PuglDisplayFunc displayFunc)
{
	window->displayFunc = displayFunc;
}

void
puglSetKeyboardFunc(PuglWindow* window, PuglKeyboardFunc keyboardFunc)
{
	window->keyboardFunc = keyboardFunc;
}

void
puglSetMotionFunc(PuglWindow* window, PuglMotionFunc motionFunc)
{
	window->motionFunc = motionFunc;
}

void
puglSetMouseFunc(PuglWindow* window, PuglMouseFunc mouseFunc)
{
	window->mouseFunc = mouseFunc;
}
	
void
puglSetReshapeFunc(PuglWindow* window, PuglReshapeFunc reshapeFunc)
{
	window->reshapeFunc = reshapeFunc;
}
