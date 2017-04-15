/* MAC STDWIN -- DIALOGS. */

/* XXX These dialogs are distinctly ugly.
   Maybe we should fix their size based on the amount of text
   and the number of buttons. */

#include "macwin.h"
#include <Dialogs.h>
#include <Packages.h>
#include <Memory.h>
#include <StandardFile.h>

#ifndef HAVE_UNIVERSAL_HEADERS
#define DlgHookUPP ProcPtr
#define FileFilterUPP ProcPtr
#define ModalFilterUPP ProcPtr
#define NewModalFilterProc(x) ((ProcPtr)x)
#endif

/* Function prototypes */

STATIC struct itemlist **mkitemlist _ARGS((void));
STATIC struct item *finditem _ARGS((struct itemlist **h, int i));
STATIC void additem _ARGS((struct itemlist **h,
	long stuff, Rect *ppos, int type, int size, char *data));
STATIC struct itemlist **ynclist _ARGS((char *prompt));
STATIC struct itemlist **oklist _ARGS((char *prompt));
STATIC struct itemlist **editlist _ARGS((char *prompt));
STATIC int do_dialog _ARGS((struct itemlist **h,
	int emphasis, int lastbutton, char *buf));

/* Mac-specific interface for applications that need different
   file types */
OSType std_type= 'TEXT';

OSType *wasktypelist= &std_type;
int waskntypes= 1;

/* Standard File interface routine */

bool
waskfile(prompt, buf, len, new)
	char *prompt;
	char *buf;
	int len;
	bool new;
{
	static Point corner= {80, 60};
	SFReply reply;
	
	if (active != NULL)
		rmcaret(active);
	if (new) {
		char *def= strrchr(buf, ':');
		if (def != NULL)
			++def;
		else
			def= buf;
		SFPutFile(PASSPOINT corner,
			PSTRING(prompt),
#ifdef CLEVERGLUE
			PSTRING(def),
#else
			/* OK to overwrite contents of buf since it is
			   output anyway */
			CtoPstr(def),
#endif
			(DlgHookUPP)NULL, &reply);
	}
	else {
		SFGetFile(PASSPOINT corner,
			  (ConstStr255Param)NULL,
			  (FileFilterUPP)NULL,
			  waskntypes,
			  wasktypelist,
			  (DlgHookUPP)NULL,
			  &reply);
	}
	set_watch();
	if (!reply.good)
		return FALSE;
	fullpath(buf, reply.vRefNum, PtoCstr(reply.fName));
	return TRUE;
}

/* Data definitions for dialog item lists (from Inside Mac). */
/* XXXX Jack wonders why these aren't exported from the headers?? */
#if GENERATINGPOWERPC
#pragma options align=mac68k
#endif

struct item {
	long stuff;		/* Handle or proc pointer */
	Rect pos;		/* Position (local coord.) */
	char type;		/* Item type */
	char size;		/* Length of data; must be even */
	char data[256]; 	/* The data; variable length */
};

struct itemlist {
	short count;		/* Number of items minus one */
	struct item data;	/* First item */
	/* NB: items are variable length. */
};

#if GENERATINGPOWERPC
#pragma options align=reset
#endif

#define ROUND_EVEN(x) (((x) + 1) & ~1) /* Round up to even */

#define FIXED_SIZE 14		/* Size of struct item w/o data */

/* Routines to manipulate Dialog item lists. */

/* Create an empty item list. */

static struct itemlist **
mkitemlist()
{
	struct itemlist **h= (struct itemlist **) NewHandle(2);
	
	(*h)->count= -1;
	return h;
}

/* Find the i'th item, starting to count from 0.
   It may be asked for the non-existing item just beyond the last,
   but not beyond that. */

static struct item *
finditem(h, i)
	struct itemlist **h;
	int i;
{
	int count= (*h)->count;
	struct item *it= &(*h)->data;
	int k;
	
	if (i < 0 || i > count+1) {
		return NULL;
	}
	for (k= 0; k < i; ++k) {
		/* I don't trust two casts in one expression: */
		char *p= (char *) it;
		int size= ROUND_EVEN(it->size);
		p += FIXED_SIZE + size;
		it= (struct item *) p;
	}
	return it;
}

/* Add an item to the list. */

static void
additem(h, stuff, ppos, type, size, data)
	struct itemlist **h;
	long stuff;
	Rect *ppos;
	int type;
	int size;
	char *data;
{
	struct item *it;
	long totalsize;
	
	if (size < 0)
		size= strlen(data);
	it= finditem(h, (*h)->count + 1);
	totalsize= (char *)it - (char *)(*h);
	SetHandleSize((Handle)h, totalsize + FIXED_SIZE + ROUND_EVEN(size));
	it= finditem(h, (*h)->count + 1);
	it->stuff= stuff;
	it->pos= *ppos;
	it->type= type;
	it->size= size;
	BlockMove(data, it->data, size);
	++(*h)->count;
}

/* Construct item list for question w/ Yes/No/Cancel response.
   Note: the statText item is first, so we can distinguish between a
   press on Return or Enter (when ModalDialog returns 1) and any
   of the three buttons. */

static struct itemlist **
ynclist(prompt)
	char *prompt;
{
	struct itemlist **h= mkitemlist();
	Rect pos;
	
	SetRect(&pos, 20, 20, 280, 70);
	additem(h, 0L, &pos, statText|itemDisable, -1, prompt);
	SetRect(&pos, 20, 80, 80, 100);
	additem(h, 0L, &pos, ctrlItem|btnCtrl, -1, "Yes");
	OffsetRect(&pos, 0, 30);
	additem(h, 0L, &pos, ctrlItem|btnCtrl, -1, "No");
	OffsetRect(&pos, 200, 0);
	additem(h, 0L, &pos, ctrlItem|btnCtrl, -1, "Cancel");
	return h;
}

