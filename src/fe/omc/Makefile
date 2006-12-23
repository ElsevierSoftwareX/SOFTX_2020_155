# RTLinux makefile
include /opt/rtldk-2.2/rtlinuxpro/rtl.mk


TARGET_RTL := omcfe.rtl
LIBRARY_OBJS := map.o
LDFLAGS_$(TARGET_RTL) := -g $(LIBRARY_OBJS)

$(TARGET_RTL): $(LIBRARY_OBJS)

omcfe.o: ../controller.c
	$(CC) $(CFLAGS) -c $< -o $@
map.o: ../map.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
fm10Gen.o: fm10Gen.c
	$(CC) $(CFLAGS) -D__KERNEL__ -DSERVO32K -c $<
crc.o: crc.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<

ALL += user_mmap $(TARGET_RTL)
CFLAGS += -I../../include
CFLAGS += -I/opt/gm/include
CFLAGS += -DSERVO32K
CFLAGS += -DOMC_CODE
CFLAGS += -g
#CFLAGS += -DNO_SYNC
#CFLAGS += -DNO_DAQ

all: $(ALL)

clean:
	rm -f $(ALL) *.o

include /opt/rtldk-2.2/rtlinuxpro/Rules.make

