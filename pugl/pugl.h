/*
  Copyright 2012-2016 David Robillard <http://drobilla.net>

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
   @file pugl.h API for Pugl, a minimal portable API for OpenGL.
*/

#ifndef PUGL_H_INCLUDED
#define PUGL_H_INCLUDED

#include <stdint.h>

#ifdef PUGL_SHARED
#    ifdef _WIN32
#        define PUGL_LIB_IMPORT __declspec(dllimport)
#        define PUGL_LIB_EXPORT __declspec(dllexport)
#    else
#        define PUGL_LIB_IMPORT __attribute__((visibility("default")))
#        define PUGL_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef PUGL_INTERNAL
#        define PUGL_API PUGL_LIB_EXPORT
#    else
#        define PUGL_API PUGL_LIB_IMPORT
#    endif
#else
#    define PUGL_API
#endif

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

/**
   @defgroup pugl Pugl
   A minimal portable API for OpenGL.
   @{
*/

/**
   A Pugl view.
*/
typedef struct PuglViewImpl PuglView;

/**
   A native window handle.

   On X11, this is a Window.
   On OSX, this is an NSView*.
   On Windows, this is a HWND.
*/
typedef intptr_t PuglNativeWindow;

/**
   Handle for opaque user data.
*/
typedef void* PuglHandle;

/**
   Return status code.
*/
typedef enum {
	PUGL_SUCCESS = 0
} PuglStatus;

/**
   Drawing context type.
*/
typedef enum {
	PUGL_GL       = 0x1,
	PUGL_CAIRO    = 0x2,
	PUGL_CAIRO_GL = 0x3
} PuglContextType;

/**
   Convenience symbols for ASCII control characters.
*/
typedef enum {
	PUGL_CHAR_BACKSPACE = 0x08,
	PUGL_CHAR_ESCAPE    = 0x1B,
	PUGL_CHAR_DELETE    = 0x7F
} PuglChar;

/**
   Keyboard modifier flags.
*/
typedef enum {
	PUGL_MOD_SHIFT = 1,       /**< Shift key */
	PUGL_MOD_CTRL  = 1 << 1,  /**< Control key */
	PUGL_MOD_ALT   = 1 << 2,  /**< Alt/Option key */
	PUGL_MOD_SUPER = 1 << 3   /**< Mod4/Command/Windows key */
} PuglMod;

/**
   Special (non-Unicode) keyboard keys.

   The numerical values of these symbols occupy a reserved range of Unicode
   points, so it is possible to express either a PuglKey value or a Unicode
   character in the same variable.  This is sometimes useful for interfacing
   with APIs that do not make this distinction.
*/
typedef enum {
	PUGL_KEY_F1 = 0xE000,
	PUGL_KEY_F2,
	PUGL_KEY_F3,
	PUGL_KEY_F4,
	PUGL_KEY_F5,
	PUGL_KEY_F6,
	PUGL_KEY_F7,
	PUGL_KEY_F8,
	PUGL_KEY_F9,
	PUGL_KEY_F10,
	PUGL_KEY_F11,
	PUGL_KEY_F12,
	PUGL_KEY_LEFT,
	PUGL_KEY_UP,
	PUGL_KEY_RIGHT,
	PUGL_KEY_DOWN,
	PUGL_KEY_PAGE_UP,
	PUGL_KEY_PAGE_DOWN,
	PUGL_KEY_HOME,
	PUGL_KEY_END,
	PUGL_KEY_INSERT,
	PUGL_KEY_SHIFT,
	PUGL_KEY_CTRL,
	PUGL_KEY_ALT,
	PUGL_KEY_SUPER
} PuglKey;

/**
   The type of a PuglEvent.
*/
typedef enum {
	PUGL_NOTHING,              /**< No event */
	PUGL_BUTTON_PRESS,         /**< Mouse button press */
	PUGL_BUTTON_RELEASE,       /**< Mouse button release */
	PUGL_CONFIGURE,            /**< View moved and/or resized */
	PUGL_EXPOSE,               /**< View exposed, redraw required */
	PUGL_CLOSE,                /**< Close view */
	PUGL_KEY_PRESS,            /**< Key press */
	PUGL_KEY_RELEASE,          /**< Key release */
	PUGL_ENTER_NOTIFY,         /**< Pointer entered view */
	PUGL_LEAVE_NOTIFY,         /**< Pointer left view */
	PUGL_MOTION_NOTIFY,        /**< Pointer motion */
	PUGL_SCROLL,               /**< Scroll */
	PUGL_FOCUS_IN,             /**< Keyboard focus entered view */
	PUGL_FOCUS_OUT             /**< Keyboard focus left view */
} PuglEventType;

