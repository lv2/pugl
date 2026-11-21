// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "xkb.h"
#include "internal.h"

#include <pugl/pugl.h>

#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include <assert.h>
#include <string.h>

PuglStatus
puglXkbOpen(PuglXkb* const xkb, const char* const locale, const bool server)
{
  xkb->context  = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  xkb->isServer = server;
  if (!xkb->context) {
    return PUGL_UNKNOWN_ERROR;
  }

  xkb->composeTable = xkb_compose_table_new_from_locale(
    xkb->context, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
  if (!xkb->composeTable) {
    puglXkbClose(xkb);
    return PUGL_UNKNOWN_ERROR;
  }

  xkb->composeState =
    xkb_compose_state_new(xkb->composeTable, XKB_COMPOSE_STATE_NO_FLAGS);
  if (!xkb->composeState) {
    puglXkbClose(xkb);
    return PUGL_UNKNOWN_ERROR;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
loadKeymap(PuglXkb* const xkb)
{
  xkb->state = xkb_state_new(xkb->keymap);
  if (!xkb->state) {
    return PUGL_NO_MEMORY;
  }

  // Build masks for each modifier to quickly build PuglMods on key presses
  xkb->shiftModMask      = xkb_keymap_mod_get_mask(xkb->keymap, "Shift");
  xkb->ctrlModMask       = xkb_keymap_mod_get_mask(xkb->keymap, "Control");
  xkb->altModMask        = xkb_keymap_mod_get_mask(xkb->keymap, "Alt");
  xkb->superModMask      = xkb_keymap_mod_get_mask(xkb->keymap, "Mod4");
  xkb->capsLockModMask   = xkb_keymap_mod_get_mask(xkb->keymap, "Lock");
  xkb->numLockModMask    = xkb_keymap_mod_get_mask(xkb->keymap, "NumLock");
  xkb->scrollLockModMask = xkb_keymap_mod_get_mask(xkb->keymap, "ScrollLock");
  return PUGL_SUCCESS;
}

PuglStatus
puglXkbLoadDefaultKeymap(PuglXkb* const xkb)
{
  assert(xkb->context);

  xkb->keymap = xkb_keymap_new_from_names2(
    xkb->context, NULL, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

  return xkb->keymap ? loadKeymap(xkb) : PUGL_UNKNOWN_ERROR;
}

PuglStatus
puglXkbLoadKeymapString(PuglXkb* const xkb, const char* const mapString)
{
  assert(xkb->context);

  xkb->keymap = xkb_keymap_new_from_string(xkb->context,
                                           mapString,
                                           XKB_KEYMAP_FORMAT_TEXT_V1,
                                           XKB_KEYMAP_COMPILE_NO_FLAGS);

  return xkb->keymap ? loadKeymap(xkb) : PUGL_UNKNOWN_ERROR;
}

void
puglXkbClose(PuglXkb* const xkb)
{
  if (xkb->composeState) {
    xkb_compose_state_unref(xkb->composeState);
  }

  if (xkb->state) {
    xkb_state_unref(xkb->state);
  }

  if (xkb->composeTable) {
    xkb_compose_table_unref(xkb->composeTable);
  }

  if (xkb->keymap) {
    xkb_keymap_unref(xkb->keymap);
  }

  if (xkb->context) {
    xkb_context_unref(xkb->context);
  }

  memset(xkb, 0, sizeof(*xkb));
}

static void
updateActiveMods(PuglXkb* const xkb, const uint32_t activeMods)
{
  xkb->activeMods =
    ((activeMods & xkb->shiftModMask) ? PUGL_MOD_SHIFT : 0U) |
    ((activeMods & xkb->ctrlModMask) ? PUGL_MOD_CTRL : 0U) |
    ((activeMods & xkb->altModMask) ? PUGL_MOD_ALT : 0U) |
    ((activeMods & xkb->superModMask) ? PUGL_MOD_SUPER : 0U) |
    ((activeMods & xkb->capsLockModMask) ? PUGL_MOD_CAPS_LOCK : 0U) |
    ((activeMods & xkb->numLockModMask) ? PUGL_MOD_NUM_LOCK : 0U) |
    ((activeMods & xkb->scrollLockModMask) ? PUGL_MOD_SCROLL_LOCK : 0U);
}

void
puglXkbOnModifiers(PuglXkb* const xkb,
                   const uint32_t modsDepressed,
                   const uint32_t modsLatched,
                   const uint32_t modsLocked,
                   const uint32_t group)
{
  if (xkb->state) {
    xkb_state_update_mask(
      xkb->state, modsDepressed, modsLatched, modsLocked, 0, 0, group);
  }

  updateActiveMods(xkb, modsDepressed | modsLocked);
}

void
puglXkbOnKey(PuglXkb* const  xkb,
             PuglView* const view,
             const double    time,
             const uint32_t  keycode,
             const bool      isPress)
{
  if (!xkb->state) {
    return;
  }

  if (xkb->isServer) {
    // Server tracks keys to update state, clients call puglXkbOnModifiers
    xkb_state_update_key(
      xkb->state, keycode, isPress ? XKB_KEY_DOWN : XKB_KEY_UP);
    updateActiveMods(
      xkb, xkb_state_serialize_mods(xkb->state, XKB_STATE_MODS_EFFECTIVE));
  }

  // Dispatch (relatively) raw key press or release event
  const xkb_keysym_t sym      = xkb_state_key_get_one_sym(xkb->state, keycode);
  const PuglKey      special  = puglXkbKeysymToSpecial(sym);
  const PuglEvent    keyEvent = {
    .key = {isPress ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE,
            0U,
            time,
            0.0,
            0.0,
            0.0,
            0.0,
            xkb->activeMods,
            keycode,
            special ? special : xkb_keysym_to_utf32(sym)}};
  puglDispatchEvent(view, &keyEvent);
  if (!isPress) {
    return;
  }

  // Update compose state for key press
  xkb_compose_state_feed(xkb->composeState, sym);
  enum xkb_compose_status status =
    xkb_compose_state_get_status(xkb->composeState);

  PuglEvent textEvent = {.text = {PUGL_NOTHING,
                                  0U,
                                  time,
                                  0.0,
                                  0.0,
                                  0.0,
                                  0.0,
                                  xkb->activeMods,
                                  keycode,
                                  0U,
                                  {0, 0, 0, 0, 0, 0, 0, 0}}};

  if (status == XKB_COMPOSE_COMPOSED) {
    // Composed text event
    const xkb_keysym_t textSym =
      xkb_compose_state_get_one_sym(xkb->composeState);
    if (textSym != XKB_KEY_NoSymbol) {
      textEvent.text.type      = PUGL_TEXT;
      textEvent.text.character = xkb_keysym_to_utf32(textSym);
      xkb_keysym_to_utf8(textSym, textEvent.text.string, 8);
    }
  } else if (status == XKB_COMPOSE_NOTHING &&
             xkb_state_key_get_utf8(xkb->state, keycode, NULL, 0)) {
    // Regular text event
    textEvent.text.type      = PUGL_TEXT;
    textEvent.text.character = xkb_state_key_get_utf32(xkb->state, keycode);
    xkb_state_key_get_utf8(xkb->state,
                           keycode,
                           textEvent.text.string,
                           sizeof(textEvent.text.string));
  }

  // Dispatch any resulting text event
  if (textEvent.text.type == PUGL_TEXT) {
    puglDispatchEvent(view, &textEvent);
  }

  // Reset compose state if a compose sequence is finished
  if (status == XKB_COMPOSE_COMPOSED || status == XKB_COMPOSE_CANCELLED) {
    xkb_compose_state_reset(xkb->composeState);
  }
}

PuglKey
puglXkbKeysymToSpecial(const xkb_keysym_t sym)
{
  // clang-format off
  switch (sym) {
  case XKB_KEY_F1:               return PUGL_KEY_F1;
  case XKB_KEY_F2:               return PUGL_KEY_F2;
  case XKB_KEY_F3:               return PUGL_KEY_F3;
  case XKB_KEY_F4:               return PUGL_KEY_F4;
  case XKB_KEY_F5:               return PUGL_KEY_F5;
  case XKB_KEY_F6:               return PUGL_KEY_F6;
  case XKB_KEY_F7:               return PUGL_KEY_F7;
  case XKB_KEY_F8:               return PUGL_KEY_F8;
  case XKB_KEY_F9:               return PUGL_KEY_F9;
  case XKB_KEY_F10:              return PUGL_KEY_F10;
  case XKB_KEY_F11:              return PUGL_KEY_F11;
  case XKB_KEY_F12:              return PUGL_KEY_F12;
  case XKB_KEY_Left:             return PUGL_KEY_LEFT;
  case XKB_KEY_Up:               return PUGL_KEY_UP;
  case XKB_KEY_Right:            return PUGL_KEY_RIGHT;
  case XKB_KEY_Down:             return PUGL_KEY_DOWN;
  case XKB_KEY_Page_Up:          return PUGL_KEY_PAGE_UP;
  case XKB_KEY_Page_Down:        return PUGL_KEY_PAGE_DOWN;
  case XKB_KEY_Home:             return PUGL_KEY_HOME;
  case XKB_KEY_End:              return PUGL_KEY_END;
  case XKB_KEY_Insert:           return PUGL_KEY_INSERT;
  case XKB_KEY_Shift_L:          return PUGL_KEY_SHIFT_L;
  case XKB_KEY_Shift_R:          return PUGL_KEY_SHIFT_R;
  case XKB_KEY_Control_L:        return PUGL_KEY_CTRL_L;
  case XKB_KEY_Control_R:        return PUGL_KEY_CTRL_R;
  case XKB_KEY_Alt_L:            return PUGL_KEY_ALT_L;
  case XKB_KEY_ISO_Level3_Shift:
  case XKB_KEY_Alt_R:            return PUGL_KEY_ALT_R;
  case XKB_KEY_Super_L:          return PUGL_KEY_SUPER_L;
  case XKB_KEY_Super_R:          return PUGL_KEY_SUPER_R;
  case XKB_KEY_Menu:             return PUGL_KEY_MENU;
  case XKB_KEY_Caps_Lock:        return PUGL_KEY_CAPS_LOCK;
  case XKB_KEY_Scroll_Lock:      return PUGL_KEY_SCROLL_LOCK;
  case XKB_KEY_Num_Lock:         return PUGL_KEY_NUM_LOCK;
  case XKB_KEY_Print:            return PUGL_KEY_PRINT_SCREEN;
  case XKB_KEY_Pause:            return PUGL_KEY_PAUSE;
  default:                       break;
  }
  // clang-format on

  return PUGL_KEY_NONE;
}
