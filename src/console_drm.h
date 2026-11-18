// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_CONSOLE_DRM_H
#define PUGL_SRC_CONSOLE_DRM_H

#include <xf86drmMode.h>

#include <stdint.h>

typedef struct {
  int             fd;
  uint32_t        connectorId;
  uint32_t        crtcId;
  uint32_t        format;
  drmModeModeInfo mode;
} PuglConsoleDrm;

int
puglOpenDrm(PuglConsoleDrm* drm,
            const char*     device,
            uint32_t        numPreferredFormats,
            const uint32_t* preferredFormats,
            uint16_t        minWidth,
            uint16_t        minHeight,
            uint16_t        maxWidth,
            uint16_t        maxHeight);

void
puglCloseDrm(PuglConsoleDrm* drm);

#endif // PUGL_SRC_CONSOLE_DRM_H
