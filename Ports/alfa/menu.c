/* TERMCAP STDWIN -- MENUS. */

#include "alfa.h"

static bool deflocal= FALSE;	/* Default menu state */

static struct menubar gmenus;	/* All global menus */
static struct menubar lmenus;	/* All local menus */

static MENU *sysmenu;		/* Window selection commands */
static MENU *curmenu;

/* Forward */
static int bufappend();

static void
addtobar(mbp, mp)
	struct menubar *mbp;
	MENU *mp;
{
	int i;
	
	for (i= 0; i < mbp->nmenus; ++i) {
		if (mp == mbp->menulist[i])
			return; /* Already attached */
	}
	L_APPEND(mbp->nmenus, mbp->menulist, MENU *, mp);
}

static void
delfrombar(mbp, mp)
	struct menubar *mbp;
	MENU *mp;
{
	int i;
	
	for (i= 0; i < mbp->nmenus; ++i) {
		if (mp == mbp->menulist[i]) {
			L_REMOVE(mbp->nmenus, mbp->menulist,
				MENU *, i);
			break;
		}
	}
}

_winitmenus()
{
	sysmenu= wmenucreate(0, "Windows");
	wmenuadditem(sysmenu, "Previous Window", -1);
	wmenuadditem(sysmenu, "Next Window", -1);
#ifndef EM
	/* Somehow, Euromath doesn't like this?!? */
	wmenuadditem(sysmenu, "Close Window", -1);
	wmenuadditem(sysmenu, "(left)", -1);
	wmenuadditem(sysmenu, "(right)", -1);
	wmenuadditem(sysmenu, "(up)", -1);
	wmenuadditem(sysmenu, "(down)", -1);
	wmenuadditem(sysmenu, "(cancel)", -1);
	wmenuadditem(sysmenu, "(backspace)", -1);
	wmenuadditem(sysmenu, "(tab)", -1);
	wmenuadditem(sysmenu, "(return)", -1);
#endif
	/* Shortcuts are compiled in the key map! */
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
	mp->local= deflocal;
	mp->dirty= TRUE;
	mp->left= mp->right= 0;
	L_INIT(mp->nitems, mp->itemlist);
	addtobar(mp->local ? &lmenus : &gmenus, mp);
	if (!mp->local)
		menubarchanged();
	return mp;
}

void
wmenudelete(mp)
	MENU *mp;
{
	int i;
	
	if (mp->local) {
		for (i= 0; i < MAXWINDOWS; ++i) {
			if (winlist[i].open)
				delfrombar(&winlist[i].mbar, mp);
		}
	}
	delfrombar(mp->local ? &lmenus : &gmenus, mp);
	for (i= 0; i < mp->nitems; ++i) {
		FREE(mp->itemlist[i].text);
		FREE(mp->itemlist[i].shortcut);
	}
	L_DEALLOC(mp->nitems, mp->itemlist);
	FREE(mp);
	menubarchanged();
}

int
wmenuadditem(mp, text, shortcut)
	MENU *mp;
	char *text;
	int shortcut;
{
	struct item it;
	
	mp->dirty= TRUE;
	it.text= strdup(text);
#ifdef EM
/* need to add the shortcut now, otherwise it will be taken from
   the keymap whenever the menu is displayed, which might be after
   opening another window and overwriting the keymap entry.
   In the AddMenuEntry code I overwrite the shortcut anyway. */
/* I don't understand this --Guido */
	if(shortcut >= 32) {
		it.shortcut=(char *)malloc(8);	
		sprintf(it.shortcut,"ESC %c", shortcut);
	} else 
#endif
	it.shortcut= NULL;
	it.enabled= (text != NULL && *text != EOS);
	it.checked= FALSE;
	L_APPEND(mp->nitems, mp->itemlist, struct item, it);
	if (shortcut >= 0)
		wsetmetakey(mp->id, mp->nitems-1, shortcut);
	return mp->nitems-1;
}

void
wmenusetitem(mp, item, text)
	MENU *mp;
	int item;
	char *text;
{
	if (item < 0 || item >= mp->nitems)
		return;
	mp->dirty= TRUE;
	FREE(mp->itemlist[item].text);
	mp->itemlist[item].text= strdup(text);
	mp->itemlist[item].enabled= (text != NULL && *text != EOS);
}

void
wmenuenable(mp, item, flag)
	MENU *mp;
	int item;
	int flag;
{
	if (item < 0 || item >= mp->nitems)
		return;
	mp->itemlist[item].enabled= flag;
}

void
wmenucheck(mp, item, flag)
	MENU *mp;
	int item;
	int flag;
{
	if (item < 0 || item >= mp->nitems)
		return;
	mp->itemlist[item].checked= flag;
}

