/*
 * Copyright (c) 1989 Stichting Mathematisch Centrum, Amsterdam, Netherlands.
 * Written by Guido van Rossum (guido@cwi.nl).
 * NO WARRANTIES!
 * Freely usable and copyable as long as this copyright message remains intact.
 */

/*
 * Function glob() as proposed in Posix 1003.2 B.6 (rev. 9).
 * Matches *, ? and [] in pathnames like sh(1).
 * The [!...] convention to negate a range is supported (SysV, Posix, ksh).
 * The interface is a superset of the one defined in Posix 1003.2 (draft 9).
 * Optional extra services, controlled by flags not defined by Posix:
 * - replace initial ~username by username's home dir (~ by $HOME);
 * - replace initial $var(but not $var elsewhere) by value of getenv("var");
 * - escaping convention: \ inhibits any special meaning the following
 *   character might have (except \ at end of string is kept);
 * - harmonica matching: /foo/** matches all files in the tree under /foo,
 *   including /foo itself.
 * If you don't want code for $ or ~ or ** expansion, simply remove the
 * corresponding #defines from glob.h.
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#ifdef SYSV
#define MAXPATHLEN 1024
#else
#include <sys/param.h> /* For MAXPATHLEN */
#endif
#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <glob.h>

extern struct passwd *getpwnam();
extern char *malloc();
extern char *realloc();
extern char *getenv();
extern int errno;

#ifndef NULL
#define NULL 0
#endif

#ifndef STATIC
#define STATIC static
#endif

typedef int bool_t;
#define FALSE 0
#define TRUE 1

#define TILDE '~'
#define DOLLAR '$'
#define UNDERSCORE '_'
#define QUOTE '\\'

#define SEP '/'
#define DOT '.'

#define STAR '*'
#define QUESTION '?'
#define LBRACKET '['
#define RBRACKET ']'
#define NOT '!'
#define RANGE '-'

#define EOS '\0'

#define HOME "HOME"

#define MARK	0x80
#define META(c) ((c)|MARK)
#define M_ALL META('*')
#define M_ONE META('?')
#define M_SET META('[')
#define M_NOT META('!')
#define M_RNG META('-')
#define M_END META(']')
#define ISMETA(c) (((c)&MARK) != 0)


#if GLOB_TILDE | GLOB_DOLLAR

/* Copy the characters between str and strend to buf, and append a
   trailing zero.  Assumes buf is MAXPATHLEN+1 long.  Returns buf. */

STATIC char *
makestr(buf, str, strend)
	char *buf; /* Size MAXPATHLEN+1 */
	char *str, *strend;
{
	int n;
	
	n = strend - str;
	if (n > MAXPATHLEN)
		n = MAXPATHLEN;
	strncpy(buf, str, n);
	buf[n] = EOS;
	return buf;
}

/* Append string to buffer, return new end of buffer.  Guarded. */

STATIC char *
addstr(dest, src, end)
	char *dest;
	char *src;
	char *end;
{
	while (*dest++ = *src++) {
		if (dest >= end)
			break;
	}
	return --dest;
}

#endif /* GLOB_TILDE | GLOB_DOLLAR */


#if GLOB_TILDE

#include <pwd.h>

/* Find a user's home directory, NULL if not found */

STATIC char *
gethome(username)
	char *username;
{
	struct passwd *p;
	
	p = getpwnam(username);
	if (p == NULL)
		return NULL;
	else
		return p->pw_dir;
}

#endif /* GLOB_TILDE */


/* String compare for qsort */

STATIC int
compare(p, q)
	char **p;
	char **q;
{
	return strcmp(*p, *q);
}


/* The main glob() routine: does optional $ and ~ substitution,
   compiles the pattern (optionally processing quotes),
   calls glob1() to do the real pattern matching,
   and finally sorts the list (unless unsorted operation is requested).
   Returns 0 if things went well, nonzero if errors occurred.
   It is not an error to find no matches. */

