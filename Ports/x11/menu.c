/* X11 STDWIN -- Menu handling */

#include "x11.h"
#include "llevent.h" /* For _w_dirty */

/* Forward */
static void mbardirty();
static void _wmenusetup();
static void inverttitle();
static int leftedge();
static void drawmenu();
static void makemenu();
static void showmenu();
static void hidemenu();
static int whichmenu();
static void hiliteitem();
static void invertitem();
static int whichitem();

static bool local; /* When zero, menus are created global */

static int nmenus;
static MENU **menulist; /* Only global menus */

static MENU *sysmenu; /* System menu, included in each window's mbar */

static void
mbardirty(win)
	WINDOW *win;
{
	_w_dirty= win->mbar.dirty= TRUE;
}

void
wmenusetdeflocal(flag)
	bool flag;
{
	local= flag;
}

void
wmenuattach(win, mp)
	WINDOW *win;
	MENU *mp;
{
	int i;
	
	for (i= 0; i < win->nmenus; ++i) {
		if (win->menulist[i] == mp)
			return; /* Already attached */
	}
	L_APPEND(win->nmenus, win->menulist, MENU *, mp);
	mbardirty(win);
}

void
wmenudetach(win, mp)
	WINDOW *win;
	MENU *mp;
{
	int i;
	
	for (i= 0; i < win->nmenus; ++i) {
		if (win->menulist[i] == mp) {
			L_REMOVE(win->nmenus, win->menulist, MENU *, i);
			mbardirty(win);
			break;
		}
	}
}

_waddmenus(win)
	WINDOW *win;
{
	int i;
	
	/* Create system menu if first time here */
	if (sysmenu == NULL && _wgetbool("xmenu", "Xmenu", 0)) {
		bool savelocal= local;
		wmenusetdeflocal(TRUE);
		sysmenu= wmenucreate(0, "X");
		if (sysmenu != NULL)
			wmenuadditem(sysmenu, "Close", -1);
		wmenusetdeflocal(savelocal);
	}
	
	/* Initialize the nonzero variables used by the menu module */
	win->curmenu= win->curitem= -1;
	
	/* Add system menu and all global menus to the menu list */
	if (sysmenu != NULL) {
		L_APPEND(win->nmenus, win->menulist, MENU *, sysmenu);
	}
	for (i= 0; i < nmenus; ++i) {
		L_APPEND(win->nmenus, win->menulist, MENU *, menulist[i]);
	}
	if (nmenus > 0)
		mbardirty(win);
}

_w_delmenus(win)
	WINDOW *win;
{
	/* Delete the menu list before closing a window */
	L_DEALLOC(win->nmenus, win->menulist);
}

MENU *
wmenucreate(id, title)
	int id;
	char *title;
{
	MENU *mp= ALLOC(MENU);
	
	if (mp == NULL)
		return NULL;
	mp->id= id;
	mp->title= strdup(title);
	mp->nitems= 0;
	mp->itemlist= NULL;
	if (!local) {
		L_APPEND(nmenus, menulist, MENU *, mp);
		_waddtoall(mp);
	}
	return mp;
}

void
wmenudelete(mp)
	MENU *mp;
{
	int i;
	
	for (i= 0; i < nmenus; ++i) {
		if (menulist[i] == mp) {
			L_REMOVE(nmenus, menulist, MENU *, i);
			break;
		}
	}
	_wdelfromall(mp);
	
	for (i= 0; i < mp->nitems; ++i)
		FREE(mp->itemlist[i].text);
	FREE(mp->itemlist);
	FREE(mp);
}

int
wmenuadditem(mp, text, shortcut)
	MENU *mp;
	char *text;
	int shortcut;
{
	struct item it;
	
	it.text= strdup(text);
	if (shortcut < 0)
		it.sctext= NULL;
	else {
		char buf[50];
		sprintf(buf, "M-%c", shortcut);
		it.sctext= strdup(buf);
	}
	it.shortcut= shortcut;
	it.enabled=  it.text != NULL && it.text[0] != EOS;
	it.checked= FALSE;
	L_APPEND(mp->nitems, mp->itemlist, struct item, it);
	return mp->nitems - 1;
}

