/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

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

#ifndef PUGL_PUGL_HPP
#define PUGL_PUGL_HPP

#include "pugl/pugl.h"

#include <cstdint>

#if defined(PUGL_HPP_THROW_FAILED_CONSTRUCTION)
#  include <exception>
#elif defined(PUGL_HPP_ASSERT_CONSTRUCTION)
#  include <cassert>
#endif

namespace pugl {

/**
   @defgroup puglxx Pugl C++ API
   Pugl C++ API wrapper.
   @{
*/

namespace detail {

/// Free function for a C object
template<typename T>
using FreeFunc = void (*)(T*);

/// Generic C++ wrapper for a C object
template<class T, FreeFunc<T> Free>
class Wrapper
{
public:
  Wrapper(const Wrapper&) = delete;
  Wrapper& operator=(const Wrapper&) = delete;

  Wrapper(Wrapper&& wrapper) noexcept
    : _ptr{wrapper._ptr}
  {
    wrapper._ptr = nullptr;
  }

  Wrapper& operator=(Wrapper&& wrapper) noexcept
  {
    _ptr         = wrapper._ptr;
    wrapper._ptr = nullptr;
    return *this;
  }

  ~Wrapper() noexcept { Free(_ptr); }

  T*       cobj() noexcept { return _ptr; }
  const T* cobj() const noexcept { return _ptr; }

protected:
  explicit Wrapper(T* ptr) noexcept
    : _ptr{ptr}
  {}

private:
  T* _ptr;
};

} // namespace detail

using Rect = PuglRect; ///< @copydoc PuglRect

/**
   @defgroup eventsxx Events
   @{
*/

/**
   A strongly-typed analogue of PuglEvent.

   This is bit-for-bit identical to the corresponding PuglEvent, so events are
   simply cast to this type to avoid any copying overhead.

   @tparam t The `type` field of the corresponding PuglEvent.

   @tparam Base The specific struct type of the corresponding PuglEvent.
*/
template<PuglEventType t, class Base>
struct Event final : Base {
  /// The type of the corresponding C event structure
  using BaseEvent = Base;

