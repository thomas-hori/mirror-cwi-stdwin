/* STANDARD WINDOWS -- TEXT MEASURING. */

#include "alfa.h"

int
wlineheight()
{
	return 1;
}

int
wbaseline()
{
	return 1;
}

#define CHARWIDTH(c) ((c) < ' ' ? 2 : (c) < 0177 ? 1 : (c) < 0200 ? 2 : 4)

int
wtextwidth(str, len)
	char *str;
	int len;
{
	int i;
	int w= 0;
	
	if (len < 0)
		len= strlen(str);
	for (i= 0; i < len; ++i) {
		unsigned char c= str[i];
		w += CHARWIDTH(c);
	}
	return w;
}

int
wcharwidth(c)
	int c;
{
	c &= 0xff;
	return CHARWIDTH(c);
}

int
wtextbreak(str, len, width)
	char *str;
	int len;
	int width;
{
	int i;
	
	if (len < 0)
		len= strlen(str);
	for (i= 0; i < len && width > 0; ++i) {
		unsigned char c= str[i];
		width -= CHARWIDTH(c);
		if (width < 0)
			break; /* Before incrementing i! */
	}
	return i;
}
