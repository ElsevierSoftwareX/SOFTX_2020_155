# RTLinux makefile
include /opt/rtldk-2.2/rtlinuxpro/rtl.mk


TARGET_RTL := setup_shmem.rtl
LIBRARY_OBJS := map.o

ALL +=  setup_shmem.rtl
CFLAGS += -I../../include  -DRTL_BUILD

all: $(ALL)

clean:
	rm -f $(ALL) *.o

include /opt/rtldk-2.2/rtlinuxpro/Rules.make

