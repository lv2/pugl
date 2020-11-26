.. default-domain:: c
.. highlight:: c

The core API (excluding backend-specific components) is declared in ``pugl.h``:

.. code-block:: c

   #include <pugl/pugl.h>

The API revolves around two main objects: the `world` and the `view`.
An application creates a world to manage top-level state,
then creates one or more views to display.

****************
Creating a World
****************

The world is the top-level object which represents an instance of Pugl.
It handles the connection to the window system,
and manages views and the event loop.

An application typically has a single world,
which is constructed once on startup and used to drive the main event loop.

Construction
============

A world must be created before any views, and it must outlive all of its views.
A world is created with :func:`puglNewWorld`, for example:

.. code-block:: c

   PuglWorld* world = puglNewWorld(PUGL_PROGRAM, 0);

For a plugin, specify :enumerator:`PUGL_MODULE <PuglWorldType.PUGL_MODULE>` instead.
In some cases, it is necessary to pass additional flags.
For example, Vulkan requires thread support:

.. code-block:: c

   PuglWorld* world = puglNewWorld(PUGL_MODULE, PUGL_WORLD_THREADS)

It is a good idea to set a class name for your project with :func:`puglSetClassName`.
This allows the window system to distinguish different applications and,
for example, users to set up rules to manage their windows nicely:

.. code-block:: c

   puglSetClassName(world, "MyAwesomeProject")

Setting Application Data
========================

Pugl will call an event handler in the application with only a view pointer and an event,
so there needs to be some way to access the data you use in your application.
This is done by setting an opaque handle on the world with :func:`puglSetWorldHandle`,
for example:

.. code-block:: c

   puglSetWorldHandle(world, myApp);

The handle can be later retrieved with :func:`puglGetWorldHandle`:

.. code-block:: c

   MyApp* app = (MyApp*)puglGetWorldHandle(world);

All non-constant data should be accessed via this handle,
to avoid problems associated with static mutable data.

***************
Creating a View
***************

A view is a drawable region that receives events.
You may think of it as a window,
though it may be embedded and not represent a top-level system window. [#f1]_

Creating a visible view is a multi-step process.
When a new view is created with :func:`puglNewView`,
it does not yet represent a "real" system view:

.. code-block:: c

   PuglView* view = puglNewView(world);

Configuring the Frame
=====================

Before display,
the necessary :doc:`frame <api/frame>` and :doc:`window <api/window>` attributes should be set.
These allow the window system (or plugin host) to arrange the view properly.
For example:

.. code-block:: c

   const double defaultWidth  = 1920.0;
   const double defaultHeight = 1080.0;

   puglSetWindowTitle(view, "My Window");
   puglSetDefaultSize(view, defaultWidth, defaultHeight);
   puglSetMinSize(view, defaultWidth / 4.0, defaultHeight / 4.0);
   puglSetAspectRatio(view, 1, 1, 16, 9);

There are also several :enum:`hints <PuglViewHint>` for basic attributes that can be set:

.. code-block:: c

   puglSetViewHint(view, PUGL_RESIZABLE, PUGL_TRUE);
   puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_TRUE);

Embedding
=========

To embed the view in another window,
you will need to somehow get the :type:`native view handle <PuglNativeView>` for the parent,
then set it with :func:`puglSetParentWindow`.
If the parent is a Pugl view,
the native handle can be accessed with :func:`puglGetNativeWindow`.
For example:

.. code-block:: c

   puglSetParentWindow(view, puglGetNativeWindow(parent));

Setting an Event Handler
========================

In order to actually do anything, a view must process events from the system.
Pugl dispatches all events to a single :type:`event handling function <PuglEventFunc>`,
which is set with :func:`puglSetEventFunc`:

.. code-block:: c

   puglSetEventFunc(view, onEvent);

See `Handling Events`_ below for details on writing the event handler itself.

Setting View Data
=================

Since the event handler is called with only a view pointer and an event,
there needs to be some way to access application data associated with the view.
Similar to `Setting Application Data`_ above,
this is done by setting an opaque handle on the view with :func:`puglSetHandle`,
for example:

.. code-block:: c

   puglSetHandle(view, myViewData);

The handle can be later retrieved,
likely in the event handler,
with :func:`puglGetHandle`:

.. code-block:: c

   MyViewData* data = (MyViewData*)puglGetHandle(view);

All non-constant data should be accessed via this handle,
to avoid problems associated with static mutable data.

