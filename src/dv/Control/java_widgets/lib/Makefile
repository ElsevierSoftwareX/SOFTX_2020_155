
# Explicit setting of SHELL - beware since this gets inherited
# by commands invoked by Make

SHELL=/bin/sh
TARGET=generic
ABIDIR=
SYSDIR=
ABICFLAGS=
ABICCFLAGS=
ABILDFLAGS=

# Default rule
all: all-${TARGET}


##: system Solaris 32bit Ansi C/C++
##: default cpp
#CC=cc
#MOTIFHOME=/usr/dt
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/openwin/include -I/usr/openwin/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO
#ABI2CFLAGS=
#ABI2CCFLAGS=-compat=5
#ABI2LDFLAGS=
#ABI2ABIDIR=/ansi32
#ABI2SYSDIR=
#ABICFLAGS=${ABI2CFLAGS}
#ABICCFLAGS=${ABI2CCFLAGS}
#ABILDFLAGS=${ABI2LDFLAGS}
#SYSDIR=
#ABIDIR=/ansi32
##: end

##: system Solaris 64bit Ansi C/C++ (sparc)
#CC=cc
#MOTIFHOME=/usr/dt
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/openwin/include -I/usr/openwin/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO
#ABI2-64CFLAGS=-xarch=v9
#ABI2-64CCFLAGS=-xarch=v9 -compat=5
#ABI2-64LDFLAGS=
#ABI2-64ABIDIR=/sparcv9
#ABI2-64SYSDIR=/sparcv9
#ABICFLAGS=${ABI2-64CFLAGS}
#ABICCFLAGS=${ABI2-64CCFLAGS}
#ABILDFLAGS=${ABI2-64LDFLAGS}
#SYSDIR=/sparcv9
#ABIDIR=/sparcv9
##: end

##: system Solaris 32bit Workshop4 Compatible C++
##: default c
#CC=cc
#MOTIFHOME=/usr/dt
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/openwin/include -I/usr/openwin/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO
#ABI1CFLAGS=
#ABI1CCFLAGS=-compat=4
#ABI1LDFLAGS=
#ABI1ABIDIR=/compat-pre5
#ABI1SYSDIR=
#ABICFLAGS=${ABI1CFLAGS}
#ABICCFLAGS=${ABI1CCFLAGS}
#ABILDFLAGS=${ABI1LDFLAGS}
#SYSDIR=
#ABIDIR=/compat-pre5
##: end

#: system Solaris 32/64 Multi-target C/C++ (sparc)
CC=cc
MOTIFHOME=/opt/openmotif/usr
MOTIFINCLUDES=-I${MOTIFHOME}/include
XINCLUDES=${MOTIFINCLUDES} -I/usr/openwin/include -I/usr/openwin/include/X11
RANLIB=@:
CFLAGS= -O -D_NO_PROTO
# Extra ABI targets for solaris2_abi_multiabi
ABI1CFLAGS=
ABI1CCFLAGS=-compat=4
ABI1LDFLAGS=
ABI1ABIDIR=/compat-pre5
ABI1SYSDIR=

all-compat-pre5::
	make -f Makefile TARGET=generic ABICFLAGS="${ABI1CFLAGS}" ABICCFLAGS="${ABI1CCFLAGS}" ABILDFLAGS="${ABI1LDFLAGS}" ABIDIR="/${ABI1ABIDIR}" SYSDIR="${ABI1SYSDIR}" all

server-compat-pre5::
	make -f Makefile TARGET=generic ABICFLAGS="${ABI1CFLAGS}" ABICCFLAGS="${ABI1CCFLAGS}" ABILDFLAGS="${ABI1LDFLAGS}" ABIDIR="/${ABI1ABIDIR}" SYSDIR="${ABI1SYSDIR}" server

ABI2CFLAGS=
ABI2CCFLAGS=-compat=5
ABI2LDFLAGS=
ABI2ABIDIR=/ansi32
ABI2SYSDIR=

all-ansi32::
	make -f Makefile TARGET=generic ABICFLAGS="${ABI2CFLAGS}" ABICCFLAGS="${ABI2CCFLAGS}" ABILDFLAGS="${ABI2LDFLAGS}" ABIDIR="/${ABI2ABIDIR}" SYSDIR="${ABI2SYSDIR}" all

server-ansi32::
	make -f Makefile TARGET=generic ABICFLAGS="${ABI2CFLAGS}" ABICCFLAGS="${ABI2CCFLAGS}" ABILDFLAGS="${ABI2LDFLAGS}" ABIDIR="/${ABI2ABIDIR}" SYSDIR="${ABI2SYSDIR}" server

ABI2-64CFLAGS=-xarch=v9
ABI2-64CCFLAGS=-xarch=v9 -compat=5
ABI2-64LDFLAGS=
ABI2-64ABIDIR=/sparcv9
ABI2-64SYSDIR=/sparcv9

