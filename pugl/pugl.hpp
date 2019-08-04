/*
  Copyright 2012-2020 David Robillard <http://drobilla.net>

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
   @file pugl.hpp Pugl C++ API wrapper.
*/

#ifndef PUGL_PUGL_HPP
#define PUGL_PUGL_HPP

#include "pugl/pugl.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <utility>

/**
   @defgroup puglxx C++

   C++ API wrapper.

   @ingroup pugl_api
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

}

using Rect = PuglRect; ///< @copydoc PuglRect

/**
   @defgroup eventsxx Events
   @ingroup puglxx
   @copydoc events
   @{
*/

template<PuglEventType t, class Base>
struct TypedEvent final : Base {
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
using CreateEvent = TypedEvent<PUGL_CREATE, PuglEventCreate>;

/// @copydoc PuglEventDestroy
using DestroyEvent = TypedEvent<PUGL_DESTROY, PuglEventDestroy>;

/// @copydoc PuglEventConfigure
using ConfigureEvent = TypedEvent<PUGL_CONFIGURE, PuglEventConfigure>;

/// @copydoc PuglEventMap
using MapEvent = TypedEvent<PUGL_MAP, PuglEventMap>;

/// @copydoc PuglEventUnmap
using UnmapEvent = TypedEvent<PUGL_UNMAP, PuglEventUnmap>;

/// @copydoc PuglEventUpdate
using UpdateEvent = TypedEvent<PUGL_UPDATE, PuglEventUpdate>;

/// @copydoc PuglEventExpose
using ExposeEvent = TypedEvent<PUGL_EXPOSE, PuglEventExpose>;

/// @copydoc PuglEventClose
using CloseEvent = TypedEvent<PUGL_CLOSE, PuglEventClose>;

/// @copydoc PuglEventFocus
using FocusInEvent = TypedEvent<PUGL_FOCUS_IN, PuglEventFocus>;

/// @copydoc PuglEventFocus
using FocusOutEvent = TypedEvent<PUGL_FOCUS_OUT, PuglEventFocus>;

/// @copydoc PuglEventKey
using KeyPressEvent = TypedEvent<PUGL_KEY_PRESS, PuglEventKey>;

/// @copydoc PuglEventKey
using KeyReleaseEvent = TypedEvent<PUGL_KEY_RELEASE, PuglEventKey>;

/// @copydoc PuglEventText
using TextEvent = TypedEvent<PUGL_TEXT, PuglEventText>;

/// @copydoc PuglEventCrossing
using EnterEvent = TypedEvent<PUGL_POINTER_IN, PuglEventCrossing>;

/// @copydoc PuglEventCrossing
using LeaveEvent = TypedEvent<PUGL_POINTER_OUT, PuglEventCrossing>;

/// @copydoc PuglEventButton
using ButtonPressEvent = TypedEvent<PUGL_BUTTON_PRESS, PuglEventButton>;

/// @copydoc PuglEventButton
using ButtonReleaseEvent = TypedEvent<PUGL_BUTTON_RELEASE, PuglEventButton>;

/// @copydoc PuglEventMotion
using MotionEvent = TypedEvent<PUGL_MOTION, PuglEventMotion>;

/// @copydoc PuglEventScroll
using ScrollEvent = TypedEvent<PUGL_SCROLL, PuglEventScroll>;

/// @copydoc PuglEventClient
using ClientEvent = TypedEvent<PUGL_CLIENT, PuglEventClient>;

/// @copydoc PuglEventTimer
using TimerEvent = TypedEvent<PUGL_TIMER, PuglEventTimer>;

/**
   @}
   @defgroup statusxx Status
   @ingroup puglxx
   @copydoc status
   @{
*/

/// @copydoc PuglStatus
enum class Status {
	success,             ///< @copydoc PUGL_SUCCESS
	failure,             ///< @copydoc PUGL_FAILURE
	unknownError,        ///< @copydoc PUGL_UNKNOWN_ERROR
	badBackend,          ///< @copydoc PUGL_BAD_BACKEND
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
   @ingroup puglxx
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
	explicit World(WorldType type, WorldFlags flags = {})
	    : Wrapper{puglNewWorld(static_cast<PuglWorldType>(type), flags)}
	    , _clock(*this)
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
   @ingroup puglxx
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

/**
   Base class for View.

   The View class is templated by the type of user data to provide a type-safe
   equivalent to puglSetHandle().  This is a non-templated base class for all
   views that implements most functionality, so it can be used as a general
   type for any view to avoid infectious view-specific template types.
*/
class ViewBase : public detail::Wrapper<PuglView, puglFreeView>
{
public:
	/**
	   @name Setup
	   Methods for creating and destroying a view.
	   @{
	*/

