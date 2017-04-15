/* Assorted things used by the VT implementation */

/* Include files */

#include "tools.h"
#include "stdwin.h"
#include "vt.h"

/* Names for control characters */

#define ESC	'\033'
#define CR	'\r'
#define LF	'\012'	/* Can't use \n or EOL because of MPW on Macintosh */
#define BS	'\b'
#define BEL	'\007'
#define TAB	'\t'
#define FF	'\014'

/* Macro to check for non-control character */

#define PRINTABLE(c)	isprint(c)

/* Debugging facilities */

/* If you want static functions visible for adb, pass '-DSTATIC=static'
   to the cc compiler. */
#ifndef STATIC
#define STATIC static
#endif

/* Macro D() to turn off code in NDEBUG mode or if debugging is off */
#ifdef NDEBUG
#define D(stmt) 0
#else
/* XXX Dependency on 'debugging' declared in calling application */
extern int debugging;
#define D(stmt) if (!debugging) { } else stmt
#endif

/* In Amoeba, MON_EVENT() can be used to keep cheap performance statistics */
#ifndef MON_EVENT
#define MON_EVENT(msg) do { } while (0) /* Funny syntax to work inside 'if' */
#endif

/* Functions used internally */
/* XXX Ought to have names beginning with _vt */

void vtcirculate _ARGS((VT *vt, int r1, int r2, int n));
void vtchange _ARGS((VT *vt, int r1, int c1, int r2, int c2));
void vtshow _ARGS((VT *vt, int r1, int c1, int r2, int c2));
void vtscroll _ARGS((VT *vt, int r1, int c1, int r2, int c2, int dr, int dc));
void vtsync _ARGS((VT *vt));
