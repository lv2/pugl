/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

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
   @file pugl.h Pugl API.
*/

#ifndef PUGL_PUGL_H
#define PUGL_PUGL_H

#include <stdbool.h>
#include <stddef.h>
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
#	define PUGL_BEGIN_DECLS extern "C" {
#	define PUGL_END_DECLS }
#else
#	define PUGL_BEGIN_DECLS
#	define PUGL_END_DECLS
#endif

PUGL_BEGIN_DECLS

/**
   @defgroup pugl_api Pugl
   A minimal portable API for embeddable GUIs.
   @{
*/

/**
   A rectangle.

   This is used to describe things like view position and size.  Pugl generally
   uses coordinates where the top left corner is 0,0.
*/
typedef struct {
	double x;
	double y;
	double width;
	double height;
} PuglRect;

/**
   @defgroup events Events

   Event definitions.

   All updates to the view happen via events, which are dispatched to the
   view's #PuglEventFunc by Pugl.  Most events map directly to one from the
   underlying window system, but some are constructed by Pugl itself so there
   is not necessarily a direct correspondence.

   @{
*/

/**
   Keyboard modifier flags.
*/
typedef enum {
	PUGL_MOD_SHIFT = 1,      ///< Shift key
	PUGL_MOD_CTRL  = 1 << 1, ///< Control key
	PUGL_MOD_ALT   = 1 << 2, ///< Alt/Option key
	PUGL_MOD_SUPER = 1 << 3  ///< Mod4/Command/Windows key
} PuglMod;

/**
   Bitwise OR of #PuglMod values.
*/
typedef uint32_t PuglMods;

/**
   Keyboard key codepoints.

   All keys are identified by a Unicode code point in PuglEventKey::key.  This
   enumeration defines constants for special keys that do not have a standard
   code point, and some convenience constants for control characters.  Note
   that all keys are handled in the same way, this enumeration is just for
   convenience when writing hard-coded key bindings.

   Keys that do not have a standard code point use values in the Private Use
   Area in the Basic Multilingual Plane (`U+E000` to `U+F8FF`).  Applications
   must take care to not interpret these values beyond key detection, the
   mapping used here is arbitrary and specific to Pugl.
*/
typedef enum {
	// ASCII control codes
	PUGL_KEY_BACKSPACE = 0x08,
	PUGL_KEY_ESCAPE    = 0x1B,
	PUGL_KEY_DELETE    = 0x7F,

	// Unicode Private Use Area
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
	PUGL_KEY_SHIFT_L = PUGL_KEY_SHIFT,
	PUGL_KEY_SHIFT_R,
	PUGL_KEY_CTRL,
	PUGL_KEY_CTRL_L = PUGL_KEY_CTRL,
	PUGL_KEY_CTRL_R,
	PUGL_KEY_ALT,
	PUGL_KEY_ALT_L = PUGL_KEY_ALT,
	PUGL_KEY_ALT_R,
	PUGL_KEY_SUPER,
	PUGL_KEY_SUPER_L = PUGL_KEY_SUPER,
	PUGL_KEY_SUPER_R,
	PUGL_KEY_MENU,
	PUGL_KEY_CAPS_LOCK,
	PUGL_KEY_SCROLL_LOCK,
	PUGL_KEY_NUM_LOCK,
	PUGL_KEY_PRINT_SCREEN,
	PUGL_KEY_PAUSE
} PuglKey;

/**
   The type of a PuglEvent.
*/
typedef enum {
	PUGL_NOTHING,        ///< No event
	PUGL_BUTTON_PRESS,   ///< Mouse button pressed, a #PuglEventButton
	PUGL_BUTTON_RELEASE, ///< Mouse button released, a #PuglEventButton
	PUGL_CONFIGURE,      ///< View moved and/or resized, a #PuglEventConfigure
	PUGL_EXPOSE,         ///< View must be drawn, a #PuglEventExpose
	PUGL_CLOSE,          ///< View will be closed, a #PuglEventAny
	PUGL_KEY_PRESS,      ///< Key pressed, a #PuglEventKey
	PUGL_KEY_RELEASE,    ///< Key released, a #PuglEventKey
	PUGL_TEXT,           ///< Character entered, a #PuglEventText
	PUGL_ENTER_NOTIFY,   ///< Pointer entered view, a #PuglEventCrossing
	PUGL_LEAVE_NOTIFY,   ///< Pointer left view, a #PuglEventCrossing
	PUGL_MOTION_NOTIFY,  ///< Pointer moved, a #PuglEventMotion
	PUGL_SCROLL,         ///< Scrolled, a #PuglEventScroll
	PUGL_FOCUS_IN,       ///< Keyboard focus entered view, a #PuglEventFocus
	PUGL_FOCUS_OUT,      ///< Keyboard focus left view, a #PuglEventFocus
	PUGL_TIMER           ///< One of the timers a user set triggered, a #PuglEventTimer
} PuglEventType;

