/* MAC STDWIN -- DOCUMENT DRAWING. */

/* Contents of this file:
   
   wchange, wscroll;
   wbegindrawing, wenddrawing, _wupdate, wupdate;
   text attribute manipulation (internal and external);
   text measuring;
   text drawing;
   coordinate conversion tools;
   ordinary drawing;
   color stuff.
   
   XXX Should be split, only things used between w{begin,end}drawing
   belong here.

   BEWARE OF COORDINATE SYSTEMS!
   
   The window's origin is not always the same.
   When the aplication isn't drawing (i.e., outside w{begin,end}drawing),
   the origin is (0, 0); this is necessary because the control mgr
   doesn't like it otherwise.
   When the application is drawing, the origin is set to win->(orgh, orgv)
   so the drawing primitives don't need to do their own coordinate
   transformation.
   Routines called while not drawing must do their own transformations.
   
   XXX Routines that may be called any time are in trouble!
   (There shouldn't be any.)
*/

#include "macwin.h"
#include <Fonts.h>
#include <Memory.h>

#ifdef __MWERKS__
#define CONSTPATCAST (ConstPatternParam) &
#else
#define CONSTPATCAST /*nothing*/
#endif


COLOR _w_fgcolor = blackColor;
COLOR _w_bgcolor = whiteColor;
static COLOR save_fgcolor, save_bgcolor;

static WINDOW *drawing;
static TEXTATTR drawsaveattr;
static int baseline;
static int lineheight;

/* Avoid name conflicts: */

#define getfinfo getfinfo_
#define wsetstyle wsetstyle_

/* Function prototypes */

STATIC void getwattr _ARGS((GrafPtr port, TEXTATTR *attr));
STATIC void setwattr _ARGS((TEXTATTR *attr));
STATIC void getfinfo _ARGS((void));

/* Patterns -- can't use the QuickDraw globals (sigh) */

static Pattern pat0   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static Pattern pat25  = {0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44};
static Pattern pat50  = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
static Pattern pat75  = {0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77, 0xdd};
static Pattern pat100 = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void
wchange(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	Rect r;
	
	SetPort(win->w);
	
	if (drawing) dprintf("warning: wchange called while drawing");
	makerect(win, &r, left, top, right, bottom);
	InvalRect(&r);
}

void
wscroll(win, left, top, right, bottom, dh, dv)
	WINDOW *win;
	int left, top, right, bottom;
	int dh, dv;
{
	Rect r;
	
	if (drawing) dprintf("warning: wchange called while drawing");
	makerect(win, &r, left, top, right, bottom);
	scrollby(win, &r, dh, dv);
}

/* Scroll a rectangle (given in window coordinates) by dh, dv.
   Also used from scroll.c. */
/* XXX need less visible name */

void
scrollby(win, pr, dh, dv)
	WINDOW *win;
	Rect *pr;
	int dh, dv;
{
	RgnHandle rgn= NewRgn();
	
	SetPort(win->w);
	
	rmcaret(win);
	
	if (!EmptyRgn(( (WindowPeek)win->w )->updateRgn)) {
		RgnHandle ur= NewRgn();
		int left, top;
		Rect r;
		
		/* Scroll the part of the update region that intersects
		   the scrolling area. */
		CopyRgn(( (WindowPeek)win->w )->updateRgn, ur);
		/* portBits.bounds is the screen in local coordinates! */
		left= win->w->portBits.bounds.left;
		top= win->w->portBits.bounds.top;
		OffsetRgn(ur, left, top); /* Global to Local */
		RectRgn(rgn, pr);
		SectRgn(rgn, ur, ur);
		if (!EmptyRgn(ur)) {
			ValidRgn(ur);
			OffsetRgn(ur, dh, dv);
			SectRgn(rgn, ur, ur);
			InvalRgn(ur);
		}
		DisposeRgn(ur);
	}
	
	/* ScrollRect seems to work only in normal mode */
	ForeColor(blackColor);
	BackColor(whiteColor);
	ScrollRect(pr, dh, dv, rgn);
	ForeColor((short)win->fgcolor);
	BackColor((short)win->bgcolor);
	
	InvalRgn(rgn);
	
	DisposeRgn(rgn);
}


