// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <puglutil/test_utils.h>

#include <pugl/cairo.h>
#include <pugl/pugl.h>

#include <cairo.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef MIN
#  define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const uint8_t borderWidth = 96U;
static const uint8_t pad         = 8U;

typedef struct {
  PuglWorld* world;
  PuglView*  parent;
  PuglView*  child;
  bool       quit;
  bool       verbose;
} App;

static void
onExpose(PuglView* const view, const PuglExposeEvent* const event)
{
  (void)event;

  App* const      app  = (App*)puglGetHandle(view);
  cairo_t* const  cr   = (cairo_t*)puglGetContext(view);
  const PuglPoint pos  = puglGetPositionHint(view, PUGL_CURRENT_POSITION);
  const PuglArea  size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);

  // Draw background
  cairo_rectangle(cr, 0.0, 0.0, size.width, size.height);
  if (view == app->child) {
    cairo_set_source_rgb(cr, 0.3294118, 0.4313725, 0.0);
  } else {
    cairo_set_source_rgb(cr, 0.5764706, 0.0666666, 0.3607843);
  }
  cairo_fill_preserve(cr);

  // Draw frame border
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, 4.0);
  cairo_stroke(cr);

  // Draw position label
  char                 labelBuf[16] = {0};
  cairo_text_extents_t extents      = {0};
  snprintf(labelBuf, sizeof(labelBuf), "%d,%d", pos.x, pos.y);
  cairo_set_font_size(cr, 24.0);
  cairo_text_extents(cr, labelBuf, &extents);
  cairo_move_to(cr, pad, pad + extents.height);
  cairo_show_text(cr, labelBuf);

  // Draw height label
  snprintf(labelBuf, sizeof(labelBuf), "%u", size.height);
  cairo_text_extents(cr, labelBuf, &extents);
  cairo_move_to(
    cr, size.width - pad - extents.width, (size.height - extents.height) / 2.0);
  cairo_show_text(cr, labelBuf);

  // Draw width label
  snprintf(labelBuf, sizeof(labelBuf), "%u", size.width);
  cairo_text_extents(cr, labelBuf, &extents);
  cairo_move_to(cr, (size.width - extents.width) / 2.0, size.height - pad);
  cairo_show_text(cr, labelBuf);

  if (puglHasFocus(view)) {
    // Draw focus label
    memcpy(labelBuf, "Focused", strlen("Focused") + 1);
    cairo_text_extents(cr, labelBuf, &extents);
    cairo_move_to(cr, size.width - pad - extents.width, pad + extents.height);
    cairo_show_text(cr, labelBuf);
  }
}

static void
swapFocus(App* app)
{
  if (puglHasFocus(app->parent)) {
    puglGrabFocus(app->child);
  } else {
    puglGrabFocus(app->parent);
  }

  puglObscureView(app->parent);
  puglObscureView(app->child);
}

static void
onKeyPress(PuglView* view, const PuglKeyEvent* event)
{
  App* app = (App*)puglGetHandle(view);

  if (event->key == '\t') {
    swapFocus(app);
  } else if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
    app->quit = true;
  } else if (event->state & PUGL_MOD_SHIFT) {
    const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    if (event->key == PUGL_KEY_UP) {
      puglSetSizeHint(view, PUGL_CURRENT_SIZE, size.width, size.height - 10U);
    } else if (event->key == PUGL_KEY_DOWN) {
      puglSetSizeHint(view, PUGL_CURRENT_SIZE, size.width, size.height + 10U);
    } else if (event->key == PUGL_KEY_LEFT) {
      puglSetSizeHint(view, PUGL_CURRENT_SIZE, size.width - 10U, size.height);
    } else if (event->key == PUGL_KEY_RIGHT) {
      puglSetSizeHint(view, PUGL_CURRENT_SIZE, size.width + 10U, size.height);
    }
  } else {
    const PuglPoint pos = puglGetPositionHint(view, PUGL_CURRENT_POSITION);
    if (event->key == PUGL_KEY_UP) {
      puglSetPositionHint(view, PUGL_CURRENT_POSITION, pos.x, pos.y - 10);
    } else if (event->key == PUGL_KEY_DOWN) {
      puglSetPositionHint(view, PUGL_CURRENT_POSITION, pos.x, pos.y + 10);
    } else if (event->key == PUGL_KEY_LEFT) {
      puglSetPositionHint(view, PUGL_CURRENT_POSITION, pos.x - 10, pos.y);
    } else if (event->key == PUGL_KEY_RIGHT) {
      puglSetPositionHint(view, PUGL_CURRENT_POSITION, pos.x + 10, pos.y);
    }
  }
}

