/* dpv -- ditroff previewer.  Ditroff virtual machine. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"

/* Basic state */

int	size;		/* current point size */
int	font;		/* current font */
int	hpos;		/* horizontal position we are to be at next; left=0 */
int	vpos;		/* current vertical position (down positive) */

/* Environment */


struct	state	state[MAXSTATE];
struct	state	*statep = state;

/* Mounted font table */

fontinfo fonts;
int	nfonts = 0;	/* start with no fonts mounted */

/* Page info */

int	ipage;		/* internal page number */

/* Global info */

int	res = 432;	/* resolution for which input was prepared */
			/* default is old troff resolution */

/* Load font info for font s on position n; optional file name s1 */

loadfont(n, s, s1)
	int n;
	char *s, *s1;
{
	if (n < 0 || n > NFONTS)
		error(FATAL, "can't load font %d", n);
	fonts.name[n]= strdup(s);
}

/* Initialize device */

t_init()
{
	/* Start somewhere */
	hpos = vpos = 0;
	size= 10;
	font= 1;
	usefont();
}

/* Begin a new block */

t_push()
{
	if (statep >= state+MAXSTATE)
		error(FATAL, "{ nested too deep");
	statep->ssize = size;
	statep->sfont = font;
	statep->shpos = hpos;
	statep->svpos = vpos;
	++statep;
}

/* Pop to previous state */

t_pop()
{
	if (--statep < state)
		error(FATAL, "extra }");
	size = statep->ssize;
	font = statep->sfont;
	hpos = statep->shpos;
	vpos = statep->svpos;
	usefont();
}

/* Called at the start of a new page.
   Returns < 0 if it is time to stop. */

int
t_page(n)
	int n;
{
	if (nextpage(n) < 0)
		return -1;
	vpos = hpos = 0;
	usefont();
	return 0;
}

/* Do whatever for the end of a line */

t_newline()
{
	hpos = 0;	/* because we're now back at the left margin */
}

/* Convert integer to internal size number */

t_size(n)
	int n;
{
	return n;
}

/* Set character height to n */

/*ARGSUSED*/
t_charht(n)
	int n;
{
	static bool warned;
	if (!warned) {
		error(WARNING, "Setting character height not implemented");
		warned= TRUE;
	}
}

/* Set slant to n */

/*ARGSUSED*/
t_slant(n)
	int n;
{
	static bool warned;
	if (!warned) {
		error(WARNING, "Setting slant not implemented");
		warned= TRUE;
	}
}

/* Convert string to font number */

t_font(s)
	char *s;
{
	return atoi(s);
}

/* Print string s as text */

t_text(s)
	char *s;
{
	int c;
	char str[100];
	
	/* XXX This function doesn't work any more since lastw is not set */

	while (c = *s++) {
		if (c == '\\') {
			switch (c = *s++) {
			case '\\':
			case 'e':
				put1('\\');
				break;
			case '(':
				str[0] = *s++;
				str[1] = *s++;
				str[2] = EOS;
				put1s(str);
				break;
			}
		} else {
			put1(c);
		}
		/*hmot(lastw);*/
	}
}

/* Reset; argument is 'p' for pause, 's' for start */

t_reset(c)
	int c;
{
	if (c == 's')
		lastpage();
}

/* Absolute vertical motion to position n */

vgoto(n)
	int n;
{
	vpos = n;
	recheck();
}

/* Set point size to n */

setsize(n)
int n;
{
	size = n;
	usefont();
}

/* Set font to n */

setfont(n)
int n;
{
	font = n;
	usefont();
}
