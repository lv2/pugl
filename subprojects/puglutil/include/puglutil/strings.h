// Copyright 2012-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGLUTIL_STRINGS_H
#define PUGLUTIL_STRINGS_H

#include <pugl/pugl.h>

#include <stdint.h>

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

static inline const char*
viewStyleFlagString(const PuglViewStyleFlag state)
{
  switch (state) {
  case PUGL_VIEW_STYLE_MODAL:
    return "modal";
  case PUGL_VIEW_STYLE_TALL:
    return "tall";
  case PUGL_VIEW_STYLE_WIDE:
    return "wide";
  case PUGL_VIEW_STYLE_HIDDEN:
    return "hidden";
  case PUGL_VIEW_STYLE_FULLSCREEN:
    return "fullscreen";
  case PUGL_VIEW_STYLE_ABOVE:
    return "above";
  case PUGL_VIEW_STYLE_BELOW:
    return "below";
  case PUGL_VIEW_STYLE_DEMANDING:
    return "demanding";
  case PUGL_VIEW_STYLE_RESIZING:
    return "resizing";
  case PUGL_VIEW_STYLE_MAPPED:
    return "mapped";
  }

  return "unknown";
}

static inline const char*
keyString(const uint32_t key)
{
  switch (key) {
  case PUGL_KEY_BACKSPACE:
    return "BACKSPACE";
  case PUGL_KEY_TAB:
    return "TAB";
  case PUGL_KEY_ENTER:
    return "ENTER";
  case PUGL_KEY_ESCAPE:
    return "ESCAPE";
  case PUGL_KEY_DELETE:
    return "DELETE";
  case PUGL_KEY_SPACE:
    return "SPACE";

  case PUGL_KEY_F1:
    return "F1";
  case PUGL_KEY_F2:
    return "F2";
  case PUGL_KEY_F3:
    return "F3";
  case PUGL_KEY_F4:
    return "F4";
  case PUGL_KEY_F5:
    return "F5";
  case PUGL_KEY_F6:
    return "F6";
  case PUGL_KEY_F7:
    return "F7";
  case PUGL_KEY_F8:
    return "F8";
  case PUGL_KEY_F9:
    return "F9";
  case PUGL_KEY_F10:
    return "F10";
  case PUGL_KEY_F11:
    return "F11";
  case PUGL_KEY_F12:
    return "F12";

  case PUGL_KEY_PAGE_UP:
    return "PAGE_UP";
  case PUGL_KEY_PAGE_DOWN:
    return "PAGE_DOWN";
  case PUGL_KEY_END:
    return "END";
  case PUGL_KEY_HOME:
    return "HOME";
  case PUGL_KEY_LEFT:
    return "LEFT";
  case PUGL_KEY_UP:
    return "UP";
  case PUGL_KEY_RIGHT:
    return "RIGHT";
  case PUGL_KEY_DOWN:
    return "DOWN";

  case PUGL_KEY_PRINT_SCREEN:
    return "PRINT_SCREEN";
  case PUGL_KEY_INSERT:
    return "INSERT";
  case PUGL_KEY_PAUSE:
    return "PAUSE";
  case PUGL_KEY_MENU:
    return "MENU";
  case PUGL_KEY_NUM_LOCK:
    return "NUM_LOCK";
  case PUGL_KEY_SCROLL_LOCK:
    return "SCROLL_LOCK";
  case PUGL_KEY_CAPS_LOCK:
    return "CAPS_LOCK";

  case PUGL_KEY_SHIFT_L:
    return "SHIFT_L";
  case PUGL_KEY_SHIFT_R:
    return "SHIFT_R";
  case PUGL_KEY_CTRL_L:
    return "CTRL_L";
  case PUGL_KEY_CTRL_R:
    return "CTRL_R";
  case PUGL_KEY_ALT_L:
    return "ALT_L";
  case PUGL_KEY_ALT_R:
    return "ALT_R";
  case PUGL_KEY_SUPER_L:
    return "SUPER_L";
  case PUGL_KEY_SUPER_R:
    return "SUPER_R";

  case PUGL_KEY_PAD_0:
    return "PAD_0";
  case PUGL_KEY_PAD_1:
    return "PAD_1";
  case PUGL_KEY_PAD_2:
    return "PAD_2";
  case PUGL_KEY_PAD_3:
    return "PAD_3";
  case PUGL_KEY_PAD_4:
    return "PAD_4";
  case PUGL_KEY_PAD_5:
    return "PAD_5";
  case PUGL_KEY_PAD_6:
    return "PAD_6";
  case PUGL_KEY_PAD_7:
    return "PAD_7";
  case PUGL_KEY_PAD_8:
    return "PAD_8";
  case PUGL_KEY_PAD_9:
    return "PAD_9";
  case PUGL_KEY_PAD_ENTER:
    return "PAD_ENTER";

  case PUGL_KEY_PAD_PAGE_UP:
    return "PAD_PAGE_UP";
  case PUGL_KEY_PAD_PAGE_DOWN:
    return "PAD_PAGE_DOWN";
  case PUGL_KEY_PAD_END:
    return "PAD_END";
  case PUGL_KEY_PAD_HOME:
    return "PAD_HOME";
  case PUGL_KEY_PAD_LEFT:
    return "PAD_LEFT";
  case PUGL_KEY_PAD_UP:
    return "PAD_UP";
  case PUGL_KEY_PAD_RIGHT:
    return "PAD_RIGHT";
  case PUGL_KEY_PAD_DOWN:
    return "PAD_DOWN";

  case PUGL_KEY_PAD_CLEAR:
    return "PAD_CLEAR";
  case PUGL_KEY_PAD_INSERT:
    return "PAD_INSERT";
  case PUGL_KEY_PAD_DELETE:
    return "PAD_DELETE";
  case PUGL_KEY_PAD_EQUAL:
    return "PAD_EQUAL";

  case PUGL_KEY_PAD_MULTIPLY:
    return "PAD_MULTIPLY";
  case PUGL_KEY_PAD_ADD:
    return "PAD_ADD";
  case PUGL_KEY_PAD_SEPARATOR:
    return "PAD_SEPARATOR";
  case PUGL_KEY_PAD_SUBTRACT:
    return "PAD_SUBTRACT";
  case PUGL_KEY_PAD_DECIMAL:
    return "PAD_DECIMAL";
  case PUGL_KEY_PAD_DIVIDE:
    return "PAD_DIVIDE";
  default:
    break;
  }

  return "";
}

static inline const char*
viewHintString(const PuglViewHint hint)
{
  switch (hint) {
  case PUGL_CONTEXT_API:
    return "Context API";
  case PUGL_CONTEXT_VERSION_MAJOR:
    return "Context major version";
  case PUGL_CONTEXT_VERSION_MINOR:
    return "Context minor version";
  case PUGL_CONTEXT_PROFILE:
    return "Context profile";
  case PUGL_CONTEXT_DEBUG:
    return "Context debug";
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
  case PUGL_SAMPLE_BUFFERS:
    return "Sample buffers";
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
  case PUGL_VIEW_TYPE:
    return "View type";
  case PUGL_DARK_FRAME:
    return "Dark frame";
  case PUGL_ACCEPT_DROP:
    return "Accept drop";
  case PUGL_CONTINUOUS:
    return "Continuous";
  }

  return "Unknown";
}

#endif // PUGLUTIL_STRINGS_H
