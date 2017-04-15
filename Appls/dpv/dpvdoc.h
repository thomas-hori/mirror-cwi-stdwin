/* dpv -- ditroff previewer.  Description of input language. */

/* This isn't really a source file but disguised as one it is
   more likely to be distributed with the rest of the source */

/******************************************************************************

    output language from ditroff:
    all numbers are character strings
    
    (These comments should suffice to write my own ditroff output
    filter, but I'm lazy...  Note that the parser expects its input to
    be error-free -- it contains unchecked fscanf calls all over.
    Also it is not clear whether relative motions may have negative
    numbers as parameters.  For filters descending from BWK's prototype
    this works for 'v' but not for 'h', as 'v' uses fscan but 'h'
    reads characters until it finds a non-digit...  GvR)

{	push environment (font, size, position)
}	pop environment
#..\n	comment
sn	size in points
fn	font as number from 1 to n
cx	ascii character x
Cxyz	funny char \(xyz. terminated by white space
Hn	go to absolute horizontal position n
Vn	go to absolute vertical position n (down is positive)
hn	go n units horizontally (relative)
vn	ditto vertically
nnc	move right nn, then print c (exactly 2 digits!)
		(this wart is an optimization that shrinks output file size
		 about 35% and run-time about 15% while preserving ascii-ness)
pn	new page begins (number n) -- set v to 0
P	spread ends -- output it. (Put in by vsort).
nb a	end of line (information only -- no action needed)
	b = space before line, a = after
w	paddable word space -- no action needed

Dt ..\n	draw operation 't':
	Dl x y		line from here by x,y (drawing char .)
			(affects position by x, y -- GvR)
	Dc d		circle of diameter d with left side here
	De x y		ellipse of axes x,y with left side here
	Da x y x1 y1	arc; see drawarc in dpvoutput.c for description
	D~ x y x y ...	B-spline curve by x,y then x,y ...

x ..\n	device control functions:
     x i	init
     x T s	name of device is s
     x r n h v	resolution is n/inch h = min horizontal motion, v = min vert
     x p	pause (can restart)
     x s	stop -- done for ever
     x t	generate trailer
     x f n s	font position n contains font s
     		(there appears to be some disagreement whether this also
     		selects s as the current font -- GvR)
     x H n	set character height to n
     x S n	set slant to N

	Subcommands like "i" are often spelled out like "init".

******************************************************************************/
