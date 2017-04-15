/* dpv -- ditroff previewer.  Definitions for output. */

extern char	*devname;	/* Device for which output is prepared */

extern WINDOW	*win;		/* output window */

extern int	showpage;	/* internal page to show in window */
				/* defined in dpvcontrol.c */
extern int	prevpage;	/* previous page shown (for '-' command) */

extern long	hscale, hdiv;	/* Horizontal scaling factors */
extern long	vscale, vdiv;	/* Vertical scaling factors */

/* Scaling method */
#define HWIN(h) ((int)(((h)*hscale+hdiv/2)/hdiv))
#define VWIN(v) ((int)(((v)*vscale+vdiv/2)/vdiv))
#define HWINDOW HWIN(hpos)
#define VWINDOW VWIN(vpos)

/* Variables used by put1 macro below, and by recheck function */

extern int	doit;		/* nonzero if output makes sense */
extern int	vwindow;	/* precomputed vpos*vscale/vdiv - baseline */
extern int	baseline;	/* wbaseline() of current font */
extern int	lineheight;	/* wlineheight() of current font */
extern int	topdraw, botdraw;	/* window area to draw */

/* Macro to output one character */

#define put1(c) if (!doit) ; else wdrawchar(HWINDOW, vwindow, c)
