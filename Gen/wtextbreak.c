/* STDWIN -- TEXT BREAK ROUTINE. */

#include "stdwin.h"

/* Portable version of wtextbreak(); use on systems where such an
   operation is not an available primitive and wtextwidth has a high
   overhead per call.
   This function makes an educated guess and then uses linear
   interpolation to find the exact answer.
   It assumes that the textwidth function is additive (if not, but
   almost, a final adjustment pass could be added). */

int
wtextbreak(str, len, width)
	char *str;
	int len;
	int width;
{
	int en= wcharwidth('n');	/* Estimated average char width */
					/* NB: adapted below in the loop! */
	int max;			/* Maximum answer */
	int min= 0;			/* Minimum answer */
	int wmin= 0;			/* Corresponding string width */
	
	if (len < 0)
		len= strlen(str);
	max= len;
	
	/* Invariants:
	   'min' characters fit, 'max+1' characters don't.
	   Ergo: we can stop when min == max. */
	
	while (min < max) {
		/* Guess a number of chars beyond min. */
		int guess= (width - wmin)/en;
		int wguess;
		if (guess <= 0)
			guess= 1;
		else if (min+guess > max)
			guess= max-min;
		wguess= wtextwidth(str+min, guess); /* Width increment */
		if (wguess > 0)
			en= (wguess + guess - 1) / guess;
		guess += min;
		wguess += wmin;
		if (wguess > width) {
			max= guess-1;
		}
		else /* wguess <= width */ {
			min= guess;
			wmin= wguess;
		}
	}

#ifdef TEXT_NOT_ADDITIVE
	/* Initially, min==max.  See if min should be smaller. */
	while (min > 0 && wtextwidth(str, min) > width) {
		--min;
	}
	if (min == max) {
		/* Previous loop didn't decrease min.
		   See if we can increase it... */
		while (max < len && wtextwidth(str, ++max) <= width) {
			++min;
		}
	}
#endif
	
	return min;
}
