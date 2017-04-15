/* Text Edit, definitions */

/* Include header files */
#include "stdwin.h"	/* Window interface */
#include "tools.h"	/* Lots of useful goodies */

#ifndef LSC
#define NDEBUG		/* Turn off all debugging code */
#endif

#define RESERVE 256	/* Increment for gap growth */
#define STARTINCR 20	/* Increment for start array growth; must be >= 1 */

/* Typedefs (for documentation only) */
typedef int focpos;	/* Logical offset (not affected by gap) */
typedef int bufpos;	/* Buffer offset (taking gap into account) */
typedef int lineno;	/* Index into start array */
typedef int coord;	/* Used to declare pairs of coordinates */
typedef coord hcoord;	/* Hor. coordinate */
typedef coord vcoord;	/* Ver. coordinate */

struct _textedit {
	/* Drawing environment */
	WINDOW *win;
	coord left, top, right, bottom;		/* Text area */
	hcoord width;	/* == right-left */
	TEXTATTR attr;	/* Text attributes */
	vcoord vspace;	/* Vertical spacing (line height) */
	hcoord tabsize;	/* Spacing of horizontal tabs */
	
	/* Text and focus representation */
	char *buf;	/* Text buffer */
	bufpos buflen;	/* Size of buffer */
	bufpos gap;	/* Start of gap */
	int gaplen;	/* Gap length */
	focpos foc;	/* Text selection focus start */
	int foclen;	/* Focus length */
	bufpos *start;	/* Array of screen line starts */
	lineno nlines;	/* Number of lines */
	lineno nstart;	/* Number of elements of start (must be > nlines) */
	hcoord aim;	/* Where vertical arrows should (try to) go */
	focpos anchor;	/* Anchor position of focus drag */
	focpos anchor2;	/* Other end of anchor */
	tbool focprev;	/* Set if foc between lines belongs to prev line */
	tbool hilite;	/* Set if focus area shown inverted */
	tbool mdown;	/* Set if mouse down */
	tbool dclick;	/* Set if mouse down in double click */
	tbool drawing;	/* FALSE if no window operations */
	tbool active;	/* FALSE to inhibit highlighting and caret */
	
	/* To optimize single char inserts */
	tbool opt_valid;	/* Set if following data is valid */
	tbool opt_in_first_word;/* Focus is in first word of line */
	lineno opt_i;		/* Line where focus is */
	coord opt_h, opt_v;	/* Caret position in window */
	hcoord opt_avail;	/* White pixels at end of line */
	hcoord opt_end;		/* End of line or next tab stop */

	/* View restriction */
	coord vleft, vtop, vright, vbottom;	/* View area */
	tbool viewing;				/* TRUE to enable view */

	/* NB: aim, opt_h, opt_v are in window coordinates,
	       i.e., tp->left or tp->top has already been added */
};

/* Constants */
#define UNDEF		(-1)	/* Undefined value, e.g., for aim */

/* NB: All macros below use a variable 'tp' pointing to the textedit struct */

/* Shorthands */
#define zfocend		(tp->foc+tp->foclen)
#define zgapend		(tp->gap+tp->gaplen)

/* Transformations between focpos and bufpos values */
#define zaddgap(f)	((f) < tp->gap ? (f) : (f)+tp->gaplen)
#define zsubgap(pos)	((pos) <= tp->gap ? (pos) : \
			 (pos) <= zgapend ? tp->gap : (pos)-tp->gaplen)

/* ++ and -- operators for bufpos variables */
#define zincr(p)	(++*(p) == tp->gap ? (*(p) += tp->gaplen) : *(p))
#define zdecr(p)	(*(p) == zgapend ? (*(p) = tp->gap - 1) : --*(p))

/* +1 and -1 operators for same */
#define znext(p)	((p) == tp->gap-1 ? zgapend : (p)+1)
#define zprev(p)	((p) == zgapend ? (tp->gap-1) : (p)-1)

/* Access characters at/before positions */
#define zcharat(p)	(tp->buf[p])
#define zcharbefore(p)	(tp->buf[zprev(p)])

/* Tab stop calculation */
#define znexttab(w) ((((w)+tp->tabsize) / tp->tabsize) * tp->tabsize)

/* Functions that don't return int */
TEXTEDIT *tesetup();
char *zmalloc();
char *zrealloc();

/* Debugging help */

#ifndef NDEBUG

#ifndef __LINE__
#define __LINE__ 0
#endif

/* General assertion (NB: type command-period to dprintf to halt) */
#define zassert(n) ((n) || dprintf("line %d: zassert(n) failed", __LINE__))

/* Check the validity of a buffer offset */
#define zcheckpos(p) \
	((p)>=0 && (p)<=tp->buflen && ((p)<tp->gap || (p)>=zgapend) || \
		dprintf("line %d: zcheckpos(p=%d) buf[%d] gap=%d+%d", \
			__LINE__, p, tp->buflen, tp->gap, tp->gaplen))

/* Sanity checking routine for entire state */
#define zcheck() techeck(tp, __LINE__)

#else /* NDEBUG */

#define zassert(n)	/*empty*/
#define zcheckpos(pos)	/*empty*/
#define zcheck()	/*empty*/

#endif /* NDEBUG */
