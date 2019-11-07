KBUILD_EXTRA_SYMBOLS=/opt/rtcds/rtscore/trunk/src/drv/ExtraSymbols.symvers

EXTRA_CFLAGS += -O3
DISDIR = /opt/srcdis
EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_DIS_KERNEL_=1 -I$(DISDIR)/src/IRM_GX/drv/src -I$(DISDIR)/src/IRM_GX/drv/src/LINUX -I$(DISDIR)/src/include -I$(DISDIR)/src/include/dis -I$(DISDIR)/src/COMMON/osif/kernel/include -I$(DISDIR)/src/COMMON/osif/kernel/include/LINUX -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000


obj-m += cdsrfmswitch.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
