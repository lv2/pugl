// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGLUTIL_TEST_UTILS_H
#define PUGLUTIL_TEST_UTILS_H

#include <pugl/pugl.h>
#include <puglutil/strings.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
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
  int  glApi;
  int  glMajorVersion;
  int  glMinorVersion;
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
                 "Modifiers:%s%s%s%s%s%s%s",
                 (mods & PUGL_MOD_SHIFT) ? " Shift" : "",
                 (mods & PUGL_MOD_CTRL) ? " Ctrl" : "",
                 (mods & PUGL_MOD_ALT) ? " Alt" : "",
                 (mods & PUGL_MOD_SUPER) ? " Super" : "",
                 (mods & PUGL_MOD_NUM_LOCK) ? " Num" : "",
                 (mods & PUGL_MOD_SCROLL_LOCK) ? " Scroll" : "",
                 (mods & PUGL_MOD_CAPS_LOCK) ? " Caps" : "");
}

static inline int
printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
{
#define PFFMT "%6.1f %6.1f"
#define PIFMT "%5d %5d"
#define PUFMT "%5u %5u"

#define PRINT(fmt, ...) (void)fprintf(stderr, fmt, __VA_ARGS__)

  if (!verbose && (event->type == PUGL_UPDATE || event->type == PUGL_BLANK ||
                   event->type == PUGL_DAMAGE || event->type == PUGL_EXPOSE ||
                   event->type == PUGL_FLUSH || event->type == PUGL_MOTION ||
                   event->type == PUGL_TIMER)) {
    return 0;
  }

  PRINT("%s%14s", prefix, puglEventTypeName(event->type));

  switch (event->type) {
  case PUGL_CONFIGURE:
    PRINT(" " PIFMT " " PUFMT " (",
          event->configure.x,
          event->configure.y,
          event->configure.width,
          event->configure.height);
    for (PuglViewStyleFlags mask = 1U; mask <= PUGL_MAX_VIEW_STYLE_FLAG;
         mask <<= 1U) {
      if (event->configure.style & mask) {
        PRINT(" %s", viewStyleFlagString((PuglViewStyleFlag)mask));
      }
    }
    PRINT("%s", " )");
    break;
  case PUGL_DAMAGE:
  case PUGL_EXPOSE:
    PRINT(" " PIFMT " " PUFMT,
          event->expose.x,
          event->expose.y,
          event->expose.width,
          event->expose.height);
    break;
  case PUGL_KEY_PRESS:
  case PUGL_KEY_RELEASE:
    PRINT(" code %3u key  U+%04X (%s) ",
          event->key.keycode,
          event->key.key,
          keyString(event->key.key));
    printModifiers(event->scroll.state);
    break;
  case PUGL_TEXT:
    PRINT(" code %3u char U+%04X (%s)",
          event->text.keycode,
          event->text.character,
          event->text.string);
    break;
  case PUGL_BUTTON_PRESS:
  case PUGL_BUTTON_RELEASE:
    PRINT(
      " %u " PFFMT " ", event->button.button, event->button.x, event->button.y);
    printModifiers(event->scroll.state);
    break;
  case PUGL_MOTION:
    PRINT(" " PFFMT, event->motion.x, event->motion.y);
    break;
  case PUGL_SCROLL:
    PRINT(" %5.1f %5.1f (%s) at " PFFMT " ",
          event->scroll.dx,
          event->scroll.dy,
          scrollDirectionString(event->scroll.direction),
          event->scroll.x,
          event->scroll.y);
    printModifiers(event->scroll.state);
    break;
  case PUGL_POINTER_IN:
  case PUGL_POINTER_OUT:
    PRINT(" " PFFMT " (%s)",
          event->crossing.x,
          event->crossing.y,
          crossingModeString(event->crossing.mode));
    break;
  case PUGL_FOCUS_IN:
  case PUGL_FOCUS_OUT:
    PRINT(" (%s)", crossingModeString(event->crossing.mode));
    break;
  case PUGL_CLIENT:
    PRINT(" %" PRIXPTR " %" PRIXPTR, event->client.data1, event->client.data2);
    break;
  case PUGL_TIMER:
    PRINT(" %" PRIuPTR, event->timer.id);
    break;
  case PUGL_DATA_OFFER:
    PRINT("%sData offer at   " PFFMT, prefix, event->offer.x, event->offer.y);
    break;
  default:
    break;
  }

  fprintf(stderr, "\n");
#undef PRINT
#undef PUFMT
#undef PIFMT
#undef PFFMT

  return 0;
}

static inline void
printViewHints(const PuglView* view)
{
  for (unsigned i = 0; i < PUGL_NUM_VIEW_HINTS; ++i) {
    fprintf(stderr,
            "%s: %d\n",
            viewHintString((PuglViewHint)i),
            puglGetViewHint(view, (PuglViewHint)i));
  }
}

static inline void
puglPrintTestUsage(const char* prog, const char* posHelp)
{
  printf("Usage: %s [OPTION]... %s\n\n"
         "  -E  Use OpenGL ES\n"
         "  -G  OpenGL context version\n"
         "  -a  Enable anti-aliasing\n"
         "  -b  Block and only update on user input\n"
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
    PUGL_OPENGL_API,
    3,
    3,
    true,
    false,
    false,
    false,
    false,
    false,
  };

  char** const argv = *pargv;
  int          i    = 1;
  for (; i < *pargc; ++i) {
    if (!strcmp(argv[i], "-E")) {
      opts.glApi = PUGL_OPENGL_ES_API;
    } else if (!strcmp(argv[i], "-G")) {
      if (++i == *pargc) {
        fprintf(stderr, "error: Missing OpenGL version argument\n");
        return opts;
      }

      const int matches =
        sscanf(argv[i], "%d.%d", &opts.glMajorVersion, &opts.glMinorVersion);
      if (matches != 2) {
        fprintf(stderr, "error: Invalid OpenGL version argument\n");
        return opts;
      }
    } else if (!strcmp(argv[i], "-a")) {
      opts.samples = 4;
    } else if (!strcmp(argv[i], "-b")) {
      opts.continuous = false;
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

#endif // PUGLUTIL_TEST_UTILS_H