typedef enum {
	PUGL_IS_SEND_EVENT = 1
} PuglEventFlag;

/**
   Reason for a PuglEventCrossing.
*/
typedef enum {
	PUGL_CROSSING_NORMAL,      /**< Crossing due to pointer motion. */
	PUGL_CROSSING_GRAB,        /**< Crossing due to a grab. */
	PUGL_CROSSING_UNGRAB       /**< Crossing due to a grab release. */
} PuglCrossingMode;

/**
   Common header for all event structs.
*/
typedef struct {
	PuglEventType type;        /**< Event type. */
	PuglView*     view;        /**< View that received this event. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
} PuglEventAny;

/**
   Button press or release event.

   For event types PUGL_BUTTON_PRESS and PUGL_BUTTON_RELEASE.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_BUTTON_PRESS or PUGL_BUTTON_RELEASE. */
	PuglView*     view;        /**< View that received this event. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
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
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
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
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        width;       /**< Width of exposed region. */
	double        height;      /**< Height of exposed region. */
	int           count;       /**< Number of expose events to follow. */
} PuglEventExpose;

/**
   Window close event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_CLOSE. */
	PuglView*     view;        /**< View that received this event. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
} PuglEventClose;

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
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
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
	PuglEventType    type;     /**< PUGL_ENTER_NOTIFY or PUGL_LEAVE_NOTIFY. */
	PuglView*        view;     /**< View that received this event. */
	uint32_t         flags;    /**< Bitwise OR of PuglEventFlag values. */
	uint32_t         time;     /**< Time in milliseconds. */
	double           x;        /**< View-relative X coordinate. */
	double           y;        /**< View-relative Y coordinate. */
	double           x_root;   /**< Root-relative X coordinate. */
	double           y_root;   /**< Root-relative Y coordinate. */
	unsigned         state;    /**< Bitwise OR of PuglMod flags. */
	PuglCrossingMode mode;     /**< Reason for crossing. */
} PuglEventCrossing;

/**
   Pointer motion event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_MOTION_NOTIFY. */
	PuglView*     view;        /**< View that received this event. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
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
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
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
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
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
	PuglEventExpose    expose;     /**< PUGL_EXPOSE. */
	PuglEventClose     close;      /**< PUGL_CLOSE. */
	PuglEventKey       key;        /**< PUGL_KEY_PRESS, PUGL_KEY_RELEASE. */
	PuglEventCrossing  crossing;   /**< PUGL_ENTER_NOTIFY, PUGL_LEAVE_NOTIFY. */
	PuglEventMotion    motion;     /**< PUGL_MOTION_NOTIFY. */
	PuglEventScroll    scroll;     /**< PUGL_SCROLL. */
	PuglEventFocus     focus;      /**< PUGL_FOCUS_IN, PUGL_FOCUS_OUT. */
} PuglEvent;

/**
   @name Initialization
   Configuration functions which must be called before creating a window.
   @{
*/

/**
   Create a Pugl view.

   To create a window, call the various puglInit* functions as necessary, then
   call puglCreateWindow().

   @param pargc Pointer to argument count (currently unused).
   @param argv  Arguments (currently unused).
   @return A newly created view.
*/
PUGL_API PuglView*
puglInit(int* pargc, char** argv);

/**
   Set the window class name before creating a window.
*/
PUGL_API void
puglInitWindowClass(PuglView* view, const char* name);

/**
   Set the parent window before creating a window (for embedding).
*/
PUGL_API void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent);

/**
   Set the window size before creating a window.
*/
PUGL_API void
puglInitWindowSize(PuglView* view, int width, int height);

