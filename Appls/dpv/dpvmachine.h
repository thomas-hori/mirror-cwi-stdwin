/* dpv -- ditroff previewer.  Definitions for ditroff virtual machine. */

/* The ditroff machine has the following state information:

   Basic state:
   - hpos, vpos		print head position on current page
   - size		current point size
   - font		current font number (range 0..NFONTS-1)
   
   Environment:
   - a stack containing up to MAXSTATE copies of the basic state
   
   Mounted font table:
   - a mapping from font number to (ditroff) font names
     (i.e., which font is mounted on which logical position)
   
   Page info:
   - current page number
   
   Global info (not used much):
   - typesetter name
   - resolution assumed when preparing
   - page dimensions
   - etc.?
   
   To restart the machine at a given page, we need to save and restore
   the basic state, environment and mounted font table.
   It could be argued that at page boundaries the environment stack
   should be empty, and that the mounted font table should be initialized
   once and for ever.
   The latter isn't the case in practice; ".fp" requests can be and are
   used anywhere in ditroff source files.
   Because the environment pushes the (page-relative) position, it is
   unlikely that restoring an environment on a different page makes any
   sense, so we could assume this (and check that the input conforms).
*/

/* Array dimensions */

#define	NFONTS   65	/* total number of fonts usable */
#define	MAXSTATE 6	/* number of environments rememberable */

/* Basic state */

extern int	hpos;	/* horizontal position we are to be at next; left=0 */
extern int	vpos;	/* current vertical position (down positive) */
extern int	size;	/* current point size */
extern int	font;	/* current font */

#define  hmot(n)	hpos += n
#define  hgoto(n)	hpos = n
#define  vmot(n)	vgoto(vpos + (n))
extern vgoto();

/* Environment */

struct state {
	int	ssize;
	int	sfont;
	int	shpos;
	int	svpos;
};

extern struct	state	state[MAXSTATE];
extern struct	state	*statep;

extern t_push();
extern t_pop();

/* Mounted font table */

typedef struct _fontinfo {
	char *name[NFONTS];
} fontinfo;

extern fontinfo fonts;	/* current mounted font table */
extern int	nfonts;	/* number of used entries (0..nfonts-1) */

/* Page info */

extern int	ipage;	/* internal page number */

/* Global typesetter info */

extern int	res;	/* resolution for which input was prepared */
			/* default is old troff resolution */