  /// The `type` field of the corresponding C event structure
  static constexpr const PuglEventType type = t;
};

using Mod          = PuglMod;          ///< @copydoc PuglMod
using Mods         = PuglMods;         ///< @copydoc PuglMods
using Key          = PuglKey;          ///< @copydoc PuglKey
using EventType    = PuglEventType;    ///< @copydoc PuglEventType
using EventFlag    = PuglEventFlag;    ///< @copydoc PuglEventFlag
using EventFlags   = PuglEventFlags;   ///< @copydoc PuglEventFlags
using CrossingMode = PuglCrossingMode; ///< @copydoc PuglCrossingMode

/// @copydoc PuglEventCreate
using CreateEvent = Event<PUGL_CREATE, PuglEventCreate>;

/// @copydoc PuglEventDestroy
using DestroyEvent = Event<PUGL_DESTROY, PuglEventDestroy>;

/// @copydoc PuglEventConfigure
using ConfigureEvent = Event<PUGL_CONFIGURE, PuglEventConfigure>;

/// @copydoc PuglEventMap
using MapEvent = Event<PUGL_MAP, PuglEventMap>;

/// @copydoc PuglEventUnmap
using UnmapEvent = Event<PUGL_UNMAP, PuglEventUnmap>;

/// @copydoc PuglEventUpdate
using UpdateEvent = Event<PUGL_UPDATE, PuglEventUpdate>;

/// @copydoc PuglEventExpose
using ExposeEvent = Event<PUGL_EXPOSE, PuglEventExpose>;

/// @copydoc PuglEventClose
using CloseEvent = Event<PUGL_CLOSE, PuglEventClose>;

/// @copydoc PuglEventFocus
using FocusInEvent = Event<PUGL_FOCUS_IN, PuglEventFocus>;

/// @copydoc PuglEventFocus
using FocusOutEvent = Event<PUGL_FOCUS_OUT, PuglEventFocus>;

/// @copydoc PuglEventKey
using KeyPressEvent = Event<PUGL_KEY_PRESS, PuglEventKey>;

/// @copydoc PuglEventKey
using KeyReleaseEvent = Event<PUGL_KEY_RELEASE, PuglEventKey>;

/// @copydoc PuglEventText
using TextEvent = Event<PUGL_TEXT, PuglEventText>;

/// @copydoc PuglEventCrossing
using PointerInEvent = Event<PUGL_POINTER_IN, PuglEventCrossing>;

/// @copydoc PuglEventCrossing
using PointerOutEvent = Event<PUGL_POINTER_OUT, PuglEventCrossing>;

/// @copydoc PuglEventButton
using ButtonPressEvent = Event<PUGL_BUTTON_PRESS, PuglEventButton>;

/// @copydoc PuglEventButton
using ButtonReleaseEvent = Event<PUGL_BUTTON_RELEASE, PuglEventButton>;

/// @copydoc PuglEventMotion
using MotionEvent = Event<PUGL_MOTION, PuglEventMotion>;

/// @copydoc PuglEventScroll
using ScrollEvent = Event<PUGL_SCROLL, PuglEventScroll>;

/// @copydoc PuglEventClient
using ClientEvent = Event<PUGL_CLIENT, PuglEventClient>;

/// @copydoc PuglEventTimer
using TimerEvent = Event<PUGL_TIMER, PuglEventTimer>;

/// @copydoc PuglEventLoopEnter
using LoopEnterEvent = Event<PUGL_LOOP_ENTER, PuglEventLoopEnter>;

/// @copydoc PuglEventLoopLeave
using LoopLeaveEvent = Event<PUGL_LOOP_LEAVE, PuglEventLoopLeave>;

/**
   @}
   @defgroup statusxx Status
   @{
*/

/// @copydoc PuglStatus
enum class Status {
  success,             ///< @copydoc PUGL_SUCCESS
  failure,             ///< @copydoc PUGL_FAILURE
  unknownError,        ///< @copydoc PUGL_UNKNOWN_ERROR
  badBackend,          ///< @copydoc PUGL_BAD_BACKEND
  badConfiguration,    ///< @copydoc PUGL_BAD_CONFIGURATION
  badParameter,        ///< @copydoc PUGL_BAD_PARAMETER
  backendFailed,       ///< @copydoc PUGL_BACKEND_FAILED
  registrationFailed,  ///< @copydoc PUGL_REGISTRATION_FAILED
  realizeFailed,       ///< @copydoc PUGL_REALIZE_FAILED
  setFormatFailed,     ///< @copydoc PUGL_SET_FORMAT_FAILED
  createContextFailed, ///< @copydoc PUGL_CREATE_CONTEXT_FAILED
  unsupportedType,     ///< @copydoc PUGL_UNSUPPORTED_TYPE
};

static_assert(Status(PUGL_UNSUPPORTED_TYPE) == Status::unsupportedType, "");

/// @copydoc puglStrerror
inline const char*
strerror(const Status status) noexcept
{
  return puglStrerror(static_cast<PuglStatus>(status));
}

/**
   @}
   @defgroup worldxx World
   @{
*/

/// @copydoc PuglWorldType
enum class WorldType {
  program, ///< @copydoc PUGL_PROGRAM
  module,  ///< @copydoc PUGL_MODULE
};

static_assert(WorldType(PUGL_MODULE) == WorldType::module, "");

/// @copydoc PuglWorldFlag
enum class WorldFlag {
  threads = PUGL_WORLD_THREADS, ///< @copydoc PUGL_WORLD_THREADS
};

static_assert(WorldFlag(PUGL_WORLD_THREADS) == WorldFlag::threads, "");

using WorldFlags = PuglWorldFlags; ///< @copydoc PuglWorldFlags

#if defined(PUGL_HPP_THROW_FAILED_CONSTRUCTION)

/// An exception thrown when construction fails
class FailedConstructionError : public std::exception
{
public:
  FailedConstructionError(const char* const msg) noexcept
    : _msg{msg}
  {}

