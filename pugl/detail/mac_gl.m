/*
  Copyright 2019 David Robillard <http://drobilla.net>

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
   @file mac_gl.m OpenGL graphics backend for MacOS.
*/

#include "pugl/detail/implementation.h"
#include "pugl/detail/mac.h"
#include "pugl/pugl_gl_backend.h"

#ifndef __MAC_10_10
#define NSOpenGLProfileVersion4_1Core NSOpenGLProfileVersion3_2Core
typedef NSUInteger NSEventModifierFlags;
typedef NSUInteger NSWindowStyleMask;
#endif

@interface PuglOpenGLView : NSOpenGLView
{
@public
	PuglView* puglview;
}

@end

@implementation PuglOpenGLView

- (id) initWithFrame:(NSRect)frame
{
	const int major   = puglview->hints.context_version_major;
	const int profile = ((puglview->hints.use_compat_profile || major < 3)
	                     ? NSOpenGLProfileVersionLegacy
	                     : puglview->hints.context_version_major >= 4
	                       ? NSOpenGLProfileVersion4_1Core
	                       : NSOpenGLProfileVersion3_2Core);

	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAOpenGLProfile, profile,
		NSOpenGLPFAColorSize,     32,
		NSOpenGLPFADepthSize,     32,
		NSOpenGLPFAMultisample,   puglview->hints.samples ? 1 : 0,
		NSOpenGLPFASampleBuffers, puglview->hints.samples ? 1 : 0,
		NSOpenGLPFASamples,       puglview->hints.samples,
		0};

	NSOpenGLPixelFormat* pixelFormat = [
		[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
	} else {
		self = [super initWithFrame:frame];
	}

	if (self) {
		[[self openGLContext] makeCurrentContext];
		[self reshape];
	}
	return self;
}

- (void) reshape
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

	[super reshape];
	[wrapper dispatchConfigure:[self bounds]];
}

- (void) update
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

	[super update];
	[wrapper dispatchConfigure:[self bounds]];
}

@end

static int
puglMacGlConfigure(PuglView* PUGL_UNUSED(view))
{
	return 0;
}

static int
puglMacGlCreate(PuglView* view)
{
	PuglInternals*  impl     = view->impl;
	PuglOpenGLView* drawView = [PuglOpenGLView alloc];

	drawView->puglview = view;
	[drawView initWithFrame:NSMakeRect(0, 0, view->width, view->height)];
	if (view->hints.resizable) {
		[drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	} else {
		[drawView setAutoresizingMask:NSViewNotSizable];
	}

	impl->drawView = drawView;
	return 0;
}

static int
puglMacGlDestroy(PuglView* view)
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[drawView removeFromSuperview];
	[drawView release];

	view->impl->drawView = nil;
	return 0;
}

static int
puglMacGlEnter(PuglView* view, bool PUGL_UNUSED(drawing))
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[[drawView openGLContext] makeCurrentContext];
	return 0;
}

static int
puglMacGlLeave(PuglView* view, bool drawing)
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	if (drawing) {
		[[drawView openGLContext] flushBuffer];
	}

	[NSOpenGLContext clearCurrentContext];

	return 0;
}

static int
puglMacGlResize(PuglView* view, int PUGL_UNUSED(width), int PUGL_UNUSED(height))
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[drawView reshape];

	return 0;
}

static void*
puglMacGlGetContext(PuglView* PUGL_UNUSED(view))
{
	return NULL;
}

const PuglBackend* puglGlBackend(void)
{
	static const PuglBackend backend = {
		puglMacGlConfigure,
		puglMacGlCreate,
		puglMacGlDestroy,
		puglMacGlEnter,
		puglMacGlLeave,
		puglMacGlResize,
		puglMacGlGetContext
	};

	return &backend;
}
