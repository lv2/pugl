// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <puglutil/demo_utils.h>
#include <puglutil/strings.h>
#include <puglutil/test_utils.h>
#include <traje/traje.h>

#include <pugl/cairo.h>
#include <pugl/pugl.h>

#include <cairo.h>

#include <math.h>
#include <stdbool.h>
#include <string.h>

enum {
  N_BUTTONS = 4U,
};

static const double turn_overlap  = 0.5; // Fraction of turns to overlap
static const double loop_duration = 2.5; // Total duration of animation loop (s)
static const double turn_duration = 1.0; // Duration of a full button turn (s)

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  TrajeTracer*    tracer;
  double          currentMouseX;
  double          currentMouseY;
  double          lastDrawnMouseX;
  double          lastDrawnMouseY;
  double          lastUpdateTime;
  double          lastBlankTime;
  double          lastFlushTime;
  unsigned        framesDrawn;
  int             quit;
  bool            entered;
  bool            mouseDown;
} PuglTestApp;

typedef struct {
  double x;
  double y;
} ViewScale;

typedef struct {
  int         x;
  int         y;
  int         w;
  int         h;
  const char* label;
} Button;

static const Button buttons[N_BUTTONS] = {{128, 128, 64, 64, "1"},
                                          {384, 128, 64, 64, "2"},
                                          {128, 384, 64, 64, "3"},
                                          {384, 384, 64, 64, "4"}};

static ViewScale
getScale(const PuglView* const view)
{
  const PuglArea  size  = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
  const ViewScale scale = {(size.width - (512.0 / size.width)) / 512.0,
                           (size.height - (512.0 / size.height)) / 512.0};
  return scale;
}

static void
resetMouse(PuglTestApp* app)
{
  app->currentMouseX   = -1.0;
  app->currentMouseY   = -1.0;
  app->lastDrawnMouseX = -1.0;
  app->lastDrawnMouseY = -1.0;
}

static void
roundedBox(cairo_t* cr, double x, double y, double w, double h)
{
  static const double radius  = 10;
  static const double degrees = 3.14159265 / 180.0;

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - radius, y + radius, radius, -90 * degrees, 0 * degrees);

  cairo_arc(
    cr, x + w - radius, y + h - radius, radius, 0 * degrees, 90 * degrees);

  cairo_arc(
    cr, x + radius, y + h - radius, radius, 90 * degrees, 180 * degrees);

  cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path(cr);
}

static void
buttonDraw(PuglTestApp*  app,
           cairo_t*      cr,
           const Button* but,
           const double  startTime,
           const double  time)
{
  cairo_save(cr);
  cairo_translate(cr, but->x, but->y);

  if (time >= startTime && time < startTime + turn_duration) {
    cairo_rotate(cr, (time - startTime) * 2.0 * 3.141592);
  }

  cairo_translate(cr, -(but->w / 2.0), -(but->h / 2.0));

  // Draw base
  if (app->mouseDown) {
    cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
  } else {
    cairo_set_source_rgba(cr, 0.3, 0.5, 0.1, 1);
  }
  roundedBox(cr, 0, 0, but->w, but->h);
  cairo_fill_preserve(cr);

  // Draw border
  cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
  cairo_set_line_width(cr, 4.0);
  cairo_stroke(cr);

  // Draw label
  cairo_text_extents_t extents;
  cairo_set_font_size(cr, 32.0);
  cairo_text_extents(cr, but->label, &extents);
  cairo_move_to(cr,
                (but->w / 2.0) - (extents.width / 2),
                (but->h / 2.0) + (extents.height / 2));
  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_show_text(cr, but->label);

  cairo_restore(cr);
}

static void
obscureButton(PuglView* view, const ViewScale scale, const Button* button)
{
  const double span = sqrt((button->w * button->w) + (button->h * button->h));
  puglObscureRegion(view,
                    (int)((button->x - (span / 2.0)) * scale.x),
                    (int)((button->y - (span / 2.0)) * scale.y),
                    (unsigned)ceil(span * scale.x),
                    (unsigned)ceil(span * scale.y));
}

static void
obscureUpdatedButtons(PuglView* view, const double lastTime, const double time)
{
  const ViewScale scale = getScale(view);

  for (unsigned i = 0U; i < N_BUTTONS; ++i) {
    const double buttonStart  = fmod((double)i * turn_overlap, loop_duration);
    const double buttonEnd    = buttonStart + turn_duration;
    const bool   movingBefore = lastTime >= buttonStart && lastTime < buttonEnd;
    const bool   movingNow    = time >= buttonStart && time < buttonEnd;
    if (movingNow || movingBefore) {
      obscureButton(view, scale, &buttons[i]);
    }
  }
}

static void
obscureAllButtons(PuglView* view)
{
  const ViewScale scale = getScale(view);

  for (unsigned i = 0U; i < N_BUTTONS; ++i) {
    obscureButton(view, scale, &buttons[i]);
  }
}

static void
onDamage(PuglTestApp* app, PuglView* view, const PuglDamageEvent* event)
{
  (void)app;
  cairo_t* cr = (cairo_t*)puglGetContext(view);

  // Add damaged rectangle to clip region
  cairo_rectangle(cr, event->x, event->y, event->width, event->height);
}

