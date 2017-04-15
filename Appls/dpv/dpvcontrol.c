/* dpv -- ditroff previewer.  User interface, controlling the rest. */

#include "dpv.h"
#include "dpvmachine.h"
#include "dpvoutput.h"

#ifdef unix
#define DO_PRINTMENU
#endif

char *devname;

preview(file, firstpage)
	char *file;
	int firstpage;
{
	initialize(file, firstpage);
	addmenus();
	eventloop(file);
	cleanup();
	wdone();
}

MENU *mainmmenu;

/* Menu IDs */
#define MAINMENU	1
#define PRINTMENU	2

/* Control menu items */
#define FIRSTPAGE	0
#define PREVPAGE	1
#define NEXTPAGE	2
#define LASTPAGE	3
/* --- */
#define GOTOPAGE	5
/* --- */
#define QUIT		7

#ifdef DO_PRINTMENU

MENU *printmenu;

#define MAXNPRINT 50	/* Max # items in print menu */

struct _printitem {
	char *text;	/* Menu item text */
	char *device;	/* Required device, or NULL if N/A */
	char *command;	/* Shell command to execute */
} printitems[MAXNPRINT]= {
	/* XXX This is extremely site-dependent */
	{"Typeset on Harris",		"har",	"lpr -Phar -n %s"},
	{"Harris emulation on Agfa",	"har",	"toagfa %s"},
	{"Print on ${PRINTER-psc}",	"psc",	"lpr -P${PRINTER-psc} -n %s"},
	{"Print on Oce",		"psc",	"lpr -Poce -n %s"},
	{"Print on PostScript",		"psc",	"lpr -Ppsc -n %s"},
	{"Print on Agfa",		"psc",	"lpr -Pagfa -n %s"},
};

int nprint;

int
countprintmenu()
{
	while (nprint < MAXNPRINT && printitems[nprint].text != NULL)
		nprint++;
}

int
addprinter(name)
	char *name;
{
	char buf[100];
	countprintmenu();
	if (nprint >= MAXNPRINT) {
		error(WARNING, "too many printer definitions, rest igonred");
		return;
	}
	sprintf(buf, "Print on %s", name);
	printitems[nprint].text = strdup(buf);
	printitems[nprint].device = NULL; /* Unspecified */
	sprintf(buf, "lpr -P%s -n %%s", name);
	printitems[nprint].command = strdup(buf);
	nprint++;
}

#endif

addmenus()
{
	MENU *mp;
	int i;
	
	mainmmenu= mp= wmenucreate(MAINMENU, "Command");
	
	wmenuadditem(mp, "First page", 'F');
	wmenuadditem(mp, "Previous page", 'P');
	wmenuadditem(mp, "Next page", 'N');
	wmenuadditem(mp, "Last page", 'L');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Go to page...", 'G');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Quit", 'Q');
	
#ifdef DO_PRINTMENU
	countprintmenu();
	printmenu= mp= wmenucreate(PRINTMENU, "Print");
	for (i= 0; i < nprint; ++i) {
		wmenuadditem(mp, printitems[i].text, -1);
		if (!canprint(i))
			wmenuenable(mp, i, FALSE);
	}
#endif
}

eventloop(filename)
	char *filename;
{
	int num= -1;
	for (;;) {
		EVENT e;
		int lastnum= num;
		wgetevent(&e);
		num= -1;
		switch(e.type) {
		
		case WE_MENU:
			switch (e.u.m.id) {
			case MAINMENU:
				if (e.u.m.item == QUIT)
					return;
				do_mainmenu(e.u.m.item);
				break;
			case PRINTMENU:
				do_printmenu(filename, e.u.m.item);
				break;
			}
			break;
		
		case WE_CHAR:
			/* The mnemonics used here may remind you of
			   the main menu's shortcuts, the 'vi' editor,
			   the 'more' pages, or the 'rn' news reader... */
			switch (e.u.character) {
			case 'q':
			case 'Q':
				return;
			case ' ':
			case '+':
			case 'n':
			case 'N':
				forwpage(lastnum);
				break;
			case '-':
				if (lastnum > 0)
					backpage(lastnum);
				else
					gotopage(prevpage);
				break;
			case 'b':
			case 'B':
			case 'p':
			case 'P':
				backpage(lastnum);
				break;
			case '^':
			case 'f':
			case 'F':
				gotopage(1);
				break;
			case '$':
				gotopage(32000);
				break;
			case 'g':
			case 'G':
			case 'l':
			case 'L':
				if (lastnum > 0)
					gotopage(lastnum);
				else
					gotopage(32000);
				break;
			case '.':
				if (lastnum > 0)
					gotopage(lastnum);
				else
					changeall(); /* Force a redraw */
				break;
			default:
				if (isdigit(e.u.character)) {
					num= e.u.character - '0';
					if (lastnum > 0)
						num += 10*lastnum;
				}
				else {
					wfleep();
					lastnum= 0;
				}
			}
			break;
		
		case WE_CLOSE:
			return;

		case WE_COMMAND:
			switch (e.u.command) {
			case WC_RETURN:
				if (lastnum > 0)
					gotopage(lastnum);
				else
					forwpage(1);
				break;
			case WC_DOWN:
				forwpage(lastnum);
				break;
			case WC_UP:
			case WC_BACKSPACE:
				backpage(lastnum);
				break;
			case WC_CLOSE:
			/*
			case WC_CANCEL:
			*/
				return;
			default:
				wfleep();
			}
			break;
		
		}
	}
}

do_mainmenu(item)
	int item;
{
	switch (item) {
	case FIRSTPAGE:
		gotopage(1);
		break;
	case PREVPAGE:
		backpage(1);
		break;
	case NEXTPAGE:
		forwpage(1);
		break;
	case LASTPAGE:
		gotopage(32000);
		break;
	case GOTOPAGE:
		{
			static char buf[10];
			int num;
			char extra;
			if (!waskstr("Go to page:", buf, sizeof buf)
					|| buf[0] == '\0')
				return;
			if (sscanf(buf, " %d %c", &num, &extra) != 1 ||
				num <= 0) {
				wmessage("Invalid page number");
				return;
			}
			gotopage(num);
		}
		break;
	}
}

do_printmenu(filename, item)
	char *filename;
	int item;
{
#ifdef DO_PRINTMENU
	char buf[256];
	int sts;
	
	if (item < 0 || item >= nprint)
		return;
	
	if (!canprint(item)) {
		sprintf(buf, "Can't convert %s output to %s",
			devname == NULL ? "unspecified" : devname,
			printitems[item].device);
		wmessage(buf);
		return;
	}
	
	sprintf(buf, printitems[item].command, filename);
	sts= system(buf);
	if (sts != 0) {
		sprintf(buf, "Print command failed, exit status 0x%x", sts);
		wmessage(buf);
	}
#endif
}

#ifdef DO_PRINTMENU
static bool
canprint(item)
	int item;
{
	return printitems[item].device == NULL ||
		devname != NULL &&
			strcmp(printitems[item].device, devname) == 0;
}
#endif
