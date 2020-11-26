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

#ifndef PUGL_GL_HPP
#define PUGL_GL_HPP

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl.hpp"

namespace pugl {

/**
   @defgroup glxx OpenGL
   OpenGL graphics support.
   @ingroup pugl_cxx
   @{
*/

/// @copydoc PuglGlFunc
using GlFunc = PuglGlFunc;

/// @copydoc puglGetProcAddress
inline GlFunc
getProcAddress(const char* name) noexcept
{
	return puglGetProcAddress(name);
}

/// @copydoc puglEnterContext
inline Status
enterContext(View& view) noexcept
{
	return static_cast<Status>(puglEnterContext(view.cobj()));
}

/// @copydoc puglLeaveContext
inline Status
leaveContext(View& view) noexcept
{
	return static_cast<Status>(puglLeaveContext(view.cobj()));
}

/// @copydoc puglGlBackend
inline const PuglBackend*
glBackend() noexcept
{
	return puglGlBackend();
}

/**
   @}
*/

} // namespace pugl

#endif // PUGL_GL_HPP
