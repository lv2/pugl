// Copyright 2012-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "internal.h"
#include "types.h"
#include "x11.h"

#include <pugl/pugl.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void
clearX11Clipboard(PuglX11Clipboard* const board)
{
  for (unsigned long i = 0; i < board->numFormats; ++i) {
    free(board->formatStrings[i]);
    board->formatStrings[i] = NULL;
  }

  board->source              = None;
  board->numFormats          = 0;
  board->acceptedFormatIndex = UINT32_MAX;
  board->acceptedFormat      = None;
  board->data.len            = 0;
}

static PuglX11Clipboard*
getX11SelectionClipboard(PuglView* const view, const Atom selection)
{
  return (selection == view->world->impl->atoms.CLIPBOARD)
           ? &view->impl->clipboard
           : NULL;
}

static PuglStatus
setClipboardFormats(PuglView* const         view,
                    PuglX11Clipboard* const board,
                    const unsigned long     numFormats,
                    const Atom* const       formats)
{
  // Clear current board formats
  for (unsigned long i = 0; i < board->numFormats; ++i) {
    free(board->formatStrings[i]);
    board->formatStrings[i] = NULL;
  }

  board->numFormats = 0;

  // Enlarge formats array
  Atom* const newFormats =
    (Atom*)realloc(board->formats, numFormats * sizeof(Atom));
  if (!newFormats) {
    return PUGL_NO_MEMORY;
  }

  board->formats = newFormats;

  // Enlarge format strings array
  char** const newFormatStrings =
    (char**)realloc(board->formatStrings, numFormats * sizeof(char*));
  if (!newFormatStrings) {
    return PUGL_NO_MEMORY;
  }

  board->formatStrings = newFormatStrings;

  for (unsigned long i = 0; i < numFormats; ++i) {
    if (formats[i]) {
      char* const name = XGetAtomName(view->world->impl->display, formats[i]);
      const char* type = NULL;

      if (strchr(name, '/')) { // MIME type (hopefully)
        type = name;
      } else if (!strcmp(name, "UTF8_STRING")) { // Plain text
        type = "text/plain";
      }

      if (type) {
        const size_t typeLen      = strlen(type);
        char* const  formatString = (char*)calloc(typeLen + 1, 1);

        memcpy(formatString, type, typeLen + 1);

        board->formats[board->numFormats]       = formats[i];
        board->formatStrings[board->numFormats] = formatString;
        ++board->numFormats;
      }

      XFree(name);
    }
  }

  return PUGL_SUCCESS;
}

static PuglStatus
retrieveSelection(const PuglWorld* const world,
                  PuglView* const        view,
                  const Atom             property,
                  const Atom             type,
                  PuglBlob* const        result)
{
  uint8_t*      value          = NULL;
  Atom          actualType     = 0U;
  int           actualFormat   = 0;
  unsigned long actualNumItems = 0U;
  unsigned long bytesAfter     = 0U;

  if (XGetWindowProperty(world->impl->display,
                         view->impl->win,
                         property,
                         0,
                         0x1FFFFFFF,
                         False,
                         type,
                         &actualType,
                         &actualFormat,
                         &actualNumItems,
                         &bytesAfter,
                         &value) != Success) {
    return PUGL_FAILURE;
  }

  if (value && actualFormat == 8 && bytesAfter == 0) {
    puglSetBlob(result, value, actualNumItems);
  }

  XFree(value);
  return PUGL_SUCCESS;
}

PuglStatus
handleSelectionClear(PuglView* const                   view,
                     const XSelectionClearEvent* const event)
{
  PuglX11Clipboard* const board =
    getX11SelectionClipboard(view, event->selection);
  if (board) {
    clearX11Clipboard(board);
  }
  return PUGL_SUCCESS;
}

