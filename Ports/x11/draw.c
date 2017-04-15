/* X11 STDWIN -- Drawing operations */

#include "x11.h"

/* Window ID and Graphics Context used implicitly by all drawing operations */

static Window wid;
static GC gc;
COLOR _w_fgcolor, _w_bgcolor;

/* Put the current font's ID in the current GC, if non-null.
   Called by _wfontswitch. */

_wgcfontswitch()
{
	if (gc != 0) {
		if (_wf->fid == 0)
			XCopyGC(_wd, DefaultGCOfScreen(_ws), GCFont, gc);
		else
			XSetFont(_wd, gc, _wf->fid);
	}
}

/* Begin drawing in the given window.
   All drawing must be executed bewteen a call to wbegindrawing
   and one to wenddrawing; in between, no other calls to wbegindrawing
   should occur. */

static TEXTATTR saveattr;
static COLOR savefgcolor, savebgcolor;

void
wbegindrawing(win)
	WINDOW *win;
{
	_wtrace(4, "wbegindrawing(win = 0x%lx)", (long)win);
	if (wid != 0)
		_werror("recursive wbegindrawing");
	savefgcolor = _w_fgcolor;
	savebgcolor = _w_bgcolor;
	saveattr= wattr;
	wid = win->wa.wid;
	gc = win->gca;
	_w_fgcolor = win->fga;
	_w_bgcolor = win->bga;
	XSetForeground(_wd, gc, (unsigned long) _w_fgcolor);
	XSetBackground(_wd, gc, (unsigned long) _w_bgcolor);
	wsettextattr(&win->attr);
	if (win->caretshown)
		_winvertcaret(win); /* Hide caret temporarily */
}

/* End drawing in the given window */

void
wenddrawing(win)
	WINDOW *win;
{
	_wtrace(4, "wenddrawing(win = 0x%lx)", (long)win);
	if (wid != win->wa.wid)
		_werror("wrong call to enddrawing");
	else {
		wnoclip();
		if (win->caretshown)
			_winvertcaret(win); /* Put it back on */
		_w_fgcolor = savefgcolor;
		_w_bgcolor = savebgcolor;
		XSetForeground(_wd, gc, (unsigned long) _w_fgcolor);
		XSetBackground(_wd, gc, (unsigned long) _w_bgcolor);
		wid = 0;
		gc = 0;
		wsettextattr(&saveattr);
		XFlush(_wd);
	}
}


/* Text measurement functions */


/* Compute the space taken by a string when drawn in the current font */

int
wtextwidth(str, len)
	char *str;
	int len;
{
	if (len < 0)
		len= strlen(str);
	len= XTextWidth(_wf, str, len);
	_wtrace(7, "wtextwidth: width=%d", len);
	return len;
}

/* Compute a character's width */

int
wcharwidth(c)
	int c;
{
	char buf[2];
	
	buf[0]= c;
	return wtextwidth(buf, 1);
}

/* Compute the right vertical spacing for the current font */

int
wlineheight()
{
	return _wf->ascent + _wf->descent;
}

/* Compute how much the baseline of the characters drawn lies below
   the v coordinate passed to wdrawtext or wdrawchar */

int
wbaseline()
{
	return _wf->ascent;
}


/* Text drawing functions */


/* Draw a text string */

void
wdrawtext(h, v, str, len)
	int h, v;
	char *str;
	int len;
{
	int right;
	int bottom= v + wlineheight();
	
	if (len < 0)
		len= strlen(str);
	_wtrace(5, "wdrawtext(%d, %d, \"%.*s%s\", %d)",
		h, v, len>20 ? 17 : len, len>20 ? "..." : "", str, len);
	if (wattr.style & (INVERSE|HILITE|UNDERLINE))
		right = h + wtextwidth(str, len);
	if (wattr.style & (INVERSE|HILITE))
		werase(h, v, right, bottom);
	XDrawString(_wd, wid, gc, h, v + _wf->ascent, str, len);
	if (wattr.style & UNDERLINE) {
		unsigned long ulpos, ulthick;
		if (!XGetFontProperty(_wf, XA_UNDERLINE_POSITION, &ulpos))
			ulpos= _wf->descent/2;
		if (!XGetFontProperty(_wf, XA_UNDERLINE_THICKNESS, &ulthick)) {
			ulthick= _wf->descent/3;
			CLIPMIN(ulthick, 1);
		}
		ulpos += v + _wf->ascent;
		winvert(h, (int)ulpos, right, (int)(ulpos + ulthick));
	}
	if (wattr.style & (INVERSE|HILITE))
		winvert(h, v, right, bottom);
}

