/* Edit Windows */

/* The data structure exported by this module is intended to
   be accessible to the caller;
   the routines provided here don't duplicate operations that
   can be done by calling text-edit or window routines directly.
   Exception: ewreplace is like tereplace, but also clears
   the 'saved' flag and sets the document size */

#include "stdwin.h"
#include "tools.h"
#include "editwin.h"

extern long ftell _ARGS((FILE *));

#define MAXFN 256 /* File name length */

static int ewnum;
static EDITWIN **ewlist;

EDITWIN *
ewfind(win)
	WINDOW *win;
{
	int i;

	if (win == NULL)
		return NULL;

	i= wgettag(win);
	
	if (i >= 0 && i < ewnum &&
			ewlist[i] != NULL && ewlist[i]->win == win)
		return ewlist[i];
	else
		return NULL;
}

int
ewcount()
{
	int count= 0;
	int i;
	for (i= 0; i < ewnum; ++i) {
		if (ewlist[i] != NULL)
			++count;
	}
	return count;
}

static void
ewdrawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	EDITWIN *ew= ewfind(win);
	if (ew != NULL)
		tedrawnew(ew->tp, left, top, right, bottom);
}

EDITWIN *
ewcreate(filename)
	char *filename;
{
	EDITWIN *ew= ALLOC(EDITWIN);
	int width, height;
	int hbar, vbar;
	int i;
	
	if (ew == NULL)
		return NULL;
	wgetdefscrollbars(&hbar, &vbar);
	wsetdefscrollbars(0, 1);
	ew->win= wopen(filename==NULL ? "Untitled" : filename, ewdrawproc);
	wsetdefscrollbars(hbar, vbar);
	if (ew->win == NULL) {
		FREE(ew);
		return NULL;
	}
	wsetwincursor(ew->win, wfetchcursor("ibeam"));
	wgetwinsize(ew->win, &width, &height);
	ew->tp= tecreate(ew->win, 0, 0, width, height);
	if (ew->tp == NULL) {
		wclose(ew->win);
		FREE(ew);
		return NULL;
	}
	if (filename != NULL) {
		if (!ewreadfile(ew, filename)) {
			tefree(ew->tp);
			wclose(ew->win);
			FREE(ew);
			return NULL;
		}
	}
	ew->filename= strdup(filename);
	ew->saved= TRUE;
	for (i= 0; i < ewnum; ++i) {
		if (ewlist[i] == NULL)
			break;
	}
	wsettag(ew->win, i);
	if (i >= ewnum) {
		L_APPEND(ewnum, ewlist, EDITWIN*, NULL);
	}
	ewlist[i]= ew;
	return ew;
}

EDITWIN *
ewnew()
{
	return ewcreate((char*)NULL);
}

EDITWIN *
ewopen()
{
	char filename[MAXFN];
	
	filename[0]= EOS;
	if (waskfile("Open file:", filename, sizeof filename, FALSE))
		return ewcreate(filename);
	else
		return NULL;
}

bool
ewclose(ew)
	EDITWIN *ew;
{
	int i;
	
	if (!ew->saved) {
		char buf[MAXFN+25];
		sprintf(buf, "Save changes to \"%s\" before closing?",
			ew->filename==NULL ? "Untitled" : ew->filename);
		switch (waskync(buf, 1)) {
		case -1:
			return FALSE;
		case 0:
			break;
		case 1:
			if (!ewsave(ew))
				return FALSE;
			break;
		}
	}
	i= wgettag(ew->win);
	if (i >= 0 && i < ewnum && ewlist[i] == ew)
		ewlist[i]= NULL;
	tefree(ew->tp);
	wclose(ew->win);
	FREE(ew->filename);
	FREE(ew);
	return TRUE;
}

bool
ewsave(ew)
	EDITWIN *ew;
{
	if (ew->saved)
		return TRUE;
	if (!ew->filename)
		return ewsaveas(ew);
	return ew->saved= ewwritefile(ew, ew->filename);
}

bool
ewsaveas(ew)
	EDITWIN *ew;
{
	return ewsaveprompt(ew, "Save as:", TRUE);
}

bool
ewsavecopy(ew)
	EDITWIN *ew;
{
	return ewsaveprompt(ew, "Save a copy as:", FALSE);
}

bool
ewsaveprompt(ew, prompt, changefile)
	EDITWIN *ew;
	char *prompt;
	bool changefile;
{
	char filename[MAXFN];
	
	filename[0]= EOS;
	if (ew->filename != NULL) {
		strncpy(filename, ew->filename, MAXFN);
		filename[MAXFN-1]= EOS;
	}
	if (!waskfile(prompt, filename, sizeof filename, TRUE))
		return FALSE;
	if (!ewwritefile(ew, filename))
		return FALSE;
	if (changefile) {
		FREE(ew->filename);
		ew->filename= strdup(filename);
		wsettitle(ew->win, filename);
		ew->saved= TRUE;
	}
	return TRUE;
}

bool
ewrevert(ew)
	EDITWIN *ew;
{
	char buf[MAXFN+50];
	
	if (ew->saved)
		return TRUE;
	if (ew->filename == NULL) {
		wmessage("Not saved yet");
		return FALSE;
	}
	sprintf(buf, "Discard changes since last save to \"%s\" ?",
		ew->filename);
	if (waskync(buf, 1) < 1)
		return FALSE;
	return ewreadfile(ew, ew->filename);
}