void
wmenuattach(win, mp)
	WINDOW *win;
	MENU *mp;
{
	if (!mp->local)
		return;
	addtobar(&win->mbar, mp);
	if (win == front)
		menubarchanged();
}

void
wmenudetach(win, mp)
	WINDOW *win;
	MENU *mp;
{
	if (!mp->local)
		return;
	delfrombar(&win->mbar, mp);
	if (win == front)
		menubarchanged();
}

void
wmenusetdeflocal(local)
	bool local;
{
	deflocal= local;
}

/* Interface routines for the rest of the library. */

void
initmenubar(mb)
	struct menubar *mb;
{
	L_INIT(mb->nmenus, mb->menulist);
}

void
killmenubar(mb)
	struct menubar *mb;
{
	L_DEALLOC(mb->nmenus, mb->menulist);
}

void
drawmenubar() /* This is part of the syswin draw procedure! */
{
	WINDOW *win= front;
	char buf[256];
	int k= 0;
	int i;
	
	buf[0]= EOS;
	for (i= 0; i < gmenus.nmenus; ++i)
		k= bufappend(buf, k, gmenus.menulist[i]);
	if (win != NULL) {
		for (i= 0; i < win->mbar.nmenus; ++i)
			k= bufappend(buf, k, win->mbar.menulist[i]);
	}
	wdrawtext(0, 0, buf, -1);
}

static int
bufappend(buf, pos, mp)
	char *buf;
	int pos;
	MENU *mp;
{
	if (pos > 0) {
		buf[pos++]= ' ';
		buf[pos++]= ' ';
	}
	mp->left= pos;
	strcpy(buf+pos, mp->title);
	pos += strlen(buf+pos);
	mp->right= pos;
	return pos;
}

/* TO DO:
   - highlight current menu title, current menu item
   - remember last menu and item
   - Allow GOTO to select menus or menu items
     (m-down on mbar opens the menu; m-up on item selects the item)
*/

static void menudraw(), menuitemdraw(), menucalcwidth();
static bool handleevt(), handlecmd(), handlemenu();
static int calcleft();

static leftblank;

static bool		curlocal;
static struct menubar *	curmbar;
static int		curimenu;
static int		curitem;
static int		lowest;

static bool
firstmenu()
{
	curlocal= FALSE;
	curmbar= &gmenus;
	curimenu= 0;
	
	if (curmbar->nmenus == 0) {
		curmbar= &lmenus;
		curlocal= TRUE;
		if (curmbar->nmenus == 0) {
			wmessage("No menus defined");
			return FALSE;
		}
	}
	curmenu= curmbar->menulist[curimenu];
	curitem= 0;
	menudraw();
	return TRUE;
}

static void
nextmenu()
{
	WINDOW *win=front; /* added mcv */
/* the menus are now taken from the window's menubar, instead
   of the menubar for *all* local menus */

	++curimenu;
	while (curimenu >= curmbar->nmenus) {
		curlocal= !curlocal;
		curmbar= curlocal ? 
				 &win->mbar /* changed mcv, was: &lmenus*/ : 
				 &gmenus;
		curimenu= 0;
	}
	curmenu= curmbar->menulist[curimenu];
	curitem= 0;
	menudraw();
}

static void
prevmenu()
{
	WINDOW *win=front;  /* added mcv */
/* the menus are now taken from the window's menubar, instead
   of the menubar for *all* local menus */

	--curimenu;
	while (curimenu < 0) {
		curlocal= !curlocal;
		curmbar= curlocal ? 
				 &win->mbar /* changed mcv, was: &lmenus*/ : 
				 &gmenus;
		curimenu= curmbar->nmenus - 1;
	}
	curmenu= curmbar->menulist[curimenu];
	curitem= 0;
	menudraw();
}

static void
nextitem()
{
	++curitem;
	if (curitem >= curmenu->nitems)
		curitem= 0;
	trmsync(curitem+1, curmenu->left);
}

static void
previtem()
{
	--curitem;
	if (curitem < 0)
		curitem= curmenu->nitems - 1;
	trmsync(curitem+1, curmenu->left);
}

static void
selectitem(ep)
	EVENT *ep;
{
	ep->type= WE_NULL;
	if (curitem >= curmenu->nitems || !curmenu->itemlist[curitem].enabled)
		return;
	ep->type= WE_MENU;
	ep->u.m.id= curmenu->id;
	ep->u.m.item= curitem;
}

void
menuselect(ep)
	EVENT *ep;
{
	leftblank= columns;
	lowest= 0;
	wmessage((char *)NULL);
	if (!firstmenu())
		return;
	for (;;) {
		wsysevent(ep, 1);
		if (handleevt(ep))
			break;
	}
}

