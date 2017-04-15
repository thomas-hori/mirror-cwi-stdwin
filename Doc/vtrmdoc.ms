.TL
.if n .po 0
.if n .ll 72
.if n .nr LL 72n
.if t .po 3.5c \" For LaserWriter only
Interfacing to Keyboard and Screen with VTRM
.br
(Draft)
.AU
Guido van Rossum
.AB
Since both groups of `interns' at the ABC project will soon need to
interface their editors to a terminal, here is a description of the VTRM
package, together with some hints on writing a simple displayer using
this package.  Footnotes are used to indicate portions of the interface
that may change in the future.
.AE
.SH
1. Introduction
.LP
The VTRM package can significantly ease terminal-independent input and
output for a screen-oriented program like a text editor, written in C.
There are
about a dozen routines, with few and simple parameters, that allow
optimized output to almost any ASCII terminal designed in the Western
hemisphere, and input of single characters without echoing.  Features
include adaptation to any screen size, scrolling, and inverse video or a
similar `stand-out' mode of displaying selected characters.  Screen
refreshes use available terminal features such as character insert or
delete to minimize the number of characters actually output to the
terminal, thus obtaining reasonable performance even at low baud rates.
.PP
Implementations of the VTRM package are available for
.UX ,
MS-DOS and the Apple Macintosh, and the package can be ported to
other similar micro-computers.  It is also possible to write an
interface to windowing systems like those found on Sun or Whitechapel
workstations.
.SH
2. Theory of operation
.LP
VTRM is a set of subroutines callable from a C program.  There is a
certain discipline required in calling these routines; there are
an initialization call, several data manipulation and input calls, and a
clean-up call.  The C program that is making the calls to VTRM is
further referred to as `the application'.  In fact, on different
systems, different soubroutines are used; but they all conform to the
same interface, which is what is described in this document.
.PP
VTRM has a simple model for a terminal screen: there are YMAX lines,
each consisting XMAX characters.  YMAX and XMAX are not constants,
but remain constant during one particular program run.*
.FS
*This causes problems with resizable windows as currently available
under 4.3 BSD.  It is possible that future versions can adapt themselves
to changing window sizes, but this will require adaptation of the
programs calling VTRM.
.FE
.PP
The coordinate system used by VTRM is different from that normally used
in geometry, but more convenient for referencing screen positions:
whereever a position on the screen is referenced, it has the form (Y, X)
where Y is the line number and X is the column number.  The top line is
line 0, the bottom line is YMAX\-1.  Likewise, the leftmost
column is column 0, the rightmost is XMAX\-1.  (Note how this is
compatible with array indexing practices in C.) **
.FS
** This will get us into trouble with bit-mapped displays, where often
the preferred character set or `font' has characters of different
widths, so that the number of characters displayable on a line is not a
constant.  This issue is ignored by VTRM; implementations on such
systems currently use a `fixed-width' font.
.FE
.PP
Each character position contains one character, which is in the ASCII
range SPACE to `~', i.e., SPACE and all printable characters.  Each
character can be displayed in one of two modes: normal or `stand-out',
which is usually inverse video or underlined (this is a choice made by
the VTRM package).  `Empty' parts of the screen contain spaces in normal
mode. ***
.FS
*** Eventually the character set allowed will have to be extended to all
or most characters in the machine's standard character set that can be
displayed.  There is a tendency for systems to extend the 7-bit ASCII
code to an 8-bit code, where the characters in the range 128-255 are
used for special graphics, accented characters etc.  VTRM currently uses
the 8th bit to indicate stand-out mode; this will have to be changed in
the interface.  Applications are discouraged to use the 8th bit of
characters as a `stand-out' flag except in the immediate interface with
VTRM.
.FE
.PP
VTRM is strongly line-oriented.  The application must offer data to VTRM
in chunks of one or more lines, and scrolling operations can only be
applied to entire lines.  This is not likely to change in the near
future.  VTRM does not necessarily send whole lines to the screen: only
changed parts of a line are actually transmitted.  VTRM recognizes most
cases of insertion or deletion of small strings in a line, which can
often be handled with character insertions and/or deletions.
.PP
VTRM may buffer its output.  There is one routine which flushes the
buffer and moves the terminal's cursor to a given (Y, X) position;
during the screen update process the screen contents and cursor position
may temporarily be undefined.  Not all implementations actually buffer
their output; micro-computer screen operations are often fast enough.
In all cases, the cursor position is undefined during the screen update.
.PP
VTRM does not automatically clear the screen when first called; it
allows an application to be written in such a manner that its output
appears at the bottom of the screen and gradually scrolls upwards,
mimicking the behaviour of older programs, if it so wishes.  On the
other hand, it is trivial to clear the screen after the first call.
.PP
On
.UX ,
VTRM sets the terminal in a special mode (formally: cbreak, no echo, no
cr to nl mapping).  This makes it
necessary that an application takes measures to call the VTRM clean-up
routine whenever it exits.
.PP
On
.UX ,
VTRM uses the `termcap' database to find out the terminal's
capabilities.  It requires that the <:TERM:> environment variable is
set, and optionally acknowledges the <:TERMCAP:> variable.  Information
on the proper use of these variables should be readily accessible to all
users of the application.
.SH
3. Description of the interface
.SH
3.1 Set-up and clean-up calls
.SH
TRMSTART
.LP
.DS
<:int trmstart(p_lines, p_columns, p_flags)
int *p_lines, *p_columns;
int *p_flags;:>
.DE
This call must be made before any of the other calls are allowed.  The
function result is 0 if initialization is successful, nonzero if an
error occurred.  In the latter case the applications should usually
print an error message and exit; the terminal state has not been
altered.  The meaning of the error codes is described in the appendix.
.PP
The three arguments must be the addresses of three integer variables
(call as, e.g., <:trmstart(&lines, &columns, &flags):>).
In <:*p_lines:> and <:*p_columns:>, YMAX and XMAX are returned.
In <:*p_flags:>, some flag bits are returned, but these can usually be
ignored by the applications (some flag bits are defined but never used).
.SH
TRMEND
.LP
.DS
<:trmend();:>
.DE
This function cleans up before the application exits.  If it is not
called (when <:trmstart:> has been called), the application may leave
the terminal in a weird mode, from which it is hard to recover for the
user (on
.UX -systems non-
other weird things may happen).
.PP
So that it can be safely called from within an interrupt handler,
<:trmend:> may be called at any time, before or even during a call to
<:trmstart:>.  Calls at any other time than after a successful call
to <:trmstart:> are ignored.  After a call to <:trmend:>, as before a
call to <:trmstart:>, calls to all other routines of VTRM are forbidden
(this is not enforced by all implementations \- but your program may
crash).
.PP
An application may engage in a sequence of interactive sessions, each
bracketed between calls to <:trmstart:> and <:trmend:>.  Outside these
sessions, normal print-style output can safely be used, thusly:
.DS
<:trmstart(...);
\fIVTRM interaction 1\fP
trmend();
printf("Hello, world\\n"); /* Etc. */
trmstart(...);
\fIVTRM interaction 2\fP
trmend();
:><:...:>
.DE
.SH
3.2 Output calls
.SH
TRMPUTDATA
.LP
.DS
<:trmputdata(y0, y1, x, data)
int y0, y1;
int x;
char *data;:>
.DE
The characters in `data' are placed on the screen, starting at line y0,
position x, and continuing up to the last position of line y1.  If data
is shorter than this space, the remaining positions are filled with
spaces; if tdata is too long, it is truncated.  The positions 0 through
x\-1 on line y0 are left unchanged.
.PP
Characters with their 8th bit on (or-ed with 0200 octal or 0x80 hex)
are displayed in `stand-out' mode.
.PP
The following is an easy way of clearing the screen:
.DS
<:trmputdata(0, YMAX-1, 0, "");:>
.DE
.SH
TRMSCROLLUP
.LP
.DS
<:trmscrollup(y0, y1, n)
int y0, y1;
int n;:>
.DE
Scrolls the screen lines y0 to y1, inclusive, up by n lines.
If n is negative, it scrolls down.  When scrolling up, the top n lines
starting at y0 disappear, the lines from y0+n to y1 move up n lines,
and n blank lines are `shifted in' at and above line y1.  Scrolling down
is similar.  If abs(n) > y1\-y0, lines y0 to y1 are blanked.
.SH
TRMSYNC
.LP
.DS
<:trmsync(y, x)
int y, x;:>
.DE
Completes any pending operations, flushes the output buffer, and moves
the cursor to position (y, x) on the screen.
.SH
TRMUNDEFINED
.LP
.DS
<:trmundefined();:>
.DE
Tells VTRM to forget whatever it has remembered about the current screen
contents.  This is necessary before doing a complete screen refresh in
response to a user command like control-L, since such a refresh is
usually intended to correct the effect of transmission errors or other
processes clobbering the screen.
.SH
TRMBELL
.LP
.DS
<:trmbell();:>
.DE
Issues an alarm to the user in a way most appropriate to the output
device being used.  This may either be an audible bell or beep,
or a `visible bell', meaning a flash of (part of) the screen.
.PP
(On
.UX ,
a control-G is sent to the terminal, unless the termcap entry specifies
the `vb' property (visible bell).  On MS-DOS, the bell is sounded
unconditionally.  On the Macintosh, `SysBeep' is called, which gives a
beep with a level determined by the volume control on the Control Panel,
or flashes the menu bar if the volume is set to 0.)
.SH
3.3 Input calls
.SH
TRMINPUT
.LP
.DS
<:int trminput();:>
.DE
Returns the next input character typed at the keyboard, waiting if
necessary until one is typed.  When an error occurs, \-1 is returned;
this is usually permanent, so further input would be futile.  This could
happen, for example, when the user `hangs up'.
.SH
TRMAVAIL
.LP
.DS
<:int trmavail();:>
.DE
Returns 1 if an input character is immediately available for
<:trminput:>; 0 if no such character is available; and \-1 if the system
can't find out (this is not an error condition; it means that the system
cannot do a `non-blocking read').
.SH
TRMSENSE
.LP
.DS
<:int trmsense(p_y, p_x)
int *p_y, *p_x;:>
.DE
Senses the current cursor or mouse position, and returns its position,
converted to screen coordinates, in <:*p_y:> and <:*p_x:>.  If the
terminal is not capable of sensing the cursor position, both values are
set to \-1 and the functions returns 0; after a successful sense, the
function returns 1.
.SH
3.4 Interrupt handling calls
.SH
TRMINTERRUPT
.LP
.DS
<:int trminterrupt();:>
.DE
Checks for keyboard-generated interrupt.  Returns 1 if one is found, 0
if not.  This may set a signal handler for <:SIGINT:>, so it may
interfere with the application's signal handling.*  It may also flush
type-ahead and (unfortunately enough) discard output buffers.
.FS
* This is an experimental feature.  Use at own risk.  Check the source
code to see if its actually implemented, and how.
.FE
.SH
TRMSUSPEND
.LP
.DS
<:trmsuspend();:>
.DE
This call does nothing except on Berkeley
.UX
supporting job control.  Because the terminal state and screen contents
are not restored when a process is suspended or resumed, programs using
VTRM must be aware of suspension.  The character control-Z is received
as a normal input character by <:trminput:>, because <:trmstart:> turns
off most special character processing.  When a control-Z is received,
the application should react as follows:
.DS
<:trmend();
trmsuspend();
trmstart(...);
\fIrepaint the screen\fR:>
.DE
The <:trmsuspend:> call suspends the application and its `process group';
this includes any subprocesses, and possibly parent processes (if the
application was run from a shell script or from another program, maybe
using a `shell escape').  It only returns when the process group is resumed
again (with the shell's `fg' command).
.SH
4. Examples
.LP
Here is a very small sample program:
.DS
<:main() {
	int lines, columns, flags;
	if (trmstart(&lines, &columns, &flags) != 0) exit(1);
	trmputdata(0, lines-1, 0, "Hello, \\327orld!");
	trmsync(1, 0);
	trmend();
	exit(0);
}:>
.DE
It prints the text <:Hello, \fBW\fRorld!:> at the top of the screen,
cleans the rest of the screen, and moves the cursor the the beginning of
the second line.  The `W' is displayed in stand-out mode.
.TL
Appendix \- <:\fBtrmstart:> error codes and flags
.sp 2
.LP
The file "trm.h" can be included to get definitions for the flags and
error codes returned by <:trmstart:>.  It contains the following:
.sp 1
.nf
<:
.ta 8n 16n 24n 32n 40n 48n 56n 64n 72n
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1986. */

/*
 * Terminal capabilities.  These correspond to bits set by trmstart in its
 * parameter flags parameter.
 */

#define HAS_STANDOUT	1	/* Terminal has inverse video or underline */
#define CAN_SCROLL	2	/* Terminal can insert/delete lines */
#define CAN_OPTIMISE	4	/* Terminal can insert/delete characters */
#define CAN_SENSE	8	/* Terminal can send cursor position */

/*
 * Error codes returned by trmstart.
 */

#define TE_OK		0	/* No errors */
#define TE_TWICE	1	/* Trmstart called again */
#define TE_NOTERM	2	/* $TERM not set or empty */
#define TE_BADTERM	3	/* $TERM not found in termcap database */
#define TE_DUMB		4	/* Terminal too dumb */
#define TE_NOTTY	5	/* Stdout not a tty device */
#define TE_NOMEM	6	/* Can't get enough memory */
#define TE_OTHER	7	/* This and higher are reserved errors */
:>
.fi
