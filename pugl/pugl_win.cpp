/*
  Copyright 2012-2015 David Robillard <http://drobilla.net>

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
   @file pugl_win.cpp Windows/WGL Pugl Implementation.
*/

#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "pugl/pugl_internal.h"

#ifndef WM_MOUSEWHEEL
#    define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#    define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WHEEL_DELTA
#    define WHEEL_DELTA 120
#endif
#ifdef _WIN64
#    ifndef GWLP_USERDATA
#        define GWLP_USERDATA (-21)
#    endif
#else
#    ifndef GWL_USERDATA
#        define GWL_USERDATA (-21)
#    endif
#endif

#define PUGL_LOCAL_CLOSE_MSG (WM_USER + 50)

struct PuglInternalsImpl {
	HWND     hwnd;
	HDC      hdc;
	HGLRC    hglrc;
	WNDCLASS wc;
};

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

PuglView*
puglInit()
{
	PuglView*      view = (PuglView*)calloc(1, sizeof(PuglView));
	PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));
	if (!view || !impl) {
		return NULL;
	}

	view->impl   = impl;
	view->width  = 640;
	view->height = 480;

	return view;
}

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
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
		glFlush();
		SwapBuffers(view->impl->hdc);
	}
#endif

	PAINTSTRUCT ps;
	EndPaint(view->impl->hwnd, &ps);
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	static const TCHAR* DEFAULT_CLASSNAME = "Pugl";

	PuglInternals* impl = view->impl;

	if (!title) {
		title = "Window";
	}

	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_OWNDC;
	wc.lpfnWndProc   = wndProc;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION); // TODO: user-specified icon
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = view->windowClass ? view->windowClass : DEFAULT_CLASSNAME;
	if (!RegisterClassEx(&wc)) {
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return NULL;
	}

	int winFlags = WS_POPUPWINDOW | WS_CAPTION;
	if (view->resizable) {
		winFlags |= WS_SIZEBOX;
		if (view->min_width || view->min_height) {
			// Adjust the minimum window size to accomodate requested view size
			RECT mr = { 0, 0, view->min_width, view->min_height };
			AdjustWindowRectEx(&mr, winFlags, FALSE, WS_EX_TOPMOST);
			view->min_width  = mr.right - mr.left;
			view->min_height = mr.bottom - mr.top;
		}
	}

	// Adjust the window size to accomodate requested view size
	RECT wr = { 0, 0, view->width, view->height };
	AdjustWindowRectEx(&wr, winFlags, FALSE, WS_EX_TOPMOST);

	impl->hwnd = CreateWindowEx(
		WS_EX_TOPMOST,
		wc.lpszClassName, title,
		(view->parent ? WS_CHILD : winFlags),
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right-wr.left, wr.bottom-wr.top,
		(HWND)view->parent, NULL, NULL, NULL);

	if (!impl->hwnd) {
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return 1;
	}

#ifdef _WIN64
	SetWindowLongPtr(impl->hwnd, GWLP_USERDATA, (LONG_PTR)view);
#else
	SetWindowLongPtr(impl->hwnd, GWL_USERDATA, (LONG)view);
#endif

	impl->hdc = GetDC(impl->hwnd);

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int format = ChoosePixelFormat(impl->hdc, &pfd);
	SetPixelFormat(impl->hdc, format, &pfd);

	impl->hglrc = wglCreateContext(impl->hdc);
	if (!impl->hglrc) {
		ReleaseDC(impl->hwnd, impl->hdc);
		DestroyWindow(impl->hwnd);
		UnregisterClass(impl->wc.lpszClassName, NULL);
		free((void*)impl->wc.lpszClassName);
		free(impl);
		free(view);
		return NULL;
	}
	wglMakeCurrent(impl->hdc, impl->hglrc);

	return 0;
}

void
puglShowWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

	ShowWindow(impl->hwnd, SW_SHOWNORMAL);
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
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(view->impl->hglrc);
	ReleaseDC(view->impl->hwnd, view->impl->hdc);
	DestroyWindow(view->impl->hwnd);
	UnregisterClass(view->impl->wc.lpszClassName, NULL);
	free(view->windowClass);
	free(view->impl);
	free(view);
}

static PuglKey
keySymToSpecial(int sym)
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

static unsigned int
getModifiers()
{
	unsigned int mods = 0;
	mods |= (GetKeyState(VK_SHIFT)   < 0) ? PUGL_MOD_SHIFT  : 0;
	mods |= (GetKeyState(VK_CONTROL) < 0) ? PUGL_MOD_CTRL   : 0;
	mods |= (GetKeyState(VK_MENU)    < 0) ? PUGL_MOD_ALT    : 0;
	mods |= (GetKeyState(VK_LWIN)    < 0) ? PUGL_MOD_SUPER  : 0;
	mods |= (GetKeyState(VK_RWIN)    < 0) ? PUGL_MOD_SUPER  : 0;
	return mods;
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

	event->button.time   = GetMessageTime();
	event->button.type   = press ? PUGL_BUTTON_PRESS : PUGL_BUTTON_RELEASE;
	event->button.x      = GET_X_LPARAM(lParam);
	event->button.y      = GET_Y_LPARAM(lParam);
	event->button.x_root = pt.x;
	event->button.y_root = pt.y;
	event->button.state  = getModifiers();
	event->button.button = button;
}

