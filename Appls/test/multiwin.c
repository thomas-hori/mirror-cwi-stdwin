/* Multiple windows, menus */

#include "stdwin.h"

#define NW 10			/* Max # of windows */

struct {
	WINDOW *win;
	TEXTEDIT *tb;
	int bottom;
} wlist[NW];			/* Window list */

drawproc(win, l, tl, r, b)
	WINDOW *win;
{
	int i= wgettag(win);
	tedraw(wlist[i].tb);
}

newwin()
{
	int i;
	
	for (i= 0; i < NW; ++i) {
		if (wlist[i].win == 0) {
			char title[20];
			int width, height;
			WINDOW *win;
			sprintf(title, "Untitled-%d", i);
			wlist[i].win= win= wopen(title, drawproc);
			wsettag(win, i);
			wgetwinsize(win, &width, &height);
			wlist[i].tb= tealloc(win, 0, 0, width);
			wlist[i].bottom= tegetbottom(wlist[i].tb);
			wsetdocsize(win, width, wlist[i].bottom);
			return;
		}
	}
	
	wmessage("Can't open another window");
}

closewin(win)
	WINDOW *win;
{
	int i= wgettag(win);
	tefree(wlist[i].tb);
	wclose(wlist[i].win);
	wlist[i].win= 0;
}

main(argc, argv)
	int argc;
	char **argv;
{
	MENU *mp;
	int inew, iquit;
	int stop= 0;
	
	winitargs(&argc, &argv);
	
	mp= wmenucreate(1, "File");
	inew= wmenuadditem(mp, "New", 'N');
	(void) wmenuadditem(mp, "", -1);
	iquit= wmenuadditem(mp, "Quit", 'Q');
	
	newwin();		/* Initial window */
	
	while (!stop) {
		EVENT e;
		
		wgetevent(&e);
		
		if (e.window != 0) {
			int i= wgettag(e.window);
			if (teevent(wlist[i].tb, &e)) {
				/*if (tegetbottom(wlist[i].tb) !=
						wlist[i].bottom)*/
				wsetdocsize(wlist[i].win,
					tegetright(wlist[i].tb),
					tegetbottom(wlist[i].tb));
				continue;
			}
		}
		
		switch (e.type) {
		
		case WE_MENU:
			switch (e.u.m.id) {
			case 1:
				if (e.u.m.item == inew)
					newwin();
				else if (e.u.m.item == iquit)
					stop= 1;
				break;
			}
			break;
		
		case WE_COMMAND:
			switch (e.u.command) {
			
			case WC_CLOSE:
				closewin(e.window);
				break;
			
			}
			break;
		
		case WE_CLOSE:
			closewin(e.window);
			break;

		case WE_SIZE:
			{
				int i= wgettag(e.window);
				int width, height;
				wgetwinsize(e.window, &width, &height);
				temove(wlist[i].tb, 0, 0, width);
				wlist[i].bottom= tegetbottom(wlist[i].tb);
				wsetdocsize(wlist[i].win,
					width, wlist[i].bottom);
			}
			break;
		}
	}
	
	wdone();
	exit(0);
}

