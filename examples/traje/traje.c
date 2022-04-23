// Copyright 2020-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "traje.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <time.h>
#  include <unistd.h>
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
typedef DWORD         TrajeProcessId;
typedef DWORD         TrajeThreadId;
typedef LARGE_INTEGER TrajeTimePoint;
#else
typedef pid_t           TrajeProcessId;
typedef pid_t           TrajeThreadId;
typedef struct timespec TrajeTimePoint;
#endif

/// A recorded event on the timeline (either an instant or a duration)
typedef struct TrajeEventImpl {
  const char*    name; ///< Name/label for this event
  TrajeTimePoint time; ///< Global time of this event
  TrajeEventType type; ///< Event type
} TrajeEvent;

struct TrajeTracerImpl {
  FILE*          file;
  size_t         events_capacity;
  size_t         events_size;
  TrajeEvent*    events;
  TrajeTimePoint start_time;
  TrajeProcessId process_id;
  TrajeProcessId thread_id;
#ifdef _WIN32
  LARGE_INTEGER timer_frequency;
#endif
};

static const double s_per_ns = 1e-9;
static const double us_per_s = 1e6;

#ifdef _WIN32

static double
traje_elapsed_us(const TrajeTracer* const tracer,
                 const TrajeTimePoint     start,
                 const TrajeTimePoint     end)
{
  return (double)(end.QuadPart - start.QuadPart) * 1e6 /
         (double)tracer->timer_frequency.QuadPart;
}

TrajeTimePoint
traje_now(void)
{
  LARGE_INTEGER count;
  QueryPerformanceCounter(&count);
  return count;
}

#else

static double
traje_microseconds(const TrajeTracer* const tracer, const TrajeTimePoint time)
{
  (void)tracer;

  return ((double)time.tv_sec + ((double)time.tv_nsec * s_per_ns)) * us_per_s;
}

static double
traje_elapsed_us(const TrajeTracer* const tracer,
                 const TrajeTimePoint     start,
                 const TrajeTimePoint     end)
{
  (void)tracer;

  assert(start.tv_sec <= end.tv_sec);

  const bool           sub_second = end.tv_nsec < start.tv_nsec;
  const TrajeTimePoint difference = {
    end.tv_sec - start.tv_sec - (sub_second ? 1 : 0),
    end.tv_nsec - start.tv_nsec + (sub_second ? 1000000000L : 0L),
  };

  return traje_microseconds(tracer, difference);
}

static TrajeTimePoint
traje_now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return ts;
}

#endif

TrajeTracer*
traje_tracer_new(const char* const filename, const size_t queue_size)
{
  FILE* file = fopen(filename, "w");
  if (!file) {
    return NULL;
  }

  TrajeEvent* events = (TrajeEvent*)calloc(queue_size, sizeof(TrajeEvent));
  if (!events) {
    fclose(file);
    return NULL;
  }

  TrajeTracer* const tracer = (TrajeTracer*)calloc(1, sizeof(TrajeTracer));
  if (!tracer) {
    fclose(file);
    free(events);
    return NULL;
  }

  tracer->file            = file;
  tracer->events_capacity = queue_size;
  tracer->events_size     = 0;
  tracer->events          = events;
  tracer->start_time      = traje_now();

#ifdef _WIN32
  tracer->process_id = GetCurrentProcessId();
  tracer->thread_id  = GetCurrentThreadId();
  QueryPerformanceFrequency(&tracer->timer_frequency);
#else
  tracer->process_id = getpid();
  tracer->thread_id  = gettid();
#endif

  fprintf(file, "{\"traceEvents\":[\n");

  fprintf(file,
          "{\"ph\":\"M\",\"pid\":%ld,\"tid\":%ld,\"ts\":0,"
          "\"name\":\"thread_name\",\"args\":{\"name\":\"main\"}}",
          (long)tracer->process_id,
          (long)tracer->thread_id);

  return tracer;
}

void
traje_tracer_free(TrajeTracer* const tracer)
{
  traje_flush(tracer);
  fprintf(tracer->file, "\n]}\n");

  fclose(tracer->file);
  free(tracer->events);
  free(tracer);
}

static void
traje_tracer_write_event(TrajeTracer* const   tracer,
                         const TrajeEventType type,
                         const char* const    name,
                         const TrajeTimePoint time)
{
  switch (type) {
  case TRAJE_BEGIN_EVENT:
    fprintf(tracer->file,
            ",\n{"
            "\"ph\":\"B\","
            "\"name\":\"%s\","
            "\"pid\":%ld,\"tid\":%ld,"
            "\"ts\":%.3f"
            "}",
            name,
            (long)tracer->process_id,
            (long)tracer->thread_id,
            traje_elapsed_us(tracer, tracer->start_time, time));
    break;
  case TRAJE_END_EVENT:
    fprintf(tracer->file,
            ",\n{"
            "\"ph\":\"E\","
            "\"name\":\"%s\","
            "\"pid\":%ld,\"tid\":%ld,"
            "\"ts\":%.3f"
            "}",
            name,
            (long)tracer->process_id,
            (long)tracer->thread_id,
            traje_elapsed_us(tracer, tracer->start_time, time));
    break;
  case TRAJE_INSTANT_EVENT:
    fprintf(tracer->file,
            ",\n{"
            "\"ph\":\"I\",\"s\":\"g\","
            "\"cat\":\"user\","
            "\"name\":\"%s\","
            "\"pid\":%ld,\"tid\":%ld,"
            "\"ts\":%.3f"
            "}",
            name,
            (long)tracer->process_id,
            (long)tracer->thread_id,
            traje_elapsed_us(tracer, tracer->start_time, time));
    break;
  }
}

static void
traje_record_event(TrajeTracer* const   tracer,
                   const TrajeEventType type,
                   const char* const    name,
                   TrajeTimePoint const time)
{
  if (tracer->events_size == tracer->events_capacity) {
    traje_flush(tracer);
    assert(tracer->events_size == 0);
  }

  const TrajeEvent event = {name, time, type};

  tracer->events[tracer->events_size++] = event;
}

void
traje_mark(TrajeTracer* const tracer, const char* const name)
{
  traje_record_event(tracer, TRAJE_INSTANT_EVENT, name, traje_now());
}

void
traje_flush(TrajeTracer* const tracer)
{
  for (size_t i = 0; i < tracer->events_size; ++i) {
    const TrajeEvent* event = &tracer->events[i];
    traje_tracer_write_event(tracer, event->type, event->name, event->time);
  }

  tracer->events_size = 0;
}

TrajeScope
traje_begin(TrajeTracer* const tracer, const char* const name)
{
  traje_record_event(tracer, TRAJE_BEGIN_EVENT, name, traje_now());

  const TrajeScope scope = {name};
  return scope;
}

void
traje_end(TrajeTracer* const tracer, const TrajeScope scope)
{
  traje_record_event(tracer, TRAJE_END_EVENT, scope.name, traje_now());
}
