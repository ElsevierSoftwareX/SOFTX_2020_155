# set your module name here
# provide the following files
#
# MODULENAME.c	the driver
# MODULENAME.h  the include file
# MODULENAME_test.c	the driver test program
# MODULENAME_test.sh	the driver test script
#
MODULENAME 	:= mmap5565
### should not need to change stuff below ######################


KDIR		:= /lib/modules/$(shell uname -r)/build
#KDIR		:= /exports/linux-2.6.12.2/
PWD		:= $(shell pwd)

CC		:= gcc -Wall

ifeq ($(KERNELRELEASE),)
all:	$(MODULENAME)_test
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
else
  obj-m	:= $(MODULENAME).o
endif

$(MODULENAME)_test: $(MODULENAME)_test.c
	$(CC) -g -o $(MODULENAME)_test $(MODULENAME)_test.c


load:
	sudo /sbin/insmod ./$(MODULENAME).ko

unload:
	sudo /sbin/rmmod $(MODULENAME)

test: all
	./$(MODULENAME)_test.sh

clean:
	rm -f *.o *.ko Modules.symvers *.mod.c .*.cmd $(MODULENAME)_test

