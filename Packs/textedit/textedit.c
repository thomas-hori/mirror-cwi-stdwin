/* Text Edit, high level routines */

#include "text.h"

/* Forward */
static void teinschar();
static void tesetoptdata();
static bool teoptinschar();
static void teinsert();
static void terecompute();
static int tesetstart();
static int teendofline();
static int tewordbegin();
static int tewordend();
static int _wdrawpar();

void
tereplace(tp, str)
	TEXTEDIT *tp;
	char *str;
{
	int len= strlen(str);
	
	if (len == 1 && teoptinschar(tp, (int) str[0]))
		return;
	
	teinsert(tp, str, len);
}

static void
teinschar(tp, c)
	TEXTEDIT *tp;
	int c;
{
	char cbuf[2];
	
	if (teoptinschar(tp, c))
		return;
	
	cbuf[0]= c;
	cbuf[1]= EOS;
	teinsert(tp, cbuf, 1);
}

/* Interfaces for wchange and wscroll that clip to the viewing rectangle */

static void
techange(tp, left, top, right, bottom)
	TEXTEDIT *tp;
	int left, top, right, bottom;
{
	if (tp->viewing) {
		CLIPMIN(left, tp->vleft);
		CLIPMIN(top, tp->vtop);
		CLIPMAX(right, tp->vright);
		CLIPMAX(bottom, tp->vbottom);
	}
	wchange(tp->win, left, top, right, bottom);
}

static void
tescroll(tp, left, top, right, bottom, dh, dv)
	TEXTEDIT *tp;
	int left, top, right, bottom;
	int dh, dv;
{
	if (tp->viewing) {
		CLIPMIN(left, tp->vleft);
		CLIPMIN(top, tp->vtop);
		CLIPMAX(right, tp->vright);
		CLIPMAX(bottom, tp->vbottom);
	}
	wscroll(tp->win, left, top, right, bottom, dh, dv);
	/* XXX Should call wchange for bits scrolled in from outside view? */
}

/* Optimization for the common case insert char.
   Assumes text measurement is additive. */

static void
tesetoptdata(tp)
	TEXTEDIT *tp;
{
	lineno i;
	bufpos k, pos, end;
	
	zcheck();
	zassert(tp->foclen == 0);
	
	pos= zaddgap(tp->foc);
	tp->opt_i= i= tewhichline(tp, pos, FALSE);
	tp->opt_h= tp->left + tetextwidth(tp, tp->start[i], pos);
	tp->opt_v= tp->top + i*tp->vspace;
	end= tp->start[i+1];
	if (end > pos && zcharbefore(end) == EOL)
		zdecr(&end);
	while (end > pos && zcharbefore(end) == ' ')
		zdecr(&end);
	for (k= pos; k < end; zincr(&k)) {
		if (zcharat(k) == '\t')
			break;
	}
	if (k < end) {
		tp->opt_end=
			tp->left + tetextwidth(tp, tp->start[i], znext(k));
		tp->opt_avail= tp->opt_end -
			(tp->left + tetextwidth(tp, tp->start[i], k));
	}
	else {
		tp->opt_end= tp->right;
		tp->opt_avail= tp->width - tetextwidth(tp, tp->start[i], end);
	}
	if (tp->start[i] > 0 && zcharbefore(tp->start[i]) != EOL) {
		tp->opt_in_first_word= TRUE;
		for (k= tp->start[i]; k < pos; zincr(&k)) {
			if (isspace(zcharat(k))) {
				tp->opt_in_first_word= FALSE;
				break;
			}
		}
	}
	else
		tp->opt_in_first_word= FALSE;
	tp->opt_valid= TRUE;
}

