/* Amoeba-specific timer code. */

#include "x11.h"

#include <amoeba.h>
#include <semaphore.h>

interval sys_milli();

/* Although win->timer and the second parameter of wsettimer() are in
   deciseconds, *all* other values are in milliseconds. */

static interval nexttimer;

/* Compute a new value for nexttimer.
   This is the system time (in msec.) when the first event should go
   off, or 0 if there are no scheduled timer events.
   Return the relevant window as a convenience. */

static WINDOW *
setnexttimer()
{
	WINDOW *win = _wnexttimer();
	
	if (win == NULL)
		nexttimer = 0;
	else
		nexttimer = win->timer * 100;
	_wdebug(4, "setnexttimer: nexttimer at %d for %x", nexttimer, win);
	return win;
}

/* Set the timer for a given window at now + deciseconds;
   or discard the timer if deciseconds is zero. */

void
wsettimer(win, deciseconds)
	WINDOW *win;
	int deciseconds;
{
	_wdebug(3, "wsettimer: deciseconds=%d", deciseconds);
	win->timer = 0;
	if (deciseconds > 0)
		win->timer = deciseconds + sys_milli() / 100;
	_wdebug(3, "wsettimer(%d): timer set to %d", deciseconds, win->timer);
	(void) setnexttimer();
}

/* Return an interval until the next timer, or -1 if no timers.
   This is suitable to pass directly to mu_trylock() etc. */

interval
nextdelay()
{
	interval now;
	
	if (nexttimer == 0)
		return -1; /* No timer events */
	now = sys_milli();
	if (nexttimer <= now)
		return 0; /* Immediate timer event */
	return nexttimer - now; /* Milliseconds until timer event */
}

/* Check if a timer has gone off, and if so, generate an appropriate event.
   This can be called at any time, but for efficiency reasons it should
   only be called when an alarm has actually gone off.
   If an alarm has gone off, it will always be found by this function. */

static bool
dotimer(ep)
	EVENT *ep;
{
	WINDOW *win;
	interval next;
	
	win = setnexttimer();
	if (win == NULL) {
		_wdebug(1, "dotimer: no event found (spurious call)");
		return FALSE;
	}
	next = nextdelay();
	if (next < 0) {
		/* Shouldn't happen -- setnexttimer should have been NULL */
		_wdebug(0, "dotimer: nextdelay() < 0 ???");
		return FALSE;
	}
	if (next == 0) {
		_wdebug(3, "dotimer: reporting timer event");
		ep->type = WE_TIMER;
		ep->window = win;
		win->timer = 0;
		(void) setnexttimer();
		return TRUE;
	}
	else {
		_wdebug(1, "dotimer: it is not yet time");
		return FALSE;
	}
}

/* Check for timer events, return TRUE if one found.
   If a timer event is due now, return it immediately.
   Otherwise, if mayblock is FALSE, return FALSE immediately.
   Otherwise, block until either an X event is available or the next
   timer event happens (in which case it is returned).
*/

bool
_w_checktimer(ep, mayblock)
	EVENT *ep;
	bool mayblock;
{
	for (;;) {
		interval delay;
		int status;
		
		delay = nextdelay();
		if (delay == 0) {
			if (dotimer(ep))
				return TRUE;
		}
		if (!mayblock)
			return FALSE;
		/* Block until next X event or next timer event */
		status = XamBlock(_wd, delay);
		_wdebug(4, "_w_checktimer: status %d", status);
		if (status > 0) {
			_wdebug(3, "_w_checktimer: external event, status %d",
								status);
			ep->type = WE_EXTERN;
			ep->window = NULL;
			ep->u.command = status;
			return TRUE;
		}
		if (status == 0) {
			_wdebug(3, "_w_checktimer: X event");
			return FALSE;
		}
		_wdebug(3, "_w_checktimer: timer went off");
		/* Negative return means timer went off
		   (or interrupted system call) */
	}
}

semaphore *_wsema; /* Global, used by winit() */

/* Interface to specify the semaphore used by XamBlock().
   *** This must be called before calling winit[new]() !!! ***
   */

void
wsetsema(ps)
	semaphore *ps;
{
	_wsema = ps;
}
