/* Virtual Terminal -- basic output routines */

#include "vtimpl.h"

#ifdef macintosh
/* Bold mode uses the "Bold" font, found in VersaTerm;
   this matches Monaco and is available only in 9 pt. */
#define DO_BOLD
#define SETBOLD() wsetfont("Bold")
#define SETNORMAL() wsetfont("Monaco")
#endif

/* Forward */
STATIC void vtdrawproc _ARGS((WINDOW *win,
				int left, int top, int right, int bottom));
STATIC void vtdraw _ARGS((VT *vt, int r1, int c1, int r2, int c2));
STATIC void vtchrange _ARGS((VT *vt, int r1, int c1, int r2, int c2));
STATIC bool slowcirculate _ARGS((VT *vt, int r1, int r2, int n));

/* Administration for vtfind */

static VT **vtlist;	/* Registered VT structs */
static int nvt;		/* Number of registered VT structs */

/* Open a VT window of given size */

VT *
vtopen(title, rows, cols, save)
	char *title;
	int rows, cols;
	int save; /* Number of rows scroll-back capacity */
{
	int row;
	VT *vt = ALLOC(VT);

	if (vt == NULL)
		return NULL;
	rows += save; /* Documentsize: cols * (rows + scrollback) */
	vt->rows = rows;
	vt->cols = cols;
	vt->topterm = save;
	vt->cwidth = wcharwidth('0');
	vt->cheight = wlineheight();

	vt->llen = NALLOC(short, rows);
	vt->data = NALLOC(char *, rows);
	vt->flags = NALLOC(unsigned char *, rows);
	for (row = 0; row < rows; ++row) {
		if (vt->data != NULL)
			vt->data[row] = NALLOC(char, cols);
		if (vt->flags != NULL)
			vt->flags[row] = NALLOC(unsigned char, cols);
	}
	/* Assume that if one NALLOC fails, all following ones
	   for the same size also fail... */
	if (vt->llen == NULL ||
		vt->data == NULL ||
		vt->data[rows-1] == NULL ||
		vt->flags == NULL ||
		vt->flags[rows-1] == NULL) {
		vt->win = NULL;
		vtclose(vt);
		return NULL;
	}

	vt->win = wopen(title, vtdrawproc);
	if (vt->win == NULL) {
		vtclose(vt);
		return NULL;
	}
	
	wsetdocsize(vt->win, cols * vt->cwidth, rows * vt->cheight);
	wsetwincursor(vt->win, wfetchcursor("ibeam"));

	vt->nlcr = FALSE;
	vt->drawing = FALSE;
	vtreset(vt); /* Clear additional fields */
	L_APPEND(nvt, vtlist, VT *, vt);

	return vt;
}

/* Close a VT window.
   Also used to clean-up when vtopen failed half-way */

void
vtclose(vt)
	VT *vt;
{
	int i, row;

	for (i = 0; i < nvt; ++i) {
		if (vt == vtlist[i]) {
			L_REMOVE(nvt, vtlist, VT *, i);
			break;
		}
	}

	if (vt->win != NULL)
		wclose(vt->win);
	for (row = 0; row < vt->rows; ++row) {
		if (vt->data != NULL)
			FREE(vt->data[row]);
		if (vt->flags != NULL)
			FREE(vt->flags[row]);
	}
	FREE(vt->data);
	FREE(vt->flags);
	FREE(vt);
}

/* Output a string to a VT window.
   This does not do escape sequence parsing or control character
   interpretation, but honors the 'modes' the VT can be in
   (insert, inverse/underline etc.) and the scrolling region.
   It updates the cursor position and scrolls if necessary.
   Note that after one or more calls to vtputstring it is necessary
   to call vtsync(vt)! */

