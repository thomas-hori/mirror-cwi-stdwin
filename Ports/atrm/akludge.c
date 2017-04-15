/* Add a few functions that should be in Amoeba's library
   but aren't. */

#include <stdio.h>

abort()
{
	fprintf(stderr, "abort() called\n");
	suicide();
}

_exit(n)
	int n;
{
	fprintf(stderr, "_exit(%d) called\n", n);
	suicide();
}
