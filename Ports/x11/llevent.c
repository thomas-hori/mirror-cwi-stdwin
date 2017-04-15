/* X11 STDWIN -- Low-level event handling */

#include "x11.h"
#include "llevent.h"
#include <X11/keysym.h>

/* Forward */
static void activate();
static void deactivate();
static void ll_key();
static void ll_button();
static void ll_expose();
static void ll_configure();

/* Double-click detection parameters */

#define DCLICKTIME	500
#define DCLICKDIST	5

/* Events are handled in two phases.
   There is low-level event handling which can be called at any time
   and will only set various flags (global and in windows) indicating
   changed circumstances.  This kind of event handling goes on even
   while we are locked in a dialog box.
   There is high-level event handling which passes the results of the
   low-level handling to the application in request to a call to
   'wgetevent'.
   This file contains the low-level event handling. */


/* Low-level event handler.
   Data items set for use by higher-level event handler include:
   
   - the 'new active window' pointer
   - the 'dirty' flags for all windows and subwindows
   - the 'size changed' flags for all top-level windows
   
   These can be accumulated; the net effect of all such events is
   correctly remembered.  The following data items are also set but need
   to be checked after each call, since they are not queued:
   
   - the mouse button state
   - the 'last-key-pressed' variable
   
   Mouse moves may be ignored, however.
   The state change flags must be be reset by the higher-level event
   handler after the changes have been noted. */

WINDOW *_w_new_active;		/* New active window */
struct button_state _w_bs;	/* Mouse button state */
KeySym _w_keysym;		/* Keysym of last non-modifier key pressed */
int _w_state;			/* Modifiers in effect at key press */

bool _w_moved;			/* Set if button moved */
bool _w_bs_changed;		/* Set if button up/down state changed */
bool _w_dirty;			/* Set if any window needs a redraw */
bool _w_resized;		/* Set if any window resized */
bool _w_focused;		/* Set if between FocusIn and FocusOut */
Time _w_lasttime = CurrentTime;	/* Last timestamp received in an event */
WINDOW *_w_close_this;		/* Window to close (WM_DELETE_WINDOW) */

_w_ll_event(e)
	XEvent *e;
{
	WINDOW *win;
	
	_wdebug(6, "_w_ll_event: event type %ld", e->type);
	
	win = _whichwin(e->xkey.window);
	if (win == NULL) {
		_wdebug(6, "_w_ll_event: event not for any window -- ignore");
		return;
	}
	
	switch (e->type) {
	
	/* Keyboard */
	
	case KeyPress:
		_w_lasttime = e->xkey.time;
		activate(win);
		ll_key(&e->xkey, win);
		break;
	
	case MappingNotify:
		XRefreshKeyboardMapping(&e->xmapping);
		break;
	
	/* Buttons */
	
	case MotionNotify:
#ifndef DONT_COMPRESS_MOTION /* XXX should be a resource option instead */
		/* Compress motion events */
		while (XPending(_wd) > 0) {
			XEvent e2;
			XPeekEvent(_wd, &e2);
			if (e2.type == MotionNotify &&
				e2.xmotion.window == e->xmotion.window &&
				e2.xmotion.subwindow == e->xmotion.subwindow) {
				/* replace current event with next one */
				XNextEvent(_wd, e);
			}
			else
				break;
		}
#endif
		/* Fall through */
	case ButtonPress:
	case ButtonRelease:
		_w_lasttime = e->xbutton.time;
		ll_button(&e->xbutton, win);
		break;
	
	/* Exposures */
	
	case GraphicsExpose:
		_wdebug(3, "GraphicsExpose event:");
	case Expose:
		ll_expose(&e->xexpose, win);
		break;
	
	/* Structure changes */
	
	case ConfigureNotify:
		ll_configure(&e->xconfigure, win);
		break;
	
	/* Input focus changes */
	
	case FocusIn:
		_w_focused= TRUE;
		activate(win);
		break;
	
	case FocusOut:
		_w_focused= FALSE;
		deactivate(win);
		break;
	
	case EnterNotify:
		_w_lasttime = e->xcrossing.time;
		if (!_w_focused && e->xcrossing.focus)
			activate(win);
		break;
	
	case LeaveNotify:
		_w_lasttime = e->xcrossing.time;
		if (!_w_focused && e->xcrossing.detail != NotifyInferior &&
				e->xcrossing.focus)
			deactivate(win);
		break;
	
	/* ICCCM */
	
	case ClientMessage:
		_wdebug(2,
			"ClientMessage, type=%ld, send_event=%ld, format=%ld",
			e->xclient.message_type,
			e->xclient.send_event,
			e->xclient.format);
		if (e->xclient.message_type == _wm_protocols) {
			_wdebug(2, "It's a WM_PROTOCOLS ClientMessage (%ld)",
				e->xclient.data.l[0]);
			_w_lasttime = e->xclient.data.l[1];
		}
		else {
			_wdebug(1, "Unexpected ClientMessage ignored (%ld)",
				e->xclient.message_type);
			break;
		}
		if (e->xclient.data.l[0] == _wm_delete_window) {
			_wdebug(1, "WM_DELETE_WINDOW");
			_w_close_this = win;
		}
		else {
			_wdebug(1,
				"Unexpected WM_PROTOCOLS ClientMessage (%ld)",
				e->xclient.data.l[0]);
		}
		break;
	
	case SelectionRequest:
		_w_selectionreply(
			e->xselectionrequest.owner,
			e->xselectionrequest.requestor,
			e->xselectionrequest.selection,
			e->xselectionrequest.target,
			e->xselectionrequest.property,
			e->xselectionrequest.time);
		break;
	
	case SelectionClear:
		_w_selectionclear(e->xselectionclear.selection);
		break;
	
	case MapNotify:
		/* Ignore this */
		break;
	
	default:
		_wdebug(1, "unexpected event type %d", e->type);
	
	}
}

