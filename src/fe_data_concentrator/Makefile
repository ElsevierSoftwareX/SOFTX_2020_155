# $Id: Makefile.demo 31471 2012-09-03 11:14:55Z royn $
# Copyright (c) 1997-2013, Dolphin Interconnect Solutions AS. All Rights Reserved.

# Makefile for sisci API programas - dynamic shared library libsisci.so
#
# To run the program, set LD_LIBRARY_PATH to include the directory that contains 
# libsisci.so. e.g. 
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/DIS/lib
# 
#
CC=gcc   #cc or gcc
CFLAGS = -m64 -D_REENTRANT   -g -O2 -m64


CFLAGS+= -g -W
FLAGS= -g
CFLAGS+= -I../include

LBITS := $(shell getconf LONG_BIT)


all : fe_dc

rfm.o: ../drv/rfm.c
	$(CC) $(CFLAGS) -c $< -o $@

crc.o: ../drv/crc.c
	$(CC) $(CFLAGS) -c $< -o $@

fe_dc: fe_dc.c 
	$(CC) $(CFLAGS) fe_dc.c -c -o fe_dc.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o fe_dc  fe_dc.o  rfm.o 
	sync

clean: 
	rm -f  *.o fe_dc

