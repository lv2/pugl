.. default-domain:: c
.. highlight:: c

################
Using Clipboards
################

Clipboards provide a way to transfer information between different views,
including views in different processes.

In Pugl, both "copy and paste" and "drag and drop" interactions are supported by the same clipboard mechanism.
Several functions are used for both,
and take a :enum:`PuglClipboard` enumerator to distinguish which clipboard the operation applies to.

Because these interactions support transfer of data between processes and negotiation of the data type,
each is a multi-step process.
As with everything, events are used to notify the application about relevant changes.

*************
Drag and Drop
*************

To enable support for receiving a particular type of dragged data,
:func:`puglRegisterDragType` must be called during setup to register a MIME type.
For example, the common case of accepting a list of files is supported by the `text/uri-list <http://amundsen.com/hypermedia/urilist/>`_ type.

.. code-block:: c

   puglRegisterDragType(view, "text/uri-list");

On Windows, this is the only type that is currently supported.
On MacOS and X11, other data types can be used,
such as ``text/plain`` (which is implicitly UTF-8 encoded),
or ``application/zip``:

.. code-block:: c

   puglRegisterDragType(view, "text/plain");
   puglRegisterDragType(view, "application/zip");

Receiving dropped data begins by receiving a :enumerator:`PUGL_DATA_OFFER` event.
This event signals that data is being "offered" by being dragged (but not yet dropped) over the view:

.. code-block:: c

   // ...

   case PUGL_DATA_OFFER:
     onDataOffer(view, &event->offer);
     break;

When handling this event,
:func:`puglGetNumClipboardTypes` and :func:`puglGetClipboardType` can be used to enumerate the available data types:

.. code-block:: c

   static void
   onDataOffer(PuglView* view, const PuglEventDataOffer* event)
   {
     PuglClipboard clipboard = event->clipboard;
     size_t        numTypes  = puglGetNumClipboardTypes(view, clipboard);

     for (size_t t = 0; t < numTypes; ++t) {
       const char* type = puglGetClipboardType(view, clipboard, t);
       printf("Offered type: %s\n", type);
     }
   }

If the view supports dropping one of the data types at the specified cursor location,
it can accept the drop with :func:`puglAcceptOffer`:

.. code-block:: c

   for (size_t t = 0; t < numTypes; ++t) {
     const char* type = puglGetClipboardType(view, clipboard, t);
     if (!strcmp(type, "text/uri-list")) {
       puglAcceptOffer(view,
                       event,
                       t,
                       puglGetFrame(view));
     }
   }

This process will happen repeatedly while the user drags the item around the view.
Different actions may be given which may affect how the drag is presented to the user,
for example by changing the mouse cursor.
The last argument specifies the region of the view which this response applies to,
which may be used as an optimization to send fewer events.
It is safe, though possibly sub-optimal, to simply specify the entire frame as is done above.

When the item is dropped,
Pugl will transfer the data in the appropriate datatype behind the scenes,
and send a :enumerator:`PUGL_DATA` event to signal that the data is ready to be fetched with :func:`puglGetClipboard`:

.. code-block:: c

   // ...

   case PUGL_DATA:
     onData(view, &event->data);
     break;

   // ...

   static void
   onData(PuglView* view, const PuglEventData* event)
   {
     PuglClipboard clipboard = event->clipboard;
     uint32_t      typeIndex = event->typeIndex;

     const char* type = puglGetClipboardType(view, clipboard, typeIndex);

     fprintf(stderr, "Received data type: %s\n", type);

     if (!strcmp(type, "text/plain")) {
       size_t      len  = 0;
       const void* data = puglGetClipboard(view, clipboard, typeIndex, &len);

       printf("Dropped: %s\n", (const char*)data);
     }
   }

**************
Copy and Paste
**************

Data can be copied to the "general" clipboard with :func:`puglSetClipboard`:

.. code-block:: c

   // ...

   if ((event->state & PUGL_MOD_CTRL) && event->key == 'c') {
     const char* someString = /* ... */;

     puglSetClipboard(view,
                      PUGL_CLIPBOARD_GENERAL,
                      "text/plain",
                      someString,
                      strlen(someString) + 1);
   }

Pasting data works nearly the same way as receiving dropped data,
except the events use :enumerator:`PUGL_CLIPBOARD_GENERAL` instead of :enumerator:`PUGL_CLIPBOARD_DRAG`.
Unlike dropping, however, the receiving application must itself initiate the transfer,
using :func:`puglPaste`:

.. code-block:: c

   if ((event->state & PUGL_MOD_CTRL) && event->key == 'v') {
     puglPaste(view);
   }

This will result in a :enumerator:`PUGL_DATA_OFFER` event being sent as above,
which must be accepted to ultimately receive the data in the desired data type.
