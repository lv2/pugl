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

/**
   @file pugl.hpp
   @brief Pugl C++ API wrapper.
*/

#ifndef PUGL_PUGL_HPP
#define PUGL_PUGL_HPP

#include "pugl/pugl.h"

#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>

/**
   @defgroup pugl_cxx C++ API
   C++ API wrapper.

   @ingroup pugl
   @{
*/

/**
   Pugl C++ API namespace.
*/
namespace pugl {

namespace detail {

/// Free function for a C object
template<typename T>
using FreeFunc = void (*)(T*);

/// Simple overhead-free deleter for a C object
template<typename T, FreeFunc<T> Free>
struct Deleter {
	void operator()(T* ptr) { Free(ptr); }
};

/// Generic C++ wrapper for a C object
template<class T, FreeFunc<T> Free>
class Wrapper
{
public:
	T*       cobj() { return _ptr.get(); }
	const T* cobj() const { return _ptr.get(); }

protected:
	explicit Wrapper(T* ptr)
	    : _ptr(ptr, Deleter<T, Free>{})
	{}

private:
	std::unique_ptr<T, Deleter<T, Free>> _ptr;
};

} // namespace detail

using Rect = PuglRect; ///< @copydoc PuglRect

/**
   @defgroup eventsxx Events
   @ingroup pugl_cxx
   @copydoc events
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
	using BaseEvent = Base;

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

/**
   @}
   @defgroup statusxx Status
   @ingroup pugl_cxx
   @copydoc status
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
static inline const char*
strerror(const pugl::Status status)
{
	return puglStrerror(static_cast<PuglStatus>(status));
}

/**
   @}
   @defgroup worldxx World
   @ingroup pugl_cxx
   @copydoc world
   @{
*/

class World;

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

/// @copydoc PuglLogLevel
enum class LogLevel {
	err     = PUGL_LOG_LEVEL_ERR,     ///< @copydoc PUGL_LOG_LEVEL_ERR
	warning = PUGL_LOG_LEVEL_WARNING, ///< @copydoc PUGL_LOG_LEVEL_WARNING
	info    = PUGL_LOG_LEVEL_INFO,    ///< @copydoc PUGL_LOG_LEVEL_INFO
	debug   = PUGL_LOG_LEVEL_DEBUG,   ///< @copydoc PUGL_LOG_LEVEL_DEBUG
};

static_assert(LogLevel(PUGL_LOG_LEVEL_DEBUG) == LogLevel::debug, "");

/// @copydoc PuglLogFunc
using LogFunc =
    std::function<void(World& world, LogLevel level, const char* msg)>;

/**
   A `std::chrono` compatible clock that uses Pugl time.
*/
class Clock
{
public:
	using rep        = double;                         ///< Time representation
	using duration   = std::chrono::duration<double>;  ///< Duration in seconds
	using time_point = std::chrono::time_point<Clock>; ///< A Pugl time point

	static constexpr bool is_steady = true; ///< Steady clock flag, always true

	/// Construct a clock that uses time from puglGetTime()
	explicit Clock(World& world)
	    : _world{world}
	{}

	/// Return the current time
	time_point now() const;

private:
	const pugl::World& _world;
};

/// @copydoc PuglWorld
class World : public detail::Wrapper<PuglWorld, puglFreeWorld>
{
public:
	explicit World(WorldType type, WorldFlags flags)
	    : Wrapper{puglNewWorld(static_cast<PuglWorldType>(type), flags)}
	    , _clock(*this)
	{
		if (!cobj()) {
			throw std::runtime_error("Failed to create pugl::World");
		}
	}

	explicit World(WorldType type)
	    : World{type, {}}
	{
		if (!cobj()) {
			throw std::runtime_error("Failed to create pugl::World");
		}
	}

	/// @copydoc puglGetNativeWorld
	void* nativeWorld() { return puglGetNativeWorld(cobj()); }

