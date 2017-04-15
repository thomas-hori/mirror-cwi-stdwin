/* This software is copyright (c) 1986 by Stichting Mathematisch Centrum.
 * You may use, modify and copy it, provided this notice is present in all
 * copies, modified or unmodified, and provided you don't make money for it.
 *
 * Written 86-jun-28 by Guido van Rossum, CWI, Amsterdam <guido@mcvax.uucp>
 */

/*
 * 'Glob' routine -- match *, ? and [...] in filenames.
 * Extra services: initial ~username is replaced by username's home dir,
 * initial ~ is replaced by $HOME, initial $var is replaced by
 * return value of getenv("var").
 * Quoting convention: \ followed by a magic character inhibits its
 * special meaning.
 * Nonexisting $var is treated as empty string; nonexisting ~user
 * is copied literally.
 */

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/dir.h>

struct passwd *getpwnam();
char *getenv();
char *malloc();
char *realloc();

#define EOS '\0'
#define SEP '/'
#define DOT '.'
#define QUOTE '\\'

#define BOOL int
#define NO 0
#define YES 1

#define MAXPATH 1024
#define MAXBASE 256

#define META(c) ((char)((c)|0200))
#define M_ALL META('*')
#define M_ONE META('?')
#define M_SET META('[')
#define M_RNG META('-')
#define M_END META(']')

struct flist {
	int len;
	char **item;
};

/* Make a null-terminated string of the chars between two pointers */
/* (Limited length, static buffer returned) */

static char *
makestr(start, end)
	char *start;
	char *end;
{
	static char buf[100];
	char *p= buf;
	
	while (start < end && p < &buf[100])
		*p++ = *start++;
	*p= EOS;
	return buf;
}

/* Append string to buffer, return new end of buffer.  Guarded. */

static char *
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

/* Form a pathname by concatenating head, maybe a / and tail. */
/* Truncates to space available. */

static void
formpath(dest, head, tail, size)
	char *dest;
	char *head;
	char *tail;
	unsigned int size; /* sizeof dest */
{
	char *start= dest;
	
	for (;;) {
		if (--size == 0)
			goto out;
		if ((*dest++ = *head++) == EOS)
			break;
	}
	if (*tail != EOS && size != 0) {
		--dest;
		++size;
		if (dest > start && dest[-1] != SEP) {
			*dest++ = SEP;
			--size;
		}
		for (;;) {
			if (--size == 0)
				goto out;
			if ((*dest++ = *tail++) == EOS)
				break;
		}
	}
	return;
	
 out:	*dest= EOS;
}

/* Find a user's home directory, NULL if not found */

static char *
gethome(username)
	char *username;
{
	struct passwd *p;
	
	p= getpwnam(username);
	if (p == NULL)
		return NULL;
	return p->pw_dir;
}

/* String compare for qsort */

static int
compare(p, q)
	char **p;
	char **q;
{
	return strcmp(*p, *q);
}

/*
 * Maintain lists of strings.
 * When memory is full, data is lost but 
 */

static void
addfile(list, head, tail)
	struct flist *list;
	char *head;
	char *tail;
{
	char *str;
	
	str= malloc((unsigned) strlen(head) + strlen(tail) + 2);
	
	if (str == NULL)
		return;
	if (list->item == 0) {
		list->len= 0;
		list->item= (char**) malloc(sizeof(char*));
	}
	else {
		list->item= (char**) realloc((char*)list->item,
				(unsigned) (list->len+1)*sizeof(char*));
		if (list->item == 0)
			list->len= 0;
	}
	if (list->item != NULL) {
		formpath(str, head, tail, MAXPATH);
		list->item[list->len++]= str;
	}
	else
		free(str);
}

/* Clear the fields of a struct flist before first use */

static void
clear(list)
	struct flist *list;
{
	list->len= 0;
	list->item= NULL;
}

/* Free memory held by struct flist after use */

static void
discard(list)
	struct flist *list;
{
	int i= list->len;
	
	if (list->item != NULL) {
		while (--i >= 0) {
			if (list->item[i] != NULL)
				free(list->item[i]);
		}
		free((char*)list->item);
		list->item= NULL;
	}
	list->len= 0;
}

/* Pattern matching function for filenames */
/* Each occurrence of the * pattern causes a recursion level */

static BOOL
match(name, pat)
	char *name;
	char *pat;
{
	char c, k;
	BOOL ok;
	
	while ((c= *pat++) != EOS) {
		switch (c) {

		case M_ONE:
			if (*name++ == EOS)
				return NO;
			break;

		case M_ALL:
			if (*pat == EOS)
				return YES;
			for (; *name != EOS; ++name) {
				if (match(name, pat))
					return YES;
			}
			return NO;

		case M_SET:
			ok= NO;
			k= *name++;
			while ((c= *pat++) != M_END) {
				if (*pat == M_RNG) {
					if (c <= k && k <= pat[1])
						ok= YES;
					pat += 2;
				}
				else if (c == k)
					ok= YES;
			}
			if (!ok)
				return NO;
			break;

		default:
			if (*name++ != c)
				return NO;
			break;

		}
	}
	return *name == EOS;
}

/* Perform pattern matching for one segment of the pathname */

static void
segment(files, mid, pat)
	struct flist *files;
	char *mid;
	char *pat;
{
	char path[MAXPATH];
	int i;
	DIR *dirp;
	struct direct *dp;
	struct flist new;
	
	clear(&new);
	for (i= 0; i < files->len; ++i) {
		strcpy(path, files->item[i]);
		strcat(path, mid);
		free(files->item[i]);
		files->item[i]= NULL;
		dirp= opendir(path);
		if (dirp == NULL)
			continue;
		while ((dp= readdir(dirp)) != NULL) {
			if (*dp->d_name == DOT && *pat != DOT)
				; /* No meta-match on initial '.' */
			else if (match(dp->d_name, pat))
				addfile(&new, path, dp->d_name);
		}
		closedir(dirp);
	}
	discard(files);
	*files= new;
}

