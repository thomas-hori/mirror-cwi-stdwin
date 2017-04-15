/*
 * Virtual TeRMinal package.
 * (For a description see at the end of this file.)
 *
 * AMOEBA TTY DRIVER VERSION
 *
 * TO DO:
 *	- properly implement trminterrupt
 */

/*
 * Includes and data definitions.
 */

/* From ~amoeba/h: */
#include "amoeba.h"
#include "stdio.h"

/* From /usr/include: */
#include <ctype.h>

/* From ../h: */
#include "vtrm.h"

/* From libc: */
char *malloc();
char *getenv();

/* From libtermcap: */
int tgetent();
int tgetnum();
int tgetflag();
char *tgetstr();
char *tgoto();

#ifdef TRACE
#define Tprintf(args) fprintf args
#else
#define Tprintf(args) /*empty*/
#endif

#ifdef lint
#define VOID (void)
#else
#define VOID
#endif

#define Forward
#define Visible
#define Hidden static
#define Procedure

typedef char *string;
typedef int bool;
#define Yes 1
#define No  0

#define Min(a,b) ((a) <= (b) ? (a) : (b))

/* Variables needed by termcap's tgoto() or tputs(): */
char PC;
char *BC;
char *UP;
short ospeed;

Forward int outchar(); 		/* procedure for termcap's tputs */
#define Putstr(str)	tputs((str), 1, outchar)

/* termcap terminal capabilities */

Hidden int lines;
Hidden int cols;

/*
 * String-valued capabilities are saved in one big array.
 * Extend this only at the end (even though it disturbs the sorting)
 * because otherwise you have to change all macros...
 */

#define par_al_str strcaps[0]	/* parametrized al (AL) */
#define cap_cm_str strcaps[1]	/* screen-relative cursor motion (CM) */
#define par_dl_str strcaps[2]	/* parametrized dl (DL) */
#define al_str strcaps[3] 	/* add new blank line */
#define cd_str strcaps[4] 	/* clear to end of display */
#define ce_str strcaps[5] 	/* clear to end of line */
#define cl_str strcaps[6] 	/* cursor home and clear screen */
#define cm_str strcaps[7] 	/* cursor motion */
#define cp_str strcaps[8]	/* cursor position sense reply */
#define cr_str strcaps[9] 	/* carriage return */
#define cs_str strcaps[10] 	/* change scrolling region */
#define dc_str strcaps[11] 	/* delete character */
#define dl_str strcaps[12] 	/* delete line */
#define dm_str strcaps[13] 	/* enter delete mode */
#define do_str strcaps[14] 	/* cursor down one line */
#define ed_str strcaps[15] 	/* end delete mode */
#define ei_str strcaps[16] 	/* end insert mode */
#define ho_str strcaps[17]	/* cursor home */
#define ic_str strcaps[18] 	/* insert character (if necessary; may pad) */
#define im_str strcaps[19] 	/* enter insert mode */
#define nd_str strcaps[20] 	/* cursor right (non-destructive space) */
#define nl_str strcaps[21]	/* newline */
#define se_str strcaps[22] 	/* end standout mode */
#define sf_str strcaps[23] 	/* scroll text up (from bottom of region) */
#define so_str strcaps[24] 	/* begin standout mode */
#define sp_str strcaps[25]	/* sense cursor position */
#define sr_str strcaps[26] 	/* scroll text down (from top of region) */
#define te_str strcaps[27] 	/* end termcap */
#define ti_str strcaps[28] 	/* start termcap */
#define vb_str strcaps[29] 	/* visible bell */
#define ve_str strcaps[30] 	/* make cursor visible again */
#define vi_str strcaps[31] 	/* make cursor invisible */
#define le_str strcaps[32] 	/* cursor left */
#define bc_str strcaps[33]	/* backspace character */
#define up_str strcaps[34] 	/* cursor up */
#define pc_str strcaps[35]	/* pad character */
#define ks_str strcaps[36]	/* keypad mode start */
#define ke_str strcaps[37]	/* keypad mode end */
/* Insert new entries here only! Don't forget to change the next line! */
#define NSTRCAPS 38 /* One more than the last entry's index */

Hidden char *strcaps[NSTRCAPS];
Hidden char strcapnames[] =
"ALCMDLalcdceclcmcpcrcsdcdldmdoedeihoicimndnlsesfsospsrtetivbvevilebcuppckske";

/* Same for Boolean-valued capabilities */

#define has_am flagcaps[0]	/* has automatic margins */
#define has_da flagcaps[1]	/* display may be retained above screen */
#define has_db flagcaps[2]	/* display may be retained below screen */
#define has_in flagcaps[3]	/* not save to have null chars on the screen */
#define has_mi flagcaps[4]	/* move safely in insert (and delete?) mode */
#define has_ms flagcaps[5]	/* move safely in standout mode */
#define has_xs flagcaps[6]	/* standout not erased by overwriting */
#define has_bs flagcaps[7]	/* terminal can backspace */
#define hardcopy flagcaps[8]	/* hardcopy terminal */
#define NFLAGS 9

Hidden char flagcaps[NFLAGS];
Hidden char flagnames[]= "amdadbinmimsxsbshc";

Hidden Procedure
getcaps(parea)
	register char **parea;
{
	register char *capname;
	register char **capvar;
	register char *flagvar;

	for (capname= flagnames, flagvar= flagcaps;
			*capname != '\0'; capname += 2, ++flagvar)
		*flagvar= tgetflag(capname);

	for (capname= strcapnames, capvar= strcaps;
			*capname != '\0'; capname += 2, ++capvar)
		*capvar= tgetstr(capname, parea);
}

