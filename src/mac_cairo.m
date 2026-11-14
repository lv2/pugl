// Copyright 2019-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "internal.h"
#include "mac.h"
#include "stub.h"

#include <pugl/cairo.h>

#include <cairo-quartz.h>

#import <Cocoa/Cocoa.h>

#include <assert.h>

@interface PuglCairoView : NSView
@end

@implementation PuglCairoView {
@public
  PuglView*        puglview;
  cairo_surface_t* surface;
  cairo_t*         cr;
}

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame:frame];

  return self;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

  [super resizeWithOldSuperviewSize:oldSize];
  [wrapper setReshaped];
}

- (void)drawRect:(NSRect)rect
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
  const NSRect*    rects   = NULL;
  NSInteger        count   = 0;
  [self getRectsBeingDrawn:&rects count:&count];
  if (count) {
    [wrapper dispatchBlank];

    for (NSInteger i = 0; i < count; ++i) {
      [wrapper dispatchExpose:rects[i] type:PUGL_DAMAGE];
    }

    [wrapper dispatchExpose:rect type:PUGL_EXPOSE];
    [wrapper dispatchFlush];
  }
}

@end

static PuglStatus
puglMacCairoCreate(PuglView* view)
{
  PuglInternals* impl     = view->impl;
  PuglCairoView* drawView = [PuglCairoView alloc];

  drawView->puglview = view;
  drawView           = [drawView initWithFrame:[impl->wrapperView bounds]];
  if (view->hints[PUGL_RESIZABLE]) {
    [drawView
      setAutoresizingMask:(NSAutoresizingMaskOptions)(NSViewWidthSizable |
                                                      NSViewHeightSizable)];
  } else {
    [drawView setAutoresizingMask:NSViewNotSizable];
  }

  impl->drawView = drawView;
  return PUGL_SUCCESS;
}

static void
puglMacCairoDestroy(PuglView* view)
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;

  [drawView removeFromSuperview];
  [drawView release];

  view->impl->drawView = nil;
}

static PuglStatus
puglMacCairoEnter(PuglView* view, const bool expose)
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;
  if (!expose) {
    return PUGL_SUCCESS;
  }

  assert(!drawView->surface);
  assert(!drawView->cr);

  const NSScreen* const screen = view->impl->window
                                   ? [view->impl->window screen]
                                 : [view->impl->wrapperView window]
                                   ? [[view->impl->wrapperView window] screen]
                                   : [NSScreen mainScreen];

  const double scale = 1.0 / [screen backingScaleFactor];
  CGContextRef context =
    (CGContextRef)[[NSGraphicsContext currentContext] CGContext];

  const CGSize sizePx = {(CGFloat)view->lastConfigure.width,
                         (CGFloat)view->lastConfigure.height};

  // Convert coordinates to standard Cairo space
  CGContextTranslateCTM(context, 0.0, sizePx.height * scale);
  CGContextScaleCTM(context, scale, -scale);

  drawView->surface = cairo_quartz_surface_create_for_cg_context(
    context, (unsigned)sizePx.width, (unsigned)sizePx.height);

  drawView->cr = cairo_create(drawView->surface);

  return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoDamage(PuglView* view, const PuglDamageEvent* PUGL_UNUSED(damage))
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;

  cairo_reset_clip(drawView->cr);

  return PUGL_SUCCESS;
}

static PuglStatus
puglMacCairoLeave(PuglView* view, const bool expose)
{
  PuglCairoView* const drawView = (PuglCairoView*)view->impl->drawView;
  if (!expose) {
    return PUGL_SUCCESS;
  }

  assert(drawView->surface);
  assert(drawView->cr);

  CGContextRef context = cairo_quartz_surface_get_cg_context(drawView->surface);

  cairo_destroy(drawView->cr);
  cairo_surface_destroy(drawView->surface);
  drawView->cr      = NULL;
  drawView->surface = NULL;

  CGContextFlush(context);

  return PUGL_SUCCESS;
}

static void*
puglMacCairoGetContext(PuglView* view)
{
  return ((PuglCairoView*)view->impl->drawView)->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglStubAttach,
                                      puglStubDetach,
                                      puglMacCairoCreate,
                                      puglMacCairoDestroy,
                                      puglMacCairoEnter,
                                      puglMacCairoDamage,
                                      puglMacCairoLeave,
                                      puglMacCairoGetContext};

  return &backend;
}
