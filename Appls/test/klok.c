/* Analog clock with alarm.
   
   Displays the date at the top, a circular clock in the middle,
   and the alarm time at the bottom of the window.
   The clock has two hands.
   
   Resizing the window recomputes the items' positions and sizes.
   
   When the alarm goes off, the clock window is made current,
   the clock face is inverted for 5 minutes, and a beep is emitted
   each minute.  The alarm can be acknowledged explicitly, which
   silences it until the next time the alarm goes off.
   
   Dragging the hands of the clock can be used to set the time
   (and the date, if you care to drag around several times).
   The alarm is currently set through a dialog only.

   TO DO:
   	- make the display prettier (how??? everything I design gets ugly :-( )
	- display alarm time as tick mark?
	- improve the alarm setting procedure
	- add more general 'nag' and 'calendar'-like facilities
	- add a button to allow/disallow setting the time
	- add a way to change the date directly
	- turn it into a subroutine package like VT or editwin
	- organize the code top-down instead of bottom-up
*/

#include "tools.h"
#include "stdwin.h"

#include <math.h>
#include <sys/time.h>

#ifndef PI
#define PI 3.1415962		/* ... */
#endif

#define ALARMTIME	5	/* Alarm goes for this many minutes */

#define LITPERC		60	/* Little hand size (percent of radius) */
#define BIGPERC		80	/* Big hand size */
#define SECPERC		100	/* Seconds hand size */

#define SETALARM	0
#define CLEARALARM	1
#define OKALARM		2
#define SECONDSHAND	4
#define QUIT		6

/* Global variables */
char *progname= "klok";		/* Program name (for error messages) */
WINDOW *win;			/* Clock window */
MENU *mp;			/* Menu pointer */
int centh, centv;		/* Clock center */
int radius;			/* Clock radius */
struct tm curtime;		/* Current time/date */
int alarm= 11*60 + 42;		/* Alarm time (hh*60 + mm); -1 if off */
bool alarmed;			/* Is it alarm time? */
bool okayed;			/* Has the current alarm been OK'ed? */
bool excited;			/* == (alarmed && !okayed) */
bool do_seconds;		/* Set if drawing 'seconds' hand */

/* Force a redraw of the entire window */

changeall()
{
	wchange(win, 0, 0, 10000, 10000);
}

/* Compute the sine of an angle given in clock units
   (zero at 12 o'clock, full circle is 60).
   We cache the sine values in a table,
   since calling sin is too slow on some systems. */

double
sine(i)
	int i;
{
	static double sines[15+1];
	static bool inited;
	
	if (!inited) {
		int k;
		inited= TRUE;
		for (k= 0; k <= 15; ++k)
			sines[k]= sin(k * PI/30);
	}
	i= i % 60;
	if (i < 0)
		i += 60;
	if (i <= 15)
		return sines[i];
	if (i <= 30)
		return sines[30-i];
	if (i <= 45)
		return -sines[i-30];
	return -sines[60-i];
}

/* Compute the cosine (from the sine) */

double
cosine(i)
	int i;
{
	return sine(i+15);
}

/* Compute the absolute position of the endpoint of a line drawn at
   i minutes, whose length is a certain percentage of the radius */

void
endpoint(i, perc, ph, pv)
	int i;		/* Minutes */
	int perc;	/* Percentage of length */
	int *ph, *pv;	/* Return values */
{
	double s= sine(i), c= cosine(i);
	
	*ph= centh + s*perc*radius/100 + 0.5;
	*pv= centv - c*perc*radius/100 + 0.5;
}

/* Draw a mark at i minutes.
   Marks at hour positions are longer, every 3 hours even longer. */

void
drawmark(i)
	int i;
{
	int begh, begv;
	int endh, endv;
	int len;
	
	endpoint(i, 100, &endh, &endv);
	if (i % 5 != 0)
		len= 3;
	else if (i % 15 != 0)
		len= 8;
	else
		len= 19;
	endpoint(i, 100-len, &begh, &begv);
	wdrawline(begh, begv, endh, endv);
}

/* Draw a hand at i minutes, whose length is a given percentage
   of the radius */

void
drawhand(i, perc)
	int i;
	int perc;
{
	int endh, endv;
	endpoint(i, perc, &endh, &endv);
	wdrawline(centh, centv, endh, endv);
}

