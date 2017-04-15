/*
This code is somehow broken.
If I click a few times on an arrow of the scroll bar the cursor freezes.
Other forms of scrolling don't have this problem.  WHAT'S WRONG?!?!?!
*/

/* MAC STDWIN -- SCROLLING. */

/* All non-public functions here assume the current grafport is set */

/* XXX The contents of this file should be organized more top-down */

#include "macwin.h"

#ifndef HAVE_UNIVERSAL_HEADERS
#define ControlActionUPP ProcPtr
#define NewControlActionProc(x) ((ProcPtr)x)
#endif

/* I am using the fact here that the scroll bars of an inactive window
   are invisible:
   when a window's origin or its document size changes while it isn't the
   active window, its controls get their new values but aren't redrawn. */

/* Key repeat constants in low memory (set by the Control Panel),
   in ticks (used here to control the continuous scrolling speed). */

#define KeyThresh	(* (short*)0x18e)	/* Delay until repeat starts */
#define KeyRepThresh	(* (short*)0x190)	/* Repeat rate */

STATIC void setscrollbarvalues _ARGS((WINDOW *win));
STATIC void usescrollbarvalues _ARGS((WINDOW *win));
STATIC void sizescrollbars _ARGS((WINDOW *win));
STATIC int calcneworigin _ARGS((int, int, int, int, int));
STATIC void calcbar _ARGS((ControlHandle bar,
	int org, int size, int begin, int end));
STATIC void setbar _ARGS((ControlHandle bar, int winsize, int val, int max));
STATIC void deltabar _ARGS((ControlHandle bar, int delta));
STATIC void showbar _ARGS((ControlHandle bar));
STATIC void hidebar _ARGS((ControlHandle bar));
STATIC void movebar _ARGS((ControlHandle bar,
	int left, int top, int right, int bottom));
void _wfixorigin _ARGS((WINDOW *));

void
wsetorigin(win, orgh, orgv)
	WINDOW *win;
	int orgh, orgv;
{
	Rect r;
	
	/* XXX wsetorigin is currently the only routine that allows
	   the application to show bits outside the document.
	   Is even this desirable? */
	
	CLIPMIN(orgh, 0);
	CLIPMIN(orgv, 0);
	getwinrect(win, &r);
	orgh -= LSLOP;
	if (orgh != win->orgh || orgv != win->orgv) {
		SetPort(win->w);
		scrollby(win, &r, win->orgh - orgh, win->orgv - orgv);
		win->orgh= orgh;
		win->orgv= orgv;
		setscrollbarvalues(win);
	}
}

void
wgetorigin(win, ph, pv)
	WINDOW *win;
	int *ph, *pv;
{
	*ph = win->orgh + LSLOP;
	*pv = win->orgv;
}

void
wshow(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	int orgh, orgv;
	int winwidth, winheight;
	int docwidth, docheight;
	
	/* Calls to wshow while the mouse is down are ignored,
	   because they tend to mess up auto-scrolling.
	   I presume it's harmless (even the right thing to do),
	   since wshow is usually called to make sure the object
	   being selected is visible, which isn't a problem while
	   the user is busy pointing at it.
	   The check must be here, not in wsetorigin, because
	   the latter is used internally by the auto-scroll code! */
	if (_wm_down)
		return;
	
	/* The code below is generic for all versions of stdwin */
	
	wgetorigin(win, &orgh, &orgv);
	wgetwinsize(win, &winwidth, &winheight);
	wgetdocsize(win, &docwidth, &docheight);
	
	orgh = calcneworigin(orgh, left, right, winwidth, docwidth);
	orgv = calcneworigin(orgv, top, bottom, winheight, docheight);
	
	wsetorigin(win, orgh, orgv);
}

/* Subroutine for wshow to calculate new origin in h or v direction */

