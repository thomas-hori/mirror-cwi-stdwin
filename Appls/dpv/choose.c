/* Application to choose ditroff special character names
   for glyphs in a given font.
   Usage: choose [-f  glyphfont] [-s size] [-c columns]
   Mac defaults: -f Symbol -s 24 -c 8
   X defaults: -f '*-symbol-*--24-*' -c 8

   TO DO:
   	- start with font choosing dialog on Mac
	- more object-like file interface (pretend we're editing
	  a table object)
	- more syntax checking on input
	- check for duplicate names?
*/

#define CHARWIDTHBUG		/* wcharwidth(i) == 0 for all i >= 128 */

#include <stdwin.h>
#include <tools.h>

/* Number of possible characters per font -- limited by char = 8 bit */
#define NGLYPHS 256

/* Table of names for each char -- 3rd is terminating zero */
char namelist[NGLYPHS][3];

/* Random global variables */
char *progname= "choose";	/* Program name for error messages */
WINDOW *win;			/* Where it all happens */
int selected= -1;		/* Glyph currently selected, -1 if none */
bool changed;			/* Set if any changes made */
char *filename;			/* Namelist file name */

/* Variables controlling the window lay-out */
char *glyphfont, *namefont;	/* Fonts used for ditto */
int glyphsize, namesize;	/* Point sizes used for glyph and name */
int firstglyph, lastglyph;	/* First and last glyphs */
int ncols, nrows;		/* Matrix dimensions */
int colwidth, rowheight;	/* Cell dimensions */

/* Parse the command line */

parse(argc, argv)
	int argc;
	char **argv;
{
	if (argc > 0 && argv[0] != NULL && argv[0][0] != EOS) {
		progname= rindex(argv[0], '/');
		if (progname == NULL)
			progname= argv[0];
		else
			progname++;
	}
	
	for (;;) {
		int c= getopt(argc, argv, "c:f:s:");
		if (c == EOF)
			break;
		switch (c) {
		case 'c':
			ncols= atoi(optarg);
			break;
		case 'f':
			glyphfont= optarg;
			break;
		case 's':
			glyphsize= atoi(optarg);
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	
	if (optind < argc)
		filename= argv[optind++];
	
	if (optind < argc)
		usage();
}

/* Print usage message and exit */

usage()
{
	wdone();
	fprintf(stderr, "usage: %s [-c columns] [-f font] [-s size]\n",
		progname);
	exit(2);
}

/* Initialize the control variables */

setup()
{
	/* Fill in defaults */
	if (ncols <= 0)
		ncols= 8;
	if (glyphfont == NULL || *glyphfont == EOS) {
#ifdef macintosh
		glyphfont= "Symbol";
#else
#ifdef X11R2
		glyphfont= "symbol22";
#else
		glyphfont= "*-symbol-*--24-*";
#endif
#endif
	}
	
	if (glyphsize <= 0)
		glyphsize= 24;
	if (namefont == NULL || *namefont == EOS) {
#ifdef machintosh
		namefont= "Courier";
#else
#ifdef X11R2
		namefont= "courier12f";
#else
		namefont= "*-courier-*--12-*";
#endif
#endif
	}
	if (namesize <= 0)
		namesize= 10;
	
	/* Find first and last existing character */
	firstglyph= 0;
	lastglyph= NGLYPHS-1;
	wsetfont(glyphfont);
	wsetsize(glyphsize);
	while (firstglyph < lastglyph && wcharwidth(firstglyph) == 0)
		++firstglyph;
	firstglyph= (firstglyph/ncols) * ncols;
#ifndef CHARWIDTHBUG
	while (lastglyph > firstglyph && wcharwidth(lastglyph) == 0)
		--lastglyph;
	lastglyph= (lastglyph/ncols + 1) * ncols - 1;
#endif
	
	/* Compute remaining variables */
	nrows= (lastglyph - firstglyph + ncols) / ncols;
	colwidth= 2*wcharwidth('M');
	rowheight= wlineheight();
	wsetfont(namefont);
	wsetsize(namesize);
	rowheight += 4 + wlineheight();
	{
		int cw= wtextwidth("MM  MM", -1) + 4;
		if (colwidth < cw)
			colwidth= cw;
	}
}

/* Draw procedure */

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	int i;
	
	/* Draw vertical grid lines */
	for (i= 1; i < ncols; ++i)
		wdrawline(i*colwidth-1, 0, i*colwidth-1, nrows*rowheight);
	
	/* Draw horizontal grid lines */
	for (i= 1; i < nrows; ++i)
		wdrawline(0, i*rowheight-1, ncols*colwidth, i*rowheight-1);
	
	/* Draw glyph cells */
	for (i= firstglyph; i <= lastglyph; ++i) {
		int h, v, h2, v2;
		int glyphwidth;
		cellbounds(i, &h, &v, &h2, &v2);
		if (v >= bottom)
			break;
		if (!intersects(h, v, h2, v2, left, top, right, bottom))
			continue;
		wsetfont(glyphfont);
		wsetsize(glyphsize);
		glyphwidth= wcharwidth(i);
#ifndef CHARWIDTHBUG
		if (glyphwidth == 0)
			continue;
#endif
		wdrawchar(h + (colwidth-glyphwidth) / 2, v+2, i);
		wsetfont(namefont);
		wsetsize(namesize);
		{
			char buf[10];
			sprintf(buf, "%02X", i);
			wdrawtext(h+2, v2 - 2 - wlineheight(), buf, -1);
		}
		if (namelist[i][0] != EOS) {
			int namewidth;
			namewidth= wtextwidth(namelist[i], -1);
			wdrawtext(h + colwidth - namewidth - 2,
				v2 - 2 - wlineheight(),
				namelist[i], -1);
		}
		if (i == selected)
			winvert(h+1, v+1, h2-2, v2-2);
	}
}
	
/* Main program */

main(argc, argv)
	int argc;
	char **argv;
{
	winitargs(&argc, &argv);
	parse(argc, argv);
	setup();
	readnamelist();
	wsetdefwinsize(colwidth*ncols, 0);
	win= wopen(glyphfont, drawproc);
	if (win == NULL) {
		wdone();
		fprintf(stderr, "%s: can't create window\n", progname);
	}
	wsetdocsize(win, colwidth*ncols, rowheight*nrows);
	eventloop();
	/*NOTREACHED*/
}

/* Event loop.  Never returns. */

eventloop() {
	for (;;) {
		EVENT e;
		wgetevent(&e);
		switch (e.type) {
		
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			do_select(&e);
			break;
		
		case WE_CHAR:
			do_char(e.u.character);
			break;
		
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
			case WC_CANCEL:
		close_it:
				if (changed) {
					int ok;
					ok= waskync("Save changes?", 1);
					if (ok > 0) {
						if (!writenamelist())
							ok= -1;
					}
					if (ok < 0)
						continue;
				}
				wclose(win);
				wdone();
				exit(0);
				/*NOTREACHED*/
			case WC_BACKSPACE:
				do_char('\b');
				break;
			}
			break;
		
		case WE_CLOSE:
			goto close_it;

		}
	}
	/*NOTREACHED*/
}

