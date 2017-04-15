/* Determine byte order. */

#include "endian.h"

int endian;

endianism()
{
	union {
		short s;
		char c[2];
	} u;
	
	u.c[0]= 1;
	u.c[1]= 2;
	
	switch (u.s) {
	case 0x0201:
		endian= LIL_ENDIAN;
		break;
	case 0x0102:
		endian= BIG_ENDIAN;
		break;
	}
}
