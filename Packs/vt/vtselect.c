/* Copy and paste operations for VT.
   The selections here are not rectangles,
   like in most other vt-stuff */

/* XXX This must be redesigned.
   In the new design, the event loop is in the caller and we remember
   the state somehow in the vt structure.
   This is necessary to make it independent of aterm (special semantics
   of WE_EXTERN events) and to support extending the selection with the
   third button.
*/

#include "vtimpl.h"


extern int extra_downs; /* For aterm */


/* Data structure used by coord_sort */

struct coord {		/* A coordinate */
	int c, r;	/* Column and row */
};

/*
 *	Bubblesort an array of coordinates.
 */
static void
coord_sort(coord, n)
    struct coord coord[];
    int n;
{
    int i, j, big;
    /* Bubble */
    for (j = n; --j > 0; ) {
	/* Find biggest in 0..j */
	for (big = i = 0; ++i <= j; ) { /* Compare big and i */
	    if (coord[big].r < coord[i].r ||
	    (coord[big].r == coord[i].r && coord[big].c < coord[i].c))
		big = i; /* Found a bigger one */
	}
	if (big != j) {
	    struct coord swap;
	    swap = coord[big]; coord[big] = coord[j]; coord[j] = swap;
	}
    }
}

/*
 *	Delete double occurences in a sorted
 *	array of coordinates (Both of them!)
 */
static int
no_double(coord, n)
    struct coord coord[];
    int n;
{
    int i, j;
    for (i = j = 0; i < n; ) {
	if (i < n - 1 && coord[i].r == coord[i + 1].r
	&& coord[i].c == coord[i + 1].c) {
	    i += 2; /* Skip both */
	}
	else {
	    if (i != j) coord[j] = coord[i]; /* Copy */
	    ++i; ++j;
	}
    }
    return j; /* Return # array elements */
}

/*
 *	Set the selection of a vt, update state variables
 */
static void
set_selection(vt, row1, col1, row2, col2)
    VT *vt;
    int row1, col1, row2, col2;
{
    /* Screen coordinates of begins/ends of selection */
    struct coord coord[4];
    int n_coord, i;

    /* Above last remembered line; repair */
    if (row1 < -vt->topterm) {
	    row1 = -vt->topterm;
	    col1 = 0;
    }

    /* Initialise array */
    coord[0].r = vt->sel_row1;
    coord[0].c = vt->sel_col1;
    coord[1].r = vt->sel_row2;
    coord[1].c = vt->sel_col2;
    coord[2].r = row1;
    coord[2].c = col1;
    coord[3].r = row2;
    coord[3].c = col2;

    /* Sort, optimise */
    coord_sort(coord, 4);
    n_coord = no_double(coord, 4);

    /* Blit */
    if (n_coord > 0) {
	VTBEGINDRAWING(vt);
	for (i = 0; i < n_coord; i += 2)
	    vtinvert(vt, coord[i].r, coord[i].c, coord[i+1].r, coord[i+1].c);
	VTENDDRAWING(vt);
    }

    /* Save state */
    vt->sel_row1 = row1;
    vt->sel_col1 = col1;
    vt->sel_row2 = row2;
    vt->sel_col2 = col2;
} /* set_selection */


/* Pass the selection to STDWIN.
   Always set cut buffer 0, also try primary selection.
   Return success/failure of the latter. */

static int
pass_selection(vt, data, len)
	VT *vt;
	char *data;
	int len;
{
	wsetcutbuffer(0, data, len);
	return wsetselection(vt->win, WS_PRIMARY, data, len);
}


/* Interface to STDWIN selection and cut buffer: save the copied text.
   Returns success/failure */

