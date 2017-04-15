/* X11 STDWIN -- caret operations */

#include "x11.h"

void
wsetcaret(win, h, v)
	WINDOW *win;
	int h, v;
{
	_whidecaret(win);
	if (h < 0 || v < 0)
		h= v= -1;
	win->careth= h;
	win->caretv= v;
	_wshowcaret(win);
}

void
wnocaret(win)
	WINDOW *win;
{
	_whidecaret(win);
	win->careth= win->caretv= -1;
}

_wshowcaret(win)
	WINDOW *win;
{
	if (!win->caretshown && win->careth >= 0 && win->caretv >= 0) {
		wbegindrawing(win);
		_winvertcaret(win);
		wenddrawing(win);
		win->caretshown= TRUE;
	}
}

_whidecaret(win)
	WINDOW *win;
{
	if (win->caretshown) {
		win->caretshown= FALSE;
		wbegindrawing(win);
		_winvertcaret(win);
		wenddrawing(win);
	}
}

_winvertcaret(win)
	WINDOW *win;
{
	/* Call this between w{begin,end}drawing only! */
	
	int left= win->careth, top= win->caretv;
	int right= left+1, bottom= top + wlineheight();
	winvert(left, top, right, bottom);
}
