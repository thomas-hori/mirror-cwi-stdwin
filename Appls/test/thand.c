/* Test wungetevent from signal handler */

#include "stdwin.h"
#include <signal.h>

#define SPECIAL 1000

TEXTEDIT *tb;

void
drawproc(win, l, t, r, b)
	WINDOW *win;
{
	tedraw(tb);
}

int
handler() {
	EVENT e;
	e.type= WE_COMMAND;
	e.u.command= SPECIAL;
	wungetevent(&e);
}

main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	int width, height;
	
	winit();
	signal(SIGINT, handler);
	if (argc >= 3) {
		int h= atoi(argv[1]), v= atoi(argv[2]);
		wsetdefwinpos(h, v);
	}
	
	win= wopen("Textedit", drawproc);
	wgetwinsize(win, &width, &height);
	wsetdocsize(win, width, height);
	
	tb= tealloc(win, 0, 0, width);
	tereplace(tb, "Hello, world\n--Guido van Rossum");
	
	for (;;) {
		EVENT e;
		wgetevent(&e);
		if (e.type == WE_CLOSE)
			break;
		if (e.type == WE_COMMAND) {
			if (e.u.command == WC_CLOSE ||
				e.u.command == WC_CANCEL)
				break;
			else if (e.u.command == SPECIAL) {
				wmessage("Got event from handler");
				continue;
			}
		}
		(void) teevent(tb, &e);
	}
	tefree(tb);
	wclose(win);
	wdone();
	exit(0);
}