/* Draw a hand in XOR mode */

void
xorhand(i, perc)
	int i;
	int perc;
{
	int endh, endv;
	endpoint(i, perc, &endh, &endv);
	wxorline(centh, centv, endh, endv);
}

/* Draw the date in the top left corner */

void
drawdate(tp)
	struct tm *tp;
{
	char buf[100];
	
	sprintf(buf, "%02d/%02d/%02d", tp->tm_year % 100,
		tp->tm_mon+1, tp->tm_mday);
	werase(0, 0, 10000, centv - radius);
	wdrawtext(0, centv - radius - wlineheight(), buf, -1);
}

/* Draw the alarm time in the bottom left corner */

void
drawalarm()
{
	char buf[100];
	
	sprintf(buf, "*%02d:%02d", alarm/60, alarm%60);
	wdrawtext(0, centv + radius, buf, -1);
}

/* Compute the AM/MP/Noon/Midnight indicator character */

int
ampm(tp)
	struct tm *tp;
{
	if (tp->tm_min == 0 && tp->tm_hour%12 == 0) {
		if (tp->tm_hour == 12)
			return 'N';
		else
			return 'M';
	}
	else if (tp->tm_hour < 12)
		return 'A';
	else
		return 'P';
}

/* Draw the AM/PM/Noon/Midnight indicator in the top right corner */

void
drawampm(c)
	int c;
{
	int dh= wcharwidth('M');
	int dv= wlineheight();
	int h= centh + radius - dh;
	int v= centv - radius - dv;
	
	werase(h, v, h+dh, v+dv);
	wdrawchar(h, v, c);
}

#ifdef UGLY

/* Draw a shaded square around the clock */

#define SHOFF 4

void
drawborder()
{
	int d= radius * 10/9;
	int left= centh-d, top= centv-d, right= centh+d, bottom= centv+d;
	wdrawbox(left, top, right, bottom);
	wshade(right, top+4, right+4, bottom+4, 50);
	wshade(left+4, bottom, right, bottom+4, 50);
}

/* Draw a shaded circle around the clock's face;
   the shadow is on the top left side, so the face appeares to
   be slightly *lower* than the surrounding material.
   Also a thin vertical line to indicate 6 and 12 o'clock. */

void
drawoutline()
{
	wdrawcircle(centh-1, centv-1, radius+2);
	wdrawelarc(centh-1, centv-1, radius+1, radius+1, 45-10, 45+180+10);
	wdrawelarc(centh-1, centv-1, radius  , radius  , 45+10, 45+180-10);
	wdrawline(centh, centv-radius, centh, centv+radius);
}

#endif /*UGLY*/

/* Compute the little hand position from hour, min */

int
littlehand(hour, min)
	int hour, min;
{
	return (hour*5 + (min+6)/12) % 60;
}

/* Draw procedure */

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	int i;
	
	/* Draw the fixed elements of the clock */
#ifdef UGLY
	drawborder();
	drawoutline();
#else
#ifdef macintosh
	wdrawcircle(centh+1, centv+1, radius+1);
#else
	wdrawcircle(centh, centv, radius);
#endif
	for (i= 0; i < 12; ++i)
		drawmark(i*5);			/* Hour marks */
#endif
	
	/* Draw the hands */
	drawhand(curtime.tm_min, BIGPERC);
	i= littlehand(curtime.tm_hour, curtime.tm_min);
	if (i != curtime.tm_min)
		xorhand(i, LITPERC);
	if (do_seconds)
		xorhand(curtime.tm_sec, SECPERC);
	
	/* Draw the other elements */
	drawdate(&curtime);
	drawampm(ampm(&curtime));
	if (alarm >= 0)
		drawalarm();
		
	/* Invert if the alarm is going */
	if (excited)
		winvert(0, 0, 10000, 10000);
}

/* Compute the nearest clock angle corresponding to
   absolute position (h, v) */

int
whereis(h, v)
	int h, v;
{
	double dnew;
	
	h -= centh;
	v -= centv;
	if (h == 0 && v == 0)
		return 0;
	dnew= atan2((double)h, (double)(-v)) * 30.0 / PI;
	if (dnew < 0)
		dnew += 60.0;
	return ((int)(dnew + 0.5)) % 60;
}

/* Show a change in time with minimal redrawing */

