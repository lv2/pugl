// Copyright 2012-2020 David Robillard <d@drobilla.net>
// Copyright 2023 Stefan Westerfeld <stefan@space.twc.de>
// SPDX-License-Identifier: ISC

#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/cairo.h"
#include "pugl/pugl.h"

#include <cairo.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  PuglWorld*      world;
  PuglTestOptions opts;
  double          mouseClickX;
  double          mouseClickY;
  double          value;
  int             quit;
  bool            entered;
  bool            mouseDown;
} PuglTestApp;

typedef struct {
  double x;
  double y;
} ViewScale;

#define WIN_HEIGHT 100.0
#define WIN_WIDTH 512.0

static ViewScale
getScale(const PuglView* const view)
{
  const PuglRect  frame = puglGetFrame(view);
  const ViewScale scale = {(frame.width - (WIN_WIDTH / frame.width)) / WIN_WIDTH,
                           (frame.height - (WIN_HEIGHT / frame.height)) / WIN_HEIGHT};
  return scale;
}

static void
onDisplay(PuglTestApp* app, PuglView* view, const PuglExposeEvent* event)
{
  cairo_t* cr = (cairo_t*)puglGetContext(view);
  char text_str[100];

  cairo_rectangle(cr, event->x, event->y, event->width, event->height);
  cairo_clip_preserve(cr);

  // Draw background
  if (app->entered) {
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  } else {
    cairo_set_source_rgb(cr, 0, 0, 0);
  }
  cairo_fill(cr);

  // Scale to view size
  const ViewScale scale = getScale(view);
  cairo_scale(cr, scale.x, scale.y);

  cairo_rectangle(cr, 10, 10, WIN_WIDTH - 20, 50);
  cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
  cairo_fill (cr);
  cairo_rectangle(cr, 10, 10, 490 * app->value, 50);
  if (app->mouseDown) {
    cairo_set_source_rgb(cr, 0.3, 0.9, 0.3);
  }
  else {
    cairo_set_source_rgb(cr, 0.1, 0.8, 0.1);
  }
  cairo_fill (cr);

  cairo_set_font_size (cr, 20);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_move_to (cr, 10, 90);
  sprintf (text_str, "%.1f%% - use SHIFT for fine adjustment", app->value * 100);
  cairo_show_text (cr, text_str);
}

static void
onClose(PuglView* view)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  app->quit = 1;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  printEvent(event, "Event: ", app->opts.verbose);

  switch (event->type) {
  case PUGL_KEY_PRESS:
    if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
      app->quit = 1;
    }
    break;
  case PUGL_BUTTON_PRESS:
    app->mouseDown = true;
    app->mouseClickX = event->button.x;
    app->mouseClickY = event->button.y;
    puglSetCursor(view, PUGL_CURSOR_NONE);
    break;
  case PUGL_BUTTON_RELEASE:
    app->mouseDown = false;
    puglSetCursor(view, PUGL_CURSOR_ARROW);
    puglSetCursorPos(view, 10 + (WIN_WIDTH - 20) * app->value, 30);
    break;
  case PUGL_MOTION:
    if (app->mouseDown)
      {
        if (event->motion.x != app->mouseClickX || event->motion.y != app->mouseClickY)
          {
            double scale = (WIN_WIDTH - 20);
            puglSetCursorPos (view, app->mouseClickX, app->mouseClickY);
            if (event->motion.state & PUGL_MOD_SHIFT)
              scale *= 10;
            app->value += (event->motion.x - app->mouseClickX) / scale;
            if (app->value < 0)
              app->value = 0;
            if (app->value > 1)
              app->value = 1;
            puglPostRedisplay(view);
          }
      }
    break;
  case PUGL_POINTER_IN:
    app->entered = true;
    puglPostRedisplay(view);
    break;
  case PUGL_POINTER_OUT:
    app->entered = false;
    puglPostRedisplay(view);
    break;
  case PUGL_UPDATE:
    if (app->opts.continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_EXPOSE:
    onDisplay(app, view, &event->expose);
    break;
  case PUGL_CLOSE:
    onClose(view);
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTestApp app;
  memset(&app, 0, sizeof(app));

  app.opts = puglParseTestOptions(&argc, &argv);
  if (app.opts.help) {
    puglPrintTestUsage("pugl_test", "");
    return 1;
  }

  app.world = puglNewWorld(PUGL_PROGRAM, 0);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglPointerLockDemo");

  PuglView* view = puglNewView(app.world);

  puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl Pointer Lock Demo");
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, WIN_WIDTH, WIN_HEIGHT);
  puglSetSizeHint(view, PUGL_MIN_SIZE, WIN_WIDTH / 2, WIN_HEIGHT / 2);
  puglSetSizeHint(view, PUGL_MAX_SIZE, WIN_WIDTH * 4, WIN_HEIGHT * 4);
  puglSetViewHint(view, PUGL_RESIZABLE, app.opts.resizable);
  puglSetHandle(view, &app);
  puglSetBackend(view, puglCairoBackend());
  puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, app.opts.ignoreKeyRepeat);
  puglSetEventFunc(view, onEvent);

  PuglStatus st = puglRealize(view);
  if (st) {
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  app.value = 0.5;
  puglShow(view, PUGL_SHOW_RAISE);

  const double   timeout    = app.opts.continuous ? (1 / 60.0) : -1.0;
  while (!app.quit) {
    puglUpdate(app.world, timeout);
  }

  puglFreeView(view);
  puglFreeWorld(app.world);
  return 0;
}
