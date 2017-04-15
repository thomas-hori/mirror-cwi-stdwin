/* A simple debugging routine, assuming you've got QuickDraw,
   windows and menus running.
   It can be called with up to 10 printf arguments.

   It beeps, draws a message over the menu bar,
   and waits for a key or mouse down event.
   If Command-Period is detected, it calls ExitToShell,
   to allow an emergency exit from a looping program.

   Warning: there are some side effects on the window manager's port,
   such as pen parameters and clipping.
   The printed string shouldn't exceed 255 chars.
*/

#include "macwin.h"
#include <stdarg.h>
#include <Menus.h>
#include <OSUtils.h>
#include <SegLoad.h>

void
dprintf(char *fmt, ...)
{
	char buf[256];
	GrafPtr saveport;
	GrafPtr screen;
	Rect r;
	va_list p;
	
	SysBeep(2);
	va_start(p, fmt);
	vsprintf(buf, fmt, p);
	va_end(p);
	GetPort(&saveport);
	GetWMgrPort(&screen);
	SetPort(screen);
	r= screen->portRect;
	r.bottom= 19; /* Height of menu bar, less border line */
	ClipRect(&r);
	PenNormal();
	/* Reset more parameters, such as text font? */
	EraseRect(&r);
	MoveTo(5, 15); /* Left margin and base line */
#ifndef CLEVERGLUE
	CtoPstr(buf);
#endif
	DrawString((ConstStr255Param)buf);
	
	/* Wait for mouse or key down event.
	   Include auto key events so a flood of dprintf
	   calls can be skipped quickly by holding a key. */
	for (;;) {
		EventRecord e;
		
		if (GetNextEvent(keyDownMask|autoKeyMask|mDownMask, &e)) {
			if (e.what == keyDown &&
				(e.message & charCodeMask) == '.' &&
				(e.modifiers & cmdKey))
				ExitToShell();
			if ((e.what == keyDown || e.what == autoKey) &&
				((e.message & charCodeMask) == '\r' ||
				 (e.message & charCodeMask) == '\3')
			    || e.what == mouseDown)
				break;
		}
	}
	
	/* Restore the situation, more or less. */
	SetPort(saveport);
	DrawMenuBar();
}

#ifdef UNUSED

/* Print the extent of the stack. */

#define CurStackBase	(* (long*)0x908)
#define ApplZone	(* (long*)0x2AA)
#define ApplLimit	(* (long*)0x130)

prstack(where)
	char *where;
{
	dprintf("ApplZone=%d, ApplLimit=%d, CurStackBase=%d, SP=%d",
		ApplZone, ApplLimit, CurStackBase, &where);
}

#endif /*UNUSED*/
