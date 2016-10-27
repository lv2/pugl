/*
  Copyright 2012-2016 David Robillard <http://drobilla.net>

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
   @file pugl_osx.m OSX/Cocoa Pugl Implementation.
*/

#include <stdlib.h>

#import <Cocoa/Cocoa.h>

#include "pugl/cairo_gl.h"
#include "pugl/gl.h"
#include "pugl/pugl_internal.h"

@class PuglOpenGLView;

struct PuglInternalsImpl {
	NSApplication*   app;
	PuglOpenGLView*  glview;
	id               window;
	NSEvent*         nextEvent;
#ifdef PUGL_HAVE_CAIRO
	cairo_surface_t* surface;
	cairo_t*         cr;
	PuglCairoGL      cairo_gl;
#endif
};

@interface PuglWindow : NSWindow
{
@public
	PuglView* puglview;
}

- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(unsigned int)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag;
- (void) setPuglview:(PuglView*)view;
- (BOOL) windowShouldClose:(id)sender;
- (BOOL) canBecomeKeyWindow:(id)sender;
@end

@implementation PuglWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(unsigned int)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
	if (![super initWithContentRect:contentRect
	                      styleMask:(NSClosableWindowMask |
	                                 NSTitledWindowMask |
	                                 NSResizableWindowMask)
	                        backing:NSBackingStoreBuffered defer:NO]) {
		return nil;
	}

	[self setAcceptsMouseMovedEvents:YES];
	return (PuglWindow*)self;
}

- (void)setPuglview:(PuglView*)view
{
	puglview = view;
	[self setContentSize:NSMakeSize(view->width, view->height)];
}

- (BOOL)windowShouldClose:(id)sender
{
	const PuglEventClose ev = {
		PUGL_CLOSE,
		puglview,
		0
	};
	puglDispatchEvent(puglview, (PuglEvent*)&ev);

	return YES;
}

- (BOOL) canBecomeKeyWindow
{
	return YES;
}

- (BOOL) canBecomeMainWindow
{
	return YES;
}

- (BOOL) canBecomeKeyWindow:(id)sender
{
	return NO;
}

@end

@interface PuglOpenGLView : NSOpenGLView
{
@public
	PuglView* puglview;

	NSTrackingArea* trackingArea;
}

- (id) initWithFrame:(NSRect)frame;
- (void) reshape;
- (void) drawRect:(NSRect)rect;
- (NSPoint) eventLocation:(NSEvent*)event;
- (void) mouseEntered:(NSEvent*)event;
- (void) mouseExited:(NSEvent*)event;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDragged:(NSEvent*)event;
- (void) rightMouseDragged:(NSEvent*)event;
- (void) mouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseDragged:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) otherMouseDragged:(NSEvent*)event;
- (void) otherMouseDown:(NSEvent*)event;
- (void) otherMouseUp:(NSEvent*)event;
- (void) scrollWheel:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;
- (void) flagsChanged:(NSEvent*)event;

@end

@implementation PuglOpenGLView

- (id) initWithFrame:(NSRect)frame
{
	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 32,
		0
	};

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
	[[self openGLContext] update];

	if (!puglview) {
		return;
	}

	const NSRect             bounds = [self bounds];
	const PuglEventConfigure ev     =  {
		PUGL_CONFIGURE,
		puglview,
		0,
		bounds.origin.x,
		bounds.origin.y,
		bounds.size.width,
		bounds.size.height,
	};

#ifdef PUGL_HAVE_CAIRO
	PuglInternals* impl = puglview->impl;
	if (puglview->ctx_type & PUGL_CAIRO) {
		cairo_surface_destroy(impl->surface);
		cairo_destroy(impl->cr);
		impl->surface = pugl_cairo_gl_create(
			&impl->cairo_gl, ev.width, ev.height, 4);
		impl->cr = cairo_create(impl->surface);
		pugl_cairo_gl_configure(&impl->cairo_gl, ev.width, ev.height);
	}
#endif

	puglDispatchEvent(puglview, (PuglEvent*)&ev);
}

