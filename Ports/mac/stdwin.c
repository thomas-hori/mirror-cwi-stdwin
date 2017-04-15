/* MAC STDWIN -- BASIC ROUTINES. */

#include "macwin.h"
#include <Fonts.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <OSUtils.h>
#include <SegLoad.h>
#include <Memory.h>


/* Parameters for top left corner choice algorithm in wopen() */

#define LEFT	20	/* Initial left */
#define TOP	40	/* Initial top */
#define HINCR	20	/* Increment for left */
#define VINCR	16	/* Increment for top */


/* GLOBAL DATA. */

			/* XXX choose less obvious names */
GrafPtr screen;		/* The Mac Window Manager's GrafPort */
TEXTATTR wattr;		/* Current or default text attributes */

static int def_left = 0;
static int def_top = 0;
static int def_width = 0;
static int def_height = 0;
static int def_horscroll = 0;
static int def_verscroll = 1;
static int next_left = LEFT;	/* For default placement algorithm */
static int next_top = TOP;


/* INITIALIZATION AND TERMINATION. */

/* Initialization */

int std_open_hook();
STATIC pascal void resume _ARGS((void));

/* Initialize, using and updating argc/argv: */
void
winitargs(pargc, pargv)
	int *pargc;
	char ***pargv;
{
	wargs(pargc, pargv);
	winit();
}

/* Initialize without touching argc/argv */
void
winit()
{
	static init_done;

	/* System 7 uses events to build argc/argv, so wargs() calls
	 ** winit(). We don't want to initialize twice, so the next time
	 ** we immedeately return.
	 */
	if (init_done)
		return;
	init_done = 1;

#ifdef CONSOLE_IO
	/* 
	   THINK C may have done these initializations for us when
	   the application has already used the console in any way.
	   Doing them again is not a good idea.
	   The THINK library avoids initializing the world if it appears
	   that it has already been initialized, but in that case it
	   will only allow output (all input requests return EOF).
	   Thus, the application has two options:
	   - call winit() or winitargs() first; then the console
	     can only be used for (debugging) output; or
	   - print at least one character to stdout first; then the
	     stdwin menu bar may not function properly.
	   From inspection of the THINK library source it appears that
	   when the console is initialized, stdin->std is cleared,
	   so the test below suffices to skip initializations.
	*/
	if (stdin->std)
#endif
	{
		MaxApplZone();
		InitGraf(&QD(thePort));
		InitFonts();
		InitWindows();
		TEInit();
#ifdef __MWERKS__
		InitDialogs((long)0);
#else
		InitDialogs(resume);
#endif
		InitMenus();
		InitCursor();
		setup_menus();
	}
	GetWMgrPort(&screen);
	initwattr();
	set_watch();
}

void
wdone()
{
}

void
wgetscrsize(pwidth, pheight)
	int *pwidth, *pheight;
{
	Rect r;
	r= screen->portRect;
	*pwidth= r.right - r.left;
	*pheight= r.bottom - r.top - MENUBARHEIGHT;
}

void
wgetscrmm(pmmwidth, pmmheight)
	int *pmmwidth, *pmmheight;
{
	int width, height;
	wgetscrsize(&width, &height);
	/* XXX Three pixels/mm is an approximation of the truth at most */
	*pmmwidth= width / 3;
	*pmmheight= height / 3;
}

int
wgetmouseconfig()
{
	return WM_BUTTON1 | WM_SHIFT | WM_META | WM_LOCK | WM_OPTION;
	/* XXX Should figure out if we have a Control button as well */
}

/* Routine called by "Resume" button in bomb box (passed to InitDialogs).
   I have yet to see a program crash where an attempted exit to the
   Finder caused any harm, so I think it's safe.
   Anyway, it's tremendously useful during debugging. */

static pascal void
resume()
{
	ExitToShell();
}


/* WINDOWS. */

/* Find the WINDOW pointer corresponding to a WindowPtr. */

WINDOW *
whichwin(w)
	WindowPtr w;
{
	if (((WindowPeek)w)->windowKind < userKind)
		return NULL; /* Not an application-created window */
	else {
		WINDOW *win;
		
		win= (WINDOW *) GetWRefCon(w);
		if (win != NULL && win->w == w)
			return win;
		else
			return NULL;
	}
}

