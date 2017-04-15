/* Text Edit, low-level routines */

/* XXX Should make return-less functions void */

/* CONVENTION:
	routines beginning with te... have a first parameter 'tp';
	routines beginning with z... don't, or are macros that
	implicitly use a variable or parameter 'tp'
*/

#include "text.h"

extern void dprintf _ARGS((char *, ...));

/* Forward */
static void teinvertfocus();
static void teinvert();
static void teshift();
static void teoffset();
static void temoveup();
static void temovedown();

/* Variants on wtextwidth, wtextbreak, wdrawtext taking the gap in account.
   These have two buffer offsets as parameters instead of a text pointer
   and a length; tetextbreak also returns a buffer offset */

/* These routines now also take tabs into account.
   They should now only be called with a line start for 'pos' ! */

int
tetextwidth(tp, pos, end)
	register TEXTEDIT *tp;
	bufpos pos, end;
{
	char *p= tp->buf + pos;
	register char *e= tp->buf +
		(tp->gap >= pos && tp->gap < end ? tp->gap : end);
	int w= 0;
	register char *k;
	
	zcheckpos(pos);
	zcheckpos(end);
	zassert(pos <= end);
	
	/* This do loop is executed once or twice only! */
	do {
		for (k= p; k < e; ++k) {
			if (*k == '\t') {
				if (k > p)
					w += wtextwidth(p, (int)(k-p));
				w= znexttab(w);
				p= k+1;
			}
		}
		if (k > p)
			w += wtextwidth(p, (int)(k-p));
		if (tp->gap >= pos) {
			p= tp->buf + zgapend;
			e= tp->buf + end;
		}
	} while (k < e);
	
	return w;
}

bufpos
tetextbreak(tp, pos, end, width)
	register TEXTEDIT *tp;
	bufpos pos, end;
	int width;
{
	char *p= tp->buf + pos;
	register char *e= tp->buf +
		(tp->gap >= pos && tp->gap < end ? tp->gap : end);
	int w= 0;
	register char *k;
	
	zcheckpos(pos);
	zcheckpos(end);
	zassert(pos <= end);
	
	/* This do loop is executed once or twice only! */
	do {
		for (k= p; k < e; ++k) {
			if (*k == '\t') {
				if (k > p) {
					int n= wtextbreak(p, (int)(k-p),
						width-w);
					if (n < k-p)
						return p - tp->buf + n;
					w += wtextwidth(p, (int)(k-p));
				}
				w= znexttab(w);
				if (w > width)
					return k - tp->buf;
				p= k+1;
			}
		}
		if (k > p) {
			int n= wtextbreak(p, (int)(k-p), width-w);
			if (n < k-p)
				return p - tp->buf + n;
			w += wtextwidth(p, (int)(k-p));
		}
		if (tp->gap >= pos) {
			p= tp->buf + zgapend;
			e= tp->buf + end;
		}
	} while (k < e);
	
	return end;
}

hcoord
tedrawtext(tp, h, v, pos, end)
	register TEXTEDIT *tp;
	int h, v;
	bufpos pos, end;
{
	char *p= tp->buf + pos;
	register char *e= tp->buf +
		(tp->gap >= pos && tp->gap < end ? tp->gap : end);
	int w= 0;
	register char *k;
	
	zcheckpos(pos);
	zcheckpos(end);
	zassert(pos <= end);
	
	/* This do loop is executed once or twice only! */
	do {
		for (k= p; k < e; ++k) {
			if (*k == '\t') {
				if (k > p)
					wdrawtext(h+w, v, p, (int)(k-p));
					w += wtextwidth(p, (int)(k-p));
				w= znexttab(w);
				p= k+1;
			}
		}
		if (k > p) {
			wdrawtext(h+w, v, p, (int)(k-p));
			w += wtextwidth(p, (int)(k-p));
		}
		if (tp->gap >= pos) {
			p= tp->buf + zgapend;
			e= tp->buf + end;
		}
	} while (k < e);
	
	return h+w;
}

/* Safe memory allocation - these abort instead of returning NULL */

char *
zmalloc(n)
	int n;
{
	char *p= malloc((unsigned)n);
	
	if (p == NULL) {
		dprintf("zmalloc(%d): out of mem", n);
		exit(1);
	}
	return p;
}

char *
zrealloc(p, n)
	char *p;
	int n;
{
	char *q= realloc(p, (unsigned)n);
	if (q == NULL) {
		dprintf("zrealloc(0x%lx, %d): out of mem", (long)p, n);
		exit(1);
	}
	return q;
}

/* Create/destroy a text-edit record */

TEXTEDIT *
tealloc(win, left, top, width)
	WINDOW *win;
{
	return tesetup(win, left, top, left+width, top, TRUE);
}

TEXTEDIT *
tecreate(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	return tesetup(win, left, top, right, bottom, TRUE);
}

