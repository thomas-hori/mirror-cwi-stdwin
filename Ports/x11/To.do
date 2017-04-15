DO THIS FIRST

Fix textedit insert

Fix horizontal bar jitter
(H scrollbar hit doesn't really know about imargin yet)

Make low-level key events available

cut/paste in dialogs?

When putting up a dialog box, use the last window that got an event
rather than the active window (this differs if some window has the
focus).  (Really a stdwin design bug -- no way to decide which window
a dialog belongs to.)

Fix menus to use a real tick mark and to truly gray out
(use a resource option to avoid graying)

Add wsetdefdocsize() and make unnecessary scroll bars disappear

Make default cursor a resource instead of using crosshair

Adapt menu bar height to menu font height

Can menus still cause level 0 debug messages?

De-lint, make void functions void, add prototypes for everything,
make secret public names less obvious, standardize naming conventions,
make certain public names available to clients.

Get rid of funny assignment forms "x= value"

Make code smaller, structure it more
(add subroutines for various things like setting window properties)

Warning handler for stdwin users

Error handler?

WE_CATASTROPHE event?

DOCUMENT IT !!!

------------------------------------------------------------------
ICCCM P.M.

modifier mapping conventions: don't assume Mod1mask is the meta key

Exactly one window should have a WM_COMMAND property

Input focus?

selections To do:
	- required targets TARGETS, MULTIPLE, TIMESTAMP
	- fetch selections > 32K
	- receive INCR targets?

------------------------------------------------------------------
NEW IDEAS

use regions again for exposure event compressing

------------------------------------------------------------------
VERY BAD BUGS (may stop all or most pgms from working)

textedit cursor remains on screen (problem is in X11 drawing module)

------------------------------------------------------------------
SERIOUS BUGS (affect only some pgms or nonstandard usage patterns)

------------------------------------------------------------------
MINOR BUGS (can live with)

Activate events seem to get lost (when exactly? after waskstr?)

Collapse multiple move events [done?]

Letter m mishandled by text drawing [huh?]

redraws after mouse move events have too low priority

Mouse-down: mouse-up lost when dialog box called in between.

Tab size in textedit is in pixels instead of 8*wcharwidth(' ') [done?]

Textedit should optimize even more (backspace, line inserts)

------------------------------------------------------------------
MAJOR FUNCTIONAL IMPROVEMENTS

Support setting point size with %d in font name, e.g., "courier%df".

Support setting bold/roman/italic with %s in font names ("", "b", "i", "bi").
Example: "courier%d%sf".

Add auto-scroll?

Clean up should at least unload font structs, close windows etc.

Delay showing the caret

Suppress h/v bar if corresponding dimension zero.

Suppress menu bar if no menus (what with close box?)

Problem: delayed redrawing ofteen feels sluggish, especially with
scrolling

------------------------------------------------------------------
MINOR FUNCTIONAL IMPROVEMENTS (REALLY DETAILS)

Add Y/N/C keyboard shortcuts to dialogs.

Too much redrawing after wscroll up combined with wchange on top; should
use general regions for clipping instead of single rect

Double redraw when window created with set geometry (same cause as above?)

Use resources to override menu shortcuts (find a notation for
Alt-left-arrow etc.)

Use low ^ instead of | for text caret?

Make 'reverse' option work for dialogs.

------------------------------------------------------------------
CODE COSMETICS

Don't reference gc->values!  This isn't guaranteed to work!

Use fewer GCs (how necessary is this with currently 2 GCs per window?

Use XrmGetResource instead of XGetDefault, to allow more flexible naming
scheme

Use XEventsQueued instead of XPending/XQLength

Modularize code in smaller pieces (not a top priority as long as the X
library itself is so big)

Even more meaningful comments

Allow compiling with no debug code (how?)

Improve use of _wdebuglevel:
use it to selectively trace more of the user calls

	fatal	system errors (can't recover)
	warning	user errors (including bad default resources etc.)
	0	"cannot happen" cases
	1	initialization; window open/close; defaults; screen inquiries
	
	N	wungetevent calls
	N+1	rare events (mouse click, enter/leave, menu, expose etc.)
	N+2	common events (move, key), all wgetevent calls
	
	X	wshow calls
	X+1	wsetcaret calls
	X+2	wchange calls
	X+3	wscroll calls
	
	K	w{begin,end}drawing, some others
	K+1	all non-text draw operations
	K+2	all text draw operations
	K+3	all text measure operations

------------------------------------------------------------------
QUESTIONS

wscroll doesn't know about pending updates (problem: there may be some
in the queue) (Isn't this really a semantic problem in STWIN?)

Figure out how to use button grabs

Focusing stops working when a second window is created by a dialog box
started using a keyboard shortcut.  Starts again when the original window
is closed.  (This is only when uwm is running; not with twm,
and not when no wm is running; probably an uwm bug.)