void
vtputstring(vt, text, len)
	VT *vt;
	char *text;
	int len;
{
	int row = vt->cur_row;
	int col = vt->cur_col;
	short *llen = vt->llen;
	int scr_top = (vt->scr_top == vt->topterm) ? 0 : vt->scr_top;
	int scr_bot = (vt->cur_row >= vt->scr_bot) ? vt->rows : vt->scr_bot;
	int wrap = 0; /* Number of times wrapped around */
	int last_drawn_col;

#ifndef NDEBUG
	if (scr_top < 0 || scr_top > vt->rows) {
		fprintf(stderr, "vtputstring: scr_top = %d\n", scr_top);
		vtpanic("vtputstring bad scr_top");
	}

	if (scr_bot < 0 || scr_bot > vt->rows) {
		fprintf(stderr, "vtputstring: scr_bot = %d\n", scr_bot);
		vtpanic("vtputstring bad scr_bot");
	}
#endif

	/* Compute text length if necessary */
	if (len < 0)
		len = strlen(text);

	/* This is a 'do' loop so that a call with an empty
	   string will still normalize the cursor position */

	/* XXX This loop is 100 lines long!  Sorry. */
	
	do {
		int llen_row, n, oldcol;

		/* Normalize the cursor position.
		   When we get past the bottom edge we must scroll,
		   but the actual scrolling is delayed until later:
		   here we just wrap around and remember how many
		   times we've wrapped.
		   Thus, scrolling multiple lines is effected as
		   a 'jump' scroll up -- not so nice-looking,
		   but essential with current performance limitations
		   of bitblit hardware.
		   When faster machines become available we may need
		   an option to turn this optimization off in favour
		   of smooth scrolling. */

		if (col >= vt->cols) {
			col = 0;
			++row;
			if (wrap > 0 && row < scr_bot)
				llen[row] = 0;	/* Clear line	*/
		}
		if (row >= scr_bot) {
			/* Should be able to turn this off? */
			if (wrap == 0) last_drawn_col = col;
			++wrap;
			row = scr_top;
			llen[row] = 0;
		}
		oldcol = col; /* For vtdraw call below */

		/* If the cursor is beyond the current line length,
		   eg because of cursor positioning, pad with space.
		   Set llen_row to the new line length. */
		llen_row = llen[row];
		if (llen_row < col) {
			do {
				vt->data[row][llen_row] = ' ';
				vt->flags[row][llen_row] = 0;
			} while (++llen_row < col);
		}

		/* Set n to the number of characters that can be
		   inserted in the current line */
		n = vt->cols - col;
		CLIPMAX(n, len);

		/* When inserting, shift the rest of the line right.
		   The last characters may fall of the edge. */
		if (vt->insert && llen_row > col && col+n < vt->cols) {
			int k;
			llen_row += n;
			CLIPMAX(llen_row, vt->cols);
			vtscroll(vt, row, col, row+1, llen_row, 0, n);
			for (k = llen_row - n; --k >= col; ) {
				vt->data[row][k+n] = vt->data[row][k];
				vt->flags[row][k+n] = vt->flags[row][k];
			}
		}

		/* Copy the characters into the line data */
#ifndef NDEBUG
		if (col + n > vt->cols || row >= vt->rows) {
			fprintf(stderr, "col=%d, n=%d, row=%d\n", col,n,row);
			vtpanic("Bad index in vtputstring");
		}
#endif
		strncpy(vt->data[row] + col, text, n);

		/* Update loop administration, maintaining the invariant:
		   'len' characters starting at 'text' still to do */
		len -= n;
		text += n;

		/* Set the corresponding flag bits.
		   The current column is set as a side effect. */
		while (--n >= 0)
			vt->flags[row][col++] = vt->gflags;

		/* Update line length */
		CLIPMIN(llen_row, col);
		llen[row] = llen_row;

		/* Maybe draw the characters now */
		if (vt->lazy) {
			D( printf("vtputstring: ") );
			vtchrange(vt, row, oldcol, row, col);
		}
		else if (wrap == 0) {
			VTBEGINDRAWING(vt);
			werase(oldcol*vt->cwidth, row*vt->cheight,
			    col*vt->cwidth, (row+1)*vt->cheight);

			D( printf("vtputstring: ") );
			last_drawn_col = col;
			vtdraw(vt, row, oldcol, row + 1, col);
		}

		/* Loop while more work to do */

	} while (len > 0);

#if 0
	/* XXX Why not? */
	vtsetcursor(vt, row, col);
#endif

	/* Process delayed scrolling. */

	if (wrap > 0) {
		/* Yes, we need to scroll.
		   When wrap > 1, we have scrolled more than a screenful;
		   then the vtscroll call is skipped, but we must still
		   circulate the lines internally. */

		/* Picture:

		   scr_top ___________________ (first line affected)
		      .    ___________________
		      .    ______ row ________
		      .    ___________________ (last line affected)
		   scr_bot

		   Data move: circulate down so that the data at row
		   moves to scr_bot - 1.
		   Screen bits move: the line below row must
		   become scr_top.
		   We must also invalidate the characters changed
		   before we started scrolling, but we must do this
		   after the vtscroll call, because some STDWIN
		   versions don't properly scroll invalidated bits.
		*/

		if (row + 1 != scr_bot) vtcirculate(vt,
					scr_top, scr_bot,
					scr_bot - (row + 1));
		if (wrap == 1) {
			int n = (row + 1) - scr_top;
			D( printf("Wrapped once; n=%d\n", n) );
			vtscroll(vt, scr_top, 0, scr_bot, vt->cols, -n, 0);
			if (!vt->lazy)
				vtdraw(vt, scr_bot - n, last_drawn_col,
					scr_bot, vt->cols);
		}
		else { /* Scrolled more than the scrolling region */
			D( printf("Whole scrolling region: ") );
			if (!vt->lazy)
				vtdraw(vt, scr_top, 0, scr_bot, vt->cols);
		}
		row = scr_bot - 1;
	}

	/* Set the new cursor position */
	vtsetcursor(vt, row, col);
}

