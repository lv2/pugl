// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <puglutil/demo_utils.h>
#include <puglutil/test_utils.h>

#include <pugl/gl.h>
#include <pugl/gl.hpp>
#include <pugl/pugl.hpp>

#include <cmath>

class CubeView : public pugl::View
{
public:
  explicit CubeView(pugl::World& world)
    : pugl::View{world}
  {
    setEventHandler(*this);
  }

  template<pugl::EventType t, class Base>
  pugl::Status onEvent(const pugl::Event<t, Base>&) noexcept
  {
    return pugl::Status::success;
  }

  pugl::Status onEvent(const pugl::UpdateEvent& event) noexcept;
  pugl::Status onEvent(const pugl::ExposeEvent& event) noexcept;
  pugl::Status onEvent(const pugl::KeyPressEvent& event) noexcept;
  pugl::Status onEvent(const pugl::CloseEvent& event) noexcept;

  bool quit() const { return _quit; }

private:
  double _xAngle{0.0};
  double _yAngle{0.0};
  double _lastDrawTime{0.0};
  bool   _quit{false};
};

pugl::Status
CubeView::onEvent(const pugl::UpdateEvent&) noexcept
{
  // Normally, we would obscure the view
  // return obscure();

  // But for testing, use sendEvent() instead:
  const auto currentSize = this->size(pugl::SizeHint::currentSize);
  return sendEvent(pugl::ExposeEvent{
    0U, pugl::Coord{0}, pugl::Coord{0}, currentSize.width, currentSize.height});
}

pugl::Status
CubeView::onEvent(const pugl::ExposeEvent&) noexcept
{
  static const float cubeStripVertices[] = {
    -1.0f, +1.0f, +1.0f, // Front top left
    +1.0f, +1.0f, +1.0f, // Front top right
    -1.0f, -1.0f, +1.0f, // Front bottom left
    +1.0f, -1.0f, +1.0f, // Front bottom right
    +1.0f, -1.0f, -1.0f, // Back bottom right
    +1.0f, +1.0f, +1.0f, // Front top right
    +1.0f, +1.0f, -1.0f, // Back top right
    -1.0f, +1.0f, +1.0f, // Front top left
    -1.0f, +1.0f, -1.0f, // Back top left
    -1.0f, -1.0f, +1.0f, // Front bottom left
    -1.0f, -1.0f, -1.0f, // Back bottom left
    +1.0f, -1.0f, -1.0f, // Back bottom right
    -1.0f, +1.0f, -1.0f, // Back top left
    +1.0f, +1.0f, -1.0f, // Back top right
  };

  static const float cubeStripColorVertices[] = {
    0.25f, 0.75f, 0.75f, // Front top left
    0.75f, 0.75f, 0.75f, // Front top right
    0.25f, 0.25f, 0.75f, // Front bottom left
    0.75f, 0.25f, 0.75f, // Front bottom right
    0.75f, 0.25f, 0.25f, // Back bottom right
    0.75f, 0.75f, 0.75f, // Front top right
    0.75f, 0.75f, 0.25f, // Back top right
    0.25f, 0.75f, 0.75f, // Front top left
    0.25f, 0.75f, 0.25f, // Back top left
    0.25f, 0.25f, 0.75f, // Front bottom left
    0.25f, 0.25f, 0.25f, // Back bottom left
    0.75f, 0.25f, 0.25f, // Back bottom right
    0.25f, 0.75f, 0.25f, // Back top left
    0.75f, 0.75f, 0.25f, // Back top right
  };

  static const float distance = 8.0f;

  const double thisTime = world().time();
  const double dTime    = thisTime - _lastDrawTime;
  const double dAngle   = dTime * 100.0;

  _xAngle = fmod(_xAngle + dAngle, 360.0);
  _yAngle = fmod(_yAngle + dAngle, 360.0);

  const auto currentSize = this->size(pugl::SizeHint::currentSize);

  const auto aspect = (static_cast<float>(currentSize.width) /
                       static_cast<float>(currentSize.height));

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CW);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, currentSize.width, currentSize.height);

  float projection[16];
  perspective(projection, 1.8f, aspect, 1.0f, 100.0f);
  glLoadMatrixf(projection);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, distance * -1.0f);
  glRotatef(static_cast<float>(_xAngle), 0.0f, 1.0f, 0.0f);
  glRotatef(static_cast<float>(_yAngle), 1.0f, 0.0f, 0.0f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, cubeStripVertices);
  glColorPointer(3, GL_FLOAT, 0, cubeStripColorVertices);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  _lastDrawTime = thisTime;

  return pugl::Status::success;
}

pugl::Status
CubeView::onEvent(const pugl::KeyPressEvent& event) noexcept
{
  if (event.key == 0x1BU || event.key == 'q') {
    _quit = true;
  }

  return pugl::Status::success;
}

pugl::Status
CubeView::onEvent(const pugl::CloseEvent&) noexcept
{
  _quit = true;

  return pugl::Status::success;
}

int
main(int argc, char** argv)
{
  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage("pugl_cpp_demo", "");
    return 1;
  }

  pugl::World    world{pugl::WorldType::program};
  CubeView       view{world};
  PuglFpsPrinter fpsPrinter{};

  world.setString(pugl::StringHint::className, "PuglCppDemo");

  view.setString(pugl::StringHint::windowTitle, "Pugl C++ Demo");
  view.setSizeHint(pugl::SizeHint::defaultSize, 512, 512);
  view.setSizeHint(pugl::SizeHint::minSize, 64, 64);
  view.setSizeHint(pugl::SizeHint::maxSize, 1024, 1024);
  view.setSizeHint(pugl::SizeHint::minAspect, 1, 1);
  view.setSizeHint(pugl::SizeHint::maxAspect, 16, 9);
  view.setBackend(pugl::glBackend());
  view.setHint(pugl::ViewHint::resizable, opts.resizable);
  view.setHint(pugl::ViewHint::samples, opts.samples);
  view.setHint(pugl::ViewHint::doubleBuffer, opts.doubleBuffer);
  view.setHint(pugl::ViewHint::swapInterval, opts.sync);
  view.setHint(pugl::ViewHint::ignoreKeyRepeat, opts.ignoreKeyRepeat);
  view.realize();
  view.show(pugl::ShowCommand::passive);

  unsigned framesDrawn = 0;
  while (!view.quit()) {
    world.update(0.0);

    ++framesDrawn;
    puglPrintFps(world.time(), &fpsPrinter, &framesDrawn);
  }

  return 0;
}
