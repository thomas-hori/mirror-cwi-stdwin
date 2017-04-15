/* Selection Interface, a la ICCCM */


/*
	Features:
	
	- supports CLIPBOARD, PRIMARY and SECONDARY selections
	- only supports STRING as selection type
	- XXX no required funny targets TARGETS, MULTIPLE, TIMESTAMP
	- no side effect targets DELETE, INSERT_*
	- no INCR targets (XXX should accept these when receiving?)
	- XXX truncates large selections to 32 K
	- fixed timeout 1 second or 10 requests
	
	XXX To do:
	
	- delete selection data when we delete its window
	- report the selection's window in WE_LOST_SEL events
*/


#include "x11.h"
#include "llevent.h" /* For _w_lasttime; */

#ifdef _IBMR2
#include <sys/select.h>
#endif


#ifndef AMOEBA

/* Provide default definitions for FD_ZERO and FD_SET */

#ifndef FD_ZERO
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

#ifndef NFDBITS
#define NFDBITS		(sizeof(long)*8)	/* Assume 8 bits/byte */
#endif

#ifndef FD_SET
#define FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#endif

#endif /* AMOEBA */


/* Data structure describing what we know about each selection */

struct seldescr {
	Atom atom;
	char *data;
	int len;
	Time time;
	int lost;
};

static struct seldescr seldata[] = {
	{None,		NULL,	0,	CurrentTime,	0},	/* CLIPBOARD */
	{XA_PRIMARY,	NULL,	0,	CurrentTime,	0},
	{XA_SECONDARY,	NULL,	0,	CurrentTime,	0},
};

#define NSEL ( sizeof(seldata) / sizeof(seldata[0]) )

static void
initseldata()
{
	/* The CLIPBOARD atom is not predefined, so intern it here... */
	if (seldata[0].atom == None)
		seldata[0].atom = XInternAtom(_wd, "CLIPBOARD", False);
}


/* Attempt to acquire selection ownership.  Return nonzero if success. */

int
wsetselection(win, sel, data, len)
	WINDOW *win;
	int sel; /* WS_CLIPBOARD, WS_PRIMARY or WS_SECONDARY */
	char *data;
	int len;
{
	Window owner;
	initseldata();
	if (sel < 0 || sel >= NSEL) {
		_wwarning("wsetselection: invalid selection number %d", sel);
		return 0;
	}
	seldata[sel].lost = 0;
	XSetSelectionOwner(_wd, seldata[sel].atom, win->wo.wid, _w_lasttime);
	owner = XGetSelectionOwner(_wd, seldata[sel].atom);
	if (owner != win->wo.wid)
		return 0; /* Didn't get it */
	/* Squirrel away the data value for other clients */
	seldata[sel].len = 0;
	seldata[sel].time = CurrentTime;
	if (seldata[sel].data != NULL)
		free(seldata[sel].data);
	seldata[sel].data = malloc(len+1);
	if (seldata[sel].data == NULL) {
		_wwarning("wsetselection: no mem for data (%d bytes)", len);
		return 0;
	}
	memcpy(seldata[sel].data, data, len);
	seldata[sel].data[len] = '\0';
				/* Trailing NULL byte for wgetselection */
	seldata[sel].len = len;
	seldata[sel].time = _w_lasttime;
	return 1;
}


/* Give up selection ownership */

void
wresetselection(sel)
	int sel;
{
	if (sel < 0 || sel >= NSEL) {
		_wwarning("wresetselection: invalid selection number %d", sel);
		return;
	}
	seldata[sel].lost = 0;
	if (seldata[sel].data != NULL) {
		XSetSelectionOwner(_wd, seldata[sel].atom, None,
							seldata[sel].time);
		free(seldata[sel].data);
		seldata[sel].data = NULL;
		seldata[sel].len = 0;
		seldata[sel].len = CurrentTime;
	}
}


/* Attempt to get the value of a selection */

