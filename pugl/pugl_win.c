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
   @file pugl_win.c Windows/WGL Pugl Implementation.
*/

#include "pugl/pugl_internal.h"

#include <windows.h>
#include <windowsx.h>

#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#ifndef WM_MOUSEWHEEL
#    define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#    define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WHEEL_DELTA
#    define WHEEL_DELTA 120
#endif
#ifndef GWLP_USERDATA
#    define GWLP_USERDATA (-21)
#endif

#define PUGL_LOCAL_CLOSE_MSG (WM_USER + 50)
#define PUGL_RESIZE_TIMER_ID 9461
#define PUGL_URGENT_TIMER_ID 9462

#define WGL_DRAW_TO_WINDOW_ARB    0x2001
#define WGL_ACCELERATION_ARB      0x2003
#define WGL_SUPPORT_OPENGL_ARB    0x2010
#define WGL_DOUBLE_BUFFER_ARB     0x2011
#define WGL_PIXEL_TYPE_ARB        0x2013
#define WGL_COLOR_BITS_ARB        0x2014
#define WGL_RED_BITS_ARB          0x2015
#define WGL_GREEN_BITS_ARB        0x2017
#define WGL_BLUE_BITS_ARB         0x2019
#define WGL_ALPHA_BITS_ARB        0x201b
#define WGL_DEPTH_BITS_ARB        0x2022
#define WGL_STENCIL_BITS_ARB      0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB         0x202b
#define WGL_SAMPLE_BUFFERS_ARB    0x2041
#define WGL_SAMPLES_ARB           0x2042

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB   0x2093
#define WGL_CONTEXT_FLAGS_ARB         0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB  0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

struct PuglInternalsImpl {
	HWND   hwnd;
	HDC    hdc;
	HGLRC  hglrc;
	DWORD  refreshRate;
	double timerFrequency;
	bool   flashing;
	bool   resizing;
	bool   mouseTracked;
};

// Scoped class to manage the fake window used during window creation
typedef struct {
	HWND hwnd;
	HDC  hdc;
} PuglFakeWindow;

static const TCHAR* DEFAULT_CLASSNAME = "Pugl";

static PuglFakeWindow
puglMakeFakeWindow(HWND wnd)
{
	const PuglFakeWindow fakeWin = {wnd, wnd ? GetDC(wnd) : 0};
	return fakeWin;
}

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

PuglInternals*
puglInitInternals(void)
{
	PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	impl->timerFrequency = (double)frequency.QuadPart;

	return impl;
}

void
puglEnterContext(PuglView* view)
{
	PAINTSTRUCT ps;
	BeginPaint(view->impl->hwnd, &ps);

#ifdef PUGL_HAVE_GL
	if (view->ctx_type == PUGL_GL) {
		wglMakeCurrent(view->impl->hdc, view->impl->hglrc);
	}
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
#ifdef PUGL_HAVE_GL
	if (view->ctx_type == PUGL_GL && flush) {
		SwapBuffers(view->impl->hdc);
	}
#endif

	PAINTSTRUCT ps;
	EndPaint(view->impl->hwnd, &ps);
}

static PIXELFORMATDESCRIPTOR
puglGetPixelFormatDescriptor(const PuglHints* hints)
{
	const int rgbBits = hints->red_bits + hints->green_bits + hints->blue_bits;

	PIXELFORMATDESCRIPTOR pfd;
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

static int
puglWinError(PuglFakeWindow* fakeWin, const int status)
{
	if (fakeWin->hwnd) {
		ReleaseDC(fakeWin->hwnd, fakeWin->hdc);
		DestroyWindow(fakeWin->hwnd);
	}

	return status;
}

static unsigned
getWindowFlags(PuglView* view)
{
	return (WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
	        (view->parent
	         ? WS_CHILD
	         : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX |
	            (view->hints.resizable ? (WS_SIZEBOX | WS_MAXIMIZEBOX) : 0))));
}

