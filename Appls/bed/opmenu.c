#include "bed.h"
#include "menu.h"

extern int	sqrsize ;

extern int	map_width ;
extern int	map_height ;

extern char	*raster ;
extern int	raster_lenght ;
extern int	stride ;

static int	bits[] = { 0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F } ;
extern int	lastbyte ;

extern WINDOW	*win ;

extern bool	changed ;

extern bool	selrect ;
extern int	sr_left ;
extern int	sr_top ;
extern int	sr_right ;
extern int	sr_bottom ;

/* Data manipulation routines */

void
clear_bits()
{
	register int	row ;

	if (!selrect) {
		sr_left = sr_top = 0 ;
		sr_right = map_width ;
		sr_bottom = map_height ;
	}

	for (row = sr_top ; row < sr_bottom ; ++row) {
		register int	col = sr_left ;
		register char	*byte = raster + (row * stride) + (col / 8) ;
		register int	pbit = col % 8 ;

		while (col++ < sr_right) {
			*byte &= ~(1 << pbit) ;

			if (++pbit >= 8) {
				pbit = 0 ;
				++byte ;
			}
		}
	}

	wchange (win, sr_left * sqrsize, sr_top * sqrsize,
		sr_right * sqrsize, sr_bottom * sqrsize) ;

	changed = TRUE ;
}

void
set_bits()
{
	register int	row ;

	if (!selrect) {
		sr_left = sr_top = 0 ;
		sr_right = map_width ;
		sr_bottom = map_height ;
	}

	for (row = sr_top ; row < sr_bottom ; ++row) {
		register int	col = sr_left ;
		register char	*byte = raster + (row * stride) + (col / 8) ;
		register int	pbit = col % 8 ;

		while (col++ < sr_right) {
			*byte |= (1 << pbit) ;

			if (++pbit >= 8) {
				pbit = 0 ;
				++byte ;
			}
		}
	}

	wchange (win, sr_left * sqrsize, sr_top * sqrsize,
		sr_right * sqrsize, sr_bottom * sqrsize) ;

	changed = TRUE ;
}

void
invert_bits()
{
	register int	row ;

	if (!selrect) {
		sr_left = sr_top = 0 ;
		sr_right = map_width ;
		sr_bottom = map_height ;
	}

	for (row = sr_top ; row < sr_bottom ; ++row) {
		register int	col = sr_left ;
		register char	*byte = raster + (row * stride) + (col / 8) ;
		register int	pbit = col % 8 ;

		while (col++ < sr_right) {
			*byte ^= (1 << pbit) ;

			if (++pbit >= 8) {
				pbit = 0 ;
				++byte ;
			}
		}
	}

	wchange (win, sr_left * sqrsize, sr_top * sqrsize,
		sr_right * sqrsize, sr_bottom * sqrsize) ;

	changed = TRUE ;
}