static int
extract_selection(vt, row1, col1, row2, col2)
    VT *vt;
{
    /* Swap if needed to make sure (row1, col1) <= (row2, col2) */
    if (row1 > row2 || row1 == row2 && col1 > col2) {
	register int scratch;
	scratch = row1; row1 = row2; row2 = scratch;
	scratch = col1; col1 = col2; col2 = scratch;
    }

    /* Make sure (0, 0) <= (row1, col1) and (row2, col2) <= (vt->rows, 0) */
    if (row1 < 0) { row1 = 0; col1 = 0; }
    if (row2 > vt->rows) { row2 = vt->rows; col2 = 0; }

    /* Now, if (row1, col1) >= (row2, col2), the selection is empty */
    if (row1 > row2 || row1 == row2 && col1 >= col2) {
	wresetselection(WS_PRIMARY); /* Clear any previous selection */
	return 1;
    }

    /* Check for simple case -- everything on one line */
    if (row1 == row2 && col2 <= vt->llen[row2])
	return pass_selection(vt, vt->data[row1] + col1, col2 - col1);

    /* Harder case: more lines, or one line + end-of-line */

    {
	char *str, *p;
	int ok;

	/* Get (more than) enough memory, fail if can't get it */
	p = str = malloc((row2 - row1 + 1) * (vt->cols + 1));
	if (p == NULL) return 0; /* Fail */

	/* Copy first line, add a newline */
	if (col1 < vt->llen[row1]) {
	    strncpy(p, vt->data[row1] + col1, vt->llen[row1] - col1);
	    p += vt->llen[row1] - col1;
	}
	*p++ = '\n';

	/* Copy intermediate lines, adding newlines */
	while (++row1 < row2) {
	    strncpy(p, vt->data[row1], vt->llen[row1]);
	    p += vt->llen[row1];
	    *p++ = '\n';
	}

	/* Copy last line -- if this is not the first as well, nor too far */
	if (row1 == row2 && row2 < vt->rows) {
	    if (col2 <= vt->llen[row2]) {
		strncpy(p, vt->data[row2], col2);
		p += col2;
	    }
	    else {
		/* Beyond last char, copy a \n as well */
		strncpy(p, vt->data[row2], vt->llen[row2]);
		p += vt->llen[row2];
		*p++ = '\n';
	    }
	}

	/* Pass the string to STDWIN, free it, and return success */
	ok = pass_selection(vt, str, (int) (p - str));
	free(str);
	return ok;
    }
}


/* Symbolic values for the modes we can be in while selecting: */

#define SEL_CHARS	0	/* Selecting chars */
#define SEL_WORDS	1	/* Selecting words */
#define SEL_LINES	2	/* Selecting lines */
#define SEL_KINDS	3	/* How many modes we know */

/* Macros to distinguish between various "brands" of characters.
   A "word" is a sequence of characters of the same brand */

#define BRAND_LETTER	0
#define BRAND_SPACE	1
#define BRAND_PUNCT	2

/* Determine the brand of a character */

static int
brand(ch)
    int ch;
{
    return (('a' <= ch && ch <= 'z') ||
	 ('A' <= ch && ch <= 'Z') || ('0' <= ch && ch <= '9') ||
	 (ch == '_') ? BRAND_LETTER :
	 (ch == ' ' || ch == '\t') ? BRAND_SPACE : BRAND_PUNCT);
}


/* Called by the mainloop upon a mouse-down. Exits when the mouse is
   up again; any other intermediate events terminates the selection.
   Returns whether the selection succeeded */

