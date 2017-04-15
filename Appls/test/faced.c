/* Simple bitmap editor.
   Uses ".face" file format (48x48).
   
   TO DO:
   	- fix row/col confusion
	- connect dots given by mouse move?
	- optimize drawing of consequtive black squares
	- add open... command
	- add flexible input format
	- support X bitmap format
	*/

#include <stdio.h>
#include "stdwin.h"

#define ROWS 48
#define COLS 48
#define SIZE (ROWS < COLS ? ROWS : COLS)

#define BITS 16 /* Bits per word */

char bit[ROWS][COLS];

int xscale= 5;
int yscale= 5;

WINDOW *win;

int changed= 0;


main(argc, argv)
	int argc;
	char **argv;
{
	FILE *fp;
	
	winitargs(&argc, &argv);
	
	if (argc > 1) {
		fp= fopen(argv[1], "r");
		if (fp == NULL) {
			wdone();
			perror(argv[1]);
			exit(1);
		}
		input(fp);
		fclose(fp);
	}
	
	setup();
	

	for (;;) {
		editing();
		
		if (changed) {
			switch (waskync("Save changes?", 1)) {
			case 1:
				if (!save(argc > 1 ? argv[1] : NULL))
					continue;
				break;
			case -1:
				continue;
			}
		}
		break; /* Out of loop */
	}
	
	wdone();
	exit(0);
}

save(name)
	char *name;
{
	FILE *fp;
	char namebuf[256];
	
	namebuf[0]= '\0';
	if (name == NULL) {
		if (!waskfile("Save as", namebuf, sizeof namebuf, 1))
			return 0;
		name= namebuf;
	}
	fp= fopen(name, "w");
	if (fp == NULL) {
		wperror(name);
		return 0;
	}
	output(fp);
	fclose(fp);
	return 1;
}

#define MAIN_MENU	1

#define QUIT_ITEM	0

#define OP_MENU		2

#define CLEAR_ITEM	0
#define SET_ITEM	1
#define INVERT_ITEM	2
#define TRANS_MAJ_ITEM	3
#define TRANS_MIN_ITEM	4
#define ROT_LEFT_ITEM	5
#define ROT_RIGHT_ITEM	6
#define FLIP_HOR_ITEM	7
#define FLIP_VERT_ITEM	8

#define NOPS		9

extern int (*oplist[])(); /* Forward */
drawproc(); /* Forward */

int text_only;

setup()
{
	MENU *mp;
	int width, height;
	
	if (wlineheight() == 1)
		text_only= xscale= yscale= 1;
	
	wsetdefwinsize(COLS*xscale, ROWS*yscale);
	
	mp= wmenucreate(MAIN_MENU, "Faced");
	wmenuadditem(mp, "Quit", 'Q');
	mp= wmenucreate(OP_MENU, "Operations");
	wmenuadditem(mp, "Clear all", -1);
	wmenuadditem(mp, "Set all", -1);
	wmenuadditem(mp, "Invert", -1);
	wmenuadditem(mp, "Transpose major", -1);
	wmenuadditem(mp, "Transpose minor", -1);
	wmenuadditem(mp, "Rotate left", -1);
	wmenuadditem(mp, "Rotate right", -1);
	wmenuadditem(mp, "Flip horizontal", -1);
	wmenuadditem(mp, "Flip vertical", -1);
	
	win= wopen("faced", drawproc);
	if (win == NULL) {
		wmessage("Can't open window");
		return;
	}
	wsetdocsize(win, COLS*xscale, ROWS*yscale);
}

editing()
{
	int value= 0;
	
	for (;;) {
		EVENT e;
		
		wgetevent(&e);
		switch (e.type) {
		case WE_MOUSE_DOWN:
			value= !getbit(e.u.where.h/xscale, e.u.where.v/yscale);
		case WE_MOUSE_MOVE:
		case WE_MOUSE_UP:
			setbit(e.u.where.h/xscale, e.u.where.v/yscale, value);
			break;
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_CLOSE:
				return;
			case WC_CANCEL:
				changed= 0;
				return;
			}
		case WE_CLOSE:
			return;
		case WE_MENU:
			switch (e.u.m.id) {
			case MAIN_MENU:
				switch (e.u.m.item) {
				case QUIT_ITEM:
					return;
				}
				break;
			case OP_MENU:
				if (e.u.m.item >= 0 && e.u.m.item < NOPS) {
					(*oplist[e.u.m.item])();
					wchange(win, 0, 0,
						COLS*xscale, ROWS*yscale);
					changed= 1;
				}
				break;
			}
		}
	}
}

int
getbit(col, row)
{
	if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
		return bit[row][col];
	else
		return 0;
}

setbit(col, row, value)
{
	if (row >= 0 && row < ROWS && col >= 0 && col < COLS &&
			bit[row][col] != value) {
		changed= 1;
		bit[row][col]= value;
		wchange(win, col*xscale, row*yscale,
			(col+1)*xscale, (row+1)*yscale);
	}
}

