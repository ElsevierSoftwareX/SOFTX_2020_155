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
export DOLPHIN_PATH=/opt/srcdis
# export DOLPHIN_PATH=/opt/Dolphin/DIS

CC=gcc   #cc or gcc


CFLAGS = -DHAVE_CONFIG_H -I$(DOLPHIN_PATH)/src/include/dis -I$(DOLPHIN_PATH)/src/include -I$(DOLPHIN_PATH)/src/SISCI/cmd/test/lib -I$(DOLPHIN_PATH)/src/SISCI/src -I$(DOLPHIN_PATH)/src/SISCI/api -I$(DOLPHIN_PATH)/src/SISCI/cmd/include -I$(DOLPHIN_PATH)/src/IRM_GX/drv/src -I$(DOLPHIN_PATH)/src/IRM_GX/drv/src/LINUX -DOS_IS_LINUX=196616 -DLINUX -DUNIX  -DLITTLE_ENDIAN -DDIS_LITTLE_ENDIAN -DCPU_WORD_IS_64_BIT -DCPU_ADDR_IS_64_BIT -DCPU_WORD_SIZE=64 -DCPU_ADDR_SIZE=64 -DCPU_ARCH_IS_X86_64 -DADAPTER_IS_IX   -m64 -D_REENTRANT   -g -O2 -m64

#CFLAGS = -I/opt/DIS/include -I/opt/DIS/include/dis 

CFLAGS+= -g -Wall
FLAGS= -g
CFLAGS+= -I../include

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
API_LIB_PATH=/opt/DIS/lib64
else
API_LIB_PATH=/opt/DIS/lib
endif



all : dix_xmit dix_recv

rfm.o: ../drv/rfm.c
	$(CC) $(CFLAGS) -c $< -o $@

# param.o: ../drv/param.c
# 	$(CC) $(CFLAGS) -c $< -o $@

crc.o: ../drv/crc.c
	$(CC) $(CFLAGS) -c $< -o $@

dix_xmit: dix_xmit.c
	$(CC) $(CFLAGS) -I../zmq_stream -I/opt/open-mx/include dix_xmit.c -c -o dix_xmit.o
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(CFLAGS) -I../zmq_stream ../zmq_stream/simple_pv.c -c -o simple_pv.o
	$(CC) $(FLAGS) -o dix_xmit  dix_xmit.o  rfm.o simple_pv.o -L $(API_LIB_PATH) -L/opt/open-mx/lib -lmyriexpress -lpthread
	sync

dix_recv: dix_recv.c
	$(CC) $(CFLAGS) dix_recv.c -c -o dix_recv.o
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o dix_recv  dix_recv.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

CPPFLAGS = -I../common -I$(top_srcdir)/common -I$(top_srcdir)/common/bsd -DNO_RTL  -D_XOPEN_SOURCE -D_BSD_SOURCE
LDFLAGS = -L/opt/open-mx/lib -lmyriexpress -lpthread  -g

clean: 
	rm -f  *.o $(APP)