/* Finish by matching the rest of the pattern (which has no metas) */

static void
findfiles(files, tail)
	struct flist *files;
	char *tail;
{
	int i;
	struct flist new;
	char path[MAXPATH];

	if (*tail == EOS || files->len == 0)
		return;
	clear(&new);
	for (i= 0; i < files->len; ++i) {
		strcpy(path, files->item[i]);
		strcat(path, tail);
		free(files->item[i]);
		files->item[i]= NULL;
		if (access(path, 0) == 0)
			addfile(&new, path, "");
	}
	discard(files);
	*files= new;
}

static void
glob1(pat, files)
	char *pat;
	struct flist *files;
{
	char mid[MAXPATH];
	char *end= mid;
	char *basestart= mid;
	BOOL meta= NO;
	char c;
	
	clear(files);
	addfile(files, "", "");
	for (;;) {
		switch (c= *pat++) {

		case EOS:
		case SEP:
			*end= EOS;
			if (meta) {
				if (basestart == mid)
					segment(files, "", basestart);
				else if (basestart == mid+1) {
					static char sepstr[]= {SEP, EOS};
					segment(files, sepstr, basestart);
				}
				else {
					basestart[-1]= EOS;
					segment(files, mid, basestart);
				}
				if (files->len == 0)
					return;
				end= basestart= mid;
				meta= NO;
			}
			else if (c == EOS)
				findfiles(files, mid);
			if (c == EOS)
				return;
			*end++= c;
			basestart= end;
			break;

		case M_ALL:
		case M_ONE:
		case M_SET:
			meta= YES;
			/* Fall through */
		default:
			*end++ = c;

		}
	}
}

/*
 * The main 'glob' routine: does $ and ~ substitution and \ quoting,
 * and then calls 'glob1' to do the pattern matching.
 * Returns 0 if file not found, number of matches otherwise.
 * The matches found are appended to the buffer, separated by
 * EOS characters.  If no matches were found, the pattern is copied
 * to the buffer after $ and ~ substitution and \ quoting.
 */

int
glob(pat, buf, size)
	char *pat;
	char *buf;
	unsigned int size; /* sizeof buf */
{
	char *p, *q;
	char c;
	struct flist files;
	char *start= buf;
	char *end= buf+size;
	
	c= *pat;
	if (c == '~') {
		p= ++pat;
		while (*p != EOS && *p != SEP)
			++p;
		if (p == pat) {
			q= getenv("HOME");
			if (q == NULL)
				--pat;
			else
				buf= addstr(buf, q, end);
		}
		else {
			q= gethome(makestr(pat, p));
			if (q == NULL)
				--pat;
			else {
				buf= addstr(buf, q, end);
				pat= p;
			}
		}
	}
	else if (c == '$') {
		p= ++pat;
		while (isalnum(*p) || *p == '_')
			++p;
		q= getenv(makestr(pat, p));
		if (q != NULL)
			buf= addstr(buf, q, end);
		pat= p;
	}
	else if (c == QUOTE && (pat[1] == '$' || pat[1] == '~'))
		++pat;
	
	while (buf < end && (c= *pat++) != EOS) {
		switch (c) {
		
		case QUOTE:
			if ((c= *pat++) != EOS && index("\\*?[", c) != NULL)
				*buf++ = c;
			else {
				*buf++ = QUOTE;
				--pat;
			}
			break;
		
		case '*':
			*buf++ = M_ALL;
			break;
		
		case '?':
			*buf++ = M_ONE;
			break;
		
		case '[':
			if (*pat == EOS || index(pat+1, ']') == NULL) {
				*buf++ = c;
				break;
			}
			*buf++ = M_SET;
			c= *pat++;
			do {
				*buf++ = c;
				if (*pat == '-' && (c= pat[1]) != ']') {
					*buf++ = M_RNG;
					*buf++= c;
					pat += 2;
				}
			} while ((c= *pat++) != ']');
			*buf++ = M_END;
			break;
		
		default:
			*buf++ = c;
			break;

		}
	}
	*buf= EOS;
	
	glob1(start, &files);
	
	if (files.len == 0) {
		/* Change meta characters back to printing characters */
		for (buf= start; *buf != EOS; ++buf) {
			if (*buf & 0200)
				*buf &= ~0200;
		}
		return 0; /* No match */
	}
	else {
		int i, len;
		
		qsort((char*)files.item, files.len, sizeof(char*), compare);
		buf= start;
		*buf= EOS;
		for (i= 0; i < files.len; ++i) {
			len= strlen(files.item[i]);
			if (len+1 > size)
				break;
			strcpy(buf, files.item[i]);
			buf += len+1;
			size -= len+1;
		}
		discard(&files);
		return i;
	}
}

#ifdef MAIN
/*
 * Main program to test 'glob' routine.
 */

main(argc, argv)
	int argc;
	char **argv;
{
	int i, j, n;
	char *p;
	char buffer[10000];

	if (argc < 2) {
		fprintf(stderr, "usage: %s pattern ...\n", argv[0]);
		exit(2);
	}
	for (i= 1; i < argc; ++i) {
		printf("%s: ", argv[i]);
		n= glob(argv[i], buffer, sizeof buffer);
		printf("%d\n", n);
		p= buffer;
		if (n == 0)
			++n;
		for (j= 0; j < n; ++j) {
			printf("\t%s\n", p);
			p += strlen(p) + 1;
		}
	}
	exit(0);
}
#endif
