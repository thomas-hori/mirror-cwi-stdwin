/* Default vtsend function */

#include "vtimpl.h"

void
vtsend(vt, text, len)
	VT *vt;
	char *text;
	int len;
{
	vtansiputs(vt, text, len);
}
