/* Scrolling.
   This is not meant to scroll the window with respect to the document
   (that's done with wshow), but to say that a change has occurred
   in the indicated rectangle for which a full update can be avoided
   by scrolling its content by (dh, dv).
   If we can't do such scrolls, we'll call wchange instead. */

#include "alfa.h"

/*ARGSUSED*/
void
wscroll(win, left, top, right, bottom, dh, dv)
	WINDOW *win;
	int left, top;
	int right, bottom;
	int dh, dv;
{
	/* if (dh != 0 || left > 0 || right < columns) {
		wchange(win, left, top, right, bottom);
		return;
	}*/
	/* Convert to screen coordinates: */
	top -= win->offset;
	bottom -= win->offset;
	/* Clip to window: */
	if (top < win->top)
		top= win->top;
	if (bottom > win->bottom)
		bottom= win->bottom;
	/* Do the scroll: */
	if (top < bottom) {
		dv= -dv;
		trmscrollup(top, bottom-1, dv);
		/* Set the rectangle filled with 'empty' now: */
		if (dv < 0) {
			if (top-dv < bottom)
				bottom= top-dv;
		}
		else {
			if (bottom-dv > top)
				top= bottom-dv;
		}
		/*
		wchange(win, left, top+win->offset,
			right, bottom+win->offset);
		*/
		wchange(win, left, win->top+win->offset,
			right, win->bottom+win->offset);
	}
}
