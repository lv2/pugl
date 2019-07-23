Pugl
====

Pugl (PlUgin Graphics Library) is a minimal portable API for GUIs which is
suitable for use in plugins.  It works on X11, MacOS, and Windows, and
optionally supports OpenGL and Cairo graphics contexts.

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

 -- David Robillard <d@drobilla.net>
