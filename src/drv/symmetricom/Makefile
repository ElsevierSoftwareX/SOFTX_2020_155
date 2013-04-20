# set your module name here
# provide the following files
#
# MODULENAME.c	the driver
# MODULENAME.h  the include file
# MODULENAME_test.c	the driver test program
# MODULENAME_test.sh	the driver test script
#
MODULENAME 	:= symmetricom


### should not need to change stuff below ######################


KDIR		:= /lib/modules/$(shell uname -r)/build
#KDIR		:= /exports/linux-2.6.12.2/
KVER		:= $(shell uname -r)
PWD		:= $(shell pwd)

CC		:= gcc -Wall -g

TARGET		:= symmetricom.ko
INSTDIR		:= /lib/modules/$(KVER)/extra

ifeq ($(KERNELRELEASE),)
#all:	$(MODULENAME)_test
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
else
  obj-m	:= $(MODULENAME).o
endif

stest: stest.c
	$(CC) -g -o stest stest.c

$(MODULENAME)_stat: $(MODULENAME)_stat.c
	$(CC) -g -o $(MODULENAME)_stat $(MODULENAME)_stat.c

install: 
	#remove all old versions of the driver
	find $(INSTALL_MOD_PATH)/lib/modules/$(KVER) -name $(TARGET) -exec rm -f {} \; || true
	find $(INSTALL_MOD_PATH)/lib/modules/$(KVER) -name $(TARGET).gz -exec rm -f {} \; || true
	# Install new driver
	install -D -m 644 $(TARGET) $(INSTALL_MOD_PATH)$(INSTDIR)/$(TARGET)
	/sbin/depmod -a || true
	/sbin/modprobe $(MODULENAME)
	if [ -e /dev/$(MODULENAME) ] ; then \
	    rm -f /dev/$(MODULENAME) ; \
	fi
	mknod /dev/$(MODULENAME) c `grep $(MODULENAME) /proc/devices|awk '{print $$1}'` 0
	chown controls /dev/$(MODULENAME) 

uninstall:
	/sbin/rmmod $(MODULENAME) || true
	if [ -e $(INSTDIR)/$(TARGET) ] ; then \
	  rm -f $(INSTDIR)/$(TARGET) ; \
	  rmdir $(INSTDIR); \
	fi
	/sbin/depmod -a
	/bin/rm -f /dev/$(MODULENAME) || true

load:
	sudo /sbin/insmod ./$(MODULENAME).ko

unload:
	sudo /sbin/rmmod $(MODULENAME)

test: all
	./$(MODULENAME)_test.sh

clean:
	rm -f *.o *.ko Module.symvers *.mod.c .*.cmd stest modules.order