int
vtselect(vt, e)
    VT *vt;
    EVENT *e;
{
    int first_row, first_col; /* Coords save at mouse-down */
    int was_lazy; /* Zapped because vtsetcursor is lazy as well */
    int csr_row, csr_col; /* Cursor saved at mouse-down */
    int button_down = 1; /* That's the reason we're called after all */
    int sel_mode = SEL_CHARS; /* This one is not yet handled */
    int timer_pending = 0;
    int success = 1; /* We are optimists */

    /* Save cursor position */
    was_lazy = vt->lazy; vt->lazy = 0;
    csr_row = vt->cur_row;
    csr_col = vt->cur_col;

    /* Init */
    first_row = e->u.where.v / vt->cheight;
    first_col = (e->u.where.h + vt->cwidth/2) / vt->cwidth;
    if (first_col > vt->llen[first_row]) {
	/* Clicked beyond eoln; use first char on next line instead */
	++first_row;
	first_col = 0;
    }

    vtsetcursor(vt, first_row, first_col);

    do {
	int row, col; /* Current mouse position */

	wgetevent(e);
	switch (e->type) {
	case WE_MOUSE_UP:
	    button_down = 0;
	    wsettimer(vt->win, 2);
	    timer_pending = 1;
	    break;
	case WE_MOUSE_MOVE:
	    break;
	case WE_MOUSE_DOWN:
	    if (e->u.where.button != 1) {
		/* E.g. paste-button; stop selection */
		timer_pending = button_down = 0;
		wungetevent(e);
		break;
	    }
	    button_down = 1;
	    sel_mode = (sel_mode + 1) % SEL_KINDS;
	    break;
	case WE_EXTERN:	/* Got output from child - save */
	    ++extra_downs;
	    continue;	/* Get next event */
	case WE_TIMER:
	    timer_pending = 0;
	    break;
	default:
	    wungetevent(e);
	    success = 0;
	    goto reset;
	}

	/*
	 * We only get here when we want
	 * to draw the current selection;
	 * Compute new coords
	 */

	row = e->u.where.v / vt->cheight;
	col = (e->u.where.h + vt->cwidth/2) / vt->cwidth;
	vtsetcursor(vt, row, col);

#if 0	/* To do: Make sure everything in the neighbourhood can be seen: */
	/* This does not work; it's far too fast */
	if (vt->rows > 3 && vt->cols > 3)
	    wshow(vt->win,
		e->u.where.h - vt->cwidth, e->u.where.v - vt->cheight,
		e->u.where.h + vt->cwidth, e->u.where.v + vt->cheight);
#endif

	switch (sel_mode) {
	int rs, cs;
	case SEL_CHARS:
	case SEL_WORDS:
	    /* Clip row and col */
	    if (row > vt->rows) row = vt->rows;
	    else if (row < 0) row = 0;
	    if (col > vt->cols) col = vt->cols;
	    else if (col < 0) col = 0;
	    rs = first_row;
	    cs = first_col;
	    if (sel_mode == SEL_CHARS) {
		/* Select including the eoln when beyond last char */
		if (vt->llen[row] < col) col = vt->cols;
	    } else {
		/* Sort coordinates */
		if (rs > row || (rs == row && cs > col)) {
		    register tmp;
		    tmp = row; row = rs; rs = tmp;
		    tmp = col; col = cs; cs = tmp;
		}
		/* Expand */
		while (cs > 0 &&
		    brand(vt->data[rs][cs]) == brand(vt->data[rs][cs - 1]))
			--cs;
		if (col >= vt->llen[row]) col = vt->cols;
		else while (++col < vt->llen[row] &&
		    brand(vt->data[row][col - 1]) == brand(vt->data[row][col]))
			;
	    }
	    /* Update screen */
	    set_selection(vt, rs, cs, row, col);
	    break;
	case SEL_LINES:
	    if (++row > vt->rows) row = vt->rows;
	    set_selection(vt,
		first_row >= row ? first_row + 1 : first_row, 0, row, 0);
	    break;
	}

    } while (button_down || timer_pending);

    /* Now pass the selection to STDWIN */
    if (!extract_selection(vt,
    		vt->sel_row1, vt->sel_col1, vt->sel_row2, vt->sel_col2))
    	wfleep(); /* Give the user some indication that it failed */

reset:
    /* Invert back to normal:	*/
    set_selection(vt, 0, 0, 0, 0);
    vtsetcursor(vt, csr_row, csr_col);
    vt->lazy = was_lazy;
    return success;
}


/* Extend the selection */

int
vtextendselection(vt, ep)
	VT *vt;
	EVENT *ep;
{
	/* XXX not yet implemented */
	wfleep();
	return 0;
}