static void
onExpose(PuglTestApp* app, PuglView* view, const PuglExposeEvent* event)
{
  (void)event;

  const double startTime = puglGetTime(app->world);

  cairo_t* cr = (cairo_t*)puglGetContext(view);

  // Clip to damaged regions
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

  const double drawTime =
    app->opts.continuous ? fmod(startTime, loop_duration) : 0.0;

  // Draw buttons
  for (unsigned i = 0U; i < N_BUTTONS; ++i) {
    buttonDraw(app, cr, &buttons[i], (double)i * turn_overlap, drawTime);
  }

  // Draw mouse cursor
  const double mouseX = app->currentMouseX / scale.x;
  const double mouseY = app->currentMouseY / scale.y;
  cairo_set_line_width(cr, 2.0);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, mouseX - 8.0, mouseY);
  cairo_line_to(cr, mouseX + 8.0, mouseY);
  cairo_move_to(cr, mouseX, mouseY - 8.0);
  cairo_line_to(cr, mouseX, mouseY + 8.0);
  cairo_stroke(cr);

  ++app->framesDrawn;
}

static void
onClose(PuglView* view)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  app->quit = 1;
}

static void
obscureMouseCursor(PuglView* const view,
                   const ViewScale scale,
                   const double    mouseX,
                   const double    mouseY)
{
  if (mouseX >= 0.0 && mouseY >= 0.0) {
    puglObscureRegion(view,
                      (int)floor(mouseX - (10.0 * scale.x)),
                      (int)floor(mouseY - (10.0 * scale.y)),
                      (unsigned)ceil(20.0 * scale.x),
                      (unsigned)ceil(20.0 * scale.y));
  }
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  traje_begin(app->tracer, puglEventTypeName(event->type));
  printEvent(event, "Event: ", app->opts.verbose);

  const ViewScale scale = getScale(view);
  switch (event->type) {
  case PUGL_KEY_PRESS:
    if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
      app->quit = 1;
    }
    break;
  case PUGL_BUTTON_PRESS:
    app->mouseDown = true;
    obscureAllButtons(view);
    break;
  case PUGL_BUTTON_RELEASE:
    app->mouseDown = false;
    obscureAllButtons(view);
    break;
  case PUGL_MOTION:
    // Obscure old cursor position
    obscureMouseCursor(view, scale, app->lastDrawnMouseX, app->lastDrawnMouseY);

    // Obscure new cursor position
    app->currentMouseX = event->motion.x;
    app->currentMouseY = event->motion.y;
    obscureMouseCursor(view, scale, app->currentMouseX, app->currentMouseY);
    break;
  case PUGL_POINTER_IN:
    app->entered = true;
    puglObscureView(view);
    break;
  case PUGL_POINTER_OUT:
    app->entered = false;
    resetMouse(app);
    puglObscureView(view);
    break;
  case PUGL_CONFIGURE:
    puglObscureView(view);
    break;
  case PUGL_UPDATE:
    if (app->opts.continuous) {
      const double now = fmod(puglGetTime(app->world), loop_duration);
      obscureUpdatedButtons(view, app->lastUpdateTime, now);
      app->lastUpdateTime = now;
    }
    break;
  case PUGL_BLANK:
    app->lastBlankTime = puglGetTime(app->world);
    break;
  case PUGL_DAMAGE:
    onDamage(app, view, &event->damage);
    break;
  case PUGL_EXPOSE:
    onExpose(app, view, &event->expose);
    break;
  case PUGL_FLUSH:
    app->lastFlushTime   = puglGetTime(app->world);
    app->lastDrawnMouseX = app->currentMouseX;
    app->lastDrawnMouseY = app->currentMouseY;
    break;
  case PUGL_CLOSE:
    onClose(view);
    break;
  default:
    break;
  }

  traje_end(app->tracer, puglEventTypeName(event->type));
  return PUGL_SUCCESS;
}

static double
updateTimeout(const PuglTestApp* const app)
{
  if (!app->opts.sync) {
    return 0.0; // VSync explicitly disabled, run as fast as possible
  }

  // Wait until we should probably start rendering the next frame
  const int    rate         = puglGetViewHint(app->view, PUGL_REFRESH_RATE);
  const double now          = puglGetTime(app->world);
  const double drawDuration = app->lastFlushTime - app->lastBlankTime;
  return (app->lastFlushTime + (1.0 / rate) - drawDuration) - now;
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

  app.tracer =
    traje_tracer_new("pugl_cairo_demo", "main", "pugl_cairo_demo.json", 128);

  app.world = puglNewWorld(PUGL_PROGRAM, 0);
  resetMouse(&app);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglCairoDemo");

  app.view = puglNewView(app.world);

  puglSetViewString(app.view, PUGL_WINDOW_TITLE, "Pugl Cairo Demo");
  puglSetSizeHint(app.view, PUGL_DEFAULT_SIZE, 512, 512);
  puglSetSizeHint(app.view, PUGL_MIN_SIZE, 256, 256);
  puglSetSizeHint(app.view, PUGL_MAX_SIZE, 2048, 2048);
  puglSetViewHint(app.view, PUGL_RESIZABLE, app.opts.resizable);
  puglSetHandle(app.view, &app);
  puglSetBackend(app.view, puglCairoBackend());
  puglSetViewHint(app.view, PUGL_IGNORE_KEY_REPEAT, app.opts.ignoreKeyRepeat);
  puglSetEventFunc(app.view, onEvent);

  PuglStatus st = puglRealize(app.view);
  if (st) {
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  puglShow(app.view, PUGL_SHOW_RAISE);

  PuglFpsPrinter fpsPrinter = {puglGetTime(app.world), 0U};
  while (!app.quit) {
    traje_begin(app.tracer, "loop");

    const double timeout = updateTimeout(&app);
    puglUpdate(app.world, timeout);

    if (app.opts.continuous) {
      puglPrintFps(puglGetTime(app.world), &fpsPrinter, &app.framesDrawn);
    }

    traje_end(app.tracer, "loop");
    traje_flush(app.tracer);
  }

  puglFreeView(app.view);
  puglFreeWorld(app.world);
  traje_tracer_free(app.tracer);
  return 0;
}
