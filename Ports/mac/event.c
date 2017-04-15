/* MAC STDWIN -- EVENT HANDLING. */

#include "macwin.h"
#include <Menus.h>
#include <Desk.h>
#include <ToolUtils.h>
#include <AppleEvents.h>
#ifdef CONSOLE_IO
#include <console.h> /* See do_update */
#endif

void (*_w_idle_proc)();	/* Function to call in idle loop */

void (*_w_high_level_event_proc)(EventRecord *, EVENT *);
			/* Function to call for High Level Events */

WINDOW *active= NULL;	/* The active window */
			/* XXX should be a less obvious name */
bool _wm_down;		/* Set if mouse is down in content rect */

static EventRecord e;	/* Global, so it's accessible to all subroutines */
			/* XXX the name is too short */

/* Function prototypes */

STATIC void make_mouse_event _ARGS((EVENT *ep, Point *pwhere));
STATIC void do_idle _ARGS((EVENT *ep));
STATIC void do_update _ARGS((EVENT *ep));
STATIC void do_mouse_down _ARGS((EVENT *ep));
STATIC void do_mouse_up _ARGS((EVENT *ep));
STATIC void do_key _ARGS((EVENT *ep));
STATIC void do_activate _ARGS((EVENT *ep));
STATIC void do_disk _ARGS((EVENT *ep));
STATIC void do_highlevel _ARGS((EVENT *ep));
STATIC void activate _ARGS((WINDOW *win));
STATIC void deactivate _ARGS((void));

STATIC void do_click _ARGS((EVENT *ep, WindowPtr w));
STATIC void do_unclick _ARGS((EVENT *ep));
STATIC void do_drag _ARGS((WindowPtr w));
STATIC void do_grow _ARGS((EVENT *ep, WindowPtr w));
STATIC void do_goaway _ARGS((EVENT *ep, WindowPtr w));
STATIC void do_zoom _ARGS((EVENT *ep, WindowPtr w, int code));
STATIC void do_size _ARGS((EVENT *ep, WindowPtr w));

static EVENT pushback= {WE_NULL};

void
wungetevent(ep)
	EVENT *ep;
{
	pushback= *ep;
}

static void
wwaitevent(ep, wait)
	EVENT *ep;
	bool wait;
{
	_wfreeclip();
	if (pushback.type != WE_NULL) {
		*ep= pushback;
		pushback.type= WE_NULL;
		return;
	}
	
	if (_wmenuhilite) {
		HiliteMenu(0);
		_wmenuhilite= FALSE;
	}
	
	if (active == NULL)
		set_arrow();
	else if (active->w != FrontWindow()) {
		/* Somehow we missed a deactivation event.
		   Fake one now. */
		ep->type= WE_DEACTIVATE;
		ep->window= active;
		deactivate();
		return;
	}
	
	ep->type= WE_NULL;
	ep->window= NULL;
	
	do {
		if (!GetNextEvent(everyEvent, &e)) {
			if (e.what == nullEvent) {
				if (wait) do_idle(ep);
				else return;
			}
		}
		else {
			switch (e.what) {
			case mouseDown:
				do_mouse_down(ep);
				break;
			case mouseUp:
				do_mouse_up(ep);
				break;
			case keyDown:
			case autoKey:
				do_key(ep);
				break;
			case updateEvt:
				do_update(ep);
				break;
			case diskEvt:
				do_disk(ep);
				break;
			case activateEvt:
				do_activate(ep);
				break;
			case kHighLevelEvent:
				do_highlevel(ep);
				break;
			}
		}
	} while (ep->type == WE_NULL);
	
	if (ep->window == NULL)
		ep->window= whichwin(FrontWindow());
	if (!_wm_down)
		set_watch();
}

int
wpollevent(ep)
        EVENT *ep;
{
        ep->type = WE_NULL;
        wwaitevent(ep, FALSE);
        return ep->type != WE_NULL;
}

void
wgetevent(ep)
        EVENT *ep;
{
        wwaitevent(ep, TRUE);
}

static void
do_idle(ep)
	EVENT *ep;
{	
	if (checktimer(ep))
		return;
	
	if (_w_idle_proc != NULL)
		(*_w_idle_proc)();
	
	/* The user idle proc may have called wungetevent: */
	if (pushback.type != WE_NULL) {
		*ep= pushback;
		pushback.type= WE_NULL;
		return;
	}
	
	SystemTask();
	
	if (active != NULL) {
		Point where;
		Rect r;
		
		where= e.where;
		SetPort(active->w);
		GlobalToLocal(&where);
		if (_wm_down) {
			autoscroll(active, where.h, where.v);
			make_mouse_event(ep, &where);
			return;
		}
		getwinrect(active, &r);
		if (PtInRect(PASSPOINT where, &r)) {
			if (e.modifiers & optionKey)
				set_hand();
			else
				set_applcursor();
		}
		else
			set_arrow();
		blinkcaret(active);
	}
}

