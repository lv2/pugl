// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_TYPES_H
#define PUGL_SRC_TYPES_H

#include "attributes.h"

#include <pugl/pugl.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Platform-specific world internals
typedef struct PuglWorldInternalsImpl PuglWorldInternals;

/// Platform-specific view internals
typedef struct PuglInternalsImpl PuglInternals;

/// View hints
typedef int PuglHints[PUGL_NUM_VIEW_HINTS];

/// Blob of arbitrary data
typedef struct {
  void*  data; ///< Dynamically allocated data
  size_t len;  ///< Length of data in bytes
} PuglBlob;

/// State of the world in the process of an update
typedef enum {
  PUGL_WORLD_IDLE,      ///< Idle, not in puglUpdate()
  PUGL_WORLD_UPDATING,  ///< Event processing stage of puglUpdate()
  PUGL_WORLD_EXPOSING,  ///< Exposing stage of puglUpdate()
  PUGL_WORLD_RECURSING, ///< Currently in recursive loop (Windows)
} PuglWorldState;

/// Stage of a view along its lifespan
typedef enum {
  PUGL_VIEW_STAGE_ALLOCATED,
  PUGL_VIEW_STAGE_REALIZED,
  PUGL_VIEW_STAGE_CONFIGURED,
} PuglViewStage;

/// Drawing state of a view during an update
typedef enum {
  PUGL_ALLOCATED, ///< Allocated but not yet realized
  PUGL_CREATED,   ///< Realized (and PUGL_CREATE has been handled)
  PUGL_BLANKED,   ///< Blanked in preparation of exposure
  PUGL_EXPOSED,   ///< Exposed at least once after blanking
} PuglDrawState;

/// Cross-platform view definition
struct PuglViewImpl {
  PuglWorld*         world;
  const PuglBackend* backend;
  PuglInternals*     impl;
  PuglHandle         handle;
  PuglEventFunc      eventFunc;
  PuglNativeView     parent;
  uintptr_t          transientParent;
  PuglConfigureEvent lastConfigure;
  PuglEvent          pendingExpose;
  PuglHints          hints;
  PuglPoint          positionHints[PUGL_NUM_POSITION_HINTS];
  PuglArea           sizeHints[PUGL_NUM_SIZE_HINTS];
  char*              strings[PUGL_NUM_STRING_HINTS];
  PuglViewStage      stage;
  PuglDrawState      drawState;
  bool               resizing;
};

/// Cross-platform world definition
struct PuglWorldImpl {
  PuglWorldInternals* impl;
  PuglWorldHandle     handle;
  double              startTime;
  size_t              numViews;
  PuglView**          views;
  char*               strings[PUGL_NUM_STRING_HINTS];
  PuglWorldType       type;
  PuglWorldState      state;
};

/// Opaque surface used by graphics backend
typedef void PuglSurface;

/// Backend flags that influence the main implementation
typedef enum {
  PUGL_BACKEND_EXCLUSIVE = 1U << 0U, // Views are entered exclusively
} PuglBackendFlag;

/// Graphics backend interface
struct PuglBackendImpl {
  /// Bitwise OR of PuglBackendFlag values
  unsigned flags;

  /// Get visual information from display and setup view as necessary
  PUGL_WARN_UNUSED_RESULT PuglStatus (*configure)(PuglView*);

  /// Create surface and drawing context
  PUGL_WARN_UNUSED_RESULT PuglStatus (*create)(PuglView*);

  /// Destroy surface and drawing context
  void (*destroy)(PuglView*);

  /**
     Enter context, for drawing if parameter is true.

     @return #PUGL_SUCCESS if the context was successfully entered, or an
     error.
  */
  PUGL_WARN_UNUSED_RESULT PuglStatus (*enter)(PuglView*, bool);

  /**
     Leave context, after drawing if parameter is true.

     @return #PUGL_SUCCESS if the context was successfully left/flushed, or an
     error.
  */
  PUGL_WARN_UNUSED_RESULT PuglStatus (*leave)(PuglView*, bool);

  /**
     Handle an event for a view.

     This is a hook that can be used by the backend to implement
     rendering-related events.  Currently, this is sent expose events with the
     context entered, allowing backends to track and update damaged regions.

     @return #PUGL_SUCCESS if handling the event was successful and should
     continue, or an error.
  */
  PUGL_WARN_UNUSED_RESULT PuglStatus (*handle)(PuglView*, const PuglEvent*);

  /// Return the puglGetContext() handle for the application if it is entered
  void* (*getContext)(PuglView*);
};

#endif // PUGL_SRC_TYPES_H