- (void) drawRect:(NSRect)rect
{
	const PuglEventExpose ev =  {
		PUGL_EXPOSE,
		puglview,
		0,
		rect.origin.x,
		rect.origin.y,
		rect.size.width,
		rect.size.height,
		0
	};

	puglDispatchEvent(puglview, (const PuglEvent*)&ev);

#ifdef PUGL_HAVE_CAIRO
	if (puglview->ctx_type & PUGL_CAIRO) {
		pugl_cairo_gl_draw(
			&puglview->impl->cairo_gl, puglview->width, puglview->height);
	}
#endif
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

static unsigned
getModifiers(PuglView* view, NSEvent* ev)
{
	const unsigned modifierFlags = [ev modifierFlags];

	unsigned mods = 0;
	mods |= (modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0;
	mods |= (modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0;
	mods |= (modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0;
	mods |= (modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0;
	return mods;
}

-(void)updateTrackingAreas
{
	if (trackingArea != nil) {
		[self removeTrackingArea:trackingArea];
		[trackingArea release];
	}

	const int opts = (NSTrackingMouseEnteredAndExited |
	                  NSTrackingMouseMoved |
	                  NSTrackingActiveAlways);
	trackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds]
	                                             options:opts
	                                               owner:self
	                                            userInfo:nil];
	[self addTrackingArea:trackingArea];
}

- (NSPoint) eventLocation:(NSEvent*)event
{
	return [self convertPoint:[event locationInWindow] fromView:nil];
}

- (void)mouseEntered:(NSEvent*)theEvent
{
	[self updateTrackingAreas];
}

- (void)mouseExited:(NSEvent*)theEvent
{
}

- (void) mouseMoved:(NSEvent*)event
{
	const NSPoint         wloc = [self eventLocation:event];
	const NSPoint         rloc = [NSEvent mouseLocation];
	const PuglEventMotion ev   =  {
		PUGL_MOTION_NOTIFY,
		puglview,
		0,
		[event timestamp],
		wloc.x,
		puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(puglview, event),
		0,
		1
	};
	puglDispatchEvent(puglview, (PuglEvent*)&ev);
}

- (void) mouseDragged:(NSEvent*)event
{
	[self mouseMoved: event];
}

- (void) rightMouseDragged:(NSEvent*)event
{
	[self mouseMoved: event];
}

- (void) otherMouseDragged:(NSEvent*)event
{
	[self mouseMoved: event];
}

- (void) mouseDown:(NSEvent*)event
{
	const NSPoint         wloc = [self eventLocation:event];
	const NSPoint         rloc = [NSEvent mouseLocation];
	const PuglEventButton ev   =  {
		PUGL_BUTTON_PRESS,
		puglview,
		0,
		[event timestamp],
		wloc.x,
		puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(puglview, event),
		[event buttonNumber] + 1
	};
	puglDispatchEvent(puglview, (PuglEvent*)&ev);
}

- (void) mouseUp:(NSEvent*)event
{
	const NSPoint         wloc = [self eventLocation:event];
	const NSPoint         rloc = [NSEvent mouseLocation];
	const PuglEventButton ev   =  {
		PUGL_BUTTON_RELEASE,
		puglview,
		0,
		[event timestamp],
		wloc.x,
		puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(puglview, event),
		[event buttonNumber] + 1
	};
	puglDispatchEvent(puglview, (PuglEvent*)&ev);
	[self updateTrackingAreas];
}

- (void) rightMouseDown:(NSEvent*)event
{
	[self mouseDown: event];
}

- (void) rightMouseUp:(NSEvent*)event
{
	[self mouseUp: event];
}

- (void) otherMouseDown:(NSEvent*)event
{
	[self mouseDown: event];
}

- (void) otherMouseUp:(NSEvent*)event
{
	[self mouseUp: event];
}

- (void) scrollWheel:(NSEvent*)event
{
	[self updateTrackingAreas];

	const NSPoint         wloc = [self eventLocation:event];
	const NSPoint         rloc = [NSEvent mouseLocation];
	const PuglEventScroll ev   =  {
		PUGL_SCROLL,
		puglview,
		0,
		[event timestamp],
		wloc.x,
		puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(puglview, event),
		[event deltaX],
		[event deltaY]
	};
	puglDispatchEvent(puglview, (PuglEvent*)&ev);
	[self updateTrackingAreas];
}

- (void) keyDown:(NSEvent*)event
{
	if (puglview->ignoreKeyRepeat && [event isARepeat]) {
		return;
	}

	const NSPoint      wloc  = [self eventLocation:event];
	const NSPoint      rloc  = [NSEvent mouseLocation];
	const NSString*    chars = [event characters];
	const char*        str   = [chars UTF8String];
	PuglEventKey       ev    =  {
		PUGL_KEY_PRESS,
		puglview,
		0,
		[event timestamp],
		wloc.x,
		puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(puglview, event),
		[event keyCode],
		puglDecodeUTF8((const uint8_t*)str),
		0,  // TODO: Special keys?
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		false
	};
	strncpy((char*)ev.utf8, str, 8);
	puglDispatchEvent(puglview, (PuglEvent*)&ev);
}

- (void) keyUp:(NSEvent*)event
{
	const NSPoint      wloc  = [self eventLocation:event];
	const NSPoint      rloc  = [NSEvent mouseLocation];
	const NSString*    chars = [event characters];
	const char*        str   = [chars UTF8String];
	const PuglEventKey ev    =  {
		PUGL_KEY_RELEASE,
		puglview,
		0,
		[event timestamp],
		wloc.x,
		puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(puglview, event),
		[event keyCode],
		puglDecodeUTF8((const uint8_t*)str),
		0,  // TODO: Special keys?
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		false,
	};
	strncpy((char*)ev.utf8, str, 8);
	puglDispatchEvent(puglview, (PuglEvent*)&ev);
}

- (void) flagsChanged:(NSEvent*)event
{
	// TODO: Is this a sensible way to handle special keys?
	/*
		const unsigned mods = getModifiers(puglview, event);
		if ((mods & PUGL_MOD_SHIFT) != (puglview->mods & PUGL_MOD_SHIFT)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_SHIFT, PUGL_KEY_SHIFT);
		} else if ((mods & PUGL_MOD_CTRL) != (puglview->mods & PUGL_MOD_CTRL)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_CTRL, PUGL_KEY_CTRL);
		} else if ((mods & PUGL_MOD_ALT) != (puglview->mods & PUGL_MOD_ALT)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_ALT, PUGL_KEY_ALT);
		} else if ((mods & PUGL_MOD_SUPER) != (puglview->mods & PUGL_MOD_SUPER)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_SUPER, PUGL_KEY_SUPER);
		}
		puglview->mods = mods;
	}
	*/
}

@end

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

void
puglEnterContext(PuglView* view)
{
	[[view->impl->glview openGLContext] makeCurrentContext];
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		cairo_set_source_rgb(view->impl->cr, 0, 0, 0);
		cairo_paint(view->impl->cr);
	}
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		pugl_cairo_gl_draw(&view->impl->cairo_gl, view->width, view->height);
	}
#endif

	if (flush) {
		[[view->impl->glview openGLContext] flushBuffer];
	}
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;

	[NSAutoreleasePool new];
	impl->app = [NSApplication sharedApplication];

	impl->glview           = [PuglOpenGLView new];
	impl->glview->puglview = view;

	if (view->transient_parent) {
		NSView* pview = (NSView*)view->transient_parent;
		[pview addSubview:impl->glview];
		[impl->glview setHidden:NO];
	} else {
		NSString* titleString = [[NSString alloc]
			                        initWithBytes:title
			                               length:strlen(title)
			                             encoding:NSUTF8StringEncoding];

		id window = [[PuglWindow new] retain];
		[window setPuglview:view];
		[window setTitle:titleString];
		if (view->min_width || view->min_height) {
			[window setContentMinSize:NSMakeSize(view->min_width,
			                                     view->min_height)];
		}
		impl->window = window;

		[window setContentView:impl->glview];
		[impl->app activateIgnoringOtherApps:YES];
		[window makeFirstResponder:impl->glview];
		[window makeKeyAndOrderFront:window];
#if 0
		if (resizable) {
			[impl->glview setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
		}
#endif
	}

	return 0;
}

void
puglShowWindow(PuglView* view)
{
	[view->impl->window setIsVisible:YES];
	view->visible = true;
}

void
puglHideWindow(PuglView* view)
{
	[view->impl->window setIsVisible:NO];
	view->visible = false;
}

void
puglDestroy(PuglView* view)
{
#ifdef PUGL_HAVE_CAIRO
	pugl_cairo_gl_free(&view->impl->cairo_gl);
#endif
	view->impl->glview->puglview = NULL;
	[view->impl->glview removeFromSuperview];
	if (view->impl->window) {
		[view->impl->window close];
	}
	[view->impl->glview release];
	if (view->impl->window) {
		[view->impl->window release];
	}
	free(view->windowClass);
	free(view->impl);
	free(view);
}

void
puglGrabFocus(PuglView* view)
{
	// TODO
}

PuglStatus
puglWaitForEvent(PuglView* view)
{
	/* OSX supposedly has queue: and untilDate: selectors that can be used for
	   a blocking non-queueing event check, but if used here cause an
	   unsupported selector error at runtime.  I have no idea why, so just get
	   the event and keep it around until the call to puglProcessEvents. */
	if (!view->impl->nextEvent) {
		view->impl->nextEvent = [view->impl->window
		                            nextEventMatchingMask: NSAnyEventMask];
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	while (true) {
		// Get the next event, or use the cached one from puglWaitForEvent
		if (!view->impl->nextEvent) {
			view->impl->nextEvent = [view->impl->window
			                            nextEventMatchingMask: NSAnyEventMask];
		}

		if (!view->impl->nextEvent) {
			break;  // No events to process, done
		}

		// Dispatch event
		[view->impl->app sendEvent: view->impl->nextEvent];
		view->impl->nextEvent = NULL;
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	//view->redisplay = true; // unused
	[view->impl->glview setNeedsDisplay: YES];
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->glview;
}

void*
puglGetContext(PuglView* view)
{
#ifdef PUGL_HAVE_CAIRO
	if (view->ctx_type & PUGL_CAIRO) {
		return view->impl->cr;
	}
#endif
	return NULL;
}
