/* Menu test - a text-edit window  */

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
	MENU *mp;
	int width, height;
	int stop= 0;
	
	winitargs(&argc, &argv);
	if (argc >= 3) {
		int h= atoi(argv[1]), v= atoi(argv[2]);
		wsetdefwinpos(h, v);
	}
	
	mp= wmenucreate(1, "File");
	wmenuadditem(mp, "Quit", 'Q');
	
	win= wopen("Menu+textedit", drawproc);
	wgetwinsize(win, &width, &height);
	wsetdocsize(win, width, height);
	
	tb= tealloc(win, 0, 0, width);
	tereplace(tb, "Hello, world\n--Guido van Rossum");
	
	do {
		EVENT e;
		wgetevent(&e);
		if (teevent(tb, &e)) {
			wsetdocsize(win, width, height= tegetbottom(tb));
			continue;
		}
		switch (e.type) {
		
		case WE_COMMAND:
			if (e.u.command == WC_CLOSE ||
				e.u.command == WC_CANCEL)
				stop= 1;
			break;
		
		case WE_CLOSE:
			stop= 1;
			break;
		
		case WE_MENU:
			if (e.u.m.id == 1 && e.u.m.item == 0)
				stop= 1;
			break;
		
		}
	} while (!stop);

 	tefree(tb);
	wclose(win);
	wmenudelete(mp);
	wdone();
	
	exit(0);
}