void
wmenusetitem(mp, i, text)
	MENU *mp;
	int i;
	char *text;
{
	if (i < 0 || i >= mp->nitems)
		return;
	FREE(mp->itemlist[i].text);
	mp->itemlist[i].text= strdup(text);
}

void
wmenuenable(mp, i, flag)
	MENU *mp;
	int i;
	bool flag;
{
	if (i < 0 || i >= mp->nitems)
		return;
	mp->itemlist[i].enabled= flag;
}

void
wmenucheck(mp, i, flag)
	MENU *mp;
	int i;
	bool flag;
{
	if (i < 0 || i >= mp->nitems)
		return;
	mp->itemlist[i].checked= flag;
}

/* --- system-dependent code starts here --- */

/*
XXX Stuff recomputed more than I like:
	- length of menu titles and menu item texts
	- menu item text widths
*/

#define NOITEM (-1)
#define NOMENU (-1)

/* Precomputed values (assume _wdrawmbar is always called first) */
static int baseline;
static int lineheight;
static int halftitledist;
#define titledist (2*halftitledist)
static int shortcutdist;

static void
_wmenusetup()
{
	shortcutdist= XTextWidth(_wmf, "    ", 4);
	halftitledist= XTextWidth(_wmf, "0", 1);
	baseline= _wmf->ascent;
	lineheight= _wmf->ascent + _wmf->descent;
}

/* Draw the menu bar.
   Called via the normal exposure event mechanism. */

_wdrawmbar(win)
	WINDOW *win;
{
	int i;
	int x;
	int y;

	if (win->mbar.wid == None)
		return;
	_wmenusetup();
	x= titledist;
	y= baseline - 1 + (win->mbar.height - lineheight) / 2;
	
	XClearWindow(_wd, win->mbar.wid);
	for (i= 0; i < win->nmenus; ++i) {
		char *title= win->menulist[i]->title;
		int len= strlen(title);
		XDrawString(_wd, win->mbar.wid, win->gc, x, y, title, len);
		x += XTextWidth(_wmf, title, len) + titledist;
	}
}

static void
inverttitle(win, it)
	WINDOW *win;
	int it;
{
	int x= leftedge(win, it);
	MENU *mp= win->menulist[it];
	char *title= mp->title;
	int len= strlen(title);
	int width= XTextWidth(_wmf, title, len);
	
	_winvert(win->mbar.wid, win->gc,
		x-halftitledist, 0, width+titledist, win->mbar.height);
}

static int
leftedge(win, it)
	WINDOW *win;
	int it;
{
	int i;
	int x= titledist;
	
	for (i= 0; i < it; ++i) {
		char *title= win->menulist[i]->title;
		int len= strlen(title);
		int width= XTextWidth(_wmf, title, len);
		x += width + titledist;
	}
	
	return x;
}

/* Draw the current menu */

static void
drawmenu(win)
	WINDOW *win;
{
	int i;
	int x= halftitledist;
	int y= baseline;
	MENU *mp= win->menulist[win->curmenu];
	
	for (i= 0; i < mp->nitems; ++i) {
		char *text= mp->itemlist[i].text;
		int len= strlen(text);
		char *marker= NULL;
		if (!mp->itemlist[i].enabled) {
			if (len > 0)
				marker= "-";
		}
		else if (mp->itemlist[i].checked)
			marker= "*";
		if (marker != NULL) {
			int width= XTextWidth(_wmf, marker, 1);
			XDrawString(_wd, win->mwin.wid, win->gc,
				(x-width)/2, y, marker, 1);
		}
		XDrawString(_wd, win->mwin.wid, win->gc, x, y, text, len);
		text= mp->itemlist[i].sctext;
		if (text != NULL) {
			int width;
			len= strlen(text);
			width= XTextWidth(_wmf, text, len);
			XDrawString(_wd, win->mwin.wid, win->gc,
				win->mwin.width - width - halftitledist, y,
				text, len);
		}
		y += lineheight;
	}
}

/* Create the window for the menu.
   It is a direct child of the outer window, but aligned with the
   top of the inner window */

