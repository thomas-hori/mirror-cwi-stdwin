/* X11 STDWIN -- error and debugging messages */

#include "x11.h"


/* Undocumented globals to reduce the output volume */

bool _wnoerrors;		/* Suppress errors and warnings if set */
bool _wnowarnings;		/* Suppress warnings if set */
int _wtracelevel;		/* Amount of trace output wanted */
int _wdebuglevel;		/* Amount of debug output wanted */


/* Fatal error message aborting the program (use for unrecoverable errors) */

/*VARARGS1*/
_wfatal(str, arg1, arg2, arg3, arg4)
	char *str;
{
	fprintf(stderr, "%s: fatal error: ", _wprogname);
	fprintf(stderr, str, arg1, arg2, arg3, arg4);
	fprintf(stderr, "\n");
	exit(1);
	/*NOTREACHED*/
}


/* Error message (use for recoverable serious errors) */

/*VARARGS1*/
_werror(str, arg1, arg2, arg3, arg4)
	char *str;
{
	if (!_wnoerrors || _wdebuglevel > 0 || _wtracelevel > 0) {
		fprintf(stderr, "%s: error: ", _wprogname);
		fprintf(stderr, str, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
}


/* Warning message (use for informative messages) */

/*VARARGS1*/
_wwarning(str, arg1, arg2, arg3, arg4)
	char *str;
{
	if (!_wnoerrors && !_wnowarnings || 
		_wdebuglevel > 0 || _wtracelevel > 0) {
		fprintf(stderr, "%s: warning: ", _wprogname);
		fprintf(stderr, str, arg1, arg2, arg3, arg4);
		fprintf(stderr, "\n");
	}
}


/* Trace message (use to trace user-level routine calls) */

/*VARARGS2*/
_wtrace(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	int level;
	char *str;
{
	if (_wtracelevel >= level) {
		fprintf(stderr, "%s: trace %d: ", _wprogname, level);
		fprintf(stderr, str,
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
		fprintf(stderr, "\n");
	}
}


/* Debug message (use for misc. debugging output) */

/*VARARGS2*/
_wdebug(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	int level;
	char *str;
{
	if (_wdebuglevel >= level) {
		fprintf(stderr, "%s: debug %d: ", _wprogname, level);
		fprintf(stderr, str,
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
		fprintf(stderr, "\n");
	}
}
