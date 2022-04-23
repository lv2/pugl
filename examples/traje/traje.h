// Copyright 2020-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef TRAJE_H
#define TRAJE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TRAJE_ENABLED

#  include <stddef.h>

/// A type of event on the timeline
typedef enum {
  TRAJE_BEGIN_EVENT   = 'B', ///< Beginning of a duration
  TRAJE_END_EVENT     = 'E', ///< End of a duration
  TRAJE_INSTANT_EVENT = 'I', ///< Event with no duration
} TrajeEventType;

/// A scope measuring time for an ongoing event
typedef struct TrajeScopeImpl {
  const char* name; ///< Name/label for this event
} TrajeScope;

/// A tracer that records events in memory and occasionally flushes to the file
typedef struct TrajeTracerImpl TrajeTracer;

/// Create a new tracer to record events to a file
TrajeTracer*
traje_tracer_new(const char* filename, size_t queue_size);

/// Destroy a tracer created with traje_tracer_new()
void
traje_tracer_free(TrajeTracer* tracer);

/// Mark an instantaneous event with the current time
void
traje_mark(TrajeTracer* tracer, const char* name);

/// Begin recording a scope that starts at the current time
TrajeScope
traje_begin(TrajeTracer* tracer, const char* name);

/// Mark an event for a scope that ends at the current time
void
traje_end(TrajeTracer* tracer, TrajeScope scope);

/// Flush any pending events to the file
void
traje_flush(TrajeTracer* tracer);

#else // !TRAJE_ENABLED

struct TrajeScopeImpl {};
struct TrajeTracerImpl {};

typedef struct TrajeScopeImpl  TrajeScope;
typedef struct TrajeTracerImpl TrajeTracer;

static const TrajeScope traje_null_scope = {};

#  define traje_tracer_new(filename, queue_size) NULL
#  define traje_tracer_free(tracer) ((void)(tracer))
#  define traje_mark(tracer, name) ((void)(tracer))
#  define traje_begin(name) traje_null_scope
#  define traje_end(tracer, scope) ((void)(tracer))
#  define traje_flush(tracer) ((void)(tracer))
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TRAJE_H