/* Internal version of wupdate -- also used from event.c */

void
_wupdate(win, pr)
	WINDOW *win;
	Rect *pr;
{
	RgnHandle rgn= NewRgn();
	
	SetPort(win->w);
	
	rmcaret(win);
	BeginUpdate(win->w);
	EraseRect(&win->w->portRect);
	_wgrowicon(win);
	DrawControls(win->w);
	getwinrect(win, pr);
	RectRgn(rgn, pr);
	SectRgn(rgn, win->w->visRgn, rgn);
	*pr= (*rgn)->rgnBBox;
	OffsetRect(pr, win->orgh, win->orgv);
	if (win->drawproc != NULL && !EmptyRect(pr)) {
		wbegindrawing(win);
		(*win->drawproc)(win, pr->left, pr->top, pr->right, pr->bottom);
		wenddrawing(win);
	}
	EndUpdate(win->w);
	DisposeRgn(rgn);
}

/* Process an update event -- call the window's draw procedure. */
/* XXX This function shouldn't be in the stdwin spec (why was it ever?) */

void
wupdate(win)
	WINDOW *win;
{
	if (win->drawproc != NULL) {
		Rect r;
		_wupdate(win, &r);
	}
}

void
wbegindrawing(win)
	WINDOW *win;
{
	Rect r;
	
	if (drawing != NULL) {
		dprintf("warning: recursive call to wbegindrawing");
		/* Fix it */
		wenddrawing(drawing);
	}
	if (win == NULL) {
		dprintf("warning: wbegindrawing(NULL) ignored");
		return;
	}
	drawing= win;
	SetPort(win->w);
	SetOrigin(win->orgh, win->orgv);
	rmcaret(win);
	getwinrect(win, &r);
	ClipRect(&r);
	PenNormal();
	wgettextattr(&drawsaveattr);
	wsettextattr(&win->attr);
	
	save_fgcolor = _w_fgcolor;
	save_bgcolor = _w_bgcolor;
	_w_fgcolor = win->fgcolor;
	_w_bgcolor = win->bgcolor;
	_w_usefgcolor(_w_fgcolor);
	_w_usebgcolor(_w_bgcolor);
}

void
wenddrawing(win)
	WINDOW *win;
{
	Rect r;
	
	if (drawing == NULL) {
		dprintf("warning: wenddrawing ignored while not drawing");
		return;
	}
	if (drawing != win) {
		dprintf("warning: wenddrawing ignored for wrong window");
		return;
	}
	
	_w_fgcolor = save_fgcolor;
	_w_bgcolor = save_bgcolor;
	_w_usefgcolor(win->fgcolor);
	_w_usebgcolor(win->bgcolor);
	
	SetOrigin(0, 0);
	SetRect(&r, -32000, -32000, 32000, 32000);
	ClipRect(&r);
	wsettextattr(&drawsaveattr);
	drawing= NULL;
}

/* Get the current text attributes of a GrafPort. */

static void
getwattr(port, attr)
	GrafPtr port;
	TEXTATTR *attr;
{
	SetPort(port); /* XXX Is this necessary to validate txFont etc.? */
	
	attr->font= port->txFont;
	attr->style= port->txFace;
	attr->size= port->txSize;
}

/* Start using the given text attributes in the current grafport. */

static void
setwattr(attr)
	TEXTATTR *attr;
{
	TextFont(attr->font);
	TextFace(attr->style & ~TX_INVERSE);
	TextSize(attr->size);
}

/* Get font info and baseline from current GrafPort */

static void
getfinfo()
{
	FontInfo finfo;
	GetFontInfo(&finfo);
	baseline= finfo.ascent + finfo.leading;
	lineheight= baseline + finfo.descent;
}

