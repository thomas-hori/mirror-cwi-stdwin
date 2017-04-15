#include "stdwin.h"

#include <strings.h>

main(argc, argv)
	char **argv;
{
	winit(&argc, &argv);
	showargs(argc, argv);
	localtest();
	wdone();
}

showargs(argc, argv)
	char **argv;
{
	char buf[100];
	char *p;
	int i;
	
	sprintf(buf, "argc=%d", argc);
	p = buf;
	for (i = 0; i < argc; ++i) {
		p = strchr(p, '\0');
		sprintf(p, " argv[%d]=\"%s\"", i, argv[i]);
	}
	wmessage(buf);
}

localtest()
{
	WINDOW *u, *v, *w;
	MENU *m, *n, *o;
	int wincount = 3;
	
	wmenusetdeflocal(1);
	
	m = wmenucreate(1, "Menu1");
	wmenuadditem(m, "Item1", -1);
	n = wmenucreate(2, "Menu2");
	wmenuadditem(n, "Item2", -1);
	o = wmenucreate(3, "Menu3");
	wmenuadditem(o, "Item3", -1);
	
	u = wopen("Win1", 0L);
	v = wopen("Win2", 0L);
	w = wopen("Win3", 0L);
	
	wmenuattach(u, m);
	wmenuattach(v, n);
	wmenuattach(w, o);
	
	for (;;) {
		EVENT e;
		wgetevent(&e);
		if (e.type == WE_CLOSE ||
			e.type == WE_COMMAND && e.u.command == WC_CLOSE) {
			wclose(e.window);
			--wincount;
			if (wincount == 0)
				break;
		}
	}
}