/*ARGSUSED*/
TEXTEDIT *
tesetup(win, left, top, right, bottom, drawing)
	WINDOW *win;
	int left, top, right, bottom;
	bool drawing;
{
	TEXTEDIT *tp= (TEXTEDIT*) zmalloc(sizeof(TEXTEDIT));
	TEXTATTR saveattr;
	
	tp->win= win;
	tp->left= left;
	tp->top= top;
	tp->right= right;
	tp->width= right-left;

	tp->viewing= FALSE;
	
	wgettextattr(&saveattr);
	if (win != NULL) {
		wgetwintextattr(win, &tp->attr);
		wsettextattr(&tp->attr);
	}
	else
		tp->attr = saveattr;
	tp->vspace= wlineheight();
	tp->tabsize= 8*wcharwidth(' ');
	if (win != NULL)
		wsettextattr(&saveattr);
	
	tp->bottom= tp->top + tp->vspace;
	
	tp->foc= tp->foclen= 0;
	
	tp->buflen= 1;
	tp->buf= zmalloc(tp->buflen);
	
	tp->gap= 0;
	tp->gaplen= tp->buflen;
	
	tp->nlines= 1;
	tp->nstart= STARTINCR;
	tp->start= (bufpos*) zmalloc(tp->nstart*sizeof(bufpos));
	tp->start[0]= tp->start[1]= tp->buflen;
	
	tp->aim= UNDEF;
	tp->focprev= FALSE;
	tp->hilite= FALSE;
	tp->mdown= FALSE;
	tp->drawing= tp->active= drawing;
	tp->opt_valid= FALSE;
	
	if (tp->active)
		tesetcaret(tp);
	
	zcheck();
	
	return tp;
}


void
tedestroy(tp)
	register TEXTEDIT *tp;
{
	if (tp->viewing)
		wchange(tp->win, tp->vleft, tp->vtop, tp->vright, tp->vbottom);
	else
		wchange(tp->win, tp->left, tp->top, tp->right, tp->bottom);
	tefree(tp);
}

void
tefree(tp)
	register TEXTEDIT *tp;
{
	if (tp->active) {
		wnocaret(tp->win);
		tehidefocus(tp);
	}
	if (tp->buf != NULL)
		free(tp->buf);
	if (tp->start != NULL)
		free((char*)tp->start);
	free((char*)tp);
}

void
tesetactive(tp, active)
	register TEXTEDIT *tp;
	bool active;
{
	if (!tp->drawing || tp->active == active)
		return;
	tp->active = active;
	if (active) {
		tesetcaret(tp);
	}
	else {
		wnocaret(tp->win);
		tehidefocus(tp);
	}
}

/* Show/hide the focus highlighting.
   The boolean hilite is set when highlighting is visible.
   teshowfocus ensures the highlighting is visible (if applicable);
   tehidefocus ensures it is invisible.
   teinvertfocus does the hard work (it is also called from zdraw) */

teshowfocus(tp)
	register TEXTEDIT *tp;
{
	if (tp->active && !tp->hilite && tp->foclen > 0) {
		wbegindrawing(tp->win);
		if (tp->viewing)
		    wcliprect(tp->vleft, tp->vtop, tp->vright, tp->vbottom);
		teinvertfocus(tp);
		wenddrawing(tp->win);
		tp->hilite= TRUE;
	}
}

tehidefocus(tp)
	register TEXTEDIT *tp;
{
	if (tp->hilite) {
		wbegindrawing(tp->win);
		if (tp->viewing)
		    wcliprect(tp->vleft, tp->vtop, tp->vright, tp->vbottom);
		teinvertfocus(tp);
		wenddrawing(tp->win);
		tp->hilite= FALSE;
	}
}

static void
teinvertfocus(tp)
	register TEXTEDIT *tp;
{
	teinvert(tp, tp->foc, zfocend);
}

/* Change to a new focus.
   Sometimes this may keep the focus visible, sometimes not. */

techangefocus(tp, f1, f2)
	register TEXTEDIT *tp;
	int f1, f2;
{
	if (tp->hilite) {
		wbegindrawing(tp->win);
		if (tp->viewing)
		    wcliprect(tp->vleft, tp->vtop, tp->vright, tp->vbottom);
		if (f1 == tp->foc)
			teinvert(tp, zfocend, f2);
		else if (f2 == zfocend)
			teinvert(tp, f1, tp->foc);
		else {
			teinvert(tp, tp->foc, zfocend);
			tp->hilite= FALSE;
		}
		wenddrawing(tp->win);
	}
	tp->foc= f1;
	tp->foclen= f2-f1;
}

/* Low-level interface: invert the area between f1 and f2 */

