/* ALFA STDWIN -- DRAWING. */

#include "alfa.h"

static char *textline[MAXLINES];
static char *modeline[MAXLINES];
static short textlen[MAXLINES];
static char texttouched[MAXLINES];

static TEXTATTR draw_saveattr;
static WINDOW *draw_win;

/* Cause a redraw of part of a window. */

/*ARGSUSED*/
void
wchange(win, left, top, right, bottom)
	WINDOW *win;
	int left, top;
	int right, bottom;
{
	int id= win - winlist;
	int i;
	
	if (id < 0 || id >= MAXWINDOWS || !win->open)
		return;
	top -= win->offset;
	bottom -= win->offset;
	if (top < win->top)
		top= win->top;
	if (bottom > win->bottom)
		bottom= win->bottom;
	for (i= top; i < bottom; ++i)
		uptodate[i]= FALSE;
	_wnewtitle(win);
}

/* (Try to) make sure a particular part of a window is visible,
   by scrolling in distant parts if necessary. */

void scrollupdate();

/*ARGSUSED*/
void
wshow(win, left, top, right, bottom)
	WINDOW *win;
	int left, top;
	int right, bottom;
{
	int id= win - winlist;
	int offset;
	int extra;
	int dv;
	
	if (id < 0 || id >= MAXWINDOWS || !win->open)
		return;
	extra= ( (win->bottom - win->top) - (bottom - top) ) / 2;
	if (extra < 0)
		extra= 0;
	offset= win->offset;
	if (bottom > win->bottom + offset)
		offset= bottom - win->bottom + extra;
	if (top < win->top + offset)
		offset= top - win->top - extra;
	if (win->top + offset < 0)
		offset= -win->top;
	dv= offset - win->offset;
	if (dv == 0)
		return;
	win->offset= offset;
	/* Now we'll use top, bottom to indicate the changed part. */
	top= win->top;
	bottom= win->bottom;
	scrollupdate(top, bottom, dv);
	trmscrollup(top, bottom-1, dv);
}

/* Similar, but by giving an explicit new origin */

/*ARGSUSED*/
void
wsetorigin(win, h, v)
	WINDOW *win;
	int h, v;
{
	int id= win - winlist;
	int top, bottom;
	int dv;

	if (id < 0 || id >= MAXWINDOWS || !win->open)
		return;
	dv = (v - win->top) - win->offset;
	if (dv == 0)
		return;
	win->offset += dv;
	top = win->top;
	bottom = win->bottom;
	scrollupdate(top, bottom, dv);
	trmscrollup(top, bottom-1, dv);
}

/* Scroll the update bits */

/*ARGSUSED*/
void
scrollupdate(top, bottom, dv)
	int top, bottom;
	int dv;
{
	int i;

#ifndef EUROGEMODDER
	for (i= top; i < bottom; ++i)
		uptodate[i] = FALSE;
#else
	/* XXX This code was never fixed to update the mode array */
	char *p;

	if(dv==0)
		return;
	if(dv > 0) {
		for (i= top; i < bottom; ++i) {
			if (uptodate[i+dv]) {
				p=textline[i];
				textline[i]=textline[i+dv];
				textlen[i]=textlen[i+dv];
				uptodate[i]= TRUE;
				textline[i+dv]= p;
				/*needed? textlen[i+dv]= 0; */
				uptodate[i+dv]= FALSE;
			} else {
				uptodate[i]= FALSE;
			}
		}
	} else {
		for (i= bottom-1; i >= top; --i) {
			if (uptodate[i+dv]) {
				p=textline[i];
				textline[i]=textline[i+dv];
				textlen[i]=textlen[i+dv];
				uptodate[i]= TRUE;
				textline[i+dv]= p;
				/*needed? textlen[i+dv]= 0; */
				uptodate[i+dv]= FALSE;
			} else {
				uptodate[i]= FALSE;
			}
		}
	}
#endif
}

/* Set the caret (cursor) position. */

void
wsetcaret(win, h, v)
	WINDOW *win;
	int h, v;
{
	win->curh= h;
	win->curv= v;
	/* wshow(win, h, v, h+1, v+1); */
	/* Shouldn't call wshow here -- it's not always desirable! */
}

/* Remove the caret altogether. */

void
wnocaret(win)
	WINDOW *win;
{
	win->curh= win->curv= -1;
}

/* Cause a redraw of the window 's title bar. */

void
_wnewtitle(win)
	WINDOW *win;
{
	if (win->top > 0)
		uptodate[win->top-1]= FALSE;
}

/* Compute the smallest rectangle that needs an update.
   As a side effect, uptodate is set to TRUE for the affected lines,
   and the lines are cleared and set to touched
   (but no actual output is performed).
   Return FALSE if the rectangle is empty. */