  virtual const char* what() const noexcept override;

private:
  const char* _msg;
};

#  define PUGL_CHECK_CONSTRUCTION(cond, msg) \
    do {                                     \
      if (!(cond)) {                         \
        throw FailedConstructionError(msg);  \
      }                                      \
    } while (0)

#elif defined(PUGL_HPP_ASSERT_CONSTRUCTION)
#  define PUGL_CHECK_CONSTRUCTION(cond, msg) assert(cond);
#else
/**
   Configurable macro for handling construction failure.

   If `PUGL_HPP_THROW_FAILED_CONSTRUCTION` is defined, then this throws a
   `pugl::FailedConstructionError` if construction fails.

   If `PUGL_HPP_ASSERT_CONSTRUCTION` is defined, then this asserts if
   construction fails.

   Otherwise, this does nothing.
*/
#  define PUGL_CHECK_CONSTRUCTION(cond, msg)
#endif

/// @copydoc PuglWorld
class World : public detail::Wrapper<PuglWorld, puglFreeWorld>
{
public:
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  World(World&&) = delete;
  World& operator=(World&&) = delete;

  ~World() = default;

  World(WorldType type, WorldFlag flag)
    : Wrapper{puglNewWorld(static_cast<PuglWorldType>(type),
                           static_cast<PuglWorldFlags>(flag))}
  {
    PUGL_CHECK_CONSTRUCTION(cobj(), "Failed to create pugl::World");
  }

  World(WorldType type, WorldFlags flags)
    : Wrapper{puglNewWorld(static_cast<PuglWorldType>(type), flags)}
  {
    PUGL_CHECK_CONSTRUCTION(cobj(), "Failed to create pugl::World");
  }

  explicit World(WorldType type)
    : World{type, WorldFlags{}}
  {}

  /// @copydoc puglGetNativeWorld
  void* nativeWorld() noexcept { return puglGetNativeWorld(cobj()); }

  /// @copydoc puglSetClassName
  Status setClassName(const char* const name) noexcept
  {
    return static_cast<Status>(puglSetClassName(cobj(), name));
  }

  /// @copydoc puglGetTime
  double time() const noexcept { return puglGetTime(cobj()); }

  /// @copydoc puglUpdate
  Status update(const double timeout) noexcept
  {
    return static_cast<Status>(puglUpdate(cobj(), timeout));
  }
};

/**
   @}
   @defgroup viewxx View
   @{
*/

using Backend    = PuglBackend;    ///< @copydoc PuglBackend
using NativeView = PuglNativeView; ///< @copydoc PuglNativeView

/// @copydoc PuglViewHint
enum class ViewHint {
  useCompatProfile,    ///< @copydoc PUGL_USE_COMPAT_PROFILE
  useDebugContext,     ///< @copydoc PUGL_USE_DEBUG_CONTEXT
  contextVersionMajor, ///< @copydoc PUGL_CONTEXT_VERSION_MAJOR
  contextVersionMinor, ///< @copydoc PUGL_CONTEXT_VERSION_MINOR
  redBits,             ///< @copydoc PUGL_RED_BITS
  greenBits,           ///< @copydoc PUGL_GREEN_BITS
  blueBits,            ///< @copydoc PUGL_BLUE_BITS
  alphaBits,           ///< @copydoc PUGL_ALPHA_BITS
  depthBits,           ///< @copydoc PUGL_DEPTH_BITS
  stencilBits,         ///< @copydoc PUGL_STENCIL_BITS
  samples,             ///< @copydoc PUGL_SAMPLES
  doubleBuffer,        ///< @copydoc PUGL_DOUBLE_BUFFER
  swapInterval,        ///< @copydoc PUGL_SWAP_INTERVAL
  resizable,           ///< @copydoc PUGL_RESIZABLE
  ignoreKeyRepeat,     ///< @copydoc PUGL_IGNORE_KEY_REPEAT
  refreshRate,         ///< @copydoc PUGL_REFRESH_RATE
};

static_assert(ViewHint(PUGL_REFRESH_RATE) == ViewHint::refreshRate, "");

using ViewHintValue = PuglViewHintValue; ///< @copydoc PuglViewHintValue

/// @copydoc PuglCursor
enum class Cursor {
  arrow,     ///< @copydoc PUGL_CURSOR_ARROW
  caret,     ///< @copydoc PUGL_CURSOR_CARET
  crosshair, ///< @copydoc PUGL_CURSOR_CROSSHAIR
  hand,      ///< @copydoc PUGL_CURSOR_HAND
  no,        ///< @copydoc PUGL_CURSOR_NO
  leftRight, ///< @copydoc PUGL_CURSOR_LEFT_RIGHT
  upDown,    ///< @copydoc PUGL_CURSOR_UP_DOWN
};

static_assert(Cursor(PUGL_CURSOR_UP_DOWN) == Cursor::upDown, "");

/// @copydoc PuglView
class View : protected detail::Wrapper<PuglView, puglFreeView>
{
public:
  /**
     @name Setup
     Methods for creating and destroying a view.
     @{
  */

