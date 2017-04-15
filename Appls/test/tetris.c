/* Tetris.

   A simple but challenging game where pieces of different shapes
   falling with a constant speed must be manoeuvered to a final
   docking position.  Piece shapes are randomly chosen from all possible
   ways to connect four squares.  The manipulations allowed are moving
   the piece left or right and rotating it; every clock tick it moves
   down one step, until it cannot move further, or the player decides
   to "drop" it to earn more points.  The game is made more or less
   challenging by making the clock tick faster or slower.  A score is
   kept.  Points earned per piece depend on the height where it is
   dropped or stopped; extra points are awarded for choosing a higher
   speed ("level").  To allow the play to continue indefinitely,
   complete rows (i.e., horizontal rows where all squares
   are filled) are removed from the board and its contents above that
   row shifted down.
   
   The user interface uses mostly the left, right and up keys, for
   moving the piece left and right and rotating it.  For Unix adepts,
   'h'=left, 'k'=up and 'l'=right also work.  Space bar or Return
   drops the piece.  The Cancel key (Command-Period on the Mac,
   Control-C on most other systems) restarts the game; close the window
   or type 'q' to quit the game.

   The origin of the game appears to be in the East Block; I've heard
   of an original Macintosh version by a Hungarian programmer, but only
   seen a version for the Atari by Jan van der Steen at CWI.
   This version is hereby put in the public domain, but the package
   STDWIN used as portable window interface is copyrighted.  STDWIN
   is freely available from me, too.

   Guido van Rossum, CWI, Kruislaan 413, 1098 SJ Amsterdam,
   The Netherlands
   Internet e-mail address: guido@cwi.nl
   April 1989
*/

/* TO DO:
	- show next piece coming up
	- change chance distribution of pieces?
	- advanced level (what does it do?)
	- rethink level <--> delay relation
	
	- pause before starting the first game
	- improve "game over" behavior
	
	- display high score
	- score and status display next to the board
	- statistics
	
	- cute graphics
*/

/* Standard include files */

#include "stdwin.h"
#include <stdio.h>

/* Parametrizations */

/* Piece size.  It only makes sense to change this if you also
   change the initialization of the 'shapes' array.
   Since we rotate pieces, their max size must always be square. */ 
#define PSIZE 4

/* Game dimensions.  Traditionally it is played on a 10x20 board. */
#ifndef BWIDTH
#define BWIDTH 10
#endif
#ifndef BHEIGHT
#define BHEIGHT 20
#endif

/* Initial timer delay.  This affects initial difficulty and scoring.
   The current value is kept in variable 'delay'. */
#ifndef DELAY
#define DELAY 10
#endif

/* Individual 'square' sizes.
   These can be adjusted according to taste and the size of pixels
   on your screen.  (You can also fine-tune window and document size
   in main() below.)
   For the ALFA version of STDWIN, where pixel size == character size,
   a fixed size of 2x1 is used. */
#ifdef ALFA
#define SQWIDTH 2
#define SQHEIGHT 1
#define BLEFT 0
#define BTOP 0
#endif
#ifndef SQWIDTH
#define SQWIDTH 12
#endif
#ifndef SQHEIGHT
#define SQHEIGHT 12
#endif

/* Left, top of board image in window */
#ifndef BLEFT
#define BLEFT 0
#endif
#ifndef BTOP
#define BTOP 4
#endif

/* Some useful macros (predefined on some but not all systems) */

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Available shapes.
   Relnext is the offset to the next piece after rotation.
   The pieces are aligned with the bottom so their scoring values are
   comparable, and the delay before their start is minimal; they are
   centered horizontally so the random placement appears even.
   Remember C's aggregate initialization rules; the initializer below
   is completely bracketed, but trailing zeros are sometimes elided. */

