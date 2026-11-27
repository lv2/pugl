// Copyright 2012-2026 David Robillard <d@drobilla.net>
// Copyright 2021-2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "web.h"
#include "internal.h"
#include "platform.h"

#include <emscripten/html5.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#define PUGL_WEB_AUTO_POINTER_LOCK

typedef struct {
  double x;
  double y;
} CanvasPoint;

static const char*
getCanvasId(const PuglView* const view)
{
  const char* const string = view->strings[PUGL_CANVAS_ID];
  return string ? string : "canvas";
}

static CanvasPoint
getCanvasPosition(const char* const canvasId)
{
  const CanvasPoint pos = {
    EM_ASM_DOUBLE(
      {
        return document.getElementById(UTF8ToString($0))
          .getBoundingClientRect()
          .x;
      },
      canvasId),
    EM_ASM_DOUBLE(
      {
        return document.getElementById(UTF8ToString($0))
          .getBoundingClientRect()
          .y;
      },
      canvasId),
  };
  return pos;
}

// World

PuglWorldInternals*
puglAllocWorldInternals(void)
{
  return (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));
}

PuglStatus
puglInitWorldInternals(PuglWorld* const world,
                       PuglWorldFlags   PUGL_UNUSED(flags))
{
  PuglWorldInternals* const impl = world->impl;

  impl->scaleFactor = emscripten_get_device_pixel_ratio();

  return PUGL_SUCCESS;
}

void*
puglGetNativeWorld(PuglWorld* PUGL_UNUSED(world))
{
  return NULL;
}

double
puglGetWorldScaleFactor(const PuglWorld* PUGL_UNUSED(world),
                        PuglNativeView   PUGL_UNUSED(parent),
                        PuglPoint        PUGL_UNUSED(position))
{
  return emscripten_get_device_pixel_ratio();
}

PuglInternals*
puglInitViewInternals(PuglWorld* PUGL_UNUSED(world))
{
  PuglInternals* const impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  impl->supportsClipboardRead = (PuglViewHintValue)EM_ASM_INT(
    // clang-format off
    {
      return (typeof(navigator.clipboard) !== 'undefined' &&
              typeof(navigator.clipboard.readText) === 'function' &&
              window.isSecureContext);
    }
    // clang-format on
  );

  // clang-format off
  impl->supportsClipboardWrite = (PuglViewHintValue)EM_ASM_INT({
      return (typeof(navigator.clipboard) !== 'undefined' &&
              typeof(navigator.clipboard.writeText) === 'function' &&
              window.isSecureContext);
  });
  // clang-format on

  return impl;
}

PuglStatus
puglSetViewStyle(PuglView*          PUGL_UNUSED(view),
                 PuglViewStyleFlags PUGL_UNUSED(flags))
{
  return PUGL_UNSUPPORTED;
}