	// TODO: setLogFunc

	Status setLogLevel(const LogLevel level)
	{
		return static_cast<Status>(
		    puglSetLogLevel(cobj(), static_cast<PuglLogLevel>(level)));
	}

	/// @copydoc puglSetClassName
	Status setClassName(const char* const name)
	{
		return static_cast<Status>(puglSetClassName(cobj(), name));
	}

	/// @copydoc puglGetTime
	double time() const { return puglGetTime(cobj()); }

	/// @copydoc puglUpdate
	Status update(const double timeout)
	{
		return static_cast<Status>(puglUpdate(cobj(), timeout));
	}

	/// Return a clock that uses Pugl time
	const Clock& clock() { return _clock; }

private:
	Clock _clock;
};

inline Clock::time_point
Clock::now() const
{
	return time_point{duration{_world.time()}};
}

/**
   @}
   @defgroup viewxx View
   @ingroup pugl_cxx
   @copydoc view
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
};

static_assert(ViewHint(PUGL_IGNORE_KEY_REPEAT) == ViewHint::ignoreKeyRepeat,
              "");

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
		if (!cobj()) {
			throw std::runtime_error("Failed to create pugl::View");
		}

		puglSetHandle(cobj(), this);
		puglSetEventFunc(cobj(), dispatchEvent);
	}

	virtual ~View() = default;

	View(const View&) = delete;
	View& operator=(const View&) = delete;

	View(View&&)   = delete;
	View&& operator=(View&&) = delete;

	const pugl::World& world() const { return _world; }
	pugl::World&       world() { return _world; }

	/// @copydoc puglSetViewHint
	Status setHint(ViewHint hint, int value)
	{
		return static_cast<Status>(
		    puglSetViewHint(cobj(), static_cast<PuglViewHint>(hint), value));
	}

	/**
	   @}
	   @name Frame
	   Methods for working with the position and size of a view.
	   @{
	*/

	/// @copydoc puglGetFrame
	Rect frame() const { return puglGetFrame(cobj()); }

	/// @copydoc puglSetFrame
	Status setFrame(Rect frame)
	{
		return static_cast<Status>(puglSetFrame(cobj(), frame));
	}

	/// @copydoc puglSetDefaultSize
	Status setDefaultSize(int width, int height)
	{
		return static_cast<Status>(puglSetDefaultSize(cobj(), width, height));
	}

	/// @copydoc puglSetMinSize
	Status setMinSize(int width, int height)
	{
		return static_cast<Status>(puglSetMinSize(cobj(), width, height));
	}

	/// @copydoc puglSetMaxSize
	Status setMaxSize(int width, int height)
	{
		return static_cast<Status>(puglSetMaxSize(cobj(), width, height));
	}

	/// @copydoc puglSetAspectRatio
	Status setAspectRatio(int minX, int minY, int maxX, int maxY)
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
	Status setWindowTitle(const char* title)
	{
		return static_cast<Status>(puglSetWindowTitle(cobj(), title));
	}

	/// @copydoc puglSetParentWindow
	Status setParentWindow(NativeView parent)
	{
		return static_cast<Status>(puglSetParentWindow(cobj(), parent));
	}

	/// @copydoc puglSetTransientFor
	Status setTransientFor(NativeView parent)
	{
		return static_cast<Status>(puglSetTransientFor(cobj(), parent));
	}

	/// @copydoc puglRealize
	Status realize() { return static_cast<Status>(puglRealize(cobj())); }

	/// @copydoc puglShowWindow
	Status showWindow() { return static_cast<Status>(puglShowWindow(cobj())); }

	/// @copydoc puglHideWindow
	Status hideWindow() { return static_cast<Status>(puglHideWindow(cobj())); }

	/// @copydoc puglGetVisible
	bool visible() const { return puglGetVisible(cobj()); }

	/// @copydoc puglGetNativeWindow
	NativeView nativeWindow() { return puglGetNativeWindow(cobj()); }

