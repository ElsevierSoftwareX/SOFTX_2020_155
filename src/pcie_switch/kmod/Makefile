MBUFSYM="$(shell ../../epics/util/find_module_symvers mbuf)"
GPSSYM="$(shell ../../epics/util/find_module_symvers gpstime)"
KBUILD_EXTRA_SYMBOLS=$MBUFSYM
KBUILD_EXTRA_SYMBOLS += $GPSSYM

EXTRA_CFLAGS += -O3
DISDIR = /opt/srcdis
KBUILD_EXTRA_SYMBOLS += $(DISDIR)/src/SCI_SOCKET/ksocket/LINUX/Module.symvers
EXTRA_CFLAGS += -DOS_IS_LINUX=1 -D_DIS_KERNEL_=1 -I$(DISDIR)/src/IRM_GX/drv/src -I$(DISDIR)/src/IRM_GX/drv/src/LINUX -I$(DISDIR)/src/include -I$(DISDIR)/src/include/dis -I$(DISDIR)/src/COMMON/osif/kernel/include -I$(DISDIR)/src/COMMON/osif/kernel/include/LINUX -DDOLPHIN_TEST=1  -DDIS_BROADCAST=0x80000000


obj-m += cdsrfmswitch.o

all:
	cp /opt/DIS/lib/modules/$(shell uname -r)/*.symvers Module.symvers
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
