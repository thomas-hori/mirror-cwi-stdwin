/* TERMCAP STDWIN -- KEY BINDING. */

/* Separated from the default tables; the application may override
   the tables but not the executable code. */

#include "alfa.h"

#ifndef MSDOS
#define HAVE_TERMCAP
#endif

#ifdef HAVE_TERMIO_H
#undef HAVE_SGTTY_H
#endif

#ifdef HAVE_SGTTY_H
#include <sgtty.h>
#endif
#ifdef HAVE_TERMIO_H
#include <termio.h>
#endif

#ifdef HAVE_TERMCAP
/* Termcap functions: */
char *tgetstr();
int tgetnum();
bool tgetflag();
#endif /* HAVE_TERMCAP */


/* Forward */
static int createmap();
static void copydefaults();
static void charbinding();
static void tcbinding();
static void setbinding();

/* Get key definitions from tty settings. */

void
getttykeydefs(fd)
	int fd;
{
#ifdef HAVE_SGTTY_H
	struct sgttyb gttybuf;
	struct tchars tcharbuf;
#endif
#ifdef HAVE_TERMIO_H
	struct termio tio;
#endif

	copydefaults();
	
#ifdef HAVE_SGTTY_H
	gtty(fd, &gttybuf);
	charbinding(SHORTCUT, 0, FIRST_CMD+WC_BACKSPACE,(int)gttybuf.sg_erase);
	
	ioctl(fd, TIOCGETC, (char *) &tcharbuf);
	charbinding(SHORTCUT, 0, FIRST_CMD+WC_CANCEL, (int)tcharbuf.t_intrc);
#endif
#ifdef HAVE_TERMIO_H
	ioctl(fd, TCGETA, &tio);
	charbinding(SHORTCUT, 0, FIRST_CMD+WC_BACKSPACE, (int)tio.c_cc[2]);
	charbinding(SHORTCUT, 0, FIRST_CMD+WC_CANCEL, (int)tio.c_cc[0]);
#endif

#ifndef HAVE_TERMIO_H
#ifdef TIOCGLTC
	{
		struct ltchars ltcharbuf;
		
		ioctl(fd, TIOCGLTC, (char *) &ltcharbuf);
		charbinding(SHORTCUT, 0, SUSPEND_PROC, ltcharbuf.t_suspc);
		charbinding(SHORTCUT, 0, REDRAW_SCREEN, ltcharbuf.t_rprntc);
		charbinding(SHORTCUT, 0, LITERAL_NEXT, ltcharbuf.t_lnextc);
	}
#endif /* TIOCGLTC */
#endif /* HAVE_TERMIO_H */
}

static void
charbinding(type, id, item, key)
	int type;
	int id, item;
	int key;
{
	if (key != 0 && (key&0xff) != 0xff) {
		char keys[2];
		keys[0]= key;
		keys[1]= EOS;
		setbinding(&_wprimap[*keys & 0xff], type, id, item, keys);
	}
}

/* Get key definitions from termcap. */

void
gettckeydefs()
{
#ifdef HAVE_TERMCAP
	copydefaults();
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_BACKSPACE,	"kb");
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_LEFT,	"kl");
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_RIGHT,	"kr");
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_UP,		"ku");
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_DOWN,	"kd");
	/*
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_CLEAR,	"kC");
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_HOME,	"kh");
	tcbinding(SHORTCUT, 0, FIRST_CMD+WC_HOME_DOWN,	"kH");
	*/
#endif /* HAVE_TERMCAP */
}

#ifdef HAVE_TERMCAP
static void
tcbinding(type, id, item, capname)
	int type;
	int id, item;
	char capname[2];	/* Termcap capability name, e.g. "k1" */
{
	char buf[100];
	char *p= buf;
	char *keys;
	
	keys= tgetstr(capname, &p);
	if (keys != NULL)
		setbinding(&_wprimap[*keys & 0xff], type, id, item, keys);
}
#endif /* HAVE_TERMCAP */

/* Bind a menu item to a meta-key.
   As there are no meta-keys on standard Unix,
   this is translated to ESC-key. */

void
wsetmetakey(id, item, key)
	int id, item;
	int key;
{
	char buf[3];
	
	buf[0]= '\033'; /* ESC */
	buf[1]= key;
	buf[2]= EOS;
	wsetshortcut(id, item, buf);
}

/* Bind a menu item to a key sequence.
   Note that this call is not part of the universal STDWIN interface,
   only of the Unix interface for ASCII terminals. */

void
wsetshortcut(id, item, keys)
	int id, item;
	char *keys;
{
	if (keys == NULL || *keys == EOS)
		return; /* Can't bind empty string */
	copydefaults();
	setbinding(&_wprimap[*keys & 0xff], SHORTCUT, id, item, keys);
}

static struct keymap *extendmap();

static int
mapsize(map)
	struct keymap *map;
{
	int size;
	
	if (map == NULL)
		return 0;
	for (size= 0; map[size].type != SENTINEL; ++size)
		;
	return size+1;
}

