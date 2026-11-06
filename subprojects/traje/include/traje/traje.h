// Copyright 2020-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef TRAJE_TRAJE_H
#define TRAJE_TRAJE_H

#ifndef TRAJE_ENABLED
#  include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// A tracer that records events in memory and occasionally flushes to file
typedef struct TrajeTracerImpl TrajeTracer;

#ifdef TRAJE_ENABLED // API

/// Create a new tracer to record events to a file
TrajeTracer*
traje_tracer_new(const char* process_name,
                 const char* thread_name,
                 const char* filename,
                 unsigned    queue_size);

/// Destroy a tracer created with traje_tracer_new()
void
traje_tracer_free(TrajeTracer* tracer);

/// Mark an instantaneous event with the current time
void
traje_mark(TrajeTracer* tracer, const char* name);

/// Begin recording a scope that starts at the current time
void
traje_begin(TrajeTracer* tracer, const char* name);

/// Mark an event for a scope that ends at the current time
void
traje_end(TrajeTracer* tracer, const char* name);

/// Flush any pending events to the file
void
traje_flush(TrajeTracer* tracer);

#else // Stub implementation

static inline TrajeTracer*
traje_tracer_new(const char* const process_name,
                 const char* const thread_name,
                 const char* const filename,
                 const unsigned    queue_size)
{
  (void)process_name;
  (void)thread_name;
  (void)filename;
  (void)queue_size;
  return NULL;
}

static inline void
traje_tracer_free(TrajeTracer* const tracer)
{
  (void)tracer;
}

static inline void
traje_mark(TrajeTracer* const tracer, const char* const name)
{
  (void)tracer;
  (void)name;
}

static inline void
traje_begin(TrajeTracer* const tracer, const char* const name)
{
  (void)tracer;
  (void)name;
}

static inline void
traje_end(TrajeTracer* const tracer, const char* const name)
{
  (void)tracer;
  (void)name;
}

static inline void
traje_flush(TrajeTracer* const tracer)
{
  (void)tracer;
}

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TRAJE_TRAJE_H