bool
wgetchange(win, pleft, ptop, pright, pbottom)
	WINDOW *win;
	int *pleft, *ptop, *pright, *pbottom;
{
	int top= win->bottom;
	int bottom= win->top;
	int i;
	
	for (i= win->top; i < win->bottom; ++i) {
		if (!uptodate[i]) {
			texttouched[i]= TRUE;
			textlen[i]= 0;
			uptodate[i]= TRUE;
			if (top > i)
				top= i;
			bottom= i+1;
		}
	}
	if (top > bottom) {
		*pleft= *pright= *ptop= *pbottom= 0;
		return FALSE;
	}
	else {
		*pleft= 0;
		*pright= columns;
		*ptop= top + win->offset;
		*pbottom= bottom + win->offset;
		return TRUE;
	}
}

void
wupdate(win)
	WINDOW *win;
{
	int left, top, right, bottom;
	
	wgetchange(win, &left, &top, &right, &bottom);
	
	wbegindrawing(win);
	
	if (win->drawproc != NULL)
		(*win->drawproc)(win, 0, top, columns, bottom);
	
	wenddrawing(win);
}

void
wbegindrawing(win)
	WINDOW *win;
{
	int i, j;
	
	if (draw_win != NULL)
		wenddrawing(draw_win); /* Finish previous job */
	
	/* Auto-initialization of the textline and modeline array.
	   The other arrays needn't be initialized,
	   since they are preset to zero. */
	/* XXX Crash if malloc() fails! */
	for (i= win->top; i < win->bottom; ++i) {
		if (textline[i] == NULL)
			textline[i]= malloc((unsigned) (columns+1));
		if (modeline[i] == NULL) {
			modeline[i]= malloc((unsigned) columns);
			for (j = 0; j < columns; j++)
				modeline[i][j] = PLAIN;
		}
	}
	
	draw_saveattr= wattr;
	wattr= win->attr;
	draw_win= win;
}

void
wenddrawing(win)
	WINDOW *win;
{
	int i;
	
	if (draw_win != win) {
		return; /* Bad call */
	}
	
	for (i= win->top; i < win->bottom; ++i) {
		if (texttouched[i]) {
			texttouched[i]= FALSE;
			textline[i][textlen[i]]= EOS;
			trmputdata(i, i, 0, textline[i], modeline[i]);
		}
	}
	
	wattr= draw_saveattr;
	draw_win= NULL;
}

void
wdrawtitle(win)
	WINDOW *win;
{
	int titline= win->top - 1;
	char buf[256];
	char *title= win->title == NULL ? "" : win->title;
	int tlen= strlen(title);
	int space= columns-tlen;
	int mask= (win == front) ? 0200 : 0;
	int k;
	
	for (k= 0; k < space/2 - 1; ++k)
		buf[k]= '-';
	buf[k++]= ' ';
	while (k < columns && *title != EOS)
		buf[k++]= *title++;
	if (k < columns)
		buf[k++]= ' ';
	while (k < columns)
		buf[k++]= '-';
	buf[k]= EOS;
	if (win == front) {
		char mode[256];
		int i;
		for (i = 0; i < k; i++)
			mode[i] = STANDOUT;
		trmputdata(titline, titline, 0, buf, mode);
	}
	else {
		trmputdata(titline, titline, 0, buf, (char *)0);
	}
	uptodate[titline]= TRUE;
}

void
wdrawtext(h, v, str, len)
	int h, v;
	char *str;
	int len;
{
	int k;
	int flag= (wattr.style == 0) ? PLAIN : STANDOUT;
	char *text;
	char *mode;
	WINDOW *win= draw_win;
	
	if (len < 0)
		len= strlen(str);
	v -= win->offset;
	if (v < win->top || v >= win->bottom || h >= columns || len == 0)
		return;
	text= textline[v];
	mode= modeline[v];
	k= textlen[v];
	for (; k < h; k++) {
		text[k]= ' ';
		mode[k]= PLAIN;
	}
	while (len > 0) {
#define APPC(c) \
	if (h >= 0) {text[h]= (c); mode[h]= flag;} if (++h >= columns) break
		unsigned char c= *str++;
		--len;
		if (c < ' ') {
			APPC('^');
			APPC(c | '@');
		}
		else  if (c < 0177) {
			APPC(c);
		}
		else if (c < 0200) {
			APPC('^');
			APPC('?');
		}
		else {
			APPC('\\');
			APPC('0' | ((c>>6) & 07));
			APPC('0' | ((c>>3) & 07));
			APPC('0' | (c & 07));
		}
#undef APPC
	}
	if (h > textlen[v])
		textlen[v]= h;
	texttouched[v]= TRUE;
}

