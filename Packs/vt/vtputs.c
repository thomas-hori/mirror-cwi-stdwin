/* A simple output function, emulating a "dumb tty" */

#include "vtimpl.h"

void
vtputs(vt, text)
	register VT *vt;
	register char *text;
{
#ifndef NDEBUG
	if (text == NULL)
		vtpanic("vtputs with NULL text");
#endif
	while (*text != EOS) {
		if (PRINTABLE(*text)) {
			register char *end = text;
			do {
				text++;
			} while (PRINTABLE(*text));
			vtputstring(vt, text, (int) (end-text));
			text = end;
		}
		else switch (*text++) {
		
		case BEL:	/* Make some noise */
			wfleep();
			break;
		
		case BS:	/* Move 1 left */
			/* Rely on vtsetcursor's clipping */
			vtsetcursor(vt, vt->cur_row, vt->cur_col - 1);
			/* Don't erase --
			   that's part of intelligent echoing */
			break;
		
		case TAB:	/* Move to next tab stop */
			/* Rely on vtsetcursor's clipping */
			vtsetcursor(vt, vt->cur_row,
				(vt->cur_col & ~7) + 8);
			/* Normalize cursor (may cause scroll!) */
			vtputstring(vt, "", 0);
			break;
		
		case LF:	/* Move cursor down in same column */
			vtsetcursor(vt, vt->cur_row + 1, vt->cur_col);
			/* Normalize cursor (may cause scroll!) */
			vtputstring(vt, "", 0);
			break;
		
		case FF:	/* Start over */
			vtreset(vt);
			break;
		
		case CR:	/* Back to col 0 on same line */
			vtsetcursor(vt, vt->cur_row, 0);
			break;
		
		}
	}
	vtsync(vt);
}
