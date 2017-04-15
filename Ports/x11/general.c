/* X11 STDWIN -- General initializations, enquiries and defaults */

#include "x11.h"

/* X11 compatibility */

#ifdef PRE_X11R5
#define XrmGetDatabase(wd) (wd->db)
#endif

/* Forward */
static int matchname();

/* Private globals */

Display *_wd;			/* The Display (== connection) */
Screen *_ws;			/* The screen */
XFontStruct *_wf;		/* The font */
XFontStruct *_wmf;		/* The menu font */
#ifdef PIPEHACK
int _wpipe[2];			/* Pipe used by wungetevent from handler */
#endif
#ifdef AMOEBA
#include <amoeba.h>
#include <semaphore.h>
extern semaphore *_wsema;	/* From amtimer.c */
#endif
char *_wprogname = "stdwin";	/* Program name from argv[0] */
static char *_wdisplayname;	/* Display name */

/* Interned atoms */

Atom _wm_protocols;
Atom _wm_delete_window;
Atom _wm_take_focus;

/* Globals used to communicate hints to wopen() */

char *_whostname;
char *_wm_command;
int _wm_command_len;

/* Copy the arguments away to _wm_command, and get the host name */

static void
storeargcargv(argc, argv)
	int argc;
	char **argv;
{
	int i;
	char *bp;
	char buf[256];
	
	/* Calculate length of buffer to allocate */
	_wm_command_len= 0;
	for (i= 0; i < argc; i++)
		_wm_command_len += strlen(argv[i]) + 1;
	if (_wm_command_len != 0) {
		_wm_command= bp= malloc((unsigned)_wm_command_len);
		for (i= 0; i < argc; i++) {
			strcpy(bp, argv[i]);
			bp += strlen(argv[i]) + 1;
		}
	}
	
	buf[0]= EOS;
	gethostname(buf, sizeof buf);
	buf[(sizeof buf)-1]= EOS;
	_whostname= strdup(buf);
}

/* Function to get a default directly from a database */

static char *
getoption(db, name, classname)
	XrmDatabase db;
	char *name;
	char *classname;
{
	char namebuf[256], classnamebuf[256];
	char *type;
	XrmValue rmvalue;
	
	sprintf(namebuf, "%s.%s", _wprogname, name);
	sprintf(classnamebuf, "Stdwin.%s", classname);
	if (XrmGetResource(db, namebuf, classnamebuf, &type, &rmvalue))
		return rmvalue.addr;
	else
		return NULL;
}

/* Command line options tables */

/* NOTE: To prevent stdwin eating user options scanned with getopt(3),
   single-letter options must never be acceptable as abbreviations.
   When necessary, this is ensured by adding dummy options, e.g., "-g:"
   because there is only one option starting with "-g" ("-geometry").
   (The colon is used as second character because getopt() uses it as a
   delimiter.)
   Watch out: if you remove the last option starting with a certain
   letter, also remove the dummy option!
*/

/* Table 1: command-line only options */

static XrmOptionDescRec options1[]= {
  
{"-debuglevel",		".debugLevel",		XrmoptionSepArg, NULL},
{"-display",		".display",		XrmoptionSepArg, NULL},

{"-n:",			NULL,			XrmoptionIsArg,  NULL},
{"-name",		".name",		XrmoptionSepArg, NULL},

{"-s:",			NULL,			XrmoptionIsArg,  NULL},
{"-synchronous",	".synchronous",		XrmoptionNoArg,  "on"},

};

/* Table 2: options to be merged with resources */

