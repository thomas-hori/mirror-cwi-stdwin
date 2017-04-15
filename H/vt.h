/* Interface for VT (Virtual Terminal windows) package */

/* WARNING: the cursor coordinate system is (row, col) here,
   like in the ANSI escape sequences.
   However, the origin is 0, and ranges are given in C style:
   first, last+1.
   ANSI escapes (which have origin 1) are translated as soon
   as they are parsed. */

#define VTNARGS 10		/* Max # args in ansi escape sequence */

/* Structure describing a VT window */

typedef struct _vt {
	WINDOW *win;		/* Window used for display */
	char **data;		/* Character data array [row][col] */
	unsigned char **flags;	/* Corresponding flags per character */
	short *llen;		/* Line length array */
	short rows, cols;	/* Size of data matrix */
	short cwidth, cheight;	/* Character cell size */
	short cur_row, cur_col;	/* Cursor position */
	short save_row, save_col;	/* Saved cursor position */
	short scr_top, scr_bot;	/* Scrolling region */
	short topterm;		/* Top line of emulated terminal */
	short toscroll;		/* Delayed screen scrolling */
	unsigned char gflags;	/* Global flags for inserted characters */

	/* XXX The following Booleans are stored in characters.
	   This is probably the most efficient way to store them.
	   Since we normally have only one VT window per application,
	   space considerations are not important and we could even use
	   ints if they were faster. */

	char insert;		/* Insert mode */
	char keypadmode;	/* Send ESC O A for arrows, not ESC [ A */
	char nlcr;		/* \n implies \r */
	char lazy;		/* Delay output operations */
	char mitmouse;		/* Send ESC [ M <button;col;row> on clicks */
	char visualbell;	/* Blink instead of beep (for ctrl-G) */
	char flagschanged;	/* Set when certain state flags changed */
	char drawing;		/* Used by VT{BEGIN,END}DRAWING() macros */

	/* State for the ANSI escape sequence interpreter */

	char modarg;		/* \0: ansi vt100; '?': DEC private mode etc */
	char *(*action)();	/* Function to call for next input */
	short *nextarg;		/* Points to current arg */
	short args[VTNARGS];	/* Argument list */

	/* State for the selected region */

	short sel_row1, sel_col1; /* Start of selected region, <0 if nothing */
	short sel_row2, sel_col2; /* End of selected region */

	/* XXX There ought to be an array of tab stops somewhere... */
} VT;

/* Flags in gflags and flags array.
   These correspond to the ANSI codes used in ESC [ ... m.
   Not all are implemented! */

#define VT_BOLD		(1 << 1)
#define VT_DIM		(1 << 2)
#define VT_UNDERLINE	(1 << 4)
#define VT_BLINK	(1 << 5)
#define VT_INVERSE	(1 << 7)
#define VT_SELECTED	(1 << 3)	/* <- This one does NOT correspond */

/* Access macros */

#define vtcheight(vt)			((vt)->cheight)
#define vtcwidth(vt)			((vt)->cwidth)
#define vtwindow(vt)			((vt)->win)

#define vtsetnlcr(vt, new_nlcr)		((vt)->nlcr = (new_nlcr))
#define vtsetlazy(vt, new_lazy)		((vt)->lazy= (new_lazy))
#define vtsetflags(vt, new_flags)	((vt)->gflags= (new_flags))
#define vtsetinsert(vt, new_insert)	((vt)->insert= (new_insert))

/* Basic functions (vt.c) */

VT *vtopen _ARGS((char *title, int rows, int cols, int save));
void vtclose _ARGS((VT *vt));
void vtputstring _ARGS((VT *vt, char *text, int len));
void vtsetcursor _ARGS((VT *vt, int row, int col));
void vtreset _ARGS((VT *vt));
VT *vtfind _ARGS((WINDOW *win));

/* Optional functions (vtansi.c, vtputs.c, vtresize.c) */

void vtansiputs _ARGS((VT *vt, char *text, int len));
void vtputs _ARGS((VT *vt, char *text));
/*bool*/int vtresize _ARGS((VT *vt, int rows, int cols, int save));
/*bool*/int vtautosize _ARGS((VT *vt));

/* Functions used by the ANSI interface (vtfunc.c) */

void vtresetattr _ARGS((VT *vt));
void vtsetattr _ARGS((VT *vt, int bit));
void vtsavecursor _ARGS((VT *vt));
void vtrestorecursor _ARGS((VT *vt));
void vtarrow _ARGS((VT *vt, int code, int repeat));
void vteolclear _ARGS((VT *vt, int row, int col));
void vteosclear _ARGS((VT *vt, int row, int col));
void vtlinefeed _ARGS((VT *vt, int repeat));
void vtrevlinefeed _ARGS((VT *vt, int repeat));
void vtinslines _ARGS((VT *vt, int n));
void vtdellines _ARGS((VT *vt, int n));
void vtscrollup _ARGS((VT *vt, int r1, int r2, int n));
void vtscrolldown _ARGS((VT *vt, int r1, int r2, int n));
void vtinschars _ARGS((VT *vt, int n));
void vtdelchars _ARGS((VT *vt, int n));
void vtsetscroll _ARGS((VT *vt, int top, int bot));
void vtsendid _ARGS((VT *vt));
void vtsendpos _ARGS((VT *vt));

/* Selection interface (vtselect.c) */

/* XXX This will change */
int  vtselect _ARGS((VT *vt, EVENT *ep));
int  vtextendselection _ARGS((VT *vt, EVENT *ep));

/* Macros to avoid too many wbegindrawing calls */

#define VTBEGINDRAWING(vt) \
	if (!(vt)->drawing) { wbegindrawing((vt)->win); (vt)->drawing= 1; }
#define VTENDDRAWING(vt) \
	if ((vt)->drawing) { wenddrawing((vt)->win); (vt)->drawing = 0; }

/* Note -- the main application *MUST* call VTENDDRAWING(vt) before
   it calls wgetevent(). */

#ifndef NDEBUG

/* Panic function.  The caller may provide one.  The library has a default. */

void vtpanic _ARGS((char *));

#endif /* NDEBUG */
