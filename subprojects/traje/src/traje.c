// Copyright 2020-2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <traje/traje.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <time.h>
#  include <unistd.h>
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// IWYU pragma: no_include <features.h>

#ifdef _WIN32
typedef DWORD         TrajeProcessId;
typedef DWORD         TrajeThreadId;
typedef LARGE_INTEGER TrajeTimePoint;
#else
typedef pid_t           TrajeProcessId;
typedef pid_t           TrajeThreadId;
typedef struct timespec TrajeTimePoint;
#endif

#if UINTPTR_MAX > UINT32_MAX
static const uintptr_t time_mask  = 0x3FFFFFFFFFFFFFFFU;
static const unsigned  type_shift = 62U;
#else
static const uintptr_t time_mask  = 0x3FFFFFFFU;
static const unsigned  type_shift = 30U;
#endif

// A type of event on the timeline
typedef enum {
  TRAJE_BEGIN_EVENT,   // Beginning of a duration (B)
  TRAJE_END_EVENT,     // End of a duration (E)
  TRAJE_INSTANT_EVENT, // Event with no duration (i)
} TrajeEventType;

// A recorded event on the timeline (either an instant or a duration)
typedef struct TrajeEventImpl {
  const char* name; // Name/label for this event
  uintptr_t   data; // Type (top 2 bits) and timestamp
} TrajeEvent;

struct TrajeTracerImpl {
  FILE*          file;
  unsigned       events_capacity;
  unsigned       events_size;
  TrajeEvent*    events;
  TrajeTimePoint start_time;
  TrajeProcessId process_id;
  TrajeProcessId thread_id;
#ifdef _WIN32
  LARGE_INTEGER timer_frequency;
#endif
};

#ifdef _WIN32

static TrajeTimePoint
traje_now(void)
{
  LARGE_INTEGER count;
  QueryPerformanceCounter(&count);
  return count;
}

static uint64_t
traje_elapsed_us(const TrajeTracer* const tracer, const TrajeTimePoint end)
{
  static const LONGLONG us_per_s = 1000000;
  return (uint64_t)((end.QuadPart - tracer->start_time.QuadPart) * us_per_s /
                    tracer->timer_frequency.QuadPart);
}

#else

static TrajeTimePoint
traje_now(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now;
}

static uint64_t
traje_elapsed_us(const TrajeTracer* const tracer, const TrajeTimePoint end)
{
  static const uint64_t us_per_s  = 1000000U;
  static const uint64_t ns_per_us = 1000U;

  (void)tracer;

  const bool            sub_second = end.tv_nsec < tracer->start_time.tv_nsec;
  const struct timespec elapsed    = {
    end.tv_sec - tracer->start_time.tv_sec - (sub_second ? 1 : 0),
    end.tv_nsec - tracer->start_time.tv_nsec + (sub_second ? 1000000000L : 0L),
  };

  return ((uint64_t)elapsed.tv_sec * us_per_s) +
         ((uint64_t)elapsed.tv_nsec / ns_per_us);
}

#endif

TrajeTracer*
traje_tracer_new(const char* const process_name,
                 const char* const thread_name,
                 const char* const filename,
                 const unsigned    queue_size)
{
#if (defined(__GLIBC__) && \
     (__GLIBC__ > 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ >= 7))
#  define FOPEN_MODE "we"
#else
#  define FOPEN_MODE "w"
#endif

  FILE* const file = fopen(filename, FOPEN_MODE);
  if (!file) {
    return NULL;
  }

  TrajeEvent* const events =
    (TrajeEvent*)calloc(queue_size, sizeof(TrajeEvent));
  if (!events) {
    (void)fclose(file);
    return NULL;
  }

  TrajeTracer* const tracer = (TrajeTracer*)calloc(1, sizeof(TrajeTracer));
  if (!tracer) {
    (void)fclose(file);
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
#  if defined(HAVE_GETTID) && HAVE_GETTID
  tracer->thread_id = gettid();
#  endif
#endif

  (void)fprintf(
    file,
    "{\"traceEvents\":[\n"
    "{\"ph\":\"M\",\"name\":\"process_name\",\"pid\":%ld,\"tid\":%ld,\"ts\":0,"
    "\"args\":{\"name\":\"%s\"}},\n"
    "{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":%ld,\"tid\":%ld,\"ts\":0,"
    "\"args\":{\"name\":\"%s\"}}",
    (long)tracer->process_id,
    (long)tracer->thread_id,
    process_name,
    (long)tracer->process_id,
    (long)tracer->thread_id,
    thread_name);

  return tracer;
}

void
traje_tracer_free(TrajeTracer* const tracer)
{
  if (tracer) {
    traje_flush(tracer);
    (void)fprintf(tracer->file, "\n]}\n");
    (void)fclose(tracer->file);
    free(tracer->events);
    free(tracer);
  }
}

static void
traje_tracer_write_event(TrajeTracer* const   tracer,
                         const TrajeEventType type,
                         const char* const    name,
                         const uintptr_t      time_us)
{
  switch (type) {
  case TRAJE_BEGIN_EVENT:
    (void)fprintf(tracer->file,
                  ",\n{"
                  "\"ph\":\"B\","
                  "\"name\":\"%s\","
                  "\"pid\":%ld,\"tid\":%ld,"
                  "\"ts\":%" PRIuPTR "}",
                  name,
                  (long)tracer->process_id,
                  (long)tracer->thread_id,
                  time_us);
    break;
  case TRAJE_END_EVENT:
    (void)fprintf(tracer->file,
                  ",\n{"
                  "\"ph\":\"E\","
                  "\"name\":\"%s\","
                  "\"pid\":%ld,\"tid\":%ld,"
                  "\"ts\":%" PRIuPTR "}",
                  name,
                  (long)tracer->process_id,
                  (long)tracer->thread_id,
                  time_us);
    break;
  case TRAJE_INSTANT_EVENT:
    (void)fprintf(tracer->file,
                  ",\n{"
                  "\"ph\":\"I\",\"s\":\"g\","
                  "\"cat\":\"user\","
                  "\"name\":\"%s\","
                  "\"pid\":%ld,\"tid\":%ld,"
                  "\"ts\":%" PRIuPTR "}",
                  name,
                  (long)tracer->process_id,
                  (long)tracer->thread_id,
                  time_us);
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
  }

  const uint64_t   time_us = traje_elapsed_us(tracer, time);
  const TrajeEvent event   = {
    name,
    ((uintptr_t)type << type_shift) | ((uintptr_t)time_us & time_mask),
  };

  tracer->events[tracer->events_size++] = event;
}

void
traje_mark(TrajeTracer* const tracer, const char* const name)
{
  traje_record_event(tracer, TRAJE_INSTANT_EVENT, name, traje_now());
}

void
traje_begin(TrajeTracer* const tracer, const char* const name)
{
  traje_record_event(tracer, TRAJE_BEGIN_EVENT, name, traje_now());
}

void
traje_end(TrajeTracer* const tracer, const char* const name)
{
  traje_record_event(tracer, TRAJE_END_EVENT, name, traje_now());
}

void
traje_flush(TrajeTracer* const tracer)
{
  for (unsigned i = 0U; i < tracer->events_size; ++i) {
    const TrajeEvent* const event = &tracer->events[i];
    traje_tracer_write_event(tracer,
                             (TrajeEventType)(event->data >> type_shift),
                             event->name,
                             event->data & time_mask);
  }

  tracer->events_size = 0;
}
