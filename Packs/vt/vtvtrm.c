/* VTRM implementation using VT as lower layer.
   Unfortunately, this isn't too useful on the input part:
   you'll really want to use wgetevent directly to get access
   to mouse clicks, menu events etc.
   Another problem is that there is no portable way to direct normal
   output (e.g., from printf and putchar) to the VTRM window.
   One other thing: you'll have to call wdone() when you really exit. */

/* XXX This has never been tested... */
/* XXX The interfaces to trmputdata and trmsense should change !!! */

#include "vtimpl.h"
#include "vtrm.h"

static VT *vt;
static bool active;

int
trmstart(rows_return, cols_return, flags_return)
	int *rows_return, *cols_return, *flags_return;
{
	if (active)
		return TE_TWICE; /* Already started */
	if (vt == NULL) {
		winit();
		wsetdefwinsize(24*wcharwidth('0'), 80*wlineheight());
		vt= vtopen("VTRM", 24, 80, 0);
		if (vt == NULL)
			return TE_NOMEM; /* Failure */
	}
	*rows_return= 24;
	*cols_return= 80;
	*flags_return=
		HAS_STANDOUT | CAN_SCROLL;
	return TE_OK;
}

trmend()
{
	active= FALSE;
}

/* XXX Need interface change */
trmputdata(row, col, data)
	int row, col;
	char *data;
{
	char *start= data;
	int mask= 0;
	vtsetcursor(vt, row, col);
	do {
		if ((*data & 0x80) != mask) {
			if (data > start) {
				if (mask) {
					char *p;
					for (p= start; p < data; ++p)
						*p &= 0x7f;
					vt->gflags= VT_INVERSE;
				}
				vtputstring(vt, start, data-start);
				if (mask) {
					char *p;
					for (p= start; p < data; ++p)
						*p |= 0x80;
					vt->gflags= 0;
				}
				start= data;
			}
			mask= *data & 0x80;
		}
	} while (*data++ != EOS);
	/* XXX This is likely to omit the final data? */
	vteolclear(vt, vt->cur_row, vt->cur_col);
}

trmscrollup(r1, r2, n)
	int r1, r2;
	int n;
{
	if (n > 0)
		vtscrollup(vt, r1, r2+1, n);
	else if (n < 0)
		vtscrolldown(vt, r1, r2+1, -n);
}

trmsync(row, col)
	int row, col;
{
	vtsetcursor(vt, row, col);
}

static lasth, lastv;

int
trminput()
{
	EVENT e;
	for (;;) {
		switch (e.type) {
		case WE_COMMAND:
			switch (e.type) {
			case WC_CANCEL:
				return '\003';
			case WC_RETURN:
				return '\r';
			case WC_TAB:
				return '\t';
			case WC_BACKSPACE:
				return '\b';
			case WC_LEFT:
				return '\034';
			case WC_RIGHT:
				return '\035';
			case WC_UP:
				return '\036';
			case WC_DOWN:
				return '\037';
			}
			break;
		case WE_CHAR:
			return e.u.character;
			break;
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			lasth= e.u.where.h;
			lastv= e.u.where.v;
			if (e.type == WE_MOUSE_UP)
				return '\007';
			break;
		}
	}
}

/* XXX Need interface change */
int
trmsense(row_return, col_return)
	int *row_return, *col_return;
{
	*row_return= lastv / vtcheight(vt);
	*col_return= lasth / vtcwidth(vt);
}