static XrmOptionDescRec options2[]= {

/* Command line options to override resources.
   All the options named in "man X" are supported, except those to
   do with borders (-bd, -bw) or national language choice (-xnl*). */

{"-background",		".background",		XrmoptionSepArg, NULL},
{"-bg",			".background",		XrmoptionSepArg, NULL},

{"-font",		".font",		XrmoptionSepArg, NULL},
{"-fn",			".font",		XrmoptionSepArg, NULL},
{"-foreground",		".foreground",		XrmoptionSepArg, NULL},
{"-fg",			".foreground",		XrmoptionSepArg, NULL},

{"-g:",			NULL,			XrmoptionIsArg,  NULL},
{"-geometry",		".geometry",		XrmoptionSepArg, NULL},

{"-iconic",		".iconic",		XrmoptionNoArg,  "on"},
{"-iconbitmap",		".iconBitmap",		XrmoptionSepArg, NULL},
{"-icongeometry",	".iconGeometry",	XrmoptionSepArg, NULL},
{"-iconmask",		".iconMask",		XrmoptionSepArg, NULL},

{"-menubackground",	".menuBackground",	XrmoptionSepArg, NULL},
{"-menubg",		".menuBackground",	XrmoptionSepArg, NULL},
{"-menubar",		".menubar",		XrmoptionNoArg,  "off"},
{"+menubar",		".menubar",		XrmoptionNoArg,  "on"},
{"-menufont",		".menuFont",		XrmoptionSepArg, NULL},
{"-menufn",		".menuFont",		XrmoptionSepArg, NULL},
{"-menuforeground",	".menuForeground",	XrmoptionSepArg, NULL},
{"-menufg",		".menuForeground",	XrmoptionSepArg, NULL},

{"-reversevideo",	".reverse",		XrmoptionNoArg,  "on"},
{"-rv",			".reverse",		XrmoptionNoArg,  "on"},
{"+rv",			".reverse",		XrmoptionNoArg,  "off"},

{"-s:",			NULL,			XrmoptionIsArg,  NULL},
{"-selectionTimeout",	".selectionTimeout",	XrmoptionSepArg, NULL},

{"-t:",			NULL,			XrmoptionIsArg,  NULL},
{"-title",		".title",		XrmoptionSepArg, NULL},

{"-xmenu",		".xmenu",		XrmoptionNoArg,  "off"},
{"+xmenu",		".xmenu",		XrmoptionNoArg,  "on"},
{"-xrm",		NULL,			XrmoptionResArg, NULL},

/* Options that end option processing completely:
   "-" means <stdin>, "--" is a getopt(3) convention
   to force the end of the option list. */

{"-",			NULL,			XrmoptionSkipLine, NULL},
{"--",			NULL,			XrmoptionSkipLine, NULL},

};

#define NUM(options) (sizeof options / sizeof options[0])


/* Initialization is split in two parts -- wargs(&argc, &argv) to process
   the command line, winit() to open the connection.  The call
   winitargs(&argc, &argv) calls wargs() and then winit().

   Programs that call winit() without calling wargs() will always
   appear to have the name "stdwin", and you can't pass X command
   line arguments on their command line.  Defaults put in the server's
   resource database for application name 'stdwin' will work, though.
   You can also set the environment variable RESOURCE_NAME. */


static XrmDatabase db = NULL; /* Passed between wargs() and winit() */
static int wargs_called; /* Set when wargs is called */

/* Part one of the initialization -- process command line arguments */

void
wargs(pargc, pargv)
	int *pargc;
	char ***pargv;
{
	char **argv= *pargv;
	char *value;
	
	if (wargs_called)
		_wfatal("wargs: called more than once");
	wargs_called = 1;
	
	/* Get the program name (similar to basename(argv[0])) */
	
	if (*pargc > 0 && argv[0] != NULL && argv[0][0] != EOS) {
		_wprogname= strrchr(argv[0], '/');
		if (_wprogname != NULL && _wprogname[1] != EOS)
			++_wprogname;
		else
			_wprogname= argv[0];
	}
	
	/* Save entire argument list for WM_COMMAND hint; plus hostname */
	
	storeargcargv(*pargc, argv);
	
	/* Get command-line-only X options */
	
	XrmParseCommand(&db, options1, NUM(options1),
		_wprogname, pargc, *pargv);
	
	/* Get -debuglevel argument from temp database.
	   This is done first so calls to _wdebug will work. */
	
	if ((value= getoption(db, "debugLevel", "DebugLevel")) != NULL) {
		_wtracelevel= _wdebuglevel= atoi(value);
#ifdef HAVE_SETLINEBUF
		setlinebuf(stderr);
#endif
		_wwarning("wargs: -debuglevel %d", _wdebuglevel, value);
	}
	
	/* Get -display argument from temp database */
	
	if ((_wdisplayname= getoption(db, "display", "Display")) != NULL)
		_wdebug(1, "wargs: -display %s", _wdisplayname);
	
	/* Get -name argument from temp database.
	   This will override the command name used by _wgetdefault.
	   You can use this to pretend the command name is different,
	   so it gets its options from a different part of the defaults
	   database.  (Try -name xmh. :-)
	   If -name is not set, getenv("RESOURCE_NAME") will override
	   _wprogname (if non-empty). */
	
	if ((value= getoption(db, "name", "Name")) != NULL) {
		_wdebug(1, "wargs: -name %s", value);
		_wprogname= strdup(value);
	}
	else if ((value = getenv("RESOURCE_NAME")) != NULL && *value != '\0') {
		_wdebug(1, "wargs: RESOURCE_NAME=%s", value);
		_wprogname = strdup(value);
	}
	
	/* Get the remaining command line options.
	   Option parsing is done in two phases so that if the user
	   specifies -name and -geometry, the geometry is stored under
	   the new name (else it won't be found by _wgetdefault later). */
	
	XrmParseCommand(&db, options2, NUM(options2),
		_wprogname, pargc, *pargv);
}


