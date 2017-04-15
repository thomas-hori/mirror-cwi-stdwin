/* Mac implementation of serial driver interface */

#define DO_IDLE

#include "vtimpl.h"
#include "vtserial.h"

#ifdef LSC
#include <SerialDvr.h>
#endif
#ifdef MPW
#include <Serial.h>
#include <OSUtils.h>
#define OpenDriver OPENDRIVER
#endif

extern void (*idle_proc)(); /* Declared in stdwin/event.c */

/* Function prototypes */

STATIC int toobad ARGS((char *msg, int err));
STATIC int setup NOARGS;
STATIC void idleserial NOARGS;

/* Print an error message if the error number is nonzero.
   Returns the error number. */

static int
toobad(msg, err)
	char *msg;
	int err;
{
	if (err != 0)
		dprintf("%s: error %d", msg, err);
	return err;
}

/* Reference numbers of the driver for input and output */

static short ain;
static short aout;

/* Buffer for the input driver */

#define IBUFSIZE (8*512)

static char *ibuf;

/* Serial line initialization.
   Returns 0 if all went well. */

static int
setup()
{
	SerShk flags;
	int ibufsize= IBUFSIZE;
	
	ibuf= malloc(IBUFSIZE);
	if (ibuf == NULL) {
		dprintf("warning: can't get memorry for input buffer");
		ibufsize= 0;
	}

	flags.fXOn= 0;
	flags.fCTS= 0;
	/*
	flags.xOn= 'Q' & 0x1f;
	flags.xOff= 'S' & 0x1f;
	*/
	flags.xOn= flags.xOff= 0;
	flags.errs= flags.evts= flags.fInX= flags.fDTR= 0;
	
	return
	    toobad("OpenDriver .AOut", OpenDriver("\p.AOut", &aout))
		||
	    toobad("OpenDriver .AIn", OpenDriver("\p.AIn", &ain))
		||
	    toobad("RAMSDOpen sPortA", RAMSDOpen(sPortA))
	    	||
	    toobad("SerHShake .AOut", SerHShake(aout, &flags))
		||
	    toobad("SerHShake .AIn", SerHShake(ain, &flags))
		||
	    toobad("SerSetBuf .AIn", SerSetBuf(ain, ibuf, ibufsize));
}

#ifdef DO_IDLE

/* Function called in the idle loop of wgetevent.
   When data is available from the serial line,
   it generates a pseudo event to wake up the main loop. */

static void
idleserial()
{
	long llen;
	if (toobad("idleserial: SerGetBuf", SerGetBuf(ain, &llen)))
		return;
	if (llen > 0) {
		EVENT e;
		e.type= WE_SERIAL_AVAIL;
		e.window= NULL;
		wungetevent(&e);
	}
}

#endif

static bool inited; /* Set when initialization succeeded */

/* Open the serial line.
   Returns TRUE if this succeeded. */

bool
openserial()
{
	if (!inited) {
		inited= (setup() == 0);
#ifdef DO_IDLE
		if (inited)
			idle_proc= idleserial;
#endif
	}
	return inited;
}

/* Close the serial line. */

bool
closeserial()
{
	if (inited) {
#ifdef DO_IDLE
		idle_proc= NULL;
#endif
		inited= FALSE;
		if (ibuf != NULL) {
			SerSetBuf(ain, (char *)NULL, 0);
			free(ibuf);
			ibuf= NULL;
		}
	}
	return TRUE;
}

/* Send 'len' bytes of data to the serial line.
   Returns TRUE if this succeeded. */

bool
sendserial(buf, len)
	char *buf;
	int len;
{
	long llen;
	if (!inited) {
		dprintf("sendserial: serial line not open");
		return FALSE;
	}
	if (toobad("sendserial: SerGetBuf", SerGetBuf(aout, &llen)))
		return FALSE;
	if (len < 0)
		len= strlen(buf);
	llen= len;
	return !toobad("sendserial: FSWrite", FSWrite(aout, &llen, buf));
}

/* Receive up to 'len' bytes of data from the serial line.
   If there is no data immediately available, returns 0.
   Returns negative if an error occurred. */

int
receiveserial(buf, len)
	char *buf;
	int len;
{
	long llen;
	if (!inited) {
		dprintf("receiveserial: serial line not open");
		return -1;
	}
	if (toobad("receiveserial: SerGetBuf", SerGetBuf(ain, &llen)))
		return -1;
	if (llen > 0) {
		if (llen > len)
			llen= len;
		if (toobad("receiveserial: FSRead", FSRead(ain, &llen, buf)))
			return -1;
	}
	return llen;
}

/* Send a break.
   The length can be influenced by setting the global breaktime
   (in tenths of a second). */

int breaktime= 3;

bool
breakserial()
{
	long n;
	
	SerSetBrk(aout);
	if (breaktime > 0)
		Delay(breaktime * 6L, &n);
	SerClrBrk(aout);
	return TRUE;
}

/* Change the line speed.  The parameter must be one of the acceptable
   line speeds (300, 600, 1200 etc., through 19200). */

bool
speedserial(speed)
	int speed;
{
	short temp= speed;
	/* This appears a new addition to the interface.
	   I can't find a defined constant for this control call. */
	Control(ain, 13, &temp);
	Control(aout, 13, &temp);
	return TRUE;
}
