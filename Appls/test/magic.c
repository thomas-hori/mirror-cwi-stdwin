/* Magic -- tool to help editing magic squares */

#include "stdwin.h"

/* Arbitrary limitations */
#define MAXSIZE 25
#define DEFSIZE 5
#define FARAWAY 10000

/* Defining data */
int size;
char sq[MAXSIZE][MAXSIZE];

/* Derived data */
int rowsum[MAXSIZE];
int colsum[MAXSIZE];
int diagsum, secdiagsum;
int ok;

/* Input parameters */
int last;

/* Output parameters */
WINDOW *win;
int origleft, origtop;
int rowheight, colwidth;

evaluate()
{
	int row, col;
	int sum;
	
	diagsum = 0;
	secdiagsum = 0;
	for (row = 0; row < size; ++row) {
		diagsum += sq[row][row];
		secdiagsum += sq[row][size-1-row];
	}
	
	ok = (diagsum == secdiagsum) && (diagsum > 0);
	
	for (row = 0; row < size; ++row) {
		sum = 0;
		for (col = 0; col < size; ++col)
			sum += sq[row][col];
		rowsum[row] = sum;
		if (ok)
			ok = (sum == diagsum);
	}
	
	for (col = 0; col < size; ++col) {
		sum = 0;
		for (row = 0; row < size; ++row)
			sum += sq[row][col];
		colsum[col] = sum;
		if (ok)
			ok = (sum == diagsum);
	}
}

center(h, v, n)
	int h, v;
	int n;
{
	char buf[25];
	int width;
	
	if (n == 0)
		return;
	sprintf(buf, "%d", n);
	width = wtextwidth(buf, -1);
	wdrawtext(h + (colwidth - width)/2, v + wlineheight()/2, buf, -1);
}

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	int h, v;
	int row, col;
	
	v = origtop;
	for (row = 0; row < size; ++row) {
		h = origleft;
		wdrawline(h, v, h+size*colwidth, v);
		for (col = 0; col < size; ++col) {
			center(h, v, sq[row][col]);
			h += colwidth;
		}
		center(h+3, v, rowsum[row]);
		v += rowheight;
	}
	wdrawline(origleft, v, origleft + size*colwidth, v);
	
	center(origleft - colwidth, v, secdiagsum);
	
	h = origleft;
	for (col = 0; col < size; ++col) {
		wdrawline(h, origtop, h, v);
		center(h, v, colsum[col]);
		h += colwidth;
	}
	wdrawline(h, origtop, h, v);
	
	center(h+3, v, diagsum);
	
	wdrawbox(origleft-1, origtop-1, h+2, v+2);
	
	if (last > 0 && ok)
		wdrawbox(origleft-3, origtop-3, h+4, v+4);
}

reset(newsize)
	int newsize;
{
	int row, col;
	char buf[100];
	
	size = newsize;
	for (row = 0; row < size; ++row)
		for (col = 0; col < size; ++col)
			sq[row][col] = 0;
	
	evaluate();
	
	last = 0;
	
	sprintf(buf, "%dx%d Magic Square", size, size);
	wsettitle(win, buf);
	
	wsetdocsize(win,
			origleft + (size+1)*colwidth + 3,
			origtop + (size+1)*rowheight);
	
	wchange(win, 0, 0, FARAWAY, FARAWAY);
}

init()
{
	colwidth = wtextwidth(" 000 ", -1);
	rowheight = wlineheight() * 2;
	origleft = colwidth;
	origtop = rowheight;
}

click(h, v)
	int h, v;
{
	int row, col;
	int oldok;
	
	if (last >= size*size) {
		wfleep();
		return;
	}
	
	if (h < origleft || v < origtop)
		return;
	
	col = (h - origleft) / colwidth;
	row = (v - origtop) / rowheight;
	if (row >= size || col >= size)
		return;
	
	if (sq[row][col] != 0) {
		wfleep();
		return;
	}
	
	sq[row][col] = ++last;
	
	oldok = ok;
	evaluate();
	if (ok != oldok)
		wchange(win, 0, 0, FARAWAY, FARAWAY);
	else
		change(row, col);
}

change(row, col)
	int row, col;
{
	wchange(win,
		origleft + col*colwidth + 1, origtop + row*rowheight + 1,
		origleft + (col+1)*colwidth, origtop + (row+1)*rowheight);
	wchange(win, 0, origtop + size*rowheight + 2, FARAWAY, FARAWAY);
	wchange(win, origleft + size*colwidth + 2, 0, FARAWAY, FARAWAY);
}

undo()
{
	int row, col;
	int oldok;
	
	if (last == 0) {
		wfleep();
		return;
	}
	
	for (row = 0; row < size; ++row) {
		for (col = 0; col < size; ++col) {
			if (sq[row][col] == last) {
				sq[row][col] = 0;
				--last;
				oldok = ok;
				evaluate();
				if (ok != oldok)
					wchange(win, 0, 0, FARAWAY, FARAWAY);
				else
					change(row, col);
				return;
			}
		}
	}
	/* Shouldn't get here */
	wfleep(); wfleep();
}

main(argc, argv)
	int argc;
	char **argv;
{
	EVENT e;
	
	winitargs(&argc, &argv);
	init();
	wsetdefwinsize(origleft + (DEFSIZE+2)*colwidth,
			origtop + (DEFSIZE+2)*rowheight);
	win = wopen("Magic Square", drawproc);
	reset(DEFSIZE);
	
	for (;;) {
		wgetevent(&e);
		switch (e.type) {
		case WE_MOUSE_UP:
			click(e.u.where.h, e.u.where.v);
			break;
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
				wdone();
				exit(0);
			case WC_CANCEL:
				reset(size);
				break;
			case WC_BACKSPACE:
				undo();
				break;
			}
			break;
		case WE_CLOSE:
			wdone();
			exit(0);
		case WE_CHAR:
			if (e.u.character >= '1' && e.u.character <= '9')
				reset(e.u.character - '0');
			else if (e.u.character == '0')
				reset(size);
			else if (e.u.character == 'q') {
				wdone();
				exit(0);
			}
			else
				wfleep();
			break;
		}
	}
}