/* terminal status */

/* calling order of Visible Procs */
Hidden bool started = No;

/* to exports the capabilities mentioned in vtrm.h: */
Hidden int flags = 0;

/* cost for impossible operations */
#define Infinity 9999
	/* Allow for adding Infinity+Infinity within range */
	/* (Range is assumed at least 2**15 - 1) */

/* The following for all sorts of undefined things (except for UNKNOWN char) */
#define Undefined (-1)

/* current mode of putting char's */
#define Normal	0
#define Insert	1
#define	Delete	2
Hidden short mode = Normal;

/* current standout mode */
#define Off	0
#define On	0200
Hidden short so_mode = Off;

/* masks for char's and short's */
#define NULCHAR	'\000'
#define CHAR	0177
#define SOBIT	On
#define SOCHAR	0377
/* if (has_xs) record cookies placed on screen in extra bit */
/* type of cookie is determined by the SO bit */
#define XSBIT	0400
#define SOCOOK	0600
#define COOKBITS SOCOOK
#define UNKNOWN	1
#define NOCOOK	UNKNOWN

/* current cursor position */
Hidden short cur_y = Undefined, cur_x = Undefined;

/* "line[y][x]" holds the char on the terminal, with the SOBIT and XSBIT.
 * the SOBIT tells whether the character is standing out, the XSBIT whether
 * there is a cookie on the screen at this position.
 * In particular a standend-cookie may be recorded AFTER the line
 * (just in case some trmputdata will write after that position).
 * "lenline[y]" holds the length of the line.
 * Unknown chars will be 1, so the optimising compare in putline will fail.
 * (Partially) empty lines are distinghuished by "lenline[y] < cols".
 */
Hidden short **line = 0, *lenline = 0;

/* Clear the screen initially iff only memory cursor addressing available */
Hidden bool mustclear = No;

/* Make the cursor invisible when trmsync() tries to move outside the screen */
Hidden bool no_cursor = No;

/* Optimise cursor motion */
Hidden int abs_cost; 		/* cost of absolute cursor motion */
Hidden int cr_cost; 		/* cost of carriage return */
Hidden int do_cost; 		/* cost of down */
Hidden int le_cost; 		/* cost of left */
Hidden int nd_cost; 		/* cost of right */
Hidden int up_cost; 		/* cost of up */

/* Optimise trailing match in put_line, iff the terminal can insert and delete
 * characters; the cost per n characters will be:
 * 	n * MultiplyFactor + OverHead
 */
Hidden int ins_mf, ins_oh, del_mf, del_oh;
Hidden int ed_cost, ei_cost; 		/* used in move() */

/* The type of scrolling possible determines which routines get used;
 * these may be:
 * (1) with addline and deleteline (termcap: al_str & dl_str);
 * (2) with a settable scrolling region, like VT100 (cs_str, sr_str, sf_str);
 * (3) no scrolling available. (NOT YET IMPLEMENTED)
 */
Hidden Procedure (*scr_up)();
Hidden Procedure (*scr_down)();
Forward Procedure scr1up();
Forward Procedure scr1down();
Forward Procedure scr2up();
Forward Procedure scr2down();
/*Forward Procedure scr3up(); */
/*Forward Procedure scr3down(); */

/*
 * Starting, Ending and (fatal) Error.
 */

/* 
 * Initialization call.
 * Determine terminal capabilities from termcap.
 * Set up tty modes.
 * Start up terminal and internal administration.
 * Return 0 if all well, error code if in trouble.
 */
Visible int
trmstart(plines, pcols, pflags)
int *plines;
int *pcols;
int *pflags;
{
	register int err;
	
	Tprintf((stderr, "\ttrmstart(&li, &co, &fl);\n"));
	if (started) {
		Tprintf((stderr, "\terr=TE_TWICE\n"));
		return TE_TWICE;
	}
	Tprintf((stderr, "\tgettermcaps: "));
	err= gettermcaps();
	Tprintf((stderr, "\terr=%d\n", err));
	if (err != TE_OK)
		return err;
	Tprintf((stderr, "\tsetttymode: "));
	err= setttymode();
	Tprintf((stderr, "\terr=%d\n", err));
	if (err != TE_OK)
		return err;
	Tprintf((stderr, "\tstart_trm: "));
	err= start_trm();
	Tprintf((stderr, "\terr=%d\n", err));
	if (err != TE_OK) {
		trmend();
		return err;
	}

	*plines = lines;
	*pcols = cols;
	*pflags = flags;

	started = Yes;
	Tprintf((stderr, "\ttrmstart: TE_OK\n"));
	return TE_OK;
}

/*
 * Termination call.
 * Reset tty modes, etc.
 * Beware that it might be called by a caught interrupt even in the middle
 * of trmstart()!
 */
Visible Procedure
trmend()
{
	Tprintf((stderr, "\ttrmend();\n"));
	set_mode(Normal);
	if (so_mode != Off)
		standend();
	Putstr(ke_str);
	Putstr(te_str);
	VOID fflush(stdout);
	resetttymode();

	started = No;
}

/*
 * Set all internal statuses to undefined, especially the contents of
 * the screen, so a hard redraw will not be optimised to heaven.
 */
Visible Procedure
trmundefined()
{
	register int y, x;
	Tprintf((stderr, "\ttrmundefined();\n"));

	cur_y = cur_x = Undefined;
	mode = so_mode = Undefined;
	
	for (y = 0; y < lines; y++) {
		for (x = 0; x <= cols; x++)
			line[y][x] = 1; /* impossible char, no so bits */
		lenline[y] = cols;
	}
}