/* Construct item list for message w/ OK button. */

static struct itemlist **
oklist(prompt)
	char *prompt;
{
	struct itemlist **h= mkitemlist();
	Rect pos;
	
	SetRect(&pos, 20, 20, 280, 100);
	additem(h, 0L, &pos, statText|itemDisable, -1, prompt);
	SetRect(&pos, 20, 110, 80, 130);
	additem(h, 0L, &pos, ctrlItem|btnCtrl, -1, "OK");
	return h;
}

/* Construct item list for dialog w/ edit-text, OK and Cancel button. */

static struct itemlist **
editlist(prompt)
	char *prompt;
{
	struct itemlist **h= mkitemlist();
	Rect pos;
	
	SetRect(&pos, 20, 20, 280, 70);
	additem(h, 0L, &pos, statText|itemDisable, -1, prompt);
	SetRect(&pos, 20, 110, 80, 130);
	additem(h, 0L, &pos, ctrlItem|btnCtrl, -1, "OK");
	OffsetRect(&pos, 200, 0);
	additem(h, 0L, &pos, ctrlItem|btnCtrl, -1, "Cancel");
	SetRect(&pos, 20, 80, 280, 96);
	additem(h, 0L, &pos, editText, 0, (char *) NULL);
	return h;
}

/* Filter procedure for ModalDialog, turns Return into hit of item #1.
   XXX This used to be the default -- what happened?
*/

static pascal Boolean filter(d, e, hit)
	DialogPtr d;
	EventRecord *e;
	short *hit;
{
	if (e->what == keyDown) {
		char c = e->message & charCodeMask;
		if (c == '\r' || c == ENTER_KEY) {
			*hit = 1;
			return 1;
		}
	}
	return 0;
}

/* Perform an entire dialog.
   It stops when an item <= lastbutton is hit, and returns the item number.
   When buf is non-NULL, the next item is assumed to be an edit-text
   item and its contents are transferred to buf. */

static int
do_dialog(h, emphasis, lastbutton, buf)
	struct itemlist **h;
	int emphasis;
	int lastbutton;
	char *buf;
{
	Rect box;
	DialogPtr d;
	short hit;
	short type;
	Handle item;
	ModalFilterUPP filterupp = NewModalFilterProc(filter);
	
	_wresetmouse(); /* Clean up mouse down status */
	if (active != NULL)
		rmcaret(active);
	
	/* Create a box of convenient size, centered horizontally,
	   somewhat below the top of the screen. */
	SetRect(&box, 0, 0, 300, 140);
	OffsetRect(&box, (screen->portRect.right - box.right)/2, 60);
	
	d= NewDialog(
		(Ptr)NULL,
		&box,
		(ConstStr255Param)"",
		true,
		dBoxProc,
		(WindowPtr)(-1),
		false,
		0L,
		(Handle)h);
	
	if (emphasis > 0) { /* Emphasize default button */
		GetDItem(d, emphasis, &type, &item, &box);
		SetPort(d);
		InsetRect(&box, -4, -4);
		PenSize(3, 3);
		FrameRoundRect(&box, 16, 16);
	}
	
	if (buf != NULL) { /* Set edit text and focus on entire text */
		GetDItem(d, lastbutton+1, &type, &item, &box);
		SetIText(item, PSTRING(buf));
		SelIText(d, lastbutton+1, 0, 32000);
	}
	
	set_arrow();
	
	/* XXX Should support Cmd-period as shortcut for Cancel;
	   perhaps other shortcuts as well? */
	
	do {
		ModalDialog(filterupp, &hit);
	} while (hit > lastbutton);
	
	set_watch();
	
	if (hit == 1 && emphasis > 0) {
		/* Pressed Return or Enter; flash default button. */
		GetDItem(d, emphasis, &type, &item, &box);
		HiliteControl((ControlHandle)item, inButton);
	}
	
	if (buf != NULL) {
		GetDItem(d, lastbutton+1, &type, &item, &box);
		GetIText(item, (unsigned char *)buf);
#ifndef CLEVERGLUE
		PtoCstr((unsigned char *)buf);
#endif
	}
	DisposDialog(d);
	return hit;
}

void
wmessage(prompt)
	char *prompt;
{
	do_dialog(oklist(prompt), 2, 2, (char *)NULL);
}

int
waskync(prompt, def)
	char *prompt;
{
	int emphasis;
	int hit;
	
	switch (def) {
	case 1:
		emphasis= 2;
		break;
	case 0:
		emphasis= 3;
		break;
	default:
		emphasis= 4;
		break;
	}
	
	hit= do_dialog(ynclist(prompt), emphasis, 4, (char *) NULL);
	
	switch (hit) {
	default: /* case 1: Return or Enter pressed */
		return def;
	case 2: /* Yes button */
		return 1;
	case 3: /* No button */
		return 0;
	case 4: /* Cancel button */
		return -1;
	}
}

bool
waskstr(prompt, buf, len)
	char *prompt;
	char *buf;
	int len;
{
	/* This code assumes 'buf' is at least 256 bytes long! */
	return do_dialog(editlist(prompt), 2, 3, buf) <= 2;
}

void
wperror(name)
	char *name;
{
	char buf[256];
	char *p= buf;
	
	if (name != NULL) {
		strcpy(p, name);
		strcat(p, ": ");
		p += strlen(p);
	}
	strcat(p, "I/O error");
	p += strlen(p);
	sprintf(p, " %d", errno);
	wmessage(buf);
}
