/* dpv -- ditroff previewer.  Main program. */

#include "dpv.h"

int	dbg;		/* Amount of debugging output wanted */
int	windowarn;	/* Set if errors must go to dialog box */

char   *progname= "dpv"; /* Program name if not taken from argv[0] */

/* Main program.  Scan arguments and call other routines. */

main(argc, argv)
	int argc;
	char **argv;
{
	int firstpage;
	
	winitargs(&argc, &argv);
	wsetdefscrollbars(1, 1);
	
	if (argc > 0 && *argv != NULL && **argv != EOS) {
		progname= rindex(argv[0], '/');
		if (progname != NULL)
			++progname;
		else
			progname= argv[0];
	}
	
	for (;;) {
		int c;
		
		c= getopt(argc, argv, "df:t:P:");
		if (c == EOF)
			break;
		switch (c) {
		
		case 'f':	/* Alternate funnytab file */
			funnyfile = optarg;
			break;
		
		case 't':	/* Alternative font translations file */
			readtrans(optarg);
			break;
		
		case 'd':	/* Ask for debugging output */
			++dbg; /* Use -dd to increment debug level */
			break;
		
		case 'P':	/* Add printer definition */
			addprinter(optarg);
			break;
		
		default:
			usage();
			/*NOTREACHED*/
		
		}
	}
	
	if (optind < argc && argv[optind][0] == '+') {
		if (argv[optind][1] == EOS ||
			argv[optind][1] == '$')
			firstpage= 32000;
		else
			firstpage= atoi(argv[optind] + 1);
		++optind;
	}
	else
		firstpage= 1;
	
	if (optind+1 != argc || strcmp(argv[optind], "-") == 0)
		usage();
	
	windowarn= 1; /* From now on, use dialog box for warnings */
	
	preview(argv[optind], firstpage);
	
	exit(0);
}

usage()
{
	error(ABORT,
"usage: %s [-d] [-f funnytab] [-t translationsfile] [-P printer] [+page] file",
	    progname);
}

/*VARARGS2*/
error(f, s, a1, a2, a3, a4, a5, a6, a7)
	int f;
	char *s;
{
	if (f != WARNING || !windowarn) { /* Use stderr */
		if (f != 0)
			wdone();
		fprintf(stderr, "%s: ", progname);
		fprintf(stderr, s, a1, a2, a3, a4, a5, a6, a7);
		fprintf(stderr, "\n");
		if (f != 0)
			exit(f);
	}
	else { /* use dialog box */
		char buf[256];
		sprintf(buf, s, a1, a2, a3, a4, a5, a6, a7);
		wmessage(buf);
	}
}
