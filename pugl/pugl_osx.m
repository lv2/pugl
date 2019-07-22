/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>
  Copyright 2017 Hanspeter Portner <dev@open-music-kontrollers.ch>

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

#define GL_SILENCE_DEPRECATION 1

#include "pugl/gl.h"
#include "pugl/pugl_internal.h"

#ifdef PUGL_HAVE_CAIRO
#include "pugl/cairo_gl.h"
#endif

#import <Cocoa/Cocoa.h>

#include <stdlib.h>
#include <time.h>

@class PuglOpenGLView;

struct PuglInternalsImpl {
	NSApplication*   app;
	PuglOpenGLView*  glview;
	id               window;
	NSEvent*         nextEvent;
	uint32_t         mods;
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

- (void) setPuglview:(PuglView*)view;

@end

@implementation PuglWindow

- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(NSWindowStyleMask)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag
{
	NSWindow* result = [super initWithContentRect:contentRect
					    styleMask:aStyle
					      backing:bufferingType
						defer:NO];

	[result setAcceptsMouseMovedEvents:YES];
	return (PuglWindow*)result;
}

- (void)setPuglview:(PuglView*)view
{
	puglview = view;
	[self setContentSize:NSMakeSize(view->width, view->height)];
}

- (BOOL) canBecomeKeyWindow
{
	return YES;
}

- (BOOL) canBecomeMainWindow
{
	return YES;
}

@end

@interface PuglOpenGLView : NSOpenGLView
{
@public
	PuglView*       puglview;
	NSTrackingArea* trackingArea;
	NSTimer*        timer;
	NSTimer*        urgentTimer;
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

