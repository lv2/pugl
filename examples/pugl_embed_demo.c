/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

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

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const int       borderWidth    = 64;
static const uintptr_t reverseTimerId = 1u;

typedef struct {
  PuglWorld* world;
  PuglView*  parent;
  PuglView*  child;
  double     xAngle;
  double     yAngle;
  double     lastMouseX;
  double     lastMouseY;
  double     lastDrawTime;
  float      dist;
  int        quit;
  bool       continuous;
  bool       mouseEntered;
  bool       verbose;
  bool       reversing;
} PuglTestApp;

// clang-format off
static const float backgroundVertices[] = {
  -1.0f,  1.0f,  -1.0f, // Top left
   1.0f,  1.0f,  -1.0f, // Top right
  -1.0f, -1.0f,  -1.0f, // Bottom left
   1.0f, -1.0f,  -1.0f, // Bottom right
};
// clang-format on

static PuglRect
getChildFrame(const PuglRect parentFrame)
{
  const PuglRect childFrame = {borderWidth,
                               borderWidth,
                               parentFrame.width - 2 * borderWidth,
                               parentFrame.height - 2 * borderWidth};

  return childFrame;
}

static void
onDisplay(PuglView* view)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  const double thisTime = puglGetTime(app->world);
  if (app->continuous) {
    const double dTime =
      (thisTime - app->lastDrawTime) * (app->reversing ? -1.0 : 1.0);

    app->xAngle = fmod(app->xAngle + dTime * 100.0, 360.0);
    app->yAngle = fmod(app->yAngle + dTime * 100.0, 360.0);
  }

  displayCube(
    view, app->dist, (float)app->xAngle, (float)app->yAngle, app->mouseEntered);

  app->lastDrawTime = thisTime;
}

static void
swapFocus(PuglTestApp* app)
{
  if (puglHasFocus(app->parent)) {
    puglGrabFocus(app->child);
  } else {
    puglGrabFocus(app->parent);
  }

  if (!app->continuous) {
    puglPostRedisplay(app->parent);
    puglPostRedisplay(app->child);
  }
}

static void
onKeyPress(PuglView* view, const PuglEventKey* event, const char* prefix)
{
  PuglTestApp* app   = (PuglTestApp*)puglGetHandle(view);
  PuglRect     frame = puglGetFrame(view);

  if (event->key == '\t') {
    swapFocus(app);
  } else if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
    app->quit = 1;
  } else if (event->state & PUGL_MOD_CTRL && event->key == 'c') {
    puglSetClipboard(view, NULL, "Pugl test", strlen("Pugl test") + 1);
    fprintf(stderr, "%sCopy \"Pugl test\"\n", prefix);
  } else if (event->state & PUGL_MOD_CTRL && event->key == 'v') {
    const char* type = NULL;
    size_t      len  = 0;
    const char* text = (const char*)puglGetClipboard(view, &type, &len);
    fprintf(stderr, "%sPaste \"%s\"\n", prefix, text);
  } else if (event->state & PUGL_MOD_SHIFT) {
    if (event->key == PUGL_KEY_UP) {
      frame.height += 10;
    } else if (event->key == PUGL_KEY_DOWN) {
      frame.height -= 10;
    } else if (event->key == PUGL_KEY_LEFT) {
      frame.width -= 10;
    } else if (event->key == PUGL_KEY_RIGHT) {
      frame.width += 10;
    } else {
      return;
    }
    puglSetFrame(view, frame);
  } else {
    if (event->key == PUGL_KEY_UP) {
      frame.y -= 10;
    } else if (event->key == PUGL_KEY_DOWN) {
      frame.y += 10;
    } else if (event->key == PUGL_KEY_LEFT) {
      frame.x -= 10;
    } else if (event->key == PUGL_KEY_RIGHT) {
      frame.x += 10;
    } else {
      return;
    }
    puglSetFrame(view, frame);
  }
}