struct shapedef {
	int relnext;
	char piece[PSIZE][PSIZE];
} shapes[] = {
	
	/* "Four in a row" (2 orientations) */
	
	{1,	{{0, 0, 0, 0},
		 {0, 0, 0, 0},
		 {0, 0, 0, 0},
		 {1, 1, 1, 1}}},
	
	{-1,	{{0, 1, 0, 0},
		 {0, 1, 0, 0},
		 {0, 1, 0, 0},
		 {0, 1, 0, 0}}},
	
	/* "L shape" (4 orientations) */
	
	{1,	{{0, 0, 0},
		 {0, 1, 0},
		 {0, 1, 0},
		 {0, 1, 1}}},
	
	{1,	{{0, 0, 0},
		 {0, 0, 0},
		 {0, 0, 1},
		 {1, 1, 1}}},
	
	{1,	{{0, 0, 0},
		 {0, 1, 1},
		 {0, 0, 1},
		 {0, 0, 1}}},
	
	{-3,	{{0, 0, 0},
		 {0, 0, 0},
		 {1, 1, 1},
		 {1, 0, 0}}},
	
	/* "Inverse L shape" (4 orientations) */
	
	{1,	{{0, 0, 0},
		 {0, 1, 1},
		 {0, 1, 0},
		 {0, 1, 0}}},
	
	{1,	{{0, 0, 0},
		 {0, 0, 0},
		 {1, 0, 0},
		 {1, 1, 1}}},
	
	{1,	{{0, 0, 0},
		 {0, 0, 1},
		 {0, 0, 1},
		 {0, 1, 1}}},
	
	{-3,	{{0, 0, 0},
		 {0, 0, 0},
		 {1, 1, 1},
		 {0, 0, 1}}},
	
	/* "Z shape" (2 orientations) */
	
	{1,	{{0, 0, 0},
		 {0, 0, 0},
		 {1, 1, 0},
		 {0, 1, 1}}},
	
	{-1,	{{0, 0, 0},
		 {0, 0, 1},
		 {0, 1, 1},
		 {0, 1, 0}}},
	
	/* "S shape" (2 orientations) */
	
	{1,	{{0, 0, 0},
		 {0, 1, 0},
		 {0, 1, 1},
		 {0, 0, 1}}},
	
	{-1,	{{0, 0, 0},
		 {0, 0, 0},
		 {0, 1, 1},
		 {1, 1, 0}}},
	
	/* "T shape" (4 orientations) */
	
	{1,	{{0, 0, 0},
		 {0, 0, 0},
		 {1, 1, 1},
		 {0, 1, 0}}},
	
	{1,	{{0, 0, 0},
		 {0, 1, 0},
		 {0, 1, 1},
		 {0, 1, 0}}},
	
	{1,	{{0, 0, 0},
		 {0, 0, 0},
		 {0, 1, 0},
		 {1, 1, 1}}},
	
	{-3,	{{0, 0, 0},
		 {0, 0, 1},
		 {0, 1, 1},
		 {0, 0, 1}}},
	
	/* "Block" (1 orientation) */
	
	{0,	{{0, 0, 0},
		 {0, 0, 0},
		 {0, 1, 1},
		 {0, 1, 1}}},

};

/* Global variables */

WINDOW *win;			/* The window where we do our drawing */
int delay;			/* Current delay */
				/* NB: level = MAX(0, DELAY-delay) */
char board[BHEIGHT][BWIDTH];	/* Contents of board, except current piece */
char (*piece)[PSIZE];		/* Piece currently being manoeuvered */
int pindex;			/* Index in the shape array of current piece */
int pleft, ptop;		/* Position of current piece */
long score;			/* Score of current game */

/* Generate an informative title from level and/or score.
   (By putting it in the title bar we don't need an info window.) */

settitle()
{
	char buf[100];
	int level = MAX(0, DELAY-delay);
	
	if (level == 0) {
		if (score == 0)
			strcpy(buf, "Tetris");
		else
			sprintf(buf, "Score %ld", score);
	}
	else {
		if (score == 0)
			sprintf(buf, "Level %d", level);
		else
			sprintf(buf, "Sc %ld Lv %d", score, level);
	}
	wsettitle(win, buf);
}

/* Erase a portion of the board on the screen.
   Call only within wbegin/enddrawing. */

