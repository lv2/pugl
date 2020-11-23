This is the documentation for Pugl, a minimal API for writing GUIs.

## Reference

Pugl is implemented in C, but also provides a header-only C++ API wrapper.

 * [C API reference](@ref pugl)
 * [C++ API reference](@ref puglxx)

## Overview

The Pugl API revolves around two main objects: the World and the View.
An application creates a single world to manage top-level state,
then creates one or more views to display.

### World

The [World](@ref PuglWorld) contains all top-level state,
and manages views and the event loop.

A world must be [created](@ref puglNewWorld) before any views,
and it must outlive all views.

### View

A [View](@ref PuglView) is a drawable region that receives events.

Creating a visible view is a multi-step process.
When a new view is [created](@ref puglNewView),
it does not yet represent a real system view or window.
To display, it must first have a [backend](@ref puglSetBackend)
and [event handler](@ref puglSetEventFunc) set,
and be configured by [setting hints](@ref puglSetViewHint)
and optionally [adjusting the frame](@ref frame).

The [Backend](@ref PuglBackend) controls drawing for a view.
Pugl includes [Cairo](@ref cairo), [OpenGL](@ref gl), and [Vulkan](@ref vulkan) backends,
as well as a [stub](@ref stub) backend that creates a native window with no portable drawing context.

Once the view is configured,
it can be [realized](@ref puglRealize) and [shown](@ref puglShow).
By default a view will correspond to a top-level system window.
To create a view within another window,
it must have a [parent window set](@ref puglSetParentWindow) before being created.

### Events

[Events](@ref PuglEvent) are sent to a view when it has received user input or must be drawn.

Events are handled by the [event handler](@ref PuglEventFunc) set during initialisation.
This function is called whenever something happens that the view must respond to.
This includes user interaction like mouse and keyboard input,
and system events like window resizing and exposure.

### Event Loop

The event loop is driven by repeatedly calling #puglUpdate which processes events from the window system,
and dispatches them to views when necessary.

Typically, a plugin calls #puglUpdate with timeout 0 in some callback driven by the host.
A program can use whatever timeout is appropriate:
event-driven applications may wait forever,
or for continuous animation,
use a timeout that is a significant fraction of the frame period
(with enough time left over to render).

Redrawing can be requested by calling #puglPostRedisplay or #puglPostRedisplayRect,
which post expose events to the queue.
Note, however, that this will not wake up a blocked #puglUpdate call on MacOS
(which does not handle drawing via events).
For continuous redrawing, call #puglPostRedisplay when a #PUGL_UPDATE event is received.
This event is sent before views are redrawn,
so can be used as a hook to expand the update region right before the view is exposed.

### Error Handling

Most functions return a [Status](@ref PuglStatus) which should be checked to detect failure.