static PuglStatus
puglUpdateSizeHints(PuglView* const view)
{
  const char* const canvasId = getCanvasId(view);

  if (!view->hints[PUGL_RESIZABLE]) {
    const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    EM_ASM(
      {
        const canvas = document.getElementById(UTF8ToString($0));
        const width  = $1 / window.devicePixelRatio;
        const height = $2 / window.devicePixelRatio;
        canvas.style.setProperty('min-width', String(width) + 'px');
        canvas.style.setProperty('max-width', String(width) + 'px');
        canvas.style.setProperty('min-height', String(height) + 'px');
        canvas.style.setProperty('max-height', String(height) + 'px');
      },
      canvasId,
      size.width,
      size.height);
  } else {
    const PuglArea minSize = view->sizeHints[PUGL_MIN_SIZE];
    if (puglIsValidArea(minSize)) {
      EM_ASM(
        {
          const canvas    = document.getElementById(UTF8ToString($0));
          const minWidth  = $1 / window.devicePixelRatio;
          const minHeight = $2 / window.devicePixelRatio;
          canvas.style.setProperty('min-width', String(minWidth) + 'px');
          canvas.style.setProperty('min-height', String(minHeight) + 'px');
        },
        canvasId,
        minSize.width,
        minSize.height);
    } else {
      EM_ASM(
        {
          const canvas = document.getElementById(UTF8ToString($0));
          canvas.style.removeProperty('min-width');
          canvas.style.removeProperty('min-height');
        },
        canvasId);
    }

    const PuglArea maxSize = view->sizeHints[PUGL_MAX_SIZE];
    if (puglIsValidArea(maxSize)) {
      EM_ASM(
        {
          const canvas    = document.getElementById(UTF8ToString($0));
          const maxWidth  = $1 / window.devicePixelRatio;
          const maxHeight = $2 / window.devicePixelRatio;
          canvas.style.setProperty('max-width', String(maxWidth) + 'px');
          canvas.style.setProperty('max-height', String(maxHeight) + 'px');
        },
        canvasId,
        maxSize.width,
        maxSize.height);
    } else {
      EM_ASM(
        {
          const canvas = document.getElementById(UTF8ToString($0));
          canvas.style.removeProperty('max-width');
          canvas.style.removeProperty('max-height');
        },
        canvasId);
    }
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglApplySizeHint(PuglView* const view, PuglSizeHint PUGL_UNUSED(hint))
{
  // No fine-grained updates, hints are always recalculated together
  return puglUpdateSizeHints(view);
}

static PuglMods
translateModifiers(const EM_BOOL ctrlKey,
                   const EM_BOOL shiftKey,
                   const EM_BOOL altKey,
                   const EM_BOOL metaKey)
{
  return (ctrlKey ? PUGL_MOD_CTRL : 0U) | (shiftKey ? PUGL_MOD_SHIFT : 0U) |
         (altKey ? PUGL_MOD_ALT : 0U) | (metaKey ? PUGL_MOD_SUPER : 0U);
}

static PuglKey
keyToSpecial(const char* const key, const unsigned long location)
{
  (void)location;

  typedef struct {
    const char* name;
    PuglKey     key;
  } KeyName;

  if (location == DOM_KEY_LOCATION_STANDARD) {
    static const KeyName keyNames[] = {
      {"Backspace", PUGL_KEY_BACKSPACE},
      {"Tab", PUGL_KEY_TAB},
      {"Enter", PUGL_KEY_ENTER},
      {"Escape", PUGL_KEY_ESCAPE},
      {"Delete", PUGL_KEY_DELETE},
      {"Space", PUGL_KEY_SPACE},
      {"F1", PUGL_KEY_F1},
      {"F2", PUGL_KEY_F2},
      {"F3", PUGL_KEY_F3},
      {"F4", PUGL_KEY_F4},
      {"F5", PUGL_KEY_F5},
      {"F6", PUGL_KEY_F6},
      {"F7", PUGL_KEY_F7},
      {"F8", PUGL_KEY_F8},
      {"F9", PUGL_KEY_F9},
      {"F10", PUGL_KEY_F10},
      {"F11", PUGL_KEY_F11},
      {"F12", PUGL_KEY_F12},
      {"PageUp", PUGL_KEY_PAGE_UP},
      {"PageDown", PUGL_KEY_PAGE_DOWN},
      {"End", PUGL_KEY_END},
      {"Home", PUGL_KEY_HOME},
      {"ArrowLeft", PUGL_KEY_LEFT},
      {"ArrowUp", PUGL_KEY_UP},
      {"ArrowRight", PUGL_KEY_RIGHT},
      {"ArrowDown", PUGL_KEY_DOWN},
      {"PrintScreen", PUGL_KEY_PRINT_SCREEN},
      {"Insert", PUGL_KEY_INSERT},
      {"Pause", PUGL_KEY_PAUSE},
      {"Menu", PUGL_KEY_MENU},
      {"NumLock", PUGL_KEY_NUM_LOCK},
      {"ScrollLock", PUGL_KEY_SCROLL_LOCK},
      {"CapsLock", PUGL_KEY_CAPS_LOCK},
    };

    static const size_t numKeyNames = sizeof(keyNames) / sizeof(keyNames[0]);
    for (size_t i = 0U; i < numKeyNames; ++i) {
      if (!strcmp(key, keyNames[i].name)) {
        return keyNames[i].key;
      }
    }
  }

  static const KeyName modNames[] = {
    {"Shift", PUGL_KEY_SHIFT_L},
    {"Control", PUGL_KEY_CTRL_L},
    {"Alt", PUGL_KEY_ALT_L},
    {"Meta", PUGL_KEY_SUPER_L},
  };

  static const size_t numModNames = sizeof(modNames) / sizeof(modNames[0]);
  for (size_t i = 0U; i < numModNames; ++i) {
    if (!strcmp(key, modNames[i].name)) {
      if (location == DOM_KEY_LOCATION_STANDARD ||
          location == DOM_KEY_LOCATION_LEFT) {
        return modNames[i].key;
      }

      if (location == DOM_KEY_LOCATION_RIGHT) {
        return modNames[i].key + 1;
      }
    }
  }

  if (location == DOM_KEY_LOCATION_NUMPAD) {
    static const KeyName padKeyNames[] = {
      {"0", PUGL_KEY_PAD_0},
      {"1", PUGL_KEY_PAD_1},
      {"2", PUGL_KEY_PAD_2},
      {"3", PUGL_KEY_PAD_3},
      {"4", PUGL_KEY_PAD_4},
      {"5", PUGL_KEY_PAD_5},
      {"6", PUGL_KEY_PAD_6},
      {"7", PUGL_KEY_PAD_7},
      {"8", PUGL_KEY_PAD_8},
      {"9", PUGL_KEY_PAD_9},
      {"Enter", PUGL_KEY_PAD_ENTER},
      {"PageUp", PUGL_KEY_PAD_PAGE_UP},
      {"PageDown", PUGL_KEY_PAD_PAGE_DOWN},
      {"End", PUGL_KEY_PAD_END},
      {"Home", PUGL_KEY_PAD_HOME},
      {"ArrowLeft", PUGL_KEY_PAD_LEFT},
      {"ArrowUp", PUGL_KEY_PAD_UP},
      {"ArrowRight", PUGL_KEY_PAD_RIGHT},
      {"ArrowDown", PUGL_KEY_PAD_DOWN},
      {"Clear", PUGL_KEY_PAD_CLEAR},
      {"Insert", PUGL_KEY_PAD_INSERT},
      {"Delete", PUGL_KEY_PAD_DELETE},
      {"Equal", PUGL_KEY_PAD_EQUAL},
      {"Multiply", PUGL_KEY_PAD_MULTIPLY},
      {"Add", PUGL_KEY_PAD_ADD},
      {"Separator", PUGL_KEY_PAD_SEPARATOR},
      {"Subtract", PUGL_KEY_PAD_SUBTRACT},
      {"Decimal", PUGL_KEY_PAD_DECIMAL},
      {"Divide", PUGL_KEY_PAD_DIVIDE},
    };

    static const size_t numPadKeyNames =
      sizeof(padKeyNames) / sizeof(padKeyNames[0]);
    for (size_t i = 0U; i < numPadKeyNames; ++i) {
      if (!strcmp(key, padKeyNames[i].name)) {
        return padKeyNames[i].key;
      }
    }
  }

  return (PuglKey)0;
}

static bool
decodeCharacterString(unsigned long PUGL_UNUSED(keyCode),
                      const EM_UTF8 key[EM_HTML5_SHORT_STRING_LEN_BYTES],
                      char          str[8])
{
  if (key[1] == 0) {
    str[0] = key[0];
    return true;
  }

  return false;
}

static EM_BOOL
puglKeyCallback(const int                            eventType,
                const EmscriptenKeyboardEvent* const keyEvent,
                void* const                          userData)
{
  PuglView* const view = (PuglView*)userData;

  if (!view->impl->visible) {
    return EM_FALSE;
  }

  if (keyEvent->repeat && view->hints[PUGL_IGNORE_KEY_REPEAT]) {
    return EM_TRUE; // Consumed
  }

  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;

  const unsigned state = translateModifiers(
    keyEvent->ctrlKey, keyEvent->shiftKey, keyEvent->altKey, keyEvent->metaKey);

  const PuglKey special = keyToSpecial(keyEvent->key, keyEvent->location);

  unsigned key = (keyEvent->key[0] >= ' ' && keyEvent->key[0] <= '~' &&
                  keyEvent->key[1] == '\0')
                   ? (unsigned)keyEvent->key[0]
                   : keyEvent->keyCode;

  if (key >= 'A' && key <= 'Z' && !keyEvent->shiftKey) {
    key += 'a' - 'A';
  }

  const bool isPress = (eventType == EMSCRIPTEN_EVENT_KEYDOWN);

  const PuglEvent event = {
    .key = {isPress ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE,
            0U,
            keyEvent->timestamp / 1e3,
            0.0,
            0.0,
            0.0,
            0.0,
            state,
            keyEvent->keyCode,
            special ? special : key},
  };

  st0 = puglDispatchEvent(view, &event);

  if (isPress && !special &&
      !(keyEvent->ctrlKey | keyEvent->altKey | keyEvent->metaKey)) {
    char str[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    if (decodeCharacterString(keyEvent->keyCode, keyEvent->key, str)) {
      PuglEvent text = {.text = {PUGL_TEXT,
                                 0U,
                                 keyEvent->timestamp / 1e3,
                                 0.0,
                                 0.0,
                                 0.0,
                                 0.0,
                                 state,
                                 keyEvent->keyCode,
                                 special ? special : key,
                                 {0, 0, 0, 0, 0, 0, 0, 0}}};
      memcpy(text.text.string, str, sizeof(event.text.string));
      st1 = puglDispatchEvent(view, &text);
    }
  }

  const bool consumed = !st0 && !st1;
  return consumed;
}

static EM_BOOL
puglMouseCallback(const int                         eventType,
                  const EmscriptenMouseEvent* const mouseEvent,
                  void* const                       userData)
{
  PuglView* const view = (PuglView*)userData;

  if (!view->impl->visible) {
    return EM_FALSE;
  }

  const double   time  = mouseEvent->timestamp / 1e3;
  const PuglMods state = translateModifiers(mouseEvent->ctrlKey,
                                            mouseEvent->shiftKey,
                                            mouseEvent->altKey,
                                            mouseEvent->metaKey);

  // workaround missing pointer lock callback, see
  // https://github.com/emscripten-core/emscripten/issues/9681
  EmscriptenPointerlockChangeEvent e;
  if (emscripten_get_pointerlock_status(&e) == EMSCRIPTEN_RESULT_SUCCESS) {
    view->impl->pointerLocked = e.isActive;
  }

  const CanvasPoint canvasPos = getCanvasPosition(getCanvasId(view));
  const double      scale     = view->world->impl->scaleFactor;
  const double      eventX    = (mouseEvent->clientX - canvasPos.x) * scale;
  const double      eventY    = (mouseEvent->clientY - canvasPos.y) * scale;
  PuglEvent         event     = {{PUGL_NOTHING, 0U}};
  switch (eventType) {
  case EMSCRIPTEN_EVENT_MOUSEDOWN:
  case EMSCRIPTEN_EVENT_MOUSEUP:
    event.button.type =
      (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN ? PUGL_BUTTON_PRESS
                                               : PUGL_BUTTON_RELEASE);

    event.button.time   = time;
    event.button.x      = eventX;
    event.button.y      = eventY;
    event.button.xRoot  = mouseEvent->screenX * scale;
    event.button.yRoot  = mouseEvent->screenY * scale;
    event.button.state  = state;
    event.button.button = (mouseEvent->button == 1)   ? 2
                          : (mouseEvent->button == 2) ? 1
                                                      : mouseEvent->button;
    break;
  case EMSCRIPTEN_EVENT_MOUSEMOVE:
    event.motion.type = PUGL_MOTION;
    event.motion.time = time;
    if (view->impl->pointerLocked) {
      // adjust local values for delta
      const double movementX = mouseEvent->movementX * scale;
      const double movementY = mouseEvent->movementY * scale;
      view->impl->lastMotion.x += movementX;
      view->impl->lastMotion.y += movementY;
      view->impl->lastMotion.xRoot += movementX;
      view->impl->lastMotion.yRoot += movementY;
      // now set x, y, xRoot and yRoot
      event.motion.x     = view->impl->lastMotion.x;
      event.motion.y     = view->impl->lastMotion.y;
      event.motion.xRoot = view->impl->lastMotion.xRoot;
      event.motion.yRoot = view->impl->lastMotion.yRoot;
    } else {
      // cache values for possible pointer lock movement later
      view->impl->lastMotion.x = event.motion.x = eventX;
      view->impl->lastMotion.y = event.motion.y = eventY;
      view->impl->lastMotion.xRoot              = event.motion.xRoot =
        mouseEvent->screenX * scale;
      view->impl->lastMotion.yRoot = event.motion.yRoot =
        mouseEvent->screenY * scale;
    }
    event.motion.state = state;
    break;
  case EMSCRIPTEN_EVENT_MOUSEENTER:
  case EMSCRIPTEN_EVENT_MOUSELEAVE:
    event.crossing.type  = eventType == EMSCRIPTEN_EVENT_MOUSEENTER
                             ? PUGL_POINTER_IN
                             : PUGL_POINTER_OUT;
    event.crossing.time  = time;
    event.crossing.x     = eventX;
    event.crossing.y     = eventY;
    event.crossing.xRoot = mouseEvent->screenX * scale;
    event.crossing.yRoot = mouseEvent->screenY * scale;
    event.crossing.state = state;
    event.crossing.mode  = PUGL_CROSSING_NORMAL;
    break;
  }

  puglDispatchEvent(view, &event);

#ifdef PUGL_WEB_AUTO_POINTER_LOCK
  if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
    emscripten_request_pointerlock(getCanvasId(view), false);
  } else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) {
    emscripten_exit_pointerlock();
  }
#endif

  return EM_FALSE; // Don't consume event, so canvas can get focus
}

static EM_BOOL
puglFocusCallback(const int                   eventType,
                  const EmscriptenFocusEvent* PUGL_UNUSED(focusEvent),
                  void* const                 userData)
{
  PuglView* const view  = (PuglView*)userData;
  const bool      isIn  = (eventType == EMSCRIPTEN_EVENT_FOCUSIN);
  const PuglEvent event = {
    .focus = {isIn ? PUGL_FOCUS_IN : PUGL_FOCUS_OUT, 0U, PUGL_CROSSING_NORMAL},
  };

  puglDispatchEvent(view, &event);
  view->impl->focused = isIn;

  return EM_FALSE; // Don't consume event, so canvas can get focus
}

static EM_BOOL
puglPointerLockChangeCallback(
  int                                           PUGL_UNUSED(eventType),
  const EmscriptenPointerlockChangeEvent* const event,
  void* const                                   userData)
{
  PuglView* const view = (PuglView*)userData;

  view->impl->pointerLocked = event->isActive;
  return EM_TRUE;
}

static EM_BOOL
puglWheelCallback(int                               PUGL_UNUSED(eventType),
                  const EmscriptenWheelEvent* const wheelEvent,
                  void* const                       userData)
{
  static const double speedFactor = 1.0 / 120.0;

  PuglView* const view = (PuglView*)userData;
  if (!view->impl->visible) {
    return EM_FALSE;
  }

  const CanvasPoint canvasPos = getCanvasPosition(getCanvasId(view));
  const double      scale     = view->world->impl->scaleFactor;

  const double eventX = (wheelEvent->mouse.clientX - canvasPos.x) * scale;
  const double eventY = (wheelEvent->mouse.clientY - canvasPos.y) * scale;
  const double dx     = wheelEvent->deltaX * speedFactor * scale;
  const double dy     = -wheelEvent->deltaY * speedFactor * scale;

  const PuglScrollDirection direction =
    (dx == 0.0 && dy < 0.0)   ? PUGL_SCROLL_UP
    : (dx == 0.0 && dy > 0.0) ? PUGL_SCROLL_DOWN
    : (dx < 0.0 && dy == 0.0) ? PUGL_SCROLL_LEFT
    : (dx > 0.0 && dy == 0.0) ? PUGL_SCROLL_RIGHT
                              : PUGL_SCROLL_SMOOTH;

  const PuglEvent scroll = {
    .scroll = {PUGL_SCROLL,
               0U,
               wheelEvent->mouse.timestamp / 1e3,
               eventX,
               eventY,
               wheelEvent->mouse.screenX,
               wheelEvent->mouse.screenY,
               translateModifiers(wheelEvent->mouse.ctrlKey,
                                  wheelEvent->mouse.shiftKey,
                                  wheelEvent->mouse.altKey,
                                  wheelEvent->mouse.metaKey),
               direction,
               wheelEvent->deltaX * speedFactor * scale,
               -wheelEvent->deltaY * speedFactor * scale}};

  return !puglDispatchEvent(view, &scroll);
}

static EM_BOOL
puglResizeCallback(int                      PUGL_UNUSED(eventType),
                   const EmscriptenUiEvent* PUGL_UNUSED(uiEvent),
                   void* const              userData)
{
  PuglView* const   view         = (PuglView*)userData;
  const char* const canvasId     = getCanvasId(view);
  const bool        isFullscreen = view->impl->isFullscreen;

  EM_ASM(
    {
      const canvas = document.getElementById(UTF8ToString($0));
      const parent = canvas.parentElement;
      parent.style.setProperty('--device-pixel-ratio', window.devicePixelRatio)
    },
    canvasId);

  const int clientWidth = EM_ASM_INT(
    {
      return ($1) ? window.innerWidth
                  : document.getElementById(UTF8ToString($0)).clientWidth;
    },
    canvasId,
    isFullscreen);

  const int clientHeight = EM_ASM_INT(
    {
      return ($1) ? window.innerHeight
                  : document.getElementById(UTF8ToString($0)).clientHeight;
    },
    canvasId,
    isFullscreen);

  if (!clientWidth || !clientHeight) {
    return EM_FALSE;
  }

  const double   scale  = emscripten_get_device_pixel_ratio();
  const PuglSpan width  = (PuglSpan)(clientWidth * scale);
  const PuglSpan height = (PuglSpan)(clientHeight * scale);

  view->world->impl->scaleFactor = scale;

  const PuglEvent configure = {.configure = {PUGL_CONFIGURE,
                                             0U,
                                             view->lastConfigure.x,
                                             view->lastConfigure.y,
                                             width,
                                             height,
                                             scale,
                                             PUGL_VIEW_STYLE_MAPPED}};
  puglDispatchEvent(view, &configure);

  emscripten_set_canvas_element_size(getCanvasId(view), width, height);
  puglObscureView(view);

  return EM_TRUE;
}

static EM_BOOL
puglFullscreenChangeCallback(
  int                                          PUGL_UNUSED(eventType),
  const EmscriptenFullscreenChangeEvent* const fscEvent,
  void* const                                  userData)
{
  PuglView* const view = (PuglView*)userData;

  view->impl->isFullscreen = fscEvent->isFullscreen;

  const double scaleFactor = emscripten_get_device_pixel_ratio();

  view->world->impl->scaleFactor = scaleFactor;

  if (!fscEvent->isFullscreen) {
    return puglResizeCallback(0, NULL, userData);
  }

  const int width  = EM_ASM_INT({ return window.innerWidth; });
  const int height = EM_ASM_INT({ return window.innerHeight; });

  PuglEvent event        = {{PUGL_CONFIGURE, 0}};
  event.configure.x      = 0;
  event.configure.y      = 0;
  event.configure.width  = (PuglSpan)width;
  event.configure.height = (PuglSpan)height;
  puglDispatchEvent(view, &event);

  emscripten_set_canvas_element_size(getCanvasId(view), width, height);

  return EM_TRUE;
}

static EM_BOOL
puglVisibilityChangeCallback(
  int                                          PUGL_UNUSED(eventType),
  const EmscriptenVisibilityChangeEvent* const visibilityChangeEvent,
  void* const                                  userData)
{
  PuglView* const view = (PuglView*)userData;

  view->impl->visible = visibilityChangeEvent->hidden == EM_FALSE;
  return EM_FALSE;
}

PuglStatus
puglRealize(PuglView* const view)
{
  PuglStatus st = PUGL_SUCCESS;

  // Ensure that we're unrealized
  if (view->impl->created) {
    return PUGL_FAILURE;
  }

  // Check that the basic required configuration has been done
  if ((st = puglPreRealize(view))) {
    return st;
  }

  const char* const canvasId = getCanvasId(view);

  // const PuglPoint defaultPos  = view->positionHints[PUGL_DEFAULT_POSITION];
  const PuglArea defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
  if (!defaultSize.width || !defaultSize.height) {
    return PUGL_BAD_CONFIGURATION;
  }

  // Attach the backend and create a surface
  if ((st = view->backend->attach(view)) ||
      (st = view->backend->create(view))) {
    view->backend->destroy(view);
    return st;
  }

  if (view->strings[PUGL_WINDOW_TITLE]) {
    emscripten_set_window_title(view->strings[PUGL_WINDOW_TITLE]);
  }

  puglDispatchSimpleEvent(view, PUGL_REALIZE);

  const PuglEvent configure = {.configure = {PUGL_CONFIGURE,
                                             0U,
                                             0,
                                             0,
                                             defaultSize.width,
                                             defaultSize.height,
                                             view->world->impl->scaleFactor,
                                             PUGL_VIEW_STYLE_MAPPED}};
  puglDispatchEvent(view, &configure);

  view->impl->created = true;

  EM_ASM(
    {
      const canvas = document.getElementById(UTF8ToString($0));
      const parent = canvas.parentElement;
      parent.style.setProperty('--device-pixel-ratio', window.devicePixelRatio);
    },
    canvasId);

  puglUpdateSizeHints(view);

  emscripten_set_canvas_element_size(
    canvasId, defaultSize.width, defaultSize.height);

  emscripten_set_focusin_callback(canvasId, view, true, puglFocusCallback);
  emscripten_set_focusout_callback(canvasId, view, true, puglFocusCallback);
  emscripten_set_keydown_callback(canvasId, view, true, puglKeyCallback);
  emscripten_set_keyup_callback(canvasId, view, true, puglKeyCallback);

  emscripten_set_mousedown_callback(canvasId, view, true, puglMouseCallback);
  emscripten_set_mouseup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, view, true, puglMouseCallback);
  emscripten_set_mousemove_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, view, true, puglMouseCallback);
  emscripten_set_mouseenter_callback(canvasId, view, true, puglMouseCallback);
  emscripten_set_mouseleave_callback(canvasId, view, true, puglMouseCallback);
  emscripten_set_wheel_callback(canvasId, view, true, puglWheelCallback);

  emscripten_set_pointerlockchange_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, view, true, puglPointerLockChangeCallback);

  emscripten_set_resize_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, view, true, puglResizeCallback);
  emscripten_set_fullscreenchange_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, view, true, puglFullscreenChangeCallback);
  emscripten_set_visibilitychange_callback(
    view, true, puglVisibilityChangeCallback);

  return PUGL_SUCCESS;
}