static void
do_update(ep)
	EVENT *ep;
{
	WINDOW *win;
	Rect r;
	
	win= whichwin((WindowPtr) e.message);
	if (win == NULL) {
		/* Update event for a window not created by STDWIN
		   (not a Desk Accessory -- these are taken care of
		   by GetNextEvent or at some other secret place.)
		   This is a problem: if we ignore it, it will come
		   back forever, so we'll never be idle again. */
#ifdef CONSOLE_IO
		/* Most likely, under THINK C, it is the console
		   window.  We can force the window to repaint itself
		   by calling any console function.  A rather harmless
		   one is cgetxy.  Use stderr as the one least likely
		   to be redirected. */
		int x, y;
		if (stderr->window)
			cgetxy(&x, &y, stderr);
#endif
		return;
	}
	_wupdate(win, &r);
	if (win->drawproc == NULL && !EmptyRect(&r)) {
		ep->type= WE_DRAW;
		ep->window= win;
		ep->u.area.left= r.left;
		ep->u.area.top= r.top;
		ep->u.area.right= r.right;
		ep->u.area.bottom= r.bottom;
	}
}

static void
do_mouse_down(ep)
	EVENT *ep;
{
	WindowPtr w;
	int code= FindWindow(PASSPOINT e.where, &w);
	
	if (code != inContent && code != inSysWindow)
		set_arrow();
	switch (code) {
	case inMenuBar:
		_wdo_menu(ep, MenuSelect(PASSPOINT e.where));	
		break;
	case inSysWindow:
		SystemClick(&e, w);
		break;
	case inContent:
		do_click(ep, w);
		break;
	case inDrag:
		do_drag(w);
		break;
	case inGrow:
		do_grow(ep, w);
		break;
	case inGoAway:
		do_goaway(ep, w);
		break;
	case inZoomIn:
	case inZoomOut:
		do_zoom(ep, w, code);
		break;
	}
}

static void
do_mouse_up(ep)
	EVENT *ep;
{
	do_unclick(ep);
}

static void
do_key(ep)
	EVENT *ep;
{
	int c= e.message & charCodeMask;
	
	if (e.modifiers & cmdKey) {
		if (c == '.') {
			ep->type= WE_COMMAND;
			ep->u.command= WC_CANCEL;
		}
		else {
			long menu_item= MenuKey(c);
			if (HiWord(menu_item) != 0) {
				_wdo_menu(ep, menu_item);
			}
			else {
				ep->type= WE_KEY;
				ep->u.key.code= c;
				ep->u.key.mask= WM_META;
				/* Should filter out arrow keys? */
			}
		}
	}
	else {
		ep->type= WE_COMMAND;
		switch (c) {
		
		default:
			ObscureCursor();
			ep->type= WE_CHAR;
			ep->u.character= c;
			break;
		
		case LEFT_ARROW:
		case RIGHT_ARROW:
		case UP_ARROW:
		case DOWN_ARROW:
			ep->u.command= c-LEFT_ARROW + WC_LEFT;
			break;
		
		case '\b':
			ObscureCursor();
			ep->u.command= WC_BACKSPACE;
			break;
		
		case '\t':
			ObscureCursor();
			ep->u.command= WC_TAB;
			break;
		
		case '\r':
		case ENTER_KEY:
			ep->u.command= WC_RETURN;
			break;
		
		}
	}
}

static void
do_disk(ep)
	EVENT *ep;
{
	/* XXX Disk events not implemented -- who cares. */
}

static void
do_highlevel(ep)
	EVENT *ep;
{
	if (_w_high_level_event_proc != NULL)
		(*_w_high_level_event_proc)(&e, ep);
}

/* XXX Need to be easier for cases where we seem to have missed events */