void
wdrawchar(h, v, c)
	int h, v;
	int c;
{
	char ch= c;
	
	if ((wattr.style & (INVERSE|HILITE|UNDERLINE)) == 0) {
#ifdef __GNUC__
		/* Work-around for GCC bug (1.31 or some such):
		   without the _wtrace, XdrawString is never called. */
		_wtrace(5, "wdrawchar(%d, %d, '\\%03o')", h, v, c&0377);
#endif
		/* Optimize plain characters */
		XDrawString(_wd, wid, gc, h, v + _wf->ascent, &ch, 1);
	}
	else {
		/* Use wdrawtext for complicated cases */
		wdrawtext(h, v, &ch, 1);
	}
}


/* Manipulate text attribute structs */


/* Get a window's text attributes */

void
wgetwintextattr(win, pattr)
	WINDOW *win;
	TEXTATTR *pattr;
{
	*pattr= win->attr;
}

/* Change a window's text attributes */

void
wsetwintextattr(win, pattr)
	WINDOW *win;
	TEXTATTR *pattr;
{
	win->attr= *pattr;
}


/* Non-text drawing primitives */


/* Draw a straight line */

void
wdrawline(h1, v1, h2, v2)
{
	_wtrace(7, "wdrawline((h1,v1)=(%d,%d), (h2,v2)=(%d,%d))",
		h1, v1, h2, v2);
	XDrawLine(_wd, wid, gc, h1, v1, h2, v2);
}

/* Draw a straight line in XOR mode */

void
wxorline(h1, v1, h2, v2)
{
	_wtrace(7, "wxorline((h1,v1)=(%d,%d), (h2,v2)=(%d,%d))",
		h1, v1, h2, v2);
	XSetFunction(_wd, gc, GXinvert);
	XSetPlaneMask(_wd, gc, _w_fgcolor ^ _w_bgcolor);
	XDrawLine(_wd, wid, gc, h1, v1, h2, v2);
	XSetFunction(_wd, gc, GXcopy);
	XSetPlaneMask(_wd, gc, AllPlanes);
}

/* Draw a rectangle *inside* the given coordinate box */

void
wdrawbox(left, top, right, bottom)
{
	_wtrace(7, "wdrawbox(left=%d, top=%d, right=%d, bottom=%d)",
		left, top, right, bottom);
	XDrawRectangle(_wd, wid, gc, left, top, right-left-1, bottom-top-1);
}

/* Erase the given rectangle */

void
werase(left, top, right, bottom)
{
	_wtrace(7, "werase(left=%d, top=%d, right=%d, bottom=%d)",
		left, top, right, bottom);
	
	if (left >= right || top >= bottom)
		return;
	
	/* Can't use XClearArea here because it ignores the clipping region.
	   Can't set function to GXclear because it doesn't work
	   with color.  So we fill with the background color. */
	
	XSetForeground(_wd, gc, (unsigned long) _w_bgcolor);
	XFillRectangle(_wd, wid, gc, left, top, right-left, bottom-top);
	XSetForeground(_wd, gc, (unsigned long) _w_fgcolor);
}

/* Invert the bits in the given rectangle.
   (This uses _winvert because this function is often used internally.) */

void
winvert(left, top, right, bottom)
{
	_wtrace(7, "winvert(left=%d, top=%d, right=%d, bottom=%d)",
		left, top, right, bottom);
	
	if (left >= right || top >= bottom)
		return;

	/* _winvert assumes the plane mask is the XOR of fg and bg color;
	   this is no longer standard now we support colors... */

	XSetPlaneMask(_wd, gc, _w_fgcolor ^ _w_bgcolor);
	_winvert(wid, gc, left, top, right-left, bottom-top);
	XSetPlaneMask(_wd, gc, AllPlanes);
}

/* Paint a given rectangle black */

void
wpaint(left, top, right, bottom)
{
	_wtrace(7, "wpaint(left=%d, top=%d, right=%d, bottom=%d)",
		left, top, right, bottom);
	
	if (left >= right || top >= bottom)
		return;
	
	XFillRectangle(_wd, wid, gc, left, top, right-left, bottom-top);
}

/* Shade a given area; this should add a lighter or darker raster
   depending on the darkness percentage (0 = no raster, 100 = black) */

void
wshade(left, top, right, bottom, percent)
{
	_wtrace(7, "wshade(left=%d, top=%d, right=%d, bottom=%d, percent=%d)",
		left, top, right, bottom, percent);
	
	if (left >= right || top >= bottom)
		return;
	
	XSetStipple(_wd, gc, _w_raster(percent));
	XSetFillStyle(_wd, gc, FillStippled);
	XFillRectangle(_wd, wid, gc, left, top, right-left, bottom-top);
	XSetFillStyle(_wd, gc, FillSolid);
}

/* Draw a circle with given center and radius */