/* Part two of the initialization -- open the display */

void
winit()
{
	char *value;
	XrmDatabase def_db;

	/* Call wargs() with dummy arguments if not already called */

	if (!wargs_called) {
		static char* def_args[]= {"stdwin", (char*)NULL};
		int argc= 1;
		char **argv= def_args;
		wargs(&argc, &argv);
	}
	
	/* Open the display, die if we can't */
	
	_wd= XOpenDisplay(_wdisplayname);
	if (_wd == NULL) {
		_wfatal("winit: can't open display (%s)",
			_wdisplayname ? _wdisplayname :
			getenv("DISPLAY") ? getenv("DISPLAY") : "<none>");
	}
	
#ifdef AMOEBA
	/* Set the semaphore.  This must be done before doing anything
	   else with the connection. */
	if (_wsema != NULL)
		XamSetSema(_wd, _wsema);
#endif
	
	/* Turn on synchronous mode if required.
	   This is not automatic when debuglevel is set,
	   since some bugs disappear in synchronous mode! */
	
	if (getoption(db, "synchronous", "Synchronous") != NULL)
		XSynchronize(_wd, True);
	
	/* Call XGetDefault() once.  We don't use it to get our
	   defaults, since it doesn't let the caller specify the class
	   name for the resource requested, but the first call to it
	   also initializes the resources database from various sources
	   following conventions defined by the X toolkit, and that code
	   is too convoluted (and X11-version specific?) to bother to
	   repeat it here. */
	
	(void) XGetDefault(_wd, _wprogname, "unused");
	
	/* From now on, use _wgetdefault() exclusively to get defaults */
	
	/* Get the debug level again, this time from the user's
	   defaults database.  This value overrides only if larger. */
	
	value= _wgetdefault("debugLevel", "DebugLevel");
	if (value != 0) {
		int k= atoi(value);
		if (k > _wdebuglevel) {
			_wtracelevel= _wdebuglevel= k;
			_wdebug(1, "winit: new debuglevel %d (%s)",
				_wdebuglevel, value);
			XSynchronize(_wd, True);
		}
	}
	
	/* Merge the command line options with the defaults database.
	   This must be done after the call to XGetDefault (above),
	   otherwise XGetDefault doesn't care to load the database.
	   I assume that the command line options get higher priority
	   than the user defaults in this way. */

	def_db = XrmGetDatabase(_wd);
	XrmMergeDatabases(db, &def_db);
	db = NULL;
	
	/* Get the default screen (the only one we use) */
	
	_ws= DefaultScreenOfDisplay(_wd);
	
	_wdebug(1, "server does%s save-unders",
		DoesSaveUnders(_ws) ? "" : "n't do");
	
	/* Intern some atoms (unconditionally) */
	
	_wm_protocols = XInternAtom(_wd, "WM_PROTOCOLS", False);
	_wm_delete_window = XInternAtom(_wd, "WM_DELETE_WINDOW", False);
	_wm_take_focus = XInternAtom(_wd, "WM_TAKE_FOCUS", False);

	/* Initialize window defaults */

	_winsetup();
	
	/* Initialize font list */
	
	_winitfonts();

	/* Initialize colors */

	_w_initcolors();
	
#ifdef PIPEHACK
	/* Create the pipe used to communicate wungetevent calls
	   from a signal handler to wgetevent */
	
	if (pipe(_wpipe) != 0) {
		_wwarning("winit: can't create pipe");
		_wpipe[0]= _wpipe[1]= -1;
	}
#endif
}


/* Call both parts of the initialization together */

void
winitargs(pargc, pargv)
	int *pargc;
	char ***pargv;
{
	wargs(pargc, pargv);
	winit();
}


