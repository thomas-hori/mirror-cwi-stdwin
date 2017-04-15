/* Main program for VT package */
/* /ufs/guido/CVSROOT/stdwin/Appls/vtest/vtest.c,v 1.1 1990/10/12 15:46:58 guido Exp */

#define DO_RESIZE

#include "vtimpl.h"
#include "vtserial.h"

main ARGS((int argc, char **argv));
STATIC void eventloop ARGS((VT *vt));
STATIC void sendarrow ARGS((VT *vt, char key));
STATIC void addmenus ARGS((WINDOW *win));
STATIC bool domenu ARGS((VT *vt, int id, int item));
STATIC void stripparity ARGS((char *buf, int len));

main(argc, argv)
	int argc;
	char **argv;
{
	VT *vt;
	
	winitnew(&argc, &argv);
	if (!openserial()) {
		wmessage("Can't open serial line");
		wdone();
		return 1;
	}
#ifdef macintosh
	wsetfont("Monaco");
	wsetsize(9);
#endif
	wsetdefwinsize(80*wcharwidth('0'), 24*wlineheight());
	vt= vtopen("VT", 24, 80, 200);
	if (vt != NULL) {
		addmenus(vtwindow(vt));
#ifdef DO_RESIZE
		if (!vtautosize(vt)) {
			vtclose(vt);
			wmessage("Out of memory!");
		}
		else
#endif
			eventloop(vt);
	}
	wdone();
	return 0;
}

static void
eventloop(vt)
	VT *vt;
{
	for (;;) {
		EVENT e;
		char buf[2048];
		int len;
		
		wgetevent(&e);
		switch (e.type) {
		
		case WE_MENU:
			if (!domenu(vt, e.u.m.id, e.u.m.item))
				return;
			break;
		
		case WE_SERIAL_AVAIL:
			len= receiveserial(buf, (sizeof buf) - 1);
			if (len > 0) {
				stripparity(buf, len);
				vtansiputs(vt, buf, len);
			}
			break;
		
#ifdef DO_RESIZE
		case WE_SIZE:
			if (!vtautosize(vt)) {
				vtclose(vt);
				wmessage("Out of memory!");
				return;
			}
			break;
#endif
		
		case WE_CHAR:
			if (e.u.character == '`')
				e.u.character= ESC;
			buf[0]= e.u.character;
			vtsend(vt, buf, 1);
			break;
		
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			vtsetcursor(vt,
				e.u.where.v / vtcheight(vt),
				e.u.where.h / vtcwidth(vt));
			break;
		
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
				return;
			case WC_RETURN:
				vtsend(vt, "\r", 1);
				break;
			case WC_BACKSPACE:
				vtsend(vt, "\b", 1);
				break;
			case WC_TAB:
				vtsend(vt, "\t", 1);
				break;
			case WC_CANCEL:
				vtsend(vt, "\003", 1);
				break;
			case WC_LEFT:
				sendarrow(vt, 'D');
				break;
			case WC_RIGHT:
				sendarrow(vt, 'C');
				break;
			case WC_UP:
				sendarrow(vt, 'A');
				break;
			case WC_DOWN:
				sendarrow(vt, 'B');
				break;
			}
			break;
		
		}
	}
}

static void
sendarrow(vt, key)
	VT *vt;
	char key;
{
	char buf[3];
	buf[0]= ESC;
	buf[1]= vt->keypadmode ? 'O' : '[';
	buf[2]= key;
	vtsend(vt, buf, 3);
}

MENU *speedmenu;

static int speedlist[]=
	{300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200};

#define NRATES (sizeof speedlist / sizeof speedlist[0])

static void
addmenus(win)
	WINDOW *win;
{
	MENU *mp;
	int i;
	
	wmenusetdeflocal(TRUE);
	
	mp= wmenucreate(1, "VT");
	wmenuadditem(mp, "DEL", '\b');
	wmenuadditem(mp, "Backquote", '`');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Send Break", '/');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Receive file...", -1);
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Quit", -1);
	wmenuattach(win, mp);
	
	mp= wmenucreate(2, "CC");
	wmenuadditem(mp, "Control-@", ' '); /* 0 */
	for (i= 'A'; i <= ']'; ++i) {
		char buf[256];
		sprintf(buf, "Control-%c", i);
		wmenuadditem(mp, buf, i); /* 1...29 */
	}
	wmenuadditem(mp, "Control-^", '6'); /* 30 */
	wmenuadditem(mp, "Control-_", '-'); /* 31 */
	wmenuattach(win, mp);
	
	speedmenu= mp= wmenucreate(3, "Speed");
	for (i= 0; i < NRATES; ++i) {
		char buf[256];
		sprintf(buf, "%d", speedlist[i]);
		wmenuadditem(mp, buf, -1);
	}
	wmenuattach(win, mp);
}

static bool
domenu(vt, id, item)
	VT *vt;
	int id, item;
{
	char cc[2];
	int n= 0;
	
	switch (id) {
	
	case 1:
		switch (item) {
		case 0:
			cc[0]= '\177';
			n= 1;
			break;
		case 1:
			cc[0]= '`';
			n= 1;
			break;
		case 3:
			breakserial();
			break;
		case 5:
			receivefile();
			break;
		case 7:
			return FALSE;
		}
		break;
	
	case 2:
		cc[0]= item;
		n= 1;
		break;
	
	case 3:
		if (item < NRATES) {
			int i;
			if (!speedserial(speedlist[item]))
				item= -1;
			for (i= 0; i < NRATES; ++i)
				wmenucheck(speedmenu, i, i==item);
		}
		break;
	
	}
	if (n > 0)
		vtsend(vt, cc, n);
	return TRUE;
}

/* Strip parity bits */

static void
stripparity(buf, len)
	register char *buf;
	int len;
{
	register char *end= buf+len;
	for (; buf < end; ++buf) {
		*buf &= 0x7f;
	}
}
