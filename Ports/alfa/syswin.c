/* STDWIN -- SYSTEM WINDOW. */

#include "alfa.h"

WINDOW *syswin;		/* Window id 0, the system window */
			/* Global because wgetevent needs to know about
			   it, so it can suppress events belonging to
			   this window. */

static void
helpmessage()
{
	char buf[256];
	char shortcut[256];
	
	getbindings(shortcut, 0, MENU_CALL);
	sprintf(buf, "[Use  %s  to get a menu of commands]", shortcut);
	wmessage(buf);
}

void
initsyswin()
{
	syswin= wopen("System", wsysdraw);
	helpmessage();
}

char *sysmsg;		/* Message to be drawn at (0, 0) */
TEXTEDIT *syste;	/* Textedit record to be drawn, too */

#ifdef EM
static char **Butt=NULL;
#endif

/*ARGSUSED*/
void
wsysdraw(win, left, top, right, bottom)
	WINDOW *win;
	int left, top;
	int right, bottom;
{
	if (sysmsg != NULL) {
		(void) wdrawtext(0, 0, sysmsg, -1);
		if (syste != NULL)
			tedraw(syste);
#ifdef EM 
		if(Butt)
			drawbuttons();	
#endif
	}
	else
		drawmenubar();
}

void
menubarchanged()
{
	uptodate[0]= FALSE;
}

/* Print a message in the system window.
   If the message is non-null, the screen is updated immediately. */

void
wmessage(str)
	char *str;
{
	if (sysmsg != NULL)
		free(sysmsg);
	sysmsg= strdup(str);
	if (syste != NULL) {
		tefree(syste);
		syste= NULL;
	}
	wchange(syswin, 0, 0, 9999, 9999);
	wnocaret(syswin);
	if (str != NULL) {
		wupdate(syswin);
		wflush();
	}
}

/* Ask for an input string. */

bool
waskstr(prompt, buf, len)
	char *prompt;
	char *buf;
	int len;
{
	WINDOW *savewin= front;
	WINDOW *win= syswin;
	bool ok= FALSE;
	bool stop= FALSE;
	int teleft;
	
	wsetactive(win);
	wmessage((char *) NULL);
	sysmsg= prompt;
	teleft= wtextwidth(prompt, -1) + wtextwidth(" ", 1);
	if (teleft > columns * 3/4)
		teleft= columns * 3/4;
	syste= tealloc(syswin, teleft, 0, columns-teleft);
	tereplace(syste, buf);
	tesetfocus(syste, 0, 9999);
	do {
		EVENT e;
		
		if (!wsysevent(&e, FALSE)) {
			wupdate(syswin);
			wflush();
			wsysevent(&e, TRUE);
		}
		e.window= syswin; /* Not filled in by wsys*event();
				     needed by teevent(). */
			
		switch (e.type) {
		
		case WE_MENU:
			if (e.u.m.id != 0) {
				wfleep();
				break;
			}
			switch (e.u.m.item) {
			
			case SUSPEND_PROC:
				_wsuspend();
				break;
			
			case REDRAW_SCREEN:
				_wredraw();
				break;
			
			default:
				if (e.u.m.item >= FIRST_CMD &&
					e.u.m.item <= LAST_CMD)
					wsyscommand(&e);
				break;
			}
			if (e.type != WE_COMMAND)
				break;
		
		/* Fall through from previous case! */
		case WE_COMMAND:
			switch (e.u.command) {
			
			case WC_RETURN:
			case WC_CANCEL:
				ok= e.u.command == WC_RETURN;
				stop= TRUE;
				break;
			
			default:
				if (!teevent(syste, &e))
					wfleep();
				break;
			
			}
			break;
		
		case WE_CHAR:
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			if (!teevent(syste, &e))
				wfleep();
			break;
		
		}
	} while (!stop);
	if (ok) {
		strncpy(buf, tegettext(syste), len);
		buf[len-1]= EOS;
	}
	sysmsg= NULL;
	wmessage((char *) NULL);
	wsetactive(savewin);
	return ok;
}

#ifdef EM
/* EuroMath hacks -- I still hop I can get rid of this again... */
#define META(c) ((c)|128)
#define UNMETA(c) ((c)&~128)
#define min(a,b) (a)<(b)?(a):(b)
char *hack;
int Curr;

