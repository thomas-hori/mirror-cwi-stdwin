/* Package to manipulate dynamic arrays represented like
   argc/argv (but not necessarily containing character pointers).
   Operations provided are macros that expand to groups of statements;
   their arguments should be side-effect-free.  The arguments argc and
   argv should be single identifiers, since they are assigned to.
   (L_DECLARE expands to a series of declarations.)
   
   This code was written because I found myself writing code very
   similar to it times and times over, or, worse, using fixed-length
   arrays out of laziness.  It's slow for large arrays (although this
   could be improved easily by rounding up the sizes passed to
   malloc/realloc), but this shouldn't be a problem for the applications
   I am currently using it for.  It sure helped me write some array
   processing code faster.
   
   Operations:
   
   L_DECLARE(argc, argv, type)		declare a list with element type 'type'
   L_INIT(argc, argv)			initialize a list
   L_DEALLOC(argc, argv)		deallocate a list
   L_SETSIZE(argc, argv, type, size)	set list to size 'size'
   L_EXTEND(argc, argv, type, count)	append 'count' unitinialized elements
   L_APPEND(argc, argv, type, elem)	append a given element
   L_REMOVE(argc, argv, type, index)	remove element number 'index'
   L_INSERT(argc, argv, type, index, elem)	insert 'elem' at 'index'
   L_SORT(argc, argv, type, compare)	sort the list ('compare' is a function)
   
   (There should also be operations to insert in the middle and to
   remove elements.)
   
   NB: the 'type' argument could be discarded (except for L_DECLARE)
   if we could live with warnings about assignments from malloc/realloc
   to other pointer types.
   
*/

/* This file only works when included by "stdwtools.h" !!! */

/* You could define GOOD_REALLOC if your realloc() calls malloc() when
   passed a NULL pointer */

#ifdef GOOD_REALLOC
#define _REALLOC(p, size) realloc(p, size)
#else
#define _REALLOC(p, size) ((p) ? realloc(p, size) : malloc(size))
#endif

#define L_DECLARE(argc, argv, type)	int argc = 0; type *argv = 0

#define L_INIT(argc, argv)		argc = 0; argv = 0

#define L_DEALLOC(argc, argv)		argc = 0; FREE(argv)

#define L_SETSIZE(argc, argv, type, size) \
	argv = (type *) _REALLOC((UNIVPTR) argv, \
		(unsigned) (size) * sizeof(type)); \
	argc = (argv == 0) ? 0 : size

#define L_EXTEND(argc, argv, type, count) \
	L_SETSIZE(argc, argv, type, argc+count)

#define L_APPEND(argc, argv, type, elem) \
	argv = (type *) _REALLOC((UNIVPTR) argv, \
		(unsigned) (argc+1) * sizeof(type)); \
	if (argv == 0) \
		argc = 0; \
	else \
		argv[argc++] = elem

#define L_REMOVE(argc, argv, type, index) \
	{ \
		int k_; \
		for (k_ = index+1; k_ < argc; ++k_) \
			argv[k_-1] = argv[k_]; \
		L_SETSIZE(argc, argv, type, argc-1); \
	}

#define L_INSERT(argc, argv, type, index, item) \
	{ \
		int k_; \
		L_SETSIZE(argc, argv, type, argc+1); \
		for (k_ = argc-1; k_ > index; --k_) \
			argv[k_] = argv[k_-1]; \
		argv[index] = item; \
	}

#define L_SORT(argc, argv, type, compare) \
	qsort((UNIVPTR)argv, argc, sizeof(type), compare)
