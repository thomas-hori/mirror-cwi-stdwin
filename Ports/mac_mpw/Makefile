# Makefile for stdwin, generated under Unix

SOURCES= ¶
	about.c ¶
	argcargv.c ¶
	caret.c ¶
	cursor.c ¶
	dialog.c ¶
	dprintf.c ¶
	draw.c ¶
	event.c ¶
	fullpath.c ¶
	menu.c ¶
	pstring.c ¶
	scrap.c ¶
	scroll.c ¶
	stdwin.c ¶
	strdup.c ¶
	timer.c ¶
	wtextbreak.c ¶
 

HEADERS= ¶
	macwin.h ¶
	menu.h ¶
 

OBJECTS= ¶
	about.c.o ¶
	argcargv.c.o ¶
	caret.c.o ¶
	cursor.c.o ¶
	dialog.c.o ¶
	dprintf.c.o ¶
	draw.c.o ¶
	event.c.o ¶
	fullpath.c.o ¶
	getopt.c.o ¶
	menu.c.o ¶
	pstring.c.o ¶
	scrap.c.o ¶
	scroll.c.o ¶
	set_open_hook.c.o ¶
	stdwin.c.o ¶
	strdup.c.o ¶
	timer.c.o ¶
	wtextbreak.c.o ¶
 

COptions= -i : -i ::h:

lib.o Ä {OBJECTS}
	Lib -o lib.o -sn Main=STDWIN {OBJECTS}

clean Ä {OBJECTS}
	delete {OBJECTS}

SysLibes= ¶
	{CLibraries}CInterface.o ¶
	{CLibraries}CRuntime.o ¶
	{CLibraries}CSANELib.o ¶
	{CLibraries}StdClib.o

test2 Ä test2.c.o lib.o
	Link -o test2 -t APPL -c xxxx -p test2.c.o lib.o {SysLibes}