static void
makemenu(win)
	WINDOW *win;
{
	int i;
	int maxwidth= 0;
	MENU *mp= win->menulist[win->curmenu];
	Window child_dummy;
	
	/* Compute the maximum item width.
	   Item width is the sum of:
	   - 1/2 title width (left margin, also used for tick marks)
	   - text width of item text
	   - if there is a shortcut:
	   	- shortcutdist (some space between text and shortcut)
	   	- text width of shortcut text
	   - 1/2 title width (right margin)
	*/

	for (i= 0; i < mp->nitems; ++i) {
		char *text= mp->itemlist[i].text;
		int len= strlen(text);
		int width= XTextWidth(_wmf, text, len);
		text= mp->itemlist[i].sctext;
		if (text != NULL) {
			len= strlen(text);
			width += XTextWidth(_wmf, text, len) + shortcutdist;
		}
		if (width > maxwidth)
			maxwidth= width;
	}
	
	win->mwin.width= maxwidth + titledist;
	win->mwin.height= mp->nitems * lineheight + 1;
	CLIPMAX(win->mwin.width, WidthOfScreen(_ws));
	CLIPMAX(win->mwin.height, HeightOfScreen(_ws));
	
	if (!XTranslateCoordinates(_wd, win->wo.wid,
		RootWindowOfScreen(_ws),
		leftedge(win, win->curmenu) -halftitledist + win->mbar.x,
		win->wi.y,
		&win->mwin.x,
		&win->mwin.y,
		&child_dummy)) {
		
		_wwarning("makemenu: XTranslateCoordinates failed");
		win->mwin.x = win->mwin.y = 0;
	}
	
	CLIPMAX(win->mwin.x, WidthOfScreen(_ws) - win->mwin.width);
	CLIPMAX(win->mwin.y, HeightOfScreen(_ws) - win->mwin.height);
	CLIPMIN(win->mwin.x, 0);
	CLIPMIN(win->mwin.y, 0);
	
	win->mwin.dirty= TRUE;
	win->mwin.border= IBORDER;
	
	(void) _wcreate1(&win->mwin, RootWindowOfScreen(_ws), XC_arrow, FALSE,
		win->fgo, win->bgo, 1);
	_wsaveunder(&win->mwin, True);
	XMapWindow(_wd, win->mwin.wid);
	i = XGrabPointer(_wd,		/* display */
		win->mwin.wid,		/* grab_window */
		False,			/* owner_events */
		ButtonMotionMask | ButtonReleaseMask,
					/* event_masks */
		GrabModeAsync,		/* pointer_mode */
		GrabModeAsync,		/* keyboard_mode */
		None,			/* confine_to */
		None,			/* cursor */
		_w_lasttime		/* timestamp */
		);
	if (i != GrabSuccess) {
		_wwarning("makemenu: XGrabPointer failed (err %d)", i);
		/* Didn't get the grab -- forget about it */
	}
}

/* Handle mouse state change in menu bar */

_whitmbar(bsp, ep)
	struct button_state *bsp;
	EVENT *ep;
{
	WINDOW *win= bsp->win;

	if (win->curmenu >= 0) {
		/* We already created a menu.
		   This is probably an event that was queued before
		   the menu window was created. */
		_wdebug(1, "_whitmbar: mouse in mwin");
	}
	
	if (!bsp->down)
		hidemenu(win);
	else if (bsp->y >= 0 && bsp->y <= win->mbar.height &&
			bsp->x >= 0 && bsp->x <= win->mbar.width)
		showmenu(win, whichmenu(win, bsp->x));
}

/* Handle mouse state change in menu.
   (This is called with a fake button state from _whitmbar,
   since the menu bar has grabbed the pointer) */

/*static*/ /* Now called from event.c */
_whitmwin(bsp, ep)
	struct button_state *bsp;
	EVENT *ep;
{
	WINDOW *win= bsp->win;
	MENU *mp= win->menulist[win->curmenu];
	int it;
	
	if (bsp->x >= 0 && bsp->x <= win->mwin.width)
		it= whichitem(mp, bsp->y);
	else
		it= NOITEM;
	_wdebug(5, "_whitmwin: hit item %d", it);
	hiliteitem(win, it);
	
	if (!bsp->down) {
		hidemenu(win);
		XFlush(_wd); /* Show it right now */
		if (it >= 0) {
			if (mp->id == 0) {
				ep->type= WE_CLOSE;
				ep->window= win;
			}
			else {
				ep->type= WE_MENU;
				ep->u.m.id= mp->id;
				ep->u.m.item= it;
				ep->window= win;
			}
		}
	}
}

