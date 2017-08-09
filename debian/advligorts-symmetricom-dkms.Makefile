#!/usr/bin/make -f

all:
	$(MAKE) -C drv/symmetricom
	cp drv/symmetricom/symmetricom.ko .

clean:
	$(MAKE) -C drv/symmetricom clean
	rm symmetricom.ko
