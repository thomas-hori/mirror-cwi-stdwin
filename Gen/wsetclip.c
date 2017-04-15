/* Simplified Cut Buffer Interface */

#include "stdwin.h"
#include "tools.h"

void
wsetclip(data, len)
	char *data;
	int len;
{
	wrotatecutbuffers(1);
	wsetcutbuffer(0, data, len);
}

char *
wgetclip()
{
	int len;
	return wgetcutbuffer(0, &len);
}
