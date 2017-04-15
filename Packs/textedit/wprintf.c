#include "stdwin.h"

/* "Printf to a window", with (h, v) and return value like wdrawtext.
   Use only in a draw procedure or between wbegindrawing/wenddrawing. */

void
wprintf(h, v, fmt, rest_of_arguments)
	int h, v;
	char *fmt;
{
	char buf[1000];
	int len;
	int width;
	
	vsprintf(buf, fmt, &rest_of_arguments);
	len = strlen(buf);
	width = wtextwidth(buf, len);
	wdrawtext(h, v, buf, len);
}

/* "Centered printf to a window": like wprintf but allows centered text.
   The first parameter, align, is a percentage: a value of 0 prints
   at the right of h, a value of 50 centers around h, a value of 100
   aligns the end of the text at h.
   The v coordinate always indicates the top of the text.
   Return value is the h coordinate at the right end of the text. */

void
wcprintf(align, h, v, fmt, rest_of_arguments)
	int align;
	int h, v;
	char *fmt;
{
	char buf[1000];
	int len;
	int width;
	
	vsprintf(buf, fmt, &rest_of_arguments);
	len = strlen(buf);
	width = wtextwidth(buf, len);
	wdrawtext(h - align*width/100, v, buf, len);
}