If data is also associated with the world,
it can be retrieved via the view using :func:`puglGetWorld`:

.. code-block:: c

   PuglWorld* world = puglGetWorld(view);
   MyApp*     app   = (MyApp*)puglGetWorldHandle(world);

Setting a Backend
=================

Before being realized, the view must have a backend set with :func:`puglSetBackend`.

The backend manages the graphics API that will be used for drawing.
Pugl includes backends and supporting API for
:doc:`Cairo <api/cairo>`, :doc:`OpenGL <api/gl>`, and :doc:`Vulkan <api/vulkan>`.

Using Cairo
-----------

Cairo-specific API is declared in the ``cairo.h`` header:

.. code-block:: c

   #include <pugl/cairo.h>

The Cairo backend is provided by :func:`puglCairoBackend()`:

.. code-block:: c

   puglSetBackend(view, puglCairoBackend());

No additional configuration is required for Cairo.
To draw when handling an expose event,
the `Cairo context <https://www.cairographics.org/manual/cairo-cairo-t.html>`_ can be accessed with :func:`puglGetContext`:

.. code-block:: c

   cairo_t* cr = (cairo_t*)puglGetContext(view);

Using OpenGL
------------

OpenGL-specific API is declared in the ``gl.h`` header:

.. code-block:: c

   #include <pugl/gl.h>

The OpenGL backend is provided by :func:`puglGlBackend()`:

.. code-block:: c

   puglSetBackend(view, puglGlBackend());

Some hints must also be set so that the context can be set up correctly.
For example, to use OpenGL 3.3 Core Profile:

.. code-block:: c

   puglSetViewHint(view, PUGL_USE_COMPAT_PROFILE, PUGL_FALSE);
   puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 3);
   puglSetViewHint(view, PUGL_CONTEXT_VERSION_MINOR, 3);

If you need to perform some setup using the OpenGL API,
there are two ways to do so.

The OpenGL context is active when
:enumerator:`PUGL_CREATE <PuglEventType.PUGL_CREATE>` and
:enumerator:`PUGL_DESTROY <PuglEventType.PUGL_DESTROY>`
events are dispatched,
so things like creating and destroying shaders and textures can be done then.

Alternatively, if it is cumbersome to set up and tear down OpenGL in the event handler,
:func:`puglEnterContext` and :func:`puglLeaveContext` can be used to manually activate the OpenGL context during application setup.
Note, however, that unlike many other APIs, these functions must not be used for drawing.
It is only valid to use the OpenGL API for configuration in a manually entered context,
rendering will not work.
For example:

.. code-block:: c

   puglEnterContext(view);
   setupOpenGL(myApp);
   puglLeaveContext(view);

   while (!myApp->quit) {
     puglUpdate(world, 0.0);
   }

   puglEnterContext(view);
   teardownOpenGL(myApp);
   puglLeaveContext(view);

Using Vulkan
------------

Vulkan-specific API is declared in the ``vulkan.h`` header.
This header includes Vulkan headers,
so if you are dynamically loading Vulkan at runtime,
you should define ``VK_NO_PROTOTYPES`` before including it.

.. code-block:: c

   #define VK_NO_PROTOTYPES

   #include <pugl/vulkan.h>

The Vulkan backend is provided by :func:`puglVulkanBackend()`:

.. code-block:: c

   puglSetBackend(view, puglVulkanBackend());

Unlike OpenGL, almost all Vulkan configuration is done using the Vulkan API directly.
Pugl only provides a portable mechanism to load the Vulkan library and get the functions used to load the rest of the Vulkan API.

Loading Vulkan
^^^^^^^^^^^^^^

For maximum compatibility,
it is best to not link to Vulkan at compile-time,
but instead load the Vulkan API at run-time.
To do so, first create a :struct:`PuglVulkanLoader`:

.. code-block:: c

   PuglVulkanLoader* loader = puglNewVulkanLoader(world);

The loader manages the dynamically loaded Vulkan library,
so it must be kept alive for as long as the application is using Vulkan.
You can get the function used to load Vulkan functions with :func:`puglGetInstanceProcAddrFunc`:

.. code-block:: c

   PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
     puglGetInstanceProcAddrFunc(loader);

This vkGetInstanceProcAddr_ function can be used to load the rest of the Vulkan API.
For example, you can use it to get the vkCreateInstance_ function,
then use that to create your Vulkan instance.
In practice, you will want to use some loader or wrapper API since there are many Vulkan functions.