all-sparcv9::
	make -f Makefile TARGET=generic ABICFLAGS="${ABI2-64CFLAGS}" ABICCFLAGS="${ABI2-64CCFLAGS}" ABILDFLAGS="${ABI2-64LDFLAGS}" ABIDIR="/${ABI2-64ABIDIR}" SYSDIR="${ABI2-64SYSDIR}" all

server-sparcv9::
	make -f Makefile TARGET=generic ABICFLAGS="${ABI2-64CFLAGS}" ABICCFLAGS="${ABI2-64CCFLAGS}" ABILDFLAGS="${ABI2-64LDFLAGS}" ABIDIR="/${ABI2-64ABIDIR}" SYSDIR="${ABI2-64SYSDIR}" server

compatCFLAGS=
compatCCFLAGS=
compatLDFLAGS=
compatABIDIR=/pre5
compatSYSDIR=

all-pre5::
	make -f Makefile TARGET=generic ABICFLAGS="${compatCFLAGS}" ABICCFLAGS="${compatCCFLAGS}" ABILDFLAGS="${compatLDFLAGS}" ABIDIR="/${compatABIDIR}" SYSDIR="${compatSYSDIR}" all

server-pre5::
	make -f Makefile TARGET=generic ABICFLAGS="${compatCFLAGS}" ABICCFLAGS="${compatCCFLAGS}" ABILDFLAGS="${compatLDFLAGS}" ABIDIR="/${compatABIDIR}" SYSDIR="${compatSYSDIR}" server

#: end

##: system HP-UX 10.x
## -Wp,-H900000 recommended if too many cpp defines.
## For K&R code generation use:
## CPPFLAGS=-Wp,-H900000
## For ANSI C code generation use:
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=-I/usr/include/Motif1.2
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11R6 -I/usr/include/X11R6/X11
#RANLIB=@:
#CFLAGS=-D_NO_PROTO -Wp,-H900000
##: end

##: system HP-UX 9.x
## -Wp,-H900000 recommended if too many cpp defines.
## For K&R code generation use:
## CPPFLAGS=-Wp,-H900000
## For ANSI C code generation use:
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=-I/usr/include/Motif1.2
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11R5
#RANLIB=@:
#CFLAGS=-D_NO_PROTO -Wp,-H900000
##: end

##: system IBM Aix
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=ranlib
#CFLAGS=-O -D_NO_PROTO
##: end

##: system Compaq "Digital Unix"
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=ranlib
#CFLAGS=
##: end

##: system Concurrent (MAXION System 5 Release 4)
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO
##: end

##: system NCR (System 5 Release 4)
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -Hnocopyr
##: end

##: system NEC (System 5 Release 4)
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO
##: end

##: system SCO UnixWare 2 (was Novell)
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/XINCLUDES/include -I/usr/XINCLUDES/include/X11
#RANLIB=@:
#CFLAGS=-D_NO_PROTO
##: end

##: system HAL Solaris 2.x
## For ANSI C code generation use:
## CPPFLAGS=-KV7 -Xa
#CC=hcc
#MOTIFHOME=/usr/dt
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/openwin/include -I/usr/openwin/include/X11
#RANLIB=ranlib
#CFLAGS=-O -D_NO_PROTO
##: end

##: system Silicon Graphics (new 32 bit)
#CC=cc
#MOTIFHOME=/usr
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -cckr
#n32CFLAGS=-n32
#n32CCFLAGS=-n32
#n32LDFLAGS=-n32
#n32ABIDIR=/n32
#n32SYSDIR=32
#ABICFLAGS=${n32CFLAGS}
#ABICCFLAGS=${n32CCFLAGS}
#ABILDFLAGS=${n32LDFLAGS}
#SYSDIR=32
#ABIDIR=/n32
##: end

##: system Silicon Graphics (new 64 bit)
#CC=cc
#MOTIFHOME=/usr
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -cckr
#n64CFLAGS=-64
#n64CCFLAGS=-64
#n64LDFLAGS=-64
#n64ABIDIR=/n64
#n64SYSDIR=64
#ABICFLAGS=${n64CFLAGS}
#ABICCFLAGS=${n64CCFLAGS}
#ABILDFLAGS=${n64LDFLAGS}
#SYSDIR=64
#ABIDIR=/n64
##: end

##: system Silicon Graphics (old 32 bit)
#CC=cc
#MOTIFHOME=/usr
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -cckr
#o32CFLAGS=-o32
#o32CCFLAGS=-o32
#o32LDFLAGS=-o32
#o32ABIDIR=/o32
#o32SYSDIR=
#ABICFLAGS=${o32CFLAGS}
#ABICCFLAGS=${o32CCFLAGS}
#ABILDFLAGS=${o32LDFLAGS}
#SYSDIR=
#ABIDIR=/o32
##: end

##: system Silicon Graphics (Irix 6.5 Defaults)
#CC=cc
#MOTIFHOME=/usr
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -cckr
#dfltCFLAGS=
#dfltCCFLAGS=
#dfltLDFLAGS=
#dfltABIDIR=/
#dfltSYSDIR=32
#ABICFLAGS=${dfltCFLAGS}
#ABICCFLAGS=${dfltCCFLAGS}
#ABILDFLAGS=${dfltLDFLAGS}
#SYSDIR=32
#ABIDIR=/
##: end