	NSOpenGLPixelFormat *pixelFormat = [
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
	[super reshape];
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

	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
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

- (BOOL) isFlipped
{
	return YES;
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

static uint32_t
getModifiers(PuglView* view, NSEvent* ev)
{
	const NSEventModifierFlags modifierFlags = [ev modifierFlags];

	return (((modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0) |
	        ((modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0) |
	        ((modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0) |
	        ((modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0));
}

static PuglKey
keySymToSpecial(PuglView* view, NSEvent* ev)
{
	NSString* chars = [ev charactersIgnoringModifiers];
	if ([chars length] == 1) {
		switch ([chars characterAtIndex:0]) {
		case NSF1FunctionKey:         return PUGL_KEY_F1;
		case NSF2FunctionKey:         return PUGL_KEY_F2;
		case NSF3FunctionKey:         return PUGL_KEY_F3;
		case NSF4FunctionKey:         return PUGL_KEY_F4;
		case NSF5FunctionKey:         return PUGL_KEY_F5;
		case NSF6FunctionKey:         return PUGL_KEY_F6;
		case NSF7FunctionKey:         return PUGL_KEY_F7;
		case NSF8FunctionKey:         return PUGL_KEY_F8;
		case NSF9FunctionKey:         return PUGL_KEY_F9;
		case NSF10FunctionKey:        return PUGL_KEY_F10;
		case NSF11FunctionKey:        return PUGL_KEY_F11;
		case NSF12FunctionKey:        return PUGL_KEY_F12;
		case NSLeftArrowFunctionKey:  return PUGL_KEY_LEFT;
		case NSUpArrowFunctionKey:    return PUGL_KEY_UP;
		case NSRightArrowFunctionKey: return PUGL_KEY_RIGHT;
		case NSDownArrowFunctionKey:  return PUGL_KEY_DOWN;
		case NSPageUpFunctionKey:     return PUGL_KEY_PAGE_UP;
		case NSPageDownFunctionKey:   return PUGL_KEY_PAGE_DOWN;
		case NSHomeFunctionKey:       return PUGL_KEY_HOME;
		case NSEndFunctionKey:        return PUGL_KEY_END;
		case NSInsertFunctionKey:     return PUGL_KEY_INSERT;
		}
		// SHIFT, CTRL, ALT, and SUPER are handled in [flagsChanged]
	}
	return (PuglKey)0;
}

- (void) updateTrackingAreas
{
	if (trackingArea != nil) {
		[self removeTrackingArea:trackingArea];
		[trackingArea release];
	}

	const int opts = (NSTrackingMouseEnteredAndExited |
	                  NSTrackingMouseMoved |
	                  NSTrackingActiveAlways);
	trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
	                                            options:opts
	                                              owner:self
	                                           userInfo:nil];
	[self addTrackingArea:trackingArea];
	[super updateTrackingAreas];
}

- (NSPoint) eventLocation:(NSEvent*)event
{
	return [self convertPoint:[event locationInWindow] fromView:nil];
}

static void
handleCrossing(PuglOpenGLView* view, NSEvent* event, const PuglEventType type)
{
	const NSPoint           wloc = [view eventLocation:event];
	const NSPoint           rloc = [NSEvent mouseLocation];
	const PuglEventCrossing ev   =  {
		type,
		view->puglview,
		0,
		[event timestamp],
		wloc.x,
		view->puglview->height - wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(view->puglview, event),
		PUGL_CROSSING_NORMAL
	};
	puglDispatchEvent(view->puglview, (const PuglEvent*)&ev);
}

- (void) mouseEntered:(NSEvent*)event
{
	handleCrossing(self, event, PUGL_ENTER_NOTIFY);
}

- (void) mouseExited:(NSEvent*)event
{
	handleCrossing(self, event, PUGL_LEAVE_NOTIFY);
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
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
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
		(uint32_t)[event buttonNumber] + 1
	};
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
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
		(uint32_t)[event buttonNumber] + 1
	};
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
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
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
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
	const uint32_t     code  = puglDecodeUTF8((const uint8_t*)str);
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
		(code != 0xFFFD) ? code : 0,
		keySymToSpecial(puglview, event),
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		false
	};
	strncpy((char*)ev.utf8, str, 8);
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
}

- (void) keyUp:(NSEvent*)event
{
	const NSPoint      wloc  = [self eventLocation:event];
	const NSPoint      rloc  = [NSEvent mouseLocation];
	const NSString*    chars = [event characters];
	const char*        str   = [chars UTF8String];
	PuglEventKey       ev    =  {
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
		keySymToSpecial(puglview, event),
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		false,
	};
	strncpy((char*)ev.utf8, str, 8);
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
}

- (void) flagsChanged:(NSEvent*)event
{
	const uint32_t mods    = getModifiers(puglview, event);
	PuglEventType  type    = PUGL_NOTHING;
	PuglKey        special = 0;

	if ((mods & PUGL_MOD_SHIFT) != (puglview->impl->mods & PUGL_MOD_SHIFT)) {
		type = mods & PUGL_MOD_SHIFT ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
		special = PUGL_KEY_SHIFT;
	} else if ((mods & PUGL_MOD_CTRL) != (puglview->impl->mods & PUGL_MOD_CTRL)) {
		type = mods & PUGL_MOD_CTRL ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
		special = PUGL_KEY_CTRL;
	} else if ((mods & PUGL_MOD_ALT) != (puglview->impl->mods & PUGL_MOD_ALT)) {
		type = mods & PUGL_MOD_ALT ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
		special = PUGL_KEY_ALT;
	} else if ((mods & PUGL_MOD_SUPER) != (puglview->impl->mods & PUGL_MOD_SUPER)) {
		type = mods & PUGL_MOD_SUPER ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
		special = PUGL_KEY_SUPER;
	}

	if (special != 0) {
		const NSPoint wloc = [self eventLocation:event];
		const NSPoint rloc = [NSEvent mouseLocation];
		PuglEventKey  ev   = {
			type,
			puglview,
			0,
			[event timestamp],
			wloc.x,
			puglview->height - wloc.y,
			rloc.x,
			[[NSScreen mainScreen] frame].size.height - rloc.y,
			mods,
			[event keyCode],
			0,
			special,
			{ 0, 0, 0, 0, 0, 0, 0, 0 },
			false
		};
		puglDispatchEvent(puglview, (const PuglEvent*)&ev);
	}

	puglview->impl->mods = mods;
}

- (BOOL) preservesContentInLiveResize
{
	return NO;
}

- (void) viewWillStartLiveResize
{
	timer = [NSTimer timerWithTimeInterval:(1 / 60.0)
	                                target:self
	                              selector:@selector(resizeTick)
	                              userInfo:nil
	                               repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer
	                             forMode:NSRunLoopCommonModes];

	[super viewWillStartLiveResize];
}

- (void) resizeTick
{
	puglPostRedisplay(puglview);
}

- (void) urgentTick
{
    [NSApp requestUserAttention:NSInformationalRequest];
}

- (void) viewDidEndLiveResize
{
	[super viewDidEndLiveResize];
	[timer invalidate];
	timer = NULL;
}

@end

@interface PuglWindowDelegate : NSObject<NSWindowDelegate>
{
	PuglWindow* window;
}

- (instancetype) initWithPuglWindow:(PuglWindow*)window;

@end

@implementation PuglWindowDelegate

- (instancetype) initWithPuglWindow:(PuglWindow*)puglWindow
{
	if ((self = [super init])) {
		window = puglWindow;
	}

	return self;
}

- (BOOL) windowShouldClose:(id)sender
{
	const PuglEventClose ev = { PUGL_CLOSE, window->puglview, 0 };
	puglDispatchEvent(window->puglview, (const PuglEvent*)&ev);
	return YES;
}

- (void) windowDidBecomeKey:(NSNotification*)notification
{
	PuglOpenGLView* glview = window->puglview->impl->glview;
	if (window->puglview->impl->glview->urgentTimer) {
		[glview->urgentTimer invalidate];
		glview->urgentTimer = NULL;
	}

	const PuglEventFocus ev = { PUGL_FOCUS_IN, window->puglview, 0, false };
	puglDispatchEvent(window->puglview, (const PuglEvent*)&ev);
}

- (void) windowDidResignKey:(NSNotification*)notification
{
	const PuglEventFocus ev = { PUGL_FOCUS_OUT, window->puglview, 0, false };
	puglDispatchEvent(window->puglview, (const PuglEvent*)&ev);
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

static NSLayoutConstraint*
puglConstraint(id item, NSLayoutAttribute attribute, float constant)
{
	return [NSLayoutConstraint
		       constraintWithItem: item
		                attribute: attribute
		                relatedBy: NSLayoutRelationGreaterThanOrEqual
		                   toItem: nil
		                attribute: NSLayoutAttributeNotAnAttribute
		               multiplier: 1.0
		                 constant: constant];
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;

	[NSAutoreleasePool new];
	impl->app = [NSApplication sharedApplication];

	impl->glview           = [PuglOpenGLView alloc];
	impl->glview->puglview = view;

	[impl->glview initWithFrame:NSMakeRect(0, 0, view->width, view->height)];
	[impl->glview addConstraint:
		     puglConstraint(impl->glview, NSLayoutAttributeWidth, view->min_width)];
	[impl->glview addConstraint:
		     puglConstraint(impl->glview, NSLayoutAttributeHeight, view->min_height)];
	if (!view->hints.resizable) {
		[impl->glview setAutoresizingMask:NSViewNotSizable];
	}

	if (view->parent) {
		NSView* pview = (NSView*)view->parent;
		[pview addSubview:impl->glview];
		[impl->glview setHidden:NO];
		[[impl->glview window] makeFirstResponder:impl->glview];
	} else {
		NSString* titleString = [[NSString alloc]
			                        initWithBytes:title
			                               length:strlen(title)
			                             encoding:NSUTF8StringEncoding];
		NSRect frame = NSMakeRect(0, 0, view->min_width, view->min_height);
		unsigned style = (NSClosableWindowMask |
		                  NSTitledWindowMask |
		                  NSMiniaturizableWindowMask );
		if (view->hints.resizable) {
			style |= NSResizableWindowMask;
		}

		id window = [[[PuglWindow alloc]
			initWithContentRect:frame
			          styleMask:style
			            backing:NSBackingStoreBuffered
			              defer:NO
		              ] retain];
		[window setPuglview:view];
		[window setTitle:titleString];
		if (view->min_width || view->min_height) {
			[window setContentMinSize:NSMakeSize(view->min_width,
			                                     view->min_height)];
		}
		impl->window = window;

		((NSWindow*)window).delegate = [[PuglWindowDelegate alloc]
			                  initWithPuglWindow:window];

		if (view->min_aspect_x && view->min_aspect_y) {
			[window setContentAspectRatio:NSMakeSize(view->min_aspect_x,
			                                         view->min_aspect_y)];
		}

		[window setContentView:impl->glview];
		[impl->app activateIgnoringOtherApps:YES];
		[window makeFirstResponder:impl->glview];
		[window makeKeyAndOrderFront:window];
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
	[view->impl->window makeKeyWindow];
}

void
puglRequestAttention(PuglView* view)
{
	if (![view->impl->window isKeyWindow]) {
		[NSApp requestUserAttention:NSInformationalRequest];
		view->impl->glview->urgentTimer =
			[NSTimer scheduledTimerWithTimeInterval:2.0
			                                 target:view->impl->glview
			                               selector:@selector(urgentTick)
			                               userInfo:nil
			                                repeats:YES];
	}
}

PuglStatus
puglWaitForEvent(PuglView* view)
{
	/* Note that dequeue:NO is broken (it blocks forever even when events are
	   pending), so we work around this by dequeueing the event here and
	   storing it in view->impl->nextEvent for later processing. */
	if (!view->impl->nextEvent) {
		view->impl->nextEvent =
			[view->impl->window nextEventMatchingMask:NSAnyEventMask];
	}

	return PUGL_SUCCESS;
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	if (view->impl->nextEvent) {
		// Process event that was dequeued earier by puglWaitForEvent
		[view->impl->app sendEvent: view->impl->nextEvent];
		view->impl->nextEvent = NULL;
	}

	// Process all pending events
	for (NSEvent* ev = NULL;
	     (ev = [view->impl->window nextEventMatchingMask:NSAnyEventMask
	                                           untilDate:nil
	                                              inMode:NSDefaultRunLoopMode
	                                             dequeue:YES]);) {
		[view->impl->app sendEvent: ev];
	}

	return PUGL_SUCCESS;
}

PuglGlFunc
puglGetProcAddress(const char *name)
{
	CFBundleRef framework =
		CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));

	CFStringRef symbol = CFStringCreateWithCString(
		kCFAllocatorDefault, name, kCFStringEncodingASCII);

	PuglGlFunc func = CFBundleGetFunctionPointerForName(framework, symbol);

	CFRelease(symbol);

	return func;
}

double
puglGetTime(PuglView* view)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((double)ts.tv_sec + ts.tv_nsec / 1000000000.0) - view->start_time;
}

void
puglPostRedisplay(PuglView* view)
{
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