wdialog(prompt, buf, len, butt, def)
char *prompt, *buf, **butt;
int len, def;
{
	WINDOW *savewin= front;
	WINDOW *win= syswin;
	bool ok= FALSE;
	bool stop= FALSE;
	int teleft;
	int y=1, i;	
	extern char* esc;
	extern char *expandCommand();

	if(butt==NULL)
		return waskstr(prompt, buf, len);

	Butt=butt;
	Curr=0;
/* highlight the default button */
	for(i=0;butt[def-1][i];i++) butt[def-1][i]=META(butt[def-1][i]);
	wsetactive(win);
	wmessage((char *) NULL);
	sysmsg= prompt;
	teleft= wtextwidth(prompt, -1) + wtextwidth(" ", 1);
	if (teleft > columns * 3/4)
		teleft= columns * 3/4;
	syste= tealloc(syswin, teleft, 0, columns-teleft);
	tereplace(syste, strdup(buf));
	tesetfocus(syste, 0, 9999);
/* calculate the number of buttons */
	while(butt[y-1]) {
		++y;
	}
	--y;
restart:
	do {
		EVENT e;
		
		if (!wsysevent(&e, FALSE)) {
			wupdate(syswin);
			wflush();
			wsysevent(&e, TRUE);
		}
		e.window= syswin; /* Not filled in by wsys*event();
				     needed by teevent(). */
			
		switch (e.type) {
		case WE_MENU:	
			if (e.u.m.id != 0) {
				wfleep();
				break;
			}
			switch (e.u.m.item) {
			
			case SUSPEND_PROC:
				_wsuspend();
				break;
			
			case REDRAW_SCREEN:
				_wredraw();
				break;
			
			default:
				if (e.u.m.item >= FIRST_CMD &&
					e.u.m.item <= LAST_CMD)
					wsyscommand(&e);
				break;
			}
			if (e.type != WE_COMMAND)
				break;
		
		/* Fall through from previous case! */
		case WE_COMMAND:
			switch (e.u.command) {
/* the arrow-keys (up & down) are used to escape from the texedit
	buffer and select a button from the buttonlist */
			case WC_UP:
				--Curr;
				if(Curr<0)
					Curr=0;
				break;
			case WC_DOWN:
				++Curr;
				if(Curr>y)
					Curr=0;	
				break;
			case WC_RETURN:
			case WC_CANCEL:
				ok= e.u.command == WC_RETURN;
				stop= TRUE;
				break;
			
			default:
				if (Curr > 0)
					wfleep();
				else if (!teevent(syste, &e))
					wfleep();
				break;
			
			}
			break;
		
		case WE_CHAR:
			if(hack && e.u.character == *esc) {
/* this is used by cmdbox(): pressing the `escape' character causes
the typed in buffer to be expanded */
				char *buf2;
				tesetfocus(syste,0,9999);
				strzcpy(buf,tegettext(syste),min(len,tegetlen(syste)+1));
				buf2=expandCommand(hack, buf);
				tereplace(syste, buf2);
				tesetfocus(syste,0,9999);
				break;
			}
		case WE_MOUSE_DOWN:
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			if(Curr > 0)
				wfleep();
			else if (!teevent(syste, &e))
				wfleep();
			break;
		
		}
	} while (!stop);
	if(hack && Curr == 3) {
/* button 3 is "EXPAND".  Used by cmdbox() */
		char *buf2;
		tesetfocus(syste,0,9999);
		strzcpy(buf,tegettext(syste),min(len,tegetlen(syste)+1));
		buf2=expandCommand(hack, tegettext(syste));
		tereplace(syste, buf2);
		tesetfocus(syste,0,9999);
		ok=stop=FALSE; Curr=0;
		goto restart;
	}	
	if (ok) {
		strzcpy(buf, tegettext(syste), min(len, tegetlen(syste)+1));
	}
	sysmsg= NULL;
	wmessage((char *) NULL);
	wsetactive(savewin);
	if(Curr==0) Curr=def;
	Butt=NULL;
	for(i=0;butt[def-1][i];i++) butt[def-1][i]=UNMETA(butt[def-1][i]);
	_wredraw();
	return Curr;
}

/* this is inserted in the syswin drawproc whenever there are buttons
active */
 
drawbuttons() 
{
int y=1;
char buf[256];

	while(Butt[y-1]) {
		if(y==Curr) sprintf(buf, "%c %s", META('o'), Butt[y-1]); 
		else sprintf(buf, "o %s", Butt[y-1]);
		trmputdata(y, y, 0, buf, (char *)0);
		++y;
	}
	trmputdata(y,y,0,"",(char *)0);
}

/* additional EuroMath WM routine, called after returning from an
Editor-session (OpenTTY) */

RedrawAll()
{
_wredraw();
}

#endif
