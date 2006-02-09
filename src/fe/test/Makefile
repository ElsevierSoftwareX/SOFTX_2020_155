# RTLinux makefile
include /opt/rtldk-2.2/rtlinuxpro/rtl.mk


TARGET_RTL := gm_simple_example_recv.rtl
LDFLAGS_$(TARGET_RTL) := $(LIBRARY_OBJS)

$(TARGET_RTL): $(LIBRARY_OBJS)

ALL +=  $(TARGET_RTL)
CFLAGS += -I../../include 
CFLAGS += -I/opt/gm/include
all: $(ALL)

clean:
	rm -f $(ALL) *.o *.debug

include /opt/rtldk-2.2/rtlinuxpro/Rules.make