static void
teinvert(tp, f1, f2)
	register TEXTEDIT *tp;
	int f1, f2;
{
	coord h, v, hlast, vlast;
	
	if (f1 == f2)
		return;
	if (f2 < f1) {
		int temp= f1;
		f1= f2;
		f2= temp;
	}
	
	tewhichpoint(tp, f1, &h, &v);
	tewhichpoint(tp, f2, &hlast, &vlast);
	
	if (v == vlast)
		winvert(h, v, hlast, v + tp->vspace);
	else {
		winvert(h, v, tp->right, v + tp->vspace);
		winvert(tp->left, v + tp->vspace, tp->right, vlast);
		winvert(tp->left, vlast, hlast, vlast + tp->vspace);
	}
}

/* Draw procedure */

void
tedraw(tp)
	register TEXTEDIT *tp;
{
	tedrawnew(tp, tp->left, tp->top, tp->right, tp->bottom);
}

void
tedrawnew(tp, left, top, right, bottom)
	register TEXTEDIT *tp;
	coord left, top, right, bottom;
{
	lineno ifirst, ilast, i;

	/* Clip given area to view */
	if (tp->viewing) {
		CLIPMIN(left, tp->vleft);
		CLIPMIN(top, tp->vtop);
		CLIPMAX(right, tp->vright);
		CLIPMAX(bottom, tp->vbottom);
	}
	
	/* Restrict drawing to intersection of view and given area */
	wcliprect(left, top, right, bottom);
	
	/* Compute first, last line to be drawn */
	ifirst= (top - tp->top)/tp->vspace;
	if (ifirst < 0)
		ifirst= 0;
	ilast= (bottom - tp->top + tp->vspace - 1)/tp->vspace;
	if (ilast > tp->nlines)
		ilast= tp->nlines;
	
	/* Draw lines ifirst...ilast-1 */
	for (i= ifirst; i < ilast; ++i) {
		bufpos pos= tp->start[i];
		bufpos end= tp->start[i+1];
		if (end > pos && zcharbefore(end) == EOL)
			zdecr(&end);
		while (end > pos && zcharbefore(end) == ' ')
			zdecr(&end);
		(void) tedrawtext(tp, tp->left, tp->top + i*tp->vspace,
			pos, end);
	}
	if (tp->hilite)
		teinvertfocus(tp);
	
	/* Reset the clip rectangle */
	wnoclip();
}

/* Move the gap to a new position */

temovegapto(tp, newgap)
	register TEXTEDIT *tp;
	bufpos newgap;
{
	zcheck();
	zassert(0<=newgap && newgap+tp->gaplen<=tp->buflen);
	
	if (newgap < tp->gap)
		teshift(tp, tp->gaplen, newgap, tp->gap);
	else if (newgap > tp->gap)
		teshift(tp, -tp->gaplen, zgapend, newgap+tp->gaplen);
	tp->gap= newgap;
	
	zcheck();
}

/* Extend the gap */

tegrowgapby(tp, add)
	register TEXTEDIT *tp;
	int add;
{
	zcheck();
	zassert(add>=0);
	
	tp->buf= zrealloc(tp->buf, tp->buflen + add);
	teshift(tp, add, zgapend, tp->buflen);
	tp->gaplen += add;
	if (tp->start[tp->nlines-1] == tp->buflen)
		tp->start[tp->nlines-1]= tp->buflen+add;
	tp->start[tp->nlines]= (tp->buflen += add);
	
	zcheck();
}

/* Shift buf[first..last-1] n bytes (positive right, negative left) */

static void
teshift(tp, n, first, last)
	register TEXTEDIT *tp;
	int n;
	bufpos first, last;
{
	teoffset(tp, n, first, last);
	if (n < 0)
		temovedown(tp, last-first, tp->buf+first, tp->buf+first+n);
	else if (n > 0)
		temoveup(tp, last-first, tp->buf+first, tp->buf+first+n);
}

static void
teoffset(tp, n, first, last)
	register TEXTEDIT *tp;
	int n;
	int first, last;
{
	int i;
	
	zassert(0<=first&&first<=last&&last<=tp->buflen);
	
	i= 0;
	while (tp->start[i] < first)
		++i;
	while (tp->start[i] < last) {
		tp->start[i] += n;
		++i;
	}
}

/*ARGSUSED*/
static void
temoveup(tp, n, from, to)
	TEXTEDIT *tp;
	int n;
	char *from, *to;
{
	zassert(from <= to);
	
	from += n, to += n;
	while (--n >= 0)
		*--to = *--from;
}

/*ARGSUSED*/
static void
temovedown(tp, n, from, to)
	TEXTEDIT *tp;
	int n;
	char *from, *to;
{
	zassert(from >= to);
	
	while (--n >= 0)
		*to++ = *from++;
}

/* Make all start entries pointing into the gap point to beyond it
   TO DO: replace by a routine to delete the focus??? */