bool
ewreadfile(ew, filename)
	EDITWIN *ew;
	char *filename;
{
	FILE *fp= fopen(filename, "r");
	long size;
	char *buf;
	
	if (fp == NULL) {
		wperror(filename);
		return FALSE;
	}
	if (fseek(fp, 0L, 2) == -1) {
		wperror (filename);
		fclose (fp);
		return FALSE;
	}
	size= ftell(fp);
	if (size >= 1L<<15) {
		char mbuf[MAXFN + 50];
		sprintf(mbuf, "File \"%s\" is big (%d K).  Still edit?",
			filename, (int) ((size+1023)/1024));
		if (waskync(mbuf, 1) < 1) {
			fclose(fp);
			return FALSE;
		}
	}
	buf= malloc((unsigned)size);
	if (buf == NULL) {
		wmessage("Can't get memory for buffer");
		fclose(fp);
		return FALSE;
	}
	if (fseek(fp, 0L, 0) == -1) {
		wperror (filename);
		fclose (fp);
		return FALSE;
	}
	if ((size = fread(buf, 1, (int) size, fp)) == -1) {
		wmessage("Read error");
		fclose(fp);
		return FALSE;
	}
	fclose(fp);
	tesetbuf(ew->tp, buf, (int) size); /* Gives the buffer away! */
	ew->saved= TRUE;
	ewsetdimensions(ew);
	return TRUE;
}

void
ewsetdimensions(ew)
	EDITWIN *ew;
{
	wsetdocsize(ew->win, 0, tegetbottom(ew->tp));
}

bool
ewwritefile(ew, filename)
	EDITWIN *ew;
	char *filename;
{
	FILE *fp= fopen(filename, "w");
	char *buf;
	int len, nwritten;
	
	if (fp == NULL) {
		wperror(filename);
		return FALSE;
	}
	buf= tegettext(ew->tp);
	len= strlen(buf);
	nwritten= fwrite(buf, 1, len, fp);
	fclose(fp);
	if (nwritten != len) {
		wmessage("Write error");
		return FALSE;
	}
	/* Can't set saved to TRUE, because of 'save a copy' */
	return TRUE;
}

bool
ewevent(ew, e, closed_return)
	EDITWIN *ew;
	EVENT *e;
	bool *closed_return;
{
	bool closed= FALSE;
	bool change= FALSE;
	
	if (ew == NULL) {
		ew= ewfind(e->window);
		if (ew == NULL)
			return FALSE;
	}
	else if (e->window != ew->win)
		return FALSE;
	
	switch (e->type) {
	
	case WE_ACTIVATE:
		break;
		
	case WE_SIZE:
		{
			int width, height;
			wgetwinsize(ew->win, &width, &height);
			temovenew(ew->tp, 0, 0, width, height);
			ewsetdimensions(ew);
		}
		break;
		
	case WE_CLOSE:
		closed= ewclose(ew);
		break;

	case WE_COMMAND:
		if (e->u.command == WC_CLOSE) {
			closed= ewclose(ew);
			break;
		}
		/* Else, fall through */
		if (e->u.command == WC_RETURN ||
			e->u.command == WC_TAB ||
			e->u.command == WC_BACKSPACE)
			change= TRUE;
		goto def; /* Go pass it on to teevent */
	
	case WE_MOUSE_DOWN:
		/* Edit the coordinates slightly so teevent always
		   believes it is in the rect */
		{
			int left= tegetleft(ew->tp);
			int right= tegetright(ew->tp);
			int top= tegettop(ew->tp);
			int bottom= tegetbottom(ew->tp);
			CLIPMIN(e->u.where.h, left);
			CLIPMAX(e->u.where.h, right);
			CLIPMIN(e->u.where.v, top);
			if (e->u.where.v >= bottom) {
				e->u.where.v= bottom;
				e->u.where.h= right;
			}
		}
		/* Fall through */
	default:
	def:
		if (!teevent(ew->tp, e))
			return FALSE;
		if (e->type == WE_CHAR)
			change= TRUE;
		if (change) {
			ew->saved= FALSE;
			ewsetdimensions(ew);
		}
		break;
		
	}
	
	if (closed_return != NULL)
		*closed_return= closed;
	return TRUE;
}

bool
ewsaveall()
{
	int i;
	
	for (i= 0; i < ewnum; ++i) {
		if (ewlist[i] != NULL && !ewsave(ewlist[i]))
			return FALSE;
	}
	return TRUE;
}

bool
ewcloseall()
{
	int i;
	
	for (i= 0; i < ewnum; ++i) {
		if (ewlist[i] != NULL && !ewclose(ewlist[i]))
			return FALSE;
	}
	return TRUE;
}

void
ewreplace(ew, str)
	EDITWIN *ew;
	char *str;
{
	if (*str == EOS && tegetfoc1(ew->tp) == tegetfoc2(ew->tp))
		return;
	tereplace(ew->tp, str);
	ew->saved= FALSE;
	ewsetdimensions(ew);
}

/*ARGSUSED*/
void
ewundo(ew)
	EDITWIN *ew;
{
	wfleep();
}

void
ewcopy(ew)
	EDITWIN *ew;
{
	int f1= tegetfoc1(ew->tp);
	int f2= tegetfoc2(ew->tp);
	char *text;
	if (f1 == f2)
		wfleep();
	else {
		text= tegettext(ew->tp);
		wsetclip(text+f1, f2-f1);
	}
		
}

void
ewpaste(ew)
	EDITWIN *ew;
{
	char *text= wgetclip();
	if (text == NULL)
		wfleep();
	else
		ewreplace(ew, text);
}
