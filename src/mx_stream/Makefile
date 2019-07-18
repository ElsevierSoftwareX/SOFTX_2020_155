SHELL = /bin/sh
CC = c++

bindir = ${exec_prefix}/bin
exec_prefix = ${prefix}
prefix = /opt/open-mx
top_srcdir = ..
srcdir = .

.SUFFIXES: .c .o .lo .d .i .obj

#include ../common.mk


# TARGETS does not append $(EXEEXT) because we don't want the GNU make
# rules to apply to Windows.
TARGETS=mx_stream mx_multi_stream mx_mem_test

mx_stream: mx_stream.o rfm.o

mx_multi_stream: mx_multi_stream.o rfm.o

mx_crc_only: mx_crc_only.o rfm.o

mx_mem_test: mx_mem_test.o rfm.o

mx_stream_single: mx_stream_single.o rfm.o

mx_fe: mx_fe.o rfm.o

mx_rcv: mx_rcv.o rfm.o

rfm.o: ../drv/rfm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

CFLAGS = -I${prefix}/include -I.
CFLAGS += -Wall -g -O2
CFLAGS += $(OPTIM_CFLAGS)
CPPFLAGS = -I../common -I$(top_srcdir)/common -I$(top_srcdir)/common/bsd -DNO_RTL  -D_XOPEN_SOURCE -D_BSD_SOURCE
#CPPFLAGS += -DMX_THREAD_SAFE=$(MX_THREAD_SAFE)

LDFLAGS = -L${prefix}/lib -lmyriexpress -lpthread  -g
#M_OS_MACOSX#LDFLAGS += 

# sem_post used by mx_wakeup_test.c requires the rt library on Solaris

# GNU make rules.
#$(TARGETS) : % : %.o $(LIB)
	#$(CC) -o $@ $^ $(LDFLAGS)

# BSD make rules.
#M_MAKE_BSD#.for X in $(TARGETS)
#M_MAKE_BSD#$(X) : $(X).o $(LIB)
#M_MAKE_BSD#	$(CC) -o $@ $^ $(LDFLAGS)
#M_MAKE_BSD#.endfor

.cc.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o mx_stream mx_multi_stream mx_stream_single mx_fe mx_rcv

.PHONY: distclean
distclean: clean
