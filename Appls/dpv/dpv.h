/* dpv -- ditroff previewer.  Global definitions */

#include "tools.h"
#include <stdwin.h>

#define DEBUGABLE		/* defined if can set dbg flag (-d) */

#define ABORT	2		/* exit code for aborts */
#define	FATAL	1		/* type of error */
#define WARNING 0		/* non-fatal (also !FATAL) */

extern int	dbg;		/* amount of debugging output wanted */

extern char	*progname;	/* program name, for error messages */
extern char	*funnyfile;	/* alternate funnytab filename */
