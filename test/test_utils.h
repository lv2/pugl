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

#ifndef TEST_TEST_UTILS_H
#define TEST_TEST_UTILS_H

#define __STDC_FORMAT_MACROS 1

#include "pugl/pugl.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#  define PUGL_LOG_FUNC(fmt, arg1) __attribute__((format(printf, fmt, arg1)))
#else
#  define PUGL_LOG_FUNC(fmt, arg1)
#endif

typedef struct {
  int  samples;
  int  doubleBuffer;
  int  sync;
  bool continuous;
  bool help;
  bool ignoreKeyRepeat;
  bool resizable;
  bool verbose;
  bool errorChecking;
} PuglTestOptions;

PUGL_LOG_FUNC(1, 2)
static int
logError(const char* fmt, ...)
{
  fprintf(stderr, "error: ");

  va_list args; // NOLINT
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  return 1;
}

static inline int
printModifiers(const uint32_t mods)
{
  return fprintf(stderr,
                 "Modifiers:%s%s%s%s\n",
                 (mods & PUGL_MOD_SHIFT) ? " Shift" : "",
                 (mods & PUGL_MOD_CTRL) ? " Ctrl" : "",
                 (mods & PUGL_MOD_ALT) ? " Alt" : "",
                 (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static inline const char*
crossingModeString(const PuglCrossingMode mode)
{
  switch (mode) {
  case PUGL_CROSSING_NORMAL:
    return "normal";
  case PUGL_CROSSING_GRAB:
    return "grab";
  case PUGL_CROSSING_UNGRAB:
    return "ungrab";
  }

  return "unknown";
}

static inline const char*
scrollDirectionString(const PuglScrollDirection direction)
{
  switch (direction) {
  case PUGL_SCROLL_UP:
    return "up";
  case PUGL_SCROLL_DOWN:
    return "down";
  case PUGL_SCROLL_LEFT:
    return "left";
  case PUGL_SCROLL_RIGHT:
    return "right";
  case PUGL_SCROLL_SMOOTH:
    return "smooth";
  }

  return "unknown";
}

static inline int
printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
{
#define FFMT "%6.1f"
#define PFMT FFMT " " FFMT
#define PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

  switch (event->type) {
  case PUGL_NOTHING:
    return 0;
  case PUGL_KEY_PRESS:
    return PRINT("%sKey press   code %3u key  U+%04X\n",
                 prefix,
                 event->key.keycode,
                 event->key.key);
  case PUGL_KEY_RELEASE:
    return PRINT("%sKey release code %3u key  U+%04X\n",
                 prefix,
                 event->key.keycode,
                 event->key.key);
  case PUGL_TEXT:
    return PRINT("%sText entry  code %3u char U+%04X (%s)\n",
                 prefix,
                 event->text.keycode,
                 event->text.character,
                 event->text.string);
  case PUGL_BUTTON_PRESS:
  case PUGL_BUTTON_RELEASE:
    return (PRINT("%sMouse %u %s at " PFMT " ",
                  prefix,
                  event->button.button,
                  (event->type == PUGL_BUTTON_PRESS) ? "down" : "up  ",
                  event->button.x,
                  event->button.y) +
            printModifiers(event->scroll.state));
  case PUGL_SCROLL:
    return (PRINT("%sScroll %5.1f %5.1f (%s) at " PFMT " ",
                  prefix,
                  event->scroll.dx,
                  event->scroll.dy,
                  scrollDirectionString(event->scroll.direction),
                  event->scroll.x,
                  event->scroll.y) +
            printModifiers(event->scroll.state));
  case PUGL_POINTER_IN:
    return PRINT("%sMouse enter  at " PFMT " (%s)\n",
                 prefix,
                 event->crossing.x,
                 event->crossing.y,
                 crossingModeString(event->crossing.mode));
  case PUGL_POINTER_OUT:
    return PRINT("%sMouse leave  at " PFMT " (%s)\n",
                 prefix,
                 event->crossing.x,
                 event->crossing.y,
                 crossingModeString(event->crossing.mode));
  case PUGL_FOCUS_IN:
    return PRINT(
      "%sFocus in (%s)\n", prefix, crossingModeString(event->crossing.mode));
  case PUGL_FOCUS_OUT:
    return PRINT(
      "%sFocus out (%s)\n", prefix, crossingModeString(event->crossing.mode));
  case PUGL_CLIENT:
    return PRINT("%sClient %" PRIXPTR " %" PRIXPTR "\n",
                 prefix,
                 event->client.data1,
                 event->client.data2);
  case PUGL_LOOP_ENTER:
    return PRINT("%sLoop enter\n", prefix);
  case PUGL_LOOP_LEAVE:
    return PRINT("%sLoop leave\n", prefix);
  default:
    break;
  }

  if (verbose) {
    switch (event->type) {
    case PUGL_CREATE:
      return fprintf(stderr, "%sCreate\n", prefix);
    case PUGL_DESTROY:
      return fprintf(stderr, "%sDestroy\n", prefix);
    case PUGL_MAP:
      return fprintf(stderr, "%sMap\n", prefix);
    case PUGL_UNMAP:
      return fprintf(stderr, "%sUnmap\n", prefix);
    case PUGL_UPDATE:
      return fprintf(stderr, "%sUpdate\n", prefix);
    case PUGL_CONFIGURE:
      return PRINT("%sConfigure " PFMT " " PFMT "\n",
                   prefix,
                   event->configure.x,
                   event->configure.y,
                   event->configure.width,
                   event->configure.height);
    case PUGL_EXPOSE:
      return PRINT("%sExpose    " PFMT " " PFMT "\n",
                   prefix,
                   event->expose.x,
                   event->expose.y,
                   event->expose.width,
                   event->expose.height);
    case PUGL_CLOSE:
      return PRINT("%sClose\n", prefix);
    case PUGL_MOTION:
      return PRINT("%sMouse motion at " PFMT "\n",
                   prefix,
                   event->motion.x,
                   event->motion.y);
    case PUGL_TIMER:
      return PRINT("%sTimer %" PRIuPTR "\n", prefix, event->timer.id);
    default:
      return PRINT("%sUnknown event type %d\n", prefix, (int)event->type);
    }
  }

#undef PRINT
#undef PFMT
#undef FFMT

  return 0;
}

static inline const char*
puglViewHintString(const PuglViewHint hint)
{
  switch (hint) {
  case PUGL_USE_COMPAT_PROFILE:
    return "Use compatible profile";
  case PUGL_USE_DEBUG_CONTEXT:
    return "Use debug context";
  case PUGL_CONTEXT_VERSION_MAJOR:
    return "Context major version";
  case PUGL_CONTEXT_VERSION_MINOR:
    return "Context minor version";
  case PUGL_RED_BITS:
    return "Red bits";
  case PUGL_GREEN_BITS:
    return "Green bits";
  case PUGL_BLUE_BITS:
    return "Blue bits";
  case PUGL_ALPHA_BITS:
    return "Alpha bits";
  case PUGL_DEPTH_BITS:
    return "Depth bits";
  case PUGL_STENCIL_BITS:
    return "Stencil bits";
  case PUGL_SAMPLES:
    return "Samples";
  case PUGL_DOUBLE_BUFFER:
    return "Double buffer";
  case PUGL_SWAP_INTERVAL:
    return "Swap interval";
  case PUGL_RESIZABLE:
    return "Resizable";
  case PUGL_IGNORE_KEY_REPEAT:
    return "Ignore key repeat";
  case PUGL_REFRESH_RATE:
    return "Refresh rate";
  case PUGL_NUM_VIEW_HINTS:
    return "Unknown";
  }

  return "Unknown";
}

static inline void
printViewHints(const PuglView* view)
{
  for (int i = 0; i < PUGL_NUM_VIEW_HINTS; ++i) {
    const PuglViewHint hint = (PuglViewHint)i;
    fprintf(stderr,
            "%s: %d\n",
            puglViewHintString(hint),
            puglGetViewHint(view, hint));
  }
}

static inline void
puglPrintTestUsage(const char* prog, const char* posHelp)
{
  printf("Usage: %s [OPTION]... %s\n\n"
         "  -a  Enable anti-aliasing\n"
         "  -c  Continuously animate and draw\n"
         "  -d  Directly draw to window (no double-buffering)\n"
         "  -e  Enable platform error-checking\n"
         "  -f  Fast drawing, explicitly disable vertical sync\n"
         "  -h  Display this help\n"
         "  -i  Ignore key repeat\n"
         "  -v  Print verbose output\n"
         "  -r  Resizable window\n"
         "  -s  Explicitly enable vertical sync\n",
         prog,
         posHelp);
}

static inline PuglTestOptions
puglParseTestOptions(int* pargc, char*** pargv)
{
  PuglTestOptions opts = {
    0,
    PUGL_TRUE,
    PUGL_DONT_CARE,
    false,
    false,
    false,
    false,
    false,
    false,
  };

  char** const argv = *pargv;
  int          i    = 1;
  for (; i < *pargc; ++i) {
    if (!strcmp(argv[i], "-a")) {
      opts.samples = 4;
    } else if (!strcmp(argv[i], "-c")) {
      opts.continuous = true;
    } else if (!strcmp(argv[i], "-d")) {
      opts.doubleBuffer = PUGL_FALSE;
    } else if (!strcmp(argv[i], "-e")) {
      opts.errorChecking = PUGL_TRUE;
    } else if (!strcmp(argv[i], "-f")) {
      opts.sync = PUGL_FALSE;
    } else if (!strcmp(argv[i], "-h")) {
      opts.help = true;
      return opts;
    } else if (!strcmp(argv[i], "-i")) {
      opts.ignoreKeyRepeat = true;
    } else if (!strcmp(argv[i], "-r")) {
      opts.resizable = true;
    } else if (!strcmp(argv[i], "-s")) {
      opts.sync = PUGL_TRUE;
    } else if (!strcmp(argv[i], "-v")) {
      opts.verbose = true;
    } else if (argv[i][0] != '-') {
      break;
    } else {
      opts.help = true;
      logError("Unknown option: %s\n", argv[i]);
    }
  }

  *pargc -= i;
  *pargv += i;

  return opts;
}

#endif // TEST_TEST_UTILS_H