/* Subroutine to invalidate the text range from (r1, c1) to (r2, c2).
   This is not the same as vtchange; this function deals with text
   ranges, while vtchange deals with rectangles. */

STATIC void
vtchrange(vt, r1, c1, r2, c2)
	VT *vt;
{
	D( printf("vtchrange [%d,%d]~[%d,%d]\n", c1, r1, c2, r2) );
	if (c1 >= vt->cols) {
		MON_EVENT("vtchrange: c1 >= vt->cols");
		++r1; c1 = 0;
	}
	if (r1 >= r2) {
		vtchange(vt, r1, c1, r2+1, c2);
	}
	else {
		vtchange(vt, r1, c1, r1+1, vt->cols);
		vtchange(vt, r1+1, 0, r2, vt->cols);
		vtchange(vt, r2, 0, r2+1, c2);
	}
}

/* Set cursor position.
   This sets the STDWIN text caret and calls wshow for the character
   at the cursor.
   The cursor position is clipped to the screen dimensions,
   but it may sit on the right edge just beyond the last character. */

void
vtsetcursor(vt, row, col)
	VT *vt;
	int row, col;
{
	CLIPMAX(row, vt->rows - 1);
	CLIPMIN(row, 0);
	CLIPMAX(col, vt->cols);
	CLIPMIN(col, 0);
	vt->cur_row = row;
	vt->cur_col = col;
	CLIPMAX(col, vt->cols - 1);
	if (!vt->lazy) {
		VTENDDRAWING(vt);
		wsetcaret(vt->win, col * vt->cwidth, row * vt->cheight);
		vtshow(vt, row, col, row + 1, col);
	}
}

/* Set scrolling region.  Lines in [top...bot) can scroll.
   If the parameters are valid, set the region and move to (0, 0);
   if there is an error, reset the region and don't move.
   (NB: the move is to (0, 0), not to the top of the region!) */

void
vtsetscroll(vt, top, bot)
	VT *vt;
	int top, bot;
{
	vtsync(vt);
	if (top >= vt->topterm && top < bot && bot <= vt->rows) {
		vt->scr_top = top;
		vt->scr_bot = bot;
		vtsetcursor(vt, vt->topterm, 0);
		/* vtshow(vt, vt->topterm, 0, vt->rows, vt->cols); */
	}
	else {
		vt->scr_top = vt->topterm;
		vt->scr_bot = vt->rows;
	}
	vtshow(vt, vt->scr_top, 0, vt->scr_bot, vt->cols);
}

/* Major reset */

void
vtreset(vt)
	VT *vt;
{
	int row;

	vtchange(vt, 0, 0, vt->rows, vt->cols);
	vtshow(vt, vt->topterm, 0, vt->rows, vt->cols);

	for (row = 0; row < vt->rows; ++row)
		vt->llen[row] = 0;

	vt->toscroll = 0;
	vtsetflags(vt, 0);
	vtsetinsert(vt, FALSE);
	vtsetscroll(vt, 0, 0);
	vtsetcursor(vt, vt->topterm, 0);
	vt->sel_col1 = vt->sel_row1 = 0;
	vt->sel_col2 = vt->sel_row2 = 0;
	vt->save_row = vt->save_col = 0;
	vt->keypadmode = FALSE;
	vt->lazy = FALSE;
	vt->mitmouse = FALSE;
	vt->visualbell = FALSE;
	vt->flagschanged = TRUE;
	vt->action = NULL; /* This invalidates all other parsing fields */
}