static PuglStatus
onParentEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp*   app         = (PuglTestApp*)puglGetHandle(view);
  const PuglRect parentFrame = puglGetFrame(view);

  printEvent(event, "Parent: ", app->verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    reshapeCube((float)event->configure.width, (float)event->configure.height);

    puglSetFrame(app->child, getChildFrame(parentFrame));
    break;
  case PUGL_UPDATE:
    if (app->continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_EXPOSE:
    if (puglHasFocus(app->parent)) {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_COLOR_ARRAY);
      glVertexPointer(3, GL_FLOAT, 0, backgroundVertices);
      glColorPointer(3, GL_FLOAT, 0, backgroundVertices);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glDisableClientState(GL_COLOR_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
    } else {
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    break;
  case PUGL_KEY_PRESS:
    onKeyPress(view, &event->key, "Parent: ");
    break;
  case PUGL_MOTION:
    break;
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  printEvent(event, "Child:  ", app->verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    reshapeCube((float)event->configure.width, (float)event->configure.height);
    break;
  case PUGL_UPDATE:
    if (app->continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_EXPOSE:
    onDisplay(view);
    break;
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  case PUGL_KEY_PRESS:
    onKeyPress(view, &event->key, "Child:  ");
    break;
  case PUGL_MOTION:
    app->xAngle -= event->motion.x - app->lastMouseX;
    app->yAngle += event->motion.y - app->lastMouseY;
    app->lastMouseX = event->motion.x;
    app->lastMouseY = event->motion.y;
    if (!app->continuous) {
      puglPostRedisplay(view);
      puglPostRedisplay(app->parent);
    }
    break;
  case PUGL_SCROLL:
    app->dist = fmaxf(10.0f, app->dist + (float)event->scroll.dy);
    if (!app->continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_POINTER_IN:
    app->mouseEntered = true;
    break;
  case PUGL_POINTER_OUT:
    app->mouseEntered = false;
    break;
  case PUGL_TIMER:
    app->reversing = !app->reversing;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTestApp app = {0};

  app.dist = 10;

  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage("pugl_test", "");
    return 1;
  }

  app.continuous = opts.continuous;
  app.verbose    = opts.verbose;

  app.world  = puglNewWorld(PUGL_PROGRAM, 0);
  app.parent = puglNewView(app.world);
  app.child  = puglNewView(app.world);

  puglSetClassName(app.world, "Pugl Test");

  const PuglRect parentFrame = {0, 0, 512, 512};
  puglSetDefaultSize(app.parent, 512, 512);
  puglSetMinSize(app.parent, borderWidth * 3, borderWidth * 3);
  puglSetMaxSize(app.parent, 1024, 1024);
  puglSetAspectRatio(app.parent, 1, 1, 16, 9);
  puglSetBackend(app.parent, puglGlBackend());

  puglSetViewHint(app.parent, PUGL_USE_DEBUG_CONTEXT, opts.errorChecking);
  puglSetViewHint(app.parent, PUGL_RESIZABLE, opts.resizable);
  puglSetViewHint(app.parent, PUGL_SAMPLES, opts.samples);
  puglSetViewHint(app.parent, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
  puglSetViewHint(app.parent, PUGL_SWAP_INTERVAL, opts.sync);
  puglSetViewHint(app.parent, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(app.parent, &app);
  puglSetEventFunc(app.parent, onParentEvent);

  PuglStatus    st      = PUGL_SUCCESS;
  const uint8_t title[] = {
    'P', 'u', 'g', 'l', ' ', 'P', 'r', 0xC3, 0xBC, 'f', 'u', 'n', 'g', 0};

  puglSetWindowTitle(app.parent, (const char*)title);

  if ((st = puglRealize(app.parent))) {
    return logError("Failed to create parent window (%s)\n", puglStrerror(st));
  }

  puglSetFrame(app.child, getChildFrame(parentFrame));
  puglSetParentWindow(app.child, puglGetNativeWindow(app.parent));

  puglSetViewHint(app.child, PUGL_USE_DEBUG_CONTEXT, opts.errorChecking);
  puglSetViewHint(app.child, PUGL_SAMPLES, opts.samples);
  puglSetViewHint(app.child, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
  puglSetViewHint(app.child, PUGL_SWAP_INTERVAL, opts.sync);
  puglSetBackend(app.child, puglGlBackend());
  puglSetViewHint(app.child, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(app.child, &app);
  puglSetEventFunc(app.child, onEvent);

  if ((st = puglRealize(app.child))) {
    return logError("Failed to create child window (%s)\n", puglStrerror(st));
  }

  puglShow(app.parent);
  puglShow(app.child);

  puglStartTimer(app.child, reverseTimerId, 3.6);

  PuglFpsPrinter fpsPrinter         = {puglGetTime(app.world)};
  unsigned       framesDrawn        = 0;
  bool           requestedAttention = false;
  while (!app.quit) {
    const double thisTime = puglGetTime(app.world);

    puglUpdate(app.world, app.continuous ? 0.0 : -1.0);
    ++framesDrawn;

    if (!requestedAttention && thisTime > 5.0) {
      puglRequestAttention(app.parent);
      requestedAttention = true;
    }

    if (app.continuous) {
      puglPrintFps(app.world, &fpsPrinter, &framesDrawn);
    }
  }

  puglFreeView(app.child);
  puglFreeView(app.parent);
  puglFreeWorld(app.world);

  return 0;
}
