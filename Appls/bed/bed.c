#include <stdio.h>
#include <ctype.h>
#include "bed.h"
#include "menu.h"

#define MINSQRSIZE	5
#define MAXSQRSIZE	15

MENU	*pfmenu ;
MENU	*pmmenu ;
MENU	*popmenu ;

int	sqrsize ;

int	map_width = DEF_COLS ;
int	map_height = DEF_ROWS ;

char	*raster = NULL ;
int	raster_lenght ;
int	stride ;

int	lastbyte ;
char	bits[] = { 0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F } ;

char	*title = NULL ;
char	*fname = NULL ;

WINDOW	*win ;

bool	changed = FALSE ;

bool	text_only = FALSE ;

extern bool	drawline ;

extern bool	drawcircle ;

extern bool	selrect ;
extern int	sr_left ;
extern int	sr_top ;
extern int	sr_right ;
extern int	sr_bottom ;

extern void	invertbit () ;

void
drawproc (win, left, top, right, bottom)
	WINDOW	*win ;
	int	left ;
	int	top ;
	int	right ;
	int	bottom ;
{
	int 	row ;
	register int 	col ;
	register int	l ;
	register int	r ;
	int	t ;
	int	b ;
	int	rend ;
	int fcol, fbyte, fbit, flr ;

	if (left < 0) left = 0 ;
	if (right > map_width * sqrsize) right = map_width * sqrsize ;
	if (top < 0) top = 0 ;
	if (bottom > map_height * sqrsize ) bottom = map_height * sqrsize ;

	rend = (right+sqrsize-1) / sqrsize ;

	fcol = left / sqrsize ;
	flr =  fcol * sqrsize ;
	fbyte = fcol / 8 ;
	fbit = fcol % 8 ;

	for (row = top / sqrsize, t = row * sqrsize, b = t + sqrsize ;
	     t < bottom ;
	     ++row, t += sqrsize, b = t + sqrsize) {

		register char	*byte ;
		register int	pbit ;

		col	= fcol ;
		l = r	= flr ;
		byte	= raster + (row * stride) + fbyte ;
		pbit	= fbit ;

		while (col++ < rend) {
			if (*byte & (1 << pbit)) {
				r += sqrsize ;
			}
			else {
				if (l < r) {
					wpaint(l, t, r, b);
					l = r ;
				}
				l += sqrsize ;
				r = l ;
			}

			if (++pbit >= 8) {
				pbit = 0 ;
				++byte ;
			}
		}

		if (l < r) wpaint (l, t, r, b) ;
	}

	if (drawline) plotline (invertbit, TRUE) ;
	else if (drawcircle) plotline (invertbit, TRUE) ;
	else if (selrect)
		wxorrect (sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;
}

void
newraster ()
{
	int	i ;

	if (raster != NULL) {
		free (raster) ;
		raster = NULL ;
	}

	lastbyte = map_width % 8 ;
	stride = (map_width + 7) / 8 ;
	raster_lenght = stride * map_height ;

	raster = (char *) malloc (raster_lenght) ;
	if (raster == NULL) {
		wdone () ;
		fprintf (stderr, "cannot get enough memory\n") ;
		exit (1) ;
	}

	for (i = 0 ; i < raster_lenght ; ++i)
		raster[i] = 0 ;
}

void
setsqrsize ()
{	
	if (wlineheight () == 1)
		text_only = sqrsize = 1 ;
	else {
		sqrsize = 1 ;
	/*	int	width ;
		int	height ;
		int	swidth ;
		int	sheight ;

		wgetscrsize (&width, &height) ;

		swidth = (2 * width) / (3 * map_width) ;
		sheight = (2 * height) / (3 * map_height) ;
		sqrsize = swidth < sheight ? swidth : sheight ;
		if (sqrsize < MINSQRSIZE)
			sqrsize = MINSQRSIZE ;
		else if (sqrsize > MAXSQRSIZE)
			sqrsize = MAXSQRSIZE ;
	*/
	}
}

void
setup()
{
	int	i ;

	setsqrsize () ;

	wsetdefwinsize (map_width * sqrsize, map_height * sqrsize) ;
	
	win = wopen (title == NULL ? "Untitled" : title, drawproc) ;
	if (win == NULL) {
		wmessage ("Can't open window") ;
		return ;
	}

	wsetdocsize (win, map_width * sqrsize, map_height * sqrsize) ;
	
	pfmenu = wmenucreate(FILE_MENU, "File"); 
	wmenuadditem (pfmenu, "Open", 'O') ;
	wmenuadditem (pfmenu, "New", 'N') ;
	wmenuadditem (pfmenu, "", -1) ;
	wmenuadditem (pfmenu, "Save", 'S') ;
	wmenuadditem (pfmenu, "Save As ...", -1) ;
	wmenuadditem (pfmenu, "", -1) ;
	wmenuadditem (pfmenu, "Quit", 'Q') ;

	pmmenu = wmenucreate (MODE_MENU, "Mode") ;
	wmenuadditem (pmmenu, "Pencil", -1) ;
	wmenuadditem (pmmenu, "Line", -1) ;
	wmenuadditem (pmmenu, "Circle", -1) ;
	wmenuadditem (pmmenu, "Select", -1) ;

	popmenu = wmenucreate(OP_MENU, "Operations") ;
	wmenuadditem (popmenu, "Clear bits", -1) ;
	wmenuadditem (popmenu, "Set bits", -1) ;
	wmenuadditem (popmenu, "Invert bits", -1) ;
	wmenuadditem (popmenu, "", -1) ;
	wmenuadditem (popmenu, "Transpose major", -1) ;
	wmenuadditem (popmenu, "Transpose minor", -1) ;
	wmenuadditem (popmenu, "Rotate left", -1) ;
	wmenuadditem (popmenu, "Rotate right", -1) ;
	wmenuadditem (popmenu, "Flip horizontal", -1) ;
	wmenuadditem (popmenu, "Flip vertical", -1) ;
	wmenuadditem (popmenu, "Zoom in", '+') ;
	wmenuadditem (popmenu, "Zoom out", '-') ;

	wmenucheck (pmmenu, PENCIL_ITEM, TRUE) ;

	for (i = TRANS_MAJ_ITEM ; i <= FLIP_VERT_ITEM ; i++)
		wmenuenable (popmenu, i, FALSE) ;
}

void
mainloop ()
{
	for (;;) {
		EVENT	e ;
	
		wgetevent (&e) ;
		switch (e.type) {
		case WE_MOUSE_DOWN :
		case WE_MOUSE_MOVE :
		case WE_MOUSE_UP :
			do_mouse (&e) ;
			break;
		case WE_COMMAND :
			switch (e.u.command) {
			case WC_CLOSE :
				if (do_quit ())
					return ;
				break ;
			case WC_CANCEL :
				return ;
			}
			break ;
		case WE_CLOSE :
			if (do_quit ())
				return ;
			break ;
		case WE_MENU :
			switch (e.u.m.id) {
			case FILE_MENU :
				if (do_file_menu (&e))
					return ;
				break ;
			case MODE_MENU :
				do_mode_menu (&e) ;
				break ;
			case OP_MENU :
				do_op_menu (&e) ;
				break ;
			}
			break ;
		}
	}
}

main (argc, argv)
	int	argc ;
	char	*argv[] ;
{
	extern char	*strip () ;

	winitargs (&argc, &argv) ;
	
	if (argc > 1) {
		fname = strdup (argv[1]) ;
		title = strip (fname) ;
		if (!readbitmap ()) {
			free (fname) ;
			fname = title = NULL ;
			newraster () ;
		}
	}
	else
		newraster () ;

	setup () ;

	mainloop () ;

	wdone () ;

	exit (0) ;
}