#ifndef NDEBUG
/*ARGSUSED*/
Hidden Procedure
check_started(m)
	char *m;
{
	if (!started) {
		trmend();
		fprintf(stderr, "bad VTRM call\n");
		abort();
	}
}
#else
#define check_started(m) /*empty*/
#endif

Hidden int ccc;

/*ARGSUSED*/
Hidden Procedure
countchar(ch)
char ch;
{
	ccc++;
}

Hidden int
strcost(str)
char *str;
{
	if (str == NULL)
		return Infinity;
	return str0cost(str);
}

Hidden int
str0cost(str)
char *str;
{
	ccc = 0;
	tputs(str, 1, countchar);
	return ccc;
}

/*
 * Get terminal capabilities from termcap and compute related static
 * properties.  Return TE_OK if all well, error code otherwise.
 */

Hidden int
gettermcaps() 
{
	string trmname;
	static char tc_buf[1024]; /* Static so main program can get keydefs */
	static char strbuf[1024]; /* Buffer where our caps are saved */
	char *area = strbuf;
	int sg;
	static bool tc_initialized = No;

	Tprintf((stderr, "\tgettermcaps\n"));
	if (tc_initialized)
		return TE_OK;
	
	Tprintf((stderr, "\tgetenv: "));
	trmname=getenv("TERM");
	if (trmname == NULL || trmname[0] == '\0')
		return TE_NOTERM;
	Tprintf((stderr, "\ttgetent: "));
	if (tgetent(tc_buf, trmname) != 1)
		return TE_BADTERM;
	
	Tprintf((stderr, "\tgetcaps: "));
	getcaps(&area); /* Read all flag and string type capabilities */
	if (hardcopy)
		return TE_DUMB;
	BC = le_str;
	if (BC == NULL) {
		BC = bc_str;
		if (BC == NULL) {
			if (has_bs)
				BC = "\b";
			else
				return TE_DUMB;
		}
	}
	UP = up_str;
	if (UP == NULL)
		return TE_DUMB;
	PC = (pc_str != NULL? pc_str[0] : NULCHAR);

	if (cm_str == NULL) {
		cm_str = cap_cm_str;
		if (cm_str == NULL) {
			if (ho_str == NULL || do_str == NULL || nd_str == NULL)
				return TE_DUMB;
		}
		else
			mustclear = Yes;
	}
	if (al_str && dl_str) {
		scr_up = scr1up;
		scr_down = scr1down;
		flags |= CAN_SCROLL;
	}
	else {
		if (sf_str == NULL)
			sf_str = "\n";
		if (cs_str && sr_str) {
			scr_up = scr2up;
			scr_down = scr2down;
			flags |= CAN_SCROLL;
		}
		else
			return TE_DUMB;
	}
	
	Tprintf((stderr, "tgetnum: "));
	lines = tgetnum("li");
	cols = tgetnum("co");
	if (lines <= 0) lines = 24;
	if (cols <= 0) cols = 80;
	
	if ((sg=tgetnum("sg")) == 0)
		has_xs = Yes;
	else if (sg > 0)
		return TE_DUMB;
	
	if (!ce_str)
		return TE_DUMB;
	if (cr_str == NULL) cr_str = "\r";
	if (do_str == NULL) {
		do_str = nl_str;
		if (do_str == NULL) do_str = "\n";
	}
	le_str = BC;
	up_str = UP;
	if (vb_str == NULL) 	/* then we will do with the audible bell */
		vb_str = "\007";
	
	/* cursor sensing (non standard) */
	if (cp_str != NULL && sp_str != NULL)
		flags |= CAN_SENSE;

	if (so_str != NULL && se_str != NULL)
		flags |= HAS_STANDOUT;

	/* calculate costs of local and absolute cursor motions */
	Tprintf((stderr, "strcost: "));
	if (cm_str == NULL)
		abs_cost = Infinity;
	else
		abs_cost = strcost(tgoto(cm_str, 0, 0));
	cr_cost = strcost(cr_str);
	do_cost = strcost(do_str);
	le_cost = strcost(le_str);
	nd_cost = strcost(nd_str);
	up_cost = strcost(up_str);

	/* cost of leaving insert or delete mode, used in move() */
	Tprintf((stderr, "str0cost: "));
	ei_cost = str0cost(ei_str);
	ed_cost = str0cost(ed_str);
	
	/* calculate insert and delete cost multiply_factor and overhead */
	if (((im_str && ei_str) || ic_str) && dc_str) {
		flags |= CAN_OPTIMISE;
		ins_mf = 1 + str0cost(ic_str);
		ins_oh = str0cost(im_str) + ei_cost;
		del_mf = str0cost(dc_str);
		del_oh = str0cost(dm_str) + ed_cost;
	}
	
	tc_initialized = Yes;
	Tprintf((stderr, "\tgettermcaps returns TE_OK\n"));
	return TE_OK;
}

Hidden int
start_trm()
{
	register int y;
	
	if (line == NULL) {
		line = (short **) malloc((unsigned) (lines * sizeof(short*)));
		if (line == NULL)
			return TE_NOMEM;
		for (y = 0; y < lines; y++) {
			line[y] = (short *)
				malloc((unsigned) ((cols+1) * sizeof(short)));
			if (line == NULL)
				return TE_NOMEM;
		}
	}
	if (lenline == NULL) {
		lenline = (short *) malloc((unsigned) (lines * sizeof(short)));
		if (lenline == NULL)
			return TE_NOMEM;
	}

	trmundefined();

	Putstr(ti_str);
	Putstr(ks_str);
	if (cs_str)
		Putstr(tgoto(cs_str, lines-1, 0));
	if (mustclear)
		clear_lines(0, lines-1);
	return TE_OK;
}


