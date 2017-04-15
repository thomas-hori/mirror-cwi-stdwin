/* X11 STDWIN -- Scroll bars */

#include "x11.h"

#define IMARGIN 2 /* See window.c */

/* Macro to compute the thumb position and size.
   Don't call when docwidth (docheight) is zero. */

/* dir is x or y; dim is width or height; bar is hbar or vbar */
#define BARCOMPUTE(dir, dim, bar) { \
		range= win->bar.dim; \
		start= range * (-win->wa.dir) / win->doc.dim; \
		size= range * win->wi.dim / win->doc.dim; \
		_wdebug(4, "BARCOMPUTE: start=%d, size=%d", start, size); \
	}

/* Functions to draw the scroll bars.
   Currently, the thumb is a rectangle separated by the scroll bar's
   borders by a one-pixel wide space */

_wdrawhbar(win)
	WINDOW *win;
{
	if (win->hbar.wid == None)
		return;
	_wdebug(3, "draw hbar");
	XClearWindow(_wd, win->hbar.wid);
	if (win->doc.width > win->wi.width) {
		int start, size, range;
		BARCOMPUTE(x, width, hbar);
		XSetTile(_wd, win->gc, _w_gray());
		XSetFillStyle(_wd, win->gc, FillTiled);
		XFillRectangle(_wd, win->hbar.wid, win->gc,
			start + 1, 1, size - 1, win->hbar.height - 2);
		XSetFillStyle(_wd, win->gc, FillSolid);
	}
}

_wdrawvbar(win)
	WINDOW *win;
{
	if (win->vbar.wid == None)
		return;
	_wdebug(3, "draw vbar");
	XClearWindow(_wd, win->vbar.wid);
	if (win->doc.height > win->wi.height) {
		int start, size, range;
		BARCOMPUTE(y, height, vbar);
		XSetTile(_wd, win->gc, _w_gray());
		XSetFillStyle(_wd, win->gc, FillTiled);
		XFillRectangle(_wd, win->vbar.wid, win->gc,
			1, start + 1, win->vbar.width - 2, size - 1);
		XSetFillStyle(_wd, win->gc, FillSolid);
	}
}

/* Functions to interpret scroll bar hits.
   Matching the Toolkit scroll bars, we should implement:
   - button 1: scroll forward, amount determined by position
   - button 2: position scroll bar begin at pointer
   - buttom 3: scroll back, amount determined by position
*/

/* dir is x or y; dim is width or height; bar is hbar or vbar;
   drawcall is _wdraw{h,v}bar; imargin is IMARGIN or 0. */
#define HITCODE(dir, dim, bar, drawcall, imargin) { \
		WINDOW *win= bsp->win; \
		int start, size, range, nstart; \
		if (win->doc.dim <= win->wi.dim - imargin) \
			return; \
		BARCOMPUTE(dir, dim, bar); \
		switch (bsp->button) { \
		case 1: \
			if (bsp->down) return; \
			nstart= start + \
				(bsp->dir * win->wi.dim + win->doc.dim/2) \
					/ win->doc.dim; \
			break; \
		default: \
			nstart= bsp->dir; \
			break; \
		case 3: \
			if (bsp->down) return; \
			nstart= start - \
				(bsp->dir * win->wi.dim + win->doc.dim/2) \
					/ win->doc.dim; \
			break; \
		} \
		CLIPMAX(nstart, range-size); \
		CLIPMIN(nstart, 0); \
		if (nstart != start) { \
			win->wa.dir= \
				-(nstart * win->doc.dim / win->bar.dim); \
			_wmove(&win->wa); \
			drawcall(win); \
		} \
	}

_whithbar(bsp, ep)
	struct button_state *bsp;
	EVENT *ep;
{
	_wdebug(3, "hit hbar");
	HITCODE(x, width, hbar, _wdrawhbar, IMARGIN);
}

_whitvbar(bsp, ep)
	struct button_state *bsp;
	EVENT *ep;
{
	_wdebug(3, "hit vbar");
	HITCODE(y, height, vbar, _wdrawvbar, 0);
}
