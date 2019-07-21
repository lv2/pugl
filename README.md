Pugl
====

Pugl (PlUgin Graphics Library) is a minimal portable API for GUIs which is
suitable for use in plugins.  It works on X11, MacOS, and Windows, and supports
OpenGL and Cairo graphics contexts.

Pugl is vaguely similar to other libraries like GLUT and GLFW, but with some
significant distinctions:

 * Minimal in scope, providing only a small interface to isolate
   platform-specific details from applications.

 * Support for embedding in other windows, so Pugl can be used to draw several
   "widgets" inside a larger GUI.

 * Simple and extensible event-based API that makes dispatching in application
   or toolkit code easy without too much boilerplate.

 * Unlike GLFW, context is explicit and there is no static data whatsoever, so
   Pugl can be used in plugins or several independent parts of a program.

 * Unlike GLUT, there is a single implementation which is modern, small,
   liberally licensed Free Software, and suitable for vendoring and static
   linking to avoid dependency problems.

 * More complete support for keyboard input than GLUT, including additional
   "special" keys, modifiers, and international text input.

For more information, see <http://drobilla.net/software/pugl>.

 -- David Robillard <d@drobilla.net>