static int
calcneworigin(org, low, high, winsize, docsize)
	int org, low, high, winsize, docsize;
{
	if (docsize <= 0)
		return org;
	
	/* Ignore requests to show bits outside doc */
	/* XXX Is this necessary? */
	CLIPMIN(low, 0);
	CLIPMAX(high, docsize);
	
	/* Ignore requests for empty range */
	/* XXX Is this what we want? */
	if (high <= low)
		return org;
	
	if (org <= low && high <= org + winsize)
		return org;
	
	if (high - low > winsize) {	/* Range too big for window */
		
		/* Don't scroll if at least 1/3 of the window is in range */
		/* XXX 1/3 is an arbitrary number! */
		if (low <= org + winsize*2/3 && high >= org + winsize/3)
			return org;
		
		/* Center the end of the range that's closest to the window */
		if (low - org >= org + winsize - high) {
			org = low - winsize/2;
			CLIPMIN(org, 0);
			return org;
		}
		else {
			org = high - winsize/2;
			CLIPMAX(org, docsize);
			CLIPMIN(org, 0);
			return org;
		}
	}
	
	/* The whole range will fit */
	
	/* See if it makes sense to scroll less than the window size */
	if (low < org) {
		if (org - low < winsize)
			return low;
	}
	else if (high > org + winsize) {
		if (high - (org + winsize) < winsize)
			return high - winsize;
	}
	
	/* Would have to scroll at least winsize -- center the range */
	org = (low + high - winsize) / 2;
	
	/* Make sure we stay within the document */
	CLIPMIN(org, 0);
	CLIPMAX(org, docsize);
	
	return org;
}

void
wsetdocsize(win, docwidth, docheight)
	WINDOW *win;
	int docwidth, docheight;
{
	CLIPMIN(docwidth, 0);
	CLIPMIN(docheight, 0);
	if (docwidth == win->docwidth && docheight == win->docheight)
		return;
	win->docwidth= docwidth;
	win->docheight= docheight;
	SetPort(win->w);
	setscrollbarvalues(win);
	_wfixorigin(win);
	
	/* Make the zoomed window size the full document size,
	   or the full screen size if the document is bigger. */
	
	if (((WindowPeek)(win->w))->dataHandle != NULL) {
		WStateData *data = (WStateData *)
			*((WindowPeek)(win->w))->dataHandle;
		if (data != NULL) {
			if (docwidth > 0) {
				data->stdState.right =
					data->stdState.left
					+ LSLOP + docwidth + RSLOP;
				if (win->vbar != NULL)
					data->stdState.right += BAR;
			}
			else
				data->stdState.right = 0x7fff;
			CLIPMAX(data->stdState.right,
				screen->portRect.right-3);
			if (win->hbar != NULL) {
				CLIPMIN(data->stdState.right, 5*BAR);
			}
			if (docheight > 0) {
				data->stdState.bottom =
					data->stdState.top + docheight;
				if (win->hbar != NULL)
					data->stdState.bottom += BAR;
			}
			else
				data->stdState.bottom = 0x7fff;
			CLIPMAX(data->stdState.bottom,
				screen->portRect.bottom-3);
			if (win->vbar != NULL) {
				CLIPMIN(data->stdState.bottom, 5*BAR);
			}
		}
	}
}

void
wgetdocsize(win, pwidth, pheight)
	WINDOW *win;
	int *pwidth, *pheight;
{
	*pwidth = win->docwidth;
	*pheight = win->docheight;
}

/* Fix the window's origin after a document or window resize.
   (Also used from do_size() in event.c) */

void
_wfixorigin(win)
	WINDOW *win;
{
	int orgh, orgv;
	int winwidth, winheight;
	int docwidth, docheight;
	wgetorigin(win, &orgh, &orgv);
	wgetwinsize(win, &winwidth, &winheight);
	wgetdocsize(win, &docwidth, &docheight);
	
	/* XXX Do we really want this?
	   This means that in a text edit window, if you are focused
	   at the bottom, every line deletion causes a scroll.
	   Oh well, I suppose that text edit windows could fix
	   their document size to include some blank lines at the
	   end...
	*/
	
	CLIPMAX(orgh, docwidth - winwidth);
	CLIPMIN(orgh, 0);
	CLIPMAX(orgv, docheight - winheight);
	CLIPMIN(orgv, 0);
	wsetorigin(win, orgh, orgv);
}

/* do_scroll is called from event.c when a click in a scroll bar
   is detected */

STATIC pascal trackbar(ControlHandle bar, short pcode); /* Forward */

static WINDOW *scrollwin;	/* The window (needed by 'trackbar') */
static int scrollstep;		/* By how much we scroll */
static long deadline;		/* When the next step may happen */