  explicit View(World& world)
    : Wrapper{puglNewView(world.cobj())}
    , _world(world)
  {
    PUGL_CHECK_CONSTRUCTION(cobj(), "Failed to create pugl::View");
  }

  const World& world() const noexcept { return _world; }
  World&       world() noexcept { return _world; }

  /**
     Set the object that will be called to handle events.

     This is a type-safe wrapper for the C functions puglSetHandle() and
     puglSetEventFunc() that will automatically dispatch events to the
     `onEvent` method of `handler` that takes the appropriate event type.
     The handler must have such a method defined for every event type, but if
     the handler is the view itself, a `using` declaration can be used to
     "inherit" the default implementation to avoid having to define every
     method.  For example:

     @code
     class MyView : public pugl::View
     {
     public:
       explicit MyView(pugl::World& world)
         : pugl::View{world}
       {
         setEventHandler(*this);
       }

       using pugl::View::onEvent;

       pugl::Status onEvent(const pugl::ConfigureEvent& event) noexcept;
       pugl::Status onEvent(const pugl::ExposeEvent& event) noexcept;
     };
     @endcode

     This facility is just a convenience, applications may use the C API
     directly to set a handle and event function to set up a different
     approach for event handling.
  */
  template<class Handler>
  Status setEventHandler(Handler& handler)
  {
    puglSetHandle(cobj(), &handler);
    return static_cast<Status>(puglSetEventFunc(cobj(), eventFunc<Handler>));
  }

  /// @copydoc puglSetBackend
  Status setBackend(const PuglBackend* backend) noexcept
  {
    return static_cast<Status>(puglSetBackend(cobj(), backend));
  }

  /// @copydoc puglSetViewHint
  Status setHint(ViewHint hint, int value) noexcept
  {
    return static_cast<Status>(
      puglSetViewHint(cobj(), static_cast<PuglViewHint>(hint), value));
  }

  /// @copydoc puglGetViewHint
  int getHint(ViewHint hint) noexcept
  {
    return puglGetViewHint(cobj(), static_cast<PuglViewHint>(hint));
  }

  /**
     @}
     @name Frame
     Methods for working with the position and size of a view.
     @{
  */

  /// @copydoc puglGetFrame
  Rect frame() const noexcept { return puglGetFrame(cobj()); }

  /// @copydoc puglSetFrame
  Status setFrame(Rect frame) noexcept
  {
    return static_cast<Status>(puglSetFrame(cobj(), frame));
  }

  /// @copydoc puglSetDefaultSize
  Status setDefaultSize(int width, int height) noexcept
  {
    return static_cast<Status>(puglSetDefaultSize(cobj(), width, height));
  }

  /// @copydoc puglSetMinSize
  Status setMinSize(int width, int height) noexcept
  {
    return static_cast<Status>(puglSetMinSize(cobj(), width, height));
  }

  /// @copydoc puglSetMaxSize
  Status setMaxSize(int width, int height) noexcept
  {
    return static_cast<Status>(puglSetMaxSize(cobj(), width, height));
  }