/* Initialize 'wattr'.  Uses the screen's grafport. */

int _w_font= applFont;
int _w_size= 9;

void
initwattr()
{
	TEXTATTR saveattr;
	
	SetPort(screen);
	
	getwattr(screen, &saveattr);
	TextFont(_w_font);
	TextSize(_w_size);
	getwattr(screen, &wattr);
	getfinfo();
	setwattr(&saveattr);
}

/* TEXT ATTRIBUTES. */

void
wgettextattr(attr)
	TEXTATTR *attr;
{
	*attr= wattr;
}

void
wsettextattr(attr)
	TEXTATTR *attr;
{
	wattr= *attr;
	if (drawing != NULL) {
		setwattr(attr);
		getfinfo();
	}
}

void
wgetwintextattr(win, attr)
	WINDOW *win;
	TEXTATTR *attr;
{
	*attr= win->attr;
}

void
wsetwintextattr(win, attr)
	WINDOW *win;
	TEXTATTR *attr;
{
	win->attr= *attr;
	if (drawing != NULL)
		dprintf("warning: wsetwintextattr called while drawing");
}

char **
wlistfontnames(pat, plen)
	char *pat;
	int *plen;
{
	/* XXX not yet implemented */
	*plen = 0;
	return NULL;
}

int
wsetfont(fontname)
	char *fontname;
{
	int ret = 1;
	if (fontname == NULL || *fontname == EOS)
		wattr.font= _w_font;
	else {
		short fnum= 0;
		GetFNum(PSTRING(fontname), &fnum);
		if (fnum == 0) {
			dprintf("can't find font %s", fontname);
			ret = 0;
		}
		wattr.font= fnum;
	}
	if (drawing != NULL) {
		TextFont(wattr.font);
		getfinfo();
	}
	return ret;
}

void
wsetsize(size)
	int size;
{
	wattr.size= size;
	if (drawing != NULL) {
		TextSize(size);
		getfinfo();
	}
}

void
wsetstyle(face)
	Style face;
{
	wattr.style= face;
	if (drawing != NULL) {
		TextFace(face & ~TX_INVERSE);
		getfinfo();
	}
}

void
wsetplain()
{
	wsetstyle(0);
}

void
wsetinverse()
{
	wattr.style |= TX_INVERSE;
}

void
wsetbold()
{
	wsetstyle(bold);
}

void
wsetitalic()
{
	wsetstyle(italic);
}

void
wsetbolditalic()
{
	wsetstyle(bold|italic);
}

void
wsetunderline()
{
	wsetstyle(underline);
}

void
wsethilite()
{
	wsetstyle(TX_INVERSE);
}

/* TEXT MEASURING. */

int
wlineheight()
{
	if (drawing == NULL) {
		TEXTATTR saveattr;
		getwattr(screen, &saveattr);
		setwattr(&wattr);
		getfinfo();
		setwattr(&saveattr);
	}
	return lineheight;
}

int
wbaseline()
{
	if (drawing == NULL)
		(void) wlineheight();
	return baseline;
}

int
wtextwidth(str, len)
	char *str;
	int len;
{
	TEXTATTR saveattr;
	int width;
	
	if (drawing == NULL) {
		getwattr(screen, &saveattr);
		setwattr(&wattr);
	}
	if (len < 0)
		len= strlen(str);
	width= TextWidth(str, 0, len);
	if (drawing == NULL)
		setwattr(&saveattr);
	return width;
}

int
wcharwidth(c)
	int c;
{
	char cbuf[1];
	cbuf[0]= c;
	return wtextwidth(cbuf, 1);
}

/* TEXT DRAWING. */

void
wdrawtext(h, v, str, len)
	int h, v;
	char *str;
	int len;
{
	Point pen;
	
	if (len < 0)
		len= strlen(str);
	MoveTo(h, v + baseline);
	DrawText(str, 0, len);
	GetPen(&pen);
	if (wattr.style & TX_INVERSE) {
		Rect r;
		SetRect(&r, h, v, pen.h, v + lineheight);
		InvertRect(&r);
	}
}