##: system Silicon Graphics (Pre 6.5 Defaults)
#CC=cc
#MOTIFHOME=/usr
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -cckr
#tradCFLAGS=
#tradCCFLAGS=
#tradLDFLAGS=
#tradABIDIR=/trad
#tradSYSDIR=
#ABICFLAGS=${tradCFLAGS}
#ABICCFLAGS=${tradCCFLAGS}
#ABILDFLAGS=${tradLDFLAGS}
#SYSDIR=
#ABIDIR=/trad
##: end

##: system Silicon Graphics 32/64 Multi-target C/C++
#CC=cc
#MOTIFHOME=/usr
#MOTIFINCLUDES=-I${MOTIFHOME}/include
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-O -D_NO_PROTO -cckr
## Extra ABI targets for sg_irix_abi_multiabi
#o32CFLAGS=-o32
#o32CCFLAGS=-o32
#o32LDFLAGS=-o32
#o32ABIDIR=/o32
#o32SYSDIR=
#
#all-o32::
#	make -f Makefile TARGET=generic ABICFLAGS="${o32CFLAGS}" ABICCFLAGS="${o32CCFLAGS}" ABILDFLAGS="${o32LDFLAGS}" ABIDIR="/${o32ABIDIR}" SYSDIR="${o32SYSDIR}" all
#
#server-o32::
#	make -f Makefile TARGET=generic ABICFLAGS="${o32CFLAGS}" ABICCFLAGS="${o32CCFLAGS}" ABILDFLAGS="${o32LDFLAGS}" ABIDIR="/${o32ABIDIR}" SYSDIR="${o32SYSDIR}" server
#
#n32CFLAGS=-n32
#n32CCFLAGS=-n32
#n32LDFLAGS=-n32
#n32ABIDIR=/n32
#n32SYSDIR=32
#
#all-n32::
#	make -f Makefile TARGET=generic ABICFLAGS="${n32CFLAGS}" ABICCFLAGS="${n32CCFLAGS}" ABILDFLAGS="${n32LDFLAGS}" ABIDIR="/${n32ABIDIR}" SYSDIR="${n32SYSDIR}" all
#
#server-n32::
#	make -f Makefile TARGET=generic ABICFLAGS="${n32CFLAGS}" ABICCFLAGS="${n32CCFLAGS}" ABILDFLAGS="${n32LDFLAGS}" ABIDIR="/${n32ABIDIR}" SYSDIR="${n32SYSDIR}" server
#
#n64CFLAGS=-64
#n64CCFLAGS=-64
#n64LDFLAGS=-64
#n64ABIDIR=/n64
#n64SYSDIR=64
#
#all-n64::
#	make -f Makefile TARGET=generic ABICFLAGS="${n64CFLAGS}" ABICCFLAGS="${n64CCFLAGS}" ABILDFLAGS="${n64LDFLAGS}" ABIDIR="/${n64ABIDIR}" SYSDIR="${n64SYSDIR}" all
#
#server-n64::
#	make -f Makefile TARGET=generic ABICFLAGS="${n64CFLAGS}" ABICCFLAGS="${n64CCFLAGS}" ABILDFLAGS="${n64LDFLAGS}" ABIDIR="/${n64ABIDIR}" SYSDIR="${n64SYSDIR}" server
#
##: end

##: system SCO OpenServer 5
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=ranlib
#CFLAGS=-D_NO_PROTO -b elf -dy -Wl,-Bexport
##: end

##: system QNX
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} 	-I/usr/include/X11
#RANLIB=ranlib
#CFLAGS=-DQNX
##: end

##: system Lynx Real-Time OS (PPC)
#CC=gcc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=ranlib
#CFLAGS=
##: end

##: system Linux (x86)
#CC=gcc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/X11/include/X11
#RANLIB=@:
#CFLAGS=-m486 -b i486-linux -Dlinux -D__i386__ -D__ELF__ -D_POSIX_SOURCE
##: end

##: system Concurrent Night Hawk (PowerUX3.1)
#CC=cc
#MOTIFHOME=
#MOTIFINCLUDES=
#XINCLUDES=${MOTIFINCLUDES} -I/usr/include/X11
#RANLIB=@:
#CFLAGS=-D_NO_PROTO -Wl,-Bexport -dy
##: end


LCLHDRS=../h
IFLAGS=-I${LCLHDRS} ${XINCLUDES}

all-generic :	Border.c Card.c Flow.c Grid.c GridBag.c Layout.c libjavawidgets.a

.c.o:
	rm -f $@
	$(CC) -c $(CFLAGS) ${ABICFLAGS} $(IFLAGS) $*.c

libjavawidgets.a:	Border.o Card.o Flow.o Grid.o GridBag.o Layout.o
	rm -f $@
	ar cru $@ *.o
	-$(RANLIB) $@

clean::
	-rm -f *.o *.a
