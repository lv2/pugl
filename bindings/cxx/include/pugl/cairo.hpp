/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef PUGL_CAIRO_HPP
#define PUGL_CAIRO_HPP

#include "pugl/cairo.h"
#include "pugl/pugl.h"

namespace pugl {

/**
   @defgroup cairoxx Cairo
   Cairo graphics support.
   @ingroup pugl_cxx
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