  /// @copydoc puglSetAspectRatio
  Status setAspectRatio(int minX, int minY, int maxX, int maxY) noexcept
  {
    return static_cast<Status>(
      puglSetAspectRatio(cobj(), minX, minY, maxX, maxY));
  }

  /**
     @}
     @name Windows
     Methods for working with top-level windows.
     @{
  */

  /// @copydoc puglSetWindowTitle
  Status setWindowTitle(const char* title) noexcept
  {
    return static_cast<Status>(puglSetWindowTitle(cobj(), title));
  }

  /// @copydoc puglSetParentWindow
  Status setParentWindow(NativeView parent) noexcept
  {
    return static_cast<Status>(puglSetParentWindow(cobj(), parent));
  }

  /// @copydoc puglSetTransientFor
  Status setTransientFor(NativeView parent) noexcept
  {
    return static_cast<Status>(puglSetTransientFor(cobj(), parent));
  }

  /// @copydoc puglRealize
  Status realize() noexcept { return static_cast<Status>(puglRealize(cobj())); }

  /// @copydoc puglShow
  Status show() noexcept { return static_cast<Status>(puglShow(cobj())); }

  /// @copydoc puglHide
  Status hide() noexcept { return static_cast<Status>(puglHide(cobj())); }

  /// @copydoc puglGetVisible
  bool visible() const noexcept { return puglGetVisible(cobj()); }

  /// @copydoc puglGetNativeWindow
  NativeView nativeWindow() noexcept { return puglGetNativeWindow(cobj()); }

  /**
     @}
     @name Graphics
     Methods for working with the graphics context and scheduling
     redisplays.
     @{
  */

  /// @copydoc puglGetContext
  void* context() noexcept { return puglGetContext(cobj()); }

  /// @copydoc puglPostRedisplay
  Status postRedisplay() noexcept
  {
    return static_cast<Status>(puglPostRedisplay(cobj()));
  }

  /// @copydoc puglPostRedisplayRect
  Status postRedisplayRect(const Rect rect) noexcept
  {
    return static_cast<Status>(puglPostRedisplayRect(cobj(), rect));
  }

  /**
     @}
     @name Interaction
     Methods for interacting with the user and window system.
     @{
  */

  /// @copydoc puglGrabFocus
  Status grabFocus() noexcept
  {
    return static_cast<Status>(puglGrabFocus(cobj()));
  }

  /// @copydoc puglHasFocus
  bool hasFocus() const noexcept { return puglHasFocus(cobj()); }

  /// @copydoc puglSetCursor
  Status setCursor(const Cursor cursor) noexcept
  {
    return static_cast<Status>(
      puglSetCursor(cobj(), static_cast<PuglCursor>(cursor)));
  }

  /// @copydoc puglRequestAttention
  Status requestAttention() noexcept
  {
    return static_cast<Status>(puglRequestAttention(cobj()));
  }

  /**
     Activate a repeating timer event.

     This starts a timer which will send a timer event to `view` every
     `timeout` seconds.  This can be used to perform some action in a view at a
     regular interval with relatively low frequency.  Note that the frequency
     of timer events may be limited by how often update() is called.

     If the given timer already exists, it is replaced.

     @param id The identifier for this timer.  This is an application-specific
     ID that should be a low number, typically the value of a constant or `enum`
     that starts from 0.  There is a platform-specific limit to the number of
     supported timers, and overhead associated with each, so applications should
     create only a few timers and perform several tasks in one if necessary.

     @param timeout The period, in seconds, of this timer.  This is not
     guaranteed to have a resolution better than 10ms (the maximum timer
     resolution on Windows) and may be rounded up if it is too short.  On X11
     and MacOS, a resolution of about 1ms can usually be relied on.

     @return #PUGL_FAILURE if timers are not supported by the system,
     #PUGL_UNKNOWN_ERROR if setting the timer failed.
  */
  Status startTimer(const uintptr_t id, const double timeout) noexcept
  {
    return static_cast<Status>(puglStartTimer(cobj(), id, timeout));
  }