static bool
teoptinschar(tp, c)
	TEXTEDIT *tp;
	int c;
{
	int w;
	
	if (tp->foclen != 0 || c == EOL || c == '\t' || tp->focprev)
		return FALSE;
	if (!tp->opt_valid)
		tesetoptdata(tp);
	if (c == ' ' && tp->opt_in_first_word)
		return FALSE;
	w= wcharwidth(c);
	if (w >= tp->opt_avail)
		return FALSE;
	
	temovegapto(tp, tp->foc);
	if (tp->gaplen < 1)
		tegrowgapby(tp, 1+RESERVE);
	if (tp->start[tp->opt_i] == zgapend)
		tp->start[tp->opt_i]= tp->gap;
	++tp->gap;
	--tp->gaplen;
	tp->buf[tp->foc]= c;
	++tp->foc;
	
	tp->opt_avail -= w;
	if (tp->active)
		wnocaret(tp->win);
	tescroll(tp,
		tp->opt_h, tp->opt_v,
		tp->opt_end, tp->opt_v + tp->vspace,
		w, 0);
	wbegindrawing(tp->win);
	if (tp->viewing)
		wcliprect(tp->vleft, tp->vtop, tp->vright, tp->vbottom);
	wdrawchar(tp->opt_h, tp->opt_v, c);
	wenddrawing(tp->win);
	tp->opt_h += w;
	if (tp->active) {
		if (!tp->viewing ||
		    tp->vleft <= tp->opt_h &&
		    tp->opt_h <= tp->vright &&
		    tp->vtop <= tp->opt_v &&
		    tp->opt_v + tp->vspace <= tp->vbottom) {
			wsetcaret(tp->win, tp->opt_h, tp->opt_v);
			wshow(tp->win,
			      tp->opt_h, tp->opt_v,
			      tp->opt_h, tp->opt_v + tp->vspace);
		}
		else
			wnocaret(tp->win);
	}
	tp->aim= tp->opt_h;
	
	return TRUE;
}

static void
teinsert(tp, str, len)
	TEXTEDIT *tp;
	char *str;
	int len;
{
	focpos oldfoc= tp->foc;
	
	tehidefocus(tp);
	temovegapto(tp, zfocend);
	tp->gap= tp->foc;
	tp->gaplen += tp->foclen;
	teemptygap(tp);
	tp->foclen= 0;
	if (tp->gaplen < len)
		tegrowgapby(tp, len-tp->gaplen+RESERVE);
	strncpy(tp->buf+tp->gap, str, len);
	tp->gap += len;
	tp->gaplen -= len;
	tp->foc += len;
	terecompute(tp, zaddgap(oldfoc), zaddgap(tp->foc));
}

static int lasteol;	/* Optimization trick for teendofline */

static void
terecompute(tp, first, last)
	TEXTEDIT *tp;
	int first, last;
{
	lineno i;
	lineno chfirst, chlast; /* Area to pass to wchange */
	lineno shift= 0; /* Lines to shift down (negative: up) */
	vcoord newbottom;
	
	tp->start[0]= zaddgap(0);
	
	i= 2;
	while (i <= tp->nlines && tp->start[i] < first)
		++i;
	i -= 2;
	chfirst= tp->nlines;
	chlast= i;
	lasteol= -1;
	
	/* TO DO: scroll up/down if inserting/deleting lines */
	
	for (;; ++i) {
		bufpos end= teendofline(tp, tp->start[i]);
		bool unchanged= (i < tp->nlines && end == tp->start[i+1]);
		if (!unchanged)
			shift += tesetstart(tp, i+1, end, last);
		if (!(unchanged && end < first)) {
			if (i < chfirst)
				chfirst= i;
			chlast= i+1;
		}
		if (end >= tp->buflen) {
			if (end > tp->start[i] && zcharbefore(end) == EOL)
				continue;
			else
				break;
		}
		if (unchanged && end > last) {
			i= tp->nlines-1;
			break;
		}
	}
	
	zassert(tp->nlines > i);
	
	if (tp->drawing) {
		if (shift != 0) {
			lineno k= chlast;
			if (shift > 0)
				k -= shift;
			tescroll(tp,
				tp->left, tp->top + k*tp->vspace,
				tp->right, tp->top + tp->nlines*tp->vspace,
				0, shift*tp->vspace);
		}
		
		techange(tp,
			tp->left, tp->top + chfirst*tp->vspace,
			tp->right, tp->top + chlast*tp->vspace);
	}
	
	tp->nlines= i+1;
	newbottom= tp->top + tp->vspace*tp->nlines;
	if (newbottom < tp->bottom)
		techange(tp,
			tp->left, newbottom, tp->right, tp->bottom);
	tp->bottom= newbottom;
	tp->aim= UNDEF;
	tp->focprev= FALSE;
	if (tp->drawing)
		tesetcaret(tp);
	
	zcheck();
}

