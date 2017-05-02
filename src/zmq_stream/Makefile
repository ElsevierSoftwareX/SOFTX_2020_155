SHELL = /bin/sh
CC = gcc

bindir = ${exec_prefix}/bin
exec_prefix = ${prefix}
top_srcdir = ..
srcdir = .

.SUFFIXES: .c .o .lo .d .i .obj

#include ../common.mk

TARGETS=zmq_multi_stream zmq_multi_rcvr

zmq_multi_stream: zmq_multi_stream.o rfm.o
zmq_multi_rcvr: zmq_multi_rcvr.o
rfm.o: ../drv/rfm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

CFLAGS = -I${prefix}/include -I.
CFLAGS += -Wall -g -O2 -std=gnu99
CFLAGS += $(OPTIM_CFLAGS)

LDFLAGS = -L/usr/local/lib -lzmq
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r *.o zmq_multi_stream zmq_multi_rcvr

.PHONY: distclean
distclean: clean

