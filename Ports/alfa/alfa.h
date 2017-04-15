/* TERMCAP STDWIN -- INTERNAL HEADER FILE */

/* BEWARE: CONFUSED COORDINATE CONVENTIONS.
   The VTRM package, used here for terminal output,
   puts the y coordinate first.
   The stdwin package itself puts the x coordinate first
   (but instead of (x, y), it uses (h, v)).
   Also, when VTRM specifies a range of lines, the second number
   is the last line included.  In stdwin, the second number is
   the first position NOT included. */

#include "tools.h"
#include "stdwin.h"
#include "menu.h"
#include "vtrm.h"

struct _window {
	short tag;		/* Window tag, usable as document id */
	short open;		/* Set if this struct window is in use */
	char *title;		/* Title string */
	void (*drawproc)();	/* Draw procedure */
	short top;		/* Top line on screen */
	short bottom;		/* Bottom line on screen + 1 */
	int offset;		/* Diff. between doc. and screen line no's */
	int curh, curv;		/* Text cursor position (doc. coord.) */
	TEXTATTR attr;		/* Text attributes */
	struct menubar mbar;	/* Collection of local menus */
	long timer;		/* Absolute timer value (see timer.c) */
	short resized;		/* Nonzero when resize event pending */
};

/* Note on the meaning of the 'offset' field:
   to convert from screen coordinates to document coordinates: add offset;
   from document coordinates to screen coordinates: subtract offset. */

/* Data structures describing windows. */

#define MAXWINDOWS	20
#define MAXLINES	120

extern WINDOW winlist[MAXWINDOWS];
extern char uptodate[MAXLINES];
extern WINDOW *wasfront, *front, *syswin;
extern int lines, columns;
extern TEXTATTR wattr;

/* KEY MAPPING. */

/* The primary key map is a 256-entry array indexed by the first
   character received.  Secondary key maps are lists terminated with a
   type field containing SENTINEL.
   The maps use the same data structure so they can be processed
   by the same routine. */

struct keymap {
	unsigned char key;	/* Character for which this entry holds */
	unsigned char type;	/* Entry type */
	unsigned char id;	/* Id and item of menu shortcut */
	unsigned char item;	/* Also parameter for other types */
};

/* Entry types: */
#define ORDINARY	0	/* Report char as itself */
#define SECONDARY	1	/* Proceed to secondary keymap [id] */
#define SHORTCUT	2	/* Menu shortcut */
#define SENTINEL	127	/* End of secondary key map */

extern struct keymap _wprimap[256];
extern struct keymap **_wsecmap;

#define SECMAPSIZE 128

/* The system menu (menu id 0) has a few entries for window manipulation,
   followed by entries corresponding to WE_COMMAND subcodes.
   WC_CLOSE happens to be the first of those, and corresponds
   with CLOSE_WIN. */

/* Item numbers in system menu: */
#define PREV_WIN	0
#define NEXT_WIN	1
#define CLOSE_WIN	2

/* Offsets between WE_COMMAND subcodes and item numbers in system menu: */
#define FIRST_CMD	(CLOSE_WIN - WC_CLOSE)
#define LAST_CMD	99

/* There are also some codes that have a shortcut and a special interpretation
   but no entry in the system menu: */
#define SUSPEND_PROC	100
#define REDRAW_SCREEN	101
#define MOUSE_DOWN	102
#define MOUSE_UP	104
#define LITERAL_NEXT	105

#define MENU_CALL	127	/* Start interactive menu selection */

void wsyscommand();
bool wsysevent();
void wmenuselect();
void wdrawtitle();
void wnewshortcuts();
void wgoto();
void _wselnext();
void _wselprev();
void _wredraw();
void _wsuspend();
void getbindings();
void initsyswin();
void drawmenubar();
void drawlocalmenubar();
void _wreshuffle();
void _wnewtitle();
void wsysdraw();
void killmenubar();
void initmenubar();
void gettckeydefs();
void getttykeydefs();
void wsetmetakey();
void menubarchanged();
void menuselect();
void _wlitnext();
