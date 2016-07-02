# $Id: Makefile.demo 31471 2012-09-03 11:14:55Z royn $
# Copyright (c) 1997-2013, Dolphin Interconnect Solutions AS. All Rights Reserved.

# Makefile for sisci API programas - dynamic shared library libsisci.so
#
# To run the program, set LD_LIBRARY_PATH to include the directory that contains 
# libsisci.so. e.g. 
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/DIS/lib
# 
#
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/DIS/lib64
CC=cc   #cc or gcc
# CFLAGS = -I/opt/Dolphin/4.4.3/IX/DIS/src/include -I/opt/Dolphin/4.4.3/IX/DIS/src/include/dis -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src -I/opt/DIS/lib/src/ -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/api -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/cmd/test/lib -I/opt/DIS/include -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src

CFLAGS = -I/opt/Dolphin/4.4.3/IX/DIS/src/include -I/opt/Dolphin/4.4.3/IX/DIS/src/include/dis -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src -I/opt/DIS/lib/src/ -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/api -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/cmd/test/lib -I/opt/DIS/include -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src

#CFLAGS = -I/opt/DIS/include -I/opt/DIS/include/dis 

CFLAGS+= -g
FLAGS= -g

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
API_LIB_PATH=/opt/DIS/lib64
else
API_LIB_PATH=/opt/DIS/lib
endif



all : dx_broadcast dx_stream

dx_stream: dx_stream.c 
	$(CC) $(CFLAGS) $(IPATH) dx_stream.c -c -o dx_stream.o
	$(CC) $(CFLAGS) -o dx_stream dx_stream.o -L $(API_LIB_PATH) -lsisci
	sync

rfm.o: ../drv/rfm.c
	$(CC) $(CFLAGS) -c $< -o $@

dx_broadcast: dx_broadcast.c 
	$(CC) $(CFLAGS) dx_broadcast.c -c -o dx_broadcast.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o dx_broadcast  dx_broadcast.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync


clean: 
	rm -f  *.o $(APP)

