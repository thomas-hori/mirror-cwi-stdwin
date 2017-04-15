/* ANSI escape sequence interpreter */

/* There is some ugly code here, since I assume that code looking
   at the input a character at a time is very time-critical.
   Action functions are called with the start of the text to process,
   and return a pointer to where they left off.
   When an action function hits the end of the string,
   it returns NULL and sets the action pointer to the function to
   call next (often itself), or to NULL for the default.
   A non-NULL return implies that the default action function must
   be called next and the action pointer is irrelevant.
   Remember that the general form of most ANSI escape sequences is
	ESC [ <n1> ; <n2> ; ... <c>
   where <c> is the command code and <n1>, <n2> etc. are optional
   numerical parameters.
*/

#include "vtimpl.h"

#ifdef DEBUG
 /* Let it be heard that a command was not recognized */
#define FAIL() wfleep()
#else
/* Silently ignore unrecognized commands */
#define FAIL() 0
#endif

/* Function prototypes */

STATIC char *def_action _ARGS((VT *vt, char *text, char *end));
STATIC char *esc_action _ARGS((VT *vt, char *text, char *end));
STATIC char *ansi_params _ARGS((VT *vt, char *text, char *end));
STATIC char *ansi_execute _ARGS((VT *vt, char *text));
STATIC void vtsetoptions _ARGS((VT *vt, bool flag));
STATIC void vtsetattrlist _ARGS((VT *vt));

/* Output a string, interpreting ANSI escapes */

void
vtansiputs(vt, text, len)
	VT *vt;
	char *text;
	int len;
{
	char *end;

#ifndef NDEBUG
	if (vt->scr_top < 0 || vt->scr_top > vt->rows) {
		fprintf(stderr, "vtansiputs: scr_top = %d\n", vt->scr_top);
		vtpanic("vtansiputs: bad scr_top");
	}
#endif

	if (len < 0)
		len = strlen(text);
	end = text + len;

	D( printf("vtansiputs %d chars\n", len) );
	
	/* Pick up where the last call left us behind  */
	if (vt->action != NULL)
		text = (*vt->action)(vt, text, end);

	/* Start in default state until text exhausted */
	while (text != NULL)
		text = def_action(vt, text, end);

	/* Execute delayed scrolling */
	vtsync(vt);
	D( printf("vtansiputs returns\n") );
}

/* Default action function */

STATIC char *
def_action(vt, text, end)
	VT *vt;
	char *text, *end;
{
again:
	for (;;) {
		if (text >= end) {
			vt->action = NULL;
			return NULL;
		}
		if (PRINTABLE(*text))
			break;
		switch (*text++) {

		case ESC:	/* ESC */
			return esc_action(vt, text, end);
		
		case BEL:	/* Bell */
			if (vt->visualbell) {
				VTBEGINDRAWING(vt);
				vtinvert(vt, 0, 0, vt->rows, vt->cols);
				VTENDDRAWING(vt);
				VTBEGINDRAWING(vt);
				vtinvert(vt, 0, 0, vt->rows, vt->cols);
				VTENDDRAWING(vt);
			}
			else {
				wfleep();
			}
			break;
		
		case BS:	/* Backspace -- move 1 left */
			/* Rely on vtsetcursor's clipping */
			vtsetcursor(vt, vt->cur_row, vt->cur_col - 1);
			/* Don't erase --
			   that's part of intelligent echoing */
			break;
		
		case TAB:	/* Tab -- move to next tab stop */
			/* Rely on vtsetcursor's clipping */
			/* TO DO: use programmable tab stops! */
			vtsetcursor(vt, vt->cur_row,
				(vt->cur_col & ~7) + 8);
			/* Normalize cursor (may cause scroll!) */
			vtputstring(vt, "", 0);
			break;
		
		case LF:	/* Linefeed -- move down one line */
			if (vt->nlcr)	/* First a \r? */
				vtsetcursor(vt, vt->cur_row, 0);
			vtlinefeed(vt, 1);
			break;
		
		case FF:	/* Formfeed */
			/* vtreset(vt); */
			break;
		
		case CR:	/* Return -- to col 0 on same line */
			vtsetcursor(vt, vt->cur_row, 0);
			break;
		
		case ' ':	/* In case isprint(c) fails for space */
			vtputstring(vt, " ", 1);
			break;

		default:
			D( printf("Unrecognized unprintable character 0x%x\n",
								text[-1]) );
			FAIL();
			break;
		}
	}

	/* We fall through the previous loop when we get a printable
	   character */
	
	{
		char *p = text;
		
		while (PRINTABLE(*++p)) {
			/* At least one character guaranteed! */
			if (p >= end)
				break;
		}
		vtputstring(vt, text, (int)(p-text));
		text = p;
		goto again;
	}
}

/* Action function called after ESC seen */