showchange(old, new)
	struct tm *old, *new;
{
	int litold= littlehand(old->tm_hour, old->tm_min);
	int litnew= littlehand(new->tm_hour, new->tm_min);
	int newampm= ampm(new);
	
	wbegindrawing(win);
	
	if (do_seconds && old->tm_sec != new->tm_sec) {
		xorhand(old->tm_sec, SECPERC);
		xorhand(new->tm_sec, SECPERC);
	}
	
	if (old->tm_min != new->tm_min) {
		xorhand(old->tm_min, BIGPERC);
		xorhand(new->tm_min, BIGPERC);
	}
	
	if (litold != litnew ||
		litold == old->tm_min || litnew == new->tm_min) {
		if (litold != old->tm_min)
			xorhand(litold, LITPERC);
		if (litnew != new->tm_min)
			xorhand(litnew, LITPERC);
	}
	
	if (old->tm_mday != new->tm_mday)
		drawdate(new);
	
	if (newampm != ampm(old))
		drawampm(newampm);
	
	wenddrawing(win);

}

/* Leap year calculation.  Input is year - 1900 (but may be >= 100). */

int
isleap(year)
	int year;
{
	year += 1900;
	
	return year%4 == 0 && (year%100 != 0 || year%400 == 0);
}

/* Increment a time variable in minutes, and show the change */

