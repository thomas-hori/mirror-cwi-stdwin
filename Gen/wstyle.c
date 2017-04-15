/* STDWIN -- TEXT ATTRIBUTES. */

#include "tools.h"
#include "stdwin.h"
#include "style.h"

TEXTATTR wattr;

/* Setting text drawing parameters. */

void
wsetplain()
{
	wattr.style= PLAIN;
}

void
wsethilite()
{
	wattr.style |= HILITE;
}

void
wsetinverse()
{
	wattr.style |= INVERSE;
}

void
wsetitalic()
{
	wattr.style |= ITALIC;
}

void
wsetbold()
{
	wattr.style |= BOLD;
}

void
wsetbolditalic()
{
	wattr.style |= BOLD|ITALIC;
}

void
wsetunderline()
{
	wattr.style |= UNDERLINE;
}

void
wgettextattr(attr)
	TEXTATTR *attr;
{
	*attr= wattr;
}

void
wsettextattr(attr)
	TEXTATTR *attr;
{
	wattr= *attr;
}
