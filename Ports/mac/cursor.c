/* MAC STDWIN -- MOUSE CURSORS. */

/* XXX Shouldn't named resources override the defaults? */

#include "macwin.h"
#include <ToolUtils.h>
#include <Resources.h>

extern CursPtr handcursorptr;

/* Fetch a cursor by name.  This really returns a resource handle,
   cast to the mythical type CURSOR *.
   If no resource by that name exists, some standard cursors may
   be returned. */

CURSOR *
wfetchcursor(name)
	char *name;
{
	CursHandle h = (CursHandle) GetNamedResource('CURS', PSTRING(name));
	if (h == NULL) {
		if (strcmp(name, "ibeam") == 0)
			h = GetCursor(iBeamCursor);
		else if (strcmp(name, "cross") == 0)
			h = GetCursor(crossCursor);
		else if (strcmp(name, "plus") == 0)
			h = GetCursor(plusCursor);
		else if (strcmp(name, "watch") == 0)
			h = GetCursor(watchCursor);
#if 0 /* XXX */
		else if (strcmp(name, "arrow") == 0) {
			/* The arrow is only a quickdraw global,
			   which we can't use.
			   Should have it as a static variable... */
			static CursPtr arrowptr;
			arrowptr = &QD(arrow);
			h = &arrowptr;
		}
#endif
		else if (strcmp(name, "hand") == 0) {
			/* The hand is hardcoded below */
			h = &handcursorptr;
		}
	}
	return (CURSOR *)h;
}

void
wsetwincursor(win, cursor)
	WINDOW *win;
	CURSOR *cursor;
{
	win->cursor = cursor;
	if (win == active)
		set_applcursor();
}

/* Set the mouse cursor shape to the standard arrow.
   This shape is used when the program is ready for input without
   having the active window. */

void
set_arrow()
{
	InitCursor();
}

/* Set the mouse cursor shape to the standard watch.
   This shape is used when a long task is being performed.
   In practice always between two calls to wgetevent()
   except when the mouse is down.
   Note: this call is ignored when the application has
   specified a cursor for the window; in this case it
   is up to the application to set an arrow when it goes
   away for a long time. */

void
set_watch()
{
	if (active == NULL || active->cursor == NULL)
		SetCursor(*GetCursor(watchCursor));
}

/* Set the cursor to the standard cursor for the active window.
   If there is no active window, use an arrow.
   If a cursor is specified for the active window, use that,
   otherwise use a default.
   The default is normally a crosshair but can be changed by
   setting the global variable _w_cursor to a cursor ID. */

int _w_cursor= crossCursor;

void
set_applcursor()
{
	if (active == NULL)
		set_arrow();
	else if (active->cursor == NULL)
		SetCursor(*GetCursor(_w_cursor));
	else {
		CursHandle h = (CursHandle) active->cursor;
		if (*h == NULL)
			LoadResource((Handle)h);
		SetCursor(*h);
	}
}

/* Set the mouse cursor shape to a little hand icon.
   This shape is used when scroll-dragging the document. */

static Cursor handcursor= {
	{	/* Data: */
		0x0180, 0x1a70, 0x2648, 0x264a, 
		0x124d, 0x1249, 0x6809, 0x9801, 
		0x8802, 0x4002, 0x2002, 0x2004, 
		0x1004, 0x0808, 0x0408, 0x0408,
	},
	{	/* Mask: */
		0x0180, 0x1bf0, 0x3ff8, 0x3ffa, 
		0x1fff, 0x1fff, 0x7fff, 0xffff, 
		0xfffe, 0x7ffe, 0x3ffe, 0x3ffc, 
		0x1ffc, 0x0ff8, 0x07f8, 0x07f8,
	},
	{8, 8}	/* Hotspot */
};

static CursPtr handcursorptr = &handcursor; /* For wfetchcursor */

void
set_hand()
{
	SetCursor(&handcursor);
}