For advanced situations,
there is also :func:`puglGetDeviceProcAddrFunc` which retrieves the vkGetDeviceProcAddr_ function instead.

The Vulkan loader is provided for convenience,
so that applications to not need to write platform-specific code to load Vulkan.
Its use it not mandatory and Pugl can be used with Vulkan loaded by some other method.

Linking with Vulkan
^^^^^^^^^^^^^^^^^^^

If you do want to link to the Vulkan library at compile time,
note that the Pugl Vulkan backend does not depend on it,
so you will have to do so explicitly.

Creating a Surface
^^^^^^^^^^^^^^^^^^

The details of using Vulkan are far beyond the scope of this documentation,
but Pugl provides a portable function, :func:`puglCreateSurface`,
to get the Vulkan surface for a view.
Assuming you have somehow created your ``VkInstance``,
you can get the surface for a view using :func:`puglCreateSurface`:

.. code-block:: c

   VkSurfaceKHR* surface = NULL;
   puglCreateSurface(puglGetDeviceProcAddrFunc(loader),
                     view,
                     vulkanInstance,
                     NULL,
                     &surface);

Showing the View
================

Once the view is configured, it can be "realized" with :func:`puglRealize`.
This creates a "real" system view, for example:

.. code-block:: c

   PuglStatus status = puglRealize(view);
   if (status) {
     fprintf(stderr, "Error realizing view (%s)\n", puglStrerror(status));
   }

Note that realizing a view can fail for many reasons,
so the return code should always be checked.
This is generally the case for any function that interacts with the window system.
Most functions also return a :enum:`PuglStatus`,
but these checks are omitted for brevity in the rest of this documentation.

A realized view is not initially visible,
but can be shown with :func:`puglShow`:

.. code-block:: c

   puglShow(view);

To create an initially visible view,
it is also possible to simply call :func:`puglShow` right away.
The view will be automatically realized if necessary.

***************
Handling Events
***************

Events are sent to a view when it has received user input,
must be drawn, or in other situations that may need to be handled such as resizing.

Events are sent to the event handler as a :union:`PuglEvent` union.
The ``type`` field defines the type of the event and which field of the union is active.
The application must handle at least :enumerator:`PUGL_CONFIGURE <PuglEventType.PUGL_CONFIGURE>`
and :enumerator:`PUGL_EXPOSE <PuglEventType.PUGL_EXPOSE>` to draw anything,
but there are many other :enum:`event types <PuglEventType>`.

For example, a basic event handler might look something like this:

.. code-block:: c

   static PuglStatus
   onEvent(PuglView* view, const PuglEvent* event)
   {
     MyApp* app = (MyApp*)puglGetHandle(view);

     switch (event->type) {
     case PUGL_CREATE:
       return setupGraphics(app);
     case PUGL_DESTROY:
       return teardownGraphics(app);
     case PUGL_CONFIGURE:
       return resize(app, event->configure.width, event->configure.height);
     case PUGL_EXPOSE:
       return draw(app, view);
     case PUGL_CLOSE:
       return quit(app);
     case PUGL_BUTTON_PRESS:
        return onButtonPress(app, view, event->button);
     default:
       break;
     }

     return PUGL_SUCCESS;
   }

Using the Graphics Context
==========================

Drawing
-------

Note that Pugl uses a different drawing model than many libraries,
particularly those designed for game-style main loops like `SDL <https://libsdl.org/>`_ and `GLFW <https://www.glfw.org/>`_.

In that style of code, drawing is performed imperatively in the main loop,
but with Pugl, the application must draw only while handling an expose event.
This is because Pugl supports event-driven applications that only draw the damaged region when necessary,
and handles exposure internally to provide optimized and consistent behavior across platforms.

Cairo Context
-------------

A Cairo context is created for each :struct:`PuglEventExpose`,
and only exists during the handling of that event.
Null is returned by :func:`puglGetContext` at any other time.

OpenGL Context
--------------

The OpenGL context is only active during the handling of these events:

- :struct:`PuglEventCreate`
- :struct:`PuglEventDestroy`
- :struct:`PuglEventConfigure`
- :struct:`PuglEventExpose`

As always, drawing is only possible during an expose.

Vulkan Context
--------------

With Vulkan, the graphics context is managed by the application rather than Pugl.
However, drawing must still only be performed during an expose.