int
glob(pattern, flags, errfunc, pglob)
	char *pattern;
	int flags;
	int (*errfunc)();
	glob_t *pglob;
{
	char *patnext = pattern;
	char patbuf[MAXPATHLEN+1];
	char *bufnext, *bufend;
	char *compilebuf, *compilepat;
	char *p, *q;
	char c;
	int err;
	int oldpathc;
	
	if ((flags & GLOB_APPEND) == 0) {
		pglob->gl_pathc = 0;
		pglob->gl_pathv = NULL;
		if ((flags & GLOB_DOOFFS) == 0)
			pglob->gl_offs = 0;
	}
	pglob->gl_flags = flags;
	pglob->gl_errfunc = errfunc;
	oldpathc = pglob->gl_pathc;
	
	bufnext = patbuf;
	bufend = bufnext+MAXPATHLEN;
	
#if GLOB_DOLLAR | GLOB_TILDE
	c = *patnext;
#endif
	
#if GLOB_DOLLAR
	if (c == DOLLAR && (flags & GLOB_DOLLAR)) {
		p = ++patnext;
		while (isalnum(*p) || *p == UNDERSCORE)
			++p;
		if ((q = getenv(makestr(bufnext, patnext, p))) != NULL)
			bufnext = addstr(bufnext, q, bufend);
		patnext = p;
	}
#endif /* GLOB_DOLLAR */

#if GLOB_TILDE
	if (c == TILDE && (flags & GLOB_TILDE)) {
		p = ++patnext;
		while (*p != EOS && *p != SEP)
			++p;
		if (p == patnext) {
			q = getenv(HOME);
			if (q == NULL)
				--patnext;
			else
				bufnext = addstr(bufnext, q, bufend);
		}
		else {
			q = gethome(makestr(bufnext, patnext, p));
			if (q == NULL)
				--patnext;
			else {
				bufnext = addstr(bufnext, q, bufend);
				patnext = p;
			}
		}
	}
#endif /* GLOB_DOLLAR */
	
	compilebuf = bufnext;
	compilepat = patnext;
	while (bufnext < bufend && (c = *patnext++) != EOS) {
		switch (c) {
		
		case QUOTE:
			if (!(flags & GLOB_QUOTE))
				*bufnext++ = QUOTE;
			else {
				if ((c = *patnext++) == EOS) {
					c = QUOTE;
					--patnext;
				}
				*bufnext++ = c;
			}
			break;
		
		case STAR:
			*bufnext++ = M_ALL;
			break;
		
		case QUESTION:
			*bufnext++ = M_ONE;
			break;
		
		case LBRACKET:
			c = *patnext;
			if (c == NOT)
				++patnext;
			if (*patnext == EOS ||
					strchr(patnext+1, RBRACKET) == NULL) {
				*bufnext++ = LBRACKET;
				if (c == NOT)
					--patnext;
				break;
			}
			*bufnext++ = M_SET;
			if (c == NOT)
				*bufnext++ = M_NOT;
			c = *patnext++;
			do {
				/* TO DO: quoting */
				*bufnext++ = c;
				if (*patnext == RANGE &&
						(c = patnext[1]) != RBRACKET) {
					*bufnext++ = M_RNG;
					*bufnext++ = c;
					patnext += 2;
				}
			} while ((c = *patnext++) != RBRACKET);
			*bufnext++ = M_END;
			break;
		
		default:
			*bufnext++ = c;
			break;

		}
	}
	*bufnext = EOS;
	
	if ((err = glob1(patbuf, pglob)) != 0)
		return err;
	
	if (pglob->gl_pathc == oldpathc) {
		if (flags & GLOB_NOCHECK) {
			if (!(flags & GLOB_QUOTE))
				strcpy(compilebuf, compilepat);
			else {
				/* Copy pattern, interpreting quotes */
				/* This is slightly different than the
				   interpretation of quotes above --
				   which should prevail??? */
				while (*compilepat != EOS) {
					if (*compilepat == QUOTE) {
						if (*++compilepat == EOS)
							--compilepat;
					}
					*compilebuf++ = *compilepat++;
				}
				*compilebuf = EOS;
			}
			return globextend(patbuf, pglob);
		}
	}
	else {
		if (!(flags & GLOB_NOSORT))
			qsort((char*)
				(pglob->gl_pathv + pglob->gl_offs + oldpathc),
				pglob->gl_pathc - oldpathc,
				sizeof(char*), compare);
	}
	
	return 0;
}

STATIC int
glob1(pattern, pglob)
	char *pattern;
	glob_t *pglob;
{
	char pathbuf[MAXPATHLEN+1];
	
	/* A null pathname is invalid (Posix 1003.1 sec. 2.4 last sentence) */
	if (*pattern == EOS)
		return 0;
	return glob2(pathbuf, pathbuf, pattern, pglob);
}

/* Functions glob2 and glob3 are mutually recursive; there is one level
   of recursion for each segment in the pattern that contains one or
   more meta characters. */