/**
   Common flags for all event types.
*/
typedef enum {
	PUGL_IS_SEND_EVENT = 1 ///< Event is synthetic
} PuglEventFlag;

/**
   Bitwise OR of #PuglEventFlag values.
*/
typedef uint32_t PuglEventFlags;

/**
   Reason for a PuglEventCrossing.
*/
typedef enum {
	PUGL_CROSSING_NORMAL, ///< Crossing due to pointer motion
	PUGL_CROSSING_GRAB,   ///< Crossing due to a grab
	PUGL_CROSSING_UNGRAB  ///< Crossing due to a grab release
} PuglCrossingMode;

/**
   Common header for all event structs.
*/
typedef struct {
	PuglEventType  type;  ///< Event type
	PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
} PuglEventAny;

/**
   Button press or release event.
*/
typedef struct {
	PuglEventType  type;   ///< #PUGL_BUTTON_PRESS or #PUGL_BUTTON_RELEASE
	PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
	double         time;   ///< Time in seconds
	double         x;      ///< View-relative X coordinate
	double         y;      ///< View-relative Y coordinate
	double         xRoot;  ///< Root-relative X coordinate
	double         yRoot;  ///< Root-relative Y coordinate
	PuglMods       state;  ///< Bitwise OR of #PuglMod flags
	uint32_t       button; ///< Button number starting from 1
} PuglEventButton;

/**
   Window resize or move event.

   A configure event is sent whenever the window is resized or moved.  When a
   configure event is received, the graphics context is active but not set up
   for drawing.  For example, it is valid to adjust the OpenGL viewport or
   otherwise configure the context, but not to draw anything.
*/
typedef struct {
	PuglEventType  type;   ///< #PUGL_CONFIGURE
	PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
	double         x;      ///< New parent-relative X coordinate
	double         y;      ///< New parent-relative Y coordinate
	double         width;  ///< New width
	double         height; ///< New height
} PuglEventConfigure;

/**
   Expose event for when a region must be redrawn.

   When an expose event is received, the graphics context is active, and the
   view must draw the entire specified region.  The contents of the region are
   undefined, there is no preservation of anything drawn previously.
*/
typedef struct {
	PuglEventType  type;   ///< #PUGL_EXPOSE
	PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
	double         x;      ///< View-relative X coordinate
	double         y;      ///< View-relative Y coordinate
	double         width;  ///< Width of exposed region
	double         height; ///< Height of exposed region
	int            count;  ///< Number of expose events to follow
} PuglEventExpose;

/**
   Key press or release event.

   This event represents low-level key presses and releases.  This can be used
   for "direct" keyboard handing like key bindings, but must not be interpreted
   as text input.

   Keys are represented portably as Unicode code points, using the "natural"
   code point for the key where possible (see #PuglKey for details).  The #key
   field is the code for the pressed key, without any modifiers applied.  For
   example, a press or release of the 'A' key will have #key 97 ('a')
   regardless of whether shift or control are being held.

   Alternatively, the raw #keycode can be used to work directly with physical
   keys, but note that this value is not portable and differs between platforms
   and hardware.
*/
typedef struct {
	PuglEventType  type;    ///< #PUGL_KEY_PRESS or #PUGL_KEY_RELEASE
	PuglEventFlags flags;   ///< Bitwise OR of #PuglEventFlag values
	double         time;    ///< Time in seconds
	double         x;       ///< View-relative X coordinate
	double         y;       ///< View-relative Y coordinate
	double         xRoot;   ///< Root-relative X coordinate
	double         yRoot;   ///< Root-relative Y coordinate
	PuglMods       state;   ///< Bitwise OR of #PuglMod flags
	uint32_t       keycode; ///< Raw key code
	uint32_t       key;     ///< Unshifted Unicode character code, or 0
} PuglEventKey;

