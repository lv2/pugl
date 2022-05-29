#####
Usage
#####

*********************
Building Against Pugl
*********************

When Pugl is installed,
pkg-config_ packages are provided that link with the core platform library and desired backend:

.. code-block:: sh

   pkg-config --cflags --libs pugl-0
   pkg-config --cflags --libs pugl-cairo-0
   pkg-config --cflags --libs pugl-gl-0
   pkg-config --cflags --libs pugl-vulkan-0

Depending on one of these packages should be all that is necessary to use Pugl,
but details on the individual libraries that are installed are available in the README.

If you are instead building directly from source,
all of the implementation files are in the ``src`` directory.
It is only necessary to build the platform and backend implementations that you need.

.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/
