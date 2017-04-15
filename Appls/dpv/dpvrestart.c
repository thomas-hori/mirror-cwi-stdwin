/* dpv -- ditroff previewer.  Functions to restart anywhere in the file. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"

typedef struct _restartinfo {
	long filepos;		/* File position for fseek */
	int pageno;		/* External page number */
	fontinfo fonts;		/* Mounted font table */
	int font, size;		/* Current font and size */
} restartinfo;

int		npages;		/* Number of pages so far */
bool		nomore;		/* Set when the last page has been seen */
restartinfo	*pagelist;	/* State needed to start each page */
int		ipage;		/* Internal page in file */
int		showpage;	/* Internal page displayed in window */
int		prevpage;	/* Last page visited (for '-' command) */
int		stoppage;	/* Internal page where to stop */
FILE		*ifile;		/* The input file */

		/* In order to avoid passing the file pointer around from
		   parse() via t_page() to nextpage(), the input file is
		   made available as a global variable. */

/* Initialize everything in the right order.  Tricky! */

initialize(filename, firstpage)
	char *filename;
	int firstpage;
{
	ifile= fopen(filename, "r");
	if (ifile == NULL)
		error(ABORT, "%s: cannot open", filename);
	setpageinfo(ftell(ifile), 0); /* Page 0 (== header) starts here */
	
	showpage= -1; /* Show no pages */
	stoppage= 1; /* Stop at beginning of page 1 */
	parse(ifile); /* Read the header */
	
	initoutput(filename); /* Create the window */
	
	showpage= 1;
	skiptopage(firstpage);
}

/* Close the file */

cleanup()
{
	fclose(ifile);
}

/* Skip n pages forward, default 1 */

forwpage(n)
	int n;
{
	if (n <= 0)
		n= 1;
	gotopage(showpage+n);
}

/* Skip n pages back, default 1 */

backpage(n)
	int n;
{
	if (n <= 0)
		n= 1;
	gotopage(showpage-n);
}

/* Go to internal page number n, and force a redraw */

gotopage(n)
	int n;
{
	int saveshowpage= showpage;
	skiptopage(n);
	if (showpage != saveshowpage)
		prevpage= saveshowpage;
	changeall();
}

/* Skip to internal page number n -- don't force a redraw */

static
skiptopage(n)
	int n;
{
	int orign= n;
	if (n <= 0)
		n= 1;
	if (n == showpage) {
		if (orign < n)
			wfleep();
		return;
	}
	if (n >= npages) {
		if (nomore) {
			n= npages-1;
			if (n == showpage) {
				wfleep();
				return;
			}
			showpage= n;
		}
		else {
			backtopage(npages-1);
			showpage= -1;
			stoppage= n;
			parse(ifile);
			showpage= npages-1;
		}
	}
	else
		showpage= n;
}

/* Draw procedure */

void
drawproc(drawwin, left, top, right, bottom)
	WINDOW *drawwin;
	int left, top, right, bottom;
{
	topdraw= top;
	botdraw= bottom;
	backtopage(showpage);
	stoppage= showpage+1;
	parse(ifile);
}

/* Record the current file position as the start of the page
   with (external) page number 'pageno'.
   Note that the 'p' command has already been consumed by the parser.
   Return < 0 if the parser can stop parsing. */

int
nextpage(pageno)
	int pageno;
{
	++ipage;
	setpageinfo(ftell(ifile), pageno);
	if (ipage >= stoppage)
		return -1; /* Stop parsing */
	else
		return 0;
}

/* Indicate that the end of the input has been reached.
   No more new pages will be accepted. */

lastpage()
{
	nomore= TRUE;
}

/* Store info about coming page.
   Called at start of file and after 'p' command processed */

static
setpageinfo(filepos, pageno)
	long filepos;
	int pageno;
{
	if (statep != state)
		error(FATAL, "setpageinfo: {} stack not empty");
	if (ipage < npages) {
		/* We've been here already.
		   Might as well check consistency. */
		/* HIRO */
	}
	else if (ipage > npages)
		error(ABORT, "setpageinfo: ipage>npages (can't happen)");
	else {
		restartinfo r;
		r.filepos= filepos;
		r.pageno= pageno;
		r.fonts= fonts;
		r.font= font;
		r.size= size;
		L_APPEND(npages, pagelist, restartinfo, r);
		if (pagelist == NULL)
			error(FATAL, "out of mem for pagelist");
	}
}

/* Position the input stream at the start of internal page i
   and restore the machine state to the state remembered for that page */

static
backtopage(i)
	int i;
{
	restartinfo *p;
	
	if (i < 0 || i >= npages)
		error(ABORT, "backtopage: called with wrong arg");
	p= &pagelist[i];
	if (fseek(ifile, p->filepos, 0) < 0)
		error(FATAL, "backtopage: can't fseek");
	ipage= i;
	fonts= p->fonts;
	font= p->font;
	size= p->size;
	hpos= 0;
	vpos= 0;
	statep= state;
	usefont();
}
