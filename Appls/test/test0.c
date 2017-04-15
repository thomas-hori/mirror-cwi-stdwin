/* Minimal test -- just call winit() and wdone(). */

#include "stdwin.h"

main(argc, argv)
	int argc;
	char **argv;
{
	winitargs(&argc, &argv);
	wdone();
	exit(0);
}