void
wdrawchar(h, v, c)
	int h, v;
	int c;
{
	if ((wattr.style & TX_INVERSE) == 0) {
		/* Attempt to optimize for appls. like dpv */
		MoveTo(h, v + baseline);
		DrawChar(c);
	}
	else {
		char cbuf[1];
		cbuf[0]= c;
		wdrawtext(h, v, cbuf, 1);
	}
}

/* COORDINATE CONVERSIONS ETC. */

/* Create a rect in current window coordinates (as for getwinrect)
   from a rectangle given in document coordinates.
   This works both while drawing and while not drawing.
   The resulting rectangle is clipped to the winrect. */

void
makerect(win, pr, left, top, right, bottom)
	WINDOW *win;
	Rect *pr;
	int left, top, right, bottom;
{
	Rect r;
	SetRect(pr, left, top, right, bottom);
	if (!drawing)
		OffsetRect(pr, - win->orgh, - win->orgv);
	getwinrect(win, &r);
	(void) SectRect(&r, pr, pr);
}

/* Retrieve the 'winrect', i.e., the contents area exclusive
   of the scroll bars and grow icon.
   Coordinates are in window coordinates, i.e.,
   the origin is (0, 0) outside w{begin,end}drawing,
   but (win->orgh, win->orgv) between calls to these routines. */

void
getwinrect(win, pr)
	WINDOW *win;
	Rect *pr;
{
	*pr= win->w->portRect;
	if (win->vbar != NULL)
		pr->right -= BAR;
	if (win->hbar != NULL)
		pr->bottom -= BAR;
}

/* ACTUAL DRAW ROUTINES. */

void
wdrawline(h1, v1, h2, v2)
{
	MoveTo(h1, v1);
	Line(h2-h1, v2-v1);
}

void
wxorline(h1, v1, h2, v2)
{
	MoveTo(h1, v1);
	PenMode(patXor);
	Line(h2-h1, v2-v1);
	PenNormal();
}

void
wdrawbox(left, top, right, bottom)
	int left, top, right, bottom;
{
	Rect r;
	
	SetRect(&r, left, top, right, bottom);
	FrameRect(&r);
}

void
werase(left, top, right, bottom)
	int left, top, right, bottom;
{
	Rect r;
	
	SetRect(&r, left, top, right, bottom);
	EraseRect(&r);
}

void
winvert(left, top, right, bottom)
	int left, top, right, bottom;
{
	Rect r;
	
	SetRect(&r, left, top, right, bottom);
	InvertRect(&r);
}

void
wpaint(left, top, right, bottom)
	int left, top, right, bottom;
{
	Rect r;
	
	SetRect(&r, left, top, right, bottom);
	FillRect(&r, CONSTPATCAST drawing->w->pnPat);
}

void
wshade(left, top, right, bottom, perc)
	int left, top, right, bottom;
	int perc;
{
	Rect r;
	PatPtr p;
	
	perc= (perc + 12)/25;
	CLIPMIN(perc, 0);
	
	switch (perc) {
	case 0:	p = &pat0;	break;
	case 1: p = &pat25;	break;
	case 2: p = &pat50; 	break;
	case 3: p = &pat75;	break;
	default: p = &pat100;	break;
	}
	
	SetRect(&r, left, top, right, bottom);

	FillRect(&r, (ConstPatternParam)p);
}

void
wdrawcircle(h, v, radius)
	int h, v;
	int radius;
{
	Rect r;
	
	SetRect(&r, h-radius, v-radius, h+radius, v+radius);
	FrameOval(&r);
}

void
wfillcircle(h, v, radius)
	int h, v;
	int radius;
{
	Rect r;
	
	SetRect(&r, h-radius, v-radius, h+radius, v+radius);
	FillOval(&r, CONSTPATCAST drawing->w->pnPat);
}

