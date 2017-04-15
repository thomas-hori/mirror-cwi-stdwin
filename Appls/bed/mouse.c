#include "bed.h"
#include "menu.h"

#ifndef ABS
#define ABS(var)	((var) >= 0 ? (var) : -(var))
#endif

extern int	sqrsize ;

extern int	map_width ;
extern int	map_height ;

extern char	*raster ;
extern int	raster_lenght ;
extern int	stride ;

extern WINDOW	*win ;

extern bool	changed ;

extern int	state ;

bool	drawline = FALSE ;
int	beg_h ;
int	beg_v ;
int	end_h ;
int	end_v ;

bool	drawcircle = FALSE ;
int	cent_h ;
int	cent_v ;
int	c_dh ;
int	c_dv ;

bool	selrect = FALSE ;
int	sr_left ;
int	sr_top ;
int	sr_right ;
int	sr_bottom ;

void
wxorrect (left, top, right, bottom)
	int	left ;
	int	top ;
	int	right ;
	int	bottom ;
{
	wxorline (left, top, right - 1, top) ;
	wxorline (right - 1, top, right -1, bottom - 1) ;
	wxorline (left, bottom - 1, right -1, bottom - 1) ;
	wxorline (left, top, left, bottom - 1) ;

	if (right - left <= bottom - top) {
		wxorline (left, top, right - 1, top + (right - left) - 1) ;
		wxorline (right - 1, top, left, top + (right - left) - 1) ;
	}
	else {
		wxorline (left, top, left + (bottom - top) - 1, bottom - 1) ;
		wxorline (right - 1, top, right - (bottom - top) - 1, bottom -1) ;
	}
}
	
void
drawselrect ()
{
	int	l = sr_left * sqrsize ;
	int	t = sr_top * sqrsize ;
	int	r = sr_right * sqrsize ;
	int	b = sr_bottom * sqrsize ;

	wbegindrawing (win) ;

	wxorrect (l, t, r, b) ;

	wenddrawing (win) ;
}

int
getbit(col, row)
{
	char	*byte ;
	int	rc ;

	if (row >= 0 && row < map_height && col >= 0 && col < map_width) {
		byte = raster + (row * stride) + (col / 8) ;
		rc = (*byte & (1 << (col % 8))) ? 1 : 0 ;
	}
	else
		rc = 0 ;

	return (rc) ;
}

void
setbit(col, row, value)
{
	char	*byte ;

	if (row >= 0 && row < map_height && col >= 0 && col < map_width) {
		changed = 1 ;
		byte = raster + (row * stride) + (col / 8) ;
		*byte = (*byte & ~(1 << (col %8))) | (value << (col % 8)) ;
		wchange (win, col * sqrsize, row * sqrsize,
			(col+1) * sqrsize, (row+1) * sqrsize);
	}
}

void
do_pencil (ep)
	EVENT	*ep ;
{
	static int	value ;

	if (ep->type == WE_MOUSE_DOWN)
		value = !getbit (ep->u.where.h / sqrsize,
				 ep->u.where.v / sqrsize) ;
	setbit (ep->u.where.h / sqrsize,
		ep->u.where.v / sqrsize, value) ;
}

void
invertbit (h, v, value)
	int	h ;
	int	v ;
	bool	value ;
{
	if (h < 0 || v < 0 || h >= map_width || v >= map_height)
		return ;

	winvert (h * sqrsize, v * sqrsize,
					(h + 1) * sqrsize, (v + 1) * sqrsize) ;
}