int
transpose_major()
{
	int	i ;
	int	j ;
	int	tmp ;

	if (selrect) {
		if (sr_right - sr_left != sr_bottom - sr_top) {
			wfleep () ;
			wmessage ("You can only transpose a square") ;
			return ;
		}

		for (i = 0 ; i < sr_bottom - sr_top ; ++i) {
			for (j = 0 ; j < i ; ++j) {
				int	row = i + sr_top ;
				int	col = j + sr_left ;
	
				tmp = getbit (col, row) ;
				setbit (col, row, getbit (i + sr_left,
								j + sr_top)) ;
				setbit (i + sr_left, j + sr_top, tmp) ;
			}
		}

		wchange (win, sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;

		changed = TRUE ;
	}
	else
		wfleep () ;
}

void
transpose_minor ()
{
	int	size ;
	int	i ;
	int	j ;
	int	tmp ;

	if (selrect) {
		if (sr_bottom - sr_top != sr_right - sr_left) {
			wfleep () ;
			wmessage ("You can only transpose a square") ;
			return ;
		}

		size = sr_bottom - sr_top ;

		for (i = 0 ; i < size ; ++i) {
			for (j = 0 ; j < size - 1 - i ; ++j) {
				tmp = getbit (j + sr_left, i + sr_top) ;
				setbit (j + sr_left, i + sr_top,
					getbit (size - 1 - i + sr_left,
						size - 1 - j + sr_top)) ;
				setbit (size - 1 - i + sr_left,
					size - 1 - j + sr_top, tmp) ;
			}
		}

		wchange (win, sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;

		changed = TRUE ;
	}
	else
		wfleep () ;

}

int
rotate_left()
{
	int	i ;
	int	j ;
	int	size ;
	int	tmp ;

	if (selrect) {
		if (sr_bottom - sr_top != sr_right - sr_left) {
			wfleep () ;
			wmessage ("You can only rotate a square") ;
			return ;	
		}

		size = sr_bottom - sr_top ;

		for (i = 0 ; i < size / 2 ; ++i) {
			for (j = 0 ; j < (size + 1) / 2 ; ++j) {
				tmp = getbit (j + sr_left, i + sr_top) ;
				setbit (j + sr_left, i + sr_top,
					getbit (size - 1 - i + sr_left,
						j + sr_top)) ;
				setbit (size - 1 - i + sr_left, j + sr_top,
					getbit (size - 1 - j + sr_left,
						size - 1 - i + sr_top)) ;
				setbit (size - 1 - j + sr_left,
					size - 1 - i + sr_top,
					getbit (i + sr_left,
						size - 1 - j + sr_top)) ;
				setbit (i + sr_left, size - 1 - j + sr_top,
					tmp) ;
			}
		}

		wchange (win, sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;

		changed = TRUE ;
	}
	else
		wfleep () ;
}

void
rotate_right()
{
	int	i ;
	int	j ;
	int	size ;
	int	tmp ;

	if (selrect) {
		if (sr_bottom - sr_top != sr_right - sr_left) {
			wfleep () ;
			wmessage ("You can only rotate a square") ;
			return ;
		}

		size = sr_bottom - sr_top ;

		for (i = 0 ; i < size / 2 ; ++i) {
			for (j = 0 ; j < (size + 1) / 2 ; ++j) {
				tmp = getbit (j + sr_left, i + sr_top) ; 
				setbit (j + sr_left, i + sr_top,
					getbit (i + sr_left,
						size - 1 - j + sr_top)) ;
				setbit (i + sr_left, size - 1 - j + sr_top,
					getbit (size - 1 - j + sr_left,
						size - 1 - i + sr_top)) ;
				setbit (size - 1 - j + sr_left,
					size - 1 - i + sr_top,
					getbit (size - 1 - i + sr_left,
						j + sr_top)) ;
				setbit (size - 1 - i + sr_left, j + sr_top,
					tmp) ;
			}
		}

		wchange (win, sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;

		changed = TRUE ;
	}
}

void
flip_horizontal()
{
	int	i ;
	int	j ;
	int	sr_width ;
	int	sr_height ;
	int	tmp ;

	if (selrect) {
		sr_width = sr_right - sr_left ;
		sr_height = sr_bottom - sr_top ;

		for (i = 0 ; i < sr_height ; ++ i) {
			for (j = 0 ; j < sr_width / 2 ; ++j) {
				tmp = getbit (j + sr_left, i + sr_top) ;
				setbit (j + sr_left, i + sr_top,
					getbit (sr_width - 1 - j + sr_left,
						i + sr_top)) ;
				setbit (sr_width - 1 - j + sr_left, i + sr_top,
					tmp) ;
			}
		}

		wchange (win, sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;

		changed = TRUE ;
	}
	else
		wfleep () ;
}

void
flip_vertical()
{
	int	i ;
	int	j ;
	int	sr_width ;
	int	sr_height ;
	int	tmp ;

	if (selrect) {
		sr_width = sr_right - sr_left ;
		sr_height = sr_bottom - sr_top ;

		for (i = 0 ; i < sr_height / 2 ; ++i) {
			for (j = 0 ; j < sr_width ; ++j) {
				tmp = getbit (j + sr_left, i + sr_top) ;
				setbit (j + sr_left, i + sr_top,
					getbit (j + sr_left,
						sr_height - 1 - i + sr_top)) ;
				setbit (j + sr_left, sr_height - 1 - i + sr_top,
					tmp) ;
			}
		}

		wchange (win, sr_left * sqrsize, sr_top * sqrsize,
			sr_right * sqrsize, sr_bottom * sqrsize) ;

		changed = TRUE ;
	}
	else
		wfleep () ;
}

void
zoom_in ()
{
	sqrsize *= 2 ;
	wsetdocsize (win, map_width * sqrsize, map_height * sqrsize) ;
	wchange (win, 0, 0, map_width * sqrsize, map_height * sqrsize) ;
}

void
zoom_out ()
{
	int oss = sqrsize ;
	if (sqrsize == 1) wfleep () ;
	else {
		if ((sqrsize /= 2) < 1) sqrsize = 1 ;
		wsetdocsize (win, map_width * sqrsize, map_height * sqrsize) ;
		wchange (win, 0, 0, map_width * oss, map_height * oss) ;
	}
}


void
do_op_menu (ep)
	EVENT	*ep ;
{
	switch (ep->u.m.item) {
	case CLEAR_ITEM :
		clear_bits () ;
		break ;
	case SET_ITEM :
		set_bits () ;
		break ;
	case INVERT_ITEM :
		invert_bits () ;
		break ;
	case TRANS_MAJ_ITEM :
		transpose_major () ;
		break ;
	case TRANS_MIN_ITEM :
		transpose_minor () ;
		break ;
	case ROT_LEFT_ITEM :
		rotate_left () ;
		break ;
	case ROT_RIGHT_ITEM :
		rotate_right () ;
		break ;
	case FLIP_HOR_ITEM :
		flip_horizontal () ;
		break ;
	case FLIP_VERT_ITEM :
		flip_vertical () ;
		break ;
	case ZOOM_IN :
		zoom_in () ;
		break ;
	case ZOOM_OUT :
		zoom_out () ;
		break ;
	}
}