PuglStatus
handleSelectionNotify(const PuglWorld* const       world,
                      PuglView* const              view,
                      const XSelectionEvent* const event)
{
  const PuglX11Atoms* const atoms = &world->impl->atoms;

  Display* const          display   = view->world->impl->display;
  const Atom              selection = event->selection;
  PuglX11Clipboard* const board     = getX11SelectionClipboard(view, selection);
  PuglEvent               puglEvent = {{PUGL_NOTHING, 0U}};

  if (event->target == atoms->TARGETS) {
    // Notification of available datatypes
    unsigned long numFormats = 0;
    Atom*         formats    = NULL;
    if (!getAtomProperty(
          view, event->requestor, event->property, &numFormats, &formats) &&
        !setClipboardFormats(view, board, numFormats, formats)) {
      const PuglDataOfferEvent offer = {
        PUGL_DATA_OFFER, 0U, (double)event->time / 1e3};

      puglEvent.offer            = offer;
      board->acceptedFormatIndex = UINT32_MAX;
      board->acceptedFormat      = None;

      XFree(formats);
    }

  } else if (event->selection == atoms->CLIPBOARD &&
             event->property == XA_PRIMARY &&
             board->acceptedFormatIndex < board->numFormats) {
    // Notification of data from the clipboard
    if (!retrieveSelection(
          world, view, event->property, event->target, &board->data)) {
      board->source = XGetSelectionOwner(display, board->selection);

      const PuglDataEvent data = {
        PUGL_DATA, 0U, (double)event->time / 1e3, board->acceptedFormatIndex};

      puglEvent.data = data;
    }
  }

  return puglDispatchEvent(view, &puglEvent);
}

PuglStatus
handleSelectionRequest(const PuglWorld* const              world,
                       PuglView* const                     view,
                       const XSelectionRequestEvent* const request)
{
  Display* const            display = world->impl->display;
  const PuglX11Atoms* const atoms   = &world->impl->atoms;

  PuglX11Clipboard* const board =
    getX11SelectionClipboard(view, request->selection);

  if (!board) {
    return PUGL_UNKNOWN_ERROR;
  }

  if (request->target == atoms->TARGETS) {
    XChangeProperty(world->impl->display,
                    request->requestor,
                    request->property,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (const uint8_t*)board->formats,
                    (int)board->numFormats);
  } else {
    XChangeProperty(world->impl->display,
                    request->requestor,
                    request->property,
                    request->target,
                    8,
                    PropModeReplace,
                    (const uint8_t*)board->data.data,
                    (int)board->data.len);
  }

  XSelectionEvent note = {SelectionNotify,
                          request->serial,
                          False,
                          display,
                          request->requestor,
                          request->selection,
                          request->target,
                          request->property,
                          request->time};

  return XSendEvent(
           world->impl->display, note.requestor, True, 0, (XEvent*)&note)
           ? PUGL_SUCCESS
           : PUGL_UNKNOWN_ERROR;
}

const void*
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  Display* const          display = view->world->impl->display;
  PuglX11Clipboard* const board   = &view->impl->clipboard;

  if (typeIndex != board->acceptedFormatIndex) {
    return NULL;
  }

  const Window owner = XGetSelectionOwner(display, board->selection);
  if (!owner || owner != board->source) {
    *len = 0;
    return NULL;
  }

  *len = board->data.len;
  return board->data.data;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const offer,
                const uint32_t                  typeIndex)
{
  (void)offer;

  PuglInternals* const    impl    = view->impl;
  Display* const          display = view->world->impl->display;
  PuglX11Clipboard* const board   = &view->impl->clipboard;

  board->acceptedFormatIndex = typeIndex;
  board->acceptedFormat      = board->formats[typeIndex];

  // Request the data in the specified type from the general clipboard
  XConvertSelection(display,
                    board->selection,
                    board->acceptedFormat,
                    board->property,
                    impl->win,
                    CurrentTime);

  return PUGL_SUCCESS;
}

PuglStatus
puglPaste(PuglView* const view)
{
  Display* const          display = view->world->impl->display;
  const PuglX11Atoms*     atoms   = &view->world->impl->atoms;
  const PuglX11Clipboard* board   = &view->impl->clipboard;

  // Request a SelectionNotify for TARGETS (available datatypes)
  XConvertSelection(display,
                    board->selection,
                    atoms->TARGETS,
                    board->property,
                    view->impl->win,
                    CurrentTime);

  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const view)
{
  return (uint32_t)view->impl->clipboard.numFormats;
}

const char*
puglGetClipboardType(const PuglView* const view, const uint32_t typeIndex)
{
  const PuglX11Clipboard* const board = &view->impl->clipboard;

  return typeIndex < board->numFormats ? board->formatStrings[typeIndex] : NULL;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  PuglInternals* const    impl    = view->impl;
  Display* const          display = view->world->impl->display;
  PuglX11Clipboard* const board   = &view->impl->clipboard;
  PuglStatus              st      = puglSetBlob(&board->data, data, len);

  if (!st) {
    const Atom format = {XInternAtom(display, type, 0)};

    st = setClipboardFormats(view, board, 1, &format);
    XSetSelectionOwner(display, board->selection, impl->win, CurrentTime);

    board->source = impl->win;
  }

  return st;
}
