/* dpv -- ditroff previewer.  Funny character translation. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"

char	*funnyfile;	/* File to read table from */

char	*funnytries[]= {	/* Alternative files to try */
		"funnytab",
		"/usr/local/lib/funnytab",
		"/userfs3/amoeba/lib/funnytab",
		"/ufs/guido/lib/funnytab",
		NULL
	};

/* Funny character translation table.
   The table implies that character 'name' (the ditroff input name,
   e.g. "bu" for bullet, which is input as "\(bu") must be translated
   the string 'trans' drawn in font 'font', or in the current font
   if 'font' is NULL.  'Trans' is a string because some characters
   need several real characters to print them (e.g., ligatures).
   For higher-quality output, the table should be extended to
   include a point size increment/decrement and possibly a (dh, dv)
   translation; but what the heck, this is only a previewer! */

static struct _funny {
	char name[4];
	char trans[4];
	char *fontname;
};

/* Read funny character translation table.
   File format:
   	name	fontname	translation
   where:
   	name is the ditroff character name, e.g., bs for \(bs
   	fontname is the font name, or - if the translation uses the current font
   	translation is one or more hexadecimal numbers, or a string
   	enclosed in double quotes
   	In any case the string may be no more than 3 characters long
*/

readfunnytab(filename)
	char *filename;
{
	char buf[BUFSIZ];
	FILE *fp= fopen(filename, "r");
	if (fp == NULL) {
		if (dbg > 0)
			fprintf(stderr, "Can't open funnytab %s\n", filename);
		return FALSE;
	}
	if (dbg > 0)
		fprintf(stderr, "Reading funnytab from %s\n", filename);
	while (fgets(buf, sizeof buf, fp) != NULL) {
		char *name;
		char *fontname;
		char *trans;
		char ctrans[4];
		char *p= buf;
		while (*p != EOS && isspace(*p))
			++p;
		if (*p == EOS)
			continue;
		name= p;
		while (*p != EOS && !isspace(*p))
			++p;
		if (*p == EOS)
			continue;
		*p++ = EOS;
		while (*p != EOS && isspace(*p))
			++p;
		if (*p == EOS)
			continue;
		fontname= p;
		while (*p != EOS && !isspace(*p))
			++p;
		if (*p == EOS)
			continue;
		*p++ = EOS;
		while (*p != EOS && isspace(*p))
			++p;
		if (*p == EOS)
			continue;
		if (*p == '"') {
			trans= ++p;
			while (*p != EOS && *p != EOL && *p != '"')
				++p;
			*p= EOS;
		}
		else if (*p == '0' && p[1] == 'x') {
			int a= 0, b= 0, c= 0;
			(void) sscanf(p, "0x%x 0x%x 0x%x", &a, &b, &c);
			ctrans[0]= a;
			ctrans[1]= b;
			ctrans[2]= c;
			ctrans[3]= EOS;
			trans= ctrans;
		}
		else
			error(WARNING, "almost-match in funnytab");
		addtranslation(name, fontname, trans);
	}
	fclose(fp);
	sorttranslations();
	return TRUE;
}

int nfunny;
struct _funny *funnytab;

static
addtranslation(name, fontname, trans)
	char *name, *fontname, *trans;
{
	struct _funny f;
	strncpy(f.name, name, 4);
	f.name[3]= EOS;
	strncpy(f.trans, trans, 4);
	f.trans[3]= EOS;
	if (fontname == NULL || fontname[0] == EOS ||
		fontname[0] == '-' && fontname[1] == EOS)
		f.fontname= NULL;
	else {
		static char *lastfontname;
		if (lastfontname == NULL ||
			strcmp(fontname, lastfontname) != 0)
			lastfontname= strdup(fontname);
		f.fontname= lastfontname;
	}
	L_APPEND(nfunny, funnytab, struct _funny, f);
	if (funnytab == NULL)
		error(FATAL, "out of mem for funnytab");
}

static
funnycmp(p, q)
	struct _funny *p, *q;
{
	return strcmp(p->name, q->name);
}

static
sorttranslations()
{
	/* Don't sort -- the lookup algorithm depends on the order */
#if 0
	if (nfunny > 1)
		qsort(funnytab, nfunny, sizeof(struct _funny), funnycmp);
#endif
}

/* Draw a funny character.  Called from put1s. */

drawfunny(name)
	register char *name;
{
	register struct _funny *f;
	TEXTATTR save;
	static bool inited;
	
	if (!inited) {
		if (funnyfile != NULL) {
			/* Explicitly specified funnyfile -- must exist */
			if (!readfunnytab(funnyfile))
				error(FATAL, "can't find funnytab file %s",
					funnyfile);
		}
		else {
			/* Try to find default funnyfile */
			int i= 0;
			while ((funnyfile= funnytries[i++]) != NULL) {
				if (readfunnytab(funnyfile))
					break;
			}
			/* If not found, limp ahead... */
			if (funnyfile == NULL)
				error(WARNING,
					"can't find default funnytab");
		}
		inited= TRUE;
	}
	
	/* If this is too slow, could use binary search and/or
	   replace the strcmp by an explicit comparison of two
	   characters.  But spend your time on the program's
	   main loop (especially, 'put1') first! */
	for (f= funnytab; f < &funnytab[nfunny]; ++f) {
		/* Assume names are always 1 or 2 chars */
		if (f->name[0] == name[0] && f->name[1] == name[1])
			break;
	}
	if (f >= &funnytab[nfunny])
		return; /* Unknown character -- don't draw it */
	if (f->fontname != NULL) {
		char buf[256];
		wgettextattr(&save);
		fonthack(f->fontname);
	}
	wdrawtext(HWINDOW, VWINDOW - wbaseline(), f->trans, -1);
	if (f->fontname != NULL)
		wsettextattr(&save);
}
