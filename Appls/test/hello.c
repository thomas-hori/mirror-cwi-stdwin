/* The "Hello, world" koan according to David Rosenthal,
   recoded for STDWIN.
   Requirements: print "Hello, world" centered in a window,
   recented after window resizes, redraw after window exposures.
   Check error returns. */

#include "stdwin.h"

char *string= "Hello, world";

int text_h, text_v;

placetext(win)
	WINDOW *win;
{
	int width, height;
	wgetwinsize(win, &width, &height);
	text_v= (height - wlineheight()) / 2;
	text_h= (width - wtextwidth(string, -1)) / 2;
	wchange(win, 0, 0, width, height);
}

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	wdrawtext(text_h, text_v, string, -1);
}

main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	winitargs(&argc, &argv);
	win= wopen("Hello", drawproc);
	
	if (win != 0) {
		placetext(win);
		for (;;) {
			EVENT e;
			wgetevent(&e);
			if (e.type == WE_CLOSE ||
					e.type == WE_COMMAND &&
					(e.u.command == WC_CLOSE ||
					 e.u.command == WC_CANCEL))
				break;
			if (e.type == WE_SIZE)
				placetext(win);
		}
	}
	
	wdone();
	exit(0);
}
