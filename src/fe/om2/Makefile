# RTLinux makefile
include /opt/rtldk-2.2/rtlinuxpro/rtl.mk


TARGET_RTL := om2fe.rtl
LIBRARY_OBJS := map.o myri.o myriexpress.o  fb.o
LDFLAGS_$(TARGET_RTL) := -g $(LIBRARY_OBJS)

$(TARGET_RTL): $(LIBRARY_OBJS)

om2fe.o: ../controller.c
	$(CC) $(CFLAGS) -c $< -o $@
map.o: ../map.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
myri.o: ../myri.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
myriexpress.o: ../myriexpress.c
	$(CC) $(CFLAGS) -D__KERNEL__ -DMX_KERNEL=1 -c $<
fb.o: ../fb.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<
fm10Gen.o: fm10Gen.c
	$(CC) $(CFLAGS) -D__KERNEL__ -DSERVO32K -c $<
crc.o: crc.c
	$(CC) $(CFLAGS) -D__KERNEL__ -c $<

ALL += user_mmap $(TARGET_RTL)
CFLAGS += -I../../include
CFLAGS += -I/opt/gm/include
CFLAGS += -I/opt/mx/include
CFLAGS += -DSERVO32K
CFLAGS += -DOM2_CODE
CFLAGS += -D_ADVANCED_LIGO=1
CFLAGS += -g
#Comment out to enable 1PPS synchronization
CFLAGS += -DNO_SYNC
#Uncomment to disable DAQ and testpoints
#CFLAGS += -DNO_DAQ
#Uncomment to enable remote IPC over MX
#CFLAGS += -DUSE_MX=1
#Uncomment to enable local frame builder; comment out USE_GM setting too
#CFLAGS += -DSHMEM_DAQ
CFLAGS += -DUSE_GM=1
#Comment out to stop A/D oversampling
CFLAGS += -DOVERSAMPLE
#Comment out to stop interpolating D/A outputs
CFLAGS += -DOVERSAMPLE_DAC
#Comment out to disable initial LIGO compatibility
CFLAGS += -DCOMPAT_INITIAL_LIGO
#Comment out to run on first available CPU
CFLAGS += -DSPECIFIC_CPU=2

ifneq ($(CDIR),)
override CFLAGS += $(patsubst %,-I../../../%,$(CDIR))
endif

all: $(ALL)

clean:
	rm -f $(ALL) *.o

include /opt/rtldk-2.2/rtlinuxpro/Rules.make

