/* TERMCAP STDWIN -- Alarm timers (BSD-specific) */

#include "alfa.h"

#include <errno.h>
#include <sys/time.h>

#ifdef AMOEBA
#define select(a, b, c, d, e) (-1) /* XXX just to get it to link... */
#endif

/* Alarm timer values must be stored as absolute times,
   but since they are in tenths of a second and stored in a long,
   there isn't enough space to store the true absolute time.
   Therefore, they are stored relative to the second when the
   first call to wsettimer was made, which is saved in torigin */

static long torigin;		/* Seconds */
static long nexttimer;		/* Deciseconds */

/* Return the window with the first timer to go off, if any, NULL otherwise */

static WINDOW *
getnexttimer()
{
	WINDOW *win;
	WINDOW *cand= NULL;
	
	for (win= &winlist[0]; win < &winlist[MAXWINDOWS]; ++win) {
		if (win->open) {
			long t= win->timer;
			if (t != 0) {
				if (cand == NULL || t < cand->timer)
					cand= win;
			}
		}
	}
	return cand;
}

/* Compute a new value for nexttimer.
   Return the relevant window as a convenience. */

static WINDOW *
setnexttimer()
{
	WINDOW *win= getnexttimer();
	
	if (win == NULL)
		nexttimer= 0;
	else
		nexttimer= win->timer;
	return win;
}

/* Set the alarm timer for a given window */

void
wsettimer(win, deciseconds)
	WINDOW *win;
	int deciseconds;
{
	win->timer= 0;
	if (deciseconds > 0) {
		struct timeval tv;
		struct timezone tz;
		
		if (gettimeofday(&tv, &tz) >= 0) {
			if (torigin == 0) {
				torigin= tv.tv_sec;
			}
			win->timer= deciseconds + 10 * (tv.tv_sec - torigin) +
				tv.tv_usec/100000;
		}
	}
	(void) setnexttimer();
}

/* Return a pointer suitable as timeout parameter for BSD select(2).
   If no alarms are currently set, return a NULL pointer. */

static struct timeval *
timeout()
{
	if (nexttimer == 0) {
		return NULL;
	}
	else {
		static struct timeval tv;
		struct timezone tz;
		
		if (gettimeofday(&tv, &tz) < 0) {
			return NULL;
		}
		else {
			long tout;
			tout= nexttimer
				- (tv.tv_sec - torigin) * 10
				- tv.tv_usec / 100000;
			if (tout <= 0)
				tv.tv_sec= tv.tv_usec= 0;
			else {
				tv.tv_sec= tout / 10; 
				tv.tv_usec= (tout % 10) * 100000;
			}
			return &tv;
		}
	}
}

/* Check if an alarm has gone off, and if so, generate an appropriate event.
   This can be called at any time, but for efficiency reasons it should
   only be called when an alarm has actually gone of (i.e., select has
   timed out).  If an alarm has gone off, it will always be found by
   this function. */

static bool
dotimer(ep)
	EVENT *ep;
{
	WINDOW *win= setnexttimer();
	struct timeval *tp;
	
	if (win == NULL) {
		/* no event found (spurious call) */
		return FALSE;
	}
	tp= timeout();
	if (tp == NULL) {
		/* unexpected NULL timeout() */
		return FALSE;
	}
	if (tp->tv_sec == 0 && tp->tv_usec == 0) {
		/* report timer event */
		ep->type= WE_TIMER;
		ep->window= win;
		win->timer= 0;
		(void) setnexttimer();
		return TRUE;
	}
	else {
		/* it is not yet time */
		return FALSE;
	}
}

/* Check for timer events.
   Call this after trmavail() returns 0, just before calling trminput() */

bool
_w_checktimer(ep, mayblock)
	EVENT *ep;
	bool mayblock;
{
	for (;;) {
		struct timeval *tp= timeout();
		/* This is naive.  BSD 4.3 really uses arrays of longs
		   as arguments to select.  Fortunately, stdin is fd 0. */
		unsigned long rd, wd, xd;
		int fd, nfd;
		int nfound;
		if (!mayblock) {
			return tp != NULL &&
				tp->tv_sec == 0 && tp->tv_usec == 0 &&
				dotimer(ep);
		}
		fd= 0; /* stdin */
		rd= 1 << fd;
		nfd= fd+1;
		wd= xd= 0;
		errno= 0;
		nfound= select(nfd, &rd, &wd, &xd, tp);
		/* Note: if select returns negative, we also break
		   out of the loop -- better drop a timer event than
		   loop forever on a select error.
		   The only exception is EINTR, which may have been caused
		   by an application's signal handler */
		if (nfound < 0) {
			if (errno == EINTR) {
				continue;
			}
		}
		if (nfound != 0)
			break;
		if (dotimer(ep))
			return TRUE;
	}
	return FALSE;
}

/* P.S.: It is not necessary to recompute nextalarm when windows are
   deleted.  This can at most cause a spurious time-out, after which
   dotimeout() is called again which recomputes nextalarm as a side
   effect (twice, even).  Applications incur a slight overhead if they
   delete a window with a timer set and no other windows have timers
   set; in this case a larger part of the timeout code is called until
   the alarm goes off (which is then ignored!). */
