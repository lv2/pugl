Pugl is an API for writing portable and embeddable GUIs.
Pugl is not a toolkit or framework,
but a minimal portability layer that sets up a drawing context and delivers events.

Compared to other libraries,
Pugl is particularly suitable for use in plugins or other loadable modules.
There is no implicit context or static data in the library,
so it may be statically linked and used multiple times in the same process.

Pugl has a modular design that separates the core library from graphics backends.
The core library is graphics agnostic,
it implements platform support and depends only on standard system libraries.
MacOS, Windows, and X11 are currently supported as platforms.

Graphics backends are separate so that applications only depend on the API that they use.
Pugl includes graphics backends for Cairo_, OpenGL_, and Vulkan_.
It is also possible to use some other graphics API by implementing a custom backend,
or simply accessing the native platform handle for a window.

.. _Cairo: https://www.cairographics.org/
.. _OpenGL: https://www.opengl.org/
.. _Vulkan: https://www.khronos.org/vulkan/