	/**
	   @}
	   @name Graphics
	   Methods for working with the graphics context and scheduling
	   redisplays.
	   @{
	*/

	/// @copydoc puglGetContext
	void* context() { return puglGetContext(cobj()); }

	/// @copydoc puglPostRedisplay
	Status postRedisplay()
	{
		return static_cast<Status>(puglPostRedisplay(cobj()));
	}

	/// @copydoc puglPostRedisplayRect
	Status postRedisplayRect(const Rect rect)
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
	Status grabFocus() { return static_cast<Status>(puglGrabFocus(cobj())); }

	/// @copydoc puglHasFocus
	bool hasFocus() const { return puglHasFocus(cobj()); }

	/// @copydoc puglSetBackend
	Status setBackend(const PuglBackend* backend)
	{
		return static_cast<Status>(puglSetBackend(cobj(), backend));
	}

	/// @copydoc puglSetCursor
	Status setCursor(const Cursor cursor)
	{
		return static_cast<Status>(
		    puglSetCursor(cobj(), static_cast<PuglCursor>(cursor)));
	}

	/// @copydoc puglRequestAttention
	Status requestAttention()
	{
		return static_cast<Status>(puglRequestAttention(cobj()));
	}

	/**
	   @}
	   @name Event Handlers
	   Methods called when events are dispatched to the view.
	   @{
	*/

	virtual Status onCreate(const CreateEvent&) PUGL_CONST_FUNC;
	virtual Status onDestroy(const DestroyEvent&) PUGL_CONST_FUNC;
	virtual Status onConfigure(const ConfigureEvent&) PUGL_CONST_FUNC;
	virtual Status onMap(const MapEvent&) PUGL_CONST_FUNC;
	virtual Status onUnmap(const UnmapEvent&) PUGL_CONST_FUNC;
	virtual Status onUpdate(const UpdateEvent&) PUGL_CONST_FUNC;
	virtual Status onExpose(const ExposeEvent&) PUGL_CONST_FUNC;
	virtual Status onClose(const CloseEvent&) PUGL_CONST_FUNC;
	virtual Status onFocusIn(const FocusInEvent&) PUGL_CONST_FUNC;
	virtual Status onFocusOut(const FocusOutEvent&) PUGL_CONST_FUNC;
	virtual Status onKeyPress(const KeyPressEvent&) PUGL_CONST_FUNC;
	virtual Status onKeyRelease(const KeyReleaseEvent&) PUGL_CONST_FUNC;
	virtual Status onText(const TextEvent&) PUGL_CONST_FUNC;
	virtual Status onPointerIn(const PointerInEvent&) PUGL_CONST_FUNC;
	virtual Status onPointerOut(const PointerOutEvent&) PUGL_CONST_FUNC;
	virtual Status onButtonPress(const ButtonPressEvent&) PUGL_CONST_FUNC;
	virtual Status onButtonRelease(const ButtonReleaseEvent&) PUGL_CONST_FUNC;
	virtual Status onMotion(const MotionEvent&) PUGL_CONST_FUNC;
	virtual Status onScroll(const ScrollEvent&) PUGL_CONST_FUNC;
	virtual Status onClient(const ClientEvent&) PUGL_CONST_FUNC;
	virtual Status onTimer(const TimerEvent&) PUGL_CONST_FUNC;

	/**
	   @}
	*/

	PuglView*       cobj() { return Wrapper::cobj(); }
	const PuglView* cobj() const { return Wrapper::cobj(); }

private:
	template<class Typed, class Base>
	static const Typed& typedEventRef(const Base& base)
	{
		const auto& event = static_cast<const Typed&>(base);
		static_assert(sizeof(event) == sizeof(typename Typed::BaseEvent), "");
		static_assert(std::is_standard_layout<Typed>::value, "");
		assert(event.type == Typed::type);
		return event;
	}