/* Draw procedure - this one is called because stdwin discovered an expose */

STATIC void
vtdrawproc(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	VT *vt = vtfind(win);

#ifndef NDEBUG
	if (vt == NULL) vtpanic("vtdrawproc not for VT window");
	if (vt->drawing) vtpanic("vtdrawproc while drawing");
#endif

	vt->drawing = 1;	/* Stdwin did this implicitely */
	{
		int cw = vt->cwidth;
		int ch = vt->cheight;
		int col1 = left / cw;
		int col2 = (right + cw - 1) / cw;
		int row1 = top / ch;
		int row2 = (bottom + ch - 1) / vt->cheight;

		D( printf("vtdrawproc: ") );
		vtdraw(vt, row1, col1, row2, col2);
	}
	vt->drawing = 0;	/* Stdwin will do this implicitely */
}

STATIC void
set_textstyle(vt, flags)
    VT *vt;
    int flags;
{
#if 0
    /* This isn't right, for various reasons.  And do we need it? */
    static int previous_flags = -3;	/* Or anything < 0	*/

    if (flags == previous_flags) return;
    D( printf("Set_textstyle: 0x%x => 0x%x\n", previous_flags, flags) );
    previous_flags = flags;
#endif
    wsetplain();
    if (flags & VT_UNDERLINE) wsetunderline();
    if (flags & VT_INVERSE) wsetinverse();
#ifdef DO_BOLD
    if (flags & VT_BOLD) SETBOLD();
    else SETNORMAL();
#endif
}

/* Draw procedure - doesn't draw [row2, col2], or any other
   char at row2, takes care of underlining, inverse video etc */

STATIC void
vtdraw(vt, row1, col1, row2, col2)
    VT *vt;
    int row1, col1, row2, col2;
{
    int cw = vt->cwidth;
    int ch = vt->cheight;
    register unsigned char cur_flags;
    int row;

    VTBEGINDRAWING(vt);
    D( printf("vtdraw [%d,%d]~[%d,%d]\n", col1, row1, col2, row2) );

    CLIPMIN(col1, 0);
    CLIPMAX(col2, vt->cols);
    CLIPMIN(row1, 0);
    CLIPMAX(row2, vt->rows);

    for (row = row1; row < row2; ++row) {
	register int col;
	char *data_row = vt->data[row] + col1;
	register unsigned char *flags_row = vt->flags[row] + col1;
	int h = col1*cw;
	int v = row*ch;
	int first = col1;
	register int last = vt->llen[row];

	CLIPMAX(last, col2);	/* Don't draw more than asked for */

	/* Set flags */
	cur_flags = flags_row[0];
	set_textstyle(vt, cur_flags);

	/* Attempt to draw as much text as possible in one wdrawtext */
	for (col = first; col < last; ++col) {
	    if (*flags_row++ != cur_flags) {
		int n = col-first;
		/* n cannot be < 0; ==0 means this
		   line has different flags */
		if (n > 0) {
		    wdrawtext(h, v, data_row, n);
		    first = col;
		    data_row += n;
		    h += n*cw;
		}

		cur_flags = flags_row[-1];	/* Set new flags */
		set_textstyle(vt, cur_flags);
	    }
	}
	/* Draw leftover text on this line and perhaps some black spaces: */
	if (col > first) wdrawtext(h, v, data_row, col-first);
    }
}

/* Find the VT corresponding to a WINDOW */

VT *
vtfind(win)
	WINDOW *win;
{
	int i;
	for (i = 0; i < nvt; ++i) {
		if (vtlist[i]->win == win)
			return vtlist[i];
	}
	return NULL;
}

/* Subroutine to circulate lines.
   For i in r1 ... r2-1, move line i to position i+n (modulo r2-r1).

   For ABS(n)==1, we have a fast solution that always works.
   For larger n, we have a slower solution allocating a temporary buffer;
   if we can't, we repeat the fast solution ABS(n) times (really slow).

   We assume reasonable input:
	0 <= r1 < r2 <= vt->rows,
	0 < abs(n) < r2-r1.
*/

