/* Trivial packet communication over serial line */
/* THIS IS TOTALLY BUGUS!  TRY AGAIN, GUIDO! */

#include "amoeba.h"
#include "tools.h"
#include "vtserial.h"

#define TIMEOUT		30	/* Time-out for retry */
#define LONGTIMEOUT	600	/* Time-out for complete failure */

int
trans(phdr, pbuf, plen, ghdr, pbuf, plen)
	header *phdr;
	bufptr pbuf;
	int plen;
	header *ghdr;
	bufptr gbuf;
	int glen;
{
	int err= putrep(phdr, pbuf, plen);
	if (err < 0)
		return err;
	return getreq(ghdr, gbuf, glen);
}

int
putrep(hdr, buf, len)
	header *hdr;
	char *buf;
	int len;
{
	int err;
	do {
		sendstart(sizeof(*hdr) + len);
		sendbytes((char*)hdr, sizeof(*hdr));
		sendbytes(buf, len);
	} while ((err= sendfinish()) > 0);
	return err;
}

int
getreq(hdr, buf, len)
	header *hdr;
	char *buf;
	int len;
{
	int err;
	do {
		int cnt= rcvstart();
		if (cnt < 0)
			return cnt;
		rcvbytes((char*)hdr, sizeof(*hdr));
		cnt -= sizeof(*hdr);
		if (cnt > len)
			cnt= len;
		rcvbytes(buf, cnt);
	} while ((err= rcvfinish()) > 0);
	return err;
}

static unsigned short sum;

static int
sendstart(len)
	int len; /* Net number of bytes to send */
{
	char buf[2];
	if (len < 0 || len >= 3*256)
		return -1;
	buf[0]= 'X' + len/256;
	buf[1]= len%256;
	sum= 0;
	return sendbytes(buf, 2);
}

static int
sendbytes(buf, len)
	char *buf;
	int len;
{
	int i;
	for (i= 0; i < len; ++i)
		sum= (sum<<1) ^ (buf[i] & 0xff);
	return sendserial(buf, len) - 1;
}

static int
sendfinish()
{
	char buf[3*256];
	int n;
	unsigned short savesum;
	buf[0]= (sum>>8) & 0xff;
	buf[1]= sum & 0xff;
	sendserial(buf, 2);
	if (!waitserial(2, TIMEOUT))
		return -1;
	n= rcvstart();
	if (n < 0)
		return n;
	rcvbytes(buf, n);
	savesum= 2;
	rcvbytes(buf, 2);
	if (savesum == ((buf[0] & 0xff) << 8) | (buf[1] & 0xff))
		return n; /* ACK: 0=ok, 1=resend */
	else
		return 1; /* ACK garbled, resend */
}

static int
rcvstart()
{
	char buf[2];
	for (;;) {
		if (!waitserial(2, LONGTIMEOUT))
			return -1;
		sum= 0;
		rcvbytes(buf, 1);
		if (buf[0] < 'X' || buf[0] > 'Z')
			continue;
		rcvbytes(buf+1, 1);
		return (buf[0] - 'X')*256 + (buf[1] & 0xff);
	}
}
