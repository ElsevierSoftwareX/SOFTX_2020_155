# CPU Shutdown Real Time Linux Makefile

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
#include "Makefile"
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(shell pwd) M=$(PWD) modules
clean:
	rm -f *.o *.ko Module.symvers *.mod.c .*.cmd $(MODULENAME)_test