static unsigned
getWindowExFlags(PuglView* view)
{
	return WS_EX_NOINHERITLAYOUT | (view->parent ? 0u : WS_EX_APPWINDOW);
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	typedef BOOL (*WglChoosePixelFormat)(
		HDC, const int*, const FLOAT*, UINT, int*, UINT*);

	typedef HGLRC (*WglCreateContextAttribs)(HDC, HGLRC, const int*);

	typedef BOOL (*WglSwapInterval)(int);

	const char* className = view->windowClass ? view->windowClass : DEFAULT_CLASSNAME;

	title = title ? title : "Window";

	// Get refresh rate for resize draw timer
	DEVMODEA devMode = {0};
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	view->impl->refreshRate = devMode.dmDisplayFrequency;

	// Register window class
	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_OWNDC;
	wc.lpfnWndProc   = wndProc;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION); // TODO: user-specified icon
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = className;
	if (!RegisterClassEx(&wc)) {
		return 1;
	}

	// Calculate window flags
	const unsigned winFlags   = getWindowFlags(view);
	const unsigned winExFlags = getWindowExFlags(view);
	if (view->hints.resizable) {
		if (view->min_width || view->min_height) {
			// Adjust the minimum window size to accomodate requested view size
			RECT mr = { 0, 0, view->min_width, view->min_height };
			AdjustWindowRectEx(&mr, winFlags, FALSE, winExFlags);
			view->min_width  = mr.right - mr.left;
			view->min_height = mr.bottom - mr.top;
		}
	}

	// Adjust the window size to accomodate requested view size
	RECT wr = { 0, 0, view->width, view->height };
	AdjustWindowRectEx(&wr, winFlags, FALSE, winExFlags);

	// Create fake window for getting at GL context
	PuglFakeWindow fakeWin = puglMakeFakeWindow(
		CreateWindowEx(winExFlags,
		               className, title,
		               winFlags,
		               CW_USEDEFAULT, CW_USEDEFAULT,
		               wr.right-wr.left, wr.bottom-wr.top,
		               (HWND)view->parent, NULL, NULL, NULL));

	if (!fakeWin.hwnd) {
		return puglWinError(&fakeWin, 2);
	}

	// Choose pixel format for fake window
	const PIXELFORMATDESCRIPTOR fakePfd = puglGetPixelFormatDescriptor(
		&view->hints);
	const int fakeFormatId = ChoosePixelFormat(fakeWin.hdc, &fakePfd);
	if (!fakeFormatId) {
		return puglWinError(&fakeWin, 3);
	} else if (!SetPixelFormat(fakeWin.hdc, fakeFormatId, &fakePfd)) {
		return puglWinError(&fakeWin, 4);
	}

	HGLRC fakeRc = wglCreateContext(fakeWin.hdc);
	if (!fakeRc) {
		return puglWinError(&fakeWin, 5);
	}

	wglMakeCurrent(fakeWin.hdc, fakeRc);

	WglChoosePixelFormat wglChoosePixelFormat = (WglChoosePixelFormat)(
		wglGetProcAddress("wglChoosePixelFormatARB"));
	WglCreateContextAttribs wglCreateContextAttribs = (WglCreateContextAttribs)(
		wglGetProcAddress("wglCreateContextAttribsARB"));
	WglSwapInterval wglSwapInterval = (WglSwapInterval)(
		wglGetProcAddress("wglSwapIntervalEXT"));

	PuglInternals* impl = view->impl;

	if (wglChoosePixelFormat && wglCreateContextAttribs) {
		// Now create real window
		impl->hwnd = CreateWindowEx(
			winExFlags,
			className, title,
			winFlags,
			CW_USEDEFAULT, CW_USEDEFAULT, wr.right-wr.left, wr.bottom-wr.top,
			(HWND)view->parent, NULL, NULL, NULL);

		impl->hdc = GetDC(impl->hwnd);

		const int pixelAttrs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,  view->hints.double_buffer,
			WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
			WGL_SAMPLE_BUFFERS_ARB, view->hints.samples ? 1 : 0,
			WGL_SAMPLES_ARB,        view->hints.samples,
			WGL_RED_BITS_ARB,       view->hints.red_bits,
			WGL_GREEN_BITS_ARB,     view->hints.green_bits,
			WGL_BLUE_BITS_ARB,      view->hints.blue_bits,
			WGL_ALPHA_BITS_ARB,     view->hints.alpha_bits,
			WGL_DEPTH_BITS_ARB,     view->hints.depth_bits,
			WGL_STENCIL_BITS_ARB,   view->hints.stencil_bits,
			0,
		};

		// Choose pixel format based on hints
		int  pixelFormatId;
		UINT numFormats;
		if (!wglChoosePixelFormat(impl->hdc, pixelAttrs, NULL, 1u, &pixelFormatId, &numFormats)) {
			return puglWinError(&fakeWin, 6);
		}

		// Set desired pixel format
		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat(impl->hdc, pixelFormatId, sizeof(pfd), &pfd);
		if (!SetPixelFormat(impl->hdc, pixelFormatId, &pfd)) {
			return puglWinError(&fakeWin, 7);
		}

		// Create final GL context
		const int contextAttribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, view->hints.context_version_major,
			WGL_CONTEXT_MINOR_VERSION_ARB, view->hints.context_version_minor,
			WGL_CONTEXT_PROFILE_MASK_ARB, (view->hints.use_compat_profile
			                               ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB
			                               : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB),
			0
		};

		if (!(impl->hglrc = wglCreateContextAttribs(impl->hdc, 0, contextAttribs))) {
			return puglWinError(&fakeWin, 8);
		}

		// Switch to new context
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(fakeRc);
		if (!wglMakeCurrent(impl->hdc, impl->hglrc)) {
			return puglWinError(&fakeWin, 9);
		}

		ReleaseDC(fakeWin.hwnd, fakeWin.hdc);
		DestroyWindow(fakeWin.hwnd);
	} else {
		// Modern extensions not available, just use the original "fake" window
		impl->hwnd   = fakeWin.hwnd;
		impl->hdc    = fakeWin.hdc;
		impl->hglrc  = fakeRc;
		fakeWin.hwnd = 0;
		fakeWin.hdc  = 0;
	}

	if (wglSwapInterval) {
		wglSwapInterval(1);
	}

	SetWindowLongPtr(impl->hwnd, GWLP_USERDATA, (LONG_PTR)view);

	return 0;
}

