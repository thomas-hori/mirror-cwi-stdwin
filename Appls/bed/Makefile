# Begin Generic
STDWIN=		..
CONF=		$(STDWIN)/conf
H=		$(STDWIN)/h
TOOLS=		$(STDWIN)/tools
ALFA=		$(STDWIN)/alfa
MG1=		$(STDWIN)/mg1
X11=		$(STDWIN)/x11
X11LIBS=	$(X11)/lib.a $(LIBX11)
LIBX11=		$(LIBXLIB)/libX11.a
LIBXT=		$(LIBXLIB)/libXaw.a $(LIBXLIB)/libXt.a
XTLIBS=		$(STDWIN)/xt/lib.a $(LIBXT) $(LIBX11)
STUBS=		$(STDWIN)/stubs
INCLUDES=	-I$(H) $(SYSINCLUDES) $(SITEINCLUDES) $(LINCLUDES)
DEFS=		$(SYSDEFS) $(SITEDEFS) $(LDEFS)
CPPFLAGS=	$(DEFS) $(INCLUDES)
OPTIONS=	$(SYSOPTIONS) $(SITEOPTIONS) $(LOPTIONS)
CFLAGS=		$(CPPFLAGS) $(OPTIONS)
LDFLAGS=	$(OPTIONS)
LINTFLAGS=	-bhv
LINT=		lint
# End Generic
# Begin Sys.bsd42am
SYSINCLUDES=	
SYSDEFS=	
SYSOPTIONS=	-g
# End Sys.bsd42am
LIBXLIB=	/usr/local/X/lib
# Begin Make.conf

SRCS=		bed.c file.c fmenu.c mouse.c mmenu.c opmenu.c
OBJS=		bed.o file.o fmenu.o mouse.o mmenu.o opmenu.o
PROG=		bed

$(PROG).X:	$(OBJS) $(X11)/lib.a
		$(CC) -o $(PROG).X $(OBJS) $(X11LIBS)

all:		$(PROG)

$(PROG).A:	$(OBJS) $(ALFA)/lib.a
		$(CC) -o $(PROG).A $(OBJS) $(ALFA)/lib.a -ltermcap

$(PROG).stubs:	$(OBJS) $(STUBS)/lib.a
		$(CC) -o $(PROG).stubs $(OBJS) $(STUBS)/lib.a -lamunix

$(PROG).M:	$(OBJS) $(MG1)/lib.a
		$(CC) -o $(PROG).M $(OBJS) $(MG1)/lib.a $(WINLIBS)

# End Make.conf
# Begin Rules
all:		Makefile

Makefile:	$(CONF)/Generic $(CONF)/Rules $(CONF)/mkmk Make.conf
		@echo "Makefile out-of-date -- rerun $(CONF)/mkmk"; exit 1

depend:		$(SRCS)
		$(CONF)/mkdep $(CPPFLAGS) $(SRCS) >@mkdep

tags:		$(SRCS) $(HDRS)
		ctags -w $(SRCS) $(HDRS)

clean::
		-rm -f core *.o [,@#]*

clobber::	clean
		-rm -f lib.a tags *.[A-Z]

lint:		$(SRCS) $(HDRS)
		$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(SRCS) $(LINTLIBS)
# End Rules
bed.o: bed.c
bed.o: /usr/include/stdio.h
bed.o: /usr/include/ctype.h
bed.o: ./bed.h
bed.o: ../h/stdwin.h
bed.o: ../h/stdevent.h
bed.o: ../h/stdtext.h
bed.o: ../h/tools.h
bed.o: ../h/configure.h
bed.o: /usr/include/stdio.h
bed.o: /usr/include/ctype.h
bed.o: /usr/include/strings.h
bed.o: ../h/l_defs.h
bed.o: ./menu.h
file.o: file.c
file.o: /usr/include/stdio.h
file.o: ./bed.h
file.o: ../h/stdwin.h
file.o: ../h/stdevent.h
file.o: ../h/stdtext.h
file.o: ../h/tools.h
file.o: ../h/configure.h
file.o: /usr/include/stdio.h
file.o: /usr/include/ctype.h
file.o: /usr/include/strings.h
file.o: ../h/l_defs.h
fmenu.o: fmenu.c
fmenu.o: /usr/include/stdio.h
fmenu.o: ./bed.h
fmenu.o: ../h/stdwin.h
fmenu.o: ../h/stdevent.h
fmenu.o: ../h/stdtext.h
fmenu.o: ../h/tools.h
fmenu.o: ../h/configure.h
fmenu.o: /usr/include/stdio.h
fmenu.o: /usr/include/ctype.h
fmenu.o: /usr/include/strings.h
fmenu.o: ../h/l_defs.h
fmenu.o: ./menu.h
mouse.o: mouse.c
mouse.o: ./bed.h
mouse.o: ../h/stdwin.h
mouse.o: ../h/stdevent.h
mouse.o: ../h/stdtext.h
mouse.o: ../h/tools.h
mouse.o: ../h/configure.h
mouse.o: /usr/include/stdio.h
mouse.o: /usr/include/ctype.h
mouse.o: /usr/include/strings.h
mouse.o: ../h/l_defs.h
mouse.o: ./menu.h
mmenu.o: mmenu.c
mmenu.o: ./bed.h
mmenu.o: ../h/stdwin.h
mmenu.o: ../h/stdevent.h
mmenu.o: ../h/stdtext.h
mmenu.o: ../h/tools.h
mmenu.o: ../h/configure.h
mmenu.o: /usr/include/stdio.h
mmenu.o: /usr/include/ctype.h
mmenu.o: /usr/include/strings.h
mmenu.o: ../h/l_defs.h
mmenu.o: ./menu.h
opmenu.o: opmenu.c
opmenu.o: ./bed.h
opmenu.o: ../h/stdwin.h
opmenu.o: ../h/stdevent.h
opmenu.o: ../h/stdtext.h
opmenu.o: ../h/tools.h
opmenu.o: ../h/configure.h
opmenu.o: /usr/include/stdio.h
opmenu.o: /usr/include/ctype.h
opmenu.o: /usr/include/strings.h
opmenu.o: ../h/l_defs.h
opmenu.o: ./menu.h
