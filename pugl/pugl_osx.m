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

#include <stdlib.h>

#import <Cocoa/Cocoa.h>

#include "pugl_internal.h"

@interface PuglOpenGLView : NSOpenGLView
{
	int colorBits;
	int depthBits;
@public
	PuglWindow* win;
}

- (id) initWithFrame:(NSRect)frame
           colorBits:(int)numColorBits
           depthBits:(int)numDepthBits;
- (void) reshape;
- (void) drawRect:(NSRect)rect;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;

@end

@implementation PuglOpenGLView

- (id) initWithFrame:(NSRect)frame
           colorBits:(int)numColorBits
           depthBits:(int)numDepthBits
{
	colorBits = numColorBits;
	depthBits = numDepthBits;

	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAColorSize,
		colorBits,
		NSOpenGLPFADepthSize,
		depthBits,
		0
	};

	NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc]
		              initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
		if (self) {
			[[self openGLContext] makeCurrentContext];
			[self reshape];
		}
	} else {
		self = nil;
	}

	return self;
}

- (void) reshape
{
	[[self openGLContext] update];

	NSRect bounds = [self bounds];
	int    width  = bounds.size.width;
	int    height = bounds.size.height;

	if (win->reshapeFunc) {
		// User provided a reshape function, defer to that
		win->reshapeFunc(win, width, height);
	} else {
		// No custom reshape function, do something reasonable
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, width/(float)height, 1.0f, 10.0f);
		glViewport(0, 0, width, height);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	win->width     = width;
	win->height    = height;
	win->redisplay = true;
}

- (void) drawRect:(NSRect)rect
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (self->win->displayFunc) {
		self->win->displayFunc(self->win);
	}

	glFlush();
	glSwapAPPLE();
}

- (void) mouseMoved:(NSEvent*)event
{
	NSPoint loc = [event locationInWindow];
	if (win->motionFunc) {
		win->motionFunc(win, loc.x, loc.y);
	}
}

- (void) mouseDown:(NSEvent*)event
{
	NSPoint loc = [event locationInWindow];
	if (win->mouseFunc) {
		win->mouseFunc(win, 1, true, loc.x, loc.y);
	}
}

- (void) mouseUp:(NSEvent*)event
{
	NSPoint loc = [event locationInWindow];
	if (win->mouseFunc) {
		win->mouseFunc(win, 1, false, loc.x, loc.y);
	}
}

- (void) rightMouseDown:(NSEvent*)event
{
	NSPoint loc = [event locationInWindow];
	if (win->mouseFunc) {
		win->mouseFunc(win, 3, true, loc.x, loc.y);
	}
}

- (void) rightMouseUp:(NSEvent*)event
{
	NSPoint loc = [event locationInWindow];
	if (win->mouseFunc) {
		win->mouseFunc(win, 3, false, loc.x, loc.y);
	}
}

- (void) scrollWheel:(NSEvent*)event
{
	if (win->scrollFunc) {
		win->scrollFunc(win, [event deltaX], [event deltaY]);
	}
}

- (void) keyDown:(NSEvent*)event
{
	NSString* chars = [event characters];;
	if (win->keyboardFunc) {
		win->keyboardFunc(win, true, [chars characterAtIndex:0]);
	}
}

- (void) keyUp:(NSEvent*)event
{
	NSString* chars = [event characters];;
	if (win->keyboardFunc) {
		win->keyboardFunc(win, false,  [chars characterAtIndex:0]);
	}
}

@end

struct PuglPlatformDataImpl {
	PuglOpenGLView* view;
	NSModalSession  session;
	id              window;
};

PuglWindow*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              width,
           int              height,
           bool             resizable)
{
	PuglWindow* win = (PuglWindow*)calloc(1, sizeof(PuglWindow));
	win->width  = width;
	win->height = height;

	win->impl = (PuglPlatformData*)calloc(1, sizeof(PuglPlatformData));

	PuglPlatformData* impl = win->impl;

	[NSAutoreleasePool new];
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	NSString* titleString = [[NSString alloc]
		                        initWithBytes:title
		                               length:strlen(title)
		                             encoding:NSUTF8StringEncoding];

	id window = [[[NSWindow alloc]
		             initWithContentRect:NSMakeRect(0, 0, 512, 512)
		                       styleMask:NSTitledWindowMask
		                         backing:NSBackingStoreBuffered
		                           defer:NO]
		            autorelease];

	[window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
	[window setTitle:titleString];
	[window setAcceptsMouseMovedEvents:YES];

	impl->view      = [PuglOpenGLView new];
	impl->window    = window;
	impl->view->win = win;

	[window setContentView:impl->view];
	[NSApp activateIgnoringOtherApps:YES];
	[window makeFirstResponder:impl->view];

	impl->session = [NSApp beginModalSessionForWindow:win->impl->window];

	return win;
}

void
puglDestroy(PuglWindow* win)
{
	[NSApp endModalSession:win->impl->session];
	[win->impl->view release];
	free(win->impl);
	free(win);
}

void
puglDisplay(PuglWindow* win)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (win->displayFunc) {
		win->displayFunc(win);
	}

	glFlush();
	win->redisplay = false;
}

PuglStatus
puglProcessEvents(PuglWindow* win)
{
	NSInteger response = [NSApp runModalSession:win->impl->session];
	if (response != NSRunContinuesResponse) {
		if (win->closeFunc) {
			win->closeFunc(win);
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
	return (PuglNativeWindow)win->impl->view;
}
