/*
  Copyright 2019-2020 David Robillard <d@drobilla.net>

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
   @file mac_gl.m
   @brief OpenGL graphics backend for MacOS.
*/

#include "pugl/detail/implementation.h"
#include "pugl/detail/mac.h"
#include "pugl/detail/stub.h"
#include "pugl/pugl_gl.h"

#ifndef __MAC_10_10
#    define NSOpenGLProfileVersion4_1Core NSOpenGLProfileVersion3_2Core
#endif

@interface PuglOpenGLView : NSOpenGLView
@end

@implementation PuglOpenGLView
{
@public
	PuglView* puglview;
}

- (id) initWithFrame:(NSRect)frame
{
	const bool     compat  = puglview->hints[PUGL_USE_COMPAT_PROFILE];
	const unsigned samples = (unsigned)puglview->hints[PUGL_SAMPLES];
	const int      major   = puglview->hints[PUGL_CONTEXT_VERSION_MAJOR];
	const unsigned profile = ((compat || major < 3)
	                              ? NSOpenGLProfileVersionLegacy
	                              : (major >= 4
	                                     ? NSOpenGLProfileVersion4_1Core
	                                     : NSOpenGLProfileVersion3_2Core));

	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAOpenGLProfile, profile,
		NSOpenGLPFAColorSize,     32,
		NSOpenGLPFADepthSize,     32,
		NSOpenGLPFAMultisample,   samples ? 1 : 0,
		NSOpenGLPFASampleBuffers, samples ? 1 : 0,
		NSOpenGLPFASamples,       samples,
		0};

	NSOpenGLPixelFormat* pixelFormat = [
		[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
	} else {
		self = [super initWithFrame:frame];
	}

	[self setWantsBestResolutionOpenGLSurface:YES];

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
	[wrapper setReshaped];
}

- (void) drawRect:(NSRect)rect
{
	PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
	[wrapper dispatchExpose:rect];
}

@end

static PuglStatus
puglMacGlCreate(PuglView* view)
{
	PuglInternals*  impl     = view->impl;
	PuglOpenGLView* drawView = [PuglOpenGLView alloc];

	drawView->puglview = view;
	[drawView initWithFrame:[impl->wrapperView bounds]];
	if (view->hints[PUGL_RESIZABLE]) {
		[drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	} else {
		[drawView setAutoresizingMask:NSViewNotSizable];
	}

	impl->drawView = drawView;
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlDestroy(PuglView* view)
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[drawView removeFromSuperview];
	[drawView release];

	view->impl->drawView = nil;
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlEnter(PuglView* view, const PuglEventExpose* PUGL_UNUSED(expose))
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	[[drawView openGLContext] makeCurrentContext];
	return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlLeave(PuglView* view, const PuglEventExpose* expose)
{
	PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

	if (expose) {
		[[drawView openGLContext] flushBuffer];
	}

	[NSOpenGLContext clearCurrentContext];

	return PUGL_SUCCESS;
}

PuglGlFunc
puglGetProcAddress(const char *name)
{
	CFBundleRef framework =
		CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));

	CFStringRef symbol = CFStringCreateWithCString(
		kCFAllocatorDefault, name, kCFStringEncodingASCII);

	PuglGlFunc func = (PuglGlFunc)CFBundleGetFunctionPointerForName(
		framework, symbol);

	CFRelease(symbol);

	return func;
}

const PuglBackend* puglGlBackend(void)
{
	static const PuglBackend backend = {puglStubConfigure,
	                                    puglMacGlCreate,
	                                    puglMacGlDestroy,
	                                    puglMacGlEnter,
	                                    puglMacGlLeave,
	                                    puglStubGetContext};

	return &backend;
}
