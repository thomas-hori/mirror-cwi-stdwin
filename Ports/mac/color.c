/* MAC STDWIN -- COLOR HANDLING. */

#include "macwin.h"

/* Provisional color encoding:
   - low 16 bits: a color as specified in <QuickDraw.h>
   - high 16 bits: a shade factor, 0 -> full, 128 -> 50%
   See also draw.c
*/

static struct colorentry {
	char *name;
	long color;
} colorlist[] = {
	{"black",	blackColor},
	{"white",	whiteColor},
	{"red",		redColor},
	{"green",	greenColor},
	{"blue",	blueColor},
	{"cyan",	cyanColor},
	{"magenta",	magentaColor},
	{"yellow",	yellowColor},
	{"gray25",	blackColor + (192L << 16)},
	{"gray50",	blackColor + (128L << 16)},
	{"gray75",	blackColor + (64L << 16)},
	{"foreground",	blackColor},
	{"background",	whiteColor},
	{NULL,		0}	/* Sentinel */
};

long
wfetchcolor(colorname)
	char *colorname;
{
	struct colorentry *p;
	
	for (p = colorlist; p->name != NULL; p++) {
		if (strcmp(p->name, colorname) == 0)
			return p->color;
	}
	
	dprintf("wmakecolor: unknown color '%s'", colorname);
	return blackColor;
}