/**
   Character input event.

   This event represents text input, usually as the result of a key press.  The
   text is given both as a Unicode character code and a UTF-8 string.

   Note that this event is generated by the platform's input system, so there
   is not necessarily a direct correspondence between text events and physical
   key presses.  For example, with some input methods a sequence of several key
   presses will generate a single character.
*/
typedef struct {
	PuglEventType  type;      ///< #PUGL_TEXT
	PuglEventFlags flags;     ///< Bitwise OR of #PuglEventFlag values
	double         time;      ///< Time in seconds
	double         x;         ///< View-relative X coordinate
	double         y;         ///< View-relative Y coordinate
	double         xRoot;     ///< Root-relative X coordinate
	double         yRoot;     ///< Root-relative Y coordinate
	PuglMods       state;     ///< Bitwise OR of #PuglMod flags
	uint32_t       keycode;   ///< Raw key code
	uint32_t       character; ///< Unicode character code
	char           string[8]; ///< UTF-8 string
} PuglEventText;

/**
   Pointer enter or leave event.

   This event is sent when the pointer enters or leaves the view.  This can
   happen for several reasons (not just the user dragging the pointer over the
   window edge), as described by the #mode field.
*/
typedef struct {
	PuglEventType    type;  ///< #PUGL_ENTER_NOTIFY or #PUGL_LEAVE_NOTIFY
	PuglEventFlags   flags; ///< Bitwise OR of #PuglEventFlag values
	double           time;  ///< Time in seconds
	double           x;     ///< View-relative X coordinate
	double           y;     ///< View-relative Y coordinate
	double           xRoot; ///< Root-relative X coordinate
	double           yRoot; ///< Root-relative Y coordinate
	PuglMods         state; ///< Bitwise OR of #PuglMod flags
	PuglCrossingMode mode;  ///< Reason for crossing
} PuglEventCrossing;

/**
   Pointer motion event.
*/
typedef struct {
	PuglEventType  type;   ///< #PUGL_MOTION_NOTIFY
	PuglEventFlags flags;  ///< Bitwise OR of #PuglEventFlag values
	double         time;   ///< Time in seconds
	double         x;      ///< View-relative X coordinate
	double         y;      ///< View-relative Y coordinate
	double         xRoot;  ///< Root-relative X coordinate
	double         yRoot;  ///< Root-relative Y coordinate
	PuglMods       state;  ///< Bitwise OR of #PuglMod flags
	bool           isHint; ///< True iff this event is a motion hint
	bool           focus;  ///< True iff this is the focused window
} PuglEventMotion;

/**
   Scroll event.

   The scroll distance is expressed in "lines", an arbitrary unit that
   corresponds to a single tick of a detented mouse wheel.  For example, #dy =
   1.0 scrolls 1 line up.  Some systems and devices support finer resolution
   and/or higher values for fast scrolls, so programs should handle any value
   gracefully.
*/
typedef struct {
	PuglEventType  type;  ///< #PUGL_SCROLL
	PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
	double         time;  ///< Time in seconds
	double         x;     ///< View-relative X coordinate
	double         y;     ///< View-relative Y coordinate
	double         xRoot; ///< Root-relative X coordinate
	double         yRoot; ///< Root-relative Y coordinate
	PuglMods       state; ///< Bitwise OR of #PuglMod flags
	double         dx;    ///< Scroll X distance in lines
	double         dy;    ///< Scroll Y distance in lines
} PuglEventScroll;

/**
   Keyboard focus event.

   This event is sent whenever the view gains or loses the keyboard focus.  The
   view with the keyboard focus will receive any key press or release events.
*/
typedef struct {
	PuglEventType  type;  ///< #PUGL_FOCUS_IN or #PUGL_FOCUS_OUT
	PuglEventFlags flags; ///< Bitwise OR of #PuglEventFlag values
	bool           grab;  ///< True iff this is a grab/ungrab event
} PuglEventFocus;

/**
	Timer event.
	
	This event is triggered when a timer set by the user triggers.
*/
typedef struct {
	PuglEventType type; ///< #PUGL_TIMER
	uint64_t id;        ///< The ID the user gave to the timer event
} PuglEventTimer;

