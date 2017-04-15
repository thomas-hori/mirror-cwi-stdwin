/* dpv -- ditroff previewer.  Font translation. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"

/* Font translation table, to map ditroff font names to font names
   STDWIN (really the underlying window system) understands.
   This information should really be read from a configuration file.
   For now, we know to map some ditroff names for Harris and PostScript
   fonts to more or less equivalent X11 and Mac fonts.
   (The X fonts used are to be found in the Andrew distribution).
   Because the current font scheme in STDWIN for X doesn't know about
   wsetsize and wsetbold c.s., the translation process is different
   for X than for the Macintosh. */

#define NSIZES 10

#define R	0
#define B	1
#define I	2
#define BI	3

static struct _translate {
	char *harname;
	char *xname;
	int style;
	int sizes[NSIZES+1];
} default_translate[]= {
#ifdef macintosh
	{"R",	"Times",	R},
	{"I",	"Times",	I},
	{"B",	"Times",	B},
	{"BI",	"Times",	BI},
	
	{"H",	"Helvetica",	R},
	{"HO",	"Helvetica",	I},
	{"HB",	"Helvetica",	B},
	{"HD",	"Helvetica",	BI},
	
	{"C",	"Courier",	R},
	{"CO",	"Courier",	I},
	{"CB",	"Courier",	B},
	{"CD",	"Courier",	BI},
	
	{"lp",	"Courier",	R},
	
	{"Vl",	"Helvetica",	R},
	{"vl",	"Helvetica",	R},
	{"V",	"Helvetica",	R},
	{"v",	"Helvetica",	R},
	
	/* Font used by funny character translation */
	
	{"Symbol", "Symbol",	R},
#else
#ifdef X11R2
	/* X11 Release 2, using fonts from Andrew */
	
	/* PostScript font names */
	
	{"R",	"times%d",	R, 8, 10, 12, 16, 22, 0},
	{"I",	"times%di",	R, 8, 10, 12, 16, 22, 0},
	{"B",	"times%db",	R, 8, 10, 12, 16, 22, 0},
	{"BI",	"times%dbi",	R, 8, 10, 12, 16, 22, 0},
	
	{"H",	"helvetica%d",	R, 8, 10, 12, 16, 22, 0},
	{"HO",	"helvetica%di",	R, 8, 10, 12, 16, 22, 0},
	{"HB",	"helvetica%db",	R, 8, 10, 12, 16, 22, 0},
	{"HD",	"helvetica%dbi",R, 8, 10, 12, 16, 22, 0},
	
	{"C",	"courier%df",	R, 8, 10, 12, 16, 22, 0},	
	{"CO",	"courier%dif",	R, 8, 10, 12, 16, 22, 0},
	{"CB",	"courier%dbf",	R, 8, 10, 12, 16, 22, 0},
	{"CD",	"courier%dbif", R, 8, 10, 12, 16, 22, 0},
	
	/* CWI Harris font names (also R, I, B, BI) */
	
	/* Vega light, Vega, Vega medium (Helvetica look-alike) */
	{"Vl",	"helvetica%d",	R, 8, 10, 12, 16, 22, 0},
	{"vl",	"helvetica%di",	R, 8, 10, 12, 16, 22, 0},
	{"V",	"helvetica%d",	R, 8, 10, 12, 16, 22, 0},
	{"v",	"helvetica%di",	R, 8, 10, 12, 16, 22, 0},
	{"Vm",	"helvetica%db",	R, 8, 10, 12, 16, 22, 0,},
	{"vm",	"helvetica%dbi",R, 8, 10, 12, 16, 22, 0,},
	
	/* Baskerville (see also small caps) */
	{"br",	"times%d",	R, 8, 10, 12, 16, 22, 0,},
	{"bi",	"times%di",	R, 8, 10, 12, 16, 22, 0,},
	{"bb",	"times%db",	R, 8, 10, 12, 16, 22, 0,},
	{"bI",	"times%dbi",	R, 8, 10, 12, 16, 22, 0,},
	
	/* Century Schoolbook */
	{"cr",	"times%d",	R, 8, 10, 12, 16, 22, 0,},
	{"ci",	"times%di",	R, 8, 10, 12, 16, 22, 0,},
	{"cb",	"times%db",	R, 8, 10, 12, 16, 22, 0,},
	{"cI",	"times%dbi",	R, 8, 10, 12, 16, 22, 0,},
	
	/* Laurel */
	{"lr",	"times%d",	R, 8, 10, 12, 16, 22, 0,},
	{"li",	"times%di",	R, 8, 10, 12, 16, 22, 0,},
	{"lb",	"times%db",	R, 8, 10, 12, 16, 22, 0,},
	{"lI",	"times%dbi",	R, 8, 10, 12, 16, 22, 0,},
	
	/* Funny fonts mapped to Helvetica, at least they differ from Times */
	{"G3",	"helvetica%d",	R, 8, 10, 12, 16, 22, 0,}, /* German no 3 */
	{"fs",	"helvetica%d",	R, 8, 10, 12, 16, 22, 0,}, /* French Script */
	{"RS",	"helvetica%di",	R, 8, 10, 12, 16, 22, 0,}, /* Rose Script */
	{"SO",	"helvetica%db",	R, 8, 10, 12, 16, 22, 0,}, /* Scitype Open */
	
	/* OCR-B (line printer font) */
	{"lp",	"courier%dbf",	R, 8, 10, 12, 0},
	
	/* Small caps fonts mapped to normal fonts */
	{"Rs",	"times%d",	R, 8, 10, 12, 16, 22, 0,}, /* Times */
	{"Bs",	"times%db",	R, 8, 10, 12, 16, 22, 0,}, /* Times bold */
	{"bs",	"times%d",	R, 8, 10, 12, 16, 22, 0,}, /* Baskerville */
	{"bS",	"times%db",	R, 8, 10, 12, 16, 22, 0,}, /* Baskerv. bold */
	
	/* Fonts used by funny character translation */
	
	{"symbol", "symbol%d",	R, 8, 10, 12, 16, 22, 0},
	{"symbola","symbola%d", R, 8, 10, 12, 16, 22, 0},
#else /* ! X11R3 */
	/* X11 Release 3 fonts */
	/* Also works for Release 4 */
	
	/* PostScript font names */
	
	{"R",	"-*-times-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"I",	"-*-times-medium-i-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"B",	"-*-times-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"BI",	"-*-times-bold-i-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	
	{"H",	"-*-helvetica-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"HO",	"-*-helvetica-medium-o-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"HB",	"-*-helvetica-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"HD",	"-*-helvetica-bold-o-*--%d-*",R, 8, 10, 12, 14, 18, 24, 0},
	
	{"C",	"-*-courier-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},	
	{"CO",	"-*-courier-medium-o-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"CB",	"-*-courier-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"CD",	"-*-courier-bold-o-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	
	/* CW is a common alias for C */
	{"CW",	"-*-courier-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},	
	
	/* CWI Harris font names (also R, I, B, BI) */
	
	/* Vega light, Vega, Vega medium (Helvetica look-alike) */
	{"Vl",	"-*-helvetica-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"vl",	"-*-helvetica-medium-o-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"V",	"-*-helvetica-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"v",	"-*-helvetica-medium-o-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
	{"Vm",	"-*-helvetica-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"vm",	"-*-helvetica-bold-o-*--%d-*",R, 8, 10, 12, 14, 18, 24, 0,},
	
	/* Baskerville (see also small caps) */
	{"br",	"-*-times-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"bi",	"-*-times-medium-i-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"bb",	"-*-times-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"bI",	"-*-times-bold-i-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	
	/* Century Schoolbook */
	{"cr",	"-*-new century schoolbook-medium-r-*--%d-*",
	R, 8, 10, 12, 14, 18, 24, 0,},
	{"ci",	"-*-new century schoolbook-medium-i-*--%d-*",
	R, 8, 10, 12, 14, 18, 24, 0,},
	{"cb",	"-*-new century schoolbook-bold-r-*--%d-*",
	R, 8, 10, 12, 14, 18, 24, 0,},
	{"cI",	"-*-new century schoolbook-bold-i-*--%d-*",
	R, 8, 10, 12, 14, 18, 24, 0,},
	
	/* Laurel */
	{"lr",	"-*-times-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"li",	"-*-times-medium-i-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"lb",	"-*-times-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	{"lI",	"-*-times-bold-i-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	
	/* Funny fonts mapped to Helvetica, at least they differ from Times */
	{"G3",	"-*-helvetica-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* German no 3 */
	{"fs",	"-*-helvetica-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* French Script */
	{"RS",	"-*-helvetica-medium-o-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* Rose Script */
	{"SO",	"-*-helvetica-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* Scitype Open */
	
	/* OCR-B (line printer font) */
	{"lp",	"-*-courier-bold-r-*--%d-*", R, 8, 10, 12, 0},
	
	/* Small caps fonts mapped to normal fonts */
	{"Rs",	"-*-times-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* Times */
	{"Bs",	"-*-times-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* Times bold */
	{"bs",	"-*-times-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* Baskerville */
	{"bS",	"-*-times-bold-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0,},
	/* Baskerville bold */
	
	/* Fonts used by funny character translation */
	
	{"r3symbol", "*-*-symbol-medium-r-*--%d-*", R, 8, 10, 12, 14, 18, 24, 0},
#endif /* ! X11R2 */
#endif /* ! macintosh */
	
	{NULL,	NULL}
};

static struct _translate *translate = default_translate;

/* Tell STDWIN to use the desired font and size.
   Call at start of page and after each font or size change. */

usefont()
{
	char *fontname;
	
	if (showpage != ipage)
		return;
	
	fontname= fonts.name[font];
	if (fontname != NULL)
		fonthack(fontname);
	/* Else, font not loaded -- assume in initialization phase */
	baseline= wbaseline();
	lineheight= wlineheight();
	recheck();
}

fonthack(fontname)
	char *fontname;
{
	int i;
	int havesize;
	char *harname;
	char buf[256];
	struct _translate *t;
	
	for (t= translate; t->harname != NULL; ++t) {
		if (strcmp(t->harname, fontname) == 0)
			break;
	}
	if (t->harname == NULL) {
		fontwarning(fontname);
	}
	else {
#ifdef macintosh
		wsetfont(t->xname);
		wsetsize(size);
		wsetplain();
		switch (t->style) {
		case I: wsetitalic(); break;
		case B: wsetbold(); break;
		case BI: wsetbolditalic(); break;
		}
#else
		havesize= size;
		for (i= 0; ; ++i) {
			if (t->sizes[i] >= havesize || t->sizes[i+1] == 0) {
				havesize= t->sizes[i];
				break;
			}
		}
		sprintf(buf, t->xname, havesize);
		wsetfont(buf);
#endif
	}
}

/* Issue a warning.
   Avoid warning many times in a row about the same font. */

fontwarning(fontname)
	char *fontname;
{
	static char lastwarning[200];
	char buf[256];
	
	if (strncmp(lastwarning, fontname, sizeof lastwarning) != 0) {
		strncpy(lastwarning, fontname, sizeof lastwarning);
		sprintf(buf, "mapping for font %s unknown", fontname);
		error(WARNING, buf);
	}
}

/* Get a token */

static char *
gettok(pp)
	char **pp;
{
	char *p = *pp;
	char *start;
	char c;
	
	while (isspace(c = *p++)) /* Skip to start of token */
		;
	if (c == '\0' || c == '\n' || c == '#') {
		if (dbg > 2)
			if (c == '#')
				fprintf(stderr, "next token is comment\n");
			else if (c == '\n')
				fprintf(stderr, "next token is newline\n");
			else
				fprintf(stderr, "next token is EOS\n");
		return NULL; /* End of line of comment */
	}
	start = p-1; /* Remember start of token */
	while (!isspace(c = *p++) && c != '\0') /* Skip to end of token */
		;
	if (c == '\0')
		p--;
	else
		p[-1] = '\0'; /* Zero-terminate token */
	*pp = p; /* Start point for next token */
	if (dbg > 2)
		fprintf(stderr, "next token: %s\n", start);
	return start;
}

static void
addtrans(pnt, pt, harname, xname, style, sizes)
	int *pnt;
	struct _translate **pt;
	char *harname;
	char *xname;
	int style;
	int sizes[];
{
	int i = (*pnt)++;
	struct _translate *t = *pt;
	
	if (i == 0)
		t = (struct _translate *) malloc(sizeof(struct _translate));
	else
		t = (struct _translate *)
			realloc((char *)t, sizeof(struct _translate) * *pnt);
	if (t == NULL)
		error(FATAL, "not enough memory for font translations");
	*pt = t;
	t += i;
	t->harname = strdup(harname);
	t->xname = strdup(xname);
	t->style = style;
	for (i = 0; i <= NSIZES; i++)
		t->sizes[i] = sizes[i];
}

/* Read a file of alternative font translations. */

readtrans(filename)
	char *filename;
{
	struct _translate *t;
	int nt;
	FILE *fp;
	int line;
	char buf[1000];
	char *p;
	char *harname;
	char *xname;
	char *sizetemp;
	int sizes[NSIZES+1];
	static int defsizes[NSIZES+1] = {8, 10, 12, 14, 18, 24, 0};
	int nsizes;
	
	fp = fopen(filename, "r");
	if (fp == NULL)
		error(FATAL, "can't find font translations file %s", filename);
	if (dbg > 0)
		fprintf(stderr, "reading translations from %s\n", filename);
	t = NULL;
	nt = 0;
	line = 0;
	while (fgets(buf, sizeof buf, fp) != NULL) {
		line++;
		if (dbg > 1)
			fprintf(stderr, "line %d: %s\n", line, buf);
		p = buf;
		harname = gettok(&p);
		if (harname == NULL)
			continue; /* Blank line or comment */
		xname = gettok(&p);
		if (xname == NULL) {
			error(WARNING, "%s(%d): incomplete line (only '%s')",
						filename, line, harname);
			continue;
		}
		/* Style is always R */
		nsizes = 0;
		while (nsizes < NSIZES && (sizetemp = gettok(&p)) != NULL) {
			sizes[nsizes] = atoi(sizetemp);
			if (sizes[nsizes] <= 0 ||
					nsizes > 0 &&
					sizes[nsizes] <= sizes[nsizes-1]) {
				error(WARNING,
				    "%s(%d): bad or non-increasing size '%s'",
				    filename, line, sizetemp);
			}
			else
				nsizes++;
		}
		if (nsizes > 0)
			while (nsizes <= NSIZES)
				sizes[nsizes++] = 0;
		addtrans(&nt, &t, harname, xname, R,
			nsizes == 0 ? defsizes : sizes);
	}
	if (dbg > 0)
		fprintf(stderr, "done reading translations.\n");
	fclose(fp);
	if (nt == 0)
		error(FATAL, "%s: no valid font translations", filename);
	addtrans(&nt, &t, (char *)NULL, (char *)NULL, R, defsizes);
	translate = t;
}