/* Return the name of the display, if known.  (X11 stdwin only.) */

char *
wdisplayname()
{
	return _wdisplayname ? _wdisplayname : getenv("DISPLAY");
}


/* Return the connection number.  (X11 stdwin only.) */

int
wconnectionnumber()
{
	return ConnectionNumber(_wd);
}


/* Clean up */

void
wdone()
{
	/* This may be called when we are not initialized */
	if (_wd) {
		_wkillwindows();
		_wkillmenus();
		XFlush(_wd); /* Show possibly queued visual effects */
		XCloseDisplay(_wd);
		_wd = NULL;
	}
}

/* Flush server queue */

void
wflush()
{
	XFlush(_wd);
}


/* Get screen size */

void
wgetscrsize(pwidth, pheight)
	int *pwidth, *pheight;
{
	*pwidth= WidthOfScreen(_ws);
	*pheight= HeightOfScreen(_ws);
}


/* Get screen size in mm */

void
wgetscrmm(pwidth, pheight)
	int *pwidth, *pheight;
{
	*pwidth= WidthMMOfScreen(_ws);
	*pheight= HeightMMOfScreen(_ws);
}

/* Subroutine to invert a rectangle.
   On a colour display, we invert all planes that have a different
   value in the foreground and background pixels, thus swapping
   fg anbd bg colors.
   Since we can't get the fg and bg pixels from a GC without cheating
   (i.e., risking future incompatibility), we depend on the fact
   that the plane mask is *always* set to the XOR of the fg and bg colors.
   If the bg color for the window is the same as the bg for the GC,
   this means all draw operations need only be concerned with those planes.
*/

_winvert(d, gc, x, y, width, height)
	Drawable d;
	GC gc;
	int x, y, width, height;
{
	XSetFunction(_wd, gc, GXinvert);
	XFillRectangle(_wd, d, gc, x, y, width, height);
	XSetFunction(_wd, gc, GXcopy);
}

/* Get a default (replaces XGetDefault calls) */

#include <ctype.h>

char *
_wgetdefault(name, classname)
	char *name;
	char *classname;
{
	char namebuf[256], classnamebuf[256];
	register char *p;
	
	p = getoption(XrmGetDatabase(_wd), name, classname);
	if (p != NULL)
		return p;
	
	/* XXX Compatibility hack.  Previous versions of STDWIN used
	   a different resource format, where the name would be
	   <progname>.stdwin.<resname> and the classname was defaulted
	   by XGetDefault to Program.Name (or some such).
	   The resource names were also all lowercase. */
	sprintf(namebuf, "stdwin.%s", name);
	sprintf(classnamebuf, "Stdwin.%s", classname);
	for (p = namebuf; *p != '\0'; p++) {
		if (isupper(*p))
			*p = tolower(*p);
	}
	_wdebug(1, "_wgetdefault(%s, %s): no luck, trying %s, %s",
		name, classname, namebuf, classnamebuf);
	return getoption(XrmGetDatabase(_wd), namebuf, classnamebuf);
}

/* Get a Boolean default.  May be 'on' or 'off', 'true' or 'false',
   '1' or '0'.  If not present, return the default value. */

int
_wgetbool(name, classname, def)
	char *name;
	char *classname;
	int def;
{
	register char *value;
	struct flags {
		char *name;
		int value;
	};
	static struct flags flags[] = {
		{"on",		1},
		{"true",	1},
		{"yes",		1},
		{"1",		1},
		
		{"off",		0},
		{"false",	0},
		{"no",		0},
		{"0",		0},
		
		{NULL,		0}	/* Sentinel */
	};
	struct flags *fp, *hit;
	
	value = _wgetdefault(name, classname);
	if (value == NULL)
		return def;
	hit = NULL;
	for (fp = flags; fp->name != NULL; fp++) {
		if (matchname(fp->name, value)) {
			if (hit != NULL)
				_wwarning(
				    "ambiguous resource value for %s: %s",
				    name, value);
			hit = fp;
		}
	}
	if (hit != NULL)
		def = hit->value;
	else
		_wwarning("unknown resource value for %s: %s", name, value);
	return def;
}

static int
matchname(fullname, shortname)
	char *fullname;
	char *shortname;
{
	register int c;
	while ((c = *shortname++) != '\0') {
		if (isupper(c))
			c = tolower(c);
		if (c != *fullname++)
			return 0;
	}
	return 1;
}