/*
 * Sensing and moving the cursor.
 */

/*
 * Sense the current (y, x) cursor position, after a possible manual
 * change by the user with local cursor motions.
 * If the terminal cannot be asked for the current cursor position,
 * or if the string returned by the terminal is garbled,
 * the position is made Undefined.
 */

Visible Procedure
trmsense(py, px)
	int *py;
	int *px;
{
	bool getpos();

	Tprintf((stderr, "\ttrmsense(&yy, &xx);\n"));
	check_started("trmsense");

	*py = *px = Undefined;
	set_mode(Normal);
	if (so_mode != Off)
		standend();
	
	if (flags&CAN_SENSE && getpos(py, px)) {
		if (*py < 0 || lines <= *py || *px < 0 || cols <= *px)
			*py = *px = Undefined;
	}
	cur_y = Undefined;
	cur_x = Undefined;
}

Hidden bool
getpos(py, px)
int *py, *px;
{
	char *format = cp_str;
	int fc; 		/* current format character */
	int ic; 		/* current input character */
	int num;
	int on_y = 1;
	bool incr_orig = No;
	int i, ni;

	Putstr(sp_str);
	VOID fflush(stdout);

	while (fc = *format++) {
		if (fc != '%') {
			if (trminput() != fc)
				return No;
		}
		else {
			switch (fc = *format++) {
			case '%':
				if (trminput() != '%')
					return No;
				continue;
			case 'r':
				on_y = 1 - on_y;
				continue;
			case 'i':
				incr_orig = Yes;
				continue;
			case 'd':
				ic = trminput();
				if (!isdigit(ic))
					return No;
				num = ic - '0';
				while (isdigit(ic=trminput()))
					num = 10*num + ic - '0';
				trmpushback(ic);
				break;
			case '2':
			case '3':
				ni = fc - '0';
		    		num = 0;
				for (i=0; i<ni; i++) {
					ic = trminput();
					if (isdigit(ic))
						num = 10*num + ic - '0';
					else
						return No;
				}
				break;
			case '+':
				num = trminput() - *format++;
				break;
			case '-':
				num = trminput() + *format++;
				break;
			default:
				return No;
			}
			/* assign num to parameter */
			if (incr_orig)
				num--;
			if (on_y)
				*py = num;
			else
				*px = num;
			on_y = 1 - on_y;
		}
	}

	return Yes;
}
		
/* 
 * To move over characters by rewriting them, we have to check:
 * (1) that the screen has been initialised on these positions;
 * (2) we do not screw up characters
 * when rewriting line[y] from x_from upto x_to
 */
Hidden bool
rewrite_ok(y, xfrom, xto)
int y, xfrom, xto;
{
	register short *plnyx, *plnyto;
	
	if (xto > lenline[y])
		return No;

	plnyto = &line[y][xto];
	for (plnyx = &line[y][xfrom]; plnyx <= plnyto; plnyx++)
		if (*plnyx == UNKNOWN
		    ||
		    (!has_xs && (*plnyx & SOBIT) != so_mode)
		   )
			return No;
	return Yes;
}
		
/*
 * Move to position y,x on the screen
 */
/* possible move types for y and x respectively: */
#define None	0
#define Down	1
#define Up	2
#define Right	1
#define ReWrite	2
#define Left	3
#define CrWrite	4

Hidden Procedure
move(y, x)
int y, x;
{
	int dy, dx;
	int y_cost, x_cost, y_move, x_move;
	int mode_cost;
	int xi;
	
	if (cur_y == y && cur_x == x)
		return;
	
	if (!has_mi || mode == Undefined)
		set_mode(Normal);
	if (!has_xs && ((!has_ms && so_mode != Off) || so_mode == Undefined))
		standend();
	
	if (cur_y == Undefined || cur_x == Undefined)
		goto absmove;
	
	dy = y - cur_y;
	dx = x - cur_x;

	if (dy > 0) {
		y_move = Down;
		y_cost = dy * do_cost;
	}
	else if (dy < 0) {
		y_move = Up;
		y_cost = -dy * up_cost;
	}
	else {
		y_move = None;
		y_cost = 0;
	}
	if (y_cost < abs_cost) {
		switch (mode) {
		case Normal:
			mode_cost = 0;
			break;
		case Insert:
			mode_cost = ei_cost;
			break;
		case Delete:
			mode_cost = ed_cost;
			break;
		}
		if (dx > 0) {
			x_cost = dx + mode_cost;
			if (dx*nd_cost < x_cost || !rewrite_ok(y, cur_x, x)) {
				x_cost = dx * nd_cost;
				x_move = Right;
			}
			else
				x_move = ReWrite;
		}
		else if (dx < 0) {
			x_cost = -dx * le_cost;
			x_move = Left;
		}
		else {
			x_cost = 0;
			x_move = None;
		}
		if (cr_cost + x + mode_cost < x_cost && rewrite_ok(y, 0, x)) {
			x_move = CrWrite;
			x_cost = cr_cost + x + mode_cost;
		}
	}
	else
		x_cost = abs_cost;

	if (y_cost + x_cost < abs_cost) {
		switch (y_move) {
		case Down:
			while (dy-- > 0) Putstr(do_str);
			break;
		case Up:
			while (dy++ < 0) Putstr(up_str);
			break;
		}
		switch (x_move) {
		case Right:
			while (dx-- > 0) Putstr(nd_str);
			break;
		case Left:
			while (dx++ < 0) Putstr(le_str);
			break;
		case CrWrite:
			Putstr(cr_str);
			cur_x = 0;
			/* FALL THROUGH */
		case ReWrite:
			set_mode(Normal);
			for (xi = cur_x; xi < x; xi++)
				putchar(line[y][xi]);
			break;
		}
	}
	else
	{
    absmove:
		if (cm_str == NULL) {
			Putstr(ho_str);
			for (cur_y = 0; cur_y < y; ++cur_y)
				Putstr(do_str);
			/* Should try to use tabs here: */
			for (cur_x = 0; cur_x < x; ++cur_x)
				Putstr(nd_str);
		}
		else
			Putstr(tgoto(cm_str, x, y));
	}
	
	cur_y = y;
	cur_x = x;
}


