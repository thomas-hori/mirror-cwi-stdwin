/* Panic function, not if NDEBUG.  The application may provide a better one. */

#include "vtimpl.h"

#ifndef NDEBUG

void
vtpanic(msg)
	char *msg;
{
	fprintf(stderr, "vtpanic: %s\n", msg);
	wdone();
	abort();
}

#endif /* NDEBUG */