teemptygap(tp)
	register TEXTEDIT *tp;
{
	lineno i;
	
	for (i= 0; tp->start[i] < tp->gap; ++i)
		;
	for (; tp->start[i] < zgapend; ++i)
		tp->start[i]= zgapend;
}

/* Interface for wshow that clips to the viewing rectangle */

static void
teshow(tp, left, top, right, bottom)
	TEXTEDIT *tp;
	int left, top, right, bottom;
{
	if (tp->viewing) {
		CLIPMIN(left, tp->vleft);
		CLIPMIN(top, tp->vtop);
		CLIPMAX(right, tp->vright);
		CLIPMAX(bottom, tp->vbottom);
	}
	wshow(tp->win, left, top, right, bottom);
}

/* Set the caret at the new focus position,
   or display the focus highlighting, if applicable.
   Also call wshow() of the focus.
   As a side effect, the optimization data is invalidated */

tesetcaret(tp)
	register TEXTEDIT *tp;
{
	coord h, v, hlast, vlast;
	
	tp->opt_valid = FALSE;
	if (!tp->active)
		return;
	
	tewhichpoint(tp, tp->foc, &h, &v);
	
	if (tp->foclen == 0) {
		if (!tp->viewing ||
		    tp->vleft <= h && h <= tp->vright &&
		    tp->vtop <= v && v + tp->vspace <= tp->vbottom) {
			wsetcaret(tp->win, h, v);
		}
		else {
			wnocaret(tp->win);
		}
		hlast= h;
		vlast= v;
	}
	else {
		tewhichpoint(tp, zfocend, &hlast, &vlast);
		wnocaret(tp->win);
		teshowfocus(tp);
	}
	teshow(tp, h, v, hlast, vlast + tp->vspace);
}

/* Coordinate transformations.
   The following coordinate systems exist;
   a position in the text can be expressed in any of these:
   
   	A) offset in buffer with gap removed (used for focus)
	B) offset in buffer (used for start[] array)
	C) (line number, offset in line taking gap into account)
	D) (h, v) on screen
   
   Conversions exist between successive pairs:
   
   	A -> B: pos= zaddgap(foc)
	B -> A: foc= zsubgap(pos)
	
	B -> C: line= zwhichline(pos, prev); offset= pos-start[line]
	C -> B: pos= offset + start[line]
	
	C -> D: v= i*vspace; h= ztextwidth(start[i], start[i]+offset)
	D -> C: i= v/wlh; offset= ztextround(i, h) - start[i]
*/

/* Find (h, v) corresponding to focus position */

tewhichpoint(tp, f, h_ret, v_ret)
	TEXTEDIT *tp;
	focpos f;
	coord *h_ret, *v_ret;
{
	bufpos pos= zaddgap(f);
	lineno i= tewhichline(tp, pos, f == tp->foc && tp->focprev);
	hcoord h= tetextwidth(tp, tp->start[i], pos);
	
	*h_ret= h + tp->left;
	*v_ret= i*tp->vspace + tp->top;
}

/* To which line does the given buffer position belong? */

lineno
tewhichline(tp, pos, prev)
	register TEXTEDIT *tp;
	bufpos pos;
	bool prev; /* Cf. focprev */
{
	lineno i;
	
	for (i= 0; pos > tp->start[i+1]; ++i)
		;
	if (pos == tp->start[i+1] && i+1 < tp->nlines) {
		++i;
		if (prev && zcharbefore(tp->start[i]) != EOL)
			--i;
	}
	
	return i;
}

/* Convert point in window to buffer position,
   possibly taking double-clicking into account.
   If required, the line number is also returned. */

bufpos
tewhereis(tp, h, v, line_return)
	register TEXTEDIT *tp;
	coord h, v;
	int *line_return;
{
	bufpos pos;
	lineno i;
	
	i= (v - tp->top)/tp->vspace;
	if (i >= tp->nlines) {
		i= tp->nlines;
		pos= tp->buflen;
	}
	else if (i < 0) {
		i= 0;
		pos= 0;
	}
	else
		pos= tetextround(tp, i, h);
	if (line_return != NULL)
		*line_return= i;
	return pos;
}

/* Find the buffer position nearest to the given h coordinate,
   in the given line */

bufpos
tetextround(tp, i, h)
	register TEXTEDIT *tp;
	lineno i;
	hcoord h;
{
	bufpos pos;
	bufpos end= tp->start[i+1];
	
	h -= tp->left;
	if (end > tp->start[i] && zcharbefore(end) == EOL)
		zdecr(&end);
	pos= tetextbreak(tp, tp->start[i], end, h);
	
	if (pos < end) {
		if (h - tetextwidth(tp, tp->start[i], pos) >=
			tetextwidth(tp, tp->start[i], znext(pos)) - h)
			zincr(&pos);
	}
	
	return pos;
}
