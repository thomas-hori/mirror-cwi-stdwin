/* Debugging test -- print events and details */

#include "stdwin.h"

void
drawproc(win, l, t, r, b)
	WINDOW *win;
{
	printf("drawproc called\n");
	wdrawtext(0, 0, "Hello, world", -1);
}

char *evname[] = {
	"null",
	"activate",
	"char",
	"command",
	"mouse_down",
	"mouse_move",
	"mouse_up",
	"menu",
	"size",
	"(move)",
	"draw",
	"timer",
	"deactivate",
	0
};

char *cmname[] = {
	"(null)",
	"close",
	"left",
	"right",
	"up",
	"down",
	"cancel",
	"backspace",
	"tab",
	"return",
	0
};

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
		switch (e.type) {
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			printf("%s event: b=%d, h=%d, v=%d\n",
				evname[e.type], e.u.where.button,
				e.u.where.h, e.u.where.v);
			break;
		case WE_COMMAND:
			printf("command event (%s)\n", cmname[e.u.command]);
			break;
		case WE_CHAR:
			printf("char event ('%c', 0x%02x)\n",
				e.u.character, e.u.character & 0xff);
			break;
		default:
			printf("%s event\n", evname[e.type]);
			break;
		}
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
