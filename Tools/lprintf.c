/* lprintf prints logging information into a circular buffer,
   where it can be seen after the program has died.
   The first call displays the address of the buffer. */

#define LBUFSIZE 4096

static char *next;
static char lbuf[LBUFSIZE];


lprintf(fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
	char *fmt;
{
	char buf[256];
	char *p;
	
	if (next == 0) {
		next= lbuf;
		dprintf("lbuf at 0x%x ... 0x%x, next at 0x%x",
				lbuf, lbuf+LBUFSIZE, &next);
	}
	
	sprintf(buf, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	
	for (p= buf; *p; ) {
		*next++ = *p++;
		if (next >= lbuf+LBUFSIZE)
			next= lbuf;
	}
}
