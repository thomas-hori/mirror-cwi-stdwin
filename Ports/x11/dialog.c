/* X11 STDWIN -- Dialog boxes */

#include "x11.h"
#include "llevent.h"

/* Forward static function declarations */

static int dialog _ARGS((char *, char *, int, int, int));
static void addbutton _ARGS((int, char *, int, bool, int, int, int, int));
static int dialogeventloop _ARGS((int));
static int charcode _ARGS((XKeyEvent *));
static bool addchar _ARGS((int));

/* The kinds of dialogs we know */

#define MESSAGEKIND	0
#define ASKYNCKIND	1
#define ASKSTRKIND	2

/* Display a message and wait until acknowledged */

void
wmessage(prompt)
	char *prompt;
{
	(void) dialog(prompt, (char*)NULL, 0, MESSAGEKIND, 0);
}

/* Ask a yes/no question.
   Return value: yes ==> 1, no ==> 0, cancel (^C) ==> -1.
   Only the first non-blank character of the string typed is checked.
   The 'def' parameter is returned when an empty string is typed. */

int
waskync(prompt, def)
	char *prompt;
	int def;
{
	return dialog(prompt, (char*)NULL, 0, ASKYNCKIND, def);
}

/* Ask for a string. */

bool
waskstr(prompt, buf, len)
	char *prompt;
	char *buf;
	int len;
{
	return dialog(prompt, buf, len, ASKSTRKIND, TRUE);
}

/* Definition for the current dialog box */

static struct windata box;	/* Window descriptor for the dialog box */
static unsigned long fg, bg;	/* Foreground, background pixel values */
static GC b_gc;			/* Corresponding Graphics Context */

/* Definition for a command "button" or text item in a dialog box */

struct button {
	char type;		/* Button, label or text */
	char *text;		/* The button label */
	int ret;		/* Value to return when chosen */
	struct windata wd;	/* Window description */
	tbool down;		/* Button is pressed */
	tbool inside;		/* Mouse inside button */
	tbool hilite;		/* Button is hilited */
	tbool def;		/* Set if default button */
};

/* Constants used in button definition 'type' field above */

#define LABEL	'l'
#define TEXT	't'
#define BUTTON	'b'

static int nbuttons;		/* number of buttons and items */
static struct button *buttonlist; /* Definition list */

/* Constants guiding button dimensions */

static int charwidth, textheight;
static int promptwidth;
static int buttonwidth;

/* Forward static function declarations */

static bool buttonevent _ARGS((struct button *, XEvent *));
static void drawbutton _ARGS((struct button *));
static void hilitebutton _ARGS((struct button *, bool));
static void gettext _ARGS((char *, int));
static void killbuttons _ARGS((void));

/* Dialog routine.
   Create the window and subwindows, then engage in interaction.
   Return the proper return value for waskync or waskstr.
*/

/* Dimension of buttons:
   vertically:
   	- 1 pixel white space around text,
   	- 1 pixel wide border,
   	- q pixel white space around border
   horizontally:
   	- 1 space character around text
   	- 1 pixel wide border
   	- 1 pixel white space around border
   The prompt and reply are placed as buttons but their
   border is white.
   For a message, the OK button is to the right of the text,
   separated from the prompt by two extra space characters.
*/

