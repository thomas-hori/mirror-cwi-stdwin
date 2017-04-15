/* Resize function */

#include "vtimpl.h"

/* Resize the screen.
   When growing, blanks are added to the right and/or at the top (!);
   when shrinking, characters are lost at those edges.
   It is really a disaster if we run out of memory halfway
   (much more so than in vtopen);
   in this case the caller should immediately call vtclose,
   and expect some memory leaks... */

bool
vtresize(vt, rows, cols, save)
	VT *vt;
	int rows, cols;
	int save;
{
	int rowdiff;
	int i;

	rows += save;
	rowdiff = rows - vt->rows;
	if (cols != vt->cols) {
		if (cols < vt->cols) {
			/* Clear the corresponding screen area */
			vtchange(vt, 0, cols, vt->rows, vt->cols);
			vt->cols = cols;
		}
		i = vt->rows;
		if (rows < i)
			i = rows;
		while (--i >= 0) {
			if (!RESIZE(vt->data[i], char, cols))
				return FALSE;
			if (!RESIZE(vt->flags[i], unsigned char, cols))
				return FALSE;
			if (vt->llen[i] > cols)
				vt->llen[i] = cols;
		}
		if (cols > vt->cols) {
			/* Clear the corresponding screen area */
			vtchange(vt, 0, vt->cols, rows, cols);
			vt->cols = cols;
		}
	}

	if (rowdiff != 0) {
		if (rowdiff < 0) {
			/* Get the last line into its new position */
			vtscrollup(vt, 0, vt->rows, -rowdiff);
			/* Free the excess lines */
			for (i = rows; i < vt->rows; ++i) {
				FREE(vt->data[i]);
				FREE(vt->flags[i]);
			}
			/* Clear the corresponding screen area */
			vtchange(vt, rows, 0, vt->rows, cols);
			vt->rows = rows;
		}

		if (!RESIZE(vt->data, char *, rows))
			return FALSE;
		if (!RESIZE(vt->flags, unsigned char *, rows))
			return FALSE;
		if (!RESIZE(vt->llen, short, rows))
			return FALSE;

		if (rowdiff > 0) {
			/* Add new lines */
			for (i = vt->rows; i < rows; ++i) {
				if ((vt->data[i] = NALLOC(char, cols)) == NULL)
					return FALSE;
				if ((vt->flags[i] = NALLOC(unsigned char, cols)) == NULL)
					return FALSE;
				vt->llen[i] = 0;
			}
			/* Clear the corresponding screen area */
			vtchange(vt, vt->rows, 0, rows, cols);
			vt->rows = rows;
			/* Get the last line into its new position */
			vtscrolldown(vt, 0, rows, rowdiff);
		}
	}

	if (rowdiff != 0 || save != vt->topterm) {
		vt->topterm = save;
		vtsetscroll(vt, 0, 0); /* Reset scrolling region */
	}

	wsetdocsize(vt->win,
		vt->cols * vt->cwidth, vt->rows * vt->cheight);

	i = vt->cur_row + rowdiff;
	CLIPMIN(i, save);
	vtsetcursor(vt, i, vt->cur_col);

	return TRUE;
}

/* Adapt the screen size to the new window dimensions.
   The 'save' parameter is computed so as to keep the total amount
   of rows the same (if possible).
   Return value as for vtresize (i.e., FALSE is a disaster). */

bool
vtautosize(vt)
	VT *vt;
{
	int width, height;
	int newrows, newcols, newsave;
	wgetwinsize(vt->win, &width, &height);
	newrows = height / vt->cheight;
	newcols = width / vt->cwidth;
	newsave = vt->rows - newrows;
	D( printf("vtautosize: size now %dx%d (char = %dx%d) => %dx%d\n",
		width,height, vt->cwidth,vt->cheight, newrows,newcols) );
	CLIPMIN(newsave, 0);
	return vtresize(vt, newrows, newcols, newsave);
}