/* Handle mouse events */

do_select(ep)
	EVENT *ep;
{
	int left, top, right, bottom;
	int col= ep->u.where.h / colwidth;
	int row= ep->u.where.v / rowheight;
	int i= firstglyph + col + ncols*row;
	wsetfont(glyphfont);
	wsetsize(glyphsize);
	if (ep->u.where.h < 0 || ep->u.where.v < 0 ||
		col >= ncols || row >= nrows ||
		i < firstglyph || i > lastglyph ||
		wcharwidth(i) == 0)
		i= -1;
	if (i != selected) {
		wbegindrawing(win);
		if (selected >= 0) {
			cellbounds(selected, &left, &top, &right, &bottom);
			winvert(left+1, top+1, right-2, bottom-2);
		}
		selected= i;
		if (selected >= 0) {
			cellbounds(selected, &left, &top, &right, &bottom);
			winvert(left+1, top+1, right-2, bottom-2);
		}
		wenddrawing(win);
	}
	/* Must do this here because wshow may have no effect
	   while the mouse is down. */
	if (selected >= 0) {
		cellbounds(selected, &left, &top, &right, &bottom);
		wshow(win, left, top, right, bottom);
	}
		
}

/* Handle character events and backspace */

do_char(c)
	int c;
{
	int n;
	int left, top, right, bottom;
	if (selected < 0) {
		wfleep();
		return;
	}
	if (c == '\b') {
		n= 0;
	}
	else {
		n= strlen(namelist[selected]) % 2;
		namelist[selected][n++]= c;
	}
	namelist[selected][n]= EOS;
	wsetfont(namefont);
	wsetsize(namesize);
	cellbounds(selected, &left, &top, &right, &bottom);
	wshow(win, left, top, right, bottom);
	wchange(win,
		right - 3*wcharwidth('m'), bottom - 2 - wlineheight(),
		right-2, bottom-2);
	changed= TRUE;
}

/* Subroutine to find a glyph's cell */

cellbounds(i, pleft, ptop, pright, pbottom)
	int i;
	int *pleft, *ptop, *pright, *pbottom;
{
	int row= (i - firstglyph) / ncols;
	int col= (i - firstglyph) % ncols;
	*pleft= col*colwidth;
	*pright= *pleft + colwidth;
	*ptop= row * rowheight;
	*pbottom= *ptop + rowheight;
}

/* Predicate for rectangle intersection */

bool
intersects(l1, t1, r1, b1, l2, t2, r2, b2)
{
	if (l1 >= r2 || r1 <= l2)
		return FALSE;
	if (t1 >= b2 || b1 <= t2)
		return FALSE;
	return TRUE;
}

/* Read the namelist */

bool
readnamelist()
{
	FILE *fp;
	char buf[256];
	if (filename == NULL) {
		buf[0]= EOS;
		if (!waskfile("Read file:", buf, sizeof buf, FALSE))
			return FALSE;
		filename= strdup(buf);
	}
	fp= fopen(filename, "r");
	if (fp == NULL) {
		char buf[256];
		sprintf(buf, "Can't read file %s", filename);
		wmessage(buf);
		return FALSE;
	}
	while (fgets(buf, sizeof buf, fp) != NULL) {
		int glyph;
		char name[256];
		if (sscanf(buf, "%s - 0x%x", name, &glyph) == 2 ||
			sscanf(buf, "%s 0x%x", name, &glyph) == 2) {
			if (glyph >= 0 && glyph < NGLYPHS)
				strncpy(namelist[glyph], name, 2);
		}
	}
	fclose(fp);
}

/* Write the namelist */

bool
writenamelist()
{
	char name[256];
	FILE *fp;
	int i;
	if (filename == NULL)
		name[0]= EOS;
	else
		strcpy(name, filename);
	if (!waskfile("Write to file:", name, sizeof name, TRUE))
		return FALSE;
	filename= strdup(name);
	fp= fopen(filename, "w");
	if (fp == NULL) {
		sprintf(name, "Can't create file %s", filename);
		wmessage(name);
		filename= NULL;
		return FALSE;
	}
	for (i= 0; i < NGLYPHS; ++i) {
		if (namelist[i][0] != EOS)
			fprintf(fp, "%-2s - 0x%02X\n", namelist[i], i);
	}
	fclose(fp);
	changed= FALSE;
	return TRUE;
}
