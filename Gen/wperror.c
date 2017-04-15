/* STDWIN -- UNIVERSAL WPERROR. */

#include "tools.h"
#include "stdwin.h"

/* STDWIN equivalent of perror(). */

void
wperror(name)
	char *name;
{
	char buf[256];
	
	sprintf(buf, "%s: Error %d.", name, errno);
	wmessage(buf);
}