static bool
handleevt(ep)
	EVENT *ep;
{
	switch (ep->type) {
	
	case WE_MENU:
		return handlemenu(ep);
	
	case WE_COMMAND:
		return handlecmd(ep);
	
	default:
		trmbell();
		return FALSE;
	
	}
}

static bool
handlecmd(ep)
	EVENT *ep;
{
	switch (ep->u.command) {
	
	default:
		trmbell();
		return FALSE;
	
	case WC_RETURN:
		selectitem(ep);
		if (curmenu == sysmenu)
			wsyscommand(ep);
		return TRUE;
	
	case WC_LEFT:
		prevmenu();
		break;
	
	case WC_RIGHT:
		nextmenu();
		break;
	
	case WC_UP:
		previtem();
		break;
	
	case WC_DOWN:
		nextitem();
		break;
	
	case WC_CANCEL:
		ep->type= WE_NULL;
		return TRUE;
	
	}
	return FALSE;
}

static bool
handlemenu(ep)
	EVENT *ep;
{
	if (ep->u.m.id != 0)
		return TRUE;
	switch (ep->u.m.item) {
	
	case SUSPEND_PROC:
		_wsuspend();
		menudraw();
		break;
	
	case REDRAW_SCREEN:
		_wredraw();
		menudraw();
		break;
	
	case MENU_CALL:
		ep->type= WE_NULL;
		return TRUE;

	default:
		if (ep->u.m.item <= LAST_CMD) {
			wsyscommand(ep);
			if (ep->type == WE_COMMAND)
				return handlecmd(ep);
			else
				return TRUE;
		}
		else {
			trmbell();
			return FALSE;
		}
	
	}
	return FALSE;
}

static void
menudraw()
{
	MENU *mp= curmenu;
	int left;
	int width;
	int i;
	
	wupdate(syswin);
	if (mp->dirty)
		menucalcwidth(mp);
	left= calcleft(mp);
	width= mp->maxwidth;
	if (left + width > columns)
		width= columns - left;
	for (i= 0; i < mp->nitems; ++i)
		menuitemdraw(i+1, left, &mp->itemlist[i], width);
	if (i+1 > lowest) {
		lowest= i+1;
		if (lowest < lines)
			uptodate[lowest]= FALSE;
	}
	trmputdata(i+1, lowest, 0, "", (char *)0);
	leftblank= left;
	trmsync(curitem+1, mp->left);
}

static int
calcleft(mp)
	MENU *mp;
{
	int left= columns - mp->maxwidth;
	
	if (left > mp->left)
		left= mp->left;
	if (left < 0)
		left= 0;
	if (left-3 < leftblank) {
		leftblank= left-3;
		if (leftblank < 4)
			leftblank= 0;
	}
	return left;
}

static void
menuitemdraw(line, left, ip, width)
	int line, left;
	struct item *ip;
	int width;
{
	char buf[256];
	int margin= left-leftblank;
	
	buf[0]= EOS;
	if (ip->text != NULL && *ip->text != EOS) {
		int space;
		char *p= buf;
		for (space= margin; space-- > 0; )
			*p++ = ' ';
		if (ip->checked && margin >= 2)
			p[-2]= '*';
		strcpy(p, ip->text);
		p += strlen(p);
		if (!ip->enabled && margin >= 1 &&
			ip->text != NULL && ip->text[0] != EOS) {
			buf[margin-1]= '(';
			*p++= ')';
			*p= '\0';
		}
		if (ip->shortcut != NULL && *ip->shortcut != EOS) {
			space= width - (p - buf - margin)
				- strlen(ip->shortcut);
			if (space <= 0)
				space= 2;
			while (--space >= 0)
				*p++ = ' ';
			strcpy(p, ip->shortcut);
		}
	}
	/* This was added because brackets and stars from disabled/marked
	   items on the first menu (after the sysmenu) weren't removed
	   from the screen.  I haven't tried to fix this in a more
	   efficient manner. */
	trmputdata(line, line, 0, "", (char *)0);
	trmputdata(line, line, leftblank, buf, (char *)0);
	uptodate[line]= FALSE;
}

static void
menucalcwidth(mp)
	MENU *mp;
{
	int i;
	int width= 0;
	
	for (i= 0; i < mp->nitems; ++i) {
		struct item *ip= &mp->itemlist[i];
		char *text= ip->text;
		if (text != NULL && *text != EOS) {
			int w= strlen(text);
			if (ip->shortcut == NULL) {
				char buf[256];
				getbindings(buf, mp->id, i);
				ip->shortcut= strdup(buf);
			}
			if (ip->shortcut != NULL && *ip->shortcut != EOS)
				w += 2 + strlen(ip->shortcut);
			if (w > width)
				width= w;
		}
	}
	mp->maxwidth= width;
	mp->dirty= FALSE;
}
