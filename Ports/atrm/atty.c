/* System-dependent code for VTRM, AMOEBA tty driver version. */

/* From ~amoeba/h: */
#include "amoeba.h"
#include "stdio.h"
#include "tty.h"

capability *getcap();

/* From ../h: */
#include "vtrm.h"

static int know_ttys;
static char flagbuf[1+NFLAGS];
#define oldflags (flagbuf+1)
static capability *incap;

static char newflags[]= {
	_SETFLAG | _ISTRIP,
	_SETFLAG | _OSTRIP,
	_CLRFLAG | _ICANON,
	_CLRFLAG | _ECHO,
	_CLRFLAG | _OPOST,
	/* I believe the following aren't necessary if ICANON is clear? */
	/*
	_CLRFLAG | _INLCR,
	_CLRFLAG | _ICRNL,
	_CLRFLAG | _ISIG,
	*/
};

int
setttymode()
{
	header h;
	short n;
	
	if (!know_ttys) {
		if (incap == NULL) {
			incap= getcap("STDIN");
			if (incap == NULL)
				return TE_NOTTY;
		}
		
		h.h_port= incap->cap_port;
		h.h_priv= incap->cap_priv;
		h.h_command= TTQ_STATUS;
		h.h_extra= _GETFLAGS;
		h.h_size= NFLAGS;
		n= trans(&h, NILBUF, 0, &h, oldflags, NFLAGS);
		if (n != NFLAGS || h.h_status != TTP_OK) {
			fprintf(stderr,
				"TTQ_STATUS: trans=%d, h_status=%d\n",
				n, h.h_status);
			return TE_NOTTY;
		}
		know_ttys= 1;
	}
	
	h.h_port= incap->cap_port;
	h.h_priv= incap->cap_priv;
	h.h_command= TTQ_CONTROL;
	h.h_size= sizeof(newflags);
	n= trans(&h, newflags, sizeof(newflags), &h, NILBUF, 0);
	if (n != 0 || h.h_status != TTP_OK) {
		fprintf(stderr,
			"TTQ_CONTROL(set): trans=%d, h_status=%d\n",
			n, h.h_status);
		return TE_NOTTY;
	}
	return TE_OK;
}

resetttymode()
{
	if (know_ttys) {
		header h;
		short n;
		
		h.h_port= incap->cap_port;
		h.h_priv= incap->cap_priv;
		h.h_command= TTQ_CONTROL;
		h.h_size= NFLAGS+1;
		oldflags[-1]= _FLAGS;
		n= trans(&h, oldflags-1, NFLAGS+1, &h, NILBUF, 0);
		if (n != 0 || h.h_status != TTP_OK)
			fprintf(stderr,
				"TTQ_CONTROL(reset): trans=%d, h_status=%d\n",
				n, h.h_status);
		know_ttys= 0;
	}
}


/*
 * Return the next input character, or -1 if read fails.
 * Only the low 7 bits are returned, so reading in RAW mode is permissible
 * (although CBREAK is preferred if implemented).
 * To avoid having to peek in the input buffer for trmavail, we use the
 * 'read' system call rather than getchar().
 * (The interface allows 8-bit characters to be returned, to accomodate
 * larger character sets!)
 */

static int pushback= -1;

int
trminput()
{
	int c;

	if (pushback >= 0) {
		c= pushback;
		pushback= -1;
		return c;
	}
	c= getchar();
	if (c == EOF)
		return EOF;
	return c & 0177;
}

trmpushback(c)
	int c;
{
	pushback= c;
}

int
trmavail()
{
	header h;
	short n;
	long avail;
	
	if (pushback >= 0)
		return 1;
	h.h_port= incap->cap_port;
	h.h_priv= incap->cap_priv;
	h.h_command= TTQ_STATUS;
	h.h_extra= _NREAD;
	n= trans(&h, NILBUF, 0, &h, &avail, sizeof avail);
	if (n != sizeof(avail) || h.h_status != TTP_OK) {
		fprintf(stderr,
			"TTQ_STATUS(avail): trans=%d, h_status=%d\n",
			n, h.h_status);
		return -1;
	}
	return avail;
}

/*
 * Suspend the editor.
 * Should be called only after trmend and before trmstart!
 */

trmsuspend()
{
	/* Can't */
}