void
do_scroll(pwhere, win, bar, pcode)
	Point *pwhere;
	WINDOW *win;
	ControlHandle bar;
	int pcode;
{
	int step, page;
	ControlActionUPP action;
	int width, height;
	
	if (bar == NULL)
		return;
	
	wgetwinsize(win, &width, &height);
	
	if (bar == win->hbar) {
		step= width / 20;
		CLIPMIN(step, 1);
		page= width / 2;
	}
	else if (bar == win->vbar) {
		step= wlineheight(); /* Should use current win's */
		page= height - step;
	}
	else
		return;
	
	page= (page/step) * step; /* Round down to multiple of step */
	action = NewControlActionProc(trackbar);
	scrollwin= win;
	
	switch (pcode) {
	case inUpButton:
		scrollstep= -step;
		break;
	case inDownButton:
		scrollstep= step;
		break;
	case inPageUp:
		scrollstep= -page;
		break;
	case inPageDown:
		scrollstep= page;
		break;
	default:
		action= NULL;
		break;
	}
	
	deadline= 0;
	if (TrackControl(bar, PASSPOINT *pwhere, action) == inThumb)
		usescrollbarvalues(win);
	
	SetPort(win->w); /*  THIS IS NEEDED!!! */
}

/* Use new-style prototype for __MWERKS__ */
STATIC pascal trackbar(ControlHandle bar, short pcode)
{
	long now= TickCount();
	if (now >= deadline && pcode != 0) {
 		deltabar(bar, scrollstep);
		usescrollbarvalues(scrollwin);
		wupdate(scrollwin);
		if (deadline == 0 && KeyThresh > KeyRepThresh)
			deadline= now + KeyThresh;
			/* Longer delay first time */
		else
			deadline= now + KeyRepThresh;
	}
}

/* Automatic scrolling when the mouse is pressed and moved outside
   the application panel. */

void
autoscroll(win, h, v)
	WINDOW *win;
	int h, v; /* Mouse location in local coordinates */
{
	Rect r;
	int dh= 0, dv= 0;
	
	getwinrect(win, &r);
	if (h < r.left)
		dh= h - r.left;
	else if (h > r.right)
		dh= h - r.right;
	if (v < r.top)
		dv= v - r.top;
	else if (v > r.bottom)
		dv= v - r.bottom;
	if (dh != 0 || dv != 0) {
		/* NOTE: assuming current grafport is win->w */
		deltabar(win->hbar, dh);
		deltabar(win->vbar, dv);
		usescrollbarvalues(win);
	}
}

/* Alternative scrolling: Option-click produces a 'hand' cursor
   and allows scrolling like in MacPaint.  Called from event.c. */

void
dragscroll(win, h, v, constrained)
	WINDOW *win;
	int h, v;
	int constrained;
{
	int do_h= !constrained, do_v= !constrained;
	
	while (StillDown()) {
		Point mouse;
		SetPort(win->w);
		GetMouse(&mouse);
		if (constrained) {
			int ah= ABS(h - mouse.h);
			int av= ABS(v - mouse.v);
			if (ABS(ah - av) < 2)
				continue;
			if (ah > av)
				do_h= TRUE;
			else
				do_v= TRUE;
			constrained= FALSE;
		}
		if (do_h)
			deltabar(win->hbar, h - mouse.h);
		if (do_v)
			deltabar(win->vbar, v - mouse.v);
		usescrollbarvalues(win);
		wupdate(win);
		h= mouse.h;
		v= mouse.v;
	}
}

void
makescrollbars(win, hor, ver)
	WINDOW *win;
	/*bool*/int hor, ver;
{
	Rect r;
	
	/* The scroll bars are initially created at a dummy location,
	   and then moved to their proper location.
	   They are not displayed here;
	   that's done only when an activate event arrives. */
	SetRect(&r, 0, 0, 1, 1); /* Dummy rectangle */
	if (hor)
		win->hbar= NewControl(win->w,
			&r, (ConstStr255Param)"", false, 0, 0, 0, scrollBarProc, 0L);
	if (ver)
		win->vbar= NewControl(win->w,
			&r, (ConstStr255Param)"", false, 0, 0, 0, scrollBarProc, 0L);
	sizescrollbars(win);
}

void
showscrollbars(win)
	WINDOW *win;
{
	_wgrowicon(win);
	showbar(win->hbar);
	showbar(win->vbar);
}

void
hidescrollbars(win)
	WINDOW *win;
{
	hidebar(win->hbar);
	hidebar(win->vbar);
}

void
movescrollbars(win)
	WINDOW *win;
{
	hidescrollbars(win);
	sizescrollbars(win);
	showscrollbars(win);
}

static void
sizescrollbars(win)
	WINDOW *win;
{
	Rect r;
	
	/* This must only be called while the scroll bars are invisible. */
	
	r = win->w->portRect;
	r.left = r.right - BAR;
	r.top--;
	r.bottom -= BAR-1;
	r.right++;
	movebar(win->vbar, r.left, r.top, r.right, r.bottom);
	
	r = win->w->portRect;
	r.left--;
	r.top = r.bottom - BAR;
	r.right -= BAR-1;
	r.bottom++;
	movebar(win->hbar, r.left, r.top, r.right, r.bottom);
	
	setscrollbarvalues(win);
}