static void
activate(win)
	WINDOW *win;
{
	if (_w_new_active != win) {
		if (_w_new_active != NULL)
			deactivate(_w_new_active);
		_w_new_active= win;
		if (win != NULL)
			XSetWindowBorder(_wd, win->wo.wid, win->fgo);
	}
}

static void
deactivate(win)
	WINDOW *win;
{
	if (win == _w_new_active) {
		_w_new_active= NULL;
		if (win != NULL)
			_w_setgrayborder(win);
	}
}

static void
ll_key(e, win)
	XKeyEvent *e;
	WINDOW *win;
{
	char sbuf[1];
	
	_wdebug(6, "keycode %d, state 0x%x", e->keycode, e->state);
	if (XLookupString(e, sbuf, 1, &_w_keysym, (XComposeStatus*)NULL) > 0)
		_w_keysym= sbuf[0];
	else if (IsModifierKey(_w_keysym))
		_w_keysym= 0;
	_w_state= e->state;
}

/* Update button status, given a Button or Motion event.
   THIS ASSUMES BUTTON AND MOTION EVENTS HAVE THE SAME LAY-OUT! */

static void
ll_button(e, win)
	XButtonEvent *e;
	WINDOW *win;
{
	Window w= e->window;
	
	/* This code is ugly.  I know. */
	
	if (!_w_bs.down) { /* New sequence */
		if (e->type != ButtonPress) {
			/* Button moved/release event while we've never
			   received a corresponding press event.
			   I suspect this is a server bug;
			   normally I never reach this code, but sometimes
			   it happens continually.  Why?... */
			_wdebug(5,
				"ll_button: spurious button move/release %d",
				e->type);
			return;
		}
		else {
			bool isdclick= FALSE;
			_w_bs.down= TRUE;
			_w_bs_changed= TRUE;
			if (_w_bs.button == e->button &&
					_w_bs.w == e->window) {
				/* Multiple-click detection; must be done
				   before setting new values */
				if (e->time <= _w_bs.time + DCLICKTIME) {
					int dx= e->x - _w_bs.x;
					int dy= e->y - _w_bs.y;
					isdclick= (dx*dx + dy*dy <=
						DCLICKDIST*DCLICKDIST);
				}
			}
			if (!isdclick)
				_w_bs.clicks= 0;
			++_w_bs.clicks;
			_w_bs.mask= e->state;
			_w_bs.button= e->button;
			_w_bs.time= e->time;
			_w_bs.win= win;
			_w_bs.w= w;
			for (_w_bs.isub= NSUBS; --_w_bs.isub >= 0; ) {
				if (w == win->subw[_w_bs.isub].wid)
					break;
			}
			if (_w_bs.isub < 0) {
				_wdebug(0, "ll_button: can't find subwin");
				_w_bs.isub= 0;
				return;
			}
			_w_bs.x= _w_bs.xdown= e->x;
			_w_bs.y= _w_bs.ydown= e->y;
		}
	}
	else { /* Continue existing sequence */
		if (win == _w_bs.win && w != _w_bs.w) {
			/* Maybe changed between mbar and mwin? */
			if (_w_bs.isub == MBAR && w == win->mwin.wid) {
				/* Change it -- because of XGrabPointer */
				_wdebug(3, "MBAR grabbed button");
				_w_bs.isub = MWIN;
				_w_bs.w = w;
			}
		}
		if (win != _w_bs.win || w != _w_bs.w) {
			/* XXX This still happens -- why? */
			_wdebug(0, "ll_button: inconsistent _w_bs");
			_w_bs.down= FALSE;
			_w_bs.mask= 0;
			_w_bs_changed= TRUE;
		}
		else {
			_w_bs.mask= e->state;
			_w_bs.x= e->x;
			_w_bs.y= e->y;
			_w_bs.time= e->time;
			if (e->type == ButtonRelease &&
				e->button == _w_bs.button) {
				_w_bs.down= FALSE;
				_w_bs_changed= TRUE;
			}
			else
				_w_moved= TRUE;
		}
	}
	_wdebug(5, "ll_button: xy=(%d, %d), down=%d, clicks=%d",
		_w_bs.x, _w_bs.y, _w_bs.down, _w_bs.clicks);
}

static void
ll_expose(e, win)
	XExposeEvent *e;
	WINDOW *win;
{
	Window w= e->window;
	
	_wdebug(3, "ll_expose called, count=%d, x=%d,y=%d,w=%d,h=%d",
		e->count, e->x, e->y, e->width, e->height);
	if (w == win->wa.wid)
		wchange(win, e->x, e->y, e->x + e->width, e->y + e->height);
	else if (w == win->mbar.wid)
		win->mbar.dirty= TRUE;
	else if (w == win->hbar.wid)
		win->hbar.dirty= TRUE;
	else if (w == win->vbar.wid)
		win->vbar.dirty= TRUE;
	else {
		_wdebug(3, "ll_expose: uninteresting event");
		return;
	}
	if (e->count == 0)
		_w_dirty= TRUE;
}

static void
ll_configure(e, win)
	XConfigureEvent *e;
	WINDOW *win;
{
	if (e->window != win->wo.wid) {
		_wdebug(0, "ll_configure: not for wo.wid");
		return;
	}
	win->wo.x= e->x + e->border_width;
	win->wo.y= e->y + e->border_width;
	win->wo.border= e->border_width;
	if (win->wo.width != e->width || win->wo.height != e->height) {
		/* Size changed */
		win->wo.width= e->width;
		win->wo.height= e->height;
		_wmovesubwins(win);
		win->resized= _w_resized= TRUE;
	}
}