/**
   View event.

   This is a union of all event types.  The #type must be checked to determine
   which fields are safe to access.  A pointer to PuglEvent can either be cast
   to the appropriate type, or the union members used.
*/
typedef union {
	PuglEventAny       any;       ///< Valid for all event types
	PuglEventType      type;      ///< Event type
	PuglEventButton    button;    ///< #PUGL_BUTTON_PRESS, #PUGL_BUTTON_RELEASE
	PuglEventConfigure configure; ///< #PUGL_CONFIGURE
	PuglEventExpose    expose;    ///< #PUGL_EXPOSE
	PuglEventKey       key;       ///< #PUGL_KEY_PRESS, #PUGL_KEY_RELEASE
	PuglEventText      text;      ///< #PUGL_TEXT
	PuglEventCrossing  crossing;  ///< #PUGL_ENTER_NOTIFY, #PUGL_LEAVE_NOTIFY
	PuglEventMotion    motion;    ///< #PUGL_MOTION_NOTIFY
	PuglEventScroll    scroll;    ///< #PUGL_SCROLL
	PuglEventFocus     focus;     ///< #PUGL_FOCUS_IN, #PUGL_FOCUS_OUT
	PuglEventTimer     timer;     ///< #PUGL_TIMER
} PuglEvent;

/**
   @}
   @defgroup status Status

   Status codes and error handling.

   @{
*/

/**
   Return status code.
*/
typedef enum {
	PUGL_SUCCESS,               ///< Success
	PUGL_FAILURE,               ///< Non-fatal failure
	PUGL_UNKNOWN_ERROR,         ///< Unknown system error
	PUGL_BAD_BACKEND,           ///< Invalid or missing backend
	PUGL_BACKEND_FAILED,        ///< Backend initialisation failed
	PUGL_REGISTRATION_FAILED,   ///< Window class registration failed
	PUGL_CREATE_WINDOW_FAILED,  ///< Window creation failed
	PUGL_SET_FORMAT_FAILED,     ///< Failed to set pixel format
	PUGL_CREATE_CONTEXT_FAILED, ///< Failed to create drawing context
	PUGL_UNSUPPORTED_TYPE,      ///< Unsupported data type
} PuglStatus;

/**
   Return a string describing a status code.
*/
PUGL_API
const char*
puglStrerror(PuglStatus status);

/**
   @}
   @defgroup world World

   The top-level context of a Pugl application or plugin.

   The world contains all library-wide state.  There is no static data in Pugl,
   so it is safe to use multiple worlds in a single process.  This is to
   facilitate plugins or other situations where it is not possible to share a
   world, but a single world should be shared for all views where possible.

   @{
*/

/**
   The "world" of application state.

   The world represents everything that is not associated with a particular
   view.  Several worlds can be created in a single process, but code using
   different worlds must be isolated so they are never mixed.  Views are
   strongly associated with the world they were created in.
*/
typedef struct PuglWorldImpl PuglWorld;

/**
   Handle for the world's opaque user data.
*/
typedef void* PuglWorldHandle;

/**
   Create a new world.

   @return A new world, which must be later freed with puglFreeWorld().
*/
PUGL_API PuglWorld*
puglNewWorld(void);

/**
   Free a world allocated with puglNewWorld().
*/
PUGL_API void
puglFreeWorld(PuglWorld* world);

/**
   Set the user data for the world.

   This is usually a pointer to a struct that contains all the state which must
   be accessed by several views.

   The handle is opaque to Pugl and is not interpreted in any way.
*/
PUGL_API void
puglSetWorldHandle(PuglWorld* world, PuglWorldHandle handle);

/**
   Get the user data for the world.
*/
PUGL_API PuglWorldHandle
puglGetWorldHandle(PuglWorld* world);

/**
   Return a pointer to the native handle of the world.

   @return
   - X11: A pointer to the `Display`.
   - MacOS: `NULL`.
   - Windows: The `HMODULE` of the calling process.
*/
PUGL_API void*
puglGetNativeWorld(PuglWorld* world);

/**
   Set the class name of the application.

   This is a stable identifier for the application, used as the window
   class/instance name on X11 and Windows.  It is not displayed to the user,
   but can be used in scripts and by window managers, so it should be the same
   for every instance of the application, but different from other
   applications.
*/
PUGL_API PuglStatus
puglSetClassName(PuglWorld* world, const char* name);

/**
   Return the time in seconds.

   This is a monotonically increasing clock with high resolution.  The returned
   time is only useful to compare against other times returned by this
   function, its absolute value has no meaning.
*/
PUGL_API double
puglGetTime(const PuglWorld* world);

