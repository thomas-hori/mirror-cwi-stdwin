/* Text-edit test -- a text-edit window. */

#include "stdwin.h"

TEXTEDIT *tb;

void
drawproc(win, l, t, r, b)
	WINDOW *win;
{
	tedraw(tb);
}

main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	int width, height;
	
	winitargs(&argc, &argv);
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
		if (e.type == WE_CLOSE || e.type == WE_COMMAND &&
			(e.u.command == WC_CLOSE || e.u.command == WC_CANCEL))
			break;
		(void) teevent(tb, &e);
	}
	tefree(tb);
	wclose(win);
	wdone();
	exit(0);
}
