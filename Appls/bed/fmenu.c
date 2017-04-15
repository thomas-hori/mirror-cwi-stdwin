#include <stdio.h>
#include "bed.h"
#include "menu.h"

extern char	*strip () ;

extern int	sqrsize ;

extern int	map_width ;
extern int	map_height ;

extern char	*fname ;
extern char	*title ;

extern WINDOW	*win ;

extern char	*raster ;

extern bool	changed ;

void
do_open_file ()
{
	if (changed) {
		switch (waskync ("Save changes ?", 1)) {
		case 1:
			if (!writebitmap ())
				return ;
			break ;
		case -1 :
			return ;
		}

		changed = FALSE ;
	}
	if (fname != NULL) {
		free (fname) ;
		fname = title = NULL ;
	}

	if (!readbitmap ()) {
		if (fname != NULL) {
			free (fname) ;
			fname = title = NULL ;
			wsettitle (win, "Untitled") ;
		}

		newraster () ;
	}
}

void
do_new_map ()
{
	char	str[20] ;

	if (changed) {
		switch (waskync ("Save changes ?", 1)) {
		case 1 :
			if (!writebitmap ())
				return ;
			break ;
		case -1 :
			return ;
		}

		changed = FALSE ;
	}

	if (fname != NULL) {
		free (fname) ;
		fname = title = NULL ;
		wsettitle (win, "Untitled") ;
	}

	*str = 0 ;

	if (waskstr ("Enter width of new raster", str, 20)) {
		while ((map_width = atoi (str)) <= 0) {
			*str = 0 ;
			if (!waskstr ("Please enter a number greater 0",
								str, 20)) {
				map_width = -1 ;
				break ;
			}
		}

		if (waskstr ("Enter height of new raster", str, 20)) {
			while ((map_height = atoi (str)) <= 0) {
				*str = 0 ;
				if (!waskstr ("Please enter a number greater 0",
								str, 20))
					break ;
			}
		}
		else
			map_width = -1 ;
	}

	if (map_width == -1) {
		map_width = DEF_COLS ;
		map_height = DEF_ROWS ;
	}

	newraster () ;
}

bool
do_quit ()
{
	if (changed) {
		switch (waskync ("Save changes ?", 1)) {
		case 1 :
			if (!writebitmap ())
				return (FALSE) ;
			break ;
		case -1 :
			return (FALSE) ;
		}

		changed = FALSE ;
	}
	wclose (win) ;
	free (raster) ;
	raster = NULL ;
	return (TRUE) ;
}

bool
do_file_menu (ep)
	EVENT	*ep ;
{
	switch (ep->u.m.item) {
	case OPEN_ITEM:
		do_open_file () ;
		setsqrsize () ;
		wchange (win, 0, 0, map_width * sqrsize,
						map_height * sqrsize) ;
		wsetdocsize (win, map_width * sqrsize, map_height * sqrsize) ;
		break ;
	case NEW_ITEM:
		do_new_map () ;
		setsqrsize () ;
		wchange (win, 0, 0, map_width * sqrsize,
						map_height * sqrsize) ;
		wsetdocsize (win, map_width * sqrsize, map_height * sqrsize) ;
		break ;
	case SAVE_ITEM:
		if (changed) {
			if (writebitmap ())
				changed = FALSE ;
		}
		break ;
	case SAVE_AS_ITEM: {
		char	namebuf[256] ;

		strcpy (namebuf, fname) ;
		if (waskfile ("Save as ?", namebuf, 256, FALSE)) {
			char	*savedname = fname ;

			fname = strdup (namebuf) ;
			title = strip (fname) ;
			(void) writebitmap () ;
			free (fname) ;
			fname = savedname ;
			title = strip (fname) ;
		}
		break ;
		}
	case QUIT_ITEM:
		return (do_quit ()) ;
	}

	return (FALSE) ;
}
