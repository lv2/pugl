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
  int  rc    = fprintf(stderr, "[");
  bool first = true;
  for (uint32_t m = 1U; m <= PUGL_MOD_CAPS_LOCK; m <<= 1U) {
    if ((mods & m)) {
      rc += first ? 0 : fprintf(stderr, " ");
      rc += fprintf(stderr, "%s", modString((PuglMod)m));
      first = false;
    }
  }

  return rc + fprintf(stderr, "]");
}

static inline int
printViewStyleFlags(const PuglViewStyleFlags flags)
{
  int  rc    = fprintf(stderr, "[");
  bool first = true;
  for (uint32_t m = 1U; m <= PUGL_MAX_VIEW_STYLE_FLAG; m <<= 1U) {
    if ((flags & m)) {
      rc += first ? 0 : fprintf(stderr, " ");
      rc += fprintf(stderr, "%s", viewStyleFlagString((PuglViewStyleFlag)m));
      first = false;
    }
  }

  return rc + fprintf(stderr, "]");
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
    PRINT(" " PIFMT " " PUFMT " x%.2f ",
          event->configure.x,
          event->configure.y,
          event->configure.width,
          event->configure.height,
          event->configure.scale);
    printViewStyleFlags(event->configure.style);
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
  case PUGL_KEY_RELEASE: {
    const char* const string = keyString(event->key.key);
    PRINT(" code %3u key  U+%04X %13s @ %.3f s ",
          event->key.keycode,
          event->key.key,
          (string[0] == '?') ? "" : string,
          event->key.time);
    printModifiers(event->scroll.state);
    break;
  }
  case PUGL_TEXT:
    PRINT(" code %3u char U+%04X %13s @ %.3f s",
          event->text.keycode,
          event->text.character,
          event->text.string,
          event->text.time);
    break;
  case PUGL_BUTTON_PRESS:
  case PUGL_BUTTON_RELEASE:
    PRINT(" %u " PFFMT " @ %.3f s ",
          event->button.button,
          event->button.x,
          event->button.y,
          event->button.time);
    printModifiers(event->scroll.state);
    break;
  case PUGL_MOTION:
    PRINT(" " PFFMT " @ %.3f s",
          event->motion.x,
          event->motion.y,
          event->motion.time);
    break;
  case PUGL_SCROLL:
    PRINT(" %5.1f %5.1f (%s) at " PFFMT " @ %.3f s ",
          event->scroll.dx,
          event->scroll.dy,
          scrollDirectionString(event->scroll.direction),
          event->scroll.x,
          event->scroll.y,
          event->scroll.time);
    printModifiers(event->scroll.state);
    break;
  case PUGL_POINTER_IN:
  case PUGL_POINTER_OUT:
    PRINT(" " PFFMT " (%s) @ %.3f s",
          event->crossing.x,
          event->crossing.y,
          crossingModeString(event->crossing.mode),
          event->crossing.time);
    break;
  case PUGL_FOCUS_IN:
  case PUGL_FOCUS_OUT:
    PRINT(" (%s)", crossingModeString(event->focus.mode));
    break;
  case PUGL_CLIENT:
    PRINT(" %" PRIXPTR " %" PRIXPTR, event->client.data1, event->client.data2);
    break;
  case PUGL_TIMER:
    PRINT(" %" PRIuPTR, event->timer.id);
    break;
  case PUGL_DATA_OFFER:
    PRINT(" at " PFFMT " @ %.3f s",
          event->offer.x,
          event->offer.y,
          event->offer.time);
    break;
  case PUGL_DATA:
    PRINT(" index %u at " PFFMT " @ %.3f s",
          event->data.typeIndex,
          event->data.x,
          event->data.y,
          event->data.time);
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
