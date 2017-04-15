/* Default Cut Buffer Interface */

#include "stdwin.h"
#include "tools.h"

static char *cb0data = NULL;
static int cb0len = 0;

void
wsetcutbuffer(ibuffer, data, len)
	int ibuffer;
	char *data;
	int len;
{
	if (ibuffer != 0)
		return;
	if (cb0data != NULL)
		free(cb0data);
	cb0len = 0;
	cb0data = malloc(len+1);
	if (cb0data != NULL) {
		memcpy(cb0data, data, len);
		cb0data[len]= EOS;
		cb0len = len;
	}
}

char *
wgetcutbuffer(ibuffer, len_return)
	int ibuffer;
	int *len_return;
{
	if (ibuffer != 0)
		return NULL;
	*len_return = cb0len;
	return cb0data;
}

/*ARGSUSED*/
void
wrotatecutbuffers(n)
	int n;
{
}
