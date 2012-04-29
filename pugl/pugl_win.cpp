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

PuglWindow*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable)
{
	PuglWindow* win = (PuglWindow*)calloc(1, sizeof(PuglWindow));

	win->impl = (PuglPlatformData*)calloc(1, sizeof(PuglPlatformData));

	PuglPlatformData* impl = win->impl;

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

	win->width  = width;
	win->height = height;

	return win;
}

void
puglDestroy(PuglWindow* win)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(win->impl->hglrc);
	ReleaseDC(win->impl->hwnd, win->impl->hdc);
	DestroyWindow(win->impl->hwnd);
	free(win);
}

void
puglReshape(PuglWindow* win, int width, int height)
{
	wglMakeCurrent(win->impl->hdc, win->impl->hglrc);

	if (win->reshapeFunc) {
		// User provided a reshape function, defer to that
		win->reshapeFunc(win, width, height);
	} else {
		// No custom reshape function, do something reasonable
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, win->width/(float)win->height, 1.0f, 10.0f);
		glViewport(0, 0, win->width, win->height);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	win->width     = width;
	win->height    = height;
}

void
puglDisplay(PuglWindow* win)
{
	wglMakeCurrent(win->impl->hdc, win->impl->hglrc);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (win->displayFunc) {
		win->displayFunc(win);
	}

	glFlush();
	SwapBuffers(win->impl->hdc);
	win->redisplay = false;
}

static void
processMouseEvent(PuglWindow* win, int button, bool press, LPARAM lParam)
{
	if (win->mouseFunc) {
		win->mouseFunc(win, button, press,
		               GET_X_LPARAM(lParam),
		               GET_Y_LPARAM(lParam));
	}
}

PuglStatus
puglProcessEvents(PuglWindow* win)
{
	MSG         msg;
	PAINTSTRUCT ps;
	int         button;
	bool        down = true;
	while (PeekMessage(&msg, /*win->impl->hwnd*/0, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_CREATE:
		case WM_SHOWWINDOW:
		case WM_SIZE:
			puglReshape(win, win->width, win->height);
			break;
		case WM_PAINT:
			BeginPaint(win->impl->hwnd, &ps);
			puglDisplay(win);
			EndPaint(win->impl->hwnd, &ps);
			break;
		case WM_MOUSEMOVE:
			if (win->motionFunc) {
				win->motionFunc(
					win, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
			}
			break;
		case WM_LBUTTONDOWN:
			processMouseEvent(win, 1, true, msg.lParam);
			break;
		case WM_MBUTTONDOWN:
			processMouseEvent(win, 2, true, msg.lParam);
			break;
		case WM_RBUTTONDOWN:
			processMouseEvent(win, 3, true, msg.lParam);
			break;
		case WM_LBUTTONUP:
			processMouseEvent(win, 1, false, msg.lParam);
			break;
		case WM_MBUTTONUP:
			processMouseEvent(win, 2, false, msg.lParam);
			break;
		case WM_RBUTTONUP:
			processMouseEvent(win, 3, false, msg.lParam);
			break;
		case WM_MOUSEWHEEL:
			if (win->scrollFunc) {
				win->scrollFunc(
					win, 0, (int16_t)HIWORD(msg.wParam) / (float)WHEEL_DELTA);
			}
			break;
		case WM_MOUSEHWHEEL:
			if (win->scrollFunc) {
				win->scrollFunc(
					win, (int16_t)HIWORD(msg.wParam) / float(WHEEL_DELTA), 0);
			}
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (win->keyboardFunc) {
				win->keyboardFunc(win, msg.message == WM_KEYDOWN, msg.wParam);
			}
			break;
		case WM_QUIT:
			if (win->closeFunc) {
				win->closeFunc(win);
			}
			break;
		default:
			DefWindowProc(
				win->impl->hwnd, msg.message, msg.wParam, msg.lParam);
		}
	}

	if (win->redisplay) {
		puglDisplay(win);
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglWindow* win)
{
	win->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglWindow* win)
{
	return (PuglNativeWindow)win->impl->hwnd;
}
