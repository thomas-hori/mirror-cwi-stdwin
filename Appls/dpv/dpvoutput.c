/* dpv -- ditroff previewer.  Output functions. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"

WINDOW	*win;			/* output window */

/* Scale factors (must be longs to avoid overflow) */

long	hscale, hdiv;		/* Horizontal scaling factors */
long	vscale, vdiv;		/* Vertical scaling factors */

int paperwidth, paperheight;	/* Paper dimenmsions */

/* Variables used to make put1(c) as fast as possible */

int	topdraw, botdraw;	/* Top, bottom of rect to redraw */
int	baseline;		/* Caches wbaseline() */
int	lineheight;		/* Caches wlineheight() */
int	doit;			/* Set if output makes sense */
int	vwindow;		/* Caches VWINDOW - baseline */

extern void	drawproc();	/* Draw procedore, in dpvcontrol.c */

/* Create the window */

initoutput(filename)
	char *filename;
{
	int winwidth, winheight;
	int hmargin, vmargin;
	
	setscale();
	wsetdefwinsize(paperwidth, paperheight);
	win= wopen(filename, drawproc);
	wsetdocsize(win, paperwidth, paperheight);
	wsetwincursor(win, wfetchcursor("arrow"));
	
	/* Center the window in the document */
	wgetwinsize(win, &winwidth, &winheight);
	hmargin = (paperwidth - winwidth) / 2;
	if (hmargin < 0)
		hmargin = 0;
	vmargin = (paperheight - winheight) / 2;
	if (vmargin < 0)
		vmargin = 0;
	wshow(win, hmargin, vmargin,
				paperwidth - hmargin, paperheight - hmargin);
}

/* Compute scale factors from screen dimensions and device resolution.
   Call before outputting anything, but after input device resolution
   is known. */

static
setscale()
{
	int scrwidth, scrheight, mmwidth, mmheight;
	int hdpi, vdpi;
	
	wgetscrsize(&scrwidth, &scrheight);
	wgetscrmm(&mmwidth, &mmheight);
	hdpi = scrwidth * 254L / 10 / mmwidth;
	vdpi = scrheight * 254L / 10 / mmwidth;
	if (dbg) {
		printf("screen %dx%d pixels, %dx%d mm.\n",
			scrwidth, scrheight, mmwidth, mmheight);
		printf("dots per inch %dx%d\n", hdpi, vdpi);
	}
	/* Adjust mm sizes to 75x75 dpi if a resolution less than 75 dpi
	   is reported (some servers lie!) */
	if (hdpi < 75) {
		fprintf(stderr,
			"Adjusting horizontal resolution to 75 dpi\n");
		mmwidth = scrwidth * 254L / 750;
	}
	if (vdpi < 75) {
		fprintf(stderr,
			"dpv: Adjusting vertical resolution to 75 dpi\n");
		mmheight = scrheight * 254L / 750;
	}
	/* Dots/mm == scrwidth/mmwidth, etc.; there are 25.4 mm/inch */
	hscale= 254L * scrwidth / 10;
	hdiv= mmwidth * res;
	vscale= 254L * scrheight / 10;
	vdiv= mmheight * res;
	/* Set desired width & height of paper */
	paperwidth= 210 * scrwidth / mmwidth;
	paperheight= 297 * scrheight / mmheight;
	if (dbg) {
		printf("hscale=%d, hdiv=%d, vscale=%d, vdiv=%d\n",
			hscale, hdiv, vscale, vdiv);
		printf("paper %dx%d\n", paperwidth, paperheight);
	}
}

/* Force a redraw event for the entire window.
   Used after a new page is made current. */

changeall()
{
	wchange(win, 0, 0, paperwidth, paperheight);
}

/* Recompute doit & vwindow.
   Call after each font change and each vertical move.
   Assumes topdraw and botdraw are set by drawproc(),
   and lineheight and baseline are set by usefont(). */