void
vtcirculate(vt, r1, r2, n)
	register VT *vt;
	int r1, r2;
	int n;
{
	if (n == -1) { /* Fast solution, move 1 up */
		char *tdata = vt->data[r1];
		unsigned char *tflags = vt->flags[r1];
		short tllen = vt->llen[r1];
		register int i;
		MON_EVENT("circulate -1");
		for (i = r1+1; i < r2; ++i) {
			vt->data[i-1] = vt->data[i];
			vt->flags[i-1] = vt->flags[i];
			vt->llen[i-1] = vt->llen[i];
		}
		vt->data[i-1] = tdata;
		vt->flags[i-1] = tflags;
		vt->llen[i-1] = tllen;
	}
	else if (n == 1) { /* Fast solution, move 1 down */
		char *tdata = vt->data[r2-1];
		unsigned char *tflags = vt->flags[r2-1];
		short tllen = vt->llen[r2-1];
		register int i;
		MON_EVENT("circulate 1");
		for (i = r2-1; i > r1; --i) {
			vt->data[i] = vt->data[i-1];
			vt->flags[i] = vt->flags[i-1];
			vt->llen[i] = vt->llen[i-1];
		}
		vt->data[i] = tdata;
		vt->flags[i] = tflags;
		vt->llen[i] = tllen;
	}
	else if (n != 0) {
		if (!slowcirculate(vt, r1, r2, n)) {
			/* Couldn't -- do ABS(n) times the fast case... */
			int step;
			if (n < 0) {
				n = -n;
				step = -1;
			}
			else step = 1;
			while (--n >= 0)
				vtcirculate(vt, r1, r2, step);
		}
	}
}

/* Slow version of the above; move lines r1..r2-1 n lines up */

STATIC bool
slowcirculate(vt, r1, r2, n)
	register VT *vt;
	int r1, r2;
	int n; /* May be negative */
{
	char **tdata; /* Data buffer */
	unsigned char **tflags; /* Flags buffer */
	short *tllen; /* Line length buffer */
	bool ok;

	if (n < 0) n += (r2 - r1);
	tdata = NALLOC(char *, n);
	tflags = NALLOC(unsigned char *, n);
	tllen = NALLOC(short, n);

	MON_EVENT("slowcirculate");
	/* Did all the malloc's work? */
	ok = tdata != NULL && tflags != NULL && tllen != NULL;
	if (ok) {
		register int i;
		r2 -= n; /* Now r2 "points" beyond the last target line */
		/* Save data, flags and lengths to be overwritten */
		for (i = 0; i < n; ++i) {
			tdata[i] = vt->data[r2+i];
			tflags[i] = vt->flags[r2+i];
			tllen[i] = vt->llen[r2+i];
		}
		/* Copy "lower" part of the lines to r1..r1+n-1 (=r2-1) */
		for (i = r2; --i >= r1; ) {
			vt->data[i+n] = vt->data[i];
			vt->flags[i+n] = vt->flags[i];
			vt->llen[i+n] = vt->llen[i];
		}
		/* Restore saved lines in r1..r1+n-1 */
		for (i = 0; i < n; ++i) {
			vt->data[r1+i] = tdata[i];
			vt->flags[r1+i] = tflags[i];
			vt->llen[r1+i] = tllen[i];
		}
	}

	FREE(tdata);
	FREE(tflags);
	FREE(tllen);

	return ok;
}

/* VT interface to wchange */

void
vtchange(vt, r1, c1, r2, c2)
	VT *vt;
	int r1, c1, r2, c2;
{
	VTENDDRAWING(vt);
	D( printf("vtchange [%d, %d]~[%d, %d]\n", c1, r1, c2, r2) );
	wchange(vt->win,
		c1 * vt->cwidth, r1 * vt->cheight,
		c2 * vt->cwidth, r2 * vt->cheight);
}

/* VT interface to wshow */

void
vtshow(vt, r1, c1, r2, c2)
	VT *vt;
	int r1, c1, r2, c2;
{
	VTENDDRAWING(vt);
	wshow(vt->win,
		c1 * vt->cwidth, r1 * vt->cheight,
		c2 * vt->cwidth, r2 * vt->cheight);
}

/* VT interface to wscroll.
   In lazy mode, the actual scrolling may be postponed
   (by setting vt->toscroll). */

