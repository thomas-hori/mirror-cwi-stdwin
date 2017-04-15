/* STDWIN TEXTEDIT PACKAGE INTERFACE */

/* This file is only ever included by "stdwin.h" */

#define TEXTEDIT struct _textedit

TEXTEDIT *tealloc _ARGS((WINDOW *win, int left, int top, int width));
TEXTEDIT *tecreate _ARGS((WINDOW *win,
	int left, int top, int right, int bottom));
void tefree _ARGS((TEXTEDIT *tp));
void tedestroy _ARGS((TEXTEDIT *tp));
void tesetactive _ARGS((TEXTEDIT *tp, /*bool*/int active));

void tedraw _ARGS((TEXTEDIT *tp));
void tedrawnew _ARGS((TEXTEDIT *tp,
	int left, int top, int right, int bottom));
void temove _ARGS((TEXTEDIT *tp, int left, int top, int width));
void temovenew _ARGS((TEXTEDIT *tp,
	int left, int top, int right, int bottom));

void tesetview _ARGS((TEXTEDIT *tp, int left, int top, int right, int bottom));
void tenoview _ARGS((TEXTEDIT *tp));

void tesetfocus _ARGS((TEXTEDIT *tp, int foc1, int foc2));
void tereplace _ARGS((TEXTEDIT *tp, char *str));
void tesetbuf _ARGS((TEXTEDIT *tp, char *buf, int buflen));

void tearrow _ARGS((TEXTEDIT *tp, int code));
void tebackspace _ARGS((TEXTEDIT *tp));
/*bool*/int teclicknew _ARGS((TEXTEDIT *tp, int h, int v, /*bool*/int extend, /*bool*/int dclick));
/*bool*/int tedoubleclick _ARGS((TEXTEDIT *tp, int h, int v));
/*bool*/int teevent _ARGS((TEXTEDIT *tp, EVENT *ep));

#define teclick(tp, h, v) teclicknew(tp, h, v, FALSE)
#define teclickextend(tp, h, v) teclicknew(tp, h, v, TRUE)

char *tegettext _ARGS((TEXTEDIT *tp));
int tegetlen _ARGS((TEXTEDIT *tp));
int tegetnlines _ARGS((TEXTEDIT *tp));
int tegetfoc1 _ARGS((TEXTEDIT *tp));
int tegetfoc2 _ARGS((TEXTEDIT *tp));
int tegetleft _ARGS((TEXTEDIT *tp));
int tegettop _ARGS((TEXTEDIT *tp));
int tegetright _ARGS((TEXTEDIT *tp));
int tegetbottom _ARGS((TEXTEDIT *tp));

/* Text paragraph drawing functions: */

int wdrawpar _ARGS((int h, int v, char *text, int width));
	/* Returns new v coord. */
int wparheight _ARGS((char *text, int width));
	/* Returns height */
