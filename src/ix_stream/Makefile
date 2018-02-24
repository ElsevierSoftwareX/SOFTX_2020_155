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
CC=gcc   #cc or gcc
# CFLAGS = -I/opt/Dolphin/4.4.3/IX/DIS/src/include -I/opt/Dolphin/4.4.3/IX/DIS/src/include/dis -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src -I/opt/DIS/lib/src/ -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/api -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/cmd/test/lib -I/opt/DIS/include -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src

#CFLAGS = -I/opt/Dolphin/4.4.3/IX/DIS/src/include -I/opt/Dolphin/4.4.3/IX/DIS/src/include/dis -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src -I/opt/DIS/lib/src/ -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/api -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/cmd/test/lib -I/opt/DIS/include -I/opt/Dolphin/4.4.3/IX/DIS/src/SISCI/src

CFLAGS = -DHAVE_CONFIG_H -I/opt/Dolphin/5.2/src/include/dis -I/opt/Dolphin/5.2/src/include -I/opt/Dolphin/5.2/src/SISCI/cmd/test/lib -I/opt/Dolphin/5.2/src/SISCI/src -I/opt/Dolphin/5.2/src/SISCI/api -I/opt/Dolphin/5.2/src/SISCI/cmd/include -I/opt/Dolphin/5.2/src/IRM_GX/drv/src -I//opt/Dolphin/5.2/src/IRM_GX/drv/src/LINUX -DOS_IS_LINUX=196616 -DLINUX -DUNIX  -DLITTLE_ENDIAN -DDIS_LITTLE_ENDIAN -DCPU_WORD_IS_64_BIT -DCPU_ADDR_IS_64_BIT -DCPU_WORD_SIZE=64 -DCPU_ADDR_SIZE=64 -DCPU_ARCH_IS_X86_64 -DADAPTER_IS_IX   -m64 -D_REENTRANT   -g -O2 -m64

#CFLAGS = -I/opt/DIS/include -I/opt/DIS/include/dis 

CFLAGS+= -g
FLAGS= -g
CFLAGS+= -I../include

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
API_LIB_PATH=/opt/DIS/lib64
else
API_LIB_PATH=/opt/DIS/lib
endif



all : ix_multi_stream ix_rcvr ix_rcvr_threads ix_dc_xmit ix_fb_rcv

rfm.o: ../drv/rfm.c
	$(CC) $(CFLAGS) -c $< -o $@

dx_utils.o: ./dx_utils.c
	$(CC) $(CFLAGS) -c $< -o $@

param.o: ../drv/param.c
	$(CC) $(CFLAGS) -c $< -o $@

crc.o: ../drv/crc.c
	$(CC) $(CFLAGS) -c $< -o $@

dx_bcLib: dx_bcLib.c 
	$(CC) $(CFLAGS) dx_bcLib.c -c -o dx_bcLib.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o dx_bcLib  dx_bcLib.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

ix_multi_stream: ix_multi_stream.c 
	$(CC) $(CFLAGS) ix_multi_stream.c -c -o ix_multi_stream.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o ix_multi_stream  ix_multi_stream.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

ix_rcvr: ix_rcvr.c 
	$(CC) $(CFLAGS) ix_rcvr.c -c -o ix_rcvr.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o ix_rcvr  ix_rcvr.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

ix_rcvr_threads: ix_rcvr_threads.c 
	$(CC) $(CFLAGS) ix_rcvr_threads.c -c -o ix_rcvr_threads.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o ix_rcvr_threads  ix_rcvr_threads.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

ix_dc_xmit: ix_dc_xmit.c 
	$(CC) $(CFLAGS) ix_dc_xmit.c -c -o ix_dc_xmit.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o ix_dc_xmit  ix_dc_xmit.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

ix_fb_rcv: ix_fb_rcv.c 
	$(CC) $(CFLAGS) ix_fb_rcv.c -c -o ix_fb_rcv.o 
	$(CC) $(CFLAGS) ../drv/rfm.c -c -o rfm.o 
	$(CC) $(FLAGS) -o ix_fb_rcv  ix_fb_rcv.o  rfm.o -L $(API_LIB_PATH) -lsisci
	sync

daq_master_config: daq_master_config.c
	$(CC) $(CFLAGS) daq_master_config.c -c -o daq_master_config.o
	$(CC) $(FLAGS) -o daq_master_config  daq_master_config.o  rfm.o  param.o crc.o dx_utils.o dx_bcLib.o -L $(API_LIB_PATH) -lsisci

datarate: datarate.c
	$(CC) $(CFLAGS) datarate.c -c -o datarate.o
	$(CC) $(FLAGS) -o datarate  datarate.o  rfm.o  param.o crc.o

test_bcLib: test_bcLib.c
	$(CC) $(CFLAGS) test_bcLib.c -c -o test_bcLib.o
	$(CC) $(FLAGS) -o test_bcLib  test_bcLib.o  dx_bcLib.o -L $(API_LIB_PATH) -lsisci

clean: 
	rm -f  *.o $(APP)

