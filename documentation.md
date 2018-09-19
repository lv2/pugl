# Drag and drop

Pugl is able to perform drag and drop, both as a source, and a target (including being both at once). This is done by using a number of callbacks. The following describes which callback is being called at what time of the DnD operation. In detail, the functions are explained in doxygen.

All mentioned positions are, if not explicitly stated, local to the Pugl window.

## Pugl is Source

- Pugl enables drag and drop if one key is being hold while the mouse is being dragged. `dndSourceKeyFunc` must return this key
- When the drag starts,
  1. The status is set to `PuglDndSourceDragged`
  2. `dndSourceDragFunc` is being called to inform Pugl about this and about where the drag occured.
- When the DnD target just changed (including if there was none previously), `PuglDndSourceOfferTypeFunc` is being called to get Pugl's offered mimetypes.
- As long as there is a target, `dndSourceActionFunc` is being called to find out the action Pugl wants to perform (copy, move or link, see PuglDndAction). The pugl implementation may (but there is no guarantee) not call this function if the target has asked Pugl not to send any position information inside a specified rectangle.
- After the source (pugl) has initiated the drop,
  1. The status is set to `PuglDndSourceDropped
  2. `dndSourceFinishedFunc` is being called to inform Pugl about whether the target has accepted the drop.
  3. The status is reset to `PuglNotDndSource`
- In case the target requests any data (this can happen at any time after the drag, even during the drop), dndSourceProvideDataFunc is being called to ask Pugl to reveal the data for a specific mimetype.

## Pugl is Target

- When the pointer of the DnD source enters the Pugl window and a drag is active,
  1. The status is set to `PuglDndTargetDragged`
  2. `dndTargetOfferTypeFunc` is being called to let Pugl know which mimetypes the source offers
- Each time a pointer enters the Pugl window, or if it changes its position:
  1. `dndTargetInformPositionFunc` tells Pugl where the mouse pointer is and what action the source wants to perform (copy, move or link, see PuglDndAction). Calls to this function may (but there is no guarantee) be suppressed if Pugl asked the source to suppress sending the pointer position in a specified rectangle.
  2. `dndTargetChooseTypesToLookupFunc` lets Pugl choose which data (e.g. belonging to which mimetype) of the source it wants to read (this can also be done after the drop).
  3. `dndTargetNoPositionInFunc` lets Pugl define a rectangle in which further position information shall not be offered by the source. This rectangle is optional, and even if specified, the source may ignore it.
- If the source has left the Pugl window without a drop, or if the drop has been canceled,
  1. `dndTargetLeaveFunc` is being called
  2. The status is reset to `PuglNotDndTarget`
- When a successful drop occurs on Pugl,
  1. The status is set to `PuglDndSourceDropped`
  2. `dndTargetDrop` is being called to inform Pugl about the drop.
  3. `dndTargetChooseTypesToLookupFunc` lets Pugl again choose which data (e.g. belonging to which mimetype) of the source it wants to read.
  4. `dndTargetAcceptDropFunc` is being called to let Pugl tell the source whether it accepts the drop or not. The function can also be used to do cleanups.
  5. The status is reset to `PuglNotDndTarget`.
- Any time the source has sent the requested data, `dndTargetReceiveDataFunc` offers the data to Pugl.

## The DnD status

It can be crucial for apps to keep track of whether a dnd operation is in progress, and if yes in what state. E.g., many apps may want to react differently to mouse motion events if a dnd operation is in progress; some may even want to suppress motion events.

In order to not let the user write their own state machine, pugl has two status variables that are synched:
- `dnd_source_status` via on `dndSourceStatusFunc`
- `dnd_target_status` via on `dndTargetStatusFunc`

It is recommended to let these functions update variables in your handle. Debug output can also be helpful in these callbacks. Any other action should be done in the other dnd callbacks.

## Limitations

- The implementation is limited to X11 currently.
- If Pugl is the target, Pugl can call `dndTargetChooseTypesToLookupFunc` to lookup the data, but this can not be used to make a decision in `dndTargetAcceptDropFunc`.
