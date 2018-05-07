# Drag and drop

Pugl is able to perform drag and drop, both as a source, and a target (including being both at once). This is done by using a number of callbacks. The following describes which callback is being called at what time of the DnD operation. In detail, the functions are explained in doxygen.

All mentioned positions are, if not explicitly stated, local to the Pugl window.

## Pugl is Source:

- Pugl enables drag and drop if one key is being hold while the mouse is being dragged. `dndSourceKeyFunc` must return this key
- When the drag starts, `dndSourceDragFunc` is being called to inform Pugl about this and about where the drag occured.
- When the DnD target just changed (including if there was none previously), `PuglDndSourceOfferTypeFunc` is being called to get Pugl's offered mimetypes.
- As long as there is a target, `dndSourceActionFunc` is being called to find out the action Pugl wants to perform (copy, move or link, see PuglDndAction). The pugl implementation may (but there is no guarantee) not call this function if the target has asked Pugl not to send any position information inside a specified rectangle.
- After the drop has been performed, `dndSourceFinishedFunc` is being called to inform Pugl about whether the drop has been accepted or not.
- In case the target requests any data (this can happen at any time after the drag, even during the drop), dndSourceProvideDataFunc is being called to ask Pugl to reveal the data for a specific mimetype.

## Pugl is Target:

- When the DnD source enters the Pugl window and a drag is active, `dndTargetOfferTypeFunc` is being called to let Pugl know which mimetypes the source offers
- When a pointer enters the Pugl window, or if it changes its position:
  1. `dndTargetInformPositionFunc` tells Pugl where the mouse pointer is and what action the source wants to perform (copy, move or link, see PuglDndAction). Calls to this function may (but there is no guarantee) be suppressed if Pugl asked the source to suppress sending the pointer position in a specified rectangle.
  2. `dndTargetChooseTypesToLookupFunc` lets Pugl choose which data (e.g. belonging to which mimetype) of the source it wants to read (this can also be done after the drop).
  3. `dndTargetNoPositionInFunc` lets Pugl define a rectangle in which further position information shall not be offered by the source. This rectangle is optional, and even if specified, the source may ignore it.
- `dndTargetLeaveFunc` is being called if the source has left the Pugl window without a drop, or if the drop has been canceled.
- When a successful drop occurs on Pugl,
  1. `dndTargetDrop` is being called to inform Pugl about the drop.
  2. `dndTargetChooseTypesToLookupFunc` lets Pugl again choose which data (e.g. belonging to which mimetype) of the source it wants to read.
  3. `dndTargetAcceptDropFunc` is being called to let Pugl tell the source whether it accepts the drop or not. The function can also be used to do cleanups.
- Any time the source has sent the requested data, `dndTargetReceiveDataFunc` offers the data to Pugl.

## Limitations

- The implementation is limited to X11 currently.
- If Pugl is the target, Pugl can call `dndTargetChooseTypesToLookupFunc` to lookup the data, but this can not be used to make a decision in `dndTargetAcceptDropFunc`.