void
incrshowtime(tp, incr)
	struct tm *tp;
	int incr;
{
	struct tm old;
	static int mdays[12]=
		{31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	mdays[1]= 28 + isleap(tp->tm_year);
	
	old= *tp;
	
	tp->tm_min += incr;
	
	while (tp->tm_min >= 60) {
		tp->tm_min -= 60;
		tp->tm_hour++;
		if (tp->tm_hour >= 24) {
			tp->tm_hour -= 24;
			tp->tm_mday++;
			tp->tm_wday= (tp->tm_wday + 1) % 7;
			if (tp->tm_mday > mdays[tp->tm_mon]) {
				tp->tm_mday= 1;
				tp->tm_mon++;
				if (tp->tm_mon >= 12) {
					tp->tm_mon= 0;
					tp->tm_year++;
					mdays[1]= 28 + isleap(tp->tm_year);
				}
			}
		}
	}
	
	while (tp->tm_min < 0) {
		tp->tm_min += 60;
		tp->tm_hour--;
		if (tp->tm_hour < 0) {
			tp->tm_hour += 24;
			tp->tm_mday--;
			tp->tm_wday= (tp->tm_wday + 6) % 7;
			if (tp->tm_mday < 1) {
				tp->tm_mon--;
				if (tp->tm_mon < 0) {
					tp->tm_mon= 11;
					tp->tm_year--;
					mdays[1]= 28 + isleap(tp->tm_year);
				}
				tp->tm_mday= mdays[tp->tm_mon];
			}
		}
	}
	
	showchange(&old, tp);
}

/* Drag the little hand */

void
draglittlehand(h, v)
	int h, v;
{
	EVENT e;
	struct tm newtime;
	int i;
	
	newtime= curtime;
	wsettimer(win, 0);
	
	do {
		wgetevent(&e);
		if (e.type != WE_MOUSE_MOVE && e.type != WE_MOUSE_UP) {
			showchange(&newtime, &curtime);
			wungetevent(&e);
			return;
		}
		i= whereis(e.u.where.h, e.u.where.v) / 5;
		if ((i - newtime.tm_hour) % 12 != 0) {
			int diff= i - newtime.tm_hour;
			while (diff > 6)
				diff -= 12;
			while (diff < -6)
				diff += 12;
			incrshowtime(&newtime, diff*60);
		}
	} while (e.type != WE_MOUSE_UP);
	setdatetime(&newtime, FALSE);
	curtime= newtime;
}

/* Drag the big hand */

void
dragbighand(h, v)
	int h, v;
{
	EVENT e;
	struct tm newtime;
	int i;
	
	newtime= curtime;
	wsettimer(win, 0);
	
	do {
		wgetevent(&e);
		if (e.type != WE_MOUSE_MOVE && e.type != WE_MOUSE_UP) {
			showchange(&newtime, &curtime);
			wungetevent(&e);
			return;
		}
		i= whereis(e.u.where.h, e.u.where.v);
		if (i != newtime.tm_min) {
			int diff= i - newtime.tm_min;
			if (diff > 30)
				diff -= 60;
			else if (diff < -30)
				diff += 60;
			incrshowtime(&newtime, diff);
		}
	} while (e.type != WE_MOUSE_UP);
	setdatetime(&newtime, TRUE);
	curtime= newtime;
}

/* Test whether the given position lies on the hand at the
   given clock angle with the given length percentage */

bool
testhand(h, v, pos, perc)
	int h, v;
	int pos;
	int perc;
{
	long dist2= (h-centh)*(h-centh)+ (v-centv)*(v-centv);
	long length2= ((long)radius*perc/100) * ((long)radius*perc/100);
	
	if (dist2 > length2)
		return FALSE;
	if ((whereis(h, v) - pos) % 60 != 0)
		return FALSE;
	return TRUE;
}

/* Recompute the time and the alarm parameters.
   Called every minute, and when other parameters may have changed. */

void
newtime(flash)
	bool flash;
{
	struct tm oldtime;
	unsigned long now;
	bool wasalarmed;
	
	/* Save the old time displayed */
	oldtime= curtime;
	
	/* Get the current time */
	time(&now);
	curtime= *localtime(&now);
	
	/* Set the window timer to go off at the next tick */
	if (do_seconds) {
		wsettimer(win, 10);
	}
	else {
		if (curtime.tm_sec >= 59) {
			/* When we wake up just at the end of the minute,
			   (which may happen if STDWIN isn't very precise),
			   pretend it's a bit later, to avoid waking up
			   again in a second */
			curtime.tm_sec -= 60;
			curtime.tm_min += 1;
		}
		wsettimer(win, 10 * (60 - curtime.tm_sec));
	}
	
	/* Check whether the alarm should go off */
	wasalarmed= alarmed;
	if (!wasalarmed)
		okayed= FALSE;
	if (alarm >= 0) {
		int a= alarm;
		int hhmm= curtime.tm_hour*60 + curtime.tm_min;
		if (hhmm < 60 && a >= 23*60)
			hhmm += 24*60; /* Correct for wrap-around */
		alarmed= hhmm >= a && hhmm < a+ALARMTIME;
	}
	else {
		alarmed= okayed= FALSE;
	}
	excited= alarmed && !okayed;
	if (excited) {
		if (!wasalarmed)
			wsetactive(win);
		wfleep();
	}
	if (excited || wasalarmed && !okayed)
		flash= TRUE;
	wmenuenable(mp, OKALARM, excited);
	
	/* Redraw the clock face or schedule a redraw */
	if (flash) {
		changeall();
	}
	else {
		showchange(&oldtime, &curtime);
	}
}

/* Time-setting procedure by dragging the hands around */

void
changehand(h, v)
	int h, v;
{
	/* Test the little hand first, so that if the hands
	   overlap, a click near the center implies the little
	   hand and a click further away implies the big hand */
	if (testhand(h, v,
		littlehand(curtime.tm_hour, curtime.tm_min), LITPERC)) {
		/* Drag the little hand -- minutes stay unchanged */
		draglittlehand(h, v);
	}
	else if (testhand(h, v, curtime.tm_min, BIGPERC)) {
		/* Drag the big hand -- hours may change, too */
		dragbighand(h, v);
	}
	else {
		/* No hit -- make some noise */
		wfleep();
	}
	newtime(FALSE);
}

/* Recompute the clock size and position
   and the time/alarm information.
   Called initially and when the window is resized. */

void
getinfo()
{
	int width, height;
	
	wgetwinsize(win, &width, &height);
	centh= width/2;
	centv= height/2;
	radius= centv - wlineheight();
	CLIPMAX(radius, centh);
	newtime(TRUE);
}

/* Set the alarm time from a string formatted as hhmm */

bool
setalarm(str)
	char *str;
{
	int al;
	
	if (str[0] == EOS || str[0] == '-' && str[1] == EOS) {
		alarm= -1;
		wmenuenable(mp, CLEARALARM, FALSE);
		return TRUE;
	}
	al= atoi(str);
	if (al < 0 || al > 2400 || al%60 >= 60)
		return FALSE;
	if (al == 2400)
		al= 0;
	alarm= (al/100)*60 + al%100;
	wmenuenable(mp, CLEARALARM, TRUE);
	return TRUE;
}

/* Set up the menu */

void
buildmenu()
{
	wmenusetdeflocal(TRUE);
	mp= wmenucreate(1, "Klok");
	
	wmenuadditem(mp, "Set alarm...", 'S');
	wmenuadditem(mp, "Clear alarm", 'C');
	wmenuadditem(mp, "OK alarm", 'O');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Seconds Hand", 'H');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Quit", 'Q');
	
	wmenuenable(mp, CLEARALARM, alarm >= 0);
	wmenucheck(mp, SECONDSHAND, do_seconds);
}