void
wxorcircle(h, v, radius)
	int h, v;
	int radius;
{
	Rect r;
	
	SetRect(&r, h-radius, v-radius, h+radius, v+radius);
	InvertOval(&r);
}

/* Draw counterclockwise elliptical arc.
   h, v are center; hrad, vrad are half axes; ang1 and ang2 are start
   and stop angles in degrees, with respect to 3 o'clock. */

void
wdrawelarc(h, v, hrad, vrad, ang1, ang2)
{
	Rect r;
	SetRect(&r, h-hrad, v-vrad, h+hrad, v+vrad);
	FrameArc(&r, 90-ang1, ang1-ang2);
}

/* Fill an elliptical arc */

void
wfillelarc(h, v, hrad, vrad, ang1, ang2)
{
	Rect r;
	SetRect(&r, h-hrad, v-vrad, h+hrad, v+vrad);
	FillArc(&r, 90-ang1, ang1-ang2, CONSTPATCAST drawing->w->pnPat);
}

/* Invert an elliptical arc */

void
wxorelarc(h, v, hrad, vrad, ang1, ang2)
{
	Rect r;
	SetRect(&r, h-hrad, v-vrad, h+hrad, v+vrad);
	InvertArc(&r, 90-ang1, ang1-ang2);
}

/* Draw a polyline of n-1 connected segments (not necessarily closed).
   This means n points.  Points are given as an array of POINTs. */

void
wdrawpoly(n, points)
	int n;
	POINT *points;
{
	if (--n > 0) {
		MoveTo(points[0].h, points[0].v);
		do {
			points++;
			LineTo(points[0].h, points[0].v);
		} while (--n > 0);
	}
}

/* Fill a polygon of n points */

void
wfillpoly(n, points)
	int n;
	POINT *points;
{
	PolyHandle ph = OpenPoly();
	wdrawpoly(n, points);
	ClosePoly();
	FillPoly(ph, CONSTPATCAST drawing->w->pnPat);
	DisposHandle((Handle)ph);
}

/* Invert a polygon of n points */

void
wxorpoly(n, points)
	int n;
	POINT *points;
{
	PolyHandle ph = OpenPoly();
	wdrawpoly(n, points);
	ClosePoly();
	InvertPoly(ph);
	DisposHandle((Handle)ph);
}

/* CLIPPING */

void
wcliprect(left, top, right, bottom)
	int left, top, right, bottom;
{
	Rect r1, r2;
	SetRect(&r1, left, top, right, bottom);
	getwinrect(drawing, &r2);
	SectRect(&r1, &r2, /*dst:*/ &r2);
	ClipRect(&r2);

}

void
wnoclip()
{
	Rect r;
	getwinrect(drawing, &r);
	ClipRect(&r);
}

/* COLOR */

COLOR
wgetfgcolor()
{
	return _w_fgcolor;
}

COLOR
wgetbgcolor()
{
	return _w_bgcolor;
}

void
wsetfgcolor(color)
	COLOR color;
{
	_w_fgcolor = color;
	if (drawing)
		_w_usefgcolor(color);
}

void
wsetbgcolor(color)
	COLOR color;
{
	_w_bgcolor = color;
	if (drawing)
		_w_usebgcolor(color);
}

void
_w_usefgcolor(color)
	COLOR color;
{
	ForeColor((short)color);
	switch (((color >> 16) + 32) / 64) {
	case 0:
		PenPat(CONSTPATCAST pat100);
		break;
	case 1:
		PenPat(CONSTPATCAST pat75);
		break;
	case 2:
		PenPat(CONSTPATCAST pat50);
		break;
	case 3:
		PenPat(CONSTPATCAST pat25);
		break;
	case 4:
		PenPat(CONSTPATCAST pat0);
		break;
	}
}

void
_w_usebgcolor(color)
	COLOR color;
{
	BackColor((short)color);
}
