// Copyright 2020-202y2 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef TRAJE_TRAJE_HPP
#define TRAJE_TRAJE_HPP

#include "traje.h"

#include <cstddef>
#include <string_view>

namespace traje {

class Tracer;

class Scope final
{
public:
  template<size_t N>
  Scope(Tracer& tracer, const char (&name)[N]) noexcept;

  Scope(const Scope&)            = delete;
  Scope& operator=(const Scope&) = delete;

  Scope(Scope&&)            = default;
  Scope& operator=(Scope&&) = delete;

  ~Scope() noexcept;

private:
  TrajeTracer* _tracer;
  const char*  _name;
};

class Tracer final
{
public:
  Tracer(const char* const process_name,
         const char* const thread_name,
         const char* const filename,
         const unsigned    queue_size) noexcept
    : _tracer{traje_tracer_new(process_name, thread_name, filename, queue_size)}
  {
    (void)filename;
    (void)queue_size;
  }

  Tracer(const Tracer&)            = delete;
  Tracer& operator=(const Tracer&) = delete;

  Tracer(Tracer&&)            = default;
  Tracer& operator=(Tracer&&) = delete;

  ~Tracer() noexcept { traje_tracer_free(_tracer); }

  template<size_t N>
  void mark(const char (&name)[N]) noexcept
  {
    traje_mark(_tracer, name);
    (void)name;
  }

  template<size_t N>
  Scope begin(const char (&name)[N]) noexcept
  {
    return Scope{*this, name};
  }

  void flush() noexcept { traje_flush(_tracer); }

  TrajeTracer* cobj() noexcept { return _tracer; }

private:
  TrajeTracer* _tracer;
};

template<size_t N>
inline Scope::Scope(Tracer& tracer, const char (&name)[N]) noexcept
  : _tracer{tracer.cobj()}
  , _name{name}
{
  traje_begin(_tracer, name);
}

inline Scope::~Scope() noexcept
{
  traje_end(_tracer, _name);
}

} // namespace traje

#endif // TRAJE_TRAJE_HPP