**********************
Driving the Event Loop
**********************

Pugl does not contain any threads or other event loop "magic".
For flexibility, the event loop is driven explicitly by repeatedly calling :func:`puglUpdate`,
which processes events from the window system and dispatches them to views when necessary.

The exact use of :func:`puglUpdate` depends on the application.
Plugins should call it with a ``timeout`` of 0 in a callback driven by the host.
This avoids blocking the main loop,
since other plugins and the host itself need to run as well.

A program can use whatever timeout is appropriate:
event-driven applications may wait forever by using a ``timeout`` of -1,
while those that draw continuously may use a significant fraction of the frame period
(with enough time left over to render).

Redrawing
=========

Occasional redrawing can be requested by calling :func:`puglPostRedisplay` or :func:`puglPostRedisplayRect`.
After these are called,
a :struct:`PuglEventExpose` will be dispatched on the next call to :func:`puglUpdate`.

For continuous redrawing,
call :func:`puglPostRedisplay` while handling a :struct:`PuglEventUpdate` event.
This event is sent just before views are redrawn,
so it can be used as a hook to expand the update region right before the view is exposed.
Anything else that needs to be done every frame can be handled similarly.

Event Dispatching
=================

Ideally, pending events are dispatched during a call to :func:`puglUpdate`,
directly within the scope of that call.

Unfortunately, this is not universally true due to differences between platforms.

MacOS
-----

On MacOS, drawing is handled specially and not by the normal event queue mechanism.
This means that configure and expose events,
and possibly others,
may be dispatched to a view outside the scope of a :func:`puglUpdate` call.
In general, you can not rely on coherent event dispatching semantics on MacOS:
the operating system can call into application code at "random" times,
and these calls may result in Pugl events being dispatched.

An application that follows the Pugl guidelines should work fine,
but there is one significant inconsistency you may encounter on MacOS:
posting a redisplay will not wake up a blocked :func:`puglUpdate` call.

Windows
-------

On Windows, the application has relatively tight control over the event loop,
so events are typically dispatched explicitly by :func:`puglUpdate`.
Drawing is handled by events,
so posting a redisplay will wake up a blocked :func:`puglUpdate` call.

However, it is possible for the system to dispatch events at other times.
So,
it is possible for events to be dispatched outside the scope of a :func:`puglUpdate` call,
but this does not happen in normal circumstances and can largely be ignored.

X11
---

On X11, the application strictly controls event dispatching,
and there is no way for the system to call into application code at surprising times.
So, all events are dispatched in the scope of a :func:`puglUpdate` call.

Recursive Event Loops
---------------------

On Windows and MacOS,
the event loop is stalled while the user is resizing the window or,
on Windows,
has displayed the window menu.
This means that :func:`puglUpdate` will block until the resize is finished,
or the menu is closed.

Pugl dispatches :struct:`PuglEventLoopEnter` and :struct:`PuglEventLoopLeave` events to notify the application of this situation.
If you want to continuously redraw during resizing on these platforms,
you can schedule a timer with :func:`puglStartTimer` when the recursive loop is entered,
and post redisplays when handling the :struct:`PuglEventTimer`.
Be sure to remove the timer with :func:`puglStopTimer` when the recursive loop is finished.

On X11, there are no recursive event loops,
and everything works as usual while the user is resizing the window.
There is nothing special about a "live resize" on X11,
and the above loop events will never be dispatched.

*************
Shutting Down
*************

When a view is closed,
it will receive a :struct:`PuglEventClose`.
An application may also set a flag based on user input or other conditions,
which can be used to break out of the main loop and stop calling :func:`puglUpdate`.

When the main event loop has finished running,
any views and the world need to be destroyed, in that order.
For example:

.. code-block:: c

   puglFreeView(view);
   puglFreeWorld(world);

.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/

.. _vkCreateInstance: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCreateInstance.html

.. _vkGetDeviceProcAddr: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetDeviceProcAddr.html

.. _vkGetInstanceProcAddr: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkGetInstanceProcAddr.html

.. rubric:: Footnotes

.. [#f1] MacOS has a strong distinction between
   `views <https://developer.apple.com/documentation/appkit/nsview>`_,
   which may be nested, and
   `windows <https://developer.apple.com/documentation/appkit/nswindow>`_,
   which may not.
   On Windows and X11, everything is a nestable window,
   but top-level windows are configured differently.
