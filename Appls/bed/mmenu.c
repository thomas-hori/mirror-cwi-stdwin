#include "bed.h"
#include "menu.h"

int	state = PENCIL_ITEM ;

extern MENU	*pmmenu ;
extern MENU	*popmenu ;

extern bool	drawline ;

extern bool	drawcircle ;

extern bool	selrect ;
extern int	sr_left ;
extern int	sr_top ;
extern int	sr_right ;
extern int	sr_bottom ;

extern void	invertbit () ;

void
do_mode_menu (ep)
	EVENT	*ep ;
{
	int	i ;

	switch (ep->u.m.item) {
	case PENCIL_ITEM :
		if (drawline) {
			plotline (invertbit, FALSE) ;
			drawline = FALSE ;
		}
		else if (drawcircle) {
			plotcircle (invertbit, FALSE) ;
			drawcircle = FALSE ;
		}
		else if (selrect) {
			drawselrect (sr_left, sr_top, sr_right, sr_bottom) ;
			selrect = FALSE ;
		}

		wmenucheck (pmmenu, state, FALSE) ;
		wmenucheck (pmmenu, PENCIL_ITEM, TRUE) ;
		state = PENCIL_ITEM ;
		break ;
	case LINE_ITEM :
		if (drawcircle) {
			plotcircle (invertbit, FALSE) ;
			drawcircle = FALSE ;
		}
		else if (selrect) {
			drawselrect (sr_left, sr_top, sr_right, sr_bottom) ;
			selrect = FALSE ;
		}

		wmenucheck (pmmenu, state, FALSE) ;
		wmenucheck (pmmenu, LINE_ITEM, TRUE) ;
		state = LINE_ITEM ;
		break ;
	case CIRCLE_ITEM :
		if (drawline) {
			plotline (invertbit, FALSE) ;
			drawline = FALSE ;
		}
		else if (selrect) {
			drawselrect (sr_left, sr_top, sr_right, sr_bottom) ;
			selrect = FALSE ;
		}

		wmenucheck (pmmenu, state, FALSE) ;
		wmenucheck (pmmenu, CIRCLE_ITEM, TRUE) ;
		state = CIRCLE_ITEM ;
		break ;
	case SELECT_ITEM :
		if (drawline) {
			plotline (invertbit, FALSE) ;
			drawline = FALSE ;
		}
		else if (drawcircle) {
			plotcircle (invertbit, FALSE) ;
			drawcircle = FALSE ;
		}

		wmenucheck (pmmenu, state, FALSE) ;
		wmenucheck (pmmenu, SELECT_ITEM, TRUE) ;
		state = SELECT_ITEM ;
		break ;
	}

	if (state == SELECT_ITEM) {
		for (i = TRANS_MAJ_ITEM ; i <= FLIP_VERT_ITEM ; ++i)
			wmenuenable (popmenu, i, TRUE) ;
	}
	else {
		for (i = TRANS_MAJ_ITEM ; i <= FLIP_VERT_ITEM ; ++i)
			wmenuenable (popmenu, i, FALSE) ;
	}
}
