/* X11 STDWIN -- High levent handling */

/* THIS CODE IS A MESS.  SHOULD BE RESTRUCTURED! */

#include "x11.h"
#include "llevent.h"
#include <X11/keysym.h>

static WINDOW *active;
static WINDOW *prev_active;

/* Forward */
static void change_key_event();

/* Ensure none of the window pointers refer to the given window --
   it's being closed so the pointer becomes invalid. */

_w_deactivate(win)
	WINDOW *win;
{
	if (win == active)
		active= NULL;
	if (win == _w_new_active)
		_w_new_active= NULL;
	if (win == prev_active)
		prev_active= NULL;
	if (win == _w_bs.win) {
		_w_bs.win= NULL;
		_w_bs.down= FALSE;
	}
}

WINDOW *
_w_get_last_active()
{
	if (_w_new_active != NULL)
		return _w_new_active;
	if (active != NULL)
		return active;
	if (prev_active != NULL)
		return prev_active;
	/* No window was ever active. Pick one at random. */
	return _w_somewin();
}

WINDOW *
wgetactive()
{
	return active;
}

/* wungetevent may be called from a signal handler.
   If this is the case, we must send an event to ourselves
   so it is picked up (eventually).
   There is really still a race condition:
   if wungetevent copies an event into the evsave buffer
   when wgetevent is halfway of copying one out, we lose.
   This can only be fixed with multiple buffers and I dont
   want to think about that now. */

static bool in_getevent;
EVENT _w_evsave;		/* Accessible by _w_ll_event */

void
wungetevent(ep)
	EVENT *ep;
{
	if (ep->type != WE_NULL) {
		_w_evsave= *ep;
#ifdef PIPEHACK
		if (in_getevent) {
			if (_wpipe[1] < 0)
			  _wwarning("wungetevent: can't interrupt wgetevent");
			else if (write(_wpipe[1], "x", 1) != 1)
			  _wwarning("wungetevent: pipe write failed");
			else
			  _wdebug(1, "wungetevent: wrote to pipe");
		}
#endif
	}
}

static void
_wwaitevent(ep, mayblock)
	EVENT *ep;
	bool mayblock;
{
	XEvent e;
	
	in_getevent= TRUE;
	
	if (_w_evsave.type != WE_NULL) {
		*ep= _w_evsave;
		_w_evsave.type= WE_NULL;
		in_getevent= FALSE;
		return;
	}
	
	/* Break out of this loop when we've got an event for the appl.,
	   or, if mayblock is false, when we would block (in XNextEvent) */
	for (;;) {
		if (_w_close_this != NULL) {
			/* WM_DELETE_WINDOW detected */
			ep->type = WE_CLOSE;
			ep->window = _w_close_this;
			_w_close_this = NULL;
			break;
		}
		if (_w_lostselection(ep))
			return;
		if (_w_keysym != 0) {
			if (_w_new_active != active) {
				if (active != NULL) {
					ep->type= WE_DEACTIVATE;
					ep->window= prev_active= active;
					active= NULL;
				}
				else {
					ep->type= WE_ACTIVATE;
					ep->window= active= _w_new_active;
				}
			}
			else {
				ep->type= WE_CHAR;
				ep->window= active;
				ep->u.character= _w_keysym;
				change_key_event(ep);
				_w_keysym= 0;
			}
			if (ep->type != WE_NULL)
				break;
		}
		if (_w_bs_changed || _w_moved) {
			ep->type= WE_NULL;
			switch (_w_bs.isub) {
			case MBAR:
				_whitmbar(&_w_bs, ep);
				break;
			case MWIN:
				_whitmwin(&_w_bs, ep); /* XXX test */
				break;
			case HBAR:
				_whithbar(&_w_bs, ep);
				break;
			case VBAR:
				_whitvbar(&_w_bs, ep);
				break;
			case WA:
				ep->type= _w_moved ?
						WE_MOUSE_MOVE :
						_w_bs.down ?
							WE_MOUSE_DOWN :
							WE_MOUSE_UP;
				ep->window= _w_bs.win;
				ep->u.where.h= _w_bs.x;
				ep->u.where.v= _w_bs.y;
				ep->u.where.button= _w_bs.button;
				ep->u.where.clicks= _w_bs.clicks;
				ep->u.where.mask= _w_bs.mask;
				break;
			}
			_w_bs_changed= _w_moved= FALSE;
			if (ep->type != WE_NULL)
				break;
		}
		if (_w_checktimer(ep, FALSE))
			break;
		if (XPending(_wd) == 0)
			XSync(_wd, FALSE);
		if (XPending(_wd) == 0) {
			if (_w_new_active != active) {
				/* Why is this code duplicated here? */
				if (active != NULL) {
					ep->type= WE_DEACTIVATE;
					ep->window= prev_active= active;
					active= NULL;
				}
				else {
					ep->type= WE_ACTIVATE;
					ep->window= active= _w_new_active;
				}
				break;
			}
			if (_w_resized && _w_doresizes(ep))
				break;
			if (_w_dirty && _w_doupdates(ep))
				break;
			if (XPending(_wd) == 0 && _w_checktimer(ep, mayblock)
					|| !mayblock)
				break;
		}
#ifdef PIPEHACK
		if (_w_evsave.type != NULL) {
			/* Lots of race conditions here! */
			char dummy[256];
			int n;
			_wdebug(1, "wgetevent: got evt from handler");
			*ep= _w_evsave;
			_w_evsave.type= WE_NULL;
			if ((n= read(_wpipe[0], dummy, sizeof dummy)) <= 0)
			  _wdebug(1, "wgetevent: read from pipe failed");
			else if (n != 1)
			  _wdebug(0, "wgetevent: got %d bytes from pipe!", n);
			break;
		}
#endif
		XNextEvent(_wd, &e);
		_w_ll_event(&e);
	}
	
	in_getevent= FALSE;
}

bool
wpollevent(ep)
	EVENT *ep;
{
	ep->type = WE_NULL;
	_wwaitevent(ep, FALSE);
	return ep->type != WE_NULL;
}

void
wgetevent(ep)
	EVENT *ep;
{
	_wwaitevent(ep, TRUE);
}

static void
change_key_event(ep)
	EVENT *ep;
{
	if (_w_state & Mod1Mask) {
		ep->type= WE_NULL;
		_w_menukey(ep->u.character, ep);
		return;
	}
	switch (ep->u.character) {
	case XK_Left:
		ep->u.command= WC_LEFT;
		break;
	case XK_Right:
		ep->u.command= WC_RIGHT;
		break;
	case XK_Up:
		ep->u.command= WC_UP;
		break;
	case XK_Down:
		ep->u.command= WC_DOWN;
		break;
	case '\n':
	case '\r':
		ep->u.command= WC_RETURN;
		break;
	case '\3':
		ep->u.command= WC_CANCEL;
		break;
	case '\b':
	case '\177': /* DEL */
		ep->u.command= WC_BACKSPACE;
		break;
	case '\t':
		ep->u.command= WC_TAB;
		break;
	default:
		if (ep->u.character < 0 || ep->u.character > 0177)
			ep->type= WE_NULL;
		return;
	}
	ep->type= WE_COMMAND;
}
