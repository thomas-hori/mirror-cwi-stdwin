#include <stdio.h>
#include "stdwin.h"

void drawproc(); /* Forward */

EVENT ev;

main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	
	winitargs(&argc, &argv);
	win = wopen("Test poll event", drawproc);
	for (;;) {
		if (wpollevent(&ev)) {
			if (ev.type == WE_CLOSE ||
			    ev.type == WE_COMMAND && ev.u.command == WC_CLOSE)
				break;
			wbegindrawing(win);
			werase(0, 0, 1000, 1000);
			drawproc(win, 0, 0, 1000, 1000);
			wenddrawing(win);
		}
		else {
			wbegindrawing(win);
			animate();
			wenddrawing(win);
		}
	}
	wdone();
	exit(0);
}

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	char buf[100];
	
	switch (ev.type) {
	
	case WE_MOUSE_DOWN:
	case WE_MOUSE_MOVE:
	case WE_MOUSE_UP:
		sprintf(buf,
			"MOUSE EVENT %d, h=%d, v=%d, button=%d, clicks=%d",
			ev.type, ev.u.where.h, ev.u.where.v,
			ev.u.where.button, ev.u.where.clicks);
		break;
	
	case WE_CHAR:
		sprintf(buf, "CHAR '%c' (%d)", ev.u.character, ev.u.character);
		break;
	
	default:
		sprintf(buf, "TYPE %d", ev.type);
		break;
	
	}
	wdrawtext(0, 0, buf, -1);
}

animate()
{
	static int h;
	int v = 20;
	
	werase(h, v, h+5, v+5);
	h = (h+1) % 400;
	wpaint(h, v, h+5, v+5);
}
