// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_STUB_HPP
#define PUGL_STUB_HPP

#include "pugl/pugl.h"
#include "pugl/stub.h"

namespace pugl {

/**
   @defgroup stubpp Stub
   Stub graphics support.
   @ingroup puglpp
   @{
*/

/// @copydoc puglStubBackend
inline const PuglBackend*
stubBackend() noexcept
{
  return puglStubBackend();
}

/**
   @}
*/

} // namespace pugl

#endif // PUGL_STUB_HPP
