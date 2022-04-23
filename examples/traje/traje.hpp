// Copyright 2020-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef TRAJE_HPP
#define TRAJE_HPP

#include "traje.h"

#include <cstddef>
#include <string_view>

namespace traje {

class Tracer;

class Scope final : public TrajeScope
{
public:
  Scope(Tracer& tracer, TrajeScope c_scope) noexcept;

  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;

  Scope(Scope&&) = default;
  Scope& operator=(Scope&&) = delete;

  ~Scope() noexcept;

private:
  Tracer& _tracer;
};

class Tracer final
{
public:
  Tracer(const std::string_view filename, const size_t queue_size) noexcept
    : _tracer{traje_tracer_new(filename.data(), queue_size)}
  {}

  Tracer(const Tracer&) = delete;
  Tracer& operator=(const Tracer&) = delete;

  Tracer(Tracer&&) = default;
  Tracer& operator=(Tracer&&) = delete;

  ~Tracer() noexcept { traje_tracer_free(_tracer); }

  template<size_t N>
  void mark(const char (&name)[N]) noexcept
  {
    traje_mark(_tracer, name);
  }

  template<size_t N>
  Scope begin(const char (&name)[N]) noexcept
  {
    return Scope{*this, traje_begin(_tracer, name)};
  }

  void flush() noexcept { traje_flush(_tracer); }

  TrajeTracer* cobj() { return _tracer; }

private:
  TrajeTracer* _tracer;
};

inline Scope::Scope(Tracer& tracer, const TrajeScopeImpl c_scope) noexcept
  : TrajeScopeImpl{c_scope}
  , _tracer{tracer}
{}

inline Scope::~Scope() noexcept
{
  traje_end(_tracer.cobj(), *this);
}

} // namespace traje

#endif // TRAJE_HPP