/**
   Set the minimum window size before creating a window.
*/
PUGL_API void
puglInitWindowMinSize(PuglView* view, int width, int height);

/**
   Set the window aspect ratio range before creating a window.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.
*/
PUGL_API void
puglInitWindowAspectRatio(PuglView* view,
                          int       min_x,
                          int       min_y,
                          int       max_x,
                          int       max_y);

/**
   Enable or disable resizing before creating a window.
*/
PUGL_API void
puglInitResizable(PuglView* view, bool resizable);

/**
   Set transient parent before creating a window.

   On X11, parent must be a Window.
   On OSX, parent must be an NSView*.
*/
PUGL_API void
puglInitTransientFor(PuglView* view, uintptr_t parent);

/**
   Set the context type before creating a window.
*/
PUGL_API void
puglInitContextType(PuglView* view, PuglContextType type);

/**
   @}
*/

/**
   @name Windows
   Functions for creating and managing a visible window for a view.
   @{
*/

/**
   Create a window with the settings given by the various puglInit functions.

   @return 1 (pugl does not currently support multiple windows).
*/
PUGL_API int
puglCreateWindow(PuglView* view, const char* title);

/**
   Show the current window.
*/
PUGL_API void
puglShowWindow(PuglView* view);

/**
   Hide the current window.
*/
PUGL_API void
puglHideWindow(PuglView* view);

/**
   Return the native window handle.
*/
PUGL_API PuglNativeWindow
puglGetNativeWindow(PuglView* view);

/**
   @}
*/

/**
   Set the handle to be passed to all callbacks.

   This is generally a pointer to a struct which contains all necessary state.
   Everything needed in callbacks should be here, not in static variables.
*/
PUGL_API void
puglSetHandle(PuglView* view, PuglHandle handle);

/**
   Get the handle to be passed to all callbacks.
*/
PUGL_API PuglHandle
puglGetHandle(PuglView* view);

/**
   Return true iff the view is currently visible.
*/
PUGL_API bool
puglGetVisible(PuglView* view);

/**
   Get the current size of the view.
*/
PUGL_API void
puglGetSize(PuglView* view, int* width, int* height);

/**
   @name Context
   Functions for accessing the drawing context.
   @{
*/

/**
   Get the drawing context.

   For PUGL_GL contexts, this is unused and returns NULL.
   For PUGL_CAIRO contexts, this returns a pointer to a cairo_t.
*/
PUGL_API void*
puglGetContext(PuglView* view);


/**
   Enter the drawing context.

   This must be called before any code that accesses the drawing context,
   including any GL functions.  This is only necessary for code that does so
   outside the usual draw callback or handling of an expose event.
*/
PUGL_API void
puglEnterContext(PuglView* view);

/**
   Leave the drawing context.

   This must be called after puglEnterContext and applies the results of the
   drawing code (for example, by swapping buffers).
*/
PUGL_API void
puglLeaveContext(PuglView* view, bool flush);

/**
   @}
*/

/**
   @name Event Handling
   @{
*/

/**
   A function called when an event occurs.
*/
typedef void (*PuglEventFunc)(PuglView* view, const PuglEvent* event);

/**
   Set the function to call when an event occurs.
*/
PUGL_API void
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc);

/**
   Ignore synthetic repeated key events.
*/
PUGL_API void
puglIgnoreKeyRepeat(PuglView* view, bool ignore);

/**
   Grab the input focus.
*/
PUGL_API void
puglGrabFocus(PuglView* view);

/**
   Block and wait for an event to be ready.

   This can be used in a loop to only process events via puglProcessEvents when
   necessary.  This function will block indefinitely if no events are
   available, so is not appropriate for use in programs that need to perform
   regular updates (e.g. animation).
*/
PUGL_API PuglStatus
puglWaitForEvent(PuglView* view);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.  This function does
   not block if no events are pending.
*/
PUGL_API PuglStatus
puglProcessEvents(PuglView* view);

/**
   @}
*/

/**
   Request a redisplay on the next call to puglProcessEvents().
*/
PUGL_API void
puglPostRedisplay(PuglView* view);

/**
   Destroy a GL window.
*/
PUGL_API void
puglDestroy(PuglView* view);

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_H_INCLUDED */