void
wdrawcircle(h, v, radius)
	int h, v;
	int radius;
{
	_wtrace(7, "wdrawcircle(h=%d, v=%d, radius=%d)", h, v, radius);
	XDrawArc(_wd, wid, gc, h-radius, v-radius, 2*radius-1, 2*radius-1,
		0, 360*64);
}

/* Fill a circle with given center and radius */

void
wfillcircle(h, v, radius)
	int h, v;
	int radius;
{
	_wtrace(7, "wfillcircle(h=%d, v=%d, radius=%d)", h, v, radius);
	XFillArc(_wd, wid, gc, h-radius, v-radius, 2*radius-1, 2*radius-1,
		0, 360*64);
}

/* Invert a circle with given center and radius */

void
wxorcircle(h, v, radius)
	int h, v;
	int radius;
{
	_wtrace(7, "wfillcircle(h=%d, v=%d, radius=%d)", h, v, radius);
	XSetFunction(_wd, gc, GXinvert);
	XSetPlaneMask(_wd, gc, _w_fgcolor ^ _w_bgcolor);
	XFillArc(_wd, wid, gc, h-radius, v-radius, 2*radius-1, 2*radius-1,
		0, 360*64);
	XSetFunction(_wd, gc, GXcopy);
	XSetPlaneMask(_wd, gc, AllPlanes);
}

/* Draw an elliptical arc.
   The begin and end angles are specified in degrees (I'm not sure this
   is a good idea, but I don't like X's degrees*64 either...).
   The arc is drawn counter-clockwise; 0 degrees is 3 o'clock.
   wdrawcircle is equivalent to wdrawarc(h, v, radius, radius, 0, 360). */

void
wdrawelarc(h, v, hhalf, vhalf, angle1, angle2)
	int h, v;		/* Center */
	int hhalf, vhalf;	/* Half axes */
	int angle1, angle2;	/* Begin, end angle */
{
	_wtrace(7, "wdrawelarc(%d, %d, %d, %d, %d, %d)",
		h, v, hhalf, vhalf, angle1, angle2);
	XDrawArc(_wd, wid, gc, h-hhalf, v-vhalf, 2*hhalf-1, 2*vhalf-1,
		angle1*64, angle2*64);
}

/* Fill an elliptical arc segment */

void
wfillelarc(h, v, hhalf, vhalf, angle1, angle2)
	int h, v;		/* Center */
	int hhalf, vhalf;	/* Half axes */
	int angle1, angle2;	/* Begin, end angle */
{
	_wtrace(7, "wfillelarc(%d, %d, %d, %d, %d, %d)",
		h, v, hhalf, vhalf, angle1, angle2);
	XFillArc(_wd, wid, gc, h-hhalf, v-vhalf, 2*hhalf-1, 2*vhalf-1,
		angle1*64, angle2*64);
}

/* Invert an elliptical arc segment */

void
wxorelarc(h, v, hhalf, vhalf, angle1, angle2)
	int h, v;		/* Center */
	int hhalf, vhalf;	/* Half axes */
	int angle1, angle2;	/* Begin, end angle */
{
	_wtrace(7, "wfillelarc(%d, %d, %d, %d, %d, %d)",
		h, v, hhalf, vhalf, angle1, angle2);
	XSetFunction(_wd, gc, GXinvert);
	XSetPlaneMask(_wd, gc, _w_fgcolor ^ _w_bgcolor);
	XFillArc(_wd, wid, gc, h-hhalf, v-vhalf, 2*hhalf-1, 2*vhalf-1,
		angle1*64, angle2*64);
	XSetFunction(_wd, gc, GXcopy);
	XSetPlaneMask(_wd, gc, AllPlanes);
}

/* Draw n-1 lines connecting n points */

void
wdrawpoly(n, points)
	int n;
	POINT *points;
{
	_wtrace(7, "wdrawpoly(%d, ...)", n);
	XDrawLines(_wd, wid, gc, (XPoint *)points, n, CoordModeOrigin);
}

/* Fill a polygon given by n points (may be self-intersecting) */

void
wfillpoly(n, points)
	int n;
	POINT *points;
{
	_wtrace(7, "wfillpoly(%d, ...)", n);
	XFillPolygon(_wd, wid, gc, (XPoint *)points, n,
		     Complex, CoordModeOrigin);
}

/* Invert a polygon given by n points (may be self-intersecting) */

void
wxorpoly(n, points)
	int n;
	POINT *points;
{
	_wtrace(7, "wfillpoly(%d, ...)", n);
	XSetFunction(_wd, gc, GXinvert);
	XSetPlaneMask(_wd, gc, _w_fgcolor ^ _w_bgcolor);
	XFillPolygon(_wd, wid, gc, (XPoint *)points, n,
		     Complex, CoordModeOrigin);
	XSetFunction(_wd, gc, GXcopy);
	XSetPlaneMask(_wd, gc, AllPlanes);
}

