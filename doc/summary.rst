Pugl is a library for writing portable and embeddable GUIs.
Pugl is not a toolkit or framework,
but a minimal portability layer that sets up a drawing context and delivers events.

Pugl is particularly suitable for use in plugins or other loadable modules.
It has no implicit context or mutable static data,
so it may be statically linked and used multiple times in the same process.

Pugl has a modular design with a core library and separate graphics backends.
The core library implements platform support and depends only on standard system libraries.
MacOS, Windows, and X11 are currently supported as platforms.

Graphics backends are built as separate libraries,
so applications can depend only on the APIs that they use.
Pugl includes graphics backends for Cairo_, OpenGL_, and Vulkan_.
Other graphics APIs can be used by implementing a custom backend.

.. _Cairo: https://www.cairographics.org/
.. _OpenGL: https://www.opengl.org/
.. _Vulkan: https://www.khronos.org/vulkan/