/**
   Poll for events that are ready to be processed.

   This polls for events that are ready for any view in the world, potentially
   blocking depending on `timeout`.

   @param world The world to poll for events.

   @param timeout Maximum time to wait, in seconds.  If zero, the call returns
   immediately, if negative, the call blocks indefinitely.

   @return #PUGL_SUCCESS if events are read, #PUGL_FAILURE if not, or an error.
*/
PUGL_API PuglStatus
puglPollEvents(PuglWorld* world, double timeout);

/**
   Dispatch any pending events to views.

   This processes all pending events, dispatching them to the appropriate
   views.  View event handlers will be called in the scope of this call.  This
   function does not block, if no events are pending then it will return
   immediately.
*/
PUGL_API PuglStatus
puglDispatchEvents(PuglWorld* world);

/**
   @}

   @defgroup view View

   A drawable region that receives events.

   A view can be thought of as a window, but does not necessarily correspond to
   a top-level window in a desktop environment.  For example, a view can be
   embedded in some other window, or represent an embedded system where there
   is no concept of multiple windows at all.

   @{
*/

/**
   A drawable region that receives events.
*/
typedef struct PuglViewImpl PuglView;

/**
   A graphics backend.

   The backend dictates how graphics are set up for a view, and how drawing is
   performed.  A backend must be set by calling puglSetBackend() before
   realising a view.

   If you are using a local copy of Pugl, it is possible to implement a custom
   backend.  See the definition of `PuglBackendImpl` in the source code for
   details.
*/
typedef struct PuglBackendImpl PuglBackend;

/**
   A native window handle.

   X11: This is a `Window`.

   MacOS: This is a pointer to an `NSView*`.

   Windows: This is a `HWND`.
*/
typedef uintptr_t PuglNativeWindow;

/**
   Handle for a view's opaque user data.
*/
typedef void* PuglHandle;

/**
   A hint for configuring a view.
*/
typedef enum {
	PUGL_USE_COMPAT_PROFILE,    ///< Use compatible (not core) OpenGL profile
	PUGL_USE_DEBUG_CONTEXT,     ///< True to use a debug OpenGL context
	PUGL_CONTEXT_VERSION_MAJOR, ///< OpenGL context major version
	PUGL_CONTEXT_VERSION_MINOR, ///< OpenGL context minor version
	PUGL_RED_BITS,              ///< Number of bits for red channel
	PUGL_GREEN_BITS,            ///< Number of bits for green channel
	PUGL_BLUE_BITS,             ///< Number of bits for blue channel
	PUGL_ALPHA_BITS,            ///< Number of bits for alpha channel
	PUGL_DEPTH_BITS,            ///< Number of bits for depth buffer
	PUGL_STENCIL_BITS,          ///< Number of bits for stencil buffer
	PUGL_SAMPLES,               ///< Number of samples per pixel (AA)
	PUGL_DOUBLE_BUFFER,         ///< True if double buffering should be used
	PUGL_SWAP_INTERVAL,         ///< Number of frames between buffer swaps
	PUGL_RESIZABLE,             ///< True if window should be resizable
	PUGL_IGNORE_KEY_REPEAT,     ///< True if key repeat events are ignored

	PUGL_NUM_WINDOW_HINTS
} PuglViewHint;

/**
   A special view hint value.
*/
typedef enum {
	PUGL_DONT_CARE = -1, ///< Use best available value
	PUGL_FALSE     = 0,  ///< Explicitly false
	PUGL_TRUE      = 1   ///< Explicitly true
} PuglViewHintValue;

/**
   A function called when an event occurs.
*/
typedef PuglStatus (*PuglEventFunc)(PuglView* view, const PuglEvent* event);

/**
   @name Setup
   Functions for creating and destroying a view.
   @{
*/

/**
   Create a new view.

   A newly created view does not correspond to a real system view or window.
   It must first be configured, then be realised by calling puglCreateWindow().
*/
PUGL_API PuglView*
puglNewView(PuglWorld* world);

/**
   Free a view created with puglNewView().
*/
PUGL_API void
puglFreeView(PuglView* view);

/**
   Return the world that `view` is a part of.
*/
PUGL_API PuglWorld*
puglGetWorld(PuglView* view);

/**
   Set the user data for a view.

   This is usually a pointer to a struct that contains all the state which must
   be accessed by a view.  Everything needed to process events should be stored
   here, not in static variables.

   The handle is opaque to Pugl and is not interpreted in any way.
*/
PUGL_API void
puglSetHandle(PuglView* view, PuglHandle handle);

