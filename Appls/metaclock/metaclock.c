/* Metaclock -- generic clock drawing program, with 12-hour alarm */

#include <string.h>
#include <math.h>
#include <time.h>

#include "stdwin.h"
#include "metaclock.h"

#define ROUND(x) (floor((x) + 0.5))

#define PI 3.14159265359

typedef double matrix[3][3];

extern clockdef bbc_clock, hema_clock;

clockdef *clocks[] = {
	&bbc_clock,
	&hema_clock,
};

char *weekday[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *month[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int alarm_on = 0, alarm_ringing = 0, alarm_quiet = 0;
int alarm_minutes = 0;
int seconds_on = 1;

clockdef *cdef;

int clock_h, clock_v, clock_radius;
int text_h, text_v;
time_t last;
char the_time[100];
matrix background, hours, minutes, seconds, alarm;

/* Forward: */
WINDOW *newwin(clockdef *);
void fixsize(WINDOW *);
COLOR getpixel(char *color, int isfg);
void getpixels(handdef *);
void redraw(WINDOW *, time_t);
void getwintime(time_t);
void drawproc(WINDOW *, int, int, int, int);
void moveseconds(WINDOW *, time_t);
void getseconds(time_t);
void do_alarm(WINDOW *, int, int, int);
void change_alarm(int);
void getmatrix(matrix, double);
void drawhand(matrix, handdef *);
void transform(matrix, int, POINT[], POINT[]);

main(argc, argv)
	int argc;
	char **argv;
{
	int i, h, v;
	MENU *mp;
	WINDOW *win;
	
	winitargs(&argc, &argv);
	
	mp = wmenucreate(1, "Clocks");
	for (i = 0; i < COUNT(clocks); i++)
		wmenuadditem(mp, clocks[i]->cd_title, -1);
	
	wsetdefscrollbars(0, 0);
	wsetdefwinsize(200, 200+wlineheight());
	win = newwin(clocks[0]);
		
	for (;;) {
		EVENT e;
		time_t now;
		
		wgetevent(&e);
		if (e.type == WE_COMMAND && e.u.command == WC_CLOSE)
			break;
		switch (e.type) {
		case WE_MENU:
			wgetwinsize(win, &h, &v);
			wsetdefwinsize(h, v);
			wgetwinpos(win, &h, &v);
			wsetdefwinpos(h, v);
			wclose(win);
			win = newwin(clocks[e.u.m.item]);
			break;
		case WE_SIZE:
			fixsize(win);
			time(&last);
			redraw(win, last);
			wsettimer(win, 1);
			break;
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			do_alarm(win, e.type, e.u.where.h, e.u.where.v);
			break;
		case WE_TIMER:
			time(&now);
			if (now == last) {
				/* We're too early; synchronize */
				wsettimer(win, 1);
			}
			else {
				if (now/60 == last/60) {
					moveseconds(win, now);
					wsettimer(win, 8);
				}
				else {
					redraw(win, now);
					wsettimer(win, 1);
				}
				last = now;
			}
			break;
		case WE_COMMAND:
			if (e.u.command == WC_RETURN) {
				time(&last);
				redraw(win, last);
				wsettimer(win, 1);
			}
			break;
		}
	}
	wdone();
	exit(0);
}

WINDOW *newwin(cd)
	clockdef *cd;
{
	COLOR fg, bg;
	WINDOW *win;
	
	cdef = cd;
	
	fg = wgetfgcolor();
	bg = wgetbgcolor();
	
	wsetfgcolor(cdef->cd_fgpixel = getpixel(cdef->cd_fgcolor, 1));
	wsetbgcolor(cdef->cd_bgpixel = getpixel(cdef->cd_bgcolor, 0));
	
	win = wopen("Metaclock", drawproc);
	
	getpixels(&cdef->cd_background);
	getpixels(&cdef->cd_hours);
	getpixels(&cdef->cd_minutes);
	getpixels(&cdef->cd_seconds);
	getpixels(&cdef->cd_alarm);
	
	wsetfgcolor(fg);
	wsetbgcolor(bg);
	
	fixsize(win);
	
	time(&last);
	getwintime(last);
	wsettimer(win, 1);
	
	wsettitle(win, cdef->cd_title);
	
	return win;
}

void fixsize(win)
	WINDOW *win;
{
	int width, height;
	int x;
	
	wgetwinsize(win, &width, &height);
	height = height - wlineheight();
	if (width < height)
		x = width/2;
	else
		x = height/2;
	clock_h = width/2;
	clock_v = height/2;
	clock_radius = x*8/10;
	text_h = 0;
	text_v = height;
	getmatrix(background, 0.0);
}

void getpixels(hd)
	handdef *hd;
{
	int i;
	polydef *pd;
	
	for (i = hd->hd_npolys, pd = hd->hd_polys; --i >= 0; pd++) {
		pd->pd_fgpixel = getpixel(pd->pd_fgcolor, 1);
		pd->pd_bgpixel = getpixel(pd->pd_bgcolor, 0);
	}
}

COLOR getpixel(color, isfg)
	char *color;
	int isfg;
{
	if (color == NULL)
		return isfg ? wgetfgcolor() : wgetbgcolor();
	else
		return wfetchcolor(color);
}

void redraw(win, now)
	WINDOW *win;
	time_t now;
{
	getwintime(now);
	wchange(win, -10, -10, 10000, 10000);
}

void getwintime(now)
	time_t now;
{
	struct tm *t;
	double angle;
	char *p;
	
	t = localtime(&now);
	
	angle = 2.0 * PI * ((t->tm_hour%12) * 60.0 + t->tm_min) / 720.0;
	getmatrix(hours, angle);
	
	angle = 2.0 * PI * (t->tm_min) / 60.0;
	getmatrix(minutes, angle);
	
	getseconds(now);
	
	if (alarm_on) {
		int minutes = (t->tm_hour%12) * 60 + t->tm_min;
		minutes -= alarm_minutes;
		while (minutes < 0)
			minutes += 60*12;
		if (minutes < 5) {
			alarm_ringing = 1;
			if (!alarm_quiet)
				wfleep();
		}
		else
			alarm_ringing = alarm_quiet = 0;
		getmatrix(alarm, 2.0 * PI * alarm_minutes / (60.0 * 12.0));
	}

	sprintf(the_time, "%02d:%02d %s %2d-%s-%d",
		t->tm_hour, t->tm_min,
		weekday[t->tm_wday],
		t->tm_mday, month[t->tm_mon], 1900 + t->tm_year);
}

void drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	wdrawtext(text_h, text_v, the_time, -1);
	drawhand(background, &cdef->cd_background);
	drawhand(hours, &cdef->cd_hours);
	drawhand(minutes, &cdef->cd_minutes);
	if (seconds_on)
		drawhand(seconds, &cdef->cd_seconds);
	if (alarm_on)
		drawhand(alarm, &cdef->cd_alarm);
	if (alarm_ringing && !alarm_quiet)
		winvert(-10, -10, 10000, 10000); /* XXX Should be a handdef! */
}

void moveseconds(win, now)
	WINDOW *win;
	time_t now;
{
	wbegindrawing(win);
	drawhand(seconds, &cdef->cd_seconds);
	getseconds(now);
	drawhand(seconds, &cdef->cd_seconds);
	wenddrawing(win);
}

void getseconds(now)
	time_t now;
{
	double angle;
	
	angle = 2.0 * PI * (now%60) / 60.0;
	getmatrix(seconds, angle);
}

void do_alarm(win, type, h, v)
	WINDOW *win;
	int type;
	int h, v;
{
	double dh, dv;
	int distance;
	int new_minutes, new_on;
	char buf[100];
	char *str;
	
	dh = h - clock_h;
	dv = v - clock_v;
	distance = sqrt(dh*dh + dv*dv);
	new_on = (distance <= clock_radius * 6 / 5);
	if (new_on && distance >= clock_radius) {
		new_minutes = atan2(dh, -dv) * 60.0 * 12.0 / (2.0 * PI);
		while (new_minutes < 0)
			new_minutes += 60*12;
		new_minutes = (new_minutes + 3) / 5 * 5; /* Round */
	}
	else
		new_minutes = alarm_minutes;
	if (new_on && type != WE_MOUSE_UP) {
		sprintf(buf, "Alarm: %02d:%02d",
			new_minutes/60, new_minutes%60);
		str = buf;
	}
	else
		str = the_time;
	if (new_on != alarm_on || alarm_ringing ||
			new_on && new_minutes != alarm_minutes ||
			type != WE_MOUSE_MOVE) {
		COLOR fg;
		wbegindrawing(win);
		if (alarm_on && alarm_ringing && !alarm_quiet) {
			alarm_quiet = 1;
			winvert(-10, -10, 10000, 10000);
		}
		fg = wgetfgcolor();
		change_alarm(0);
		alarm_minutes = new_minutes;
		change_alarm(new_on);
		/*wsetfgcolor(fg);XXX*/
		werase(text_h, text_v, text_h + 1000, text_v + wlineheight());
		wdrawtext(text_h, text_v, str, -1);
		wenddrawing(win);
	}
	
	if (!alarm_on)
		alarm_ringing = alarm_quiet = 0;
}

void change_alarm(new_on)
	int new_on;
{
	if (new_on != alarm_on) {
		alarm_on = new_on;
		if (alarm_on)
			getmatrix(alarm,
				2.0 * PI * alarm_minutes / (60.0 * 12.0));
		drawhand(alarm, &cdef->cd_alarm);
	}
}

void drawhand(m, hd)
	matrix m;
	handdef *hd;
{
	COLOR fg, bg;
	POINT tmp[MAXPOINTS];
	int i;
	polydef *pd;
	
	fg = wgetfgcolor();
	bg = wgetbgcolor();
	
	for (i = hd->hd_npolys, pd = hd->hd_polys; --i >= 0; pd++) {
		transform(m, pd->pd_npoints, pd->pd_points, tmp);
		wsetfgcolor(pd->pd_fgpixel);
		wsetbgcolor(pd->pd_bgpixel);
		if (pd->pd_func == NULL)
			wxorline(tmp[0].h, tmp[0].v, tmp[1].h, tmp[1].v);
		else
			(*pd->pd_func)(pd->pd_npoints, tmp);
	}
	
	wsetfgcolor(fg);
	wsetbgcolor(bg);
}

void transform(m, n, inpoints, outpoints)
	matrix m;
	int n;
	POINT inpoints[];
	POINT outpoints[];
{
	register int i;
	register int x, y;
	
	for (i = 0; i < n; i++) {
		x = inpoints[i].h;
		y = inpoints[i].v;
		outpoints[i].h = ROUND(m[0][0]*x + m[0][1]*y + m[0][2]);
		outpoints[i].v = ROUND(m[1][0]*x + m[1][1]*y + m[1][2]);
	}
}

void getmatrix(m, angle)
	matrix m;
	double angle; /* 0...2pi, clockwise (of course... :-) */
{
	double f = (double)clock_radius / SCALE;
	double s = f*sin(angle);
	double c = f*cos(angle);
	
	m[0][0] = c;
	m[1][0] = s;
	m[2][0] = 0.0;
	
	m[0][1] = s;
	m[1][1] = -c;
	m[2][1] = 0.0;
	
	m[0][2] = (double)clock_h;
	m[1][2] = (double)clock_v;
	m[2][2] = 1.0;
}