/*
 * Putting data on the screen.
 */

/*
 * Fill screen area with given data.
 * Characters with the SO-bit (0200) set are put in standout mode.
 */
Visible Procedure
trmputdata(yfirst, ylast, indent, data)
int yfirst;
int ylast;
register int indent;
register string data;
{
	register int y;
	int x, len, lendata, space;
		
	Tprintf((stderr, "\ttrmputdata(%d, %d, %d, \"%s\");\n", yfirst, ylast, indent, data));
	check_started("trmputdata");
	
	if (yfirst < 0)
		yfirst = 0;
	if (ylast >= lines)
		ylast = lines-1;
	space = cols*(ylast-yfirst+1) - indent;
	if (space <= 0)
		return;
	yfirst += indent/cols;
	indent %= cols;
	y= yfirst;
	if (!data)
		data= ""; /* Safety net */
	x = indent;
	lendata = strlen(data);
	if (ylast == lines-1 && lendata >= space)
		lendata = space - 1;
	len = Min(lendata, cols-x);
	while (y <= ylast) {
		put_line(y, x, data, len);
		y++;
		lendata -= len;
		if (lendata > 0) {
			x = 0;
			data += len;
			len = Min(lendata, cols);
		}
		else
			break;
	}
	if (y <= ylast)
		clear_lines(y, ylast);
}

/* 
 * We will first try to get the picture:
 *
 *                  op>>>>>>>>>>>op          oq<<<<<<<<<<<<<<<<<<<<<<<<oq
 *                  ^            ^           ^                         ^
 *           <xskip><-----m1----><----od-----><-----------m2----------->
 *   OLD:   "You're in a maze of twisty little pieces of code, all alike"
 *   NEW:          "in a maze of little twisting pieces of code, all alike"
 *                  <-----m1----><-----nd------><-----------m2----------->
 *                  ^            ^             ^                         ^
 *                  np>>>>>>>>>>>np            nq<<<<<<<<<<<<<<<<<<<<<<<<nq
 * where
 *	op, oq, np, nq are pointers to start and end of Old and New data,
 * and
 *	xskip = length of indent to be skipped,
 *	m1 = length of Matching part at start,
 *	od = length of Differing mid on screen,
 *	nd = length of Differing mid in data to be put,
 *	m2 = length of Matching trail.
 *
 * Then we will try to find a long blank-or-cleared piece in <nd+m2>:
 *
 *    <---m1---><---d1---><---nb---><---d2---><---m2--->
 *              ^         ^         ^        ^         ^
 *              np        bp        bq1      nq        nend
 * where
 *	bp, bq are pointers to start and AFTER end of blank piece,
 * and
 *	d1 = length of differing part before blank piece,
 *	nb = length of blank piece to be skipped,
 *	d2 = length of differing part after blank piece.
 * Remarks:
 *	d1 + nb + d2 == nd,
 * and
 *	d2 maybe less than 0.
 */