/**
   Get the user data for a view.
*/
PUGL_API PuglHandle
puglGetHandle(PuglView* view);

/**
   Set the graphics backend to use for a view.

   This must be called once to set the graphics backend before calling
   puglCreateWindow().

   Pugl includes the following backends:

   - puglGlBackend(), declared in pugl_gl.h
   - puglCairoBackend(), declared in pugl_cairo.h

   Note that backends are modular and not compiled into the main Pugl library
   to avoid unnecessary dependencies.  To use a particular backend,
   applications must link against the appropriate backend library, or be sure
   to compile in the appropriate code if using a local copy of Pugl.
*/
PUGL_API PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend);

/**
   Set the function to call when an event occurs.
*/
PUGL_API PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc);

/**
   Set a hint to configure window properties.

   This only has an effect when called before puglCreateWindow().
*/
PUGL_API PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value);

/**
   @}
   @anchor frame
   @name Frame
   Functions for working with the position and size of a view.
   @{
*/

/**
   Get the current position and size of the view.

   The position is in screen coordinates with an upper left origin.
*/
PUGL_API PuglRect
puglGetFrame(const PuglView* view);

/**
   Set the current position and size of the view.

   The position is in screen coordinates with an upper left origin.
*/
PUGL_API PuglStatus
puglSetFrame(PuglView* view, PuglRect frame);

/**
   Set the minimum size of the view.

   If an initial minimum size is known, this should be called before
   puglCreateWindow() to avoid stutter, though it can be called afterwards as
   well.
*/
PUGL_API PuglStatus
puglSetMinSize(PuglView* view, int width, int height);

/**
   Set the window aspect ratio range.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.

   Note that setting different minimum and maximum constraints does not
   currenty work on MacOS (the minimum is used), so only setting a fixed aspect
   ratio works properly across all platforms.

   If an initial aspect ratio is known, this should be called before
   puglCreateWindow() to avoid stutter, though it can be called afterwards as
   well.
*/
PUGL_API PuglStatus
puglSetAspectRatio(PuglView* view, int minX, int minY, int maxX, int maxY);

/**
   @}
   @name Windows
   Functions for working with top-level windows.
   @{
*/

/**
   Set the title of the window.

   This only makes sense for non-embedded views that will have a corresponding
   top-level window, and sets the title, typically displayed in the title bar
   or in window switchers.
*/
PUGL_API PuglStatus
puglSetWindowTitle(PuglView* view, const char* title);

/**
   Set the parent window for embedding a view in an existing window.

   This must be called before puglCreateWindow(), reparenting is not supported.
*/
PUGL_API PuglStatus
puglSetParentWindow(PuglView* view, PuglNativeWindow parent);

/**
   Set the transient parent of the window.

   Set this for transient children like dialogs, to have them properly
   associated with their parent window.  This should be called before
   puglCreateWindow().
*/
PUGL_API PuglStatus
puglSetTransientFor(PuglView* view, PuglNativeWindow parent);

/**
   Realise a view by creating a corresponding system view or window.

   The view should be fully configured using the above functions before this is
   called.  This function may only be called once per view.
*/
PUGL_API PuglStatus
puglCreateWindow(PuglView* view, const char* title);

/**
   Show the current window.

   If the window is currently hidden, it will be shown and possibly raised to
   the top depending on the platform.
*/
PUGL_API PuglStatus
puglShowWindow(PuglView* view);

/**
   Hide the current window.
*/
PUGL_API PuglStatus
puglHideWindow(PuglView* view);

/**
   Return true iff the view is currently visible.
*/
PUGL_API bool
puglGetVisible(PuglView* view);

/**
   Return the native window handle.
*/
PUGL_API PuglNativeWindow
puglGetNativeWindow(PuglView* view);

/**
   @}
   @name Graphics
   Functions for working with the graphics context and scheduling redisplays.
   @{
*/

/**
   Get the graphics context.

   This is a backend-specific context used for drawing if the backend graphics
   API requires one.  It is only available during an expose.

   @return
   - OpenGL: `NULL`.
   - Cairo: A pointer to a
     [`cairo_t`](http://www.cairographics.org/manual/cairo-cairo-t.html).
*/
PUGL_API void*
puglGetContext(PuglView* view);