STATIC char *
esc_action(vt, text, end)
	VT *vt;
	char *text, *end;
{
	if (text >= end) {
		vt->action = esc_action;
		return NULL;
	}
	switch (*text++) {
/*
	case '(':
	case ')':
	case '*':
	case '+':
		return cset_action(vt, text, end);
*/
	case '=':
		vt->keypadmode = TRUE;
		break;
	case '>':
		vt->keypadmode = FALSE;
		break;
	case '7':
		vtsavecursor(vt);
		break;
	case '8':
		vtrestorecursor(vt);
		break;
	case 'D':
		vtlinefeed(vt, 1);
		break;
	case 'E':
		/* Next Line */
		break;
	case 'H':
		/* Tab set */
		break;
	case 'M':
		vtrevlinefeed(vt, 1);
		break;
	case '[':
		vt->nextarg = vt->args;
		*vt->nextarg = -1;
		vt->modarg = '\0';
		return ansi_params(vt, text, end);
	case 'c':
		vtreset(vt);
		break;
	default:
		D( printf("Urecognized: esc-%c (0x%x)\n",
			    text[-1], text[-1]) );
		FAIL();
		break;
	}
	return text;
}

/* Action function called after ESC-[ plus possible parameters seen */

STATIC char *
ansi_params(vt, text, end)
	VT *vt;
	char *text, *end;
{
again:
	if (text >= end) {
		vt->action = ansi_params;
		return NULL;
	}
	if (isdigit(*text)) {
		long a = *vt->nextarg;
		if (a < 0) /* Still unused? */
			a = 0;
		do {
			a = a*10 + (*text - '0');
			CLIPMAX(a, 0x7fff); /* Avoid overflow */
		} while (++text < end && isdigit(*text));
		*vt->nextarg = a; /* Remember arg under construction */
		if (text >= end)
			goto again;
	}
	switch (*text) {

	case ';': /* Arg no longer under construction */
		++text;
		if (vt->nextarg < &vt->args[VTNARGS-1])
			++vt->nextarg; /* Else: overflow; who cares? */
		*vt->nextarg = -1; /* Mark unused */
		goto again;

	case '?':
		++text;
		if (vt->nextarg == vt->args && /* No arguments yet */
			*vt->nextarg < 0 && vt->modarg == '\0') {
			vt->modarg = '?';
			goto again;
		}
		else { /* Illegal here */
			D( printf("Wrong argcount in DEC private mode\n") );
			FAIL();
			return text;
		}

	default:
		return ansi_execute(vt, text);
	}
}

/* Called after complete ESC [ ? <number> h is parsed.
   This is called DEC private mode set. Most stuff is not
   implemented. The vt is guarantueed to contain the one
   and only argument allowed here. */

STATIC void
vtprivset(vt)
	VT *vt;
{
	switch (vt->args[0]) {
	case 1:	/* Application cursor keys */
		if (!vt->keypadmode) {
			vt->keypadmode = 1;
			vt->flagschanged = 1;
		}
		break;
#if 0
	case 3: /* 132 column mode */
	case 4: /* Smooth (slow) scroll */
#endif
	case 5: /* Reverse video */
		wfleep();
		break;
#if 0
	case 6: /* Origin mode */
	case 7: /* Wraparound mode */
	case 8: /* Autorepeat keys */
#endif
	case 9: /* Send MIT mouse row & column on Button Press */
		if (!vt->mitmouse) { /* If not already so */
			vt->mitmouse = 1;
			vt->flagschanged = 1;
		}
		break;
#if 0
	case 38: /* Enter Tektronix Mode */
	case 40: /* Allow 80 <-> 132 mode */
	case 41: /* Curses(5) fix */
	case 44: /* Turn on margin bell */
	case 45: /* Reverse wraparound mode */
	case 46: /* Start logging */
	case 47: /* Use alternate screen buffer */
	case 1000: /* Send vt200 mouse row & column on Button Press */
	case 1003: /* Send vt200 Hilite mouse row & column on Button Press */
#endif
	default:
		D( printf("Unsupported DEC private mode set:") );
		D( printf("esc [ ? %d h\n", vt->args[0]) );
		wfleep();
	}
}

/* Called after complete ESC [ ? <number> l is parsed.
   This is called DEC private mode reset. The vt is guarantueed
   to contain the one and only argument allowed here. */

STATIC void
vtprivreset(vt)
	VT *vt;
{
	switch (vt->args[0]) {
	case 1: /* Normal cursor keys */
		if (vt->keypadmode) {
			vt->keypadmode = 0;
			vt->flagschanged = 1;
		}
		break;
#if 0	/* These are not supprted: */
	case 3: /* 80 column mode */
	case 4: /* Jumpscroll */
#endif
	case 5: /* Normal video (i.e. not reverse) */
		/* XXX Why this beep? */
		wfleep();
		break;
#if 0
	case 6: /* Normal cursor mode */
	case 7: /* No wraparound */
	case 8: /* No autorepeat */
		break;
#endif
	case 9: /* Don't send mouse row & column on button press */
	case 1000:	/* Same */
	case 1003:	/* Same */
		if (vt->mitmouse) { /* If not already so */
			vt->mitmouse = 0;
			vt->flagschanged = 1;
		}
		break;
#if 0
	case 40: /* Disallow 80 <-> 132 mode */
	case 41: /* No curses(5) fix */
	case 44: /* Turn off Margin bell */
	case 45: /* No reverse wraparound */
	case 46: /* Stop logging */
	case 47: /* Use normal screen buffer (opposed to alternate buffer) */
		break;
#endif
	default:
		D( printf("Unsupported DEC private mode reset:") );
		D( printf("esc [ ? %d l\n", vt->args[0]) );
		wfleep();
		break;
	}
}

