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
    return "NORMAL";
  case PUGL_CROSSING_GRAB:
    return "GRAB";
  case PUGL_CROSSING_UNGRAB:
    return "UNGRAB";
  }

  return "?";
}

static inline const char*
scrollDirectionString(const PuglScrollDirection direction)
{
  switch (direction) {
  case PUGL_SCROLL_UP:
    return "UP";
  case PUGL_SCROLL_DOWN:
    return "DOWN";
  case PUGL_SCROLL_LEFT:
    return "LEFT";
  case PUGL_SCROLL_RIGHT:
    return "RIGHT";
  case PUGL_SCROLL_SMOOTH:
    return "SMOOTH";
  }

  return "?";
}

static inline const char*
viewStyleFlagString(const PuglViewStyleFlag state)
{
  switch (state) {
  case PUGL_VIEW_STYLE_MODAL:
    return "MODAL";
  case PUGL_VIEW_STYLE_TALL:
    return "TALL";
  case PUGL_VIEW_STYLE_WIDE:
    return "WIDE";
  case PUGL_VIEW_STYLE_HIDDEN:
    return "HIDDEN";
  case PUGL_VIEW_STYLE_FULLSCREEN:
    return "FULLSCREEN";
  case PUGL_VIEW_STYLE_ABOVE:
    return "ABOVE";
  case PUGL_VIEW_STYLE_BELOW:
    return "BELOW";
  case PUGL_VIEW_STYLE_DEMANDING:
    return "DEMANDING";
  case PUGL_VIEW_STYLE_RESIZING:
    return "RESIZING";
  case PUGL_VIEW_STYLE_MAPPED:
    return "MAPPED";
  }

  return "?";
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

  return "?";
}

static inline const char*
viewHintString(const PuglViewHint hint)
{
  switch (hint) {
  case PUGL_CONTEXT_API:
    return "CONTEXT_API";
  case PUGL_CONTEXT_VERSION_MAJOR:
    return "CONTEXT_MAJOR_VERSION";
  case PUGL_CONTEXT_VERSION_MINOR:
    return "CONTEXT_MINOR_VERSION";
  case PUGL_CONTEXT_PROFILE:
    return "CONTEXT_PROFILE";
  case PUGL_CONTEXT_DEBUG:
    return "CONTEXT_DEBUG";
  case PUGL_RED_BITS:
    return "RED_BITS";
  case PUGL_GREEN_BITS:
    return "GREEN_BITS";
  case PUGL_BLUE_BITS:
    return "BLUE_BITS";
  case PUGL_ALPHA_BITS:
    return "ALPHA_BITS";
  case PUGL_DEPTH_BITS:
    return "DEPTH_BITS";
  case PUGL_STENCIL_BITS:
    return "STENCIL_BITS";
  case PUGL_SAMPLE_BUFFERS:
    return "SAMPLE_BUFFERS";
  case PUGL_SAMPLES:
    return "SAMPLES";
  case PUGL_DOUBLE_BUFFER:
    return "DOUBLE_BUFFER";
  case PUGL_SWAP_INTERVAL:
    return "SWAP_INTERVAL";
  case PUGL_RESIZABLE:
    return "RESIZABLE";
  case PUGL_IGNORE_KEY_REPEAT:
    return "IGNORE_KEY_REPEAT";
  case PUGL_REFRESH_RATE:
    return "REFRESH_RATE";
  case PUGL_VIEW_TYPE:
    return "VIEW_TYPE";
  case PUGL_DARK_FRAME:
    return "DARK_FRAME";
  case PUGL_ACCEPT_DROP:
    return "ACCEPT_DROP";
  case PUGL_CONTINUOUS:
    return "CONTINUOUS";
  }

  return "?";
}

static inline const char*
puglEventTypeName(const PuglEventType type)
{
  switch (type) {
  case PUGL_NOTHING:
    return "NOTHING";
  case PUGL_REALIZE:
    return "REALIZE";
  case PUGL_UNREALIZE:
    return "UNREALIZE";
  case PUGL_CONFIGURE:
    return "CONFIGURE";
  case PUGL_UPDATE:
    return "UPDATE";
  case PUGL_BLANK:
    return "BLANK";
  case PUGL_DAMAGE:
    return "DAMAGE";
  case PUGL_EXPOSE:
    return "EXPOSE";
  case PUGL_FLUSH:
    return "FLUSH";
  case PUGL_CLOSE:
    return "CLOSE";
  case PUGL_FOCUS_IN:
    return "FOCUS_IN";
  case PUGL_FOCUS_OUT:
    return "FOCUS_OUT";
  case PUGL_KEY_PRESS:
    return "KEY_PRESS";
  case PUGL_KEY_RELEASE:
    return "KEY_RELEASE";
  case PUGL_TEXT:
    return "TEXT";
  case PUGL_POINTER_IN:
    return "POINTER_IN";
  case PUGL_POINTER_OUT:
    return "POINTER_OUT";
  case PUGL_BUTTON_PRESS:
    return "BUTTON_PRESS";
  case PUGL_BUTTON_RELEASE:
    return "BUTTON_RELEASE";
  case PUGL_MOTION:
    return "MOTION";
  case PUGL_SCROLL:
    return "SCROLL";
  case PUGL_CLIENT:
    return "CLIENT";
  case PUGL_TIMER:
    return "TIMER";
  case PUGL_LOOP_ENTER:
    return "LOOP_ENTER";
  case PUGL_LOOP_LEAVE:
    return "LOOP_LEAVE";
  case PUGL_DATA_OFFER:
    return "DATA_OFFER";
  case PUGL_DATA:
    return "DATA";
  }

  return "?";
}

#endif // PUGLUTIL_STRINGS_H