STATIC int
glob2(pathbuf, pathend, pattern, pglob)
	char *pathbuf; /*[MAXPATHLEN+1]*/
	char *pathend;
	char *pattern;
	glob_t *pglob;
{
	bool_t anymeta = FALSE;
	char *p, *q;
	
	/* Loop over pattern segments until end of pattern or until
	   segment with meta character found. */
	for (;;) {
		/* End of pattern? */
		if (*pattern == EOS) {
			struct stat sbuf;
			*pathend = EOS;
			if (stat(pathbuf, &sbuf) != 0)
				return 0; /* Need error call here? */
			if ((pglob->gl_flags & GLOB_MARK) &&
					pathend[-1] != SEP &&
					(sbuf.st_mode & S_IFMT) == S_IFDIR) {
				*pathend++ = SEP;
				*pathend = EOS;
			}
			return globextend(pathbuf, pglob);
		}
		
		/* Find end of next segment, copy tentatively to pathend */
		q = pathend;
		p = pattern;
		while (*p != EOS && *p != SEP) {
			if (ISMETA(*p))
				anymeta = TRUE;
			*q++ = *p++;
		}
		
		if (!anymeta) {
			/* No expansion needed, go on with next segment */
			pathend = q;
			pattern = p;
			while (*pattern == SEP)
				*pathend++ = *pattern++;
		}
		else {
			/* Need expansion, start another recursion level */
#ifdef GLOB_HARMONICA
			if ((pglob->gl_flags & GLOB_HARMONICA) &&
					isharmonica(pattern, p)) {
				/* Special case: foo|**|bar matches foo|bar */
				int err = glob2a(pathbuf, pathend, p, pglob);
				if (err != 0)
					return err;
			}
#endif /* GLOB_HARMONICA */
			return glob3(pathbuf, pathend, pattern, p, pglob);
		}
	}
	/* We never get here */
}

#ifdef GLOB_HARMONICA

STATIC int
glob2a(pathbuf, pathend, restpattern, pglob)
	char *pathbuf; /*[MAXPATHLEN+1]*/
	char *pathend;
	char *restpattern;
	glob_t *pglob;
{
	/* Various special cases to get foo|**|bar right.
	   Table (| substituted for / to avoid ending the comment!):
	
		pathbuf:	restpattern:
				** (1)		**| (2)		**|bar (3)
	   (A)	foo/		foo		foo/		foo/bar
	   (B)	/		/		/		/bar
	   (C)	empty		none		none		bar
	
	   In case (2), any number of tailing slashes might be present:
	   **|||||||.
	   
	   Note that, because of the way glob2() and glob3() work,
	   if pathbuf is not empty, it ends in a slash.
	   Similarly, we know that restpattern[0] is either a slash
	   or the end of the string.
	*/
	
	int err;
	
	if (pathend > pathbuf) { /* Cases (A) and (B) */
		if (restpattern[0] == SEP) /* Cases (2) and (3) */
			restpattern++;
		else if (pathend >= pathbuf+2) /* Case (A)(1) */
			pathend--; /* <--- Ouch! */
		/* Else, case (B)(1), nothing changes */
	}
	else { /* Case (C) */
		while (restpattern[0] == SEP)
			restpattern++;
		if (*restpattern == EOS) /* Cases (1) or (2) -- don't match */
			return 0;
	}
	
	err = glob2(pathbuf, pathend, restpattern, pglob);
	*pathend = SEP; /* Restore slash removed by line 'Ouch!' above */
	return err;
}

#endif /* GLOB_HARMONICA */

STATIC int
glob3(pathbuf, pathend, pattern, restpattern, pglob)
	char *pathbuf; /*[MAXPATHLEN+1]*/
	char *pathend;
	char *pattern, *restpattern;
	glob_t *pglob;
{
	DIR *dirp;
	struct dirent *dp;
	int len;
	int err;
#ifdef GLOB_HARMONICA
	int harmonica = (pglob->gl_flags & GLOB_HARMONICA) &&
				isharmonica(pattern, restpattern);
#endif /* GLOB_HARMONICA */
	
	*pathend = EOS;
	errno = 0;
	if ((dirp = opendir(*pathbuf ? pathbuf : ".")) == NULL) {
		/* TO DO: don't call for ENOENT or ENOTDIR */
		if (pglob->gl_errfunc != NULL &&
				(*pglob->gl_errfunc)(pathbuf, errno) != 0 ||
				(pglob->gl_flags & GLOB_ERR))
			return GLOB_ABEND;
		else
			return 0;
	}
	
	err = 0;
	
	/* Search directory for matching names */
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == DOT && *pattern != DOT)
			continue; /* Initial DOT must be matched literally */
		if (
#ifdef GLOB_HARMONICA
			!harmonica &&
#endif
			!match(dp->d_name, pattern, restpattern))
			continue; /* No match */
		len = strlen(dp->d_name);
		strcpy(pathend, dp->d_name);
		err = glob2(pathbuf, pathend+len, restpattern, pglob);
		if (err != 0)
			break;
