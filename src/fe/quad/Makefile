# RTLinux makefile
include /opt/rtldk-2.2/rtlinuxpro/rtl.mk


TARGET_RTL := quadfe.rtl
LIBRARY_OBJS := map.o
LDFLAGS_$(TARGET_RTL) := $(LIBRARY_OBJS)

$(TARGET_RTL): $(LIBRARY_OBJS)

map.o: ../map.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
fm10Gen.o: fm10Gen.c
	$(CC) $(CFLAGS) -D__KERNEL__ -DSERVO16K -c $<
crc.o: crc.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<

ALL +=  user_mmap $(TARGET_RTL)
CFLAGS += -I../../include 
CFLAGS += -I/opt/gm/include
CFLAGS += -DSERVO16K 
CFLAGS += -DLSC_LVEA 
CFLAGS += -DDSC 
CFLAGS += -DQUAD 
 
#CFLAGS += -DRFM 

all: $(ALL)

clean:
	rm -f $(ALL)

include /opt/rtldk-2.2/rtlinuxpro/Rules.make