void
wdrawchar(h, v, c)
	int h, v;
	int c;
{
	char cbuf[2];
	cbuf[0]= c;
	cbuf[1]= EOS;
	wdrawtext(h, v, cbuf, 1);
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
}

/* Non-text drawing dummy functions: */

/*ARGSUSED*/
void
wdrawline(h1, v1, h2, v2)
	int h1, v1, h2, v2;
{
}

/*ARGSUSED*/
void
wxorline(h1, v1, h2, v2)
	int h1, v1, h2, v2;
{
}

/*ARGSUSED*/
void
wdrawcircle(h, v, radius)
	int h, v;
	int radius;
{
}

/*ARGSUSED*/
void
wfillcircle(h, v, radius)
	int h, v;
	int radius;
{
}

/*ARGSUSED*/
void
wxorcircle(h, v, radius)
	int h, v;
	int radius;
{
}

/*ARGSUSED*/
void
wdrawelarc(h, v, radh, radv, ang1, ang2)
	int h, v;
	int radh, radv;
	int ang1, ang2;
{
}

/*ARGSUSED*/
void
wfillelarc(h, v, radh, radv, ang1, ang2)
	int h, v;
	int radh, radv;
	int ang1, ang2;
{
}

/*ARGSUSED*/
void
wxorelarc(h, v, radh, radv, ang1, ang2)
	int h, v;
	int radh, radv;
	int ang1, ang2;
{
}

/*ARGSUSED*/
void
wdrawbox(left, top, right, bottom)
	int left, top, right, bottom;
{
}

/*ARGSUSED*/
void
wpaint(left, top, right, bottom)
	int left, top, right, bottom;
{
}

/*ARGSUSED*/
void
wshade(left, top, right, bottom, percentage)
	int left, top, right, bottom;
	int percentage;
{
}

/*ARGSUSED*/
void
winvert(left, top, right, bottom)
	int left, top, right, bottom;
{
	WINDOW *win= draw_win;
	int v;
	
	if (left < 0)
		left= 0;
	if (right >= columns)
		right= columns;
	if (left >= right)
		return;
	top -= win->offset;
	bottom -= win->offset;
	if (top < win->top)
		top= win->top;
	if (bottom > win->bottom)
		bottom= win->bottom;
	for (v= top; v < bottom; ++v) {
		int k= textlen[v];
		char *text= textline[v];
		char *mode= modeline[v];
		if (k < right) {
			do {
				text[k]= ' ';
				mode[k]= PLAIN;
				k++;
			} while (k < right);
			textlen[v]= k;
		}
		for (k= left; k < right; ++k) {
			if (mode[k] != PLAIN)
				mode[k]= PLAIN;
			else
				mode[k]= STANDOUT;
		}
		texttouched[v]= TRUE;
	}
}

/*ARGSUSED*/
void
werase(left, top, right, bottom)
	int left, top, right, bottom;
{
	WINDOW *win= draw_win;
	int v;
	
	if (left < 0)
		left= 0;
	if (right >= columns)
		right= columns;
	if (left >= right)
		return;
	top -= win->offset;
	bottom -= win->offset;
	if (top < win->top)
		top= win->top;
	if (bottom > win->bottom)
		bottom= win->bottom;
	for (v= top; v < bottom; ++v) {
		int k= textlen[v];
		char *text= textline[v];
		char *mode= modeline[v];
		if (k > right)
			k= right;
		while (--k >= left) {
			text[k]= ' ';
			mode[k]= PLAIN;
		}
		texttouched[v]= TRUE;
	}
}

/*ARGSUSED*/
void
wdrawpoly(n, points)
	int n;
	POINT *points;
{
}

/*ARGSUSED*/
void
wfillpoly(n, points)
	int n;
	POINT *points;
{
}

/*ARGSUSED*/
void
wxorpoly(n, points)
	int n;
	POINT *points;
{
}

/*ARGSUSED*/
void
wcliprect(left, top, right, bottom)
	int left, top, right, bottom;
{
	/* XXX not implemented */
}

/*ARGSUSED*/
void
wnoclip()
{
	/* XXX not implemented */
}

/*ARGSUSED*/
int
wsetfont(fontname)
	char *fontname;
{
	return 0; /* There are no fonts... */
}

/*ARGSUSED*/
void
wsetsize(size)
	int size;
{
}

/*ARGSUSED*/
COLOR
wfetchcolor(colorname)
	char *colorname;
{
	return 0;
}

/*ARGSUSED*/
void
wsetfgcolor(color)
	COLOR color;
{
}

COLOR
wgetfgcolor()
{
	return 0;
}

/*ARGSUSED*/
void
wsetbgcolor(color)
	COLOR color;
{
}

COLOR
wgetbgcolor()
{
	return 0;
}