static int
tesetstart(tp, i, pos, last)
	register TEXTEDIT *tp;
	register lineno i;
	bufpos pos, last;
{
	if (i > tp->nlines) {
		tp->nlines= i;
		if (tp->nlines >= tp->nstart) {
			tp->nstart= tp->nlines + STARTINCR;
			tp->start= (bufpos*)zrealloc((char*)tp->start,
				tp->nstart*sizeof(int));
		}
		tp->start[i]= pos;
		return 0;
	}
	else {
		lineno shift= 0;
		lineno k;
		for (k= i; k < tp->nlines; ++k) {
			if (tp->start[k] > pos)
				break;
		}
		shift= k-1 - i;
		/* start[k] should really be start[i+1] */
		if (shift < 0 && tp->start[k] >= last) { /* Insert one */
			++tp->nlines;
			if (tp->nlines >= tp->nstart) {
				tp->nstart= tp->nlines + STARTINCR;
				tp->start= (int*)zrealloc((char*)tp->start,
					tp->nstart*sizeof(int));
			}
			for (k= tp->nlines; k > i; --k)
				tp->start[k]= tp->start[k-1];
		}
		else if (shift > 0 && pos >= last) { /* Delete some */
			for (; k <= tp->nlines; ++k)
				tp->start[k-shift]= tp->start[k];
			tp->nlines -= shift;
			if (tp->nlines < tp->nstart - STARTINCR) {
				tp->nstart= tp->nlines+1;
				tp->start= (int*)zrealloc((char*)tp->start,
					tp->nstart*sizeof(int));
			}
		}
		else
			shift= 0; /* Don't shift (yet) */
		tp->start[i]= pos;
		return -shift;
	}
}

static int
teendofline(tp, pos)
	TEXTEDIT *tp;
	bufpos pos;
{
	bufpos end= tp->buflen;
	bufpos k;
	
	/* Find first EOL if any */
	if (lasteol >= pos)
		k= lasteol;
	else {
		for (k= pos; k < end && zcharat(k) != EOL; zincr(&k))
			;
		lasteol= k;
	}
	
	end= tetextbreak(tp, pos, k, tp->width);
	
	/* Extend with any spaces immediately following end */
	for (; end < tp->buflen && zcharat(end) == ' '; zincr(&end))
		;
	
	if (end < tp->buflen) {
		/* Extend with immediately following EOL */
		if (zcharat(end) == EOL)
			zincr(&end);
		else {
			/* Search back for space before last word */
			for (k= end; zdecr(&k) >= pos && !isspace(zcharat(k)); )
				;
			
			if (k >= pos)
				end= znext(k);
		}
	}

	/* Each line must be at least one character long,
	   otherwise a very narrow text-edit box would cause
	   the size calculation to last forever */
	if (end == pos && end < tp->buflen)
		zincr(&end);
	
	return end;
}