  /**
     Stop an active timer.

     @param id The ID previously passed to startTimer().

     @return #PUGL_FAILURE if timers are not supported by this system,
     #PUGL_UNKNOWN_ERROR if stopping the timer failed.
  */
  Status stopTimer(const uintptr_t id) noexcept
  {
    return static_cast<Status>(puglStopTimer(cobj(), id));
  }

  /**
     @}
  */

  PuglView*       cobj() noexcept { return Wrapper::cobj(); }
  const PuglView* cobj() const noexcept { return Wrapper::cobj(); }

private:
  template<class Target>
  static Status dispatch(Target& target, const PuglEvent* event)
  {
    switch (event->type) {
    case PUGL_NOTHING:
      return Status::success;
    case PUGL_CREATE:
      return target.onEvent(static_cast<const CreateEvent&>(event->any));
    case PUGL_DESTROY:
      return target.onEvent(static_cast<const DestroyEvent&>(event->any));
    case PUGL_CONFIGURE:
      return target.onEvent(
        static_cast<const ConfigureEvent&>(event->configure));
    case PUGL_MAP:
      return target.onEvent(static_cast<const MapEvent&>(event->any));
    case PUGL_UNMAP:
      return target.onEvent(static_cast<const UnmapEvent&>(event->any));
    case PUGL_UPDATE:
      return target.onEvent(static_cast<const UpdateEvent&>(event->any));
    case PUGL_EXPOSE:
      return target.onEvent(static_cast<const ExposeEvent&>(event->expose));
    case PUGL_CLOSE:
      return target.onEvent(static_cast<const CloseEvent&>(event->any));
    case PUGL_FOCUS_IN:
      return target.onEvent(static_cast<const FocusInEvent&>(event->focus));
    case PUGL_FOCUS_OUT:
      return target.onEvent(static_cast<const FocusOutEvent&>(event->focus));
    case PUGL_KEY_PRESS:
      return target.onEvent(static_cast<const KeyPressEvent&>(event->key));
    case PUGL_KEY_RELEASE:
      return target.onEvent(static_cast<const KeyReleaseEvent&>(event->key));
    case PUGL_TEXT:
      return target.onEvent(static_cast<const TextEvent&>(event->text));
    case PUGL_POINTER_IN:
      return target.onEvent(
        static_cast<const PointerInEvent&>(event->crossing));
    case PUGL_POINTER_OUT:
      return target.onEvent(
        static_cast<const PointerOutEvent&>(event->crossing));
    case PUGL_BUTTON_PRESS:
      return target.onEvent(
        static_cast<const ButtonPressEvent&>(event->button));
    case PUGL_BUTTON_RELEASE:
      return target.onEvent(
        static_cast<const ButtonReleaseEvent&>(event->button));
    case PUGL_MOTION:
      return target.onEvent(static_cast<const MotionEvent&>(event->motion));
    case PUGL_SCROLL:
      return target.onEvent(static_cast<const ScrollEvent&>(event->scroll));
    case PUGL_CLIENT:
      return target.onEvent(static_cast<const ClientEvent&>(event->client));
    case PUGL_TIMER:
      return target.onEvent(static_cast<const TimerEvent&>(event->timer));
    case PUGL_LOOP_ENTER:
      return target.onEvent(static_cast<const LoopEnterEvent&>(event->any));
    case PUGL_LOOP_LEAVE:
      return target.onEvent(static_cast<const LoopLeaveEvent&>(event->any));
    }

    return Status::failure;
  }

  template<class Target>
  static PuglStatus eventFunc(PuglView* view, const PuglEvent* event) noexcept
  {
    auto* target = static_cast<Target*>(puglGetHandle(view));

#ifdef __cpp_exceptions
    try {
      return static_cast<PuglStatus>(dispatch(*target, event));
    } catch (...) {
      return PUGL_UNKNOWN_ERROR;
    }
#else
    return static_cast<PuglStatus>(pugl::dispatch(*target, event));
#endif
  }

  World& _world;
};

/**
   @}
   @}
*/

} // namespace pugl

#endif // PUGL_PUGL_HPP
