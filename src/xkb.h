// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_XKB_H
#define PUGL_SRC_XKB_H

#include <pugl/pugl.h>

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t xkb_keysym_t;

/// Context and state to track XKB keyboard input
typedef struct {
  struct xkb_context*       context;
  struct xkb_keymap*        keymap;
  struct xkb_state*         state;
  struct xkb_compose_table* composeTable;
  struct xkb_compose_state* composeState;
  PuglMods                  activeMods;
  uint32_t                  shiftModMask;
  uint32_t                  ctrlModMask;
  uint32_t                  altModMask;
  uint32_t                  superModMask;
  uint32_t                  capsLockModMask;
  uint32_t                  numLockModMask;
  uint32_t                  scrollLockModMask;
  bool                      isServer;
} PuglXkb;

/// Connect to the keyboard and load compose table from locale
PuglStatus
puglXkbOpen(PuglXkb* xkb, const char* locale, bool server);

/// Load the default keymap
PuglStatus
puglXkbLoadDefaultKeymap(PuglXkb* xkb);

/// Load a keymap from a string
PuglStatus
puglXkbLoadKeymapString(PuglXkb* xkb, const char* mapString);

/// Close the keyboard connection
void
puglXkbClose(PuglXkb* xkb);

/// Handle a change in modifier state
void
puglXkbOnModifiers(PuglXkb* xkb,
                   uint32_t modsDepressed,
                   uint32_t modsLatched,
                   uint32_t modsLocked,
                   uint32_t group);

/// Handle a key press or release
void
puglXkbOnKey(PuglXkb*  xkb,
             PuglView* view,
             double    time,
             uint32_t  keycode,
             bool      isPress);

/// Convert a keysym to a special key, or return PUGL_KEY_NONE
PuglKey
puglXkbKeysymToSpecial(xkb_keysym_t sym);

#endif // PUGL_SRC_XKB_H
