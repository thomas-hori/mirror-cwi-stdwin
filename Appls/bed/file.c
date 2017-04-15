#include <stdio.h>
#include "bed.h"

extern char	*rindex () ;

extern int	sqrsize ;

extern int	map_width ;
extern int	map_height ;

extern char	*raster ;
extern int	raster_lenght ;
extern int	stride ;

extern char	*fname ;
extern char	*title ;

extern WINDOW	*win ;

extern bool	changed ;

char
*strip (fname)
	char	*fname ;
{
	char	*p ;

	if ((p = rindex (fname, '/')) == NULL &&	/* UNIX		      */
	    (p = rindex (fname, '\\')) == NULL &&	/* GEMDOS & MSDOS     */
	    (p = rindex (fname, ':')) == NULL)
		return (fname) ;
	return (p + 1) ;
}

bool
input(fp)
	FILE *fp;
{
	int	value ;
	char	ibuf[81] ;
	char	*id ;
	char	*p ;
	char	*type ;
	int	i ;

	for (;;) {
		if (fgets (ibuf, 81, fp) == NULL)
			/*
			** EOF
			*/
			break ;

		if (strlen (ibuf) == 80) {
			/*
			** Line too long, not a bitmap file
			*/
			fclose (fp) ;
			return (FALSE) ;
		}

		if (strncmp (ibuf, "#define ", 8) == 0) {
			id = ibuf + 8 ;
			if ((type = rindex (id, '_')) == NULL)
				type = id ;
			else
				type++ ;

			if (!strncmp (type, "width", 5)) {
				map_width = atoi (type + 5) ;
			}

			if (!strncmp (type, "height", 6)) {
				map_height = atoi (type + 6) ;
			}

			continue ;
		}

		if (strncmp (ibuf, "static unsigned char ", 21) == 0)
			id = ibuf + 21 ;
		else if (strncmp (ibuf, "static char ", 12) == 0)
			id = ibuf + 12 ;
		else
			continue ;

		if ((type = rindex (id, '_')) == NULL)
			type = id ;
		else
			type++ ;

		if (strncmp (type, "bits[]", 6) != 0)
			continue ;

		newraster () ;

		for (i = 0, p = raster ; i < raster_lenght ; ++i, ++p) {
			if (fscanf (fp, " 0x%2x%*[,}]%*[  \n]", &value) != 1) {
				free (raster) ;
				raster = NULL ;
				fclose (fp) ;
				return (FALSE) ;
			}
			*p = value ;
		}
	}

	fclose (fp) ;
	return (TRUE) ;
}

bool
readbitmap ()
{
	FILE	*fp ;
	char	namebuf[256] ;

	if (fname == NULL) {
		namebuf[0] = 0 ;

		if (!waskfile ("Read file", namebuf, sizeof (namebuf), FALSE))
			return (FALSE) ;

		fname = strdup (namebuf) ;
		title = strip (fname) ;

		wsettitle (win, title) ;
	}

	fp = fopen (fname, "r") ;
	if (fp == NULL) {
		perror(fname);
		return (FALSE) ;
	}

	return (input(fp)) ;
}

void
output(fp)
	FILE	*fp ;
{
	int	i ;

	fprintf (fp, "#define %s_width\t%d\n", title, map_width) ;
	fprintf (fp, "#define %s_height\t%d\n", title, map_height) ;
	fprintf (fp, "static char %s_bits[] {\n   0x%02x", title,
							raster[0] & 0xFF) ;

	for (i = 1 ; i < raster_lenght ; ++i) {
		fprintf (fp, i % 12 ? ", " : ",\n   ") ;
		fprintf (fp, "0x%02x", raster[i] & 0xFF) ;
	}
	fprintf (fp, "};\n") ;
}

bool
writebitmap()
{
	FILE	*fp;
	char	namebuf[256];
	
	namebuf[0] = '\0' ;
	if (fname == NULL) {
		if (!waskfile ("Save as", namebuf, sizeof (namebuf), TRUE))
			return (FALSE) ;
		fname = strdup (namebuf) ;
		title = strip (namebuf) ;
		wsettitle (win, title) ;
	}

	fp = fopen (fname, "w") ;
	if (fp == NULL) {
		wperror (fname) ;
		return (FALSE) ;
	}

	output (fp) ; 
	fclose (fp) ;
	return (TRUE) ;
}