drawproc(win, left, top, right, bottom)
	WINDOW *win;
{
	int row, col;
	
	if (left < 0)
		left= 0;
	if (right > COLS*xscale)
		right= COLS*xscale;
	if (top < 0)
		top= 0;
	if (bottom > ROWS*yscale)
		bottom= ROWS*yscale;
	
	for (row= top/yscale; row*yscale < bottom; ++row) {
		for (col= left/xscale; col*xscale < right; ++col) {
			if (bit[row][col]) {
				if (text_only)
					wdrawchar(col, row, 'x');
				else
					wpaint(col*xscale, row*yscale,
					    (col+1)*xscale, (row+1)*yscale);
			}
		}
	}
}


/* File I/O routines */

input(fp)
	FILE *fp;
{
	int row;
	
	for (row= 0; row < ROWS; ++row) {
		read_row(fp, row);
	}
}

read_row(fp, row)
	FILE *fp;
	int row;
{
	int left= 0;
	long ibuf;
	int col;
	
	for (col= 0; col < COLS; ++col) {
		if (left <= 0) {
			if (fscanf(fp, " 0x%4lx,", &ibuf) != 1) {
				wdone();
				fprintf(stderr,
					"Bad input format, row %d\n", row);
				exit(1);
			}
			left= BITS;
		}
		--left;
		bit[row][col]= (ibuf >> left) & 1;
	}
}

output(fp)
	FILE *fp;
{
	int row;
	
	for (row= 0; row < ROWS; ++row)
		write_row(fp, row);
}

write_row(fp, row)
	FILE *fp;
	int row;
{
	int col;
	int left= BITS;
	long ibuf= 0;
	
	for (col= 0; col < COLS; ++col) {
		if (left <= 0) {
			fprintf(fp, "0x%04lX,", ibuf);
			ibuf= 0;
			left= BITS;
		}
		--left;
		if (bit[row][col]) {
			ibuf |= 1<<left;
		}
	}
	if (left < BITS)
		fprintf(fp, "0x%04lX,", ibuf);
	fprintf(fp, "\n");
}


/* Data manipulation routines */

clear_bits()
{
	int row, col;
	
	for (row= 0; row < ROWS; ++row)
		for (col= 0; col < COLS; ++col)
			bit[row][col]= 0;
}

set_bits()
{
	int row, col;
	
	for (row= 0; row < ROWS; ++row)
		for (col= 0; col < COLS; ++col)
			bit[row][col]= 1;
}

invert_bits()
{
	int row, col;
	
	for (row= 0; row < ROWS; ++row)
		for (col= 0; col < COLS; ++col)
			bit[row][col]= !bit[row][col];
}

transpose_major()
{
	int row, col;
	char tmp;
	
	for (row= 0; row < SIZE; ++row) {
		for (col= 0; col < row; ++col) {
			tmp= bit[row][col];
			bit[row][col]= bit[col][row];
			bit[col][row]= tmp;
		}
	}
}

transpose_minor()
{
	int row, col;
	char tmp;
	
	for (row= 0; row < SIZE; ++row) {
		for (col= 0; col < SIZE-1-row; ++col) {
			tmp= bit[row][col];
			bit[row][col]= bit[SIZE-1-col][SIZE-1-row];
			bit[SIZE-1-col][SIZE-1-row]= tmp;
		}
	}
}

rotate_left()
{
	int row, col;
	char tmp;
	
	for (row= 0; row < SIZE/2; ++row) {
		for (col= 0; col < SIZE/2; ++col) {
			tmp= bit[row][col];
			bit[row][col]= bit[col][SIZE-1-row];
			bit[col][SIZE-1-row]= bit[SIZE-1-row][SIZE-1-col];
			bit[SIZE-1-row][SIZE-1-col]= bit[SIZE-1-col][row];
			bit[SIZE-1-col][row]= tmp;
		}
	}
}

rotate_right()
{
	int row, col;
	char tmp;
	
	for (row= 0; row < SIZE/2; ++row) {
		for (col= 0; col < SIZE/2; ++col) {
			tmp= bit[row][col];
			bit[row][col]= bit[SIZE-1-col][row];
			bit[SIZE-1-col][row]= bit[SIZE-1-row][SIZE-1-col];
			bit[SIZE-1-row][SIZE-1-col]= bit[col][SIZE-1-row];
			bit[col][SIZE-1-row]= tmp;
		}
	}
}

flip_horizontal()
{
	int row, col;
	char tmp;
	
	for (row= 0; row < ROWS; ++row) {
		for (col= 0; col < COLS/2; ++col) {
			tmp= bit[row][col];
			bit[row][col]= bit[row][COLS-1-col];
			bit[row][COLS-1-col]= tmp;
		}
	}
}

flip_vertical()
{
	int row, col;
	char tmp;
	
	for (row= 0; row < ROWS/2; ++row) {
		for (col= 0; col < COLS; ++col) {
			tmp= bit[row][col];
			bit[row][col]= bit[ROWS-1-row][col];
			bit[ROWS-1-row][col]= tmp;
		}
	}
}

int (*oplist[])() = {
	clear_bits,
	set_bits,
	invert_bits,
	transpose_major,
	transpose_minor,
	rotate_left,
	rotate_right,
	flip_horizontal,
	flip_vertical,
};
