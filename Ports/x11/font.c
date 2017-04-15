/* X11 STDWIN -- font manipulation */

#include "x11.h"

TEXTATTR wattr;		/* Global text attributes */

/* Forward */
static int fontnum();
static void makelower();

/* Setting text drawing parameters. */

void
wsetplain()
{
	wattr.style= PLAIN;
}

void
wsethilite()
{
	wattr.style |= HILITE;
}

void
wsetinverse()
{
	wattr.style |= INVERSE;
}

void
wsetitalic()
{
	wattr.style |= ITALIC;
}

void
wsetbold()
{
	wattr.style |= BOLD;
}

void
wsetbolditalic()
{
	wattr.style |= BOLD|ITALIC;
}

void
wsetunderline()
{
	wattr.style |= UNDERLINE;
}

int
wsetfont(fontname)
	char *fontname;
{
	int i = fontnum(fontname);
	if (i == 0)
		return 0;
	wattr.font= i;
	_wfontswitch();
	return 1;
}

void
wsetsize(pointsize)
	int pointsize;
{
	/* Ignored for now; must be present for compatibility */
}

void
wgettextattr(attr)
	TEXTATTR *attr;
{
	*attr= wattr;
}

void
wsettextattr(attr)
	TEXTATTR *attr;
{
	wattr= *attr;
	_wfontswitch();
}

/* Font administration */

struct font {
	char *name;		/* Font name, (lower case), NULL if unknown */
	XFontStruct *info;	/* Text measuring info */
};

static int nfonts;
static struct font *fontlist;

/* Initialize the font stuff */

_winitfonts()
{
	struct font stdfont, menufont;
	
	/* Get the user-specified or server default font */
	
	stdfont.name= _wgetdefault("font", "Font");
	if (stdfont.name != NULL) {
		stdfont.info= XLoadQueryFont(_wd, stdfont.name);
		if (stdfont.info != NULL) {
			stdfont.name= strdup(stdfont.name);
		}
		else {
		    _wwarning(
		      "_winitfonts: can't load font %s; using server default",
		      stdfont.name);
		    stdfont.name= NULL;
		}
	}
	if (stdfont.name == NULL) {
		stdfont.info= XQueryFont(_wd,
			XGContextFromGC(DefaultGCOfScreen(_ws)));
		if (stdfont.info == NULL)
			_wfatal("_winitfonts: no server default font");
		stdfont.info->fid= 0;
	}
	L_APPEND(nfonts, fontlist, struct font, stdfont);
	_wmf= _wf= stdfont.info;
	
	/* Get the user-specified default menu font.
	   If no user-specified default, use the the standard font. */
	
	menufont.name= _wgetdefault("menuFont", "Font");
	if (menufont.name != NULL) {
		menufont.info= XLoadQueryFont(_wd, menufont.name);
		if (menufont.info != NULL) {
			menufont.name= strdup(menufont.name);
			L_APPEND(nfonts, fontlist, struct font, menufont);
			_wmf= menufont.info;
		}
		else {
		    _wwarning(
		      "_winitfonts: can't load font %s; using server default",
		      menufont.name);
		}
	}
}

/* Start using the font from 'wattr' */

_wfontswitch()
{
	if (wattr.font < 0 || wattr.font >= nfonts)
		_werror("_wfontswitch: wattr.font out of range");
	else {
		_wf= fontlist[wattr.font].info;
		_wgcfontswitch();
	}
}

/* Return the font number for a given font name.
   If not already in the font list, add it.
   Return 0 (the number of the system font) if the font doesn't exist. */

static int
fontnum(name)
	char *name;
{
	struct font newfont;
	char lcname[256];
	int i;
	
	if (name == NULL || name[0] == EOS)
		return 0; /* Use standard font */
	
	makelower(lcname, name);
	for (i= 0; i < nfonts; ++i) {
		if (fontlist[i].name != NULL &&
			strcmp(fontlist[i].name, lcname) == 0)
			return i;
	}
	newfont.info= XLoadQueryFont(_wd, lcname);
	if (newfont.info == NULL) {
		_wdebug(2, "fontnum: no font %s", lcname);
		return 0; /* Not found; use system font */
	}
	_wdebug(1, "fontnum: new font %s", lcname);
	newfont.name= strdup(lcname);
	L_APPEND(nfonts, fontlist, struct font, newfont);
	return i;
}

/* Copy a string like strcpy, converting to lower case in the process */

static void
makelower(dest, src)
	char *dest, *src;
{
	char c;
	while ((c= *src++) != EOS) {
		if (isupper(c))
			c= tolower(c);
		*dest++ = c;
	}
	*dest= EOS;
}

/* List of font names returned by last wlistfontnames call */

static char **fontnames = NULL;

/* Return a list of all available font names matching a pattern */

char **
wlistfontnames(pattern, p_count)
	char *pattern;
	int *p_count;
{
	if (pattern == NULL || pattern[0] == '\0')
		pattern = "*";
	if (fontnames != NULL) {
		XFreeFontNames(fontnames);
		fontnames = NULL;
	}
	fontnames = XListFonts(_wd, pattern, 5000, p_count);
	return fontnames;
}
