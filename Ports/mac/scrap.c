/* Macintosh Scrap (Clipboard) Interface */

#include "macwin.h"
#include <Scrap.h>
#include <Memory.h>

static Handle hclip;
static long lenclip;

/* _wfreeclip is called from wgetevent and from wsetclip. */

void
_wfreeclip()
{
	if (hclip != NULL) {
		HUnlock(hclip);
		DisposHandle(hclip);
		hclip= NULL;
	}
}

char *
wgetclip()
{
	long offset;
	
	/* NB if the use of hclip or lenclip changes,
	   also change wgetcutbuffer() below !!! */
	
	if (hclip == NULL)
		hclip= NewHandle(1L);
	else
		HUnlock(hclip);
	lenclip= GetScrap(hclip, 'TEXT', &offset);
	if (lenclip < 0)
		return NULL;
	SetHandleSize(hclip, lenclip+1);
	HLock(hclip);
	(*hclip)[lenclip]= EOS;
#ifndef CLEVERGLUE
	{
	/* Convert imported \r into \n */
		char *p= *hclip;
		while ((p= strchr(p, '\r')) != NULL)
			*p++ = '\n';
	}
#endif
	return *hclip;
}

void
wsetclip(p, len)
	char *p;
	int len;
{
	int err;
#ifndef CLEVERGLUE
	/* Convert \n into \r before exporting.  Must make a copy, shit! */
	char *q= malloc(len+1);
	if (q != NULL) {
		strncpy(q, p, len);
		q[len]= EOS;
		p= q;
		while ((p= strchr(p, '\n')) != NULL)
			*p++ = '\r';
		p= q;
	}
	/* If there's no memory, export with \n's left in... */
#endif
	_wfreeclip();
	err= ZeroScrap();
	if (err != 0) dprintf("wsetclip: ZeroScrap error %d", err);
	err= PutScrap((long)len, 'TEXT', p);
	if (err != 0) dprintf("wsetclip: PutScrap error %d", err);
#ifndef CLEVERGLUE
	if (q != NULL)
		free(q);
#endif
}

/* For "compatibility" with X11 STDWIN: */

int
wsetselection(win, sel, data, len)
	WINDOW *win;
	int sel;
	char *data;
	int len;
{
	return 0;
}

char *
wgetselection(sel, plen)
	int sel;
	int *plen;
{
	return NULL;
}

void
wresetselection(sel)
	int sel;
{
}

void
wsetcutbuffer(ibuffer, data, len)
	int ibuffer;
	char *data;
	int len;
{
	if (ibuffer == 0)
		wsetclip(data, len);
}

char *
wgetcutbuffer(ibuffer, plen)
	int ibuffer;
	int *plen;
{
	if (ibuffer != 0)
		return NULL;
	if (wgetclip() == NULL)
		return NULL;
	/* This knows about the implementation of wgetclip() */
	*plen = lenclip; /* NB: long --> int, may truncate... */
	return *hclip;
}

void
wrotatecutbuffers(n)
	int n;
{
}
