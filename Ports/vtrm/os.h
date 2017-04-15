/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1991-1994. */

/* Auto-configuration file for vtrm.
   Edit this if you have portability problems. */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* 4.2 BSD select() system call available? (assume yes) */
#define HAVE_SELECT

/* can #include <setjmp.h>? (assume yes) */
#define HAVE_SETJMP_H

/* VOID is used in casts only, for quieter lint */
/* make it empty if your compiler doesn't have void */
#define VOID (void)