static void
do_activate(ep)
	EVENT *ep;
{
	WINDOW *win= whichwin((WindowPtr)e.message);
	
	if (win == NULL) {
		/* dprintf("(de)activate evt for alien window"); */
		return;
	}
	
	if (e.modifiers & activeFlag) { /* Activation */
		if (active != NULL) {
			/* Perhaps reactivation after modal dialog */
			if (active == win)
				return;
			/* If we get here we've missed a
			   deactivate event... */
			/* dprintf("activate without deactivate"); */
		}
		activate(win);
		ep->type= WE_ACTIVATE;
		ep->window= active;
	}
	else { /* Deactivation */
		if (win != active) {
			/* Spurious deactivation event.
			   This always happens when we open
			   two or more windows without intervening
			   call to wgetevent().
			   Perhaps an conscious hack in the
			   ROM to "help" programs that believe
			   windows are created active? */
			return;
		}
		ep->type= WE_DEACTIVATE;
		ep->window= active;
		deactivate();
	}
}

static void
deactivate()
{
	SetPort(active->w);
	rmcaret(active);
	hidescrollbars(active);
	rmlocalmenus(active);
	_wgrowicon(active);
	active= NULL;
	set_arrow();
}

static void
activate(win)
	WINDOW *win;
{
	if (active != NULL)
		deactivate();
	if (win != NULL) {
		SetPort(win->w);
		active= win;
		showscrollbars(win);
		addlocalmenus(active);
		valid_border(win->w); /* Avoid flicker when window pops up */
	}
}

static void
do_click(ep, w)
	EVENT *ep;
	WindowPtr w;
{
	WINDOW *win= whichwin(w);
	Point where;
	int pcode;
	ControlHandle bar;
	
	if (win == NULL) {
		/* dprintf("click in alien window"); */
		return;
	}
	if (win != active) {
		set_arrow();
		if (e.modifiers & optionKey) {
			/* Option-click sends a window behind. */
			SendBehind(w, (WindowPtr) NULL);
		}
		else
			SelectWindow(win->w);
		return;
		/* Let activate events do the rest. */
	}
	where= e.where;
	SetPort(win->w);
	GlobalToLocal(&where);
	pcode= FindControl(PASSPOINT where, w, &bar);
	if (pcode != 0) {
		set_arrow();
		do_scroll(&where, win, bar, pcode);
	}
	else {
		Rect r;
		
		getwinrect(win, &r);
		if (PtInRect(PASSPOINT where, &r)) {
			if (e.modifiers & optionKey) {
				set_hand();
				dragscroll(win,
					where.h, where.v,
					e.modifiers & shiftKey);
			}
			else {
				set_applcursor();
				make_mouse_event(ep, &where);
			}
		}
	}
}

static void
do_unclick(ep)
	EVENT *ep;
{
	if (active != NULL) {
		Point where;
		
		where= e.where;
		SetPort(active->w);
		GlobalToLocal(&where);
		make_mouse_event(ep, &where);
	}
}

static void
do_drag(w)
	WindowPtr w;
{
	if (e.modifiers & optionKey) {
		/* Nonstandard: option-click sends a window behind. */
		SendBehind(w, (WindowPtr) NULL);
	}
	else {
		Rect r;
		
		r= screen->portRect;
		r.top += MENUBARHEIGHT;
		InsetRect(&r, 4, 4);
		DragWindow(w, PASSPOINT e.where, &r);
	}
}

static void
do_grow(ep, w)
	EVENT *ep;
	WindowPtr w;
{
	Rect r;
	long reply;
	WINDOW *win = whichwin(w);
	
	/* Don't mess at all with non-stdwin windows */
	if (win == NULL)
		return;
	
	/* Set minimal window size --
	  1x1 at least, plus space needed for scroll bars */
	r.left = LSLOP + 1 + RSLOP;
	r.top = 1;
	if (win->hbar != NULL)
		r.left = 3*BAR;
	if (win->vbar != NULL)
		r.top = 3*BAR;
	if (win->vbar != NULL)
		r.left += BAR;
	if (win->hbar != NULL)
		r.top += BAR;
	
	/* Windows may become as large as the user can get them,
	   within reason -- the limit 0x7000 should avoid integer
	   overflow in QuickDraw. */
	r.right = r.bottom = 0x7000;
	
	reply= GrowWindow(w, PASSPOINT e.where, &r);
	if (reply != 0) {
		SetPort(w);
		inval_border(w);
		SizeWindow(w, LoWord(reply), HiWord(reply), TRUE);
		do_size(ep, w);
	}
}

static void
do_goaway(ep, w)
	EVENT *ep;
	WindowPtr w;
{
	/* XXX shouldn't mess at all with non-stdwin windows */
	
	if (TrackGoAway(w, PASSPOINT e.where)) {
		ep->type= WE_CLOSE;
		ep->window= whichwin(w);
	}
}