bool
teevent(tp, e)
	TEXTEDIT *tp;
	EVENT *e;
{
	if (e->window != tp->win)
		return FALSE;
	
	switch (e->type) {
	
	case WE_CHAR:
		teinschar(tp, e->u.character);
		break;
	
	case WE_COMMAND:
		switch (e->u.command) {
		case WC_BACKSPACE:
			tebackspace(tp);
			break;
		case WC_RETURN:
			teinschar(tp, EOL);
			break;
		case WC_TAB:
			teinschar(tp, '\t');
			break;
		case WC_LEFT:
		case WC_RIGHT:
		case WC_UP:
		case WC_DOWN:
			tearrow(tp, e->u.command);
			break;
		default:
			return FALSE;
		}
		break;
	
	case WE_MOUSE_DOWN:
		{
			int h= e->u.where.h, v= e->u.where.v;
			if (tp->viewing) {
				if (h < tp->vleft || h > tp->vright ||
				    v < tp->vtop || v > tp->vbottom)
					return FALSE;
			}
			else {
				if (h < tp->left || h > tp->right ||
				    v < tp->top || v > tp->bottom)
					return FALSE;
			}
			teclicknew(tp, h, v,
				   e->u.where.button == 3 ||
				   (e->u.where.mask & WM_SHIFT),
				   e->u.where.clicks > 1);
		}
		break;
	
	case WE_MOUSE_MOVE:
	case WE_MOUSE_UP:
		if (!tp->mdown)
			return FALSE;
		teclicknew(tp, e->u.where.h, e->u.where.v, TRUE, tp->dclick);
		if (e->type == WE_MOUSE_UP)
			tp->mdown= FALSE;
		break;
	
	case WE_DRAW:
		wbegindrawing(tp->win);
		tedrawnew(tp, e->u.area.left, e->u.area.top,
				e->u.area.right, e->u.area.bottom);
		wenddrawing(tp->win);
		break;
	
	default:
		return FALSE;
	
	}
	
	/* If broke out of switch: */
	return TRUE;
}

void
tearrow(tp, code)
	TEXTEDIT *tp;
	int code;
{
	lineno i;
	bufpos pos;
	
	tehidefocus(tp);
	
	switch (code) {
	
	case WC_LEFT:
		if (tp->foclen != 0)
			tp->foclen= 0;
		else {
			if (tp->foc > 0)
				--tp->foc;
			else
				wfleep();
		}
		tp->aim= UNDEF;
		tp->focprev= FALSE;
		break;
	
	case WC_RIGHT:
		if (tp->foclen != 0) {
			tp->foc += tp->foclen;
			tp->foclen= 0;
		}
		else {
			if (tp->foc < tp->buflen-tp->gaplen)
				++tp->foc;
			else
				wfleep();
		}
		tp->aim= UNDEF;
		tp->focprev= FALSE;
		break;
	
	/* TO DO: merge the following two cases */
	
	case WC_UP:
		if (tp->foclen > 0)
			tp->foclen= 0;
		else {
			pos= zaddgap(tp->foc);
			i= tewhichline(tp, pos, (bool) tp->focprev);
			if (i <= 0)
				wfleep();
			else {
				if (tp->aim == UNDEF)
					tp->aim= tp->left + tetextwidth(tp,
						tp->start[i], pos);
				--i;
				pos= tetextround(tp, i, tp->aim);
				tp->foc= zsubgap(pos);
				tp->focprev= (pos == tp->start[i+1]);
			}
		}
		break;
	
	case WC_DOWN:
		if (tp->foclen > 0) {
			tp->foc += tp->foclen;
			tp->foclen= 0;
		}
		else {
			pos= zaddgap(tp->foc);
			i= tewhichline(tp, pos, (bool) tp->focprev);
			if (i+1 >= tp->nlines)
				wfleep();
			else {
				if (tp->aim == UNDEF)
					tp->aim= tp->left + tetextwidth(tp,
						tp->start[i], pos);
				++i;
				pos= tetextround(tp, i, tp->aim);
				tp->foc= zsubgap(pos);
				tp->focprev= (pos == tp->start[i+1]);
			}
		}
		break;
	
	default:
		dprintf("tearrow: bad code %d", code);
		break;
		
	}
	tesetcaret(tp);
}

