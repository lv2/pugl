// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_CAIRO_HPP
#define PUGL_CAIRO_HPP

#include <pugl/cairo.h>
#include <pugl/pugl.h>

namespace pugl {

/**
   @defgroup pugl_cpp_cairo Cairo
   Cairo graphics support.
   @ingroup pugl_cpp
   @{
*/

/// @copydoc puglCairoBackend
inline const PuglBackend*
cairoBackend() noexcept
{
  return puglCairoBackend();
}

/**
   @}
*/

} // namespace pugl

#endif // PUGL_CAIRO_HPP
