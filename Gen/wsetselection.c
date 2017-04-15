/* Dummy Selection Interface (always fails) */

#include "stdwin.h"
#include "tools.h"

/*ARGSUSED*/
int
wsetselection(win, sel, data, len)
	WINDOW *win;
	int sel;
	char *data;
	int len;
{
	return 0;
}

/*ARGSUSED*/
char *
wgetselection(sel, len_return)
	int sel;
	int *len_return;
{
	return NULL;
}

/*ARGSUSED*/
void
wresetselection(sel)
	int sel;
{
}
