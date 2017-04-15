/* Basic test -- open and close a window; no output or events. */

#include "stdwin.h"

main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	winitargs(&argc, &argv);
	win= wopen("Basic test", (void (*)()) 0);
	wclose(win);
	wdone();
	exit(0);
}
