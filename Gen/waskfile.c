/* STDWIN -- UNIVERSAL WASKFILE. */

#include "tools.h"
#include "filedefs.h"
#include "stdwin.h"

/* Ask for a file name; default is initial contents of buf.
   Checks are made that the name typed is sensible:
   if 'new' is TRUE, it should be writable or creatable,
   and if it already exists confirmation is asked;
   if 'new' is FALSE, it should exist. */

/* XXX This should also refuse to open directories */

bool
waskfile(prompt, buf, len, new)
	char *prompt;
	char *buf;
	int len;
	bool new;
{
	for (;;) {
		if (!waskstr(prompt, buf, len) || buf[0] == EOS)
			return FALSE;
		if (new) {
			if (access(buf, NOMODE) >= 0) {	/* Existing file */
				if (access(buf, WMODE) >= 0) {
					switch (waskync(
					    "Overwrite existing file?", 0)) {
					case -1:	return FALSE;
					case 1:		return TRUE;
					}
				}
				else
					wmessage("No write permission");
			}
			else {
				char *p= strrchr(buf, SEP);
				if (p == NULL) {
					if (access(CURDIR, WMODE) >= 0)
						return TRUE;
				}
				else {
					*p= EOS;
					if (access(buf, WMODE) >= 0) {
						*p= SEP;
						return TRUE;
					}
				}
				wmessage("Can't create file");
			}
		}
		else {
			if (access(buf, RMODE) >= 0)
				return TRUE;
			wmessage("File not found");
		}
		break;
	}
	return FALSE;
}
