Pugl
====

Pugl (PlUgin Graphics Library) is a minimal portable API for GUIs which is
suitable for use in plugins.  It works on X11, MacOS, and Windows, and
optionally supports Vulkan, OpenGL, and Cairo graphics contexts.

Pugl is vaguely similar to libraries like GLUT and GLFW, but with some
distinguishing features:

 * Minimal in scope, providing only a thin interface to isolate
   platform-specific details from applications.

 * Zero dependencies, aside from standard system libraries.

 * Support for embedding in native windows, for example as a plugin or
   component within a larger application that is not based on Pugl.

 * Simple and extensible event-based API that makes dispatching in application
   or toolkit code easy with minimal boilerplate.

 * Suitable not only for continuously rendering applications like games, but
   also event-driven applications that only draw when necessary.

 * Explicit context and no static data whatsoever, so that several instances
   can be used within a single program at once.

 * Small, liberally licensed Free Software implementation that is suitable for
   vendoring and/or static linking.  Pugl can be installed as a library, or
   used by simply copying the headers into a project.

Stability
---------

Pugl is currently being developed towards a long-term stable API.  For the time
being, however, the API may break occasionally.  Please report any relevant
feedback, or file feature requests, so that we can ensure that the released API
is stable for as long as possible.

Documentation
-------------

Pugl is a C library that includes C++ bindings.
Each API is documented separately:

 * [C Documentation (single page)](https://lv2.gitlab.io/pugl/c/singlehtml/)
 * [C Documentation (paginated)](https://lv2.gitlab.io/pugl/c/html/)
 * [C++ Documentation (single page)](https://lv2.gitlab.io/pugl/cpp/singlehtml/)
 * [C++ Documentation (paginated)](https://lv2.gitlab.io/pugl/cpp/html/)

The documentation can also be built from the source by configuring with `--docs`.

Testing
-------

There are a few unit tests included, but unfortunately manual testing is still
required.  The tests and example programs will be built if you pass the
`--test` option when configuring:

    ./waf configure --test

Then, after building, the unit tests can be run:

    ./waf
    ./waf test --gui-tests

The `examples` directory contains several programs that serve as both manual
tests and demonstrations:

 * `pugl_embed_demo` shows a view embedded in another, and also tests
   requesting attention (which happens after 5 seconds), keyboard focus
   (switched by pressing tab), view moving (with the arrow keys), and view
   resizing (with the arrow keys while shift is held).  This program uses only
   very old OpenGL and should work on any system.

 * `pugl_window_demo` demonstrates multiple top-level windows.

 * `pugl_shader_demo` demonstrates using more modern OpenGL (version 3 or 4)
   where dynamic loading and shaders are required.  It can also be used to test
   performance by passing the number of rectangles to draw on the command line.

 * `pugl_cairo_demo` demonstrates using Cairo on top of the native windowing
   system (without OpenGL), and partial redrawing.

 * `pugl_print_events` is a utility that prints all received events to the
   console in a human readable format.

 * `pugl_cxx_demo` is a simple cube demo that uses the C++ API.

 * `pugl_vulkan_demo` is a simple example of using Vulkan in C that simply
   clears the window.

 * `pugl_vulkan_cxx_demo` is a more advanced Vulkan demo in C++ that draws many
   animated rectangles like `pugl_shader_demo`.

All example programs support several command line options to control various
behaviours, see the output of `--help` for details.  Please file an issue if
any of these programs do not work as expected on your system.

 -- David Robillard <d@drobilla.net>
