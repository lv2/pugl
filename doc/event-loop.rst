.. default-domain:: c
.. highlight:: c

######################
Driving the Event Loop
######################

Pugl doesn't have any internal threads, the event loop is driven explicitly by repeatedly calling :func:`puglUpdate`.
This processes events from the window system and dispatches them to views when necessary and is usually called once per frame in the main loop.

The exact use of :func:`puglUpdate` depends on the application.
Modules or plugins should call it in a callback driven by the host, with a ``timeout`` of 0 to avoid blocking the main loop.

A program can use whatever timeout is appropriate:
event-driven applications may wait forever by using a ``timeout`` of -1,
while those that draw continuously may use a significant fraction of the frame period
(with enough time left over to render).

*****************
Event Dispatching
*****************

With the exception of a few special events like :enumerator:`PUGL_REALIZE`, events are generally dispatched during a call to :func:`puglUpdate`, directly within the scope of that call.
Depending on the platform, however, events may be dispatched at other times.

MacOS
=====

On MacOS, drawing is handled specially and not by the normal event queue mechanism.
This means that configure and expose events,
and possibly others,
may be dispatched to a view outside the scope of a :func:`puglUpdate` call.

Because drawing isn't handled by events, posting a redisplay will not wake up a blocked :func:`puglUpdate` call.

Windows
=======

On Windows, the application has relatively tight control over the event loop,
so most events are dispatched explicitly by :func:`puglUpdate`.
Drawing is handled by events, so obscuring a view will wake up a blocked :func:`puglUpdate` call.

However, it's possible for the system to dispatch events at other times.

X11
===

On X11, the application strictly controls event dispatching, the system can't call into application code at "surprising" times.
So, all events are dispatched in the scope of a :func:`puglUpdate` call.

*********************
Recursive Event Loops
*********************

On Windows and MacOS,
the event loop is stalled while the user is resizing the window or,
on Windows,
has displayed the window menu.
This means that :func:`puglUpdate` will block until the resize is finished,
or the menu is closed.

Pugl dispatches :struct:`PuglLoopEnterEvent` and :struct:`PuglLoopLeaveEvent` events to notify the application of this situation.
If you want to continuously redraw during resizing on these platforms,
you can schedule a timer with :func:`puglStartTimer` when the recursive loop is entered,
and post redisplays when handling the :struct:`PuglTimerEvent`.
Be sure to remove the timer with :func:`puglStopTimer` when the recursive loop is finished.

On X11, there are no recursive event loops,
and everything works as usual while the user is resizing the window.
There is nothing special about a "live resize" on X11,
and the above loop events will never be dispatched.

