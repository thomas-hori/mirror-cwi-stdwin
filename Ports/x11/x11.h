/* X11 STDWIN -- private definitions */

/* Includes */
#include "tools.h"
#include "stdwin.h"
#include "style.h"		/* For wattr */

/* Includes from X11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

/* Additional system includes */
#include <sys/time.h>

/* Figure out which X11 release we have */
#ifndef XlibSpecificationRelease
#define PRE_X11R5
#else
#if XlibSpecificationRelease < 5
#define PRE_X11R5
#endif
#endif

#define IBORDER		1	/* Border width of (most) internal windows */

/* Menu item */
struct item {
	char *text;
	char *sctext;
	short shortcut;
	tbool enabled;
	tbool checked;
};

/* Menu definition */
struct _menu {
	int id;
	char *title;
	int nitems;
	struct item *itemlist;
}; /* == MENU from <stdwin.h> */

/* Window or subwindow essential data */
struct windata {
	Window wid;		/* X Window id */
				/* The parent is implicit in the use */
	int x, y;		/* Origin w/ respect to parent */
	int width, height;	/* Width and height */
	short border;		/* Border width */
	tbool dirty;		/* Set if update pending */
				/* Some windows also have an update area */
};

#define NSUBS 7			/* Number of subwindows */

/* Window struct */
struct _window {
	short tag;		/* Must be first member and must be short! */
	void (*drawproc)();	/* Draw procedure */
	struct {
		int width, height;
	} doc;			/* Document dimension */
	int careth, caretv;	/* Caret position; (-1, -1) if no caret */
	bool caretshown;	/* Is the caret currently on display? */
	long timer;		/* Deciseconds till next alarm */
	TEXTATTR attr;		/* Text attributes */
	
	struct windata subw[NSUBS];	/* Subwindows */
	
	unsigned long fgo, bgo; /* pixel values for all but WA */
	unsigned long fga, bga;	/* pixel values for WA */
	
	GC gc;			/* Graphics Context for all but WA */
	GC gca;			/* Graphics Context for WA */
	
	Region inval;		/* Invalid area in WA window */

	int nmenus;		/* Number of menus */
	MENU **menulist;	/* List of attached menus (incl. global) */
	int curmenu;
	int curitem;
	
	tbool resized;		/* True if WE_SIZE event pending */

	/* Margins around inner window */
	int lmargin, tmargin, rmargin, bmargin;
}; /* == WINDOW from <stdwin.h> */

/* Shorthands for subwindow array access.
   Order is important! --for wopen() */
#define WO	0
#define MBAR	1
#define VBAR	2
#define HBAR	3
#define WI	4
#define WA	5
#define MWIN	6

#define wo	subw[WO]	/* Outer window (direct child of root)  */
#define mbar	subw[MBAR]	/* Menu bar (child of wo) */
#define vbar	subw[VBAR]	/* Scroll bars (children of wo) */
#define hbar	subw[HBAR]	/* " */
#define wi	subw[WI]	/* Inner window (child of wo) */
#define wa	subw[WA]	/* Application window (child of wi) */
#define mwin	subw[MWIN]	/* Currently visible menu */

/* Button state record */
struct button_state {
	bool down;		/* We think a button is down */
	int button;		/* Which button */
	int mask;		/* Buttons down (1|2|4) */
	int clicks;		/* Click status */
	long time;		/* Time stamp, milliseconds */
	WINDOW *win;		/* The WINDOW where it applies */
	Window w;		/* SubWindow id */
	int isub;		/* Subwindow index */
	int x, y;		/* Current position */
	int xdown, ydown;	/* Position of initial press */
};

/* Private globals */
extern Display *_wd;		/* general.c */
extern Screen *_ws;		/* general.c */
extern XFontStruct *_wf;	/* general.c */
extern XFontStruct *_wmf;	/* general.c */
extern int _wpipe[2];		/* general.c */
extern char *_wprogname;	/* general.c */
extern int _wdebuglevel;	/* errors.c */
extern int _wtracelevel;	/* errors.c */
extern COLOR _w_fgcolor;	/* draw.c */
extern COLOR _w_bgcolor;	/* draw.c */

/* Interned atoms */
extern Atom _wm_protocols;
extern Atom _wm_delete_window;
extern Atom _wm_take_focus;

/* Externals used to communicate hints to wopen() */
extern char *_whostname;
extern char *_wm_command;
extern int _wm_command_len;

/* Functions not returning int */
WINDOW *_wsetup();		/* window.c */
WINDOW *_whichwin();		/* window.c */
WINDOW *_wnexttimer();		/* window.c */
GC _wgcreate();			/* window.c */
unsigned long _wgetpixel();	/* window.c */
unsigned long _w_fetchpixel();	/* window.c */
WINDOW *_w_get_last_active();	/* event.c */
WINDOW *_w_somewin();		/* window.c */
Pixmap _w_gray();		/* window.c */
Pixmap _w_raster();		/* window.c */
char *_wgetdefault();		/* general.c */
void _w_setgrayborder();	/* window.c */
void _wsetmasks();		/* window.c */
void _w_initcolors();		/* draw.c */

/* Function _wcreate has an extra parameter now */
#define _wcreate(wp, parent, cursor, map, fg, bg) \
	_wcreate1(wp, parent, cursor, map, fg, bg, 0)
