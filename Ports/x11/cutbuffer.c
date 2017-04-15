/* Cut Buffer Interface */

#include "x11.h"

void
wsetcutbuffer(ibuffer, data, len)
	int ibuffer;
	char *data;
	int len;
{
	XStoreBuffer(_wd, data, len, ibuffer);
}

char *
wgetcutbuffer(ibuffer, len_return)
	int ibuffer;
	int *len_return;
{
	static char *lastdata;
	if (lastdata != NULL)
		free(lastdata);
	lastdata= XFetchBuffer(_wd, len_return, ibuffer);
	if (lastdata != NULL) {
		lastdata= realloc(lastdata, *len_return+1);
		if (lastdata != NULL)
			lastdata[*len_return] = EOS;
	}
	return lastdata;
}

void
wrotatecutbuffers(n)
	int n;
{
	static int been_here;
	if (!been_here) {
		/* XRotateBuffers fails if not all 8 buffers have been
		   stored into.  Make sure they are all defined. */
		int i;
		for (i = 0; i < 8; i++) {
			int len;
			if (wgetcutbuffer(i, &len) == NULL)
				wsetcutbuffer(i, "", 0);
		}
		been_here = 1;
	}
	XRotateBuffers(_wd, n);
}
