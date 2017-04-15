/* STDWIN -- EVENTS. */

#include "alfa.h"

/* There are three levels of event processing.
   The lowest ('raw') level only inputs single characters.
   The middle ('sys') level translates sequences of keys to menu shortcuts
   and 'commands'.
   The highest ('appl') level executes certain commands and can call the
   interactive menu selection code.  It also sets the window parameter.
   For each level there are variants to wait or poll.
   Additional routines to get from one level to another:
   wmapchar: inspect key map and possibly convert char event
   wsyscommand: execute/transform system menu

/* Forward */
static bool wevent();

/* Raw event -- read one character.  If blocking is FALSE,
   don't read if trmavail() returns -1, meaning it can't tell whether
   there is input or not.  In that case only synchronous input works. */

static bool
wrawevent(ep, blocking)
	EVENT *ep;
	bool blocking;
{
	if (!blocking && trmavail() <= 0)
		return FALSE;
	ep->u.character= trminput();
	if (ep->u.character < 0) {
		ep->type= WE_NULL;
		return FALSE;
	}
	else {
		ep->type= WE_CHAR;
		return TRUE;
	}
}

static void wmapchar(); /* Forward */

/* System event -- read a character and translate using the key map. */

bool
wsysevent(ep, blocking)
	EVENT *ep;
	bool blocking;
{
	struct keymap mapentry;
	
	if (!wrawevent(ep, blocking))
		return FALSE;
	/* Using a temporary to avoid a tahoe C compiler bug */
	mapentry= _wprimap[ep->u.character & 0xff];
	wmapchar(ep, mapentry);
	return ep->type != WE_NULL;
}

/* Key map translation from raw event.
   This may read more characters if the start of a multi-key sequence is
   found. */

static void
wmapchar(ep, mapentry)
	EVENT *ep;
	struct keymap mapentry;
{
	switch (mapentry.type) {
	
	case SHORTCUT:
		ep->type= WE_MENU;
		ep->u.m.id= mapentry.id;
		ep->u.m.item= mapentry.item;
		break;
	
	case SECONDARY:
		if (wrawevent(ep, TRUE)) {
			struct keymap *map;
			struct keymap *altmap= NULL;
			for (map= _wsecmap[mapentry.id];
				map->type != SENTINEL;
				++map) {
				if (map->key == ep->u.character) {
					wmapchar(ep, *map);
					return;
				}
				else if (islower(ep->u.character) &&
					map->key == toupper(ep->u.character))
					altmap= map;
			}
			if (altmap != NULL) {
				wmapchar(ep, *altmap);
				return;
			}
		}
		trmbell();
		ep->type= WE_NULL;
		break;
	
	}
}

/* XXX These should be gotten from some $TERM-dependent configuration file */
static char *sense_str = "";
static char *format_str = "";

void
wgoto(ep, type)
	EVENT *ep;
	int type;
{
	int h, v;
	
	trmsense(sense_str, format_str, &v, &h);
	if (v >= 0) {
		WINDOW *win;
		for (win= &winlist[1]; win < &winlist[MAXWINDOWS]; ++win) {
			if (win->open && v >= win->top && v < win->bottom) {
				if (win == front) {
					ep->type= type;
					ep->u.where.h= h;
					ep->u.where.v= v + win->offset;
					ep->u.where.clicks= 1;
					ep->u.where.button= 1;
					ep->u.where.mask=
						(type == WE_MOUSE_UP ? 0 : 1);
					return;
				}
				wsetactive(win);
				break;
			}
		}
	}
	ep->type= WE_NULL;
}

void
wmouseup(ep)
	EVENT *ep;
{
	int h, v;
	
	trmsense(sense_str, format_str, &v, &h);
	if (v < 0)
		ep->type= WE_NULL;
	else {
		ep->type= WE_MOUSE_UP;
		ep->u.where.h= h;
		ep->u.where.v= v + front->offset;
		ep->u.where.clicks= 1;
		ep->u.where.button= 1;
		ep->u.where.mask= 0;
	}
}

static void wdoupdates(); /* Forward */

/* Application event. */

static EVENT ebuf;

void
wungetevent(ep)
	EVENT *ep;
{
	ebuf= *ep;
}

