/* Display character set.
   usage: charset [-font font] [minchar [maxchar]] */

#include "tools.h"
#include "stdwin.h"

int minchar= 0;			/* First char to draw */
int maxchar= 255;

main(argc, argv)
	int argc;
	char **argv;
{
	int *which= &minchar;
	int i;
	
	winitargs(&argc, &argv);
	
	for (i= 1; i < argc; ++i) {
		if (isnumeric(argv[i])) {
			*which= atoi(argv[i]);
			which= &maxchar;
		}
		else
			break;
	}
	if (i < argc) {
		wdone();
		fprintf(stderr, "usage: %s [minchar [maxchar]]\n", argv[0]);
		exit(2);
	}
	
	if (maxchar < minchar) {
		wdone();
		fprintf(stderr, "%s: maxchar < minchar\n", argv[0]);
		exit(2);
	}
	
	setup();
	mainloop();
	wdone();
	exit(0);
}

bool
isnumeric(str)
	char *str;
{
	while (*str != EOS) {
		if (!isdigit(*str))
			return FALSE;
		++str;
	}
	return TRUE;
}

WINDOW *win;
int ncols, nrows;
int colwidth, rowheight;

void drawproc(win, l, t, r, b)
	WINDOW *win;
{
	int lineheight= wlineheight();
	int i= (t/rowheight) * ncols;
	for (; i <= maxchar-minchar; ++i) {
		int c= minchar+i;
		int col= i%ncols;
		int row= i/ncols;
		int h= col*colwidth;
		int v= row*rowheight;
		int charwidth= wcharwidth(c);
		if (v >= b)
			break;
		wdrawchar(h + (colwidth - charwidth) / 2,
			  v + (rowheight - lineheight) / 2,
			  c);
	}
}

setup()
{
	int scrwidth, scrheight;
	int maxwidth= 1;
	int i;
	
	wgetscrsize(&scrwidth, &scrheight);
	while (minchar <= maxchar && wcharwidth(minchar) == 0)
		++minchar;
	while (maxchar >= minchar && wcharwidth(maxchar) == 0)
		--maxchar;
	if (minchar > maxchar) {
		wdone();
		fprintf(stderr, "no chars in given range\n");
		exit(1);
	}
	for (i= minchar; i <= maxchar; i++) {
		int w= wcharwidth(i);
		if (w > maxwidth)
			maxwidth= w;
	}
	colwidth= maxwidth * 2;
	rowheight= wlineheight() * 3 / 2;
	ncols= 16;
	if ((ncols+2) * colwidth > scrwidth) {
		ncols= scrwidth/colwidth - 2;
		if (ncols < 1)
			ncols= 1;
	}
	nrows= (maxchar - minchar + ncols) / ncols;
	if (nrows < 1)
		nrows= 1;
	wsetdefwinsize(ncols*colwidth, nrows*rowheight);
	wsetmaxwinsize(ncols*colwidth, nrows*rowheight);
	win= wopen("Character Set", drawproc);
	if (win == NULL) {
		wdone();
		fprintf(stderr, "can't open window\n");
		exit(1);
	}
	(void) wmenucreate(1, "Press q to quit");
}

mainloop()
{
	for (;;) {
		EVENT e;
		wgetevent(&e);
		switch (e.type) {
		case WE_CHAR:
			switch (e.u.character) {
			case 'q':
			case 'Q':
				return;
			}
			break;
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CANCEL:
			case WC_CLOSE:
				wclose(win);
				return;
			}
			break;
		case WE_CLOSE:
			wclose(win);
			return;
		case WE_MOUSE_DOWN:
			charinfo(e.u.where.h, e.u.where.v);
			break;
		}
	}
}

charinfo(h, v)
	int h, v;
{
	int row= v/rowheight;
	int col= h/colwidth;
	int i= minchar + row*ncols + col;
	if (i >= minchar && i <= maxchar) {
		char buf[256];
		sprintf(buf, "Char '%c' (%d. 0x%x), width %d",
			i>=' ' && i<0x7f ? i : '?', i, i, wcharwidth(i));
		wmessage(buf);
	}
	else
		wfleep();
}
