/* 7-segment digit definitions */

#define __	<<1) | 1)
#define  _	<<1) | 0)
#define I __
#define i  _
#define AA ((((((((((((((0

short sevenseg[10]= {

AA	 __
	I  I
	  _
	I  I
	 __	,	/* 0 */

AA	  _
	i  I
	  _
	i  I
	  _	,	/* 1 */

AA	 __
	i  I
	 __
	I  i
	 __	,	/* 2 */

AA	 __
	i  I
	 __
	i  I
	 __	,	/* 3 */

AA	  _
	I  I
	 __
	i  I
	  _	,	/* 4 */

AA	 __
	I  i
	 __
	i  I
	 __	,	/* 5 */

AA	 __
	I  i
	 __
	I  I
	 __	,	/* 6 */

AA	 __
	i  I
	  _
	i  I
	  _	,	/* 7 */

AA	 __
	I  I
	 __
	I  I
	 __	,	/* 8 */

AA	 __
	I  I
	 __
	i  I
	 __		/* 9 */

};

#undef __
#undef  _
#undef I
#undef i
#undef AA
