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
#include "pugl/pugl_internal_types.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct PuglInternalsImpl {
	Display*        display;
	int             screen;
	XVisualInfo*    vi;
	Window          win;
	XIM             xim;
	XIC             xic;
	PuglDrawContext ctx;
	PuglSurface*    surface;

	struct {
		Atom WM_PROTOCOLS;
		Atom WM_DELETE_WINDOW;
		Atom NET_WM_STATE;
		Atom NET_WM_STATE_DEMANDS_ATTENTION;
	} atoms;
};
