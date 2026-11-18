// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "console_drm.h"

#include <drm.h>
#include <drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <fcntl.h>
#include <unistd.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>

static uint32_t
findId(const uint32_t        numFormats,
       const uint32_t* const formats,
       const uint32_t        format)
{
  for (uint32_t i = 0; i < numFormats; ++i) {
    if (formats[i] == format) {
      return i;
    }
  }

  return UINT32_MAX;
}

static drmModeEncoder*
findEncoder(const int                     drmFd,
            const drmModeRes* const       res,
            const drmModeConnector* const connector)
{
  for (int j = 0; j < connector->count_encoders; ++j) {
    drmModeEncoder* const encoder =
      drmModeGetEncoder(drmFd, connector->encoders[j]);

    if (!connector->encoder_id ||
        connector->encoder_id == encoder->encoder_id) {
      if (!encoder->crtc_id) {
        // No CRTC assigned, take the first valid one
        for (int k = 0; k < res->count_crtcs; ++k) {
          if ((encoder->possible_crtcs & (1U << k))) {
            encoder->crtc_id = res->crtcs[k];
            break;
          }
        }
      }

      if (encoder->crtc_id) {
        return encoder;
      }
    }

    drmModeFreeEncoder(encoder);
  }

  return NULL;
}

static int
getDrmModeProperty(const int                            drmFd,
                   const drmModeObjectProperties* const props,
                   const char* const                    name,
                   uint64_t* const                      value)
{
  for (unsigned i = 0; i < props->count_props; ++i) {
    drmModePropertyRes* const p = drmModeGetProperty(drmFd, props->props[i]);
    if (!strcmp(p->name, name)) {
      *value = props->prop_values[i];
      drmModeFreeProperty(p);
      return 0;
    }

    drmModeFreeProperty(p);
  }

  return -1;
}

static uint32_t
findPlaneFormat(const int                 drmFd,
                const uint32_t            numPreferredFormats,
                const uint32_t* const     preferredFormats,
                const drmModePlane* const plane,
                const uint32_t            crtcIndex,
                const uint32_t            crtcId)
{
  // Skip plane without properties
  drmModeObjectProperties* const props =
    drmModeObjectGetProperties(drmFd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
  if (!props) {
    return 0U;
  }

  // Skip non-primary plane
  uint64_t plane_type = 0U;
  if (getDrmModeProperty(drmFd, props, "type", &plane_type) < 0 ||
      plane_type != DRM_PLANE_TYPE_PRIMARY) {
    drmModeFreeObjectProperties(props);
    return 0U;
  }

  // Skip plane that isn't compatible with this CRTC
  if (!(plane->possible_crtcs & (1U << crtcIndex))) {
    drmModeFreeObjectProperties(props);
    return 0U;
  }

  // Skip plane assigned to another CRTC
  if (plane->crtc_id && plane->crtc_id != crtcId) {
    drmModeFreeObjectProperties(props);
    return 0U;
  }

  drmModeFreeObjectProperties(props);

  for (unsigned f = 0; f < numPreferredFormats; ++f) {
    if (findId(plane->count_formats, plane->formats, preferredFormats[f]) <
        plane->count_formats) {
      return preferredFormats[f];
    }
  }

  return 0U;
}

static uint32_t
findCrtcFormat(const int             drmFd,
               const uint32_t        numPreferredFormats,
               const uint32_t* const preferredFormats,
               const uint32_t        crtcIndex,
               const uint32_t        crtcId)
{
  uint32_t format = 0U;
  drmSetClientCap(drmFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

  drmModePlaneRes* const planeRes = drmModeGetPlaneResources(drmFd);
  if (planeRes) {
    // Search for a suitable plane with one of our desired formats
    for (unsigned i = 0U; !format && i < planeRes->count_planes; ++i) {
      drmModePlane* const plane = drmModeGetPlane(drmFd, planeRes->planes[i]);
      if (plane) {
        format = findPlaneFormat(drmFd,
                                 numPreferredFormats,
                                 preferredFormats,
                                 plane,
                                 crtcIndex,
                                 crtcId);
      }
      drmModeFreePlane(plane);
    }
  }

  drmModeFreePlaneResources(planeRes);
  drmSetClientCap(drmFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
  return format;
}

// Find the largest available connector mode within a given size range
static const drmModeModeInfo*
findMode(const drmModeConnector* const connector,
         const uint16_t                minWidth,
         const uint16_t                minHeight,
         const uint16_t                maxWidth,
         const uint16_t                maxHeight)
{
  const drmModeModeInfo* bestMode = NULL;

  for (int m = 0; m < connector->count_modes; ++m) {
    const drmModeModeInfo* const mode = &connector->modes[m];
    if (mode->hdisplay >= minWidth && mode->vdisplay >= minHeight &&
        mode->hdisplay <= maxWidth && mode->vdisplay <= maxHeight &&
        (!bestMode || ((mode->hdisplay * mode->vdisplay) >
                       (bestMode->hdisplay * bestMode->vdisplay)))) {
      bestMode = mode;
    }
  }

  return bestMode;
}

int
puglOpenDrm(PuglConsoleDrm* const drm,
            const char* const     device,
            const uint32_t        numPreferredFormats,
            const uint32_t* const preferredFormats,
            const uint16_t        minWidth,
            const uint16_t        minHeight,
            const uint16_t        maxWidth,
            const uint16_t        maxHeight)
{
  // Open DRI device
  const int drmFd = open(device, O_RDWR | O_CLOEXEC);
  if (drmFd < 0) {
    return -1;
  }

  // Get resources
  drmModeRes* const res = drmModeGetResources(drmFd);
  if (!res) {
    close(drmFd);
    return -1;
  }

  bool found = false;
  for (int i = 0; !found && i < res->count_connectors; ++i) {
    // Skip disconnected connector
    drmModeConnector* const connector =
      drmModeGetConnector(drmFd, res->connectors[i]);
    if (connector->connection != DRM_MODE_CONNECTED) {
      drmModeFreeConnector(connector);
      continue;
    }

    // Find a suitable encoder and CRTC
    drmModeEncoder* const encoder = findEncoder(drmFd, res, connector);
    if (!encoder) {
      drmModeFreeConnector(connector);
      continue;
    }

    const uint32_t connectorId = connector->connector_id;
    const uint32_t numCrtcs    = (uint32_t)res->count_crtcs;
    const uint32_t crtcId      = encoder->crtc_id;
    const uint32_t crtcIndex   = findId(numCrtcs, res->crtcs, crtcId);
    assert(crtcIndex < (uint32_t)res->count_crtcs);

    const uint32_t format = findCrtcFormat(
      drmFd, numPreferredFormats, preferredFormats, crtcIndex, crtcId);

    const drmModeModeInfo* const mode =
      findMode(connector, minWidth, minHeight, maxWidth, maxHeight);

    if (format && mode) {
      drm->fd          = drmFd;
      drm->connectorId = connectorId;
      drm->crtcId      = crtcId;
      drm->format      = format;
      drm->mode        = *mode;
      found            = true;
    }

    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
  }

  drmModeFreeResources(res);
  return !found;
}

void
puglCloseDrm(PuglConsoleDrm* const drm)
{
  close(drm->fd);
  memset(drm, 0, sizeof(*drm));
  drm->fd = -1;
}