char *
wgetselection(sel, len_return)
	int sel;
	int *len_return;
{
	static char *prop;
	WINDOW *win;
	XEvent e;
	int i;
	
	initseldata();
	
	/* Free data retrieved last time */
	if (prop != NULL) {
		XFree(prop);
		prop = NULL;
	}
	
	/* Check selection code */
	if (sel < 0 || sel >= NSEL) {
		_wwarning("wgetselection: invalid selection number %d", sel);
		return NULL;
	}
	
	/* We may own this selection ourself... */
	if (seldata[sel].data != NULL) {
		_wdebug(2, "wgetselection: we have it");
		*len_return = seldata[sel].len;
		return seldata[sel].data;
	}
	
	/* Have to ask some other client (the selection owner) */
	win = _w_get_last_active();
	if (win == NULL) {
		_wwarning("wgetselection: no window");
		return NULL;
	}
	
	/* Tell the server to ask the selection owner */
	XConvertSelection(_wd,
		seldata[sel].atom,	/* selection */
		XA_STRING,		/* target */
		XA_STRING,		/* property (why not?) */
		win->wo.wid,		/* window */
		_w_lasttime);		/* time */
	
	/* Now wait for a SelectionNotify event -- 10 times */
	_wdebug(2, "waiting for SelectionNotify");
	for (i = 0; i < 10; i++) {
		e.xselection.property = None;
		if (XCheckTypedWindowEvent(_wd, win->wo.wid,
						SelectionNotify, &e)) {
			_wdebug(2, "got a SelectionNotify");
			break;
		}
		else {
#ifdef AMOEBA
			/* XXX For now, just sleep a bit -- there is a
			   routine to do this, though */
			millisleep(100);
#else /* !AMOEBA */
			/* Use select */
			/* SGI/SYSV interface */
			fd_set readfds;
			struct timeval timeout;
			int nfound;
			int cfd = ConnectionNumber(_wd);
			FD_ZERO(&readfds);
			FD_SET(cfd, &readfds);
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000;
			nfound = select(cfd+1, &readfds,
				(fd_set *)NULL, (fd_set *)NULL, &timeout);
			if (nfound < 0)
				_wwarning("select failed, errno %d", errno);
			else
				_wdebug(3, "select: nfound=%d\n", nfound);
#endif /* !AMOEBA */
		}
	}
	
	if (e.xselection.property != None) {
		/* Got a reply, now fetch the property */
		int status;
		Atom actual_type;
		int actual_format;
		unsigned long nitems;
		unsigned long bytes_after;
		status = XGetWindowProperty(_wd,
			win->wo.wid,
			e.xselection.property,
			0L,
			8192L, /* Times sizeof(long) is 32K ! */
			True,
			AnyPropertyType,
			&actual_type,
			&actual_format,
			&nitems,
			&bytes_after,
			(unsigned char **)&prop);
		if (status != Success) {
			_wdebug(1, "XGetWindowProperty failed (%d)", status);
			return NULL;
		}
		if (bytes_after != 0) {
			_wwarning("truncated %d property bytes", bytes_after);
			/* XXX should fetch the rest as well */
			XDeleteProperty(_wd, win->wo.wid,
						e.xselection.property);
		}
		if (actual_type != XA_STRING) {
			_wdebug(1, "bad property type: 0x%lx", actual_type);
			if (prop != NULL) {
				XFree(prop);
				prop = NULL;
			}
			return NULL;
		}
		*len_return = nitems * (actual_format/8);
		_wdebug(2, "got a selection, %d bytes", *len_return);
		return prop;
		/* Will be freed next time this function is called */
	}
	
	_wdebug(1, "SelectionNotify with property=None");
	return NULL;
}


/* Handle a SelectionClear event -- called from llevent.c */

void
_w_selectionclear(selection)
	Atom selection;
{
	int sel;
	
	_wdebug(2, "clearing a selection");
	initseldata();
	
	for (sel = 0; sel < NSEL; sel++) {
		if (seldata[sel].atom == selection) {
			wresetselection(sel);
			seldata[sel].lost = 1;
			break;
		}
	}
}


/* Generate a WE_LOST_SEL event if one is pending -- called from event.c */

int
_w_lostselection(ep)
	EVENT *ep;
{
	int sel;
	
	for (sel = 0; sel < NSEL; sel++) {
		if (seldata[sel].lost) {
			seldata[sel].lost = 0;
			ep->type = WE_LOST_SEL;
			/* XXX Should we report the window?
			   This is not easily available. */
			ep->window = NULL;
			ep->u.sel = sel;
			return 1;
		}
	}
	return 0;
}


/* Reply to a SelectionRequest event -- called from llevent.c */

void
_w_selectionreply(owner, requestor, selection, target, property, time)
	Window owner;
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;
	Time time;
{
	XSelectionEvent e;
	int sel;
	
	_wdebug(2, "replying to selection request");
	initseldata();
	
	/* Fill in the reply event */
	e.type = SelectionNotify;
	e.requestor = requestor;
	e.selection = selection;
	e.target = target;
	e.property = None; /* Replaced by property type if OK */
	e.time = time;
	
	if (property == None)
		property = target; /* Obsolete client */
	
	for (sel = 0; sel < NSEL; sel++) {
		if (seldata[sel].atom == selection) {
			/* It's this selection */
			if (seldata[sel].data != NULL &&
				(time == CurrentTime ||
				seldata[sel].time == CurrentTime ||
				((long)time - (long)seldata[sel].time) >= 0)) {
				
				/* And we have it.  Store it as a property
				   on the requestor window.  The requestor
				   will eventually delete it (says ICCCM). */
				
				/* XXX Always convert to STRING, OK? */
				XChangeProperty(_wd,
					requestor,
					property,
					XA_STRING,
					8,
					PropModeReplace,
					(unsigned char *)seldata[sel].data,
					seldata[sel].len);
				/* Tell requestor we stored the property */
				e.property = property;
			}
			else {
				/* But we don't have it or the time is wrong */
				_wdebug(1, "stale selection request");
			}
			break;
		}
	}
	
	if (!XSendEvent(_wd, requestor, False, 0L, (XEvent*)&e))
		_wdebug(1, "XSendEvent failed");
}
