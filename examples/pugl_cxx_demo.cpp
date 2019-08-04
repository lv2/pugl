/*
  Copyright 2012-2020 David Robillard <http://drobilla.net>

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

/**
   @file pugl_cxx_demo.cpp A simple demo of the Pugl C++ API.
*/

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl.hpp"
#include "pugl/pugl_gl.hpp"

#include <cmath>

struct CubeData {
	double   xAngle{0.0};
	double   yAngle{0.0};
	double   lastDrawTime{0.0};
	unsigned framesDrawn{0};
	bool     quit{false};
};

using CubeView = pugl::View<CubeData>;

static pugl::Status
onConfigure(CubeView&, const pugl::ConfigureEvent& event)
{
	reshapeCube(static_cast<float>(event.width),
	            static_cast<float>(event.height));

	return pugl::Status::success;
}

static pugl::Status
onUpdate(CubeView& view, const pugl::UpdateEvent&)
{
	return view.postRedisplay();
}

static pugl::Status
onExpose(CubeView& view, const pugl::ExposeEvent&)
{
	const pugl::World& world    = view.world();
	CubeData&          data     = view.data();
	const double       thisTime = world.time();
	const double       dTime    = thisTime - data.lastDrawTime;
	const double       dAngle   = dTime * 100.0;

	data.xAngle = fmod(data.xAngle + dAngle, 360.0);
	data.yAngle = fmod(data.yAngle + dAngle, 360.0);
	displayCube(view.cobj(),
	            8.0f,
	            static_cast<float>(data.xAngle),
	            static_cast<float>(data.yAngle),
	            false);

	data.lastDrawTime = thisTime;
	++data.framesDrawn;

	return pugl::Status::success;
}

static pugl::Status
onKeyPress(CubeView& view, const pugl::KeyPressEvent& event)
{
	if (event.key == PUGL_KEY_ESCAPE || event.key == 'q') {
		view.data().quit = true;
	}

	return pugl::Status::success;
}

int
main(int argc, char** argv)
{
	const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);

	pugl::World    world{pugl::WorldType::program};
	CubeView       view{world};
	PuglFpsPrinter fpsPrinter{};

	world.setClassName("PuglCppTest");

	view.setWindowTitle("Pugl C++ Test");
	view.setFrame({0, 0, 512, 512});
	view.setMinSize(64, 64);
	view.setAspectRatio(1, 1, 16, 9);
	view.setBackend(pugl::glBackend());
	view.setHint(pugl::ViewHint::resizable, opts.resizable);
	view.setHint(pugl::ViewHint::samples, opts.samples);
	view.setHint(pugl::ViewHint::doubleBuffer, opts.doubleBuffer);
	view.setHint(pugl::ViewHint::swapInterval, opts.doubleBuffer);
	view.setHint(pugl::ViewHint::ignoreKeyRepeat, opts.ignoreKeyRepeat);
	view.setEventFunc(onConfigure);
	view.setEventFunc(onUpdate);
	view.setEventFunc(onExpose);
	view.setEventFunc(onKeyPress);
	view.realize();
	view.showWindow();

	while (!view.data().quit) {
		world.update(0.0);

		puglPrintFps(world.cobj(), &fpsPrinter, &view.data().framesDrawn);
	}

	return 0;
}