/**
   Enter the graphics context.

   Note that, unlike some similar libraries, Pugl automatically enters and
   leaves the graphics context when required and application should not
   normally do this.  Drawing in Pugl is only allowed during the processing of
   an expose event.

   However, this can be used to enter the graphics context elsewhere, for
   example to call any GL functions during setup.

   @param view The view being entered.
   @param drawing If true, prepare for drawing.
*/
PUGL_API PuglStatus
puglEnterContext(PuglView* view, bool drawing);

/**
   Leave the graphics context.

   This must be called after puglEnterContext() with a matching `drawing`
   parameter.

   @param view The view being left.
   @param drawing If true, finish drawing, for example by swapping buffers.
*/
PUGL_API PuglStatus
puglLeaveContext(PuglView* view, bool drawing);

/**
   Request a redisplay for the entire view.

   This will cause an expose event to be dispatched later.  If called from
   within the event handler, the expose should arrive at the end of the current
   event loop iteration, though this is not strictly guaranteed on all
   platforms.  If called elsewhere, an expose will be enqueued to be processed
   in the next event loop iteration.
*/
PUGL_API PuglStatus
puglPostRedisplay(PuglView* view);

/**
   Request a redisplay of the given rectangle within the view.

   This has the same semantics as puglPostRedisplay(), but allows giving a
   precise region for redrawing only a portion of the view.
*/
PUGL_API PuglStatus
puglPostRedisplayRect(PuglView* view, PuglRect rect);

/**
   @}
   @anchor interaction
   @name Interaction
   Functions for interacting with the user.
   @{
*/

/**
   Grab the keyboard input focus.
*/
PUGL_API PuglStatus
puglGrabFocus(PuglView* view);

/**
   Return whether `view` has the keyboard input focus.
*/
PUGL_API bool
puglHasFocus(const PuglView* view);

/**
   Set the clipboard contents.

   This sets the system clipboard contents, which can be retrieved with
   puglGetClipboard() or pasted into other applications.

   @param view The view.
   @param type The MIME type of the data, "text/plain" is assumed if `NULL`.
   @param data The data to copy to the clipboard.
   @param len The length of data in bytes (including terminator if necessary).
*/
PUGL_API PuglStatus
puglSetClipboard(PuglView*   view,
                 const char* type,
                 const void* data,
                 size_t      len);

/**
   Get the clipboard contents.

   This gets the system clipboard contents, which may have been set with
   puglSetClipboard() or copied from another application.

   @param view The view.
   @param[out] type Set to the MIME type of the data.
   @param[out] len Set to the length of the data in bytes.
   @return The clipboard contents.
*/
PUGL_API const void*
puglGetClipboard(PuglView* view, const char** type, size_t* len);

/**
   Request user attention.

   This hints to the system that the window or application requires attention
   from the user.  The exact effect depends on the platform, but is usually
   something like a flashing task bar entry or bouncing application icon.
*/
PUGL_API PuglStatus
puglRequestAttention(PuglView* view);

/**
  Register a new timer, which will send timer events to the given view at rate times per second.
  Two timers should not be registered on the same view with the same ID, or PUGL_FAILURE will be returned. Two timers on different views with the same ID are fine, and two timers with different IDs on the same view are fine.
*/
PUGL_API PuglStatus
puglRegisterTimer(PuglView* view, uint64_t id, double rate);

/**
  Deactivate a currently registered timer, or do nothing if it doesn't exist.
*/
PUGL_API void
puglDeregisterTimer(PuglView* view, uint64_t id);

/**
   @}
*/

#ifndef PUGL_DISABLE_DEPRECATED

/**
   @}
   @name Deprecated API
   @{
*/

#if defined(__clang__)
#    define PUGL_DEPRECATED_BY(name) __attribute__((deprecated("", name)))
#elif defined(__GNUC__)
#    define PUGL_DEPRECATED_BY(name) __attribute__((deprecated("Use " name)))
#else
#    define PUGL_DEPRECATED_BY(name)
#endif

/**
   Create a Pugl application and view.

   To create a window, call the various puglInit* functions as necessary, then
   call puglCreateWindow().

   @deprecated Use puglNewApp() and puglNewView().

   @param pargc Pointer to argument count (currently unused).
   @param argv  Arguments (currently unused).
   @return A newly created view.
*/
static inline PUGL_DEPRECATED_BY("puglNewView") PuglView*
puglInit(const int* pargc, char** argv)
{
	(void)pargc;
	(void)argv;

	return puglNewView(puglNewWorld());
}