	explicit ViewBase(World& world)
	    : Wrapper{puglNewView(world.cobj())}
	    , _world(world)
	{
		if (!cobj()) {
			throw std::runtime_error("Failed to create pugl::View");
		}

		puglSetHandle(cobj(), this);
	}

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

	/// @copydoc puglSetMinSize
	Status setMinSize(int width, int height)
	{
		return static_cast<Status>(puglSetMinSize(cobj(), width, height));
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

	/// @copydoc puglRequestAttention
	Status requestAttention()
	{
		return static_cast<Status>(puglRequestAttention(cobj()));
	}

	/**
	   @}
	*/

private:
	World& _world;
};

/**
   A drawable region that receives events.

   This is a relatively thin wrapper for #PuglView, but it adds a type-safe API
   for view-specific user data (to replace puglSetHandle()) and event
   dispatching (to replace puglSetEventFunc()).

   @tparam Data The type for user data passed during construction and returned
   by the `data()` method.
*/
template<typename Data>
class View : public ViewBase
{
public:
	explicit View(World& world)
	    : ViewBase{world}
	    , _data{}
	{
		puglSetEventFunc(cobj(), _onEvent);
	}

	View(World& world, Data data)
	    : ViewBase{world}
	    , _data{data}
	{
		puglSetEventFunc(cobj(), _onEvent);
	}

	template<class E>
	using TypedEventFunc = std::function<pugl::Status(View&, const E&)>;

	template<class HandledEvent>
	Status setEventFunc(std::function<pugl::Status(View&, const HandledEvent&)> handler)
	{
		std::get<HandledEvent::type>(_eventFuncs) = handler;

		return Status::success;
	}

	template<class E>
	using TypedEventHandler = pugl::Status (*)(View&, const E&);

	template<class HandledEvent>
	Status setEventFunc(TypedEventHandler<HandledEvent> handler)
	{
		std::get<HandledEvent::type>(_eventFuncs) = handler;

		return Status::success;
	}

	const Data& data() const { return _data; }
	Data&       data() { return _data; }

private:
	using NothingEvent = TypedEvent<PUGL_NOTHING, PuglEvent>;

	/**
	   A tuple of event handlers, one for each event type.

	   Note that the indices here must correspond to PuglEventType.
	*/
	using EventFuncs = std::tuple<TypedEventFunc<NothingEvent>,
	                              TypedEventFunc<CreateEvent>,
	                              TypedEventFunc<DestroyEvent>,
	                              TypedEventFunc<ConfigureEvent>,
	                              TypedEventFunc<MapEvent>,
	                              TypedEventFunc<UnmapEvent>,
	                              TypedEventFunc<UpdateEvent>,
	                              TypedEventFunc<ExposeEvent>,
	                              TypedEventFunc<CloseEvent>,
	                              TypedEventFunc<FocusInEvent>,
	                              TypedEventFunc<FocusOutEvent>,
	                              TypedEventFunc<KeyPressEvent>,
	                              TypedEventFunc<KeyReleaseEvent>,
	                              TypedEventFunc<TextEvent>,
	                              TypedEventFunc<EnterEvent>,
	                              TypedEventFunc<LeaveEvent>,
	                              TypedEventFunc<ButtonPressEvent>,
	                              TypedEventFunc<ButtonReleaseEvent>,
	                              TypedEventFunc<MotionEvent>,
	                              TypedEventFunc<ScrollEvent>,
	                              TypedEventFunc<ClientEvent>,
	                              TypedEventFunc<TimerEvent>>;

