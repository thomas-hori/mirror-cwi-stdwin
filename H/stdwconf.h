/* "stdwconf.h" -- define/derive feature test macros.

   This file is supposed to be portable between all OSes,
   and may be included by applications (though it is not automatically
   included by stdwin.h).  It just so happens that on UNIX, feature
   test macros are now computed by the configure script, so all that's
   left in this file is Mac specific....

   It is also reentrant.
*/

/* MPW defines "applec" and "macintosh"; THINK C defines "THINK_C";
   metrowerks defines "__MWERKS__".  We want "macintosh" defined for
   any Mac implementation, and "MPW" for MPW.   The other compilers
   are recognized by their own defines ("THINK_C" or "__MWERKS__").
*/

/* If "applec" is defined, it must be MPW */
#ifdef applec
#ifndef MPW
#define MPW
#endif
#endif

/* Under THINK C, define "macintosh" if not already defined */
#ifdef THINK_C
#ifndef macintosh
#define macintosh
#endif
#endif

/* Under MetroWerks, define "macintosh" if not already defined */
#ifdef __MWERKS__
#ifndef macintosh
#define macintosh
#endif
#endif
