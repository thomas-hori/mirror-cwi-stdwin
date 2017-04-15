/* The type of signal handlers is somewhat problematic.
   This file encapsulates my knowledge about it:
   - on the Mac (THINK C), it's int
   - on other systems, it's usually void, except it's int on vax Ultrix.
   Pass -DSIGTYPE=... to cc if you know better. */

#ifndef SIGTYPE

#ifdef THINK_C

#define SIGTYPE int

#else /* !THINK_C */

#if defined(vax) && !defined(AMOEBA)
#define SIGTYPE int
#else
#define SIGTYPE void
#endif

#endif /* !THINK_C */

#endif /* !SIGTYPE */