static int
dialog(prompt, buf, len, kind, def)
	char *prompt;		/* Prompt string */
	char *buf;		/* Reply buffer, or NULL if unused */
	int len;		/* Size of reply buffer */
	int kind;		/* Dialog kind */
	int def;		/* Default return value */
{
	WINDOW *act;
	XFontStruct *save_wf= _wf;
	int ret;
	int textlen= 60;
	
	_wf= _wmf; /* So we can use wtextwidth() etc. */
	
	/* Compute various useful dimensions */
	charwidth= wcharwidth(' ');
	textheight= wlineheight();
	promptwidth= wtextwidth(prompt, -1);
	if (kind == MESSAGEKIND)
		buttonwidth= wtextwidth("OK", 2);
	else
		buttonwidth= wtextwidth("Cancel", 6);
	
	/* Compute the outer box dimensions */
	switch (kind) {
	case MESSAGEKIND:
		box.width= promptwidth + buttonwidth + 6*charwidth + 8;
		box.height= textheight + 6;
		break;
	case ASKYNCKIND:
		box.width= promptwidth + 2*charwidth + 4;
		CLIPMIN(box.width, 3*(buttonwidth + 2*charwidth + 4));
		box.height= 2 * (textheight + 6);
		break;
	default:
		_wdebug(0, "dialog: bad kind");
		kind= ASKSTRKIND;
		/* Fall through */
	case ASKSTRKIND:
		box.width= promptwidth + 2*charwidth + 4;
		CLIPMAX(textlen, len);
		CLIPMIN(box.width, textlen*charwidth);
		CLIPMIN(box.width, 2*(buttonwidth + 2*charwidth + 4));
		box.height= 3 * (textheight + 6);
		break;
	}
	CLIPMAX(box.width, WidthOfScreen(_ws));
	CLIPMAX(box.height, HeightOfScreen(_ws));
	
	/* Compute the box position:
	   above a window if possible,
	   on the screen if necessary. */
	
	/* XXX Default placement could use an option resource */
	
	act= _w_get_last_active();
	if (act != NULL) {
		Window child_dummy;
		if (!XTranslateCoordinates(_wd, act->wi.wid,
			RootWindowOfScreen(_ws),
#ifdef CENTERED
			(act->wi.width - box.width) / 2,
			(act->wi.height - box.height) / 2,
#else
			0,
			0,
#endif
			&box.x, &box.y, &child_dummy))
			act= NULL; /* Couldn't do it -- center on screen */
	}
	if (act == NULL) {
		/* No window to cover */
#ifdef CENTERED
		/* center the box in the screen */
		box.x= (WidthOfScreen(_ws) - box.width) / 2;
		box.y= (HeightOfScreen(_ws) - box.height) / 2;
#else
		/* use top left screen corner */
		box.x= box.y = 2*IBORDER + 1;
		/* well, 1 pixel from the corner, to fool twm */
#endif
	}
	
	/* Clip the box to the screen */
	
	CLIPMAX(box.x, WidthOfScreen(_ws) - box.width);
	CLIPMAX(box.y, HeightOfScreen(_ws) - box.height);
	CLIPMIN(box.x, 0);
	CLIPMIN(box.y, 0);
	_wdebug(1, "dialog box: x=%d y=%d w=%d h=%d",
		box.x, box.y, box.width, box.height);
	
	/* Create the box window and its GC */
	
	fg = _wgetpixel("menuForeground", "MenuForeground", _w_fgcolor);
	bg = _wgetpixel("menuBackground", "MenuBackground", _w_bgcolor);
	box.border= 2*IBORDER;
	(void) _wcreate(&box, RootWindowOfScreen(_ws), 0, FALSE, fg, bg);
	_wsaveunder(&box, True);
	XSelectInput(_wd, box.wid,
		ExposureMask|KeyPressMask|StructureNotifyMask);
	b_gc= _wgcreate(box.wid, _wf->fid, fg, bg);
	XSetPlaneMask(_wd, b_gc, fg ^ bg);
	
	/* Keep window managers happy:
	   a name for WM's that insist on displaying a window title;
	   class hints;
	   WM hints;
	   size hints to avoid interactive window placement;
	   and "transient hints" to link it to an existing window. 
	   The latter two only if the dialog box belongs to a window. */
	
	/* XXX This code could be unified with similar code in windows.c */
	
	/* The name is taken from _wprogname, to make it clear what
	   application a dialog belongs to (especially if there is
	   no window for it yet). */
	
	XStoreName(_wd, box.wid, _wprogname);
	
	/* Set class hints */
	{
		XClassHint classhint;
		classhint.res_name= _wprogname;
		classhint.res_class= "StdwinDialog";
		XSetClassHint(_wd, box.wid, &classhint);
	}
	
	/* Set WM hints */
	{
		XWMHints wmhints;
		wmhints.flags = InputHint | StateHint;
		wmhints.input = 1;
		wmhints.initial_state = NormalState;
		XSetWMHints(_wd, box.wid, &wmhints);
	}
	
	if (act != NULL) {
		XSizeHints sizehints;
		
		/* Pretend the user specified the size and position,
		   in an attempt to avoid window manager interference */
		sizehints.x= box.x - box.border;
		sizehints.y= box.y - box.border;
		sizehints.width= box.width;
		sizehints.height= box.height;
		sizehints.flags= USPosition|USSize;
		XSetNormalHints(_wd, box.wid, &sizehints);
		
		XSetTransientForHint(_wd, box.wid, act->wo.wid);
	}
	
	/* Create the prompt label */
	
	addbutton(LABEL, prompt, def, FALSE,
		2,
		2,
		promptwidth + 2*charwidth,
		textheight + 2);
	
	/* Create the command buttons and text field (for ASKSTRKIND).
	   Note that our x, y convention differs from XCreateWindow:
	   we don't include the border width. */
	
	switch (kind) {
	case MESSAGEKIND:
		addbutton(BUTTON, "OK", def, FALSE,
			box.width - buttonwidth - 2*charwidth - 2, 2,
			buttonwidth + 2*charwidth,
			textheight + 2);
		break;
	case ASKYNCKIND:
		addbutton(BUTTON, "Yes", 1, def==1,
			2,
			box.height - textheight - 4,
			buttonwidth + 2*charwidth,
			textheight + 2);
		addbutton(BUTTON, "No", 0, def==0,
			buttonwidth + 2*charwidth + 6,
			box.height - textheight - 4,
			buttonwidth + 2*charwidth,
			textheight + 2);
		addbutton(BUTTON, "Cancel", -1, def==-1,
			box.width - buttonwidth - 2*charwidth - 2,
			box.height - textheight - 4,
			buttonwidth + 2*charwidth,
			textheight + 2);
		break;
	case ASKSTRKIND:
		addbutton(BUTTON, "OK", 1, FALSE,
			2,
			box.height - textheight - 4,
			buttonwidth + 2*charwidth,
			textheight + 2);
		addbutton(BUTTON, "Cancel", 0, FALSE,
			box.width - buttonwidth - 2*charwidth - 2,
			box.height - textheight - 4,
			buttonwidth + 2*charwidth,
			textheight + 2);
		addbutton(TEXT, buf, def, FALSE,
			2,
			textheight + 8,
			box.width - 4,
			textheight + 2);
		break;
	}
	
	/* Finish the work.
	   Map the window, process events, extract text,
	   destroy everything, return value. */
	
	XMapRaised(_wd, box.wid);
	
	ret=dialogeventloop(def);
	if (kind == ASKSTRKIND && ret < 0)
		ret= 0;
	
	if (ret > 0 && len > 0)
		gettext(buf, len);
	
	XFreeGC(_wd, b_gc);
	killbuttons();
	XDestroyWindow(_wd, box.wid);
	XFlush(_wd);
	
	_wf= save_wf;
	
	return ret;
}

