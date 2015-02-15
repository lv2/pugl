/*
  Copyright 2014 David Robillard <http://drobilla.net>

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

/**
   @addtogroup pugl
   @{
*/

#ifndef PUGL_EVENT_H_INCLUDED
#define PUGL_EVENT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

#include "pugl/common.h"

/**
   The type of a PuglEvent.
*/
typedef enum {
	PUGL_BUTTON_PRESS,
	PUGL_BUTTON_RELEASE,
	PUGL_CONFIGURE,
	PUGL_EXPOSE,
	PUGL_KEY_PRESS,
	PUGL_KEY_RELEASE,
	PUGL_ENTER_NOTIFY,
	PUGL_LEAVE_NOTIFY,
	PUGL_MOTION_NOTIFY,
	PUGL_NOTHING,
	PUGL_SCROLL,
	PUGL_FOCUS_IN,
	PUGL_FOCUS_OUT
} PuglEventType;

/**
   Reason for a PuglEventCrossing.
*/
typedef enum {
	PUGL_CROSSING_NORMAL,  /**< Crossing due to pointer motion. */
	PUGL_CROSSING_GRAB,    /**< Crossing due to a grab. */
	PUGL_CROSSING_UNGRAB   /**< Crossing due to a grab release. */
} PuglCrossingMode;

/**
   Common header for all event structs.
*/
typedef struct {
	PuglEventType type;        /**< Event type. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
} PuglEventAny;

/**
   Button press or release event.

   For event types PUGL_BUTTON_PRESS and PUGL_BUTTON_RELEASE.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_BUTTON_PRESS or PUGL_BUTTON_RELEASE. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	uint32_t      time;        /**< Time in milliseconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        x_root;      /**< Root-relative X coordinate. */
	double        y_root;      /**< Root-relative Y coordinate. */
	unsigned      state;       /**< Bitwise OR of PuglMod flags. */
	unsigned      button;      /**< 1-relative button number. */
} PuglEventButton;

/**
   Configure event for when window size or position has changed.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_CONFIGURE. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	double        x;           /**< New parent-relative X coordinate. */
	double        y;           /**< New parent-relative Y coordinate. */
	double        width;       /**< New width. */
	double        height;      /**< New height. */
} PuglEventConfigure;

/**
   Expose event for when a region must be redrawn.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_EXPOSE. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        width;       /**< Width of exposed region. */
	double        height;      /**< Height of exposed region. */
	int           count;       /**< Number of expose events to follow. */
} PuglEventExpose;

/**
   Key press/release event.

   Keys that correspond to a Unicode character have `character` and `utf8` set.
   Other keys will have `character` 0, but `special` may be set if this is a
   known special key.

   A key press may be part of a multi-key sequence to generate a wide
   character.  If `filter` is set, this event is part of a multi-key sequence
   and should be ignored if the application is reading textual input.
   Following the series of filtered press events, a press event with
   `character` and `utf8` (but `keycode` 0) will be sent.  This event will have
   no corresponding release event.

   Generally, an application should either work with raw keyboard press/release
   events based on `keycode` (ignoring events with `keycode` 0), or
   read textual input based on `character` or `utf8` (ignoring releases and
   events with `filter` 1).  Note that blindly appending `utf8` will yield
   incorrect text, since press events are sent for both individually composed
   keys and the resulting synthetic multi-byte press.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_KEY_PRESS or PUGL_KEY_RELEASE. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	uint32_t      time;        /**< Time in milliseconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        x_root;      /**< Root-relative X coordinate. */
	double        y_root;      /**< Root-relative Y coordinate. */
	unsigned      state;       /**< Bitwise OR of PuglMod flags. */
	unsigned      keycode;     /**< Raw key code. */
	uint32_t      character;   /**< Unicode character code, or 0. */
	PuglKey       special;     /**< Special key, or 0. */
	uint8_t       utf8[8];     /**< UTF-8 string. */
	bool          filter;      /**< True if part of a multi-key sequence. */
} PuglEventKey;

/**
   Pointer crossing event (enter and leave).
*/
typedef struct {
	PuglEventType    type;        /**< PUGL_ENTER_NOTIFY or PUGL_LEAVE_NOTIFY. */
	PuglView*        view;        /**< View that received this event. */
	bool             send_event;  /**< True iff event was sent explicitly. */
	uint32_t         time;        /**< Time in milliseconds. */
	double           x;           /**< View-relative X coordinate. */
	double           y;           /**< View-relative Y coordinate. */
	double           x_root;      /**< Root-relative X coordinate. */
	double           y_root;      /**< Root-relative Y coordinate. */
	unsigned         state;       /**< Bitwise OR of PuglMod flags. */
	PuglCrossingMode mode;        /**< Reason for crossing. */
} PuglEventCrossing;

/**
   Pointer motion event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_MOTION_NOTIFY. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	uint32_t      time;        /**< Time in milliseconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        x_root;      /**< Root-relative X coordinate. */
	double        y_root;      /**< Root-relative Y coordinate. */
	unsigned      state;       /**< Bitwise OR of PuglMod flags. */
	bool          is_hint;     /**< True iff this event is a motion hint. */
	bool          focus;       /**< True iff this is the focused window. */
} PuglEventMotion;

/**
   Scroll event.

   The scroll distance is expressed in "lines", an arbitrary unit that
   corresponds to a single tick of a detented mouse wheel.  For example, `dy` =
   1.0 scrolls 1 line up.  Some systems and devices support finer resolution
   and/or higher values for fast scrolls, so programs should handle any value
   gracefully.
 */
typedef struct {
	PuglEventType type;        /**< PUGL_SCROLL. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	uint32_t      time;        /**< Time in milliseconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        x_root;      /**< Root-relative X coordinate. */
	double        y_root;      /**< Root-relative Y coordinate. */
	unsigned      state;       /**< Bitwise OR of PuglMod flags. */
	double        dx;          /**< Scroll X distance in lines. */
	double        dy;          /**< Scroll Y distance in lines. */
} PuglEventScroll;

/**
   Keyboard focus event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_FOCUS_IN or PUGL_FOCUS_OUT. */
	PuglView*     view;        /**< View that received this event. */
	bool          send_event;  /**< True iff event was sent explicitly. */
	bool          grab;        /**< True iff this is a grab/ungrab event. */
} PuglEventFocus;

/**
   Interface event.

   This is a union of all event structs.  The `type` must be checked to
   determine which fields are safe to access.  A pointer to PuglEvent can
   either be cast to the appropriate type, or the union members used.
*/
typedef union {
	PuglEventType      type;       /**< Event type. */
	PuglEventAny       any;        /**< Valid for all event types. */
	PuglEventButton    button;     /**< PUGL_BUTTON_PRESS, PUGL_BUTTON_RELEASE. */
	PuglEventConfigure configure;  /**< PUGL_CONFIGURE. */
	PuglEventCrossing  crossing;   /**< PUGL_ENTER_NOTIFY, PUGL_LEAVE_NOTIFY. */
	PuglEventExpose    expose;     /**< PUGL_EXPOSE. */
	PuglEventKey       key;        /**< PUGL_KEY_PRESS, PUGL_KEY_RELEASE. */
	PuglEventMotion    motion;     /**< PUGL_MOTION_NOTIFY. */
	PuglEventScroll    scroll;     /**< PUGL_SCROLL. */
	PuglEventFocus     focus;      /**< PUGL_FOCUS_IN, PUGL_FOCUS_OUT. */
} PuglEvent;

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_EVENT_H_INCLUDED */