/* Clip drawing output to a rectangle. */

void
wcliprect(left, top, right, bottom)
	int left, top, right, bottom;
{
	XRectangle clip;
	
	_wtrace(7, "wcliprect(%d, %d, %d, %d)", left, top, right, bottom);
	clip.x = left;
	clip.y = top;
	clip.width = right-left;
	clip.height = bottom-top;
	XSetClipRectangles(_wd, gc, 0, 0, &clip, 1, Unsorted);
}

/* Cancel any clipping in effect. */

void
wnoclip()
{
	XSetClipMask(_wd, gc, None);
}


/* Color stuff. */

void
_w_initcolors()
{
	_w_fgcolor = _wgetpixel("foreground", "Foreground",
						BlackPixelOfScreen(_ws));
	_w_bgcolor = _wgetpixel("background", "Background",
						WhitePixelOfScreen(_ws));
	
	/* Swap the pixel values if 'reverse' specified */
	if (_wgetbool("reverse", "Reverse", 0)) {
		unsigned long temp= _w_fgcolor;
		_w_fgcolor = _w_bgcolor;
		_w_bgcolor = temp;
	}
}

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
	if (color != BADCOLOR)
		_w_fgcolor = color;
	if (gc != 0)
		XSetForeground(_wd, gc, (unsigned long) _w_fgcolor);
}

void
wsetbgcolor(color)
	COLOR color;
{
	if (color != BADCOLOR)
		_w_bgcolor = color;
	if (gc != 0)
		XSetBackground(_wd, gc, (unsigned long) _w_bgcolor);
}


/* Bitmap stuff. */

BITMAP *
wnewbitmap(width, height)
	int width, height;
{
	char *data;
	XImage *xim;

	data = calloc(height * (width+31)/32, 4);
	if (data == NULL) {
		_werror("wnewbitmap: no mem for data");
		return NULL;
	}
	xim = XCreateImage(_wd, DefaultVisualOfScreen(_ws), 1, XYBitmap, 0,
			       data, width, height, 32, 0);
	if (xim == NULL) {
		_werror("wnewbitmap: XCreateImage failed");
		free(data);
		return NULL;
	}
	return (BITMAP *)xim;
}

void
wfreebitmap(bp)
	BITMAP *bp;
{
	if (bp)
		XDestroyImage((XImage*)bp);
}

void
wgetbitmapsize(bp, p_width, p_height)
	BITMAP *bp;
	int *p_width;
	int *p_height;
{
	XImage *xim = (XImage*)bp;
	*p_width = xim->width;
	*p_height = xim->height;
}

void
wsetbit(bp, h, v, bit)
	BITMAP *bp;
	int h, v;
	int bit;
{
	XImage *xim = (XImage*)bp;
	if (0 <= h && h < xim->width && 0 <= v && v < xim->height)
		XPutPixel(xim, h, v, (unsigned long)bit);
}

int
wgetbit(bp, h, v)
	BITMAP *bp;
	int h, v;
{
	XImage *xim = (XImage*)bp;
	if (0 <= h && h < xim->width && 0 <= v && v < xim->height)
		return XGetPixel(xim, h, v);
	else
		return 0;
}

void
wdrawbitmap(h, v, bp, mask)
	int h, v;
	BITMAP *bp;
	BITMAP *mask;
{
	XImage *xim = (XImage*)bp;
	if (mask == ALLBITS) {
		_wtrace(6, "wdrawbitmap(%d, %d, bp, ALLBITS)", h, v);
		XPutImage(_wd, wid, gc, xim, 0, 0, h, v,
			  xim->width, xim->height);
	}
	else if (mask == bp) {
		/* Create a bitmap in the server, fill it with our bits,
		   use it as a stipple, set the origin, and fill it */
		Pixmap pm = XCreatePixmap(_wd, wid,
					  xim->width, xim->height, 1);
		GC gc1 = XCreateGC(_wd, pm, 0L, (XGCValues*)0);
		_wtrace(6, "wdrawbitmap(%d, %d, bp, bp)", h, v);
		XSetForeground(_wd, gc1, 1L);
		XSetBackground(_wd, gc1, 0L);
		XPutImage(_wd, pm, gc1, xim, 0, 0, 0, 0,
			  xim->width, xim->height);
		XFreeGC(_wd, gc1);
		XSetStipple(_wd, gc, pm);
		XSetTSOrigin(_wd, gc, h, v);
		XSetFillStyle(_wd, gc, FillStippled);
		XFillRectangle(_wd, wid, gc, h, v, xim->width, xim->height);
		XSetFillStyle(_wd, gc, FillSolid);
		XFreePixmap(_wd, pm);
	}
	else {
		_werror("wdrawbitmap with generalized mask not supported");
	}
}
