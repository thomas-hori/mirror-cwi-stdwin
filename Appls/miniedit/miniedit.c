/* Mini-editor using editwin module */

#include "stdwin.h"
#include "tools.h"
#include "editwin.h"

void menusetup(void);
void openfiles(int, char**);
void mainloop();

int
main(argc, argv)
	int argc;
	char **argv;
{
	winitargs(&argc, &argv);
	menusetup();
	openfiles(argc, argv);
	mainloop();
	wdone();
	exit(0);
}

MENU *filemenu, *editmenu, *findmenu;

#define FILE_MENU	1
#define EDIT_MENU	2
#define FIND_MENU	3

/* File items */
#define NEW_ITEM	0
#define OPEN_ITEM	1
#define CLOSE_ITEM	3
#define SAVE_ITEM	4
#define SAVE_AS_ITEM	5
#define SAVE_COPY_ITEM	6
#define REVERT_ITEM	7
#define QUIT_ITEM	9

/* Edit items */
#define UNDO_ITEM	0
#define CUT_ITEM	2
#define COPY_ITEM	3
#define PASTE_ITEM	4
#define CLEAR_ITEM	5
#define SEL_ALL_ITEM	6
#define EXEC_ITEM	8

/* Find items */
#define FIND_ITEM	0
#define FIND_SAME_ITEM	1
#define REPL_ITEM	3
#define REPL_SAME_ITEM	4
#define REPL_ALL_ITEM	5

void
menusetup()
{
	MENU *mp;
	
	filemenu= mp= wmenucreate(FILE_MENU, "File");
	
	wmenuadditem(mp, "New", 'N');
	wmenuadditem(mp, "Open...", 'O');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Close", 'K');
	wmenuadditem(mp, "Save", 'S');
	wmenuadditem(mp, "Save as...", -1);
	wmenuadditem(mp, "Save a Copy...", -1);
	wmenuadditem(mp, "Revert to Saved", -1);
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Quit", 'Q');
	
	editmenu= mp= wmenucreate(EDIT_MENU, "Edit");
	
	wmenuadditem(mp, "Undo", 'Z');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Cut", 'X');
	wmenuadditem(mp, "Copy", 'C');
	wmenuadditem(mp, "Paste", 'V');
	wmenuadditem(mp, "Clear", 'B');
	wmenuadditem(mp, "Select All", 'A');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Execute", '\r');
	
	findmenu= mp= wmenucreate(FIND_MENU, "Find");
	
	wmenuadditem(mp, "Find...", 'F');
	wmenuadditem(mp, "Find Same", 'G');
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "Replace...", 'R');
	wmenuadditem(mp, "Replace Same", 'T');
	wmenuadditem(mp, "Replace All", -1);
}

void
fixmenus(ew)
	EDITWIN *ew;
{
	bool focus;
	
	if (ew == NULL)
		return;
	
	wmenuenable(filemenu, SAVE_ITEM, !ew->saved);
	wmenuenable(filemenu, REVERT_ITEM, !ew->saved);
	
	wmenuenable(editmenu, UNDO_ITEM, FALSE);
	wmenuenable(editmenu, EXEC_ITEM, FALSE);
	
	focus= tegetfoc1(ew->tp) < tegetfoc2(ew->tp);
	wmenuenable(editmenu, CUT_ITEM, focus);
	wmenuenable(editmenu, COPY_ITEM, focus);
	wmenuenable(editmenu, CLEAR_ITEM, focus);
}

void
openfiles(argc, argv)
	int argc;
	char **argv;
{
	int i;
	
	for (i= 1; i < argc; ++i) {
		(void) ewcreate(argv[i]);
	}
	if (ewcount() == 0) {
		if (ewopen() == NULL && ewnew() == NULL) {
			wdone();
			fprintf(stderr, "Can't open any windows\n");
			exit(1);
		}
	}
}