	using EventFunc = std::function<pugl::Status(View&, const PuglEvent&)>;

	static PuglStatus _onEvent(PuglView* view, const PuglEvent* event) noexcept
	{
		View* self = static_cast<View*>(puglGetHandle(view));

		return static_cast<PuglStatus>(self->dispatchEvent(*event));
	}

	Status dispatchEvent(const PuglEvent& event)
	{
		switch (event.type) {
		case PUGL_NOTHING:
			return Status::success;
		case PUGL_CREATE:
			return dispatch(static_cast<const CreateEvent&>(event.any));
		case PUGL_DESTROY:
			return dispatch(static_cast<const DestroyEvent&>(event.any));
		case PUGL_CONFIGURE:
			return dispatch(
			    static_cast<const ConfigureEvent&>(event.configure));
		case PUGL_MAP:
			return dispatch(static_cast<const MapEvent&>(event.any));
		case PUGL_UNMAP:
			return dispatch(static_cast<const UnmapEvent&>(event.any));
		case PUGL_UPDATE:
			return dispatch(static_cast<const UpdateEvent&>(event.any));
		case PUGL_EXPOSE:
			return dispatch(static_cast<const ExposeEvent&>(event.expose));
		case PUGL_CLOSE:
			return dispatch(static_cast<const CloseEvent&>(event.any));
		case PUGL_FOCUS_IN:
			return dispatch(static_cast<const FocusInEvent&>(event.focus));
		case PUGL_FOCUS_OUT:
			return dispatch(static_cast<const FocusOutEvent&>(event.focus));
		case PUGL_KEY_PRESS:
			return dispatch(static_cast<const KeyPressEvent&>(event.key));
		case PUGL_KEY_RELEASE:
			return dispatch(static_cast<const KeyReleaseEvent&>(event.key));
		case PUGL_TEXT:
			return dispatch(static_cast<const TextEvent&>(event.text));
		case PUGL_POINTER_IN:
			return dispatch(static_cast<const EnterEvent&>(event.crossing));
		case PUGL_POINTER_OUT:
			return dispatch(static_cast<const LeaveEvent&>(event.crossing));
		case PUGL_BUTTON_PRESS:
			return dispatch(static_cast<const ButtonPressEvent&>(event.button));
		case PUGL_BUTTON_RELEASE:
			return dispatch(
			    static_cast<const ButtonReleaseEvent&>(event.button));
		case PUGL_MOTION:
			return dispatch(static_cast<const MotionEvent&>(event.motion));
		case PUGL_SCROLL:
			return dispatch(static_cast<const ScrollEvent&>(event.scroll));
		case PUGL_CLIENT:
			return dispatch(static_cast<const ClientEvent&>(event.client));
		case PUGL_TIMER:
			return dispatch(static_cast<const TimerEvent&>(event.timer));
		}

		return Status::failure;
	}

	template<class E>
	Status dispatch(const E& event)
	{
		static_assert(sizeof(E) == sizeof(typename E::BaseEvent), "");
		static_assert(std::is_standard_layout<E>::value, "");

		assert(event.type == E::type);

		auto& handler = std::get<E::type>(_eventFuncs);
		if (handler) {
			return handler(*this, event);
		}

		return Status::success;
	}

	Data       _data;
	EventFuncs _eventFuncs;
};

/**
   @}
*/

} // namespace pugl

/**
   @}
*/

#endif /* PUGL_PUGL_HPP */