void
vtscroll(vt, r1, c1, r2, c2, drow, dcol)
	VT *vt;
	int r1, c1, r2, c2;
	int drow, dcol; /* Translation vector */
{
	int scr_top = vt->scr_top;
	if (scr_top == vt->topterm)
		scr_top = 0;

	D( printf("vtscroll %d lines\n", drow) );

	if (vt->lazy && dcol == 0 && r1 == scr_top && r2 == vt->scr_bot &&
		c1 == 0 && c2 == vt->cols) {
		if (drow * vt->toscroll < 0)
			vtsync(vt);
		vt->toscroll += drow;
	}
	else {
		if (vt->toscroll != 0 && r1 < vt->scr_bot && r2 > scr_top)
			vtsync(vt); /* Execute leftover scrolling first */

		/* Convert to STDWIN coordinates */
		c1 *= vt->cwidth;
		c2 *= vt->cwidth;
		dcol *= vt->cwidth;

		r1 *= vt->cheight;
		r2 *= vt->cheight;
		drow *= vt->cheight;

		VTENDDRAWING(vt);
		wnocaret(vt->win);
		wscroll(vt->win, c1, r1, c2, r2, dcol, drow);

		/* Despite what the stdwin document says,
		   wscroll doesn't generate wchanges anymore */
		if (vt->lazy) {
			if (drow < 0) {		/* Scrolled upwards	*/
				wchange(vt->win, c1, r2+drow, c2, r2);
				D( printf("^: wchange(%d, %d, %d, %d)\n",
						c1, r2+drow, c2, r2) );
			}
			else if (drow > 0) {	/* Scrolled downwards	*/
				wchange(vt->win, c1, r1, c2, r1+drow);
				D( printf("V: wchange(%d, %d, %d, %d)\n",
						c1, r1, c2, r1+drow) );
			}
			if (dcol < 0) {		/* Scrolled to the left	*/
				wchange(vt->win, c2+dcol, r1, c2, r2);
				D( printf("<: wchange(%d, %d, %d, %d)\n",
						c2+dcol, r1, c2, r2) );
			}
			else if (dcol > 0) {	/* Scrolled to the right */
				wchange(vt->win, c1, r1, c1+dcol, r2);
				D( printf(">: wchange(%d, %d, %d, %d)\n",
						c1, r1, c1+dcol, r2) );
			}
		}
	}
}

/* Execute delayed scrolling.
   Don't call from within drawproc: wscroll while drawing is BAD. */

void
vtsync(vt)
	VT *vt;
{
	VTENDDRAWING(vt);
	if (vt->toscroll != 0) {
		int scr_top = vt->scr_top;
#if 0
		/* XXX Why not? */
		if (vt->toscroll < 0) vtpanic("vtsync: toscroll < 0");
#endif
		if (scr_top == vt->topterm)
			scr_top = 0;
		D( printf("VtSync [,%d]~[,%d] %d: wscroll\n",
			scr_top, vt->scr_bot, vt->toscroll) );
		wscroll(vt->win,
			0, scr_top * vt->cheight,
			vt->cols * vt->cwidth, vt->scr_bot * vt->cheight,
			0, vt->toscroll * vt->cheight);

		D( printf("vtsync: ") );
		/* I could get speedup from remembering
		 * min and max columns for the call here
		 */
		vtchange(vt, vt->scr_bot + vt->toscroll, 0,
			vt->scr_bot, vt->cols);
		vt->toscroll = 0;
	}
	/* vtsetcursor doesn't do this if vt->lazy: */
	if (vt->lazy) {
		MON_EVENT("vtsync for lazy");
		wsetcaret(vt->win,
			vt->cur_col * vt->cwidth,
			vt->cur_row * vt->cheight);
		vtshow(vt, vt->cur_row,vt->cur_col, vt->cur_row+1,vt->cur_col);
	}
}

/* Internal VT interface to winvert.
   Must be called between wbegindrawing and wenddrawing. */

void
vtinvert(vt, row1, col1, row2, col2)
	VT *vt;
	int row1, col1, row2, col2;
{
	/* XXX Here was some code */

	if (row1 == row2) {
		/* Whole selection within one line */
		winvert(col1 * vt->cwidth, row1 * vt->cheight,
			col2 * vt->cwidth, (row2 + 1) * vt->cheight);
	}
	else {
		/* Invert first line of the selection */
		winvert(col1 * vt->cwidth, row1 * vt->cheight,
			vt->cols * vt->cwidth, (row1 + 1) * vt->cheight);

		/* Invert intermediate lines, if any */
		if (row1 + 1 < row2) {
			winvert(0, (row1 + 1) * vt->cheight,
				vt->cols * vt->cwidth, row2 * vt->cheight);
		}

		/* Invert last line */
		winvert(0, row2 * vt->cheight,
			col2 * vt->cwidth, (row2 + 1) * vt->cheight);
	}
}