/* Show and hide menus */

static void
showmenu(win, newmenu)
	WINDOW *win;
	int newmenu;
{
	if (newmenu != win->curmenu) {
		hidemenu(win);
		if (newmenu >= 0) {
			win->curmenu= newmenu;
			win->curitem= NOITEM;
			inverttitle(win, win->curmenu);
			makemenu(win);
			drawmenu(win);
		}
	}
}

static void
hidemenu(win)
	WINDOW *win;
{
	if (win->curmenu >= 0) {
		hiliteitem(win, NOITEM);
		XDestroyWindow(_wd, win->mwin.wid);
		win->mwin.wid= 0;
		inverttitle(win, win->curmenu);
		win->curmenu= NOMENU;
	}
}

/* Compute which menu was hit */

static int
whichmenu(win, xhit)
	WINDOW *win;
	int xhit;
{
	int i;
	int x= halftitledist;
	
	if (xhit < x)
		return NOMENU;
	
	for (i= 0; i < win->nmenus; ++i) {
		char *title= win->menulist[i]->title;
		int len= strlen(title);
		x += XTextWidth(_wmf, title, len) + titledist;
		if (xhit < x)
			return i;
	}
	
	return NOMENU;
}

/* (Un)hilite the given menu item */

static void
hiliteitem(win, it)
	WINDOW *win;
	int it;
{
	if (it != win->curitem) {
		if (win->curitem >= 0)
			invertitem(win, win->curitem);
		if (it >= 0)
			invertitem(win, it);
		win->curitem= it;
	}
}

static void
invertitem(win, it)
	WINDOW *win;
	int it;
{
	int top, size;
	
	size= lineheight;
	top= it*size;
	_winvert(win->mwin.wid, win->gc, 0, top, win->mwin.width, size);
}

/* Compute which item was hit */

static int
whichitem(mp, yhit)
	MENU *mp;
	int yhit;
{
	int it= yhit < 0 ? NOITEM : yhit / lineheight;
	
	_wdebug(4, "whichitem: yhit=%d, it=%d", yhit, it);
	if (it >= 0 && it < mp->nitems && mp->itemlist[it].enabled)
		return it;
	else
		return NOITEM;
}

/* Generate a menu selection event from a meta-key press.
   *ep has the window already filled in. */

bool
_w_menukey(c, ep)
	int c;
	EVENT *ep;
{
	WINDOW *win= ep->window;
	int i;
	int altc;
	bool althit= FALSE;
	
	c &= 0xff;
	
	if (islower(c))
		altc= toupper(c);
	else if (isupper(c))
		altc= tolower(c);
	else
		altc= 0;
	
	for (i= 0; i < win->nmenus; ++i) {
		MENU *mp= win->menulist[i];
		int j;
		for (j= 0; j < mp->nitems; ++j) {
			if (mp->itemlist[j].shortcut == c) {
				ep->type= WE_MENU;
				ep->u.m.id= mp->id;
				ep->u.m.item= j;
				return TRUE;
			}
			else if (altc != 0 && !althit &&
				mp->itemlist[j].shortcut == altc) {
				ep->u.m.id= mp->id;
				ep->u.m.item= j;
				althit= TRUE;
			}
		}
	}
	if (althit)
		ep->type= WE_MENU;
	else {
		/* Return a WE_KEY event with the meta bit set */
		ep->type = WE_KEY;
		ep->u.key.code = c;
		ep->u.key.mask = WM_META;
	}
	return TRUE;
}

/* Delete all menus -- called by wdone().
   (In fact local menus aren't deleted since they aren't in the menu list). */

_wkillmenus()
{
	while (nmenus > 0)
		wmenudelete(menulist[nmenus-1]);
	if (sysmenu != NULL) {
		wmenudelete(sysmenu);
		sysmenu= NULL;
	}
}