static void
initScrollEvent(PuglEvent* event, PuglView* view, LPARAM lParam, WPARAM wParam)
{
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ScreenToClient(view->impl->hwnd, &pt);

	event->scroll.time   = GetMessageTime();
	event->scroll.type   = PUGL_SCROLL;
	event->scroll.x      = pt.x;
	event->scroll.y      = pt.y;
	event->scroll.x_root = GET_X_LPARAM(lParam);
	event->scroll.y_root = GET_Y_LPARAM(lParam);
	event->scroll.state  = getModifiers();
	event->scroll.dx     = 0;
	event->scroll.dy     = 0;
}

static unsigned int
utf16_to_code_point(const wchar_t* input, size_t input_size)
{
	unsigned int code_unit = *input;
	// Equiv. range check between 0xD800 to 0xDBFF inclusive
	if ((code_unit & 0xFC00) == 0xD800) {
		if (input_size < 2) {
			// "Error: is surrogate but input_size too small"
			return 0xFFFD;  // replacement character
		}

		unsigned int code_unit_2 = *++input;
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
	event->key.time      = GetMessageTime();
	event->key.state     = getModifiers();
	event->key.x_root    = rpos.x;
	event->key.y_root    = rpos.y;
	event->key.x         = cpos.x;
	event->key.y         = cpos.y;
	event->key.keycode   = (lParam & 0xFF0000) >> 16;
	event->key.character = 0;
	event->key.special   = static_cast<PuglKey>(0);
	event->key.filter    = 0;
}

static void
wcharBufToEvent(wchar_t* buf, int n, PuglEvent* event)
{
	if (n > 0) {
		char* charp = reinterpret_cast<char*>(event->key.utf8);
		if (!WideCharToMultiByte(CP_UTF8, 0, buf, n,
		                         charp, 8, NULL, NULL)) {
			/* error: could not convert to utf-8,
			   GetLastError has details */
			memset(event->key.utf8, 0, 8);
			// replacement character
			event->key.utf8[0] = 0xEF;
			event->key.utf8[1] = 0xBF;
			event->key.utf8[2] = 0xBD;
		}

		event->key.character = utf16_to_code_point(buf, n);
	} else {
		// replacement character
		event->key.utf8[0]   = 0xEF;
		event->key.utf8[1]   = 0xBF;
		event->key.utf8[2]   = 0xBD;
		event->key.character = 0xFFFD;
	}
}

static void
translateMessageParamsToEvent(LPARAM lParam, WPARAM wParam, PuglEvent* event)
{
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
	UINT c = MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
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

static LRESULT
handleMessage(PuglView* view, UINT message, WPARAM wParam, LPARAM lParam)
{
	PuglEvent   event;
	void*       dummy_ptr = NULL;
	RECT        rect;
	MINMAXINFO* mmi;
	POINT       pt;
	bool        dispatchThisEvent = true;

	memset(&event, 0, sizeof(event));

	event.any.type = PUGL_NOTHING;
	event.any.view = view;
	if (InSendMessageEx(dummy_ptr)) {
		event.any.flags |= PUGL_IS_SEND_EVENT;
	}

	switch (message) {
	case WM_CREATE:
	case WM_SHOWWINDOW:
	case WM_SIZE:
		GetWindowRect(view->impl->hwnd, &rect);
		event.configure.type   = PUGL_CONFIGURE;
		event.configure.x      = rect.left;
		event.configure.y      = rect.top;
		view->width            = rect.right - rect.left;
		view->height           = rect.bottom - rect.top;
		event.configure.width  = view->width;
		event.configure.height = view->height;
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
	case WM_MOUSEMOVE:
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		ClientToScreen(view->impl->hwnd, &pt);

		event.motion.type    = PUGL_MOTION_NOTIFY;
		event.motion.time    = GetMessageTime();
		event.motion.x       = GET_X_LPARAM(lParam);
		event.motion.y       = GET_Y_LPARAM(lParam);
		event.motion.x_root  = pt.x;
		event.motion.y_root  = pt.y;
		event.motion.state   = getModifiers();
		event.motion.is_hint = false;
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
		initScrollEvent(&event, view, lParam, wParam);
		event.scroll.dy = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	case WM_MOUSEHWHEEL:
		initScrollEvent(&event, view, lParam, wParam);
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
	case WM_QUIT:
	case PUGL_LOCAL_CLOSE_MSG:
		event.close.type = PUGL_CLOSE;
		break;
	default:
		return DefWindowProc(
			view->impl->hwnd, message, wParam, lParam);
	}

	puglDispatchEvent(view, &event);

	return 0;
}

void
puglGrabFocus(PuglView* view)
{
	// TODO
}

PuglStatus
puglWaitForEvent(PuglView* view)
{
	WaitMessage();
	return PUGL_SUCCESS;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	MSG msg;
	while (PeekMessage(&msg, view->impl->hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		handleMessage(view, msg.message, msg.wParam, msg.lParam);
	}

	if (view->redisplay) {
		InvalidateRect(view->impl->hwnd, NULL, FALSE);
		view->redisplay = false;
	}

	return PUGL_SUCCESS;
}

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN64
	PuglView* view = (PuglView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	PuglView* view = (PuglView*)GetWindowLongPtr(hwnd, GWL_USERDATA);
#endif

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

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->hwnd;
}