eraseboard(ileft, itop, iright, ibottom)
	int ileft, itop, iright, ibottom;
{
	werase(BLEFT + ileft*SQWIDTH,  BTOP + itop*SQHEIGHT,
	       BLEFT + iright*SQWIDTH, BTOP + ibottom*SQHEIGHT);
}

/* Draw a portion of the board, and a border around it.
   Call only within wbegin/enddrawing.
   This may be called with out-of-range parameters.
   Draw those squares of the game that lie (partly) in the rectangle
   given by the parameters.  Assume the background is blank.
   This contains #ifdef'ed code for the alfa version of STDWIN,
   which only supports text output. */

drawboard(ileft, itop, iright, ibottom)
	int ileft, itop, iright, ibottom;
{
	int ih, iv;
	int h, v;
	int flag;
	
	ileft = MAX(0, ileft);
	itop = MAX(0, itop);
	iright = MIN(BWIDTH, iright);
	ibottom = MIN(BHEIGHT, ibottom);
	for (iv = itop, v = BTOP + iv*SQHEIGHT; iv < ibottom;
					++iv, v += SQHEIGHT) {
		for (ih = ileft, h = BLEFT + ih*SQWIDTH; ih < iright;
						++ih, h += SQWIDTH) {
			flag = board[iv][ih];
			if (!flag && pleft <= ih && ih < pleft+PSIZE &&
					ptop <= iv && iv < ptop+PSIZE)
				flag = piece[iv-ptop][ih-pleft];
			if (flag) {
#ifdef ALFA
				wdrawchar(h, v, '#');
#else
				wshade(h+1, v+1, h+SQWIDTH, v+SQHEIGHT, 50);
#endif
			}
		}
	}
#ifdef ALFA
	/* Draw markers at the right margin */
	wdrawchar(BLEFT + BWIDTH*SQWIDTH, BTOP, '|');
	wdrawchar(BLEFT + BWIDTH*SQWIDTH, BTOP + (BHEIGHT-1)*SQHEIGHT, '|');
#else
	/* Draw a box around the board */
	wdrawbox(BLEFT - 1, BTOP - 1,
		BLEFT + BWIDTH*SQWIDTH + 2, BTOP + BHEIGHT*SQHEIGHT + 2);
#endif
}

/* Erase and redraw part of the board.
   Unlike eraseboard and drawboard above, this includes calls to
   wbegin/enddrawing. */

redrawboard(ileft, itop, iright, ibottom)
	int ileft, itop, iright, ibottom;
{
	wbegindrawing(win);
	eraseboard(ileft, itop, iright, ibottom);
	drawboard(ileft, itop, iright, ibottom);
	wenddrawing(win);
}

/* Draw procedure, passed to STDWIN's wopen */

void
drawproc(win, left, top, right, bottom)
	WINDOW *win;
	int left, top, right, bottom;
{
	drawboard((left-BLEFT)/SQWIDTH, (top-BTOP)/SQHEIGHT,
		(right-BLEFT+SQWIDTH-1)/SQWIDTH,
		(bottom-BTOP+SQHEIGHT-1)/SQHEIGHT);
}

/* Check if the piece can be at (dh, dv).
   This is used to check for legal moves.
   No part of the piece can be on a filled spot in the board or
   be outside it, but it can stick out above the top. */

int
allowed(dh, dv)
	int dh, dv;
{
	int ih, iv;
	
	for (iv = 0; iv < PSIZE; ++iv) {
		for (ih = 0; ih < PSIZE; ++ih) {
			if (piece[iv][ih]) {
				if (ih+dh < 0 || ih+dh >= BWIDTH)
					return 0;
				if (iv+dv < 0)
					continue;
				if (iv+dv >= BHEIGHT)
					return 0;
				if (board[iv+dv][ih+dh])
					return 0;
			}
		}
	}
	return 1;
}

/* Return a random integer in the range [0..n-1] */

int
uniform(n)
	int n;
{
	return rand() % n;
}

/* Rotate the piece 90 degrees counterclockwise.  No drawing is done.
   The implementation is trivial: just take the "next" element from the
   shape array. */

