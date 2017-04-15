/* Functions implementing ANSI operations */

#include "vtimpl.h"

/* Linefeed */

void
vtlinefeed(vt, n)
	VT *vt;
	int n;
{
	while (--n >= 0) {
		if (vt->cur_row == vt->scr_bot - 1) {
			int scr_top = vt->scr_top;
			if (scr_top == vt->topterm)
				scr_top = 0;
			vtscrollup(vt, scr_top, vt->scr_bot, 1);
		}
		else
			vtsetcursor(vt, vt->cur_row + 1, vt->cur_col);
	}
}

/* Reverse linefeed */

void
vtrevlinefeed(vt, n)
	VT *vt;
	int n;
{
	while (--n >= 0) {
		if (vt->cur_row == vt->scr_top)
			vtscrolldown(vt, vt->scr_top, vt->scr_bot, 1);
		else
			vtsetcursor(vt, vt->cur_row - 1, vt->cur_col);
	}
}

/* Reset underline, inverse video attributes */

void
vtresetattr(vt)
	VT *vt;
{
	vtsetflags(vt, 0);
}

/* Set attribute flag (without clearing the others) */

void
vtsetattr(vt, bit)
	VT *vt;
	int bit;
{
	vtsetflags(vt, vt->gflags | (1 << bit));
}

/* Save cursor position */

void
vtsavecursor(vt)
	VT *vt;
{
	vt->save_row = vt->cur_row;
	vt->save_col = vt->cur_col;
}

/* Restore cursor position */

void
vtrestorecursor(vt)
	VT *vt;
{
	vtsetcursor(vt, vt->save_row, vt->save_col);
}

/* Process an arrow key (possibly repeated) */

void
vtarrow(vt, code, repeat)
	VT *vt;
	int code;
	int repeat;
{
	int row = vt->cur_row;
	int col = vt->cur_col;
	int minrow = 0, maxrow = vt->rows;
	
	CLIPMAX(col, vt->cols-1);
	switch (code) {
	case WC_LEFT:
		col -= repeat;
		break;
	case WC_RIGHT:
		col += repeat;
		break;
	case WC_UP:
		row -= repeat;
		break;
	case WC_DOWN:
		row += repeat;
		break;
	}
	CLIPMAX(col, vt->cols-1);
	CLIPMIN(col, 0);
	if (vt->cur_row >= vt->scr_top)
		minrow = vt->scr_top;
	if (vt->cur_row < vt->scr_bot)
		maxrow = vt->scr_bot;
	CLIPMIN(row, minrow);
	CLIPMAX(row, maxrow-1);
	vtsetcursor(vt, row, col);
}

/* Clear to end of line */

void
vteolclear(vt, row, col)
	VT *vt;
	int row, col;
{
	if (row < vt->rows) {
		if (vt->llen[row] > col) {
			if (vt->lazy)
				vtchange(vt, row, col, row + 1, vt->llen[row]);
			else {
				VTBEGINDRAWING(vt);
				werase(col*vt->cwidth, row*vt->cheight,
				    vt->llen[row]*vt->cwidth,
				    (row+1)*vt->cheight);
			}
			vt->llen[row] = col;
		}
	}
}

/* Clear to end of screen */

void
vteosclear(vt, row, col)
	VT *vt;
	int row, col;
{
	vteolclear(vt, row, col);
	if (vt->lazy)
		vtchange(vt, row + 1, 0, vt->rows, vt->cols);
	else {
		VTBEGINDRAWING(vt);
		werase(0, (row + 1) * vt->cheight,
			vt->cols * vt->cwidth, vt->rows * vt->cheight);
	}
	for (row = row + 1; row < vt->rows; ++row)
		vt->llen[row] = 0;
}

/* Delete n lines */

void
vtdellines(vt, n)
	VT *vt;
	int n;
{
	vtscrollup(vt, vt->cur_row, vt->scr_bot, n);
}

/* Insert n lines */

void
vtinslines(vt, n)
	VT *vt;
	int n;
{
	vtscrolldown(vt, vt->cur_row, vt->scr_bot, n);
}

/* Scroll a range of lines n positions up */

void
vtscrollup(vt, r1, r2, n)
	VT *vt;
	int r1, r2;
	int n;
{
	if (n > 0 && r1 < r2) {
		int i;
		vtcirculate(vt, r1, r2, -n);
		/* Clear lines at bottom of scrolling screenpart */
		for (i = r2 - n; i < r2; ++i)
			vt->llen[i] = 0;
		vtscroll(vt, r1, 0, r2, vt->cols, -n, 0);
	}
}


/* Scroll a range of lines n positions down */

void
vtscrolldown(vt, r1, r2, n)
	VT *vt;
	int r1, r2;
	int n;
{
	if (n > 0 && r1 < r2) {
		int i;
		vtcirculate(vt, r1, r2, n);
		for (i = r1 + n; --i >= r1; )
			vt->llen[i] = 0;
		vtscroll(vt, r1, 0, r2, vt->cols, n, 0);
	}
}

/* Insert n characters */

void
vtinschars(vt, n)
	VT *vt;
	int n;
{
	int row;
	
	if (n > 0 && (row= vt->cur_row) < vt->rows) {
		int col = vt->cur_col;
		int len = vt->llen[row];
		if (len > col) {
			if (col+n >= vt->cols) {
				vtchange(vt, row, col, row+1, len);
				vt->llen[row] = col;
			}
			else {
				register int i;
				char *data = vt->data[row];
				unsigned char *flags = vt->flags[row];
				len += n;
				if (len > vt->cols)
					len = vt->cols;
				for (i = len-n; --i >= col; )
					data[i+n] = data[i];
				vtscroll(vt, row, col, row+1, len, 0, n);
				vt->llen[row] = len;
				/* Clear the inserted stretch */
				for (i = col+n; --i >= col; ) {
					data[i] = ' ';
					flags[i] = 0;
				}
			}
		}
	}
}

/* Delete n characters */

void
vtdelchars(vt, n)
	VT *vt;
	int n;
{
	int row;
	
	if (n > 0 && (row = vt->cur_row) < vt->rows) {
		int col = vt->cur_col;
		int len = vt->llen[row];
		if (len > col) {
			if (len <= col+n) {
				vtchange(vt, row, col, row+1, len);
				vt->llen[row] = col;
			}
			else {
				register int i;
				char *data = vt->data[row];
				for (i = col+n; i < len; ++i)
					data[i-n] = data[i];
				vtscroll(vt, row, col, row+1, len, 0, -n);
				vt->llen[row] -= n;
			}
		}
	}
}
