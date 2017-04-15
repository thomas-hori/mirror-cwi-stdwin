.\" To format, use (di)troff -ms; and make sure macros.ms is around!
.so macros.ms
.SH
Using X11 Selections and Cut Buffers in STDWIN
.LP
The C STDWIN interface has been extended to support the
.I selection
mechanism used by X11.
Support for the ring of 8 cut buffers is also provided.
.PP
This document assumes you are familiar with STDWIN as a programmer and
with X11 as an end user (at least xterm experience and some idea of the
client-server model).
.NH
What are Selections?
.LP
X11 supports an arbitrary number of selections, but version 1.0 of the
ICCCM (Inter-Client Communication Conventions Manual, by David Rosenthal)
requires only that clients support three selections called PRIMARY,
SECONDARY and CLIPBOARD.
I will further reference these in lower case.
.PP
The
.I "primary selection"
is the mechanism you normally use to transfer data interactively
between clients (or within one client).
In xterm and text-oriented toolkit clients, you make a primary
selection by dragging with the left mouse button.
The contents of the selection are highlighted using inverse video.
The contents of the primary selection can be pasted into the same or
another (toolkit) client by pressing the middle mouse button.
.PP
Conventions for use of the
.I "secondary selection"
are less well-known.
It is used as the second argument of operations with two arguments,
such as swap or replace operations.
There is no default support for it in the toolkit, but it is
possible to customize toolkit clients to use it.
.PP
The
.I clipboard
is intended to hold data that the user has deleted and wants to insert
somewhere later.
It is supported by the standard X11 client
.I xclipboard ,
which displays the current contents of the clipboard.
Other toolkit clients can be customized to use it.
.NH
Selections From the Client's Point of View
.LP
It is important to realize that the contents of selections are
transferred between clients at the time they are pasted; selections are
not normally stored by the server.
At any time, a selection is ``owned'' by at most one client, the
.I "selection owner" .
Different selections may be owned by different clients.
The server keeps track of selection owners.
When a client wants to become owner of a particular selection, it tells
the server so, and the server will send the previous owner an event to
tell it that it no longer owns the selection.
.PP
The protocol to transfer the contents of a selection between clients is
fairly involved.
Somewhat simplified, it works as follows:
.IP \(bu
A client requests the contents of a selection from the server.
.IP \(bu 2
The server sends the owner an event asking ``please give me this
selection.''
.IP \(bu 2
The owner stores the contents of the selection as a
.I property\(dg
.FS
\(dg
A window
.I property
is a named arbitrary piece of data associated with a window, stored by
the server.
.FE
on the requestor's window, and sends an event back (via the server)
telling the requestor that the selection is stored.
.IP \(bu 2
The requestor fetches the contents of the property, and deletes
the property.
.PP
Various complications are possible:
.IP \(bu 2
Race conditions: a client may be slow to respond to a mouse click, and
the impatient user may have clicked again in another client's window.
When the first client finally decides it wants to become selection
owner, the second client may already own it.
.IP \(bu
Crashes: a selection owner may crash before it has responded to a
request for the selection's contents.
.IP \(bu
Space limitations on the server.
.IP \(bu
Negotiations about the type of the selection: a string could be stored
in simple ASCII or in a special format that retains font information,
and in fact a selection may contain a picture or an arbitrarily
complicated data structure that only a few clients know about.
.NH
The STDWIN Interface to Selections
.LP
STDWIN simplifies the concept of a selection somewhat and takes care of
most details of communicating with other clients.
In STDWIN, selections are always ASCII strings and only the three
standard selections are supported.
The strings are null-terminated but may also contain embedded null
characters, so a length is always provided.
.PP
The header
.cW <stdwin.h>
defines constants that identify the selections:
.cW WS_PRIMARY ,
.cW WS_SECONDARY ,
and
.cW WS_CLIPBOARD .
The application calls the function
.cW wsetselection()
to become owner of a selection.
It may later receive an event of type
.cW WE_LOST_SEL
telling it that it lost ownership again.
It is also possible to voluntarily give up selection ownership by
calling
.cW wresetselection() .
To access the contents of a selection, whether you own it or not, call
the function
.cW wgetselection() .
Transfer of selection contents to other clients is done automatically by
.cW wgetevent()
and
.cW wpollevent() .
Detailed descriptions of the new functions follow:
.NH 2
Function \*<wsetselection()\*>
.LP
.sC L
bool wsetselection(win, sel, data, len)
WINDOW *win;
int sel;
char *data;
int len;
.eC
.LP
Parameters:
.IP \*<win\*> 6n
Specifies a window that will be associated with the selection.
This has no other meaning that limiting the lifetime of the selection to
the lifetime of the window.
It will not be reported in
.cW WE_LOST_SEL
events.
.IP \*<sel\*>
Specifies which selection should be set.
It should be one of the codes
.cW WS_PRIMARY ,
.cW WS_SECONDARY ,
or
.cW WS_CLIPBOARD .
.IP \*<data\*>
Specifies the data comprising the selection's contents.
This need not be null-terminated.
It is copied to a safe place by the function so it can be sent to
other clients later.
.IP \*<len\*>
Specifies the length of the data, not including a terminating null byte.
.LP
This function attempts to acquirpe ownership of the specified selection.
This may fail because of race conditions.
The function returns nonzero if it succeeds.
If it fails, the application should give the user visual feedback of the
failure,
.I e.g. ,
by not inverting the selected text.
.NH 2
Function \*<wresetselection()\*>
.LP
.sC L
void wresetselection(sel)
int sel;
.eC
.LP
Parameters:
.IP \*<sel\*> 6n
Specifies which selection should be reset.
It should be one of the codes
.cW WS_PRIMARY ,
.cW WS_SECONDARY ,
or
.cW WS_CLIPBOARD .
.LP
If the application owns the specified selection, this function cancels
ownership.
No
.cW WE_LOST_SEL
events are generated for the selection.
.NH 2
Function \*<wgetselection()\*>
.LP
.sC L
char *wgetselection(sel, plen)
int sel;
int *plen;
.eC
Parameters:
.IP \*<sel\*> 6n
Specifies which selection should be retrieved.
It should be one of the codes
.cW WS_PRIMARY ,
.cW WS_SECONDARY ,
or
.cW WS_CLIPBOARD .
.IP \*<plen\*>
Into this parameter, the length of the data is returned, excluding the
terminating null byte.
.LP
This function retrieves the contents of the specified selection.
If it succeeds, a pointer to its data is returned.
The data is terminated by a null byte but may contain null bytes, so the
length is returned separately.
The data pointer points to space internal to the STDWIN library.
It remains valid until the next call involving selections.
If the selection could not be retrieved somehow, a NULL pointer is
returned.
Selections longer that 32K may be truncated.
Since the transfer mechanism requires the use of a window, a NULL
pointer is returned when the application currently has no windows open.
.NH 2
The \*<WE_LOST_SEL\*> event type
.LP
A STDWIN application receives this event type when it owns a selection
(a call to
.cW wsetselection()
has succeeded) and another client has become the new owner.
The
.cW window
member of the
.cW EVENT
is set to
.cW NULL .
The
.cW u.sel
union member is set to the code for the selection.
This event is not generated when selection ownership is given up by
calling
.cW wresetselection()
or by deleting its window.
.NH
The STDWIN Interface to Cut Buffers
.LP
For compatibility with old STDWIN or X11 clients and Andrew clients,
an interface to the cut buffer interface is also provided.
This is a ring of 8 buffers maintained at the server.
The following functions are available:
.NH 2
Function \*<wsetcutbuffer()\*>
.LP
.sC L
void wsetcutbuffer(ibuffer, data, len)
int ibuffer;
char *data;
int len;
.eC
.LP
Parameters:
.IP \*<ibuffer\*> 10n
Specifies which buffer should be set, in the range [0 ... 7].
.IP \*<data\*>
Specifies the data.
It need not be null-terminated.
.IP \*<len\*>
Specifies the length of the data, excluding a terminating null byte.
.LP
This function sets the contents of the specified cut buffer to the given
data.
No indication of success or failure is given.
.NH 2
Function \*<wgetcutbuffer()\*>
.LP
.sC L
char *wgetcutbuffer(ibuffer, plen)
int ibuffer;
int *plen;
.eC
.LP
Parameters:
.IP \*<ibuffer\*> 10n
Specifies which buffer should be retrieved, in the range [0 ... 7].
.IP \*<plen\*>
Returns the length of the data, excluding the terminating null byte.
.LP
This function returns the contents of the specified cut buffer,
terminated by a null byte.
If the cut buffer is not accessible, it returns
.cW NULL .
.NH 2
Function \*<wrotatecutbuffers()\*>
.LP
.sC L
void wrotatecutbuffers(n)
int n;
.eC
.LP
Parameters:
.IP \*<n\*> 4n
Specifies the amount of rotation.
This may be negative.
.LP
The cut buffers are rotated as follows:
buffer n gets the contents of buffer 0, buffer n+1 (mod 8) gets the
contents of buffer 1, etc.
.NH 2
Function \*<wsetclip()\*>
.LP
.sC L
void wsetclip(data, len)
char *data;
int len;
.eC
This function is equivalent to
.sC
wsetcutbuffer(0, data, len);
wrotatecutbuffers(1);
.eC
.NH 2
Function \*<wgetclip()\*>
.LP
.sC L
char *wgetclip()
.eC
This function is equivalent to
.sC
int len;
return wgetcutbuffer(0, &len);
.eC
(It throws away the length.)
.NH
Suggested Usage
.LP
To conform to X11 conventions, STDWIN applications should normally use
the primary selection, but use the cut buffers as a ``fall-back''
mechanism.
.PP
When the user has selected some text, the application should transfer
the text to cut buffer 0 and rotate the buffers (the easiest way to do
this is to call
.cW wsetclip() ),
and then call
.cW wsetselection()
to set the primary selection to the text.
If the latter call fails, the inverse video on the selected text should
be removed.
The inverse video should also be removed when a
.cW WE_LOST_SEL
event is received.
If there is a text insertion point associated with the selection, it
should be left at the position indicated by the last mouse click, or to
the beginning of the selected text.
.PP
When the user desires to paste some text, the applcation should first
get the contents of the primary selection, and if this returns a NULL
pointer, it should get the contents of cut buffer 0.
If this returns a NULL pointer as well, the paste request should be
refused (with a beep, or something similar).
.PP
The conventions for selecting and pasting text are:
.IP \(bu 2
Dragging with the left (first) mouse button is used to make a selection.
Double-clicking selects ``words.''
.IP \(bu
Clicking with the middle mouse button requests a ``paste'' operation.
It does not move the insert point to the position of the click.
.IP \(bu
Clicking with the right mouse button extends the selection at the end
closest to the button click.
.NH
Selections and Macintosh STDWIN
.LP
The Macintosh user interface standards prescribe only a single cut
buffer, called the Clipboard.
For source compatibility with STDWIN applications developed for X11,
dummy versions of the selection functions are provided:
.cW wsetselection()
and
.cW wgetselection()
always fail, and
.cW wresetselection()
is ignored.
Versions of the cut buffer functions are provided that identify cut
buffer 0 with the Macintosh Clipboard and ignore the other cut buffers.
.PP
The net effect is that STDWIN applications written for X11 selections
that use the cut buffers as a fall-back mechanism will support the
Macintosh Clipboard, albeit with an X11-like interface.
Macintosh applications are encouraged to provide a standard Edit menu
with the operations Cut, Copy and Paste and the standard shortcuts for
them: Command-X, Command-C and Command-V.
