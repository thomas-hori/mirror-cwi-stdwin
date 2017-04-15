/* Here's a very simple bitmap editor.
   A window shows a magnified 64x64 bitmap
   which you can edit by clicking the mouse.
   Clicking on a bit that's off turns it on,
   clicking on one that's on turns it off.
   Drag the mouse to set or clear more bits.
   Additional facilities like saving to a file
   are left as exercises for the reader. */

#include "stdwin.h"

/* Bitmap dimensions */

#define WIDTH 64
#define HEIGHT 64

/* The bitmap */

char bit[WIDTH][HEIGHT];

/* Magnification factors.
   (Should really be calculated from screen dimensions). */

int xscale= 5;
int yscale= 5;

/* The window */

WINDOW *win;

/* Main program */

main(argc, argv)
	int argc;
	char **argv;
{
	winitargs(&argc, &argv);
	setup();
	mainloop();
	wdone();
	exit(0);
}

/* Create the window */

setup()
{
	extern void drawproc(); /* Forward */
	
	wsetdefwinsize(WIDTH*xscale, HEIGHT*yscale);
	win= wopen("Bitmap Editor Demo", drawproc);
	wsetdocsize(win, WIDTH*xscale, HEIGHT*yscale);
}

/* Main event loop */

mainloop()
{
	int value= 0;
	
	for (;;) {
		EVENT e;
		
		wgetevent(&e);
		
		switch (e.type) {
		
		case WE_MOUSE_DOWN:
			value= !getbit(e.u.where.h/xscale, e.u.where.v/yscale);
			/* Fall through to next cases */
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			setbit(e.u.where.h/xscale, e.u.where.v/yscale, value);
			break;
		
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
			case WC_CANCEL:
				return;
			}
		
		case WE_CLOSE:
			return;
		
		}
	}
}

/* Draw procedure */

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
#ifdef UNOPTIMIZED
	int h, v;

	for (v= top/yscale; v*yscale < bottom; ++v) {
		for (h= left/xscale; h*xscale < right; ++h) {
			if (getbit(h, v))
				wpaint(h*xscale, v*yscale,
					(h+1)*xscale, (v+1)*yscale);
		}
	}
#else
	int h, v;
	int start;

	for (v= top/yscale; v*yscale < bottom; ++v) {
		start = -1;
		for (h= left/xscale; h*xscale < right; ++h) {
			if (getbit(h, v)) {
				if (start == -1)
					start = h;
			} else {
				if (start != -1) {
					wpaint(start*xscale, v*yscale,
						h*xscale, (v+1)*yscale);
					start = -1;
				}
			}
		}
		if (start != -1)
			wpaint(start*xscale, v*yscale, h*xscale, (v+1)*yscale);
	}
#endif
}

/* Macro to test for bit coordinates in range */

#define INRANGE(h, v) ((v) >= 0 && (v) < HEIGHT && (h) >= 0 && (h) < WIDTH)

/* Return a bit's value; return 0 if outside range */

int
getbit(h, v)
{
	if (INRANGE(h, v))
		return bit[h][v];
	else
		return 0;
}

/* Change a bit's value (if in range) and arrange for it to be drawn. */

setbit(h, v, value)
{
	if (INRANGE(h, v) && bit[h][v] != value) {
		bit[h][v]= value;
		wchange(win, h*xscale, v*yscale, (h+1)*xscale, (v+1)*yscale);
	}
}
