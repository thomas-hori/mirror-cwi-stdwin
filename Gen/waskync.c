/* STDWIN -- ASK YES/NO QUESTIONS. */

#include "tools.h"
#include "stdwin.h"

/* Ask a yes/no question.
   Return value: yes ==> 1, no ==> 0, cancel (^C) ==> -1.
   Only the first non-blank character of the string typed is checked.
   The 'deflt' parameter is returned when an empty string is typed. */

int
waskync(question, def)
	char *question;
	int def;
{
	char buf[100];
	char *p= "";
	
	switch (def) {
	case 1:	p= "Yes"; break;
	case 0: p= "No"; break;
	}
	strcpy(buf, p);
	for (;;) {
		if (!waskstr(question, buf, sizeof buf))
			return -1;
		p= buf;
		while (isspace(*p))
			++p;
		if (*p == EOS)
			return def;
		switch (*p) {
		case 'y':
		case 'Y':	return 1;
		case 'n':
		case 'N':	return 0;
		}
		wfleep();
	}
}