Hidden int
put_line(y, xskip, data, len)
int y, xskip;
string data;
int len;
{
	register short *op, *oq;
	register char *np, *nq, *nend;
	char *bp, *bq1, *p, *q;
	int m1, m2, od, nd, delta, dd, d1, nb, d2;
	bool skipping;
	int cost, o_cost; 	/* normal and optimising cost */
	
	/* Bugfix GvR 19-June-87: */
	while (lenline[y] < xskip)
		line[y][lenline[y]++] = ' ';
	
	/* calculate the magic parameters */
	op = &line[y][xskip];
	oq = &line[y][lenline[y]-1];
	np = data;
	nq = nend = data + len - 1;
	m1 = m2 = 0;
	while ((*op&SOCHAR) == (((short)*np)&SOCHAR) && op <= oq && np <= nq)
		op++, np++, m1++;
	if (flags & CAN_OPTIMISE)
		while ((*oq&SOCHAR) == (((short)*nq)&SOCHAR) && op <= oq && np <= nq)
			oq--, nq--, m2++;
	od = oq - op + 1;
	nd = nq - np + 1;
	/* now we have the first picture above */

	if (od==0 && nd==0)
		return;
	delta = nd - od;

	/* find the blank piece */
	p = q = bp = bq1 = np;
	oq += m2; 		/* back to current eol */
	if (!has_in) {
		while (p <= nend) {
			while (q<=nend && *q==' ' && (op>oq || *op==' '))
				q++, op++;
			if (q - p > bq1 - bp)
				bp = p, bq1 = q;
			p = ++q;
			op++;
		}
	}
	d1 = bp - np;
	nb = bq1 - bp;
	d2 = nq - bq1 + 1;
	
	/* what is cheapest:
	 *	normal: put nd+m2;                         (dd = nd+m2)
	 *	skipping: put d1, skip nb, put d2+m2;      (dd = d2+m2)
	 *	optimise: put dd, insert or delete delta.  (dd = min(od,nd))
	 */
	cost = nd + m2; 	/* normal cost */
	if (nb > abs_cost || (d1 == 0 && nb > 0)) {
		skipping = Yes;
		cost -= nb - (d1>0 ? abs_cost : 0); /* skipping cost */
		dd = d2;
	}
	else {
		skipping = No;
		dd = nd;
	}
	
	if (m2 != 0) {
		/* try optimising */
		o_cost = Min(od, nd);
		if (delta > 0)
			o_cost += delta * ins_mf + ins_oh;
		else if (delta < 0)
			o_cost += -delta * del_mf + del_oh;
		if (o_cost >= cost) {
			/* discard m2, no optimise */
			dd += m2;
			m2 = 0;
		}
		else {
			dd = Min(od, nd);
			skipping = No;
		}
	}

	/* and now for the real work */
	if (!skipping || d1 > 0)
		move(y, xskip + m1);

	if (has_xs)
		get_so_mode();
	
	if (skipping) {
		if (d1 > 0) {
			set_mode(Normal);
			put_str(np, d1, No);
		}
		if (has_xs && so_mode != Off)
			standend();
		set_blanks(y, xskip+m1+d1, xskip+m1+d1+nb);
		if (dd != 0 || delta < 0) {
			move(y, xskip+m1+d1+nb);
			np = bq1;
		}
	}
	
	if (dd > 0) {
		set_mode(Normal);
		put_str(np, dd, No);
	}
	
	if (m2 > 0) {
		if (delta > 0) {
			set_mode(Insert);
			ins_str(np+dd, delta);
		}
		else if (delta < 0) {
			if (so_mode != Off)
				standend();
				/* Some terminals fill with standout spaces! */
			set_mode(Delete);
			del_str(-delta);
		}
	}
	else {
		if (delta < 0) {
			clr_to_eol();
			return;
		}
	}
	
	lenline[y] = xskip + len;
	if (cur_x == cols) {
		if (!has_mi)
			set_mode(Normal);
		if (!has_ms)
			so_mode = Undefined;
		if (has_am)
			cur_y++;
		else
			Putstr(cr_str);
		cur_x = 0;
	}
	else if (has_xs) {
		if (m2 == 0) {
			if (so_mode == On)
				standend();
		}
		else {
			if (!(line[cur_y][cur_x] & XSBIT)) {
				if (so_mode != (line[cur_y][cur_x] & SOBIT))
					(so_mode ? standend() : standout());
			}
		}
	}
}

Hidden Procedure
set_mode(m)
int m;
{
	if (m == mode)
		return;
	switch (mode) {
	case Insert:
		Putstr(ei_str);
		break;
	case Delete:
		Putstr(ed_str);
		break;
	case Undefined:
		Putstr(ei_str);
		Putstr(ed_str);
		break;
	}
	switch (m) {
	case Insert:
		Putstr(im_str);
		break;
	case Delete:
		Putstr(dm_str);
		break;
	}
	mode = m;
}

Hidden Procedure
get_so_mode()
{
	if (cur_x >= lenline[cur_y] || line[cur_y][cur_x] == UNKNOWN)
		so_mode = Off;
	else
		so_mode = line[cur_y][cur_x] & SOBIT;
}

Hidden Procedure
standout()
{
	Putstr(so_str);
	so_mode = On;
	if (has_xs)
		line[cur_y][cur_x] |= SOCOOK;
}

Hidden Procedure
standend()
{
	Putstr(se_str);
	so_mode = Off;
	if (has_xs)
		line[cur_y][cur_x] = (line[cur_y][cur_x] & ~SOBIT) | XSBIT;
}

Hidden Procedure
put_str(data, n, inserting)
char *data;
int n;
bool inserting;
{
	register short c, so;
	short *ln_y_x, *ln_y_end;
	
	so = so_mode;
	if (has_xs) {
		ln_y_x = &line[cur_y][cur_x];
		ln_y_end = &line[cur_y][lenline[cur_y]];
	}
	while (n-- > 0) {
		if (has_xs && ln_y_x <= ln_y_end && ((*ln_y_x)&XSBIT))
			so = so_mode = (*ln_y_x)&SOBIT;
			/* this also checks for the standend cookie AFTER */
			/* the line because off the equals sign in <= */
		c = ((short)(*data++))&SOCHAR;
		if ((c&SOBIT) != so) {
			so = c&SOBIT;
			so ? standout() : standend();
 		}
		if (inserting)
			Putstr(ic_str);
		put_c(c);
		if (has_xs)
			ln_y_x++;
	}
}

Hidden Procedure
ins_str(data, n)
char *data;
int n;
{
	int x;
	
	/* x will start AFTER the line, because there might be a cookie */
	for (x = lenline[cur_y]; x >= cur_x; x--)
		line[cur_y][x+n] = line[cur_y][x];
	put_str(data, n, Yes);
}