/* Action function called at last char of ANSI sequence.
   (This is only called when the char is actually seen,
   so there is no need for an 'end' parameter). */

STATIC char *
ansi_execute(vt, text)
	VT *vt;
	char *text;
{
	int a1 = vt->args[0];
	int a2 = (vt->nextarg > vt->args) ? vt->args[1] : -1;

	if (vt->modarg == '?') {
		/* These escape sequences have exactly one numeric parameter */
		if (a1 < 0 || a2 > 0) {
			wfleep();
			return text+1;
		}
		switch (*text++) {
		case 'h':	/* DEC Private mode Set */
			vtprivset(vt);
			break;
		case 'l':	/* DEC Private mode Reset */
			vtprivreset(vt);
			break;
		/* To do: add private mode memory to vt-descriptor? */
		case 'r':	/* DEC Private mode Restore */
		case 's':	/* DEC Private mode Save */
		default:
			/* Not supported or simply wrong */
			wfleep();
		}
		return text;
	}

	CLIPMIN(a1, 1);
	CLIPMIN(a2, 1);

	switch (*text++) {
	case '@': vtinschars(vt, a1);		break;
	case 'A': vtarrow(vt, WC_UP, a1);	break;
	case 'B': vtarrow(vt, WC_DOWN, a1);	break;
	case 'C': vtarrow(vt, WC_RIGHT, a1);	break;
	case 'D': vtarrow(vt, WC_LEFT, a1);	break;
	case 'H':
	case 'f': if (vt->nextarg > &vt->args[0])
		    vtsetcursor(vt, vt->topterm + a1 - 1, a2 - 1);
		  else vtsetcursor(vt, vt->topterm, 0);
		  break;
	case 'J':
		switch (vt->args[0]) {
		case -1:
		case 0:
			vteosclear(vt, vt->topterm, 0);
			break;
		case 1:
			/* clear above cursor */
			break;
		case 2:
			vteosclear(vt, vt->cur_row, vt->cur_col);
			break;
		default:
			FAIL();
			break;
		}
		break;
	case 'K':
		switch (vt->args[0]) {
		case -1:
		case 0:
			vteolclear(vt, vt->cur_row, vt->cur_col);
			break;
		case 1:
			/* Clear left of cursor */
			break;
		case 2:
			vteolclear(vt, vt->cur_row, 0);
			break;
		default:
			FAIL();
			break;
		}
		break;
	case 'L': vtinslines(vt, a1);		break;
	case 'M': vtdellines(vt, a1);		break;
	case 'P': vtdelchars(vt, a1);		break;
	case 'S': vtlinefeed(vt, a1);		break;
	case 'T': vtrevlinefeed(vt, a1);	break;
	case 'c': vtsendid(vt); break;
	case 'g': /* Tab clear */		break;
		/* 0: current col; 3: all */
	case 'h': vtsetoptions(vt, TRUE);	break;
	case 'l': vtsetoptions(vt, FALSE);	break;
	case 'm': vtsetattrlist(vt);		break;
	case 'n':
		if (a1 == 6) vtsendpos(vt);
		/* 5: echo 'ESC [ 0 n' */
		break;
	case 'r':
		vtsetscroll(vt, vt->topterm+vt->args[0]-1, vt->topterm+a2);
		break;
	case 'x': /* Send terminal params */
		break;
	default:
		FAIL();
		break;
	}

	return text;
}

/* Set/reset numbered options given in args array */

STATIC void
vtsetoptions(vt, flag)
	VT *vt;
	bool flag; /* Set/reset */
{
	short *a;
	for (a = vt->args; a <= vt->nextarg; ++a) {
		switch (*a) {
		case 4:
			vtsetinsert(vt, flag);
			break;
		case -1:
			/* Empty parameter, don't beep */
			break;
		default:
			FAIL();
			break;
		}
	}
}

/* Set/reset output mode attributes given in args array */

STATIC void
vtsetattrlist(vt)
	VT *vt;
{
	short *a;
	for (a = vt->args; a <= vt->nextarg; ++a) {
		switch (*a) {
		case -1:
			if (a == vt->args)
				vtresetattr(vt);
			break;
		case 0:
			vtresetattr(vt);
			break;
		default:
			vtsetattr(vt, *a);
			break;
		}
	}
}
