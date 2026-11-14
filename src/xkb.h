// Copyright 2022-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_XKB_H
#define PUGL_SRC_XKB_H

#include <pugl/pugl.h>

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t xkb_keysym_t;

/// The context and state to track XKB keyboard input
typedef struct {
  struct xkb_context*       context;
  struct xkb_keymap*        keymap;
  struct xkb_state*         state;
  struct xkb_compose_table* composeTable;
  struct xkb_compose_state* composeState;
  PuglMods                  depressedMods;
  uint32_t                  shiftModMask;
  uint32_t                  ctrlModMask;
  uint32_t                  altModMask;
  uint32_t                  superModMask;
} XkbKeyboard;

/// Connect to the keyboard and load keymap from a string
PuglStatus
xkbKeyboardOpen(XkbKeyboard* xkb, const char* locale, const char* mapString);

/// Close the keyboard connection
void
xkbKeyboardClose(XkbKeyboard* xkb);

/// Handle a change in modifier state
void
xkbOnModifiers(XkbKeyboard* xkb,
               uint32_t     modsDepressed,
               uint32_t     modsLatched,
               uint32_t     modsLocked,
               uint32_t     group);

/// Handle a key press or release
void
xkbOnKey(XkbKeyboard* xkb,
         PuglView*    view,
         double       time,
         uint32_t     keycode,
         bool         isPress);

/// Convert a keysym to a special key, or return PUGL_KEY_NONE
PuglKey
xkbKeysymToSpecial(xkb_keysym_t sym);

#endif // PUGL_SRC_XKB_H
