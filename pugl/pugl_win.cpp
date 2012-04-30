/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "pugl_internal.h"

struct PuglPlatformDataImpl {
	HWND  hwnd;
	HDC   hdc;
	HGLRC hglrc;
};

LRESULT CALLBACK
wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		PostMessage(hwnd, WM_SHOWWINDOW, TRUE, 0);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_DESTROY:
		return 0;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		PostMessage(hwnd, message, wParam, lParam);
		return 0;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));

	view->impl = (PuglPlatformData*)calloc(1, sizeof(PuglPlatformData));

	PuglPlatformData* impl = view->impl;

	WNDCLASS wc;
	wc.style         = CS_OWNDC;
	wc.lpfnWndProc   = wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = 0;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "Pugl";
	RegisterClass(&wc);

	impl->hwnd = CreateWindow("Pugl", title,
	                          WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
	                          0, 0, width, height,
	                          (HWND)parent, NULL, NULL, NULL);

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
	wglMakeCurrent(impl->hdc, impl->hglrc);

	view->width  = width;
	view->height = height;

	return view;
}

void
puglDestroy(PuglView* view)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(view->impl->hglrc);
	ReleaseDC(view->impl->hwnd, view->impl->hdc);
	DestroyWindow(view->impl->hwnd);
	free(view);
}

void
puglReshape(PuglView* view, int width, int height)
{
	wglMakeCurrent(view->impl->hdc, view->impl->hglrc);

	if (view->reshapeFunc) {
		// User provided a reshape function, defer to that
		view->reshapeFunc(view, width, height);
	} else {
		// No custom reshape function, do something reasonable
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, view->width/(float)view->height, 1.0f, 10.0f);
		glViewport(0, 0, view->width, view->height);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	view->width     = width;
	view->height    = height;
}

void
puglDisplay(PuglView* view)
{
	wglMakeCurrent(view->impl->hdc, view->impl->hglrc);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (view->displayFunc) {
		view->displayFunc(view);
	}

	glFlush();
	SwapBuffers(view->impl->hdc);
	view->redisplay = false;
}

static void
processMouseEvent(PuglView* view, int button, bool press, LPARAM lParam)
{
	if (view->mouseFunc) {
		view->mouseFunc(view, button, press,
		                GET_X_LPARAM(lParam),
		                GET_Y_LPARAM(lParam));
	}
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	MSG         msg;
	PAINTSTRUCT ps;
	int         button;
	bool        down = true;
	while (PeekMessage(&msg, /*view->impl->hwnd*/0, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_CREATE:
		case WM_SHOWWINDOW:
		case WM_SIZE:
			puglReshape(view, view->width, view->height);
			break;
		case WM_PAINT:
			BeginPaint(view->impl->hwnd, &ps);
			puglDisplay(view);
			EndPaint(view->impl->hwnd, &ps);
			break;
		case WM_MOUSEMOVE:
			if (view->motionFunc) {
				view->motionFunc(
					view, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
			}
			break;
		case WM_LBUTTONDOWN:
			processMouseEvent(view, 1, true, msg.lParam);
			break;
		case WM_MBUTTONDOWN:
			processMouseEvent(view, 2, true, msg.lParam);
			break;
		case WM_RBUTTONDOWN:
			processMouseEvent(view, 3, true, msg.lParam);
			break;
		case WM_LBUTTONUP:
			processMouseEvent(view, 1, false, msg.lParam);
			break;
		case WM_MBUTTONUP:
			processMouseEvent(view, 2, false, msg.lParam);
			break;
		case WM_RBUTTONUP:
			processMouseEvent(view, 3, false, msg.lParam);
			break;
		case WM_MOUSEWHEEL:
			if (view->scrollFunc) {
				view->scrollFunc(
					view, 0, (int16_t)HIWORD(msg.wParam) / (float)WHEEL_DELTA);
			}
			break;
		case WM_MOUSEHWHEEL:
			if (view->scrollFunc) {
				view->scrollFunc(
					view, (int16_t)HIWORD(msg.wParam) / float(WHEEL_DELTA), 0);
			}
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (view->keyboardFunc) {
				view->keyboardFunc(view, msg.message == WM_KEYDOWN, msg.wParam);
			}
			break;
		case WM_QUIT:
			if (view->closeFunc) {
				view->closeFunc(view);
			}
			break;
		default:
			DefWindowProc(
				view->impl->hwnd, msg.message, msg.wParam, msg.lParam);
		}
	}

	if (view->redisplay) {
		puglDisplay(view);
	}

	return PUGL_SUCCESS;
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