leftrotate()
{
	pindex += shapes[pindex].relnext;
	piece = shapes[pindex].piece;
}

/* Move the piece by the given vector (dh, dv), if this is a legal move..
   Return 1 if moved, 0 if not (then no changes were made). */

int
moveby(dh, dv)
	int dh, dv;
{
	int ileft, itop, iright, ibottom;
	
	if (!allowed(pleft+dh, ptop+dv)) {
		return 0;
	}
	ileft   = pleft + MIN(dh, 0);
	itop    = ptop  + MIN(dv, 0);
	iright  = pleft + PSIZE + MAX(dh, 0);
	ibottom = ptop  + PSIZE + MAX(dv, 0);
	pleft += dh;
	ptop += dv;
	redrawboard(ileft, itop, iright, ibottom);
	return 1;
}

/* Rotate the piece n quarter left turns, if this is a legal move.
   Return 1 if moved, 0 if not (then no changes were made). */

int
rotateby(n)
	int n;
{
	int i;
	
	for (i = 0; i < n; ++i)
		leftrotate();
	if (!allowed(pleft, ptop)) {
		for (i = 0; i < 4-n; ++i)
			leftrotate();
		return 0;
	}
	redrawboard(pleft, ptop, (pleft+PSIZE), (ptop+PSIZE));
	return 1;
}


/* Trivial routines to implement the commands. */

left()
{
	(void) moveby(-1, 0);
}

right()
{
	(void) moveby(1, 0);
}

rot()
{
	(void) rotateby(1);
}

/* Generate a new piece.  Its initial position is just above the top of
   the board, so that a single move down will show its bottom row.
   (This is one reason why the pieces are aligned with the bottom in the
   'shapes' array.) */

generate()
{
	pindex = uniform(sizeof shapes / sizeof shapes[0]);
	
	piece = shapes[pindex].piece;
	pleft = (BWIDTH-PSIZE) / 2;
	ptop = -PSIZE;
}

/* Start a new game.
   Reset deley/level, score, board and title; generate a new piece.
   The game is not restarted immediately. */

reset()
{
	int ih, iv;
	
	wsettimer(win, 0);
	delay = DELAY;
	score = 0;
	for (iv = 0; iv < BHEIGHT; ++iv) {
		for (ih = 0; ih < BWIDTH; ++ih)
			board[iv][ih] = 0;
	}
	generate();
	redrawboard(0, 0, BWIDTH, BHEIGHT);
	settitle();
}

/* Remove any full rows found, shifting the board above down */

removerows()
{
	int ih, iv;
	int jv;
	
	for (iv = 0; iv < BHEIGHT; ++iv) {
		for (ih = 0; ih < BWIDTH; ++ih) {
			if (!board[iv][ih])
				goto next; /* Two-level continue */
		}
		for (jv = iv; jv > 0; --jv) {
			for (ih = 0; ih < BWIDTH; ++ih)
				board[jv][ih] = board[jv-1][ih];
		}
		for (ih = 0; ih < BWIDTH; ++ih)
			board[jv][ih] = 0;
		wscroll(win,
			BLEFT, BTOP,
			BLEFT + BWIDTH*SQWIDTH, BTOP + (iv+1)*SQHEIGHT,
			0, SQHEIGHT);
	next:	;
	}
}

/* Add the score for the current piece to the total score.
   The title is not regenerated; that is done later in finish(). */

addscore()
{
	int level = MAX(0, DELAY-delay);
	int height = MAX(0, BHEIGHT-ptop);
	
	score += height + 2*level /* *(advanced?2:1) */ ;
}

/* Finish a piece off by dropping it; score and generate a new one.
   Called by the user and from timer of the piece can't move down.
   This also contains a hack to detect the end of the game:
   if the new piece can't move one step, it is over. */

