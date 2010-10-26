# Generated automatically from Makefile.in by configure.
#
#		Makefile for Data Acquisition Daemon
#
# ==============================================================================

# Multi-processor `C'
.SUFFIXES: .mpc

SHELL = /bin/sh

top_srcdir=.
srcdir=.

prefix      = /usr/local
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin
infodir     = ${prefix}/info
mandir      = ${prefix}/man
infodir     = ${prefix}/info
libdir      = ${exec_prefix}/lib

CXX=c++
CCC=${CXX}
#MPC=cc -c -fast -xO4 -xautopar -xloopinfo
CC=gcc 
LIBS=-lsocket -lnsl 

DEBUGFLAGS=-g
PERFORMANCEFLAGS=-DNDEBUG 
RELEASEFLAGS=-DNDEBUG -DNPROBE -O5

#INCLUDEFLAGS=-I. -I. -I$(HOME)/include -I/usr/local/include
INCLUDEFLAGS=-I../../daqd
CFLAGS= ${INCLUDEFLAGS}  -g 
CCFLAGS=${CFLAGS}

all: datasrv.o daqc_access.o

clean:
	-/bin/rm  -f *.o


test: test.o datasrv.o daqc_access.o
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o test test.o datasrv.o daqc_access.o -lpthread -L../Lib/UTC_GPS -ltai $(LIBS) -lm

datasrv.o: datasrv.c 
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -I../Lib -c datasrv.c

caudio: caudio.o daqc_access.o
	cc -p $(DEBUGFLAGS) -o caudio caudio.o daqc_access.o -lpthread -L/usr/demo/SOUND/lib -laudio $(LIBS)

caudio.o: caudio.c
	cc -p -I/usr/demo/SOUND/include $(CCFLAGS) $<

daqc_access.o: ../../daqd/daqc_access.c
	$(CC) $(CFLAGS) -c ../../daqd/daqc_access.c