bool
wpollevent(ep)
	EVENT *ep;
{
	return wevent(ep, FALSE);
}

void
wgetevent(ep)
	EVENT *ep;
{
	(void) wevent(ep, TRUE);
}

static bool
wevent(ep, blocking)
	EVENT *ep;
	bool blocking;
{
	static int lasttype= WE_NULL;
	
	if (ebuf.type != WE_NULL) {
		*ep= ebuf;
		ebuf.type= WE_NULL;
		return TRUE;
	}
	
	ep->type= WE_NULL;
	ep->window= NULL;
	if (lasttype == WE_MOUSE_DOWN)
		wmouseup(ep);
	while (ep->type == WE_NULL) {
		if (front != wasfront) {
			if (wasfront != NULL) {
				ep->type= WE_DEACTIVATE;
				ep->window= wasfront;
				wasfront= NULL;
			}
			else {
				menubarchanged();
				ep->type= WE_ACTIVATE;
				wasfront= front;
			}
		}
		else {
			if (trmavail() <= 0)
				wdoupdates(ep);
			if (ep->type == WE_NULL) {
				/* Remove this call if no select() exists */
				if (_w_checktimer(ep, blocking))
					break;
				if (!wsysevent(ep, blocking)) {
					ep->type= WE_NULL;
					break;
				}
				if (ep->type == WE_MENU && ep->u.m.id == 0)
					wsyscommand(ep);
			}
		}
	}
	if (ep->window == NULL && front != syswin)
		ep->window= front;
	if (ep->type == WE_COMMAND && ep->u.command == WC_CLOSE)
		ep->type= WE_CLOSE; /* Too hard to do earlier... */
	lasttype= ep->type;
	return ep->type != WE_NULL;
}

void
wsyscommand(ep)
	EVENT *ep;
{
	ep->type= WE_NULL;
	
	switch (ep->u.m.item) {
	
	default:
		if (ep->u.m.item <= LAST_CMD) {
			ep->type= WE_COMMAND;
			ep->u.command= ep->u.m.item - FIRST_CMD;
		}
		break;
	
	case NEXT_WIN:
		_wselnext();
		break;
	
	case PREV_WIN:
		_wselprev();
		break;
	
	case SUSPEND_PROC:
		_wsuspend();
		break;
	
	case REDRAW_SCREEN:
		_wredraw();
		break;
	
	case MOUSE_DOWN:
	case MOUSE_UP:
		wgoto(ep, ep->u.m.item - MOUSE_DOWN + WE_MOUSE_DOWN);
		break;
	
	case LITERAL_NEXT:
		_wlitnext(ep);
		break;
	
	case MENU_CALL:
		menuselect(ep);
		break;
	
	}
}

/* Flush output and place cursor. */

void
wflush()
{
	int y= front->curv - front->offset;
	int x= front->curh;
	
	if (front->curv < 0 || y < front->top || y >= front->bottom) {
		y= front->top;
		if (y > 0)
			--y;
		x= 0;
	}
	else if (x < 0)
		x= 0;
	else if (x >= columns)
		x= columns-1;
	trmsync(y, x);
}

/* Process pending window updates.
   If a window has no draw procedure, a WE_DRAW event is generated instead. */

static void
wdoupdates(ep)
	EVENT *ep;
{
	WINDOW *win;
	int left, top, right, bottom;
	
	for (win= winlist; win < &winlist[MAXWINDOWS]; ++win) {
		if (win->open) {
			if (win->top > 0 && !uptodate[win->top - 1])
				wdrawtitle(win);
			if (wgetchange(win, &left, &top, &right, &bottom)) {
				if (win->drawproc == NULL) {
					ep->type= WE_DRAW;
					ep->window= win;
					ep->u.area.left= left;
					ep->u.area.top= top;
					ep->u.area.right= right;
					ep->u.area.bottom= bottom;
					return;
				}
				else {
					wbegindrawing(win);
					(*win->drawproc)(win,
						left, top, right, bottom);
					wenddrawing(win);
				}
			}
			if(win->resized) {
				win->resized = FALSE;
				ep->type = WE_SIZE;
				ep->window = win;
				return;
			}
		}
	}
	wflush();
}

void
_wlitnext(ep)
	EVENT *ep;
{
	(void) wrawevent(ep, TRUE);
}