static void
do_zoom(ep, w, code)
	EVENT *ep;
	WindowPtr w;
	int code;
{
	/* XXX shouldn't mess at all with non-stdwin windows */
	
	/* This code will never be reached on a machine
	   with old (64K) ROMs, because FindWindow will
	   never return inZoomIn or inZoomOut.
	   Therefore, no check for new ROMs is necessary.
	   A warning in Inside Macintosh IV says that
	   it is necessary to make the zoomed window
	   the current GrafPort before calling ZoomWindow.
	   True enough, it fails spectacularly otherwise,
	   but still this looks like a bug to me - there
	   are no similar requirements for SizeWindow
	   or DragWindow. */
	
	SetPort(w);
	if (TrackBox(w, PASSPOINT e.where, code)) {
		inval_border(w);
		ZoomWindow(w, code, TRUE);
		do_size(ep, w);
	}
}

/* do_size assumes w is already the current grafport */

static void
do_size(ep, w)
	EVENT *ep;
	WindowPtr w;
{
	WINDOW *win= whichwin(w);
	
	if (win == NULL) {
		/* dprintf("alien window resized"); */
		return;
	}
	inval_border(w);
	movescrollbars(win);
	ep->type= WE_SIZE;
	ep->window= win;
	_wfixorigin(win);
}

void
inval_border(w)
	WindowPtr w;
{
	Rect r;
	WINDOW *win = whichwin(w);
	
	if (win->vbar != NULL) {
		r = w->portRect;
		r.left = r.right - BAR;
		InvalRect(&r);
	}
	if (win->hbar != NULL) {
		r = w->portRect;
		r.top = r.bottom - BAR;
		InvalRect(&r);
	}
}

void
valid_border(w)
	WindowPtr w;
{
	Rect r;
	WINDOW *win = whichwin(w);
	
	if (win->vbar != NULL) {
		r = w->portRect;
		r.left = r.right - BAR;
		ValidRect(&r);
	}
	if (win->hbar != NULL) {
		r = w->portRect;
		r.top = r.bottom - BAR;
		ValidRect(&r);
	}
}

/* Variables needed in click and move detection. */

static int m_h, m_v;		/* Doc. coord. of last mouse evt. */
static long m_when;		/* TickCount of last mouse evt. */
static int m_clicks;		/* N-fold click stage */

static void
make_mouse_event(ep, pwhere)
	EVENT *ep;
	Point *pwhere;		/* Mouse pos. in local coord. */
{
	WINDOW *win= active;
	int h= pwhere->h + win->orgh;
	int v= pwhere->v + win->orgv;
	int dh= h - m_h;
	int dv= v - m_v;
	int mask;
	
	if (m_clicks != 0 && dh*dh + dv*dv > CLICK_DIST*CLICK_DIST)
		m_clicks= 0;	/* Moved too much for a click */
	
	if (e.what == mouseDown) {
		if (e.when > m_when + GetDblTime())
			m_clicks= 1;
		else
			++m_clicks;
		ep->type= WE_MOUSE_DOWN;
		_wm_down= TRUE;
	}
	else if (e.what == mouseUp) {
		if (!_wm_down)
			return;
		ep->type= WE_MOUSE_UP;
		_wm_down= FALSE;
	}
	else {
		if (!_wm_down || m_clicks > 0 || (dh == 0 && dv == 0))
			return;
		ep->type= WE_MOUSE_MOVE;
	}
	mask= (ep->type == WE_MOUSE_UP) ? 0 : WM_BUTTON1;
	if (e.modifiers & cmdKey)
		mask |= WM_META;
	if (e.modifiers & shiftKey)
		mask |= WM_SHIFT;
	if (e.modifiers & alphaLock)
		mask |= WM_LOCK;
	if (e.modifiers & optionKey)
		mask |= WM_OPTION;
	if (e.modifiers & controlKey)
		mask |= WM_CONTROL;
	ep->u.where.h= m_h= h;
	ep->u.where.v= m_v= v;
	ep->u.where.clicks= m_clicks;
	ep->u.where.button= 1;
	ep->u.where.mask= mask;
	ep->window= win;
	m_when= e.when;
}

/* Reset the mouse state.
   Called when a dialog is started. */

void
_wresetmouse()
{
	_wm_down= FALSE;
}

void
wsetactive(win)
	WINDOW *win;
{
	SelectWindow(win->w);
}

WINDOW *
wgetactive()
{
	return whichwin(FrontWindow());
}