/**
   Destroy an app and view created with `puglInit()`.

   @deprecated Use puglFreeApp() and puglFreeView().
*/
static inline PUGL_DEPRECATED_BY("puglFreeView") void
puglDestroy(PuglView* view)
{
	PuglWorld* const world = puglGetWorld(view);

	puglFreeView(view);
	puglFreeWorld(world);
}

/**
   Set the window class name before creating a window.
*/
static inline PUGL_DEPRECATED_BY("puglSetClassName") void
puglInitWindowClass(PuglView* view, const char* name)
{
	puglSetClassName(puglGetWorld(view), name);
}

/**
   Set the window size before creating a window.

   @deprecated Use puglSetFrame().
*/
static inline PUGL_DEPRECATED_BY("puglSetFrame") void
puglInitWindowSize(PuglView* view, int width, int height)
{
	PuglRect frame = puglGetFrame(view);

	frame.width = width;
	frame.height = height;

	puglSetFrame(view, frame);
}

/**
   Set the minimum window size before creating a window.
*/
static inline PUGL_DEPRECATED_BY("puglSetMinSize") void
puglInitWindowMinSize(PuglView* view, int width, int height)
{
	puglSetMinSize(view, width, height);
}

/**
   Set the window aspect ratio range before creating a window.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.

   Note that setting different minimum and maximum constraints does not
   currenty work on MacOS (the minimum is used), so only setting a fixed aspect
   ratio works properly across all platforms.
*/
static inline PUGL_DEPRECATED_BY("puglSetAspectRatio") void
puglInitWindowAspectRatio(PuglView* view,
                          int       minX,
                          int       minY,
                          int       maxX,
                          int       maxY)
{
	puglSetAspectRatio(view, minX, minY, maxX, maxY);
}

/**
   Set transient parent before creating a window.

   On X11, parent must be a Window.
   On OSX, parent must be an NSView*.
*/
static inline PUGL_DEPRECATED_BY("puglSetTransientFor") void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
	puglSetTransientFor(view, (PuglNativeWindow)parent);
}

/**
   Enable or disable resizing before creating a window.

   @deprecated Use puglSetViewHint() with #PUGL_RESIZABLE.
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint") void
puglInitResizable(PuglView* view, bool resizable)
{
	puglSetViewHint(view, PUGL_RESIZABLE, resizable);
}

/**
   Get the current size of the view.

   @deprecated Use puglGetFrame().

*/
static inline PUGL_DEPRECATED_BY("puglGetFrame") void
puglGetSize(PuglView* view, int* width, int* height)
{
	const PuglRect frame = puglGetFrame(view);

	*width  = (int)frame.width;
	*height = (int)frame.height;
}

/**
   Ignore synthetic repeated key events.

   @deprecated Use puglSetViewHint() with #PUGL_IGNORE_KEY_REPEAT.
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint") void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, ignore);
}

/**
   Set a hint before creating a window.

   @deprecated Use puglSetWindowHint().
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint") void
puglInitWindowHint(PuglView* view, PuglViewHint hint, int value)
{
	puglSetViewHint(view, hint, value);
}

/**
   Set the parent window before creating a window (for embedding).

   @deprecated Use puglSetWindowParent().
*/
static inline PUGL_DEPRECATED_BY("puglSetParentWindow") void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
	puglSetParentWindow(view, parent);
}

/**
   Set the graphics backend to use.

   @deprecated Use puglSetBackend().
*/
static inline PUGL_DEPRECATED_BY("puglSetBackend") int
puglInitBackend(PuglView* view, const PuglBackend* backend)
{
	return (int)puglSetBackend(view, backend);
}

/**
   Block and wait for an event to be ready.

   This can be used in a loop to only process events via puglProcessEvents when
   necessary.  This function will block indefinitely if no events are
   available, so is not appropriate for use in programs that need to perform
   regular updates (e.g. animation).

   @deprecated Use puglPollEvents().
*/
PUGL_API PUGL_DEPRECATED_BY("puglPollEvents") PuglStatus
puglWaitForEvent(PuglView* view);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.  This function does
   not block if no events are pending.

   @deprecated Use puglDispatchEvents().
*/
PUGL_API PUGL_DEPRECATED_BY("puglDispatchEvents") PuglStatus
puglProcessEvents(PuglView* view);

#endif  /* PUGL_DISABLE_DEPRECATED */

/**
   @}
   @}
*/

PUGL_END_DECLS

#endif  /* PUGL_PUGL_H */