Hidden Procedure
del_str(n)
int n;
{
	int x, xto;
	
	xto = lenline[cur_y] - n; /* again one too far because of cookie */
	if (has_xs) {
		for (x = cur_x + n; x >= cur_x; x--) {
			if (line[cur_y][x] & XSBIT)
				break;
		}
		if (x >= cur_x)
			line[cur_y][cur_x+n] =
				(line[cur_y][cur_x+n] & CHAR)
				|
				(line[cur_y][x] & COOKBITS);
	}
	for (x = cur_x; x <= xto; x++)
		line[cur_y][x] = line[cur_y][x+n];
	while (n-- > 0)
		Putstr(dc_str);
}

Hidden Procedure
put_c(c)
int c;
{
	char ch;
	short xs_flag;
	
	ch = c&CHAR;
	if (!isprint(ch) && ch != ' ') /* V7 isprint doesn't include blank */
		ch= '?';
	putchar(ch);
	if (has_xs)
		xs_flag = line[cur_y][cur_x]&XSBIT;
	else
		xs_flag = 0;
	line[cur_y][cur_x] = (c&SOCHAR)|xs_flag;
	cur_x++;
}

Hidden Procedure
clear_lines(yfirst, ylast)
int yfirst, ylast ;
{
	register int y;
	
	if (!has_xs && so_mode != Off)
		standend();
	if (cl_str && yfirst == 0 && ylast == lines-1) {
		Putstr(cl_str);
		cur_y = cur_x = 0;
		for (y = 0; y < lines; ++y) {
			lenline[y] = 0;
			if (has_xs) line[y][0] = NOCOOK;
		}
		return;
	}
	for (y = yfirst; y <= ylast; y++) {
		if (lenline[y] > 0) {
			move(y, 0);
			if (ylast == lines-1 && cd_str) {
				Putstr(cd_str);
				while (y <= ylast) {
					if (has_xs) line[y][0] = NOCOOK;
					lenline[y++] = 0;
				}
				break;
			}
			else {
				clr_to_eol();
			}
		}
	}
}

Hidden Procedure
clr_to_eol()
{
	lenline[cur_y] = cur_x;
	if (!has_xs && so_mode != Off)
		standend();
	Putstr(ce_str);
	if (has_xs) {
		if (cur_x == 0)
			line[cur_y][0] = NOCOOK;
		else if (line[cur_y][cur_x-1]&SOBIT)
			standend();
	}
}

Hidden Procedure
set_blanks
(y, xfrom, xto)
int y, xfrom, xto;
{
	register int x;
	
	for (x = xfrom; x < xto; x++) {
		line[y][x] = (line[y][x]&XSBIT) | ' ';
	}
}

/* 
 * outchar() is used by termcap's tputs;
 * we can't use putchar because that's probably a macro
 */
Hidden int
outchar(ch)
char ch;
{
	putchar(ch);
}

/*
 * Scrolling (part of) the screen up (or down, dy<0).
 */

Visible Procedure
trmscrollup(yfirst, ylast, by)
register int yfirst;
register int ylast;
register int by;
{
	Tprintf((stderr, "\ttrmscrollup(%d, %d, %d);\n", yfirst, ylast, by));
	check_started("trmscrollup");
	
	if (yfirst < 0)
		yfirst = 0;
	if (ylast >= lines)
		ylast = lines-1;

	if (yfirst > ylast)
		return;

	if (!has_xs && so_mode != Off)
		standend();
	
	if (by > 0 && yfirst + by > ylast
	    ||
	    by < 0 && yfirst - by > ylast)
	{
		clear_lines(yfirst, ylast);
		return;
	}
	
	if (by > 0) {
		(*scr_up)(yfirst, ylast, by);
		scr_lines(yfirst, ylast, by, 1);
	}
	else if (by < 0) {
		(*scr_down)(yfirst, ylast, -by);
		scr_lines(ylast, yfirst, -by, -1);
	}
}

Hidden Procedure
scr_lines(yfrom, yto, n, dy)
int yfrom, yto, n, dy;
{
	register int y;
	short *saveln;
	
	while (n-- > 0) {
		saveln = line[yfrom];
		for (y = yfrom; y != yto; y += dy) {
			line[y] = line[y+dy];
			lenline[y] = lenline[y+dy];
		}
		line[yto] = saveln;
		lenline[yto] = 0;
		if (has_xs) line[yto][0] = NOCOOK;
	}
}

Hidden Procedure
scr1up(yfirst, ylast, n)
	int yfirst;
	int ylast;
	int n;
{
	move(yfirst, 0);
	dellines(n);
	if (ylast < lines-1) {
		move(ylast-n+1, 0);
		addlines(n);
	}
}


Hidden Procedure
scr1down(yfirst, ylast, n)
	int yfirst;
	int ylast;
	int n;
{
	if (ylast == lines-1) {
		clear_lines(ylast-n+1, ylast);
	}
	else {
		move(ylast-n+1, 0);
		dellines(n);
	}
	move(yfirst, 0);
	addlines(n);
}


Hidden Procedure
addlines(n)
register int n;
{
	if (par_al_str && n > 1)
			Putstr(tgoto(par_al_str, n, n));
	else {
		while (n-- > 0)
			Putstr(al_str);
	}
}


Hidden Procedure
dellines(n)
register int n;
{
	if (par_dl_str && n > 1)
		Putstr(tgoto(par_dl_str, n, n));
	else {
		while (n-- > 0)
			Putstr(dl_str);
	}
}


Hidden Procedure
scr2up(yfirst, ylast, n)
int yfirst, ylast, n;
{
	Putstr(tgoto(cs_str, ylast, yfirst));
	cur_y = cur_x = Undefined;
	move(ylast, 0);
	while (n-- > 0) {
		Putstr(sf_str);
		if (has_db && ylast == lines-1)
			clr_to_eol();
	}
	Putstr(tgoto(cs_str, lines-1, 0));
	cur_y = cur_x = Undefined;
}


