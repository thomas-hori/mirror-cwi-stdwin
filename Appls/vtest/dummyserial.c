/* Dummy implementation of serial driver interface.
   This code behaves like a device echoing all input. */

#include "tools.h"
#include "stdwin.h"
#include "vt.h"
#include "vtserial.h"

bool
openserial()
{
	return TRUE;
}

bool
closeserial()
{
	return TRUE;
}

static int nchars;
static char *buffer;

bool
sendserial(buf, len)
	char *buf;
	int len;
{
	int n= nchars;
	
	if (len < 0)
		len= strlen(buf);
	L_EXTEND(nchars, buffer, char, len);
	if (nchars == n + len) {
		EVENT e;
		memcpy(buffer + n, buf, len);
		e.type= WE_SERIAL_AVAIL;
		wungetevent(&e);
	}
}

int
receiveserial(buf, len)
	char *buf;
	int len;
{
	CLIPMAX(len, nchars);
	if (len > 0) {
		memcpy(buf, buffer, len);
		if (len < nchars)
			memcpy(buffer, buffer+len, nchars - len);
		L_SETSIZE(nchars, buffer, char, nchars - len);
	}
	return len;
}

bool
breakserial()
{
	return FALSE;
}

bool
speedserial()
{
	return FALSE;
}

void
receivefile()
{
}

bool
vtsend(vt, buf, len)
	VT *vt;
	char *buf;
	int len;
{
	return sendserial(buf, len);
}