#ifdef GLOB_HARMONICA
		if (harmonica) {
			pathend[len] = EOS;
			if (isdir(pathbuf)) {
				pathend[len++] = SEP;
				pathend[len] = EOS;
				err = glob3(pathbuf, pathend+len,
					pattern, restpattern, pglob);
				if (err != 0)
					break;
			}
		}
#endif /* GLOB_HARMONICA */
	}
	/* TO DO: check error from readdir? */
	
	closedir(dirp);
	return err;
}


#ifdef GLOB_HARMONICA

/* Test for harmonica pattern (a pattern consisting of exactly two stars) */

STATIC bool_t
isharmonica(pat, patend)
	char *pat, *patend;
{
	return (pat[0] & 0xff) == M_ALL && (pat[1] & 0xff) == M_ALL &&
			pat+2 == patend;
}

/* Test for directory -- use lstat() to avoid endless recursion */

STATIC bool_t
isdir(path)
	char *path;
{
	struct stat sbuf;
	
	if (lstat(path, &sbuf) < 0)
		return FALSE;
	return (sbuf.st_mode & S_IFMT) == S_IFDIR;
}

#endif /* GLOB_HARMONICA */


/* Extend the gl_pathv member of a glob_t structure to accomodate
   a new item, add the new item, and update gl_pathc.
   
   *** This assumes the BSD realloc, which only copies the block
   when its size crosses a power-of-two boundary; for v7 realloc,
   this would cause quadratic behavior. ***
   
   Return 0 if new item added, error code if memory couldn't be allocated.
   
   Invariant of the glob_t structure:
   
	Either gl_pathc is zero and gl_pathv is NULL; or
	gl_pathc > 0 and gl_pathv points to (gl_offs + gl_pathc + 1) items.
*/

STATIC int
globextend(path, pglob)
	char *path;
	glob_t *pglob;
{
	register char **pathv;
	register int i;
	unsigned int newsize;
	unsigned int copysize;
	char *copy;
	
	newsize = sizeof(*pathv) * (2 + pglob->gl_pathc + pglob->gl_offs);
	if ((pathv = pglob->gl_pathv) == NULL)
		pathv = (char **)malloc(newsize);
	else
		pathv = (char **)realloc((char *)pathv, newsize);
	if (pathv == NULL)
		return GLOB_NOSPACE;
	
	if (pglob->gl_pathv == NULL && pglob->gl_offs > 0) {
		/* First time around -- clear initial gl_offs items */
		pathv += pglob->gl_offs;
		for (i = pglob->gl_offs; --i >= 0; )
			*--pathv = NULL;
	}
	pglob->gl_pathv = pathv;
	
	copysize = strlen(path) + 1;
	if ((copy = malloc(copysize)) != NULL) {
		strcpy(copy, path);
		pathv[pglob->gl_offs + pglob->gl_pathc++] = copy;
	}
	pathv[pglob->gl_offs + pglob->gl_pathc] = NULL;
	
	return (copy == NULL) ? GLOB_NOSPACE : 0;
}


/* Pattern matching function for filenames.
   Each occurrence of the * pattern causes a recursion level. */

STATIC bool_t
match(name, pat, patend)
	register char *name;
	register char *pat, *patend;
{
	char c;
	
	while (pat < patend) {
		c = *pat++;
		switch (c & 0xff) {

		case M_ONE:
			if (*name++ == EOS)
				return FALSE;
			break;

		case M_ALL:
			if (pat == patend)
				return TRUE;
			for (; *name != EOS; ++name) {
				if (match(name, pat, patend))
					return TRUE;
			}
			return FALSE;

		case M_SET:
		    {
			char k;
			bool_t ok;
			bool_t negate_range;
			
			ok = FALSE;
			k = *name++;
			if (negate_range = (*pat & 0xff) == M_NOT)
				++pat;
			while (((c = *pat++) & 0xff) != M_END) {
				if ((*pat & 0xff) == M_RNG) {
					if (c <= k && k <= pat[1])
						ok = TRUE;
					pat += 2;
				}
				else if (c == k)
					ok = TRUE;
			}
			if (ok == negate_range)
				return FALSE;
			break;
		    }

		default:
			if (*name++ != c)
				return FALSE;
			break;

		}
	}
	return *name == EOS;
}


/* Free allocated data belonging to a glob_t structure.
   This is part of the Posix interface. */

void
globfree(pglob)
	glob_t *pglob;
{
	if (pglob->gl_pathv != NULL) {
		char **pp = pglob->gl_pathv + pglob->gl_offs;
		int i;
		for (i = 0; i < pglob->gl_pathc; ++i) {
			if (*pp != NULL)
				free(*pp);
		}
		free((char *)pp);
	}
}
