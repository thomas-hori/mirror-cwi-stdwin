/* X11 STDWIN -- Alarm timers (BSD-specific) */

#ifdef AMOEBA
#include "amtimer.c"
#else /* !AMOEBA */

#include "x11.h"

#include <errno.h>

/* Alarm timer values must be stored as absolute times,
   but since they are in tenths of a second and stored in a long,
   there isn't enough space to store the true absolute time.
   Therefore, they are stored relative to the second when the
   first call to wsettimer was made, which is saved in torigin */

static long torigin;		/* Seconds */
static long nexttimer;		/* Deciseconds */

/* Compute a new value for nexttimer.
   Return the relevant window as a convenience */

static WINDOW *
setnexttimer()
{
	WINDOW *win= _wnexttimer();
	
	if (win == NULL)
		nexttimer= 0;
	else
		nexttimer= win->timer;
	_wdebug(3, "setnexttimer: nexttimer at %d for %x", nexttimer, win);
	return win;
}

/* Set the alarm timer for a given window */

void wsettimer(win, deciseconds)
	WINDOW *win;
	int deciseconds;
{
	_wdebug(3, "wsettimer: deciseconds=%d", deciseconds);
	win->timer= 0;
	if (deciseconds > 0) {
		struct timeval tv;
		struct timezone tz;
		
		if (gettimeofday(&tv, &tz) < 0) {
			_wwarning("wsettimer: can't get time of day");
		}
		else {
			if (torigin == 0) {
				torigin= tv.tv_sec;
				_wdebug(3, "wsettimer: torigin %d", torigin);
			}
			win->timer= deciseconds + 10 * (tv.tv_sec - torigin) +
				tv.tv_usec/100000;
			_wdebug(4, "wsettimer(%d): timer set to %d",
				deciseconds, win->timer);
		}
	}
	(void) setnexttimer();
}

/* Return a pointer suitable as timeout parameter for BSD select(2).
   If no alarms are currently set, return a NULL pointer */

static struct timeval *
timeout()
{
	if (nexttimer == 0) {
		_wdebug(4, "timeout: no timers set");
		return NULL;
	}
	else {
		static struct timeval tv;
		struct timezone tz;
		
		if (gettimeofday(&tv, &tz) < 0) {
			_wwarning("timeout: can't get time of day");
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
			_wdebug(4, "timeout: delay %d sec %d usec",
				tv.tv_sec, tv.tv_usec);
			return &tv;
		}
	}
}

/* Check if an alarm has gone off, and if so, generate an appropriate event.
   This can be called at any time, but for efficiency reasons it should
   only be called when an alarm has actually gone of (i.e., select has
   timed out).  If an alarm has gone off, it will always be found by
   this function */

static bool
dotimer(ep)
	EVENT *ep;
{
	WINDOW *win= setnexttimer();
	struct timeval *tp;
	
	if (win == NULL) {
		_wdebug(1, "dotimer: no event found (spurious call)");
		return FALSE;
	}
	tp= timeout();
	if (tp == NULL) {
		_wdebug(1, "dotimer: unexpected NULL timeout()");
		return FALSE;
	}
	if (tp->tv_sec == 0 && tp->tv_usec == 0) {
		_wdebug(3, "dotimer: reporting timer event");
		ep->type= WE_TIMER;
		ep->window= win;
		win->timer= 0;
		(void) setnexttimer();
		return TRUE;
	}
	else {
		_wdebug(1, "dotimer: it is not yet time");
		return FALSE;
	}
}

/* Check for timer events.
   Call this after XPending returns 0, just before calling XNextEvent */

bool
_w_checktimer(ep, mayblock)
	EVENT *ep;
	bool mayblock;
{
	for (;;) {
		struct timeval *tp= timeout();
		/* This is naive.  BSD 4.3 really uses an array of longs
		   as arguments to select.  Hope there aren't >32 fds, or
		   the socket and pipe are among the first 32 files opened */
		unsigned long rd, wd, xd;
		int fd, nfd;
		int nfound;
		if (tp != NULL)
			_wdebug(4, "_wchecktimer: sec=%d, usec=%d",
				tp->tv_sec, tp->tv_usec);
		if (!mayblock) {
			if (tp != NULL)
				_wdebug(4, "_wchecktimer: !mayblock");
			return tp != NULL &&
				tp->tv_sec == 0 && tp->tv_usec == 0 &&
				dotimer(ep);
		}
		fd= ConnectionNumber(_wd);
		rd= 1 << fd;
		nfd= fd+1;
#ifdef PIPEHACK
		if (_wpipe[0] >= 0) {
			rd |= (1 << _wpipe[0]);
			if (_wpipe[0]+1 > nfd)
				nfd= _wpipe[0]+1;
		}
#endif
		wd= xd= 0;
		if (tp == NULL)
			_wdebug(4, "wchecktimer: select w/o timeout");
		else
			_wdebug(4,
				"_wchecktimer: select call, sec=%d, usec=%d",
				tp->tv_sec, tp->tv_usec);
#ifdef AMOEBA
		/* Amoeba is far superior to unix:
		   we don't need something as silly as select... */
		nfound= EIO;
#else
		nfound= select(nfd, &rd, &wd, &xd, tp);
#endif
		_wdebug(4, "_wchecktimer: select returned %d", nfound);
		/* Note: if select returns negative, we also break
		   out of the loop -- better drop a timer event than
		   loop forever on a select error.
		   The only exception is EINTR, which may have been caused
		   by an application's signal handler */
		if (nfound < 0) {
			if (errno == EINTR) {
				_wdebug(4, "_wchecktimer: select interrupted");
				continue;
			}
			else {
#ifdef AMOEBA
				if (errno != EIO)
#endif
				_wwarning(
				    "_wchecktimer: select failed, errno=%d",
				    errno);
			}
		}
		if (nfound != 0)
			break;
		if (dotimer(ep))
			return TRUE;
	}
	_wdebug(4, "_wchecktimer: returning FALSE");
	return FALSE;
}

/* P.S.: It is not necessary to recompute nextalarm when windows are
   deleted.  This can at most cause a spurious time-out, after which
   dotimeout() is called again which recomputes nextalarm as a side
   effect (twice, even).  Applications incur a slight overhead if they
   delete a window with a timer set and no other windows have timers
   set; in this case a larger part of the timeout code is called until
   the alarm goes off (which is then ignored!). */

#endif /* !AMOEBA */
