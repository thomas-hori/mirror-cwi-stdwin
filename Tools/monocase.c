/* monocasecmp -- like strcmp but upper and lower case letters
   are considered identical. */

#include <ctype.h>

#define EOS '\0'

int
monocasecmp(a, b)
	register char *a;
	register char *b;
{
	for (;;) {
		register char ca= *a++;
		register char cb= *b++;
		if (ca == cb) {
			if (ca == EOS)
				return 0;
		}
		else {
			if (islower(ca))
				ca= toupper(ca);
			if (islower(cb))
				cb= toupper(cb);
			if (ca != cb)
				return ca - cb;
		}
	}
}
