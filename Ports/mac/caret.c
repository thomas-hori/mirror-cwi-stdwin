/* MAC STDWIN -- TEXT CARET HANDLING. */

/* The 'caret' is a blinking vertical bar indicating the text insertion point.
   Each window has its own caret position (hcaret, vcaret) telling where the
   insertion point is; if either is negative no caret is shown.
   Only the caret of the active window is actually visible.
   The blinking is done by repeatedly calling (in the event loop when idling)
   blinkcaret(), which flips the caret's state if it's been stable long enough.
   The file-static variable lastflip records when the caret has last changed
   state.
   A struct member caret_on indicates per window whether the caret is
   currently drawn or not.  (A file-static variable would really suffice.)

   NB: all routines except w{set,no}caret assume win->w is the current GrafPort.	
*/

#include "macwin.h"

/* Function prototypes */
STATIC void invertcaret _ARGS((WINDOW *win));
STATIC void flipcaret _ARGS((WINDOW *win));

static long lastflip = 0;	/* Time when the caret last blinked */

/* Invert the caret's pixels. */

static void
invertcaret(win)
	WINDOW *win;
{
	Rect r;
	FontInfo finfo;
	
	if (win->hcaret < 0 || win->vcaret < 0)
		return;
	GetFontInfo(&finfo);
	makerect(win, &r,
		win->hcaret-1,
		win->vcaret,
		win->hcaret,
		win->vcaret + finfo.leading + finfo.ascent + finfo.descent);
	InvertRect(&r);
}

/* Flip the caret state, if the window has a caret.
   Also set lastflip, so blinkcaret() below knows when to flip it again. */

static void
flipcaret(win)
	WINDOW *win;
{
	if (win->hcaret < 0 || win->vcaret < 0)
		return;
	invertcaret(win);
	win->caret_on = !win->caret_on;
	lastflip= TickCount();
}

/* Make sure the caret is invisible.
   This is necessary before operations like handling mouse clicks,
   bringing up dialog boxes, or when the window goes inactive. */

void
rmcaret(win)
	WINDOW *win;
{
	if (win->caret_on)
		flipcaret(win);
}

/* Blink the caret, if there is one.
   This should be called sufficiently often from the main event loop.
   We only blink if enough ticks have passed since the last flip,
   whether caused by us or by an explicit call to rmcaret(). */

void
blinkcaret(win)
	WINDOW *win;
{
	if (win->hcaret < 0 || win->vcaret < 0)
		return;
	if (TickCount() >= lastflip + GetCaretTime())
		flipcaret(win);
}

/* User-callable routine to specify the caret position.
   The caret is not drawn now, but when the next blink is due. */

void
wsetcaret(win, h, v)
	WINDOW *win;
	int h, v;
{
	SetPort(win->w);
	rmcaret(win);
	win->hcaret = h;
	win->vcaret = v;
}

/* User-callable routine to specify that the window has no caret.
   This is indicated by setting (hcaret, vcaret) to (-1, -1). */

void
wnocaret(win)
	WINDOW *win;
{
	SetPort(win->w);
	rmcaret(win);	/* See remark in wsetcaret() */
	win->hcaret = win->vcaret= -1;
}
