/* Typical test -- a window saying Hello, world. */

#include "stdwin.h"

void
drawproc(win, l, t, r, b)
	WINDOW *win;
{
	wdrawtext(0, 0, "Hello, world", -1);
}

main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	winitargs(&argc, &argv);
	win= wopen("Hello test", drawproc);
	for (;;) {
		EVENT e;
		wgetevent(&e);
		if (e.type == WE_CHAR && e.u.character == 'q')
			break;
		if (e.type == WE_CLOSE ||
			e.type == WE_COMMAND &&
			(e.u.command == WC_CLOSE || e.u.command == WC_CANCEL))
			break;
	}
	wclose(win);
	wdone();
	exit(0);
}
