/* VT functions that send data back to the serial port. */

#include "vtimpl.h"

void
vtsendid(vt)
	VT *vt;
{
	vtsend(vt, "\033[?0;0;0c", -1);
}

void
vtsendpos(vt)
	VT *vt;
{
	char buf[256];
	int row = vt->cur_row - vt->topterm + 1;
	int col = vt->cur_col + 1;
	CLIPMIN(row, 1);
	sprintf(buf, "\033[%d;%dR", row, col);
	vtsend(vt, buf, -1);
}
