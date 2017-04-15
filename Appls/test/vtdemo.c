#include "stdwin.h"
#include "vt.h"

main() {
	VT *vt;
	winit();
	wsetdefwinsize(80*wcharwidth('0'), 24*wlineheight());
	vt= vtopen("VT", 24, 80, 100); /* Should check outcome */
	vtautosize(vt);
	vtsetnlcr(vt, 1); /* Map LF ro CR LF */
	vtansiputs(vt, "Hello, world\n", -1);
	eventloop();
	wdone();
	exit(0);
}

eventloop(vt) VT *vt; {
	for (;;) {
		EVENT e;
		wgetevent(&e);
		switch (e.type) {
		case WE_SIZE:
			vtautosize(vt); /* Should check outcome */
			break;
		case WE_CHAR:
			{ char buf[2];
			  buf[0]= e.u.character;
			  vtansiputs(vt, buf, 1);
			  break; }
		case WE_MOUSE_DOWN:
			vtsetcursor(vt,
			            e.u.where.v / vtcheight(vt),
			            e.u.where.h / vtcwidth(vt));
			break;
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:                              return;
			case WC_RETURN: vtansiputs(vt, "\n", 1);    break;
			case WC_BACKSPACE: vtansiputs(vt, "\b", 1); break;
			case WC_TAB: vtansiputs(vt, "\t", 1);       break;
			}
			break;
		case WE_CLOSE:
			return;
		}
	}
}