/* Add a command button */

static void
addbutton(type, text, ret, def, x, y, width, height)
	int type;
	char *text;
	int ret;
	bool def;
	int x, y, width, height;
{
	struct button b;
	int cursor= (type == BUTTON) ? XC_arrow : 0;
	
	b.type= type;
	b.text= strdup(text);
	b.ret= ret;
	b.def= def;
	
	b.wd.x= x;
	b.wd.y= y;
	b.wd.width= width;
	b.wd.height= height;
	
	if (type == LABEL)
		b.wd.border= 0;
	else
		b.wd.border= IBORDER;
	(void) _wcreate(&b.wd, box.wid, cursor, TRUE, fg, bg);
	XSelectInput(_wd, b.wd.wid,
		type == BUTTON ?
			ExposureMask|ButtonPressMask|ButtonReleaseMask|
			EnterWindowMask|LeaveWindowMask
		:	ExposureMask);
	
	b.down= 0;
	b.inside= FALSE;
	b.hilite= type == TEXT && text[0] != EOS;
	
	L_APPEND(nbuttons, buttonlist, struct button, b);
}

/* Dialog event processing loop.
   Return number of button selected, -1 for Return, -2 for ^C. */

static struct button *whichbutton(); /* Forward */

static int
dialogeventloop(def)
	int def;
{
	for (;;) {
		XEvent e;
		struct button *bp;
		int c;
		
		XNextEvent(_wd, &e);
		_wdebug(3, "dlog2: evt type %d", e.type);
		bp= whichbutton(e.xbutton.window);
		if (bp != NULL) {
			if (buttonevent(bp, &e)) {
				return bp->ret;
			}
		}
		else if (e.xbutton.window == box.wid || e.type == KeyPress) {
			switch (e.type) {
			
			case KeyPress:
				switch (c= charcode(&e.xkey)) {
				case EOS:
					break;
				case '\003': /* ^C, interrupt */
					return -1;
					/* Fall through */
				case '\n':
				case '\r':
					return def;
				default:
					if (!addchar(c))
						XBell(_wd, 0);
					break;
				}
				break;
			
			case MapNotify:
				/* Could set the input focus if another
				   of our windows has it */
				break;
			
			default:
				_wdebug(3, "dialog: box ev %d", e.type);
				if (e.type == ButtonPress)
					XBell(_wd, 0);
				break;
			
			}
		}
		else {
			switch (e.type) {
			case ButtonPress:
			case KeyPress:
				_wdebug(3, "dialog: alien press %d", e.type);
				XBell(_wd, 0);
				break;
			case ButtonRelease:
			case MotionNotify:
				_wdebug(3, "dialog: alien move %d", e.type);
				break;
			default:
				_wdebug(3, "dialog: alien llev %d", e.type);
				_w_ll_event(&e);
				break;
			}
		}
	}
}

/* Find out which button a given Window ID belongs to (if any) */

