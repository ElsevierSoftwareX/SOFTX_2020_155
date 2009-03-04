# RTAI Makeflie

prefix := $(shell rtai-config --prefix)

ifeq ($(prefix),)
$(error Please add <rtai-install>/bin to your PATH variable)
endif

ARCH = $(shell rtai-config --arch)
# If an arch-dependent dir is found, process it too.
ARCHDIR = $(shell test -d $(ARCH) && echo $(ARCH) || echo)

CC = $(shell rtai-config --cc)

LXRT_CFLAGS = $(shell rtai-config --lxrt-cflags)
LINUX_DIR = $(shell rtai-config --linux-dir)


all::
	$(MAKE) -C $(LINUX_DIR) CC=$(CC) SUBDIRS=$$PWD V=$(V) modules
#	@if test -e $(LINUX_DIR)/Module.symvers; then mv -f $(LINUX_DIR)/Module.symvers $(LINUX_DIR)/__Module.symvers; fi; if test -e $(LINUX_DIR)/vmlinux; then mv -f $(LINUX_DIR)/vmlinux $(LINUX_DIR)/__vmlinux; fi; \
	$(MAKE) -C $(LINUX_DIR) CC=$(CC) SUBDIRS=$$PWD V=$(V) modules \
	&& if test -e $(LINUX_DIR)/__Module.symvers; then mv -f $(LINUX_DIR)/__Module.symvers $(LINUX_DIR)/Module.symvers; fi && if test -e $(LINUX_DIR)/__vmlinux; then mv -f $(LINUX_DIR)/__vmlinux $(LINUX_DIR)/vmlinux; fi

clean::
	$(RM) $(LINUX_DIR)/.tmp_versions/*_rt.mod *.o *.ko *.mod.c .*.cmd