PuglStatus
puglUnrealize(PuglView* PUGL_UNUSED(view))
{
  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view, PuglShowCommand PUGL_UNUSED(command))
{
  view->impl->visible      = true;
  view->impl->needsRepaint = true;
  return puglObscureView(view);
}

PuglStatus
puglHide(PuglView* const view)
{
  view->impl->visible = false;
  return PUGL_FAILURE;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    if (view->backend) {
      emscripten_set_mouseup_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
      emscripten_set_mousemove_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
      emscripten_set_pointerlockchange_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
      emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
      emscripten_set_fullscreenchange_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
      emscripten_set_visibilitychange_callback(NULL, false, NULL);
      view->backend->destroy(view);
    }
    free(view->impl->clipboardData);
    free(view->impl->timers);
    free(view->impl);
  }
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  free(world->impl);
}

PuglStatus
puglGrabFocus(PuglView* PUGL_UNUSED(view))
{
  return PUGL_UNSUPPORTED;
}

bool
puglHasFocus(const PuglView* const view)
{
  return view->impl->focused;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return view->world->impl->scaleFactor;
}

double
puglGetTime(const PuglWorld* PUGL_UNUSED(view))
{
  return emscripten_get_now() / 1e3;
}

PuglStatus
puglUpdateWorld(PuglWorld* const world, double PUGL_UNUSED(timeout))
{
  for (size_t i = 0; i < world->numViews; ++i) {
    PuglView* const view = world->views[i];

    if (!view->impl->visible) {
      continue;
    }

    puglDispatchSimpleEvent(view, PUGL_UPDATE);
    if (view->hints[PUGL_CONTINUOUS]) {
      puglObscureView(view);
    }

    if (!view->impl->needsRepaint) {
      continue;
    }

    view->impl->needsRepaint = false;

    puglDispatchSimpleEvent(view, PUGL_BLANK);

    const PuglEvent damage = {
      .damage = {PUGL_DAMAGE,
                 0U,
                 0,
                 0,
                 view->lastConfigure.width,
                 view->lastConfigure.height},
    };
    puglDispatchEvent(view, &damage);

    const PuglEvent expose = {
      .expose = {PUGL_EXPOSE,
                 0U,
                 0,
                 0,
                 view->lastConfigure.width,
                 view->lastConfigure.height},
    };
    puglDispatchEvent(view, &expose);

    puglDispatchSimpleEvent(view, PUGL_FLUSH);
  }

  return PUGL_SUCCESS;
}