void
plotline (fp, value)
	void	(*fp) () ;
	bool	value ;
{
	int	d_h = end_h - beg_h ;
	int	d_v = end_v - beg_v ;
	int	e ;
	int	h = 0 ;
	int	v = 0 ;
	int	hsign = 1 ;
	int	vsign = 1 ;
	int	i ;

	if (d_h < 0) {
		d_h = -d_h ;
		hsign = -1 ;
	}

	if (d_v < 0) {
		d_v = -d_v ;
		vsign = -1 ;
	}

	wbegindrawing (win) ;

	if (d_v <= d_h) {
		for (i = 0, e = 2 * d_v - d_h ; i <= d_h ; ++i) {
			(*fp) (beg_h + (hsign * h), beg_v + (vsign * v), value) ;

			if (e > 0) {
				++v ;
				e -= (2 * d_h) ;
			}

			e += (2 * d_v) ;
			++h ;
		}
	}
	else {
		for (i = 0, e = 2 * d_h - d_v ; i <= d_v ; ++i) {
			(*fp) (beg_h + (hsign * h), beg_v + (vsign * v), value) ;

			if (e > 0) {
				++h ;
				e -= (2 * d_v) ;
			}

			e += (2 * d_h) ;
			++v ;
		}
	}

	wenddrawing (win) ;
}

void
plotcircle (fp, value)
	void	(*fp) () ;
	bool	value ;
{
	long	h_sqr = c_dh * c_dh ;
	long	v_sqr = c_dv * c_dv ;
	long	r_sqr = h_sqr * v_sqr ;
	long	prev_r = r_sqr ;
	long	min_h ;
	long	min_v ;
	long	min_hv ;
	int	h = c_dh ;
	int	v = 0 ;

	wbegindrawing (win) ;

	while (h >= 0 && v <= c_dv) {
		(*fp) (cent_h + h, cent_v + v, value) ;
		if (v)
			(*fp) (cent_h + h, cent_v - v, value) ;
		if (h)
			(*fp) (cent_h - h, cent_v + v, value) ;
		if (h && v)
			(*fp) (cent_h - h, cent_v - v, value) ;

		min_h = (long) (2 * h - 1) * v_sqr ;
		min_v = (long) (2 * v + 1) * -h_sqr ;
		min_hv = min_h + min_v ;

		if (ABS (r_sqr - (prev_r - min_h)) <=
					ABS (r_sqr - (prev_r - min_v))) {
			if (ABS (r_sqr - (prev_r - min_hv)) <=
					ABS (r_sqr - (prev_r - min_h))) {
				prev_r = prev_r - min_hv ;
				--h ;
				++v ;
			}
			else {
				prev_r = prev_r - min_h ;
				--h ;
			}
		}
		else {
			if (ABS (r_sqr - (prev_r - min_hv)) <=
					ABS (r_sqr - (prev_r - min_v))) {
				prev_r = prev_r - min_hv ;
				--h ;
				++v ;
			}
			else {
				prev_r = prev_r - min_v ;
				++v ;
			}
		}
	}

	wenddrawing (win) ;
}

void
do_line (ep)
	EVENT	*ep ;
{
	int	curr_h = ep->u.where.h / sqrsize ;
	int	curr_v = ep->u.where.v / sqrsize ;

	if (curr_h > map_height - 1)
		curr_h = map_height - 1 ;
	if (curr_v > map_width - 1)
		curr_v = map_width - 1 ;

	switch (ep->type) {
	case WE_MOUSE_DOWN :
		if (drawline) {
			plotline (invertbit, FALSE) ;
			drawline = FALSE ;
		}

		drawline = TRUE ;

		beg_h = end_h = curr_h ;
		beg_v = end_v = curr_v ;

		plotline (invertbit, TRUE) ;
		break ;
	case WE_MOUSE_MOVE :
		if (drawline) {
			if (curr_h == end_h && curr_v == end_v)
				return ;

			plotline (invertbit, FALSE) ;

			end_h = curr_h ;
			end_v = curr_v ;

			plotline (invertbit, TRUE) ;
		}
		break ;
	case WE_MOUSE_UP :
		if (drawline) {
			if (curr_h != end_h || curr_v != end_v) {
				plotline (invertbit, FALSE) ;

				end_h = curr_h ;
				end_v = curr_v ;

				plotline (invertbit, TRUE) ;
			}

			plotline (setbit, TRUE) ;

			drawline = FALSE ;
		}
		break ;
	}
}

