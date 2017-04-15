/* Function to convert a C string to a Pascal string.
   The conversion is not in-line, but returns a pointer to a static buffer.
   This is needed when calling some toolbox routines.
   MPW does the conversion in the glue.
*/

#include "macwin.h"

#ifndef CLEVERGLUE

unsigned char *
PSTRING(src)
	register char *src;
{
	static Str255 buf;
	register unsigned char *dst;
	
	dst = &buf[1];
	while ((*dst++ = *src++) != '\0' && dst < &buf[256])
		;
	buf[0] = dst - &buf[1] - 1;
	return buf;
}

#endif /* CLEVERGLUE */