void
mainloop()
{
	for (;;) {
		EVENT e;
		EDITWIN *ew;
		
		wgetevent(&e);
		ew= ewfind(e.window);
		switch (e.type) {
		case WE_MENU:
			switch (e.u.m.id) {
			
			case FILE_MENU:
				switch (e.u.m.item) {
				case NEW_ITEM:
					(void) ewnew();
					break;
				case OPEN_ITEM:
					(void) ewopen();
					break;
				case CLOSE_ITEM:
					if (ew != NULL) {
						ewclose(ew);
						if (ewcount() == 0)
							return;
					}
					break;
				case SAVE_ITEM:
					if (ew != NULL)
						ewsave(ew);
					break;
				case SAVE_AS_ITEM:
					if (ew != NULL)
						ewsaveas(ew);
					break;
				case SAVE_COPY_ITEM:
					if (ew != NULL)
						ewsavecopy(ew);
					break;
				case REVERT_ITEM:
					if (ew != NULL)
						ewrevert(ew);
					break;
				case QUIT_ITEM:
					if (ewcloseall())
						return;
					break;
				}
				break;
			
			case EDIT_MENU:
				if (ew == NULL) {
					wfleep();
					break;
				}
				switch (e.u.m.item) {
				case UNDO_ITEM:
					ewundo(ew);
					break;
				case CUT_ITEM:
					ewcopy(ew);
					ewreplace(ew, "");
					break;
				case COPY_ITEM:
					ewcopy(ew);
					break;
				case PASTE_ITEM:
					ewpaste(ew);
					break;
				case CLEAR_ITEM:
					ewreplace(ew, "");
					break;
				case SEL_ALL_ITEM:
					tesetfocus(ew->tp, 0,
						tegetlen(ew->tp));
					break;
				case EXEC_ITEM:
					wfleep();
					break;
				}
				break;
			
			case FIND_MENU:
				switch (e.u.m.item) {
				case FIND_ITEM:
					find(ew, TRUE);
					break;
				case FIND_SAME_ITEM:
					find(ew, FALSE);
					break;
				case REPL_ITEM:
					replace(ew, TRUE);
					break;
				case REPL_SAME_ITEM:
					replace(ew, FALSE);
					break;
				case REPL_ALL_ITEM:
					replall(ew);
					break;
				}
				break;
			
			}
			break;
		
		default:
			if (ew != NULL) {
				bool closed;
				wsetwincursor(ew->win, wfetchcursor("watch"));
				ewevent(ew, &e, &closed);
				if (!closed)
					wsetwincursor(ew->win,
						   wfetchcursor("ibeam"));
				if (closed && ewcount() == 0)
					return;
			}
			break;
		
		}
		
		fixmenus(ew);
	}
}

#include "regexp.h"

char whatbuf[256];	/* Last regular expression typed */
regexp *prog;		/* Compiled regular expression */
char replbuf[256];	/* Last replacement string */

bool
find(ew, dodialog)
	EDITWIN *ew;
	bool dodialog;
{
	return finddialog(ew, dodialog, "Find regular expression:") &&
		findit(ew, FALSE);
}

bool
replace(ew, dodialog)
	EDITWIN *ew;
	bool dodialog;
{
	if (!finddialog(ew, dodialog, "Replace regular expression:")
		|| !findit(ew, TRUE))
		return FALSE;
	wupdate(ew->win); /* Show what we've found */
	return repldialog(ew, dodialog) && replit(ew);
}

bool
replall(ew)
	EDITWIN *ew;
{
	if (!replace(ew, TRUE))
		return FALSE;
	while (findit(ew, TRUE)) {
		wupdate(ew->win);
		replit(ew);
		wupdate(ew->win);
		/* What if it takes forever? */
	}
	return TRUE;
}

bool
finddialog(ew, dodialog, prompt)
	EDITWIN *ew;
	bool dodialog;
	char *prompt;
{
	if (ew == NULL)
		return FALSE;
	if (dodialog || prog == NULL) {
		if (!waskstr(prompt, whatbuf, sizeof whatbuf))
			return FALSE;
		if (prog != NULL)
			free((char*)prog);
		prog= regcomp(whatbuf);
		if (prog == NULL)
			return FALSE;
	}
	return TRUE;
}

bool
repldialog(ew, dodialog)
	EDITWIN *ew;
	bool dodialog;
{
	if (!dodialog)
		return TRUE;
	return waskstr("Replacement string:", replbuf, sizeof replbuf);
}

bool
findit(ew, begin)
	EDITWIN *ew;
	bool begin; /* TRUE if must start at foc1 */
{
	int foc1= tegetfoc1(ew->tp);
	int foc2= tegetfoc2(ew->tp);
	int len= tegetlen(ew->tp);
	char *text= tegettext(ew->tp);
	
	if (!reglexec(prog, text, begin ? foc1 : foc2)) {
		wfleep();
		return FALSE;
	}
	if (!begin && prog->startp[0] == text+foc2 && foc1 == foc2 &&
		prog->startp[0] == prog->endp[0]) {
		/* Found empty string at empty focus;
		   try again at next position (if in range) */
		if (++foc2 > len)
			return FALSE;
		if (!reglexec(prog, text, foc2)) {
			wfleep();
			return FALSE;
		}
	}
	tesetfocus(ew->tp, (int)(prog->startp[0] - text),
		(int)(prog->endp[0] - text));
	return TRUE;
}

bool
replit(ew)
	EDITWIN *ew;
{
	char substbuf[1024];
	/* Should be bigger -- what if the entire file matches? */
	
	regsub(prog, replbuf, substbuf);
	ewreplace(ew, substbuf);
	return TRUE;
}

void
regerror(str)
	char *str;
{
	char buf[256];
	
	sprintf(buf, "Regular expression error: %s", str);
	wmessage(buf);
}