/* Handle a menu selection */

void
domenu(item)
	int item;
{
	bool flash= FALSE;
	
	switch (item) {
	case SETALARM: {
		char buf[6];
		if (alarm < 0)
			buf[0]= EOS;
		else
			sprintf(buf, "%02d%02d", alarm/60, alarm%60);
		if (!waskstr("Set alarm:", buf, sizeof buf))
			return;
		if (!setalarm(buf))
			wmessage("Invalid alarm (must be hhmm)");
		okayed= FALSE;
		flash= TRUE;
		break;
		}
	case CLEARALARM:
		if (alarm >= 0) {
			setalarm("");
			flash= TRUE;
		}
		break;
	case OKALARM:
		if (excited) {
			flash= okayed= TRUE;
		}
		break;
	case SECONDSHAND:
		do_seconds= !do_seconds;
		wmenucheck(mp, SECONDSHAND, do_seconds);
		wbegindrawing(win);
		xorhand(curtime.tm_sec, SECPERC);
		wenddrawing(win);
		break;
	case QUIT:
		wclose(win);
		wdone();
		exit(0);
		break;
	}
	newtime(flash);
}

#ifndef macintosh
/* Print usage message and exit; called for command line errors */

void
usage()
{
	wdone();
	fprintf(stderr, "usage: %s [-s] [-m] [-a hhmm]\n", progname);
	exit(2);
}
#endif

/* Main program */

main(argc, argv)
	int argc;
	char **argv;
{
#ifdef macintosh
	do_seconds= TRUE;
#endif
	
	/* Initialize */
	winitnew(&argc, &argv);
	buildmenu(); /* Must be done before setalarm is called */
	
	/* Find out program name */
	if (argc > 0) {
		progname= rindex(argv[0], '/');
		if (progname == NULL)
			progname= argv[0];
		else
			++progname;
	}
	
#ifndef macintosh
	/* Parse command line */
	for (;;) {
		int c= getopt(argc, argv, "msa:");
		if (c == EOF)
			break;
		switch (c) {
		case 's':
			do_seconds= TRUE;
			break;
		case 'm':
			do_seconds= FALSE;
			break;
		case 'a':
			if (!setalarm(optarg))
				usage();
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	wmenucheck(mp, SECONDSHAND, do_seconds);
#endif
	
	/* Create the window */
	wsetmaxwinsize(400, 400);
	wsetdefwinsize(120, 120);
	win= wopen("klok", drawproc);
	wmenuattach(win, mp);
	
	/* Main loop */
	getinfo();
	for (;;) {
		EVENT e;
		wgetevent(&e);
		
		switch (e.type) {
		
		case WE_MOUSE_DOWN:
			if (excited)
				domenu(OKALARM);
			else
				changehand(e.u.where.h, e.u.where.v);
			break;
		
		case WE_MENU:
			if (e.u.m.id == 1)
				domenu(e.u.m.item);
			break;
		
		case WE_CHAR:
			if (excited)
				break;
			switch (e.u.character) {
			case 's':
			case 'S':
				domenu(SETALARM);
				break;
			case 'c':
			case 'C':
				domenu(CLEARALARM);
				break;
			case 'h':
			case 'H':
				domenu(SECONDSHAND);
				break;
			case 'q':
			case 'Q':
				domenu(QUIT);
				break;
			}
			break;
		
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_RETURN:
				newtime(FALSE);
				break;
			case WC_CLOSE:
			case WC_CANCEL:
				domenu(QUIT);
				break;
			}
			break;
		
		case WE_SIZE:
			getinfo();
			break;
		
		case WE_TIMER:
			newtime(FALSE);
			break;
		
		}
	}
}

#ifdef unix
#include "bsdsetdate.c"
#endif