static void
configureChild(App* const app, const PuglConfigureEvent* const configure)
{
  PuglView* const child = app->child;
  const PuglArea  size  = puglGetSizeHint(child, PUGL_CURRENT_SIZE);
  const PuglPoint pos   = puglGetPositionHint(child, PUGL_CURRENT_POSITION);

  const PuglArea maxChildSize = {(PuglSpan)(configure->width - pos.x),
                                 (PuglSpan)(configure->height - pos.y)};

  if (size.width > maxChildSize.width || size.height > maxChildSize.height) {
    puglSetSizeHint(child,
                    PUGL_CURRENT_SIZE,
                    MIN(maxChildSize.width, size.width),
                    MIN(maxChildSize.height, size.height));
  }
}

static PuglStatus
onParentEvent(PuglView* view, const PuglEvent* event)
{
  App* const app = (App*)puglGetHandle(view);

  printEvent(event, "Parent: ", app->verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    configureChild(app, &event->configure);
    puglObscureView(view);
    break;
  case PUGL_EXPOSE:
    onExpose(view, &event->expose);
    break;
  case PUGL_FOCUS_IN:
  case PUGL_FOCUS_OUT:
    puglObscureView(view);
    break;
  case PUGL_KEY_PRESS:
    onKeyPress(view, &event->key);
    break;
  case PUGL_MOTION:
    break;
  case PUGL_CLOSE:
    app->quit = true;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onChildEvent(PuglView* view, const PuglEvent* event)
{
  App* app = (App*)puglGetHandle(view);

  printEvent(event, "Child:  ", app->verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    puglObscureView(view);
    break;
  case PUGL_EXPOSE:
    onExpose(view, &event->expose);
    break;
  case PUGL_CLOSE:
    app->quit = true;
    break;
  case PUGL_FOCUS_IN:
  case PUGL_FOCUS_OUT:
    puglObscureView(view);
    break;
  case PUGL_KEY_PRESS:
    onKeyPress(view, &event->key);
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglStatus st  = PUGL_SUCCESS;
  App        app = {0};

  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage("pugl_test", "");
    return 1;
  }

  app.verbose = opts.verbose;
  app.world   = puglNewWorld(PUGL_PROGRAM, 0);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglEmbedDemo");

  app.parent = puglNewView(app.world);
  puglSetBackend(app.parent, puglCairoBackend());
  puglSetViewString(app.parent, PUGL_WINDOW_TITLE, "Pugl Embed Parent");
  puglSetSizeHint(app.parent, PUGL_DEFAULT_SIZE, 640, 480);
  puglSetSizeHint(app.parent, PUGL_MIN_SIZE, 320, 240);
  puglSetSizeHint(app.parent, PUGL_MIN_ASPECT, 1, 1);
  puglSetSizeHint(app.parent, PUGL_MAX_ASPECT, 16, 9);
  puglSetViewHint(app.parent, PUGL_RESIZABLE, opts.resizable);
  puglSetViewHint(app.parent, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(app.parent, &app);
  puglSetEventFunc(app.parent, onParentEvent);

  if ((st = puglRealize(app.parent))) {
    return logError("Failed to realize parent (%s)\n", puglStrerror(st));
  }

  const PuglArea parentSize = puglGetSizeHint(app.parent, PUGL_CURRENT_SIZE);

  app.child = puglNewView(app.world);
  puglSetParent(app.child, puglGetNativeView(app.parent));
  puglSetPositionHint(
    app.child, PUGL_DEFAULT_POSITION, borderWidth, borderWidth);
  puglSetSizeHint(app.child,
                  PUGL_DEFAULT_SIZE,
                  parentSize.width - (2U * borderWidth),
                  parentSize.height - (2U * borderWidth));

  puglSetBackend(app.child, puglCairoBackend());
  puglSetViewHint(app.child, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(app.child, &app);
  puglSetEventFunc(app.child, onChildEvent);

  if ((st = puglRealize(app.child))) {
    return logError("Failed to realize child (%s)\n", puglStrerror(st));
  }

  puglShow(app.parent, PUGL_SHOW_RAISE);
  puglShow(app.child, PUGL_SHOW_RAISE);

  while (!app.quit) {
    puglUpdate(app.world, -1.0);
  }

  puglFreeView(app.child);
  puglFreeView(app.parent);
  puglFreeWorld(app.world);

  return 0;
}
