// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_CAIRO_HPP
#define PUGL_CAIRO_HPP

#include "pugl/cairo.h"
#include "pugl/pugl.h"

namespace pugl {

/**
   @defgroup cairopp Cairo
   Cairo graphics support.
   @ingroup puglpp
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
