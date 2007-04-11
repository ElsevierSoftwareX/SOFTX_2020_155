# RTLinux makefile
include /opt/rtldk-2.2/rtlinuxpro/rtl.mk


TARGET_RTL := gm_rcvr.rtl
LIBRARY_OBJS := myri.o fb.o manage_network.o
LDFLAGS_$(TARGET_RTL) := -g $(LIBRARY_OBJS)

$(TARGET_RTL): $(LIBRARY_OBJS)

gm_rcvr.o: gm_rcvr.c
	$(CC) $(CFLAGS)  -c $< -o $@
myri.o: ../myri.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
fb.o: ../fb.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
manage_network.o: manage_network.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<

ALL += user_mmap $(TARGET_RTL)
CFLAGS += -I../../include
CFLAGS += -I/opt/gm/include
CFLAGS += -DSERVO2K
CFLAGS += -DSAS_CODE
CFLAGS += -D_ADVANCED_LIGO=1
CFLAGS += -DUSE_GM=1
CFLAGS += -g
#CFLAGS += -DNO_SYNC
#CFLAGS += -DNO_DAQ

all: $(ALL)

clean:
	rm -f $(ALL) *.o

include /opt/rtldk-2.2/rtlinuxpro/Rules.make

