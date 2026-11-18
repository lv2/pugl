// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "console.h"
#include "console_drm.h"
#include "egl_surface.h"
#include "stub.h"
#include "types.h"

#include <pugl/gl.h>
#include <pugl/pugl.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <sys/select.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct gbm_bo;

typedef struct {
  PuglConsoleDrm      drm;
  struct gbm_device*  gbmDevice;
  struct gbm_surface* gbmSurface;
  PuglEglSurface      egl;
  struct gbm_bo*      bo;
  int                 waitingForFlip;
} PuglConsoleGlSurface;

static int
rankConfig(EGLDisplay display, EGLConfig config)
{
  EGLint gbmFormat = 0;
  if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &gbmFormat)) {
    return 0;
  }

  return (gbmFormat == GBM_FORMAT_XRGB8888)   ? 2
         : (gbmFormat == GBM_FORMAT_ARGB8888) ? 1
                                              : 0;
}

static PuglStatus
puglConsoleGlAttach(PuglView* const view)
{
  static const char* const device = "/dev/dri/card1";

  PuglStatus st = PUGL_SUCCESS;

  PuglConsoleGlSurface* const surface =
    (PuglConsoleGlSurface*)calloc(1, sizeof(PuglConsoleGlSurface));

  // Get acceptable range of sizes / display modes
  const PuglArea minSize = puglGetSizeHint(view, PUGL_MIN_SIZE);
  PuglArea       maxSize = puglGetSizeHint(view, PUGL_MAX_SIZE);
  if (!maxSize.width || !maxSize.height) {
    maxSize.width  = INT16_MAX;
    maxSize.height = INT16_MAX;
  }

  static const uint32_t drmFormats[] = {
    DRM_FORMAT_XRGB8888,
    DRM_FORMAT_ARGB8888,
  };

  static const uint32_t numDrmFormats = 2U;

  // Open DRM and find a display with an appropriate mode
  if (puglOpenDrm(&surface->drm,
                  device,
                  numDrmFormats,
                  drmFormats,
                  minSize.width,
                  minSize.height,
                  maxSize.width,
                  maxSize.height)) {
    free(surface);
    return PUGL_UNKNOWN_ERROR;
  }

  (void)fprintf(stderr,
                "Connector %u CRTC %u format %c%c%c%c mode %ux%u @ %u Hz\n",
                surface->drm.connectorId,
                surface->drm.crtcId,
                ((const char*)&surface->drm.format)[0],
                ((const char*)&surface->drm.format)[1],
                ((const char*)&surface->drm.format)[2],
                ((const char*)&surface->drm.format)[3],
                surface->drm.mode.hdisplay,
                surface->drm.mode.vdisplay,
                surface->drm.mode.vrefresh);

  // Open GBM
  surface->gbmDevice = gbm_create_device(surface->drm.fd);
  if (!surface->gbmDevice) {
    puglCloseDrm(&surface->drm);
    free(surface);
    return PUGL_UNKNOWN_ERROR;
  }

  // Create GBM surface
  surface->gbmSurface = gbm_surface_create(
    surface->gbmDevice,
    surface->drm.mode.hdisplay,
    surface->drm.mode.vdisplay,
    (surface->drm.format == DRM_FORMAT_XRGB8888) ? GBM_FORMAT_XRGB8888
                                                 : GBM_FORMAT_ARGB8888,
    GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  if (!surface->gbmSurface) {
    gbm_surface_destroy(surface->gbmSurface);
    puglCloseDrm(&surface->drm);
    free(surface);
    return PUGL_UNKNOWN_ERROR;
  }

  // Create an EGL surface for the GBM surface
  if ((st = puglEglAttach(&surface->egl,
                          surface->gbmDevice,
                          (EGLNativeWindowType)surface->gbmSurface,
                          rankConfig,
                          view->hints))) {
    gbm_surface_destroy(surface->gbmSurface);
    puglCloseDrm(&surface->drm);
    free(surface);
    return st;
  }

  view->impl->surface = surface;
  return PUGL_SUCCESS;
}

static void
puglConsoleGlDetach(PuglView* const view)
{
  PuglConsoleGlSurface* const surface =
    (PuglConsoleGlSurface*)view->impl->surface;

  if (surface) {
    puglEglDetach(&surface->egl);
    gbm_surface_destroy(surface->gbmSurface);
    puglCloseDrm(&surface->drm);
    free(surface);
    view->impl->surface = NULL;
  }
}

typedef struct {
  int      drmFd;
  uint32_t fbId;
} Buffer;

static void
destroyBufferCallback(struct gbm_bo* const bo, void* const data)
{
  (void)bo;

  Buffer* const buffer = (Buffer*)data;

  if (buffer->fbId) {
    drmModeRmFB(buffer->drmFd, buffer->fbId);
  }

  free(buffer);
}

static struct gbm_bo*
lockFrontBuffer(PuglConsoleGlSurface* const surface)
{
  // Must always be called right after eglSwapBuffers
  struct gbm_bo* const bo = gbm_surface_lock_front_buffer(surface->gbmSurface);

  Buffer* buffer = (Buffer*)gbm_bo_get_user_data(bo);
  if (!buffer) {
    buffer        = (Buffer*)calloc(1U, sizeof(Buffer));
    buffer->drmFd = surface->drm.fd;

    const uint32_t handles[4] = {gbm_bo_get_handle(bo).u32, 0U, 0U, 0U};
    const uint32_t pitches[4] = {gbm_bo_get_stride(bo), 0U, 0U, 0U};
    const uint32_t offsets[4] = {0U};

    if (drmModeAddFB2(surface->drm.fd,
                      gbm_bo_get_width(bo),
                      gbm_bo_get_height(bo),
                      gbm_bo_get_format(bo),
                      handles,
                      pitches,
                      offsets,
                      &buffer->fbId,
                      0)) {
      free(buffer);
      return NULL;
    }

    gbm_bo_set_user_data(bo, buffer, destroyBufferCallback);
  }

  return bo;
}

static PuglStatus
puglConsoleGlCreate(PuglView* const view)
{
  PuglConsoleGlSurface* const surface =
    (PuglConsoleGlSurface*)view->impl->surface;

  assert(!surface->bo);

  PuglStatus st = puglEglCreate(&surface->egl, view->hints);
  if (st || (st = puglEglEnter(&surface->egl, false))) {
    return st;
  }

  // Do an initial buffer swap so we can lock a front buffer
  eglSwapBuffers(surface->egl.display, surface->egl.surface);
  surface->bo = lockFrontBuffer(surface);
  puglEglLeave(&surface->egl, false);

  Buffer* const buffer = (Buffer*)gbm_bo_get_user_data(surface->bo);
  const int     rc     = drmModeSetCrtc(surface->drm.fd,
                                        surface->drm.crtcId,
                                        buffer->fbId,
                                        0,
                                        0,
                                        &surface->drm.connectorId,
                                        1,
                                        &surface->drm.mode);

  return rc ? PUGL_UNKNOWN_ERROR : PUGL_SUCCESS;
}

static void
puglConsoleGlDestroy(PuglView* const view)
{
  PuglConsoleGlSurface* const surface =
    (PuglConsoleGlSurface*)view->impl->surface;

  if (surface->bo) {
    // Release "next" buffer that will never be used
    gbm_surface_release_buffer(surface->gbmSurface, surface->bo);
    surface->bo = NULL;
  }

  puglEglDestroy(&surface->egl);
}

static PuglStatus
puglConsoleGlEnter(PuglView* const view, bool expose)
{
  PuglConsoleGlSurface* const surface =
    (PuglConsoleGlSurface*)view->impl->surface;

  assert(surface->bo);
  return puglEglEnter(&surface->egl, expose);
}

static void
onPageFlip(const int      fd,
           const unsigned frame,
           const unsigned sec,
           const unsigned usec,
           void* const    data)
{
  (void)fd;
  (void)frame;
  (void)sec;
  (void)usec;

  int* const waitingForFlip = (int*)data;
  *waitingForFlip           = 0;
}

static PuglStatus
puglConsoleGlLeave(PuglView* const view, bool expose)
{
  PuglConsoleGlSurface* const surface =
    (PuglConsoleGlSurface*)view->impl->surface;

  assert(surface->bo);

  const PuglStatus st = puglEglLeave(&surface->egl, expose);
  if (expose) {
    struct gbm_bo* const nextBo = lockFrontBuffer(surface);
    Buffer* const        buffer = (Buffer*)gbm_bo_get_user_data(nextBo);

    surface->waitingForFlip = 1;

    int rc = drmModePageFlip(surface->drm.fd,
                             surface->drm.crtcId,
                             buffer->fbId,
                             DRM_MODE_PAGE_FLIP_EVENT,
                             &surface->waitingForFlip);
    if (rc) {
      return PUGL_UNKNOWN_ERROR;
    }

    while (surface->waitingForFlip) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(surface->drm.fd, &fds);

      rc = select(surface->drm.fd + 1, &fds, NULL, NULL, NULL);
      if (rc < 0) {
        return PUGL_UNKNOWN_ERROR;
      }

      if (!rc) {
        return PUGL_FAILURE;
      }

      drmEventContext eventContext = {
        .version           = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = onPageFlip,
      };
      drmHandleEvent(surface->drm.fd, &eventContext);
    }

    gbm_surface_release_buffer(surface->gbmSurface, surface->bo);
    surface->bo = nextBo;
  }

  return st;
}

PuglGlFunc
puglGetProcAddress(const char* const name)
{
  return eglGetProcAddress(name);
}

PuglStatus
puglEnterContext(PuglView* view)
{
  return view->backend->enter(view, false);
}

PuglStatus
puglLeaveContext(PuglView* view)
{
  return view->backend->leave(view, false);
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglConsoleGlAttach,
                                      puglConsoleGlDetach,
                                      puglConsoleGlCreate,
                                      puglConsoleGlDestroy,
                                      puglConsoleGlEnter,
                                      puglStubDamage,
                                      puglConsoleGlLeave,
                                      puglStubGetContext};

  return &backend;
}