static struct button *
whichbutton(w)
	Window w;
{
	int i;
	
	for (i= nbuttons; --i >= 0; ) {
		if (w == buttonlist[i].wd.wid)
			return &buttonlist[i];
	}
	return NULL;
}

/* Return the character code corresponding to a key event.
   Returns EOS if no ASCII character. */

static int
charcode(kep)
	XKeyEvent *kep;
{
	KeySym keysym;
	char strbuf[10];
	
	if (XLookupString(kep, strbuf, sizeof strbuf,
		&keysym, (XComposeStatus*)NULL) <= 0)
		return EOS;
	else
		return strbuf[0];
}

/* Append a character to the text item.
   Here it is that we assume that the text item is last.
   Unfortunately we can't use textedit (yet) to support all
   desirable functionality, but we do support backspace
   and overwriting the entire text. */

static bool
addchar(c)
	int c;
{
	struct button *bp= &buttonlist[nbuttons - 1];
	int i;
	
	if (bp->type != TEXT || bp->text == NULL)
		return FALSE;
	i= strlen(bp->text);
	if (c == '\b' || c == '\177' /*DEL*/ ) {
		if (i > 0) {
			bp->text[i-1]= EOS;
			bp->hilite= FALSE;
		}
	}
	else {
		if (bp->hilite) {
			i= 0;
			bp->hilite= FALSE;
		}
		bp->text= realloc(bp->text, (unsigned)(i+2));
		if (bp->text != NULL) {
			bp->text[i]= c;
			bp->text[i+1]= EOS;
		}
	}
	drawbutton(bp);
	return TRUE;
}

/* Process an event directed to a given button.
   This also updates the highlighting. */

static bool
buttonevent(bp, xep)
	struct button *bp;
	XEvent *xep;
{
	bool hit= FALSE;
	
	switch (xep->type) {
	
	case Expose:
		drawbutton(bp);
		return FALSE;
	
	case ButtonPress:
		if (bp->down == 0)
			bp->down= xep->xbutton.button;
		break;
	
	case ButtonRelease:
		if (bp->down == xep->xbutton.button) {
			hit= bp->inside;
			bp->down= 0;
		}
		break;
	
	case EnterNotify:
		bp->inside= TRUE;
		break;
	
	case LeaveNotify:
		bp->inside= FALSE;
		break;
	
	default:
		return FALSE;
	
	}
	hilitebutton(bp, bp->down > 0 && bp->inside);
	return hit;
}

/* Draw procedure to draw a command button or text item */

static void
drawbutton(bp)
	struct button *bp;
{
	char *text= bp->text == NULL ? "" : bp->text;
	int len= strlen(text);
	int width= XTextWidth(_wmf, text, len);
	int x= (bp->type == BUTTON) ? (bp->wd.width - width) / 2 : charwidth;
	int y= (bp->wd.height + _wmf->ascent - _wmf->descent) / 2;
	
	XClearWindow(_wd, bp->wd.wid);
	XDrawString(_wd, bp->wd.wid, b_gc, x, y, text, len);
	/* Indicate the default button with an underline */
	if (bp->def) {
		unsigned long ulpos, ulthick;
		if (!XGetFontProperty(_wmf, XA_UNDERLINE_POSITION, &ulpos))
			ulpos= _wmf->descent/2;
		if (!XGetFontProperty(_wmf, XA_UNDERLINE_THICKNESS,&ulthick)) {
			ulthick= _wmf->descent/3;
			CLIPMIN(ulthick, 1);
		}
		_winvert(bp->wd.wid, b_gc,
			x, (int)(y + ulpos), width, (int)ulthick);
	}
	if (bp->hilite)
		_winvert(bp->wd.wid, b_gc,
			0, 0, bp->wd.width, bp->wd.height);
}

/* Highlight or unhighlight a command button */

static void
hilitebutton(bp, hilite)
	struct button *bp;
	bool hilite;
{
	if (bp->hilite != hilite) {
		_winvert(bp->wd.wid, b_gc,
			0, 0, bp->wd.width, bp->wd.height);
		bp->hilite= hilite;
	}
}

/* Extract the text from the text item */

static void
gettext(buf, len)
	char *buf;
	int len;
{
	struct button *bp= &buttonlist[nbuttons - 1];
	
	if (bp->type != TEXT || bp->text == NULL)
		return;
	strncpy(buf, bp->text, len-1);
	buf[len-1]= EOS;
}

/* Destroy all buttons and associated data structures */

static void
killbuttons()
{
	int i;
	for (i= 0; i < nbuttons; ++i)
		free(buttonlist[i].text);
	L_SETSIZE(nbuttons, buttonlist, struct button, 0);
}
