.. default-domain:: c
.. highlight:: c

***************
Handling Events
***************

Events are sent to views in situations that might need handling, such as user input being available or the graphics needing to be drawn.
This is the main way that application code interacts with the user and window system.

Events are sent as a :union:`PuglEvent` to the callback function set with :func:`puglSetEventFunc`.
The ``type`` field defines the type of the event and which field is active.
There are many :enum:`event types <PuglEventType>`, but visible applications must handle at least :enumerator:`PUGL_CONFIGURE <PuglEventType.PUGL_CONFIGURE>`
and :enumerator:`PUGL_EXPOSE <PuglEventType.PUGL_EXPOSE>`.

For example, a basic event handler might look something like this:

.. code-block:: c

   static PuglStatus
   onEvent(PuglView* view, const PuglEvent* event)
   {
     MyApp* app = (MyApp*)puglGetHandle(view);

     switch (event->type) {
     case PUGL_REALIZE:
       return setupGraphics(app);
     case PUGL_UNREALIZE:
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

Drawing
=======

Drawing is only performed in response to an exposure (:struct:`PuglExposeEvent`), allowing portable code to make efficient use of the graphics system.
The graphics context is only available for this and perhaps a few other events, depending on the backend in use.

Cairo
-----

A Cairo context (`cairo_t <https://cairographics.org/manual/cairo-cairo-t.html>`_) is created for each exposure, and can be accessed with :func:`puglGetContext` during its handling.
The returned pointer is only valid until the end of the exposure.

OpenGL
------

The OpenGL context is only active while handling realization (:enumerator:`PUGL_REALIZE`, :enumerator:`PUGL_UNREALIZE`) and exposure, and can only be used for drawing during exposure.
Note that it's *not* active while handling configuration (:enumerator:`PUGL_CONFIGURE`), so the application must store the configuration and apply it to the graphics context on the next exposure.
The OpenGL context is implicit state, so :func:`puglGetContext` always returns null.

Vulkan
------

With Vulkan, the graphics context is managed by the application, so :func:`puglGetContext` always returns null.
The application must render and submit a frame when handling an exposure.

Requesting Redrawing
====================

A view or a region of it can be scheduled for redrawing with :func:`puglObscureView` or :func:`puglObscureRegion`.
This is usually done during an update (:enumerator:`PUGL_UPDATE`), but a view can be obscured at any time except during exposure.

After a view is obscured, an exposure (:struct:`PuglExposeEvent`) will be dispatched when it's time to draw the updated contents.
Note however that exposures are always mediated by the window system, there's no direct correspondence between them and any earlier obscure calls.