	static PuglStatus dispatchEvent(PuglView* view, const PuglEvent* event) noexcept {
		try {
			View* self = static_cast<View*>(puglGetHandle(view));

			return self->dispatch(event);
		} catch (...) {
			return PUGL_UNKNOWN_ERROR;
		}
	}

	PuglStatus dispatch(const PuglEvent* event)
	{
		switch (event->type) {
		case PUGL_NOTHING:
			return PUGL_SUCCESS;
		case PUGL_CREATE:
			return static_cast<PuglStatus>(
			    onCreate(typedEventRef<CreateEvent>(event->any)));
		case PUGL_DESTROY:
			return static_cast<PuglStatus>(
			    onDestroy(typedEventRef<DestroyEvent>(event->any)));
		case PUGL_CONFIGURE:
			return static_cast<PuglStatus>(onConfigure(
			    typedEventRef<ConfigureEvent>(event->configure)));
		case PUGL_MAP:
			return static_cast<PuglStatus>(
			    onMap(typedEventRef<MapEvent>(event->any)));
		case PUGL_UNMAP:
			return static_cast<PuglStatus>(
			    onUnmap(typedEventRef<UnmapEvent>(event->any)));
		case PUGL_UPDATE:
			return static_cast<PuglStatus>(
			    onUpdate(typedEventRef<UpdateEvent>(event->any)));
		case PUGL_EXPOSE:
			return static_cast<PuglStatus>(
			    onExpose(typedEventRef<ExposeEvent>(event->expose)));
		case PUGL_CLOSE:
			return static_cast<PuglStatus>(
			    onClose(typedEventRef<CloseEvent>(event->any)));
		case PUGL_FOCUS_IN:
			return static_cast<PuglStatus>(
			    onFocusIn(typedEventRef<FocusInEvent>(event->focus)));
		case PUGL_FOCUS_OUT:
			return static_cast<PuglStatus>(
			    onFocusOut(typedEventRef<FocusOutEvent>(event->focus)));
		case PUGL_KEY_PRESS:
			return static_cast<PuglStatus>(
			    onKeyPress(typedEventRef<KeyPressEvent>(event->key)));
		case PUGL_KEY_RELEASE:
			return static_cast<PuglStatus>(
			    onKeyRelease(typedEventRef<KeyReleaseEvent>(event->key)));
		case PUGL_TEXT:
			return static_cast<PuglStatus>(
			    onText(typedEventRef<TextEvent>(event->text)));
		case PUGL_POINTER_IN:
			return static_cast<PuglStatus>(onPointerIn(
			    typedEventRef<PointerInEvent>(event->crossing)));
		case PUGL_POINTER_OUT:
			return static_cast<PuglStatus>(onPointerOut(
			    typedEventRef<PointerOutEvent>(event->crossing)));
		case PUGL_BUTTON_PRESS:
			return static_cast<PuglStatus>(onButtonPress(
			    typedEventRef<ButtonPressEvent>(event->button)));
		case PUGL_BUTTON_RELEASE:
			return static_cast<PuglStatus>(onButtonRelease(
			    typedEventRef<ButtonReleaseEvent>(event->button)));
		case PUGL_MOTION:
			return static_cast<PuglStatus>(
			    onMotion(typedEventRef<MotionEvent>(event->motion)));
		case PUGL_SCROLL:
			return static_cast<PuglStatus>(
			    onScroll(typedEventRef<ScrollEvent>(event->scroll)));
		case PUGL_CLIENT:
			return static_cast<PuglStatus>(
			    onClient(typedEventRef<ClientEvent>(event->client)));
		case PUGL_TIMER:
			return static_cast<PuglStatus>(
			    onTimer(typedEventRef<TimerEvent>(event->timer)));
		}

		return PUGL_FAILURE;
	}

	World& _world;
};

/**
   @}
*/

} // namespace pugl

/**
   @}
*/

#endif /* PUGL_PUGL_HPP */