Hidden Procedure
scr2down(yfirst, ylast, n)
int yfirst, ylast, n;
{
	Putstr(tgoto(cs_str, ylast, yfirst));
	cur_y = cur_x = Undefined;
	move(yfirst, 0);
	while (n-- > 0) {
		Putstr(sr_str);
		if (has_da && yfirst == 0)
			clr_to_eol();
	}
	Putstr(tgoto(cs_str, lines-1, 0));
	cur_y = cur_x = Undefined;
}


/*
 * Synchronization, move cursor to given position (or previous if < 0).
 */

Visible Procedure
trmsync(y, x)
	int y;
	int x;
{
	Tprintf((stderr, "\ttrmsync(%d, %d);\n", y, x));
	check_started("trmsync");
	
	if (0 <= y && y < lines && 0 <= x && x < cols) {
		move(y, x);
		if (no_cursor) {
			Putstr(ve_str);
			no_cursor = No;
		}
	}
	else if (no_cursor == No) {
		Putstr(vi_str);
		no_cursor = Yes;
	}
	VOID fflush(stdout);
}


/*
 * Send a bell, visible if possible.
 */

Visible Procedure
trmbell()
{
	Tprintf((stderr, "\ttrmbell();\n"));
	check_started("trmbell");
	
	Putstr(vb_str);
	VOID fflush(stdout);
}


#ifdef SHOW

/*
 * Show the current internal statuses of the screen on stderr.
 * For debugging only.
 */

Visible Procedure
trmshow(s)
char *s;
{
	int y, x;
	
	fprintf(stderr, "<<< %s >>>\n", s);
	for (y = 0; y < lines; y++) {
		for (x = 0; x <= lenline[y] /*** && x < cols-1 ***/ ; x++) {
			fputc(line[y][x]&CHAR, stderr);
		}
		fputc('\n', stderr);
		for (x = 0; x <= lenline[y] && x < cols-1; x++) {
			if (line[y][x]&SOBIT)
				fputc('-', stderr);
			else
				fputc(' ', stderr);
		}
		fputc('\n', stderr);
		for (x = 0; x <= lenline[y] && x < cols-1; x++) {
			if (line[y][x]&XSBIT)
				fputc('+', stderr);
			else
				fputc(' ', stderr);
		}
		fputc('\n', stderr);
	}
	fprintf(stderr, "CUR_Y = %d, CUR_X = %d.\n", cur_y, cur_x);
	VOID fflush(stderr);
}
#endif


/*
 * DESCRIPTION.
 *
 * This package uses termcap to determine the terminal capabilities.
 *
 * The lines and columns of our virtual terminal are numbered 
 *	y = {0...lines-1} from top to bottom, and
 *	x = {0...cols-1} from left to right,
 * respectively.
 *
 * The Visible Procedures in this package are:
 *
 * trmstart(&lines, &cols, &flags)
 * 	Obligatory initialization call (sets tty modes etc.),
 * 	Returns the height and width of the screen to the integers
 * 	whose addresses are passed as parameters, and a flag that
 *	describes some capabilities.
 *	Function return value: 0 if all went well, an error code if there
 *	is any trouble.  No messages are printed for errors.
 *
 * trmundefined()
 *	Sets internal representation of screen and attributes to undefined.
 *	This is necessary for a hard redraw, which would get optimised to
 *	oblivion,
 *
 * trmsense(&y, &x)
 *	Returns the cursor position through its parameters
 *	after a possible manual change by the user.
 *
 * trmputdata(yfirst, ylast, indent, data)
 * 	Fill lines {yfirst..ylast} with data, after skipping the initial
 *	'indent' positions. It is assumed that these positions do not contain
 *	anything dangerous (like standout cookies or null characters).
 *
 * trmscrollup(yfirst, ylast, by)
 * 	Shift lines {yfirst..ylast} up by lines (down |by| if by < 0).
 *
 * trmsync(y, x)
 * 	Call to output data to the terminal and set cursor position.
 *
 * trmbell()
 *	Send a (possibly visible) bell, immediately (flushing stdout).
 *
 * trmend()
 * 	Obligatory termination call (resets tty modes etc.).
 *
 * You may call these as one or more cycles of:
 * 	+ trmstart
 * 	+    zero or more times any of the other routines
 * 	+ trmend
 * Trmend may be called even in the middle of trmstart; this is necessary
 * to make it possible to write an interrupt handler that resets the tty
 * state before exiting the program.
 *
 * ADDITIONAL SPECIFICATIONS (ROUTINES FOR CHARACTER INPUT)
 *
 * trminput()
 *	Return the next input character (with its parity bit cleared
 *	if any).  This value is a nonnegative int.  Returns -1 if the
 *	input can't be read any more.
 *
 * trmavail()
 *	Return 1 if there is an input character immediately available,
 *	0 if not.  Return -1 if not implementable.
 *
 * trminterrupt()
 *	Return 1 if an interrupt has occurred since the last call to
 *	trminput or trmavail, 0 else.  [Currently not implemented.]
 *
 * trmsuspend()
 *	When called in the proper environment (4BSD with job control
 *	enabled), suspends the editor, temporarily popping back to
 *	the calling shell.  The caller should have called trmend()
 *	first, and must call trmstart again afterwards.
 *	BUG: there is a timing window where keyboard-generated
 *	signals (such as interrupt) can reach the program.
 */