WINDOW *
wopen(title, drawproc)
	char *title;
	void (*drawproc)();
{
	WINDOW *win= ALLOC(WINDOW);
	Rect r;
	int width, height;		/* As seen by the user */
	int tot_width, tot_height;	/* Including slop & scroll bars */
	int left, top;			/* Window corner as seen by the user */
	
	if (win == NULL) {
		dprintf("wopen: ALLOC failed");
		return NULL;
	}
	
	/* Determine window size.
	   If the program specified a size, use that, within reason --
	   sizes are clipped to 0x7000 to avoid overflow in QuickDraw's
	   calculations.
	   Otherwise, use two-third of the screen size, but at most
	   512x342 (to avoid creating gigantic windows by default on
	   large screen Macs). */
	
	if (def_width <= 0) {
		width = screen->portRect.right * 2/3;
		CLIPMAX(width, 512);
	}
	else {
		width = def_width;
		CLIPMAX(width, 0x7000);
	}
	if (def_horscroll)
		CLIPMIN(width, 3*BAR);
	tot_width = width + LSLOP + RSLOP;
	if (def_verscroll)
		tot_width += BAR;
	
	if (def_height <= 0) {
		height= screen->portRect.bottom * 2/3;
		CLIPMAX(height, 342);
	}
	else {
		height = def_height;
		CLIPMAX(height, 0x7000);
	}
	if (def_verscroll)
		CLIPMIN(height, 3*BAR);
	tot_height = height;
	if (def_horscroll)
		tot_height += BAR;
	
	/* Determine window position.
	   It the program specified a position, use that, but make sure
	   that at least a small piece of the title bar is visible --
	   so the user can recover a window that "fell off the screen".
	   Exception: the title bar may hide completely under the menu
	   bar, since this is the only way to get an (almost) full
	   screen window.
	   Otherwise, place the window a little to the right and below
	   the last window placed; it it doesn't fit, move it up.
	   With default placement, the window will never hide under the
	   title bar. */
	
	if (def_left <= 0) {
		left = next_left;
		if (left + tot_width >= screen->portRect.right) {
			left = LEFT;
			CLIPMAX(left, screen->portRect.right - tot_width);
			CLIPMIN(left, 0);
		}
	}
	else {
		left = def_left - LSLOP;
		CLIPMAX(left, screen->portRect.right - BAR);
		CLIPMIN(left, BAR - tot_width);
	}
	
	if (def_top <= 0) {
		top = next_top;
		if (top + tot_height >= screen->portRect.bottom) {
			top = TOP;
			CLIPMAX(top, screen->portRect.bottom - tot_height);
			CLIPMIN(top, MENUBARHEIGHT + TITLEBARHEIGHT);
		}
	}
	else {
		top = def_top;
		CLIPMAX(top, screen->portRect.bottom);
		CLIPMIN(top, MENUBARHEIGHT);
	}
	
	next_left = left + HINCR;
	next_top = top + VINCR;
	
	/* Create the window now and initialize its attributes */
	
	SetRect(&r, left, top, left + tot_width, top + tot_height);
	win->w= NewWindow((Ptr)NULL, &r, PSTRING(title), TRUE, zoomDocProc,
		(WindowPtr)(-1), TRUE, 0L);
	SetWRefCon(win->w, (long)win);
	
	win->tag= 0;
	win->drawproc= drawproc;
	win->hcaret= win->vcaret= -1;
	win->caret_on= FALSE;
	win->attr= wattr;
	win->hbar= win->vbar= NULL;
	win->docwidth= 0;
	win->docheight= 0;
	win->orgh= -LSLOP;
	win->orgv= 0;
	win->timer= 0;
	win->cursor = NULL;
	win->fgcolor = _w_fgcolor;
	win->bgcolor = _w_bgcolor;
	
	SetPort(win->w);
	_w_usefgcolor(win->fgcolor);
	_w_usebgcolor(win->bgcolor);
	
	initmbar(&win->mbar);
	makescrollbars(win, def_horscroll, def_verscroll);
	
	return win;
}

void
wclose(win)
	WINDOW *win;
{
	if (win == active) {
		rmlocalmenus(win);
		active= NULL;
	}
	killmbar(&win->mbar);
	DisposeWindow(win->w);
	FREE(win);
}

void
wgetwinsize(win, pwidth, pheight)
	WINDOW *win;
	int *pwidth, *pheight;
{
	Rect r;
	
	getwinrect(win, &r);
	*pwidth= r.right - r.left - LSLOP - RSLOP;
	*pheight= r.bottom - r.top;
}

void
wsetwinsize(win, width, height)
	WINDOW *win;
	int width, height;
{
	/* XXX not yet implemented */
}

void
wgetwinpos(win, ph, pv)
	WINDOW *win;
	int *ph, *pv;
{
	Point p;
	GrafPtr saveport;
	
	GetPort(&saveport);
	
	SetPort(win->w);
	p.h = win->w->portRect.left + LSLOP;
	p.v = win->w->portRect.top;
	LocalToGlobal(&p);
	*ph = p.h;
	*pv = p.v;
	
	SetPort(saveport);
}

void
wsetwinpos(win, left, top)
	WINDOW *win;
	int left, top;
{
	/* XXX not yet implemented */
}

void
wsettitle(win, title)
	WINDOW *win;
	char *title;
{
	SetWTitle(win->w, PSTRING(title));
}

char *
wgettitle(win)
	WINDOW *win;
{
	static Str255 title;
	GetWTitle(win->w, title);
#ifndef CLEVERGLUE
	PtoCstr(title);
#endif
	return (char *)title;
}

void
wfleep()
{
	SysBeep(5);
}

void
wsetmaxwinsize(width, height)
	int width, height;
{
	/* Not supported yet (should be stored in the window struct
	   and used by do_grow). */
	/* XXX This procedure should disappear completely, it was
	   only invented for the Whitechapel which allocates bitmap
	   memory to the window when it is first created! */
	/* XXX Well, maybe it has some use.  In fact both min and max
	   window size are sometimes useful... */
}

void
wsetdefwinpos(h, v)
	int h, v;
{
	def_left = h;
	def_top = v;
}

void
wgetdefwinpos(ph, pv)
	int *ph, *pv;
{
	*ph = def_left;
	*pv = def_top;
}

void
wsetdefwinsize(width, height)
	int width, height;
{
	def_width= width;
	def_height= height;
}

void
wgetdefwinsize(pwidth, pheight)
	int *pwidth, *pheight;
{
	*pwidth = def_width;
	*pheight = def_height;
}

void
wsetdefscrollbars(hor, ver)
	int hor, ver;
{
	def_horscroll = hor;
	def_verscroll = ver;
}

void
wgetdefscrollbars(phor, pver)
	int *phor, *pver;
{
	*phor = def_horscroll;
	*pver = def_verscroll;
}