void
do_circle (ep)
	EVENT	*ep ;
{
	int	curr_h = ep->u.where.h / sqrsize ;
	int	curr_v = ep->u.where.v / sqrsize ;

	if (curr_h > map_height - 1)
		curr_h = map_height - 1 ;
	if (curr_v > map_width - 1)
		curr_v = map_width - 1 ;

	switch (ep->type) {
	case WE_MOUSE_DOWN :
		if (drawcircle) {
			plotcircle (invertbit, FALSE) ;
			drawcircle = FALSE ;
		}

		drawcircle = TRUE ;

		cent_h = curr_h ;
		cent_v = curr_v ;
		c_dh = c_dv = 0 ;

		plotcircle (invertbit, TRUE) ;
		break ;
	case WE_MOUSE_MOVE :
		if (drawcircle) {
			if (ABS ((curr_h - cent_h) + 1) == c_dh &&
					ABS ((curr_v - cent_v) + 1) == c_dv)
				return ;

			plotcircle (invertbit, FALSE) ;

			c_dh = ABS (curr_h - cent_h) ;
			c_dv = ABS (curr_v - cent_v) ;

			plotcircle (invertbit, TRUE) ;
		}
		break ;
	case WE_MOUSE_UP :
		if (drawcircle) {
			if (ABS ((curr_h - cent_h) + 1) != c_dh ||
					ABS ((curr_v - cent_v) + 1) != c_dv) {
				plotcircle (invertbit, FALSE) ;

				c_dh = ABS (curr_h - cent_h) ;
				c_dv = ABS (curr_v - cent_v) ;

				plotcircle (invertbit, TRUE) ;
			}

			plotcircle (setbit, TRUE) ;

			drawcircle = FALSE ;
		}
		break ;
	}
}

void
do_select (ep)
	EVENT	*ep ;
{
	static int	sr_h ;	/* coord. of mouse down */
	static int	sr_v ;
	int	curr_h = ep->u.where.h / sqrsize ;
	int	curr_v = ep->u.where.v / sqrsize ;

	switch (ep->type) {
	case WE_MOUSE_DOWN :
		if (selrect) {
			drawselrect () ;
			selrect = FALSE ;
		}

		if (curr_h >= map_width || curr_v >= map_height)
			return ;

		selrect = TRUE ;
		sr_h = curr_h ;
		sr_v = curr_v ;

		sr_left = curr_h ;
		sr_top = curr_v ;
		sr_right = sr_left + 1 ;
		sr_bottom = sr_top + 1 ;

		drawselrect () ;
		break ;
	case WE_MOUSE_MOVE :
		if (selrect) {
			if (curr_h >= map_width)
				curr_h = map_width - 1 ;
			if (curr_v >= map_height)
				curr_v = map_height - 1 ;

			if (curr_h < 0)
				curr_h = 0 ;
			if (curr_v < 0)
				curr_v = 0 ;

			drawselrect () ;

			if (curr_h < sr_h) {
				sr_left = curr_h ;
				sr_right = sr_h + 1 ;
			}
			else {
				sr_left = sr_h ;
				sr_right = curr_h + 1 ;
			}

			if (curr_v < sr_v) {
				sr_top = curr_v ;
				sr_bottom = sr_v + 1 ;
			}
			else {
				sr_top = sr_v ;
				sr_bottom = curr_v + 1 ;
			}

			drawselrect () ;
		}
		break ;
	case WE_MOUSE_UP :
			break ;
	}
}

void
do_mouse (ep)
	EVENT	*ep ;
{
	switch (state) {
	case PENCIL_ITEM :
		do_pencil (ep) ;
		break ;
	case LINE_ITEM :
		do_line (ep) ;
		break ;
	case CIRCLE_ITEM :
		do_circle (ep) ;
		break ;
	case SELECT_ITEM :
		do_select (ep) ;
		break ;	
	}
}
