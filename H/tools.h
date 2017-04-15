/* Useful includes, definitions etc. */

#include "stdwconf.h"	/* Figure out on which system we are */
#include "_ARGS.h"	/* Define _ARGS() macro appropriately */


/****************************/
/* Auto-configuration tests */
/****************************/

#ifdef macintosh
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_STRCHR
#define HAVE_STRRCHR
#define HAVE_MEMCMP
#define HAVE_MEMCPY
#define VOID_PTR
#endif

#ifdef SYSV
#define VOID_PTR
#endif

#ifdef AMOEBA
/* Amoeba has its own way of telling which one we want: */
#ifndef VOIDSTAR
#define VOID_PTR
#endif
#endif

#ifdef __STDC__
#ifndef VOID_PTR
#define VOID_PTR
#endif
#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H
#endif
#endif


/*********************/
/* Universal pointer */
/*********************/

#ifdef VOID_PTR
#define UNIVPTR void *
#else
#define UNIVPTR char *
#endif


/*************/
/* C library */
/*************/

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

/* Systems that don't have <string.h> should at least have <strings.h> */
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

/* Systems that don't have str(r)chr should at least have (r)index */
#ifndef HAVE_STRCHR
#define strchr index
#endif
#ifndef HAVE_STRRCHR
#define strrchr rindex
#endif

/* Systems that don't have memcpy/memcmp should at least have bcopy/bcmp */
#ifndef HAVE_MEMCPY
#define memcpy(dest, src, n)	bcopy(src, dest, n)
#endif
#ifndef HAVE_MEMCMP
#define memcmp(a, b, cnt)	bcmp(a, b, cnt)
#endif


#ifdef HAVE_STDLIB_H

#include <stdlib.h>

#else

#include <sys/types.h> /* For size_t -- hope it doesn't break other things */

UNIVPTR malloc _ARGS((size_t));
UNIVPTR calloc _ARGS((size_t, size_t));
UNIVPTR realloc _ARGS((UNIVPTR, size_t));

#ifndef NO_VOID_FREE
void free _ARGS((UNIVPTR));
#endif

#ifndef NO_VOID_EXIT
void exit _ARGS((int));
#endif

char *getenv _ARGS((char *));

#endif /* !HAVE_STDLIB_H */

/* According to the C Standard, errno may be a macro on systems with
   multiple threads.  But on older systems, it may not be declared at
   all in <errno.h>.  So we declare it here, except if it is a macro. */

#ifndef errno
extern int errno;
#endif


/*************************************/
/* Miscellaneous useful declarations */
/*************************************/

/* Interface to getopt(): */
extern int optind;
extern char *optarg;
int getopt (); /* _ARGS((int, char **, char *)); */

/* Boolean data type: */
#define bool int	/* For single variable, argument or return value */
#define tbool char	/* Tiny bool, used in structs or arrays */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* Character shorthands: */
#define EOS '\0'
#define EOL '\n'

/* Copy string to malloc'ed memory: */
char *strdup _ARGS((const char *));

/* Other useful macros: */

#define CNTRL(x) ((x) & 0x1f) /* Usage: CNTRL('X') */

#define ABS(x) ((x) < 0 ? -(x) : (x))

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define CLIPMIN(var, min) if ((var) >= (min)) ; else (var)= (min)
#define CLIPMAX(var, max) if ((var) <= (max)) ; else (var)= (max)

/* Memory allocation macros: */

#define ALLOC(type) ((type*) malloc(sizeof(type)))
#define FREE(p) if ((p) != 0) { free((UNIVPTR)p); p = 0; } else /*empty*/

/* Array (re)allocation macros.
   RESIZE yields nonzero if the realloc succeeded. */

#define NALLOC(type, n) ((type*) malloc((unsigned)(n) * sizeof(type)))
#define RESIZE(var, type, n) \
	(var= (type *) realloc((UNIVPTR)var, (n) * sizeof(type)))

/* Dynamic array macros: */

#include "lists.h"