void
puglShowWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	ShowWindow(impl->hwnd, SW_SHOWNORMAL);
	SetFocus(impl->hwnd);
	view->visible = true;
}

void
puglHideWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	ShowWindow(impl->hwnd, SW_HIDE);
	view->visible = false;
}

void
puglDestroy(PuglView* view)
{
	if (view) {
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(view->impl->hglrc);
		ReleaseDC(view->impl->hwnd, view->impl->hdc);
		DestroyWindow(view->impl->hwnd);
		UnregisterClass(view->windowClass ? view->windowClass : DEFAULT_CLASSNAME, NULL);
		free(view->windowClass);
		free(view->impl);
		free(view);
	}
}

static PuglKey
keySymToSpecial(WPARAM sym)
{
	switch (sym) {
	case VK_F1:      return PUGL_KEY_F1;
	case VK_F2:      return PUGL_KEY_F2;
	case VK_F3:      return PUGL_KEY_F3;
	case VK_F4:      return PUGL_KEY_F4;
	case VK_F5:      return PUGL_KEY_F5;
	case VK_F6:      return PUGL_KEY_F6;
	case VK_F7:      return PUGL_KEY_F7;
	case VK_F8:      return PUGL_KEY_F8;
	case VK_F9:      return PUGL_KEY_F9;
	case VK_F10:     return PUGL_KEY_F10;
	case VK_F11:     return PUGL_KEY_F11;
	case VK_F12:     return PUGL_KEY_F12;
	case VK_LEFT:    return PUGL_KEY_LEFT;
	case VK_UP:      return PUGL_KEY_UP;
	case VK_RIGHT:   return PUGL_KEY_RIGHT;
	case VK_DOWN:    return PUGL_KEY_DOWN;
	case VK_PRIOR:   return PUGL_KEY_PAGE_UP;
	case VK_NEXT:    return PUGL_KEY_PAGE_DOWN;
	case VK_HOME:    return PUGL_KEY_HOME;
	case VK_END:     return PUGL_KEY_END;
	case VK_INSERT:  return PUGL_KEY_INSERT;
	case VK_SHIFT:   return PUGL_KEY_SHIFT;
	case VK_CONTROL: return PUGL_KEY_CTRL;
	case VK_MENU:    return PUGL_KEY_ALT;
	case VK_LWIN:    return PUGL_KEY_SUPER;
	case VK_RWIN:    return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static uint32_t
getModifiers(void)
{
	return (((GetKeyState(VK_SHIFT)   < 0) ? PUGL_MOD_SHIFT  : 0u) |
	        ((GetKeyState(VK_CONTROL) < 0) ? PUGL_MOD_CTRL   : 0u) |
	        ((GetKeyState(VK_MENU)    < 0) ? PUGL_MOD_ALT    : 0u) |
	        ((GetKeyState(VK_LWIN)    < 0) ? PUGL_MOD_SUPER  : 0u) |
	        ((GetKeyState(VK_RWIN)    < 0) ? PUGL_MOD_SUPER  : 0u));
}

static void
initMouseEvent(PuglEvent* event,
               PuglView*  view,
               int        button,
               bool       press,
               LPARAM     lParam)
{
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ClientToScreen(view->impl->hwnd, &pt);

	if (press) {
		SetCapture(view->impl->hwnd);
	} else {
		ReleaseCapture();
	}

	event->button.time   = GetMessageTime() / 1e3;
	event->button.type   = press ? PUGL_BUTTON_PRESS : PUGL_BUTTON_RELEASE;
	event->button.x      = GET_X_LPARAM(lParam);
	event->button.y      = GET_Y_LPARAM(lParam);
	event->button.x_root = pt.x;
	event->button.y_root = pt.y;
	event->button.state  = getModifiers();
	event->button.button = (uint32_t)button;
}

static void
initScrollEvent(PuglEvent* event, PuglView* view, LPARAM lParam)
{
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ScreenToClient(view->impl->hwnd, &pt);

	event->scroll.time   = GetMessageTime() / 1e3;
	event->scroll.type   = PUGL_SCROLL;
	event->scroll.x      = pt.x;
	event->scroll.y      = pt.y;
	event->scroll.x_root = GET_X_LPARAM(lParam);
	event->scroll.y_root = GET_Y_LPARAM(lParam);
	event->scroll.state  = getModifiers();
	event->scroll.dx     = 0;
	event->scroll.dy     = 0;
}

static uint32_t
utf16_to_code_point(const wchar_t* input, const int input_size)
{
	uint32_t code_unit = *input;
	// Equiv. range check between 0xD800 to 0xDBFF inclusive
	if ((code_unit & 0xFC00) == 0xD800) {
		if (input_size < 2) {
			// "Error: is surrogate but input_size too small"
			return 0xFFFD;  // replacement character
		}

		uint32_t code_unit_2 = *++input;
		// Equiv. range check between 0xDC00 to 0xDFFF inclusive
		if ((code_unit_2 & 0xFC00) == 0xDC00) {
			return (code_unit << 10) + code_unit_2 - 0x35FDC00;
		}

		// TODO: push_back(code_unit_2);
		// "Error: Unpaired surrogates."
		return 0xFFFD;  // replacement character
	}
	return code_unit;
}

static void
initKeyEvent(PuglEvent* event, PuglView* view, bool press, LPARAM lParam)
{
	POINT rpos = { 0, 0 };
	GetCursorPos(&rpos);

	POINT cpos = { rpos.x, rpos.y };
	ScreenToClient(view->impl->hwnd, &rpos);

	event->key.type      = press ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
	event->key.time      = GetMessageTime() / 1e3;
	event->key.state     = getModifiers();
	event->key.x_root    = rpos.x;
	event->key.y_root    = rpos.y;
	event->key.x         = cpos.x;
	event->key.y         = cpos.y;
	event->key.keycode   = (uint32_t)((lParam & 0xFF0000) >> 16);
	event->key.character = 0;
	event->key.special   = (PuglKey)0;
	event->key.filter    = 0;
}

static void
wcharBufToEvent(wchar_t* buf, int n, PuglEvent* event)
{
	if (n > 0) {
		char* charp = (char*)event->key.string;
		if (!WideCharToMultiByte(CP_UTF8, 0, buf, n,
		                         charp, 8, NULL, NULL)) {
			/* error: could not convert to utf-8,
			   GetLastError has details */
			memset(event->key.string, 0, 8);
			// replacement character
			event->key.string[0] = 0xEF;
			event->key.string[1] = 0xBF;
			event->key.string[2] = 0xBD;
		}

		event->key.character = utf16_to_code_point(buf, n);
	} else {
		// replacement character
		event->key.string[0] = 0xEF;
		event->key.string[1] = 0xBF;
		event->key.string[2] = 0xBD;
		event->key.character = 0xFFFD;
	}
}

static void
translateMessageParamsToEvent(LPARAM lParam, WPARAM wParam, PuglEvent* event)
{
	(void)lParam;

	/* TODO: This is a kludge.  Would be nice to use ToUnicode here, but this
	   breaks composed keys because it messes with the keyboard state.  Not
	   sure how to correctly handle this on Windows. */

	// This is how I really want to do this, but it breaks composed keys (é,
	// è, ü, ö, and so on) because ToUnicode messes with the keyboard state.

	//wchar_t buf[5];
	//BYTE keyboard_state[256];
	//int wcharCount = 0;
	//GetKeyboardState(keyboard_state);
	//wcharCount = ToUnicode(wParam, MapVirtualKey(wParam, MAPVK_VK_TO_VSC),
	//                       keyboard_state, buf, 4, 0);
	//wcharBufToEvent(buf, wcharCount, event);

	// So, since Google refuses to give me a better solution, and if no one
	// else has a better solution, I will make a hack...
	wchar_t buf[5] = { 0, 0, 0, 0, 0 };
	WPARAM c = MapVirtualKey((unsigned)wParam, MAPVK_VK_TO_CHAR);
	buf[0] = c & 0xffff;
	// TODO: This does not take caps lock into account
	// TODO: Dead keys should affect key releases as well
	if (!(event->key.state && PUGL_MOD_SHIFT))
		buf[0] = towlower(buf[0]);
	wcharBufToEvent(buf, 1, event);
	event->key.filter = ((c >> 31) & 0x1);
}

static void
translateCharEventToEvent(WPARAM wParam, PuglEvent* event)
{
	wchar_t buf[2];
	int wcharCount;
	if (wParam & 0xFFFF0000) {
		wcharCount = 2;
		buf[0] = (wParam & 0xFFFF);
		buf[1] = ((wParam >> 16) & 0xFFFF);
	} else {
		wcharCount = 1;
		buf[0] = (wParam & 0xFFFF);
	}
	wcharBufToEvent(buf, wcharCount, event);
}

static bool
ignoreKeyEvent(PuglView* view, LPARAM lParam)
{
	return view->ignoreKeyRepeat && (lParam & (1 << 30));
}

static RECT
handleConfigure(PuglView* view, PuglEvent* event)
{
	RECT rect;
	GetClientRect(view->impl->hwnd, &rect);
	view->width  = rect.right - rect.left;
	view->height = rect.bottom - rect.top;

	event->configure.type   = PUGL_CONFIGURE;
	event->configure.x      = rect.left;
	event->configure.y      = rect.top;
	event->configure.width  = view->width;
	event->configure.height = view->height;

	return rect;
}

static void
handleCrossing(PuglView* view, const PuglEventType type, POINT pos)
{
	POINT root_pos = pos;
	ClientToScreen(view->impl->hwnd, &root_pos);

	const PuglEventCrossing ev = {
		type,
		0,
		GetMessageTime() / 1e3,
		(double)pos.x,
		(double)pos.y,
		(double)root_pos.x,
		(double)root_pos.y,
		getModifiers(),
		PUGL_CROSSING_NORMAL
	};
	puglDispatchEvent(view, (const PuglEvent*)&ev);
}

static void
stopFlashing(PuglView* view)
{
	if (view->impl->flashing) {
		KillTimer(view->impl->hwnd, PUGL_URGENT_TIMER_ID);
		FlashWindow(view->impl->hwnd, FALSE);
	}
}

static LRESULT
handleMessage(PuglView* view, UINT message, WPARAM wParam, LPARAM lParam)
{
	PuglEvent   event;
	void*       dummy_ptr = NULL;
	RECT        rect;
	MINMAXINFO* mmi;
	POINT       pt;

	memset(&event, 0, sizeof(event));

	event.any.type = PUGL_NOTHING;
	if (InSendMessageEx(dummy_ptr)) {
		event.any.flags |= PUGL_IS_SEND_EVENT;
	}

	switch (message) {
	case WM_SHOWWINDOW:
		rect = handleConfigure(view, &event);
		puglPostRedisplay(view);
		break;
	case WM_SIZE:
		rect = handleConfigure(view, &event);
		RedrawWindow(view->impl->hwnd, NULL, NULL,
		             RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_INTERNALPAINT|
		             RDW_UPDATENOW);
		break;
	case WM_ENTERSIZEMOVE:
		view->impl->resizing = true;
		SetTimer(view->impl->hwnd,
		         PUGL_RESIZE_TIMER_ID,
		         1000 / view->impl->refreshRate,
		         NULL);
		break;
	case WM_TIMER:
		if (wParam == PUGL_RESIZE_TIMER_ID) {
			RedrawWindow(view->impl->hwnd, NULL, NULL,
			             RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_INTERNALPAINT);
		} else if (wParam == PUGL_URGENT_TIMER_ID) {
			FlashWindow(view->impl->hwnd, TRUE);
		}
		break;
	case WM_EXITSIZEMOVE:
		KillTimer(view->impl->hwnd, PUGL_RESIZE_TIMER_ID);
		view->impl->resizing = false;
		puglPostRedisplay(view);
		break;
	case WM_GETMINMAXINFO:
		mmi                   = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = view->min_width;
		mmi->ptMinTrackSize.y = view->min_height;
		break;
	case WM_PAINT:
		GetUpdateRect(view->impl->hwnd, &rect, false);
		event.expose.type   = PUGL_EXPOSE;
		event.expose.x      = rect.left;
		event.expose.y      = rect.top;
		event.expose.width  = rect.right - rect.left;
		event.expose.height = rect.bottom - rect.top;
		event.expose.count  = 0;
		break;
	case WM_ERASEBKGND:
		return true;
	case WM_MOUSEMOVE:
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		if (!view->impl->mouseTracked) {
			TRACKMOUSEEVENT tme = {0};
			tme.cbSize    = sizeof(tme);
			tme.dwFlags   = TME_LEAVE;
			tme.hwndTrack = view->impl->hwnd;
			TrackMouseEvent(&tme);

			stopFlashing(view);
			handleCrossing(view, PUGL_ENTER_NOTIFY, pt);
			view->impl->mouseTracked = true;
		}

		ClientToScreen(view->impl->hwnd, &pt);
		event.motion.type    = PUGL_MOTION_NOTIFY;
		event.motion.time    = GetMessageTime() / 1e3;
		event.motion.x       = GET_X_LPARAM(lParam);
		event.motion.y       = GET_Y_LPARAM(lParam);
		event.motion.x_root  = pt.x;
		event.motion.y_root  = pt.y;
		event.motion.state   = getModifiers();
		event.motion.is_hint = false;
		break;
	case WM_MOUSELEAVE:
		GetCursorPos(&pt);
		ScreenToClient(view->impl->hwnd, &pt);
		handleCrossing(view, PUGL_LEAVE_NOTIFY, pt);
		view->impl->mouseTracked = false;
		break;
	case WM_LBUTTONDOWN:
		initMouseEvent(&event, view, 1, true, lParam);
		break;
	case WM_MBUTTONDOWN:
		initMouseEvent(&event, view, 2, true, lParam);
		break;
	case WM_RBUTTONDOWN:
		initMouseEvent(&event, view, 3, true, lParam);
		break;
	case WM_LBUTTONUP:
		initMouseEvent(&event, view, 1, false, lParam);
		break;
	case WM_MBUTTONUP:
		initMouseEvent(&event, view, 2, false, lParam);
		break;
	case WM_RBUTTONUP:
		initMouseEvent(&event, view, 3, false, lParam);
		break;
	case WM_MOUSEWHEEL:
		initScrollEvent(&event, view, lParam);
		event.scroll.dy = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	case WM_MOUSEHWHEEL:
		initScrollEvent(&event, view, lParam);
		event.scroll.dx = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	case WM_KEYDOWN:
		if (!ignoreKeyEvent(view, lParam)) {
			initKeyEvent(&event, view, true, lParam);
			if (!(event.key.special = keySymToSpecial(wParam))) {
				event.key.type = PUGL_NOTHING;
			}
		}
		break;
	case WM_CHAR:
		if (!ignoreKeyEvent(view, lParam)) {
			initKeyEvent(&event, view, true, lParam);
			translateCharEventToEvent(wParam, &event);
		}
		break;
	case WM_DEADCHAR:
		if (!ignoreKeyEvent(view, lParam)) {
			initKeyEvent(&event, view, true, lParam);
			translateCharEventToEvent(wParam, &event);
			event.key.filter = 1;
		}
		break;
	case WM_KEYUP:
		initKeyEvent(&event, view, false, lParam);
		if (!(event.key.special = keySymToSpecial(wParam))) {
			translateMessageParamsToEvent(lParam, wParam, &event);
		}
		break;
	case WM_SETFOCUS:
		stopFlashing(view);
		event.type = PUGL_FOCUS_IN;
		break;
	case WM_KILLFOCUS:
		event.type = PUGL_FOCUS_OUT;
		break;
	case WM_QUIT:
	case PUGL_LOCAL_CLOSE_MSG:
		event.close.type = PUGL_CLOSE;
		break;
	default:
		return DefWindowProc(view->impl->hwnd, message, wParam, lParam);
	}

	puglDispatchEvent(view, &event);

	return 0;
}

void
puglGrabFocus(PuglView* view)
{
	SetFocus(view->impl->hwnd);
}

void
puglRequestAttention(PuglView* view)
{
	if (!view->impl->mouseTracked || GetFocus() != view->impl->hwnd) {
		FlashWindow(view->impl->hwnd, TRUE);
		SetTimer(view->impl->hwnd, PUGL_URGENT_TIMER_ID, 500, NULL);
		view->impl->flashing = true;
	}
}

PuglStatus
puglWaitForEvent(PuglView* view)
{
	(void)view;
	WaitMessage();
	return PUGL_SUCCESS;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	MSG msg;
	while (PeekMessage(&msg, view->impl->hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return PUGL_SUCCESS;
}

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PuglView* view = (PuglView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message) {
	case WM_CREATE:
		PostMessage(hwnd, WM_SHOWWINDOW, TRUE, 0);
		return 0;
	case WM_CLOSE:
		PostMessage(hwnd, PUGL_LOCAL_CLOSE_MSG, wParam, lParam);
		return 0;
	case WM_DESTROY:
		return 0;
	default:
		if (view && hwnd == view->impl->hwnd) {
			return handleMessage(view, message, wParam, lParam);
		} else {
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
	return (PuglGlFunc)wglGetProcAddress(name);
}

double
puglGetTime(PuglView* view)
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    const double now = (double)count.QuadPart / view->impl->timerFrequency;
    return now - view->start_time;
}

void
puglPostRedisplay(PuglView* view)
{
	RedrawWindow(view->impl->hwnd, NULL, NULL,
	             RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_INTERNALPAINT);
	UpdateWindow(view->impl->hwnd);
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->hwnd;
}
