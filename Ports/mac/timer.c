/* MAC STDWIN -- TIMER. */

/* XXX The timer mechanism should be coupled less strongly with the
   stdwin implementation -- stdwin should support a single timer,
   while thin layers on top could implement various timer strategies. */

#include "macwin.h"

static unsigned long nexttimer;

/* Set a window's timer.
   The second parameter is the number of deciseconds from now
   when the timer is to go off, or 0 to clear the timer.
   When the timer goes off (or sometime later),
   a WE_TIMER event will be delivered.
   As a service to 'checktimer' below, a NULL window parameter
   is ignored, but causes the 'nexttimer' value to be recalculated.
   (The inefficient simplicity of the code here is due to MINIX,
   with some inventions of my own.) */

void
wsettimer(win, deciseconds)
	WINDOW *win;
	int deciseconds;
{
	WindowPeek w;
	
	nexttimer= 0;
	if (win != NULL) {
		if (deciseconds == 0)
			win->timer= 0;
		else {
			nexttimer= win->timer= TickCount() +
				deciseconds*(TICKSPERSECOND/10);
		}
	}
	for (w= (WindowPeek)FrontWindow(); w != NULL; w= w->nextWindow) {
		win= whichwin((WindowPtr)w);
		if (win != NULL && win->timer != 0) {
			if (nexttimer == 0 || win->timer < nexttimer)
				nexttimer= win->timer;
		}
	}
}

/* Check if a timer went off.
   If so, return TRUE and set the event record correspondingly;
   otherwise, return FALSE. */

bool
checktimer(ep)
	EVENT *ep;
{
	WindowPtr w;
	WINDOW *win;
	unsigned long now;
	
	if (nexttimer == 0) {
		return FALSE;
	}
	now= TickCount();
	if (now < nexttimer) {
		return FALSE;
	}
	ep->type= WE_NULL;
	ep->window= NULL;
	for (w= FrontWindow(); w != NULL;
		w= (WindowPtr)((WindowPeek)w)->nextWindow) {
		win= whichwin(w);
		if (win != NULL && win->timer != 0) {
			if (win->timer <= now) {
				ep->type= WE_TIMER;
				ep->window= win;
				now= win->timer;
			}
		}
	}
	wsettimer(ep->window, 0);
	return ep->type == WE_TIMER;
}
