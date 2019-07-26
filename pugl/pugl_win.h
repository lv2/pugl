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

#include "pugl/pugl_internal_types.h"

#include <windows.h>

#include <stdbool.h>

typedef PIXELFORMATDESCRIPTOR PuglWinPFD;

struct PuglInternalsImpl {
	PuglWinPFD   pfd;
	int          pfId;
	HWND         hwnd;
	HDC          hdc;
	const PuglBackend* backend;
	PuglSurface* surface;
	DWORD        refreshRate;
	double       timerFrequency;
	bool         flashing;
	bool         resizing;
	bool         mouseTracked;
};

static inline PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints* const hints)
{
	const int rgbBits = hints->red_bits + hints->green_bits + hints->blue_bits;

	PuglWinPFD pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize        = sizeof(pfd);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;
	pfd.dwFlags     |= hints->double_buffer ? PFD_DOUBLEBUFFER : 0;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits   = (BYTE)rgbBits;
	pfd.cRedBits     = (BYTE)hints->red_bits;
	pfd.cGreenBits   = (BYTE)hints->green_bits;
	pfd.cBlueBits    = (BYTE)hints->blue_bits;
	pfd.cAlphaBits   = (BYTE)hints->alpha_bits;
	pfd.cDepthBits   = (BYTE)hints->depth_bits;
	pfd.cStencilBits = (BYTE)hints->stencil_bits;
	pfd.iLayerType   = PFD_MAIN_PLANE;
	return pfd;
}

static inline PuglStatus
puglWinCreateWindow(const PuglView* const view,
                    const char* const     title,
                    HWND* const           hwnd,
                    HDC* const            hdc)
{
	const char* className = view->windowClass ? view->windowClass : "Pugl";

	const unsigned winFlags =
		(WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		 (view->parent
		  ? WS_CHILD
		  : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX |
		     (view->hints.resizable ? (WS_SIZEBOX | WS_MAXIMIZEBOX) : 0))));

	const unsigned winExFlags =
		WS_EX_NOINHERITLAYOUT | (view->parent ? 0u : WS_EX_APPWINDOW);

	// Calculate total window size to accommodate requested view size
	RECT wr = { 0, 0, view->width, view->height };
	AdjustWindowRectEx(&wr, winFlags, FALSE, winExFlags);

	// Create window and get drawing context
	if (!(*hwnd = CreateWindowEx(winExFlags, className, title, winFlags,
	                             CW_USEDEFAULT, CW_USEDEFAULT,
	                             wr.right-wr.left, wr.bottom-wr.top,
	                             (HWND)view->parent, NULL, NULL, NULL))) {
		return PUGL_ERR_CREATE_WINDOW;
	} else if (!(*hdc = GetDC(*hwnd))) {
		DestroyWindow(*hwnd);
		*hwnd = NULL;
		return PUGL_ERR_CREATE_WINDOW;
	}

	return PUGL_SUCCESS;
}