finish()
{
	int ih, iv;
	
	addscore();
	while (moveby(0, 1))
		;
	for (iv = 0; iv < PSIZE; ++iv) {
		for (ih = 0; ih < PSIZE; ++ih) {
			if (piece[iv][ih] && iv+ptop >= 0)
				board[iv+ptop][ih+pleft] = 1;
		}
	}
	removerows();
	generate();
	settitle();
	if (moveby(0, 1))
		wsettimer(win, delay);
	else {
#ifdef ALFA
		/* Alfa STDWIN's wmessage doesn't wait for an OK */
		char buffer[10];
		strcpy(buffer, "Game over");
		(void) waskstr("", buffer, strlen(buffer));
#else
		wmessage("Game over");
#endif
		reset();
	}
}

/* The clock has ticked.
   Try to move down; if it can't, finish it off and start a new one. */

timer()
{
	if (moveby(0, 1))
		wsettimer(win, delay);
	else
		finish();
}

/* Make the clock tick faster (increase level) */

faster()
{
	if (delay > 1)
		--delay;
	settitle();
}

/* Make the clock tick slower (decrease level) */

slower()
{
	++delay;
	settitle();
}

/* Quit the program.
   Note that wdone() MUST be called before a STDWIN application exits. */

quit()
{
	wdone();
	exit(0);
}

/* Main event loop.
   React on commands, ignoring illegal ones, and react to clock ticks.
   Call various routines to execute the commands.
   Never return; calls quit() to exit. */

mainloop()
{
	EVENT e;
	
	for (;;) {
		wgetevent(&e);
		
		switch (e.type) {
		
		case WE_TIMER:
			timer();
			break;
		
		case WE_CHAR:
			switch (e.u.character) {
			case '+':
				faster();
				break;
			case '-':
				slower();
				break;
			case 'g':
				timer();
				break;
			case ' ':
				finish();
				break;
			case 'h':
				left();
				break;
			case 'k':
			 	rot();
			 	break;
			case 'l':
				right();
				break;
			case 'q':
				quit();
				break;
			case 'r':
				reset();
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				delay = DELAY-(e.u.character-'0');
				delay = MAX(1, delay);
				settitle();
				break;
			}
			break;
		
		case WE_COMMAND:
			switch (e.u.command) {
			case WC_RETURN:
				timer();
				break;
			case WC_CANCEL:
				reset();
				break;
			case WC_CLOSE:
				quit();
				break;
			case WC_LEFT:
				left();
				break;
			case WC_UP:
				rot();
				break;
			case WC_RIGHT:
				right();
				break;
			}
			break;
		
		}
	}
	/*NOTREACHED*/
}

/* Add a menu, only used as a cheap way to display some help */

addhelpmenu()
{
	MENU *mp;
	
	mp = wmenucreate(1, "Help");
	wmenuadditem(mp, "g or return starts the game", -1);
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "h or left arrow moves left", -1);
	wmenuadditem(mp, "l or right arrow moves right", -1);
	wmenuadditem(mp, "k or up arrow rotates", -1);
	wmenuadditem(mp, "space drops", -1);
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "+/- increases/decreases level (speed)", -1);
	wmenuadditem(mp, "0-9 chooses level directly", -1);
	wmenuadditem(mp, "", -1);
	wmenuadditem(mp, "r or cancel restarts", -1);
	wmenuadditem(mp, "close or q quits the game", -1);
}

/* Main program.
   Initialize STDWIN, create the window and the help menu,
   reset the game and call 'mainloop()' to play it. */

main(argc, argv)
	int argc;
	char **argv;
{
	long t;
	
	time(&t);
	srand((short)t ^ (short)(t>>16));
	winitnew(&argc, &argv);
	wsetdefwinsize(BLEFT + BWIDTH*SQWIDTH + 1,
					BTOP + BHEIGHT*SQHEIGHT + 5);
	win = wopen("Tetris", drawproc);
	if (win == NULL) {
		printf("Can't create window\n");
		wdone();
		exit(1);
	}
	wsetdocsize(win,
		BLEFT + BWIDTH*SQWIDTH + 1, BTOP + BHEIGHT*SQHEIGHT + 1);
	addhelpmenu();
	reset();
	mainloop();
	/*NOTREACHED*/
}