static void
setscrollbarvalues(win)
	WINDOW *win;
{
	Rect r;
	
	getwinrect(win, &r);
	calcbar(win->hbar,
		win->orgh, win->docwidth, LSLOP, r.right - r.left - RSLOP);
	calcbar(win->vbar,
		win->orgv, win->docheight, 0, r.bottom - r.top);
	if (win == active) {
		/* XXX Should only draw those controls that have changed. */
		DrawControls(win->w);
		valid_border(win->w);
	}
}

/*
 * Calculate (and set!) the new value of a scroll bar.
 * Parameters (all pertaining to either h or v): 
 * org: 	origin of window (win->orgh or win->orgv)
 * size:	extent of document (win->docwidth or win->docheight)
 * begin:	position of document origin if bar is at its beginning
 * end:		position of document end if bar is at its end
 * Situation sketch:
 *                0    begin                          end  winwidth
 *                +====+==============================+====+
 *                     +--------------------------------------------------+
 * +--------------------------------------------------+
 * Window shown on top; min document position in the middle;
 * max document position below.  Org is (begin of win) - (begin of doc).
 */

static void
calcbar(bar, org, size, begin, end)
	ControlHandle bar;
	int org, size;
	int begin, end;
{
	int range;
	
	if (bar == NULL)
		return;
	
	/* For the caller it's easier to remember to pass win->org{h,v};
	   but for our calculations it's easier to have the sign reversed! */
	org= -org;
	CLIPMIN(begin, org);
	CLIPMIN(size, 0);
	CLIPMAX(end, org + size);
	range = size - (end - begin);
	CLIPMIN(range, 0);
	setbar(bar, end - begin, begin - org, range);
}

static void
usescrollbarvalues(win)
	WINDOW *win;
{
	int orgh = 0;
	int orgv = 0;
	if (win->hbar != NULL)
		orgh = GetCtlValue(win->hbar) - GetCtlMin(win->hbar);
	if (win->vbar != NULL)
		orgv = GetCtlValue(win->vbar) - GetCtlMin(win->vbar);
	wsetorigin(win, orgh, orgv);
	/* Implies setscrollbarvalues! */
}

static void
setbar(bar, winsize, val, max)
	ControlHandle bar;
	int winsize, val, max;
{
	if (bar != NULL) {
		ControlPtr p= *bar;
		if (max < 0)
			max= 0;
		if (val > max)
			val= max;
		if (val < 0)
			val= 0;
		p->contrlMin= winsize;
		p->contrlValue= val + winsize;
		p->contrlMax= max + winsize;
		/* Must be drawn by DrawControls or ShowControl. */
	}
}

static void
deltabar(bar, delta)
	ControlHandle bar;
	int delta;
{
	int min, val, max, newval;
	
	if (bar == NULL)
		return;
	min = GetCtlMin(bar);
	val = GetCtlValue(bar);
	max = GetCtlMax(bar);
	newval = val + delta;
	
	if (newval > max)
		newval= max;
	if (newval < min)
		newval= min;
	if (newval != val)
		SetCtlValue(bar, newval);
}

static void
showbar(bar)
	ControlHandle bar;
{
	if (bar != NULL)
		ShowControl(bar);
}

static void
hidebar(bar)
	ControlHandle bar;
{
	if (bar != NULL)
		HideControl(bar);
}

static void
movebar(bar, left, top, right, bottom)
	ControlHandle bar;
	int left, top, right, bottom;
{
	if (bar != NULL) {
		/* This works best while the scroll bar is invisible. */
		MoveControl(bar, left, top);
		SizeControl(bar, right-left, bottom-top);
	}
}

void
_wgrowicon(win)
	WINDOW *win;
{
	Rect r;
	
	/* If there are no scroll bars, there is no room to draw the
	   grow icon; the entire window belongs to the application.
	   However, clicks in the bottom right corner will still be
	   intercepted by the window manager. */
	
	if (win->hbar == NULL && win->vbar == NULL)
		return;
	
	/* Clip the drawing of DrawGrowIcon to the scroll bars present. */
	
	r= win->w->portRect;
	if (win->hbar == NULL)
		r.left = r.right - BAR;
	if (win->vbar == NULL)
		r.top = r.bottom - BAR;
	ClipRect(&r);
	
	DrawGrowIcon(win->w);
	
	/* Reset clipping */
	
	SetRect(&r, -32000, -32000, 32000, 32000);
	ClipRect(&r);
}
