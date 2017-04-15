/* MAC STDWIN -- "ABOUT STDWIN" MESSAGE. */

#include "macwin.h"
#include "patchlevel.h"
#include <TextEdit.h>

/* Default About... message; applications may assign to this.
   You may also assign to about_item in "menu.c", *before*
   calling winit() or winitargs().
   Also see your THINK C licence.
*/
#ifdef THINK_C
#define COMPILER " [THINK C]"
#endif

#ifdef __MWERKS__
#ifdef __powerc
#define COMPILER " [CW PPC]"
#else
#define COMPILER " [CW 68K]"
#endif
#endif

#ifndef COMPILER
#define COMPILER
#endif

char *about_message=
	"STDWIN version " PATCHLEVEL COMPILER "\r\r\
Copyright \251 1988-1995\r\
Stichting Mathematisch Centrum, Amsterdam\r\r\
Author: Guido van Rossum <guido@cwi.nl>\r\
FTP: host ftp.cwi.nl, directory pub/stdwin\r\r\
Hints:\r\
Option-click in windows: drag-scroll\r\
Option-click in title: send window behind";
	/* \251 is the (c) Copyright symbol.
	   I don't want non-ASCII characters in my source. */


/* "About ..." procedure.
   This is self-contained -- if you have a better idea, change it.
*/

void
do_about()
{
	Rect r;
	WindowPtr w;
	EventRecord e;
	
	SetRect(&r, 0, 0, 340, 200); /* XXX Shouldn't be hardcoded */
	OffsetRect(&r, (screen->portRect.right - r.right)/2, 40);
	
	w = NewWindow(
		(Ptr) NULL,	/* No storage */
		&r,		/* Bounds rect */
		(ConstStr255Param)"", /* No title */
		true, 		/* Visible */
		dBoxProc,	/* Standard dialog box border */
		(WindowPtr) -1,	/* In front position */
		false,		/* No go-away box */
		0L);		/* RefCon */
	SetPort(w);
	r = w->portRect;
	InsetRect(&r, 10, 10);
	TextBox(about_message, strlen(about_message), &r, teJustCenter);
	while (!GetNextEvent(mDownMask|keyDownMask, &e))
		;
	DisposeWindow(w);
}
