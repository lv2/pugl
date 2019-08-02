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
   @file mac.m MacOS implementation.
*/

#define GL_SILENCE_DEPRECATION 1

#include "pugl/detail/implementation.h"
#include "pugl/detail/mac.h"
#include "pugl/pugl.h"

#import <Cocoa/Cocoa.h>

#include <mach/mach_time.h>

#include <stdlib.h>

@implementation PuglWindow

- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(NSWindowStyleMask)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag
{
	(void)flag;

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

@implementation PuglWrapperView

- (void) resizeWithOldSuperviewSize:(NSSize)oldSize
{
	[super resizeWithOldSuperviewSize:oldSize];

	const NSRect bounds = [self bounds];
	puglview->backend->resize(puglview, bounds.size.width, bounds.size.height);
}

- (void) drawRect:(NSRect)rect
{
	const PuglEventExpose ev =  {
		PUGL_EXPOSE,
		0,
		rect.origin.x,
		rect.origin.y,
		rect.size.width,
		rect.size.height,
		0
	};

	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
}

- (BOOL) isFlipped
{
	return YES;
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

- (void) dispatchConfigure:(NSRect)bounds
{
	const PuglEventConfigure ev =  {
		PUGL_CONFIGURE,
		0,
		bounds.origin.x,
		bounds.origin.y,
		bounds.size.width,
		bounds.size.height,
	};

	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
}

static uint32_t
getModifiers(const NSEvent* const ev)
{
	const NSEventModifierFlags modifierFlags = [ev modifierFlags];

	return (((modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0) |
	        ((modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0) |
	        ((modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0) |
	        ((modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0));
}

static PuglKey
keySymToSpecial(const NSEvent* const ev)
{
	NSString* chars = [ev charactersIgnoringModifiers];
	if ([chars length] == 1) {
		switch ([chars characterAtIndex:0]) {
		case NSF1FunctionKey:          return PUGL_KEY_F1;
		case NSF2FunctionKey:          return PUGL_KEY_F2;
		case NSF3FunctionKey:          return PUGL_KEY_F3;
		case NSF4FunctionKey:          return PUGL_KEY_F4;
		case NSF5FunctionKey:          return PUGL_KEY_F5;
		case NSF6FunctionKey:          return PUGL_KEY_F6;
		case NSF7FunctionKey:          return PUGL_KEY_F7;
		case NSF8FunctionKey:          return PUGL_KEY_F8;
		case NSF9FunctionKey:          return PUGL_KEY_F9;
		case NSF10FunctionKey:         return PUGL_KEY_F10;
		case NSF11FunctionKey:         return PUGL_KEY_F11;
		case NSF12FunctionKey:         return PUGL_KEY_F12;
		case NSDeleteCharacter:        return PUGL_KEY_BACKSPACE;
		case NSDeleteFunctionKey:      return PUGL_KEY_DELETE;
		case NSLeftArrowFunctionKey:   return PUGL_KEY_LEFT;
		case NSUpArrowFunctionKey:     return PUGL_KEY_UP;
		case NSRightArrowFunctionKey:  return PUGL_KEY_RIGHT;
		case NSDownArrowFunctionKey:   return PUGL_KEY_DOWN;
		case NSPageUpFunctionKey:      return PUGL_KEY_PAGE_UP;
		case NSPageDownFunctionKey:    return PUGL_KEY_PAGE_DOWN;
		case NSHomeFunctionKey:        return PUGL_KEY_HOME;
		case NSEndFunctionKey:         return PUGL_KEY_END;
		case NSInsertFunctionKey:      return PUGL_KEY_INSERT;
		case NSMenuFunctionKey:        return PUGL_KEY_MENU;
		case NSScrollLockFunctionKey:  return PUGL_KEY_SCROLL_LOCK;
		case NSClearLineFunctionKey:   return PUGL_KEY_NUM_LOCK;
		case NSPrintScreenFunctionKey: return PUGL_KEY_PRINT_SCREEN;
		case NSPauseFunctionKey:       return PUGL_KEY_PAUSE;
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
handleCrossing(PuglWrapperView* view, NSEvent* event, const PuglEventType type)
{
	const NSPoint           wloc = [view eventLocation:event];
	const NSPoint           rloc = [NSEvent mouseLocation];
	const PuglEventCrossing ev   =  {
		type,
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
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
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
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
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
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
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
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
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
		[event scrollingDeltaX],
		[event scrollingDeltaY]
	};
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
}

- (void) keyDown:(NSEvent*)event
{
	if (puglview->hints.ignoreKeyRepeat && [event isARepeat]) {
		return;
	}

	const NSPoint      wloc  = [self eventLocation:event];
	const NSPoint      rloc  = [NSEvent mouseLocation];
	const PuglKey      spec  = keySymToSpecial(event);
	const NSString*    chars = [event charactersIgnoringModifiers];
	const char*        str   = [[chars lowercaseString] UTF8String];
	const uint32_t     code  = (
		spec ? spec : puglDecodeUTF8((const uint8_t*)str));

	const PuglEventKey ev =  {
		PUGL_KEY_PRESS,
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
		[event keyCode],
		(code != 0xFFFD) ? code : 0
	};

	puglDispatchEvent(puglview, (const PuglEvent*)&ev);

	if (!spec) {
		[self interpretKeyEvents:@[event]];
	}
}

- (void) keyUp:(NSEvent*)event
{
	const NSPoint      wloc  = [self eventLocation:event];
	const NSPoint      rloc  = [NSEvent mouseLocation];
	const PuglKey      spec  = keySymToSpecial(event);
	const NSString*    chars = [event charactersIgnoringModifiers];
	const char*        str   = [[chars lowercaseString] UTF8String];
	const uint32_t     code  =
		(spec ? spec : puglDecodeUTF8((const uint8_t*)str));

	const PuglEventKey ev = {
		PUGL_KEY_RELEASE,
		0,
		[event timestamp],
		wloc.x,
		wloc.y,
		rloc.x,
		[[NSScreen mainScreen] frame].size.height - rloc.y,
		getModifiers(event),
		[event keyCode],
		(code != 0xFFFD) ? code : 0
	};
	puglDispatchEvent(puglview, (const PuglEvent*)&ev);
}

- (BOOL) hasMarkedText
{
	return [markedText length] > 0;
}

- (NSRange) markedRange
{
	return (([markedText length] > 0)
	        ? NSMakeRange(0, [markedText length] - 1)
	        : NSMakeRange(NSNotFound, 0));
}

- (NSRange) selectedRange
{
	return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selected
     replacementRange:(NSRange)replacement
{
	(void)selected;
	(void)replacement;
	[markedText release];
	markedText = (
		[string isKindOfClass:[NSAttributedString class]]
		? [[NSMutableAttributedString alloc] initWithAttributedString:string]
		: [[NSMutableAttributedString alloc] initWithString:string]);
}

- (void) unmarkText
{
	[[markedText mutableString] setString:@""];
}

- (NSArray*) validAttributesForMarkedText
{
	return @[];
}

- (NSAttributedString*)
 attributedSubstringForProposedRange:(NSRange)range
                         actualRange:(NSRangePointer)actual
{
	(void)range;
	(void)actual;
	return nil;
}

- (NSUInteger) characterIndexForPoint:(NSPoint)point
{
	(void)point;
	return 0;
}

- (NSRect) firstRectForCharacterRange:(NSRange)range
                          actualRange:(NSRangePointer)actual
{
	(void)range;
	(void)actual;

	const NSRect frame = [(id)puglview bounds];
	return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (void) insertText:(id)string
   replacementRange:(NSRange)replacement
{
	(void)replacement;

	NSEvent* const  event      = [NSApp currentEvent];
	NSString* const characters =
		([string isKindOfClass:[NSAttributedString class]]
		 ? [string string]
		 : (NSString*)string);

	const NSPoint wloc = [self eventLocation:event];
	const NSPoint rloc = [NSEvent mouseLocation];
	for (size_t i = 0; i < [characters length]; ++i) {
		const uint32_t code    = [characters characterAtIndex:i];
		char           utf8[8] = {0};
		NSUInteger     len     = 0;

		[characters getBytes:utf8
		           maxLength:sizeof(utf8)
		          usedLength:&len
		            encoding:NSUTF8StringEncoding
		             options:0
		               range:NSMakeRange(i, i + 1)
		      remainingRange:nil];

		PuglEventText ev = { PUGL_TEXT,
		                     0,
		                     [event timestamp],
		                     wloc.x,
		                     wloc.y,
		                     rloc.x,
		                     [[NSScreen mainScreen] frame].size.height - rloc.y,
		                     getModifiers(event),
		                     [event keyCode],
		                     code,
		                     { 0, 0, 0, 0, 0, 0, 0, 0 } };

		memcpy(ev.string, utf8, len);
		puglDispatchEvent(puglview, (const PuglEvent*)&ev);
	}
}

- (void) flagsChanged:(NSEvent*)event
{
	const uint32_t mods    = getModifiers(event);
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
			0,
			[event timestamp],
			wloc.x,
			wloc.y,
			rloc.x,
			[[NSScreen mainScreen] frame].size.height - rloc.y,
			mods,
			[event keyCode],
			special
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
	(void)sender;

	PuglEvent ev = { 0 };
	ev.type = PUGL_CLOSE;
	puglDispatchEvent(window->puglview, &ev);
	return YES;
}

- (void) windowDidBecomeKey:(NSNotification*)notification
{
	(void)notification;

	PuglWrapperView* wrapperView = window->puglview->impl->wrapperView;
	if (wrapperView->urgentTimer) {
		[wrapperView->urgentTimer invalidate];
		wrapperView->urgentTimer = NULL;
	}

	PuglEvent ev = { 0 };
	ev.type       = PUGL_FOCUS_IN;
	ev.focus.grab = false;
	puglDispatchEvent(window->puglview, &ev);
}

- (void) windowDidResignKey:(NSNotification*)notification
{
	(void)notification;

	PuglEvent ev = { 0 };
	ev.type       = PUGL_FOCUS_OUT;
	ev.focus.grab = false;
	puglDispatchEvent(window->puglview, &ev);
}

@end

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
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

	// Create wrapper view to handle input
	impl->wrapperView             = [PuglWrapperView alloc];
	impl->wrapperView->puglview   = view;
	impl->wrapperView->markedText = [[NSMutableAttributedString alloc] init];
	[impl->wrapperView setAutoresizesSubviews:YES];
	[impl->wrapperView initWithFrame:NSMakeRect(0, 0, view->width, view->height)];
	[impl->wrapperView addConstraint:
		     puglConstraint(impl->wrapperView, NSLayoutAttributeWidth, view->min_width)];
	[impl->wrapperView addConstraint:
		     puglConstraint(impl->wrapperView, NSLayoutAttributeHeight, view->min_height)];

	// Create draw view to be rendered to
	int st = 0;
	if ((st = view->backend->configure(view)) ||
	    (st = view->backend->create(view))) {
		return st;
	}

	// Add draw view to wraper view
	[impl->wrapperView addSubview:impl->drawView];
	[impl->wrapperView setHidden:NO];
	[impl->drawView setHidden:NO];

	if (view->parent) {
		NSView* pview = (NSView*)view->parent;
		[pview addSubview:impl->wrapperView];
		[impl->drawView setHidden:NO];
		[[impl->drawView window] makeFirstResponder:impl->wrapperView];
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

		[window setContentView:impl->wrapperView];
		[impl->app activateIgnoringOtherApps:YES];
		[window makeFirstResponder:impl->wrapperView];
		[window makeKeyAndOrderFront:window];
	}

	[impl->wrapperView updateTrackingAreas];

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
	view->backend->destroy(view);
	[view->impl->wrapperView removeFromSuperview];
	view->impl->wrapperView->puglview = NULL;
	if (view->impl->window) {
		[view->impl->window close];
	}
	[view->impl->wrapperView release];
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
		view->impl->wrapperView->urgentTimer =
			[NSTimer scheduledTimerWithTimeInterval:2.0
			                                 target:view->impl->wrapperView
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

	PuglGlFunc func = (PuglGlFunc)CFBundleGetFunctionPointerForName(
		framework, symbol);

	CFRelease(symbol);

	return func;
}

double
puglGetTime(PuglView* view)
{
	return (mach_absolute_time() / 1e9) - view->start_time;
}

void
puglPostRedisplay(PuglView* view)
{
	[view->impl->wrapperView setNeedsDisplay: YES];
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->wrapperView;
}