static void
setbinding(map, type, id, item, keys)
	struct keymap *map;
	int type;
	int id, item;
	char *keys;
{
	if (keys[1] == EOS) {
		map->key= *keys;
		map->type= type;
		map->id= id;
		map->item= item;
	}
	else {
		struct keymap *nmap;
		if (map->type != SECONDARY) {
			map->type= SECONDARY;
			map->id= createmap();
		}
		for (nmap= _wsecmap[map->id];
			nmap->type != SENTINEL; ++nmap) {
			if (nmap->key == keys[1])
				break;
		}
		if (nmap->type == SENTINEL)
			nmap= extendmap((int) map->id, (int) keys[1]);
		if (nmap != NULL)
			setbinding(nmap, type, id, item, keys+1);
	}
}

static struct keymap *
extendmap(imap, c)
	int imap;
	int c;
{
	L_DECLARE(size, map, struct keymap);
	
	if (imap == 0 || imap >= SECMAPSIZE || (map= _wsecmap[imap]) == NULL)
		return NULL;
	size= mapsize(map);
	L_EXTEND(size, map, struct keymap, 1);
	_wsecmap[imap]= map;
	if (map == NULL)
		return NULL;
	map += size - 2;
	map->type= ORDINARY;
	map->key= c;
	map[1].type= SENTINEL;
	return map;
}

static int
createmap()
{
	L_DECLARE(size, map, struct keymap);
	int i;
	
	L_EXTEND(size, map, struct keymap, 1);
	if (map == NULL)
		return 0;
	map->type= SENTINEL;
	for (i= 0; i < SECMAPSIZE && _wsecmap[i] != NULL; ++i)
		;
	if (i >= SECMAPSIZE) { /* Overflow of _wsecmap array */
		L_DEALLOC(size, map);
		return 0;
	}
	else {
		_wsecmap[i]= map;
		return i;
	}
}

/* Copy existing secondary key maps to dynamic memory.
   Note: don't copy map 0, which is a dummy to force a dead-end street. */

static void
copydefaults()
{
	static bool been_here= FALSE;
	int i;
	struct keymap *map;
	
	if (been_here)
		return;
	been_here= TRUE;
	
	for (i= 1; i < SECMAPSIZE; ++i) {
		map= _wsecmap[i];
		if (map != NULL) {
			int size= mapsize(map);
			struct keymap *nmap;
			int k;
			nmap= (struct keymap *) malloc(
				(unsigned) (size * sizeof(struct keymap)));
			if (nmap != NULL) {
				for (k= 0; k < size; ++k)
					nmap[k]= map[k];
			}
			_wsecmap[i]= nmap;
		}
	}
}

/* Routines to get a nice description of a menu item's shortcuts.
   TO DO: protect against buffer overflow; cache output so it
   isn't called so often (i.e., twice for each drawitem call!). */

static char *
charrepr(c)
	int c;
{
	static char repr[10];

	switch (c) {

	case 033:
		return "ESC";
	
	case '\r':
		return "CR";
	
	case '\b':
		return "BS";
	
	case '\t':
		return "TAB";
	
	case 0177:
		return "DEL";

	default:
		if (c < ' ') {
			repr[0]= '^';
			repr[1]= c|'@';
			repr[2]= '\0';
		}
		else if (c < 0177) {
			repr[0]= c;
			repr[1]= '\0';
		}
		else {
			repr[0]= '\\';
			repr[1]= '0' + (c>>6);
			repr[2]= '0' + ((c>>3) & 07);
			repr[3]= '0' + (c & 07);
			repr[4]= '\0';
		}
		return repr;

	}
}

static char *
addrepr(cp, c)
	char *cp;
	int c;
{
	char *rp= charrepr(c);
	
	while (*rp != '\0')
		*cp++ = *rp++;
	return cp;
}

static char *
followmap(cp, map, id, item, stack, level)
	char *cp;
	struct keymap *map;
	int id, item;
	unsigned char *stack;
	int level;
{
	if (map->type == SHORTCUT) {
		if (map->id == id && map->item == item) {
			int i;
			for (i= 0; i < level; ++i) {
				cp= addrepr(cp, (int) stack[i]);
				*cp++ = '-';
			}
			cp= addrepr(cp, (int) map->key);
			*cp++ = ',';
			*cp++ = ' ';
		}
	}
	else if (map->type == SECONDARY) {
		stack[level]= map->key;
		map= _wsecmap[map->id];
		for (; map->type != SENTINEL; ++map)
			cp= followmap(cp, map, id, item, stack, level+1);
	}
	return cp;
}

void
getbindings(buf, id, item)
	char *buf;
	int id;
	int item;
{
	char *cp= buf;
	unsigned char stack[50];
	struct keymap *map;
	
	for (map= _wprimap; map < &_wprimap[256]; ++map)
		cp= followmap(cp, map, id, item, stack, 0);
	if (cp > buf)
		cp -= 2;
	*cp= EOS;
}