void
tebackspace(tp)
	TEXTEDIT *tp;
{
	if (tp->foclen == 0) {
		if (tp->foc == 0) {
			wfleep();
			return;
		}
		--tp->foc;
		tp->foclen= 1;
	}
	teinsert(tp, "", 0);
}

bool
teclicknew(tp, h, v, extend, dclick)
	TEXTEDIT *tp;
	coord h, v;
	bool extend, dclick;
{
	lineno i;
	bufpos pos;
	focpos f;
	
	tp->dclick= dclick;
	pos= tewhereis(tp, h, v, &i);
	f= zsubgap(pos);
	if (extend) {
		if (!tp->mdown) {
			tp->mdown= TRUE;
			if (f - tp->foc < tp->foc + tp->foclen - f)
				tp->anchor= tp->foc + tp->foclen;
			else
				tp->anchor= tp->foc;
			tp->anchor2= tp->anchor;
		}
		if (f >= tp->anchor) {
			if (dclick)
				f= tewordend(tp, f);
			techangefocus(tp, tp->anchor, f);
		}
		else {
			if (dclick)
				f= tewordbegin(tp, f);
			techangefocus(tp, f, tp->anchor2);
		}
	}
	else {
		tp->mdown= TRUE;
		tp->anchor= tp->anchor2= f;
		if (dclick) {
			tp->anchor= tewordbegin(tp, tp->anchor);
			tp->anchor2= tewordend(tp, f);
		}
		techangefocus(tp, tp->anchor, tp->anchor2);
	}
	tp->aim= UNDEF;
	tp->focprev= (tp->foclen == 0 && pos == tp->start[i+1]);
	tesetcaret(tp);
	return TRUE;
}

/* Return f, 'rounded down' to a word begin */

static int
tewordbegin(tp, f)
	TEXTEDIT *tp;
	int f;
{
	f= zaddgap(f);
	for (;;) {
		if (f == 0 || isspace(zcharbefore(f)))
			break;
		zdecr(&f);
	}
	return zsubgap(f);
}

/* Ditto to word end */

static int
tewordend(tp, f)
	TEXTEDIT *tp;
	int f;
{
	f= zaddgap(f);
	for (;;) {
		if (f >= tp->buflen || isspace(zcharat(f)))
			break;
		zincr(&f);
	}
	return zsubgap(f);
}

int
tegetleft(tp)
	TEXTEDIT *tp;
{
	return tp->left;
}

int
tegettop(tp)
	TEXTEDIT *tp;
{
	return tp->top;
}

int
tegetright(tp)
	TEXTEDIT *tp;
{
	return tp->right;
}

int
tegetbottom(tp)
	TEXTEDIT *tp;
{
	return tp->bottom;
}

void
temove(tp, left, top, width)
	TEXTEDIT *tp;
	coord left, top, width;
{
	temovenew(tp, left, top, left+width, top + tp->nlines*tp->vspace);
}

/*ARGSUSED*/
void
temovenew(tp, left, top, right, bottom)
	TEXTEDIT *tp;
	int left, top, right, bottom;
{
	int oldheight= tp->bottom - tp->top;
	tp->left= left;
	tp->top= top;
	tp->right= right;
	tp->bottom= tp->top + oldheight;
	if (right - left != tp->width) {
		tp->width= right - left;
		tp->nlines= 0; /* Experimental! */
		terecompute(tp, 0, tp->buflen);
	}
}

void
tesetview(tp, left, top, right, bottom)
	TEXTEDIT *tp;
	int left, top, right, bottom;
{
	tp->viewing= TRUE;
	tp->vleft= left;
	tp->vtop= top;
	tp->vright= right;
	tp->vbottom= bottom;
}

