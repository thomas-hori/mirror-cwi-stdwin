/* MAC STDWIN -- GET ARGC/ARGV. */

/* Copy the arguments passed from the finder into argc/argv.
   No Mac C compiler's runtime system does this, making argc/argv
   pretty useless.  By using wargs(&argc, &argv) you get the arguments
   that you expect.  When called to print, a "-p" flag is passed
   before the first argument.

   This is for System 6 or earlier.  For System 7, don't use this.
   Use argcargv_ae.c instead.  */

#include "macwin.h"

/* GetAppParms and friends are obsolete... */
#define OBSOLETE
#include <SegLoad.h>

void
wargs(pargc, pargv)
	int *pargc;
	char ***pargv;
{
	L_DECLARE(argc, argv, char *);
	Str255 apname;
	char buf[256];
	short aprefnum;
	Handle apparam;
	short message;
	short count;
	short i;
	
	GetAppParms(apname, &aprefnum, &apparam);
#ifndef CLEVERGLUE
	PtoCstr(apname);
#endif
	L_APPEND(argc, argv, char *, strdup((char *)apname));
	
	CountAppFiles(&message, &count);
	if (message == appPrint) { /* Must have braces around L_*! */
		L_APPEND(argc, argv, char *, "-p");
	}
	
	for (i = 1; i <= count; ++i) {
		AppFile thefile;
		GetAppFiles(i, &thefile);
		fullpath(buf, thefile.vRefNum, PtoCstr(thefile.fName));
		L_APPEND(argc, argv, char *, strdup(buf));
	}
	
	L_APPEND(argc, argv, char *, NULL);
	*pargc = argc - 1;
	*pargv = argv;
}
