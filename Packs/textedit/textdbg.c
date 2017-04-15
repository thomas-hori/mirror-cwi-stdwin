/* Text Edit, debugging code */

#include "text.h"

#ifndef macintosh

/*VARARGS1*/
dprintf(fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
	char *fmt;
{
	printf("\r\n");
	printf(fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	printf("\r\n");
}

#endif

#ifndef NDEBUG

/* Check the world's consistency */

techeck(tp, line)
	TEXTEDIT *tp;
	int line;
{
	lineno i;

#define zck(n) ((n) || dprintf("zck(n) line %d", line))
	
	zck(tp->nlines >= 1);
	
	zck(tp->start[0] == zaddgap(0));
	zck(tp->start[tp->nlines] == tp->buflen);
	
	zck(0 <= tp->gap);
	zck(0 <= tp->gaplen);
	zck(zgapend <= tp->buflen);
	
	zck(0 <= tp->foc);
	zck(0 <= tp->foclen);
	zck(zfocend <= tp->buflen-tp->gaplen);
	
	for (i= 0; i < tp->nlines; ++i) {
		if (i < tp->nlines-1)
			{ zck(tp->start[i] < tp->start[i+1]); }
		else
			{ zck(tp->start[i] <= tp->start[tp->nlines]); }
		zck(tp->start[i] < tp->gap || zgapend <= tp->start[i]);
	}

#undef zck

}

#if 0

/* Dump the world's state to the screen (call from drawproc) */

zdebug(left, top, right, bottom)
{
	int h, v;
	int i, j;
	
	h= 0, v= 15*wlh; if (v >= bottom) return;
	zprintf(h, v, "buflen=%d nlines=%d foc=%d foclen=%d gap=%d gaplen=%d.",
		buflen, nlines, foc, foclen, gap, gaplen);
	h= 0, v += wlh; if (v >= bottom) return;
	for (i= 0; i <= nlines; ++i) {
		h= zprintf(h, v, "%d:%d ", i, start[i]);
	}
	h= 0, v += wlh; if (v >= bottom) return;
	for (i= 0; i <= buflen; ++i) {
		h= zprintf(h, v, "%c",
			i == zaddgap(foc) ?
				(foclen == 0 ? '|' : '[') :
				(i == zaddgap(focend) ? ']' : ' '));
		if (i >= buflen)
			break;
		if (i >= gap && i < gapend)
			h= zprintf(h, v, "**");
		else
			h= zprintf(h, v, "%02x", buf[i] & 0xff);
	}
	h= 0, v += wlh; if (v >= bottom) return;
	for (i= 0; i <= buflen; ++i) {
		for (j= 0; j <= nlines; ++j)
			if (i == start[j])
				break;
		if (j <= nlines)
			h= zprintf(h, v, "%-3d", j);
		else
			h= zprintf(h, v, "   ");
	}
}

/* Printf into the window (could be of general use).
   NB: doesn't recognize \n */

static
zprintf(h, v, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
	int h, v;
	char *fmt;
{
	char buf[256];
	sprintf(buf, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	return wdrawtext(h, v, buf, -1);
}

#endif

#endif /* !NDEBUG */
