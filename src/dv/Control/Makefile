ifeq ($(shell uname), Linux)
include Makefile.linux
else ifeq ($(shell uname), Darwin)
include Makefile.darwin
else

# Explicit setting of SHELL - beware since this gets inherited
# by commands invoked by Make
SHELL=/bin/sh

UILFLAGS=-I/usr/include/uil
MRMLIBS=-lMrm


# solaris 2.x
XINCLUDES=-I/usr/dt/include -I/usr/openwin/include -I/usr/openwin/include/X11
XLIBS=-L/usr/dt/lib -L/usr/openwin/lib -R/usr/dt/lib -R/usr/openwin/lib 
LDLIBS=-lgen -lposix4
CCC=CC
CC=gcc
MRMLIBS=-L/usr/dt/lib -lMrm
UILFLAGS=-I/usr/include/uil -I/usr/dt/include/uil
VISUROOT1=/apps/workshop5/SUNWspro/WS5.0
#VISUROOT=/opt/CDS/e/visual/SUNWspro/WS4.0
VISUROOT=$(VISUROOT1)
USRLIB = -L../Lib/UTC_GPS -ltai 

##
JAVA_WIDGETS = ${VISUROOT1}/src/java_widgets
JAVA_WIDGETS_OBJ = ${VISUROOT1}/src/java_widgets/lib
XINCLUDES+=-I${JAVA_WIDGETS}/h
#LDLIBS+=-L${JAVA_WIDGETS_OBJ} -ljavawidgets


#LDLIBS = -lPW
# Program configuration
# ---------------------

XPMLIBDIR = ${VISUROOT}/user_widgets/obj
XPMDIR = ${VISUROOT}/contrib/xpm/lib
LDFLAGS = ${XLIBS} -L${XPMLIBDIR}
MOTIFLIBS = -lXm -lXt -lX11

XPCLASS = $(VISUROOT)/src/xdclass
XPCLASSLIBS = $(XPCLASS)/lib/libxdclass.a


CFLAGS=-g -I../../daqd -I. ${XINCLUDES} -I${XPMDIR}
CCFLAGS=-g ${CFLAGS} -I${XPCLASS}/h

UIL=uil

#MOTIFLIBS = -lXm -lXt -lX11 -lPW

#DO NOT EDIT >>>
XD_C_PROGRAMS=\
		dc3

#<<< DO NOT EDIT

#DO NOT EDIT >>>
XD_C_PROGRAM_OBJECTS=\
		dc3.o

#<<< DO NOT EDIT

#DO NOT EDIT >>>
XD_C_PROGRAM_SOURCES=\
		dc3.c

#<<< DO NOT EDIT

#DO NOT EDIT >>>
XD_C_STUB_OBJECTS=\
		dc3_stubs.o

#<<< DO NOT EDIT

#DO NOT EDIT >>>
XD_C_STUB_SOURCES=\
		dc3_stubs.c

#<<< DO NOT EDIT

XD_ALL_C_SOURCES=$(XD_C_PROGRAM_SOURCES) $(XD_C_SOURCES) $(XD_C_STUB_SOURCES) $(XD_C_FOR_UIL_PROGRAM_SOURCES) $(XD_C_FOR_UIL_SOURCES)

XD_ALL_CC_SOURCES=$(XD_CC_PROGRAM_SOURCES) $(XD_CC_SOURCES) $(XD_CC_STUB_SOURCES)

all: CHECKENV CHECKROOT $(XD_C_PROGRAMS) $(XD_CC_PROGRAMS) $(XD_C_FOR_UIL_PROGRAMS) $(XD_UIL_OBJECTS)

depend:
	makedepend -- $(CFLAGS) $(CPPFLAGS) -- $(XD_ALL_C_SOURCES)
	makedepend -a -- $(CCFLAGS) $(CPPFLAGS) -- $(XD_ALL_CC_SOURCES)

clean:
	rm -f $(XD_C_PROGRAMS) $(XD_C_PROGRAM_OBJECTS) $(XD_C_OBJECTS) \
	      $(XD_CC_PROGRAMS) $(XD_CC_PROGRAM_OBJECTS) $(XD_CC_OBJECTS) \
	      $(XD_C_STUB_OBJECTS) \
	      $(XD_CC_STUB_OBJECTS) \
	      $(XD_C_FOR_UIL_PROGRAMS) $(XD_C_FOR_UIL_PROGRAM_OBJECTS) $(XD_C_FOR_UIL_OBJECTS) \
	      $(XD_UIL_OBJECTS) *.o

CHECKENV:
	@test -n "$(VISUROOT)" || (echo You must set \$$VISUROOT in the makefile or in your shell environment; exit 1)

CHECKROOT:
	#@test -d "$(XPCLASS)" || (echo \$$VISUROOT must point to a valid root directory; exit 1)
	#@test -d "$(VISUROOT)/make_templates" || (echo \$$VISUROOT must point to a valid root directory; exit 1)

XD_C_OBJECTS += main.o

#DO NOT EDIT >>>
dc3: dc3.o $(XD_C_OBJECTS) $(XD_C_STUB_OBJECTS) TabBook.o ../Th/datasrv.o ../Th/daqc_access.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o dc3 dc3.o TabBook.o $(XD_C_OBJECTS) $(XD_C_STUB_OBJECTS) $(MOTIFLIBS) $(LDLIBS) ../Th/datasrv.o ../Th/daqc_access.o $(USRLIB) -lpthread -lm -lnsl -lsocket 
#<<< DO NOT EDIT

.c.o:
	$(CC) -DMOTIF $(CFLAGS) $(CPPFLAGS) -c $<
dc3_stubs.o: dc3_stubs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c dc3_stubs.c 
endif
# DO NOT DELETE
