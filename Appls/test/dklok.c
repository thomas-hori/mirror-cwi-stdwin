/* Digital clock */

#ifdef ns32000
#define void int
#endif

#include <ctype.h>
#include <stdio.h>
#include <time.h>

#include "stdwin.h"

#include "sevenseg.h"

extern long time();

int text_only;				/* Non-zero on text-only device */

WINDOW *win;				/* Our window */

#define NIMAGE	5			/* Number of chars in image */
char image[NIMAGE+1];			/* Text to draw, plus trailing \0 */

int aleft, atop, aright, abottom;	/* Rect in which to draw */

getinfo()
{
	wgetwinsize(win, &aright, &abottom);
	wsetdocsize(win, aright, abottom);
	newtime();
}

newtime()
{
	unsigned long now;
	struct tm *tp;
	
	wchange(win, aleft, atop, aright, abottom);
	time(&now);
	tp= localtime(&now);
	wsettimer(win, 10 * (60 - now%60));
	sprintf(image, "%02d:%02d", tp->tm_hour, tp->tm_min);
	
}

void
drawdigit(digit, left, top, right, bottom, dh, dv)
	int digit;
	int left, top, right, bottom;
	int dh, dv; /* Segment thickness */
{
	int bits= sevenseg[digit];
	int mid= (top + bottom) / 2;
	void (*func)();
	
	if (text_only)
		func= winvert;
	else
		func= wpaint;
	
	if (bits & (1<<6))
		func(left, top, right, top+dv);
	if (bits & (1<<5))
		func(left, top, left+dh, mid);
	if (bits & (1<<4))
		func(right-dh, top, right, mid);
	if (bits & (1<<3))
		func(left, mid-dv/2, right, mid+dv-dv/2);
	if (bits & (1<<2))
		func(left, mid, left+dh, bottom);
	if (bits & (1<<1))
		func(right-dh, mid, right, bottom);
	if (bits & (1<<0))
		func(left, bottom-dv, right, bottom);
}

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	int width= aright - aleft;
	int height= abottom - atop;
	int dh= width / (NIMAGE*6 + 1);
	int dv= height / 9;
	int spacing;
	int i;
	
	if (dh < 1)
		dh= 1;
	if (dv < 1)
		dv= 1;
	
	spacing= dh*6;
	
	for (i= 0; i < NIMAGE && image[i] != '\0'; ++i) {
		if (isdigit(image[i])) {
			drawdigit(image[i] - '0',
				aleft + dh + i*spacing, atop + dv,
				aleft + dh + (i+1)*spacing - dh, abottom - dv,
				dh, dv);
		}
	}
}

main(argc, argv)
	int argc;
	char **argv;
{
	winitargs(&argc, &argv);
	setup();
	mainloop();
	wdone();
	exit(0);
}

setup()
{
	if (wlineheight() == 1) {
		text_only= 1;
		wmessage("Text-only version");
	}
	win= wopen("DIGITAL CLOCK", drawproc);
	getinfo();
}

mainloop()
{
	for (;;) {
		EVENT e;
		wgetevent(&e);
		switch (e.type) {
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
			case WC_CANCEL:
				wclose(win);
				return 0;
			}
			break;
		case WE_CLOSE:
			wclose(win);
			return 0;
		case WE_SIZE:
			getinfo();
			break;
		case WE_TIMER:
			newtime();
			break;
		}
	}
}