static void
mainIteration(void* const arg)
{
  PuglWorld* const world = (PuglWorld*)arg;
  puglUpdate(world, 0.0);
}

PuglStatus
puglRun(PuglWorld* const world)
{
  emscripten_set_main_loop_arg(mainIteration, world, 60, true);
  emscripten_runtime_keepalive_push();
  return PUGL_SUCCESS;
}

PuglStatus
puglObscureView(PuglView* const view)
{
  view->impl->needsRepaint = true;
  return PUGL_SUCCESS;
}

PuglStatus
puglObscureRegion(PuglView*      view,
                  const int      x,
                  const int      y,
                  const unsigned width,
                  const unsigned height)
{
  if (!puglIsValidPosition(x, y) || !puglIsValidSize(width, height)) {
    return PUGL_BAD_PARAMETER;
  }

  view->impl->needsRepaint = true;
  return PUGL_FAILURE;
}

PuglNativeView
puglGetNativeView(const PuglView* PUGL_UNUSED(view))
{
  return 0;
}

PuglStatus
puglApplyViewString(PuglView*            PUGL_UNUSED(view),
                    const PuglStringHint key,
                    const char* const    value)
{
  if (key == PUGL_WINDOW_TITLE) {
    emscripten_set_window_title(value);
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetWindowPosition(PuglView* PUGL_UNUSED(view),
                      int       PUGL_UNUSED(x),
                      int       PUGL_UNUSED(y))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglSetWindowSize(PuglView* const view,
                  const unsigned  width,
                  const unsigned  height)
{
  if (view->impl->created) {
    const char* const canvasId = getCanvasId(view);
    emscripten_set_canvas_element_size(canvasId, (int)width, (int)height);
  }

  return PUGL_SUCCESS;
}

static EM_BOOL
puglTimerLoopCallback(double PUGL_UNUSED(timeout), void* const arg)
{
  PuglTimer* const     timer = (PuglTimer*)arg;
  PuglInternals* const impl  = timer->view->impl;

  // only handle active timers
  for (uint32_t i = 0; i < impl->numTimers; ++i) {
    if (impl->timers[i].id == timer->id) {
      PuglEvent event = {{PUGL_TIMER, 0}};
      event.timer.id  = timer->id;
      puglDispatchEvent(timer->view, &event);
      return EM_TRUE;
    }
  }

  return EM_FALSE;
}

int
puglFetchViewHint(const PuglView* PUGL_UNUSED(view),
                  PuglViewHint    PUGL_UNUSED(hint))
{
  return PUGL_DONT_CARE;
}

PuglStatus
puglReconfigure(PuglView* PUGL_UNUSED(view))
{
  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* const view, const uintptr_t id, const double timeout)
{
  PuglInternals* const impl       = view->impl;
  const uint32_t       timerIndex = impl->numTimers++;

  PuglTimer* const newTimers =
    (PuglTimer*)realloc(impl->timers, sizeof(PuglTimer) * impl->numTimers);
  if (!newTimers) {
    return PUGL_NO_MEMORY;
  }

  impl->timers = newTimers;

  PuglTimer* const timer = &impl->timers[timerIndex];
  timer->view            = view;
  timer->id              = id;

  emscripten_set_timeout_loop(puglTimerLoopCallback, timeout * 1e3, timer);
  return PUGL_SUCCESS;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
  PuglInternals* const impl = view->impl;

  if (!impl->timers || !impl->numTimers) {
    return PUGL_FAILURE;
  }

  for (uint32_t i = 0; i < impl->numTimers; ++i) {
    if (impl->timers[i].id == id) {
      memmove(impl->timers + i,
              impl->timers + (i + 1),
              sizeof(PuglTimer) * (impl->numTimers - 1));
      --impl->numTimers;
      return PUGL_SUCCESS;
    }
  }

  return PUGL_FAILURE;
}

PuglStatus
puglSendEvent(PuglView* PUGL_UNUSED(view), const PuglEvent* PUGL_UNUSED(event))
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglRejectOffer(PuglView*                 PUGL_UNUSED(view),
                const PuglDataOfferEvent* PUGL_UNUSED(offer),
                int                       PUGL_UNUSED(regionX),
                int                       PUGL_UNUSED(regionY),
                unsigned                  PUGL_UNUSED(regionWidth),
                unsigned                  PUGL_UNUSED(regionHeight))
{
  return PUGL_SUCCESS;
}

void
// NOLINTNEXTLINE(misc-use-internal-linkage)
onClipboardText(PuglView* view, const char* text);

void
onClipboardText(PuglView* const view, const char* const text)
{
  puglSetString(&view->impl->clipboardData, text);

  const PuglEvent offer = {.offer = {PUGL_DATA_OFFER,
                                     0U,
                                     puglGetTime(view->world),
                                     0.0,
                                     0.0,
                                     PUGL_CLIPBOARD_GENERAL}};

  puglDispatchEvent(view, &offer);
}

PuglStatus
puglPaste(PuglView* const view)
{
  if (!view->impl->supportsClipboardRead) {
    return PUGL_UNSUPPORTED;
  }

  free(view->impl->clipboardData);
  view->impl->clipboardData = NULL;
  EM_ASM({navigator.clipboard.readText().then(function(text) {
           Module.ccall(
             'onClipboardText', null, ['number', 'string'], [$0, text]);
         })},
         view);

  if (!view->impl->clipboardData) {
    return PUGL_FAILURE;
  }

  const PuglDataOfferEvent offer = {
    PUGL_DATA_OFFER,
    0U,
    emscripten_get_now() / 1e3,
    0.0,
    0.0,
    PUGL_CLIPBOARD_GENERAL,
  };

  PuglEvent offerEvent;
  offerEvent.offer = offer;
  puglDispatchEvent(view, &offerEvent);
  return PUGL_SUCCESS;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const offer,
                const uint32_t                  typeIndex,
                PuglDataAction                  PUGL_UNUSED(action),
                const int                       regionX,
                const int                       regionY,
                unsigned                        PUGL_UNUSED(regionWidth),
                unsigned                        PUGL_UNUSED(regionHeight))
{
  if (typeIndex != 0) {
    return PUGL_UNSUPPORTED;
  }

  const PuglEvent event = {.data = {PUGL_DATA,
                                    0U,
                                    emscripten_get_now() / 1e3,
                                    regionX,
                                    regionY,
                                    offer->clipboard,
                                    typeIndex}};

  puglDispatchEvent(view, &event);
  return PUGL_SUCCESS;
}

PuglStatus
puglRegisterDropType(PuglView* PUGL_UNUSED(view), const char* PUGL_UNUSED(type))
{
  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const view,
                         PuglClipboard         PUGL_UNUSED(clipboard))
{
  return view->impl->clipboardData ? 1U : 0U;
}

const char*
puglGetClipboardType(const PuglView* const view,
                     PuglClipboard         PUGL_UNUSED(clipboard),
                     const uint32_t        typeIndex)
{
  return (!typeIndex && view->impl->clipboardData) ? "text/plain" : NULL;
}

const void*
puglGetClipboard(PuglView* const view,
                 PuglClipboard   PUGL_UNUSED(clipboard),
                 uint32_t        PUGL_UNUSED(typeIndex),
                 size_t*         PUGL_UNUSED(len))
{
  return view->impl->clipboardData;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 PuglClipboard     PUGL_UNUSED(clipboard),
                 const char* const type,
                 const void* const data,
                 size_t            PUGL_UNUSED(len))
{
  if (!type || !!strcmp(type, "text/plain")) {
    return PUGL_UNSUPPORTED;
  }

  if (!view->impl->supportsClipboardWrite) {
    puglSetString(&view->impl->clipboardData, (const char*)data);
    return PUGL_UNSUPPORTED;
  }

  const char* const canvasId = getCanvasId(view);
  const char* const text     = (const char*)data;

  // clang-format off
  EM_ASM(
    {
      if (typeof(navigator.clipboard) !== 'undefined' &&
          typeof(navigator.clipboard.writeText) === 'function' &&
          window.isSecureContext) {
        navigator.clipboard.writeText(UTF8ToString($1));
      } else {
        const canvasClipboardObjName = UTF8ToString($0) + '_clipboard';
        var canvasClipboardElem =
          document.getElementById(canvasClipboardObjName);

        if (!canvasClipboardElem) {
          canvasClipboardElem    = document.createElement('textarea');
          canvasClipboardElem.id = canvasClipboardObjName;
          canvasClipboardElem.style.position   = 'fixed';
          canvasClipboardElem.style.whiteSpace = 'pre';
          canvasClipboardElem.style.zIndex     = '-1';
          canvasClipboardElem.setAttribute('readonly', true);
          document.body.appendChild(canvasClipboardElem);
        }

        canvasClipboardElem.textContent = UTF8ToString($1);
        canvasClipboardElem.select();
        document.execCommand('copy');
      }
    },
    canvasId,
    text);
  // clang-format on

  // FIXME proper return status
  return PUGL_SUCCESS;
}

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
  static const char* cursorNames[] = {
    "default",
    "text",
    "crosshair",
    "grabbing",
    "not-allowed",
    "ew-resize",
    "ns-resize",
    "se-resize",
    "sw-resize",
    "move",
  };

  const char* const canvasId = getCanvasId(view);

  EM_ASM(
    {
      const canvas     = document.getElementById(UTF8ToString($0));
      const cursorName = UTF8ToString($1);

      canvas.style.cursor = cursorName;
    },
    canvasId,
    cursorNames[cursor]);

  return PUGL_SUCCESS;
}

PuglStatus
puglSetTransientParent(PuglView*      PUGL_UNUSED(view),
                       PuglNativeView PUGL_UNUSED(parent))
{
  return PUGL_UNSUPPORTED;
}