recheck()
{
	if (ipage != showpage)
		doit= FALSE;
	else {
		vwindow= VWINDOW - baseline;
		doit= vwindow < botdraw && vwindow+lineheight > topdraw;
	}
}

/* Output a funny character, called in response to 'Cxx' input.
   Don't rely on 'doit'; the character might be in a different font,
   invalidating the clipping computations in recheck(). */

put1s(s)
	char *s;
{
	if (ipage == showpage)
		drawfunny(s);
}

/* Line drawing functions.
   There really do some of the work that belongs in dpvmachine.c,
   and even dpvparse.c. */

drawline(dh, dv)
	int dh, dv;
{
	if (ipage == showpage)
		wdrawline(HWINDOW, VWINDOW, HWIN(hpos+dh), VWIN(vpos+dv));
	hpos += dh;
	vpos += dv;
}

drawcirc(diameter)
	int diameter;
{
	if (ipage == showpage)
		wdrawcircle(HWIN(hpos+diameter/2), VWINDOW,
			(int)(diameter/2*hscale/hdiv));
	/* I assume hpos, vpos remain unchanged here */
}

drawellip(haxis, vaxis)
	int haxis, vaxis;
{
	if (ipage == showpage) {
		wdrawelarc(HWIN(hpos+haxis/2), VWIN(vpos),
			(int) (haxis*hscale/hdiv/2),
			(int) (vaxis*vscale/vdiv/2),
			0, 360);
	}
}

#define PI 3.1415726

drawarc(n, m, n1, m1)
/*	current position is start of arc
 *	n,m is position of center relative to current
 *	n1,m1 is intersection point of tangents at start- and endpoints
 *	of the arc
 */
{
	double rad, angle1, angle2, sqrt(), atan2(), c;
	int iang1, iang2;
	
	c = 180.0/PI;
	rad = sqrt((double)((n*n)+(m*m)));
	angle1 = c*atan2((double)m,(double)-n);
	angle2 = -2 * (angle1 + c * atan2((double)(m1-m),(double)(n1-n)));
	iang1 = (int) (angle1 + 0.5);
	iang2 = (int) (angle2 + 0.5);
	while (iang1 < 0) iang1 += 360;
	while (iang2 < 0) iang2 += 360;
	while (iang1 >= 360) iang1 -= 360;
	while (iang2 >= 360) iang2 -= 360;
	/* params for wdrawelarc are
	 *	x,y for center,
	 *	horizontal and vertical radii (different for ellipses)
	 *	start angle in degrees and number of degrees to draw.
	 *	angles measured anticlockwise from positive x-axis
	 */
	if (ipage == showpage)
		wdrawelarc(HWIN(hpos+n), VWIN(vpos+m),
			(int)(rad*hscale/hdiv), (int)(rad*vscale/vdiv),
			iang1, iang2);
}

drawwig(buf, fp, flag)
	char *buf;
	FILE *fp;
{
	int dh, dv, x = hpos, y = vpos;

	while (getint(&buf, &dh) && getint(&buf, &dv))
		if (ipage == showpage) {
			/* HIRO: should always do this (for side effects)? */
			drawline(x + dh/4 - hpos, y + dv/4 - vpos);
			drawline(dh / 2, dv / 2);
			x += dh;
			y += dv;
		}
	drawline(dh / 4, dv / 4);
	if (strchr(buf, EOL) == NULL) {
		int c;
		/* HIRO: don't know how to do handle this */
		error(WARNING, "rest of very long wiggly line ignored");
		do {
			c = getc(fp);
		} while (c != EOL && c != EOF);
	}
}

static bool
getint(pbuf, px)
	char **pbuf;
	int *px;
{
	char *buf= *pbuf;
	while (*buf != EOS && isspace(*buf))
		++buf;
	if (*buf == EOS || *buf != '-' && !isdigit(*buf))
		return FALSE;
	*px= atoi(buf);
	while (*buf != EOS && !isspace(*buf))
		++buf;
	*pbuf= buf;
	return TRUE;
}