void
tenoview(tp)
	TEXTEDIT *tp;
{
	tp->viewing= FALSE;
}

void
tesetfocus(tp, foc1, foc2)
	TEXTEDIT *tp;
	focpos foc1, foc2;
{
	if (foc1 > tp->buflen - tp->gaplen)
		foc1= tp->buflen - tp->gaplen;
	if (foc2 > tp->buflen - tp->gaplen)
		foc2= tp->buflen - tp->gaplen;
	if (foc1 < 0)
		foc1= 0;
	if (foc2 < foc1)
		foc2= foc1;
	techangefocus(tp, foc1, foc2);
	tp->aim= UNDEF;
	tp->focprev= FALSE;
	tesetcaret(tp);
}

int
tegetfoc1(tp)
	TEXTEDIT *tp;
{
	return tp->foc;
}

int
tegetfoc2(tp)
	TEXTEDIT *tp;
{
	return tp->foc + tp->foclen;
}

int
tegetnlines(tp)
	TEXTEDIT *tp;
{
	return tp->nlines;
}

char *
tegettext(tp)
	TEXTEDIT *tp;
{
	temovegapto(tp, tp->buflen - tp->gaplen);
	if (tp->gaplen < 1)
		tegrowgapby(tp, 1+RESERVE);
	tp->buf[tp->gap]= EOS;
	return tp->buf;
}

int
tegetlen(tp)
	TEXTEDIT *tp;
{
	return tp->buflen - tp->gaplen;
}

void
tesetbuf(tp, buf, buflen)
	TEXTEDIT *tp;
	char *buf;
	int buflen;
{
	bool drawing= tp->drawing;
	
	if (buf == NULL || buflen < 0)
		return;
	if (drawing)
		techange(tp, tp->left, tp->top, tp->right, tp->bottom);
	free(tp->buf);
	tp->buf= buf;
	tp->buflen= buflen;
	tp->foc= tp->foclen= tp->gap= tp->gaplen= 0;
	tp->drawing= FALSE;
	terecompute(tp, 0, tp->buflen);
	if (drawing) {
		tp->drawing= TRUE;
		techange(tp, tp->left, tp->top, tp->right, tp->bottom);
	}
}


/* The following paragraph-drawing routines are experimental.
   They cannibalize on the existing text-edit code,
   which makes it trivial to ensure the lay-out is the same,
   but causes overhead to initialize a text-edit struct.
   The flag 'drawing' has been added to the textedit struct,
   which suppresses calls to tesetcaret, wchange and wscroll
   from tesetup and terecompute.
   It doesn't suppress actual drawing, which only occurs when
   tedraw is called.
   Note -- this could be optimized, but it is infrequently,
   so I don't care until I get complaints. */

/* Draw a paragraph of text, exactly like tedraw would draw it.
   Parameters are the top left corner, the width, the text and its length.
   Return value is the v coordinate of the bottom line.
   An empty string is drawn as one blank line. */

int
wdrawpar(left, top, text, width)
	int left, top;
	char *text;
	int width;
{
	return _wdrawpar(left, top, text, width, TRUE);
}

/* Measure the height of a paragraph of text, when drawn with wdrawpar. */

int
wparheight(text, width)
	char *text;
	int width;
{
	return _wdrawpar(0, 0, text, width, FALSE);
}

/* Routine to do the dirty work for the above two.
   Size calculations are implemented by going through the normal
   routine but suppressing the actual drawing. */

static int
_wdrawpar(left, top, text, width, draw)
	int left, top;
	char *text;
	int width;
	bool draw;
{
	TEXTEDIT *tp= tesetup((WINDOW*)NULL, left, top, left+width, top, FALSE);
	int v;
	
	tesetbuf(tp, text, strlen(text));
	if (draw)
		tedraw(tp);
	v= tegetbottom(tp);
	tp->buf= NULL;
	tefree(tp);
	return v;
}
