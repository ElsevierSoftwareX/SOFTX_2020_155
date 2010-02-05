SHELL = /bin/sh

bindir = ${exec_prefix}/bin
exec_prefix = ${prefix}
prefix = /cvs/apps/omx1.2
top_srcdir = ..
srcdir = .
VPATH = ./../common:./../common/bsd

.SUFFIXES:
.SUFFIXES: .c .o .lo .d .i .obj

#include ../common.mk

SRC =	mx_stream.c 

# TARGETS does not append $(EXEEXT) because we don't want the GNU make
# rules to apply to Windows.
TARGETS=$(SRC:.c=)

# By using SRC instead of TARGETS, we pick up the Windows rules.
all: $(SRC:.c=$(EXEEXT))

CFLAGS = -I${prefix}/include -I.
CFLAGS += -Wall -g
CFLAGS += $(OPTIM_CFLAGS)
CPPFLAGS = -I../common -I$(top_srcdir)/common -I$(top_srcdir)/common/bsd
#CPPFLAGS += -DMX_THREAD_SAFE=$(MX_THREAD_SAFE)

LDFLAGS = -L${prefix}/lib64 -lmyriexpress -lpthread  -g
#M_OS_MACOSX#LDFLAGS += 

# sem_post used by mx_wakeup_test.c requires the rt library on Solaris

# GNU make rules.
$(TARGETS) : % : %.o $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

# BSD make rules.
#M_MAKE_BSD#.for X in $(TARGETS)
#M_MAKE_BSD#$(X) : $(X).o $(LIB)
#M_MAKE_BSD#	$(CC) -o $@ $^ $(LDFLAGS)
#M_MAKE_BSD#.endfor

# Windows build rules.
%.exe: %.obj $(LIB) mx_uni.obj getopt.obj
	link /OUT:$@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.c.obj:
	$(CC) $(CFLAGS) $(CPPFLAGS) /c $< /Fo$@

POSTPROC = $(SRC:.c=.i)
.PHONY: postproc
postproc: $(POSTPROC)

.c.i:
	$(CC) $(CFLAGS) $(CPPFLAGS) -E $< -o $@

DEP_CPPFLAGS = $(CPPFLAGS)
DEP_SRC = $(SRC)
DEP_CC = $(CC)
DEP_OBJEXT = $(OBJEXT)
#include ../dependencies.mk

.PHONY: clean
clean:
	rm -f *.i
	rm -f *.o
	rm -f *.obj
	rm -f *.pdb
	rm -f *.exe

.PHONY: distclean
distclean: clean
	rm -f *~ *.d $(SRC:.c=$(EXEEXT))
