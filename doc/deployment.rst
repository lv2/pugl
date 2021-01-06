#####
Usage
#####

*********************
Building Against Pugl
*********************

When Pugl is installed,
pkg-config_ packages are provided that link with the core platform library and desired backend:

- ``pugl-cairo-0``
- ``pugl-gl-0``
- ``pugl-vulkan-0``

Depending on one of these packages should be all that is necessary to use Pugl,
but details on the individual libraries that are installed are available in the README.

If you are instead including the source directly in your project,
the structure is quite simple and hopefully obvious.
It is only necessary to copy the platform and backend implementations that you need.

.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/
