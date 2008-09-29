# Real Time Application Interface

LDFLAGS_$(TARGET_RTL) := -g $(LIBRARY_OBJS)

$(TARGET_RTL): $(LIBRARY_OBJS)

setup_shmem.o: setup_shmem.c
	$(CC) $(CFLAGS) -c $< -o $@

EXTRA_CFLAGS += -DRTAI=1
EXTRA_CFLAGS += -I$(SUBDIRS)/../../include
#EXTRA_CFLAGS += -I/opt/gm/include
EXTRA_CFLAGS += -DSERVO32K
EXTRA_CFLAGS += -DOMC_CODE
EXTRA_CFLAGS += -D_ADVANCED_LIGO=1
EXTRA_CFLAGS += -g
#Comment out to enable 1PPS synchronization
EXTRA_CFLAGS += -DNO_SYNC
#Uncomment to disable DAQ and testpoints
EXTRA_CFLAGS += -DNO_DAQ
#Uncomment to enable local frame builder; comment out USE_GM setting too
EXTRA_CFLAGS += -DSHMEM_DAQ
#EXTRA_CFLAGS += -DUSE_GM=1
#Comment out to stop A/D oversampling
EXTRA_CFLAGS += -DOVERSAMPLE
#Comment out to stop interpolating D/A outputs
EXTRA_CFLAGS += -DOVERSAMPLE_DAC
#Comment out to disable initial LIGO compatibility
EXTRA_CFLAGS += -DCOMPAT_INITIAL_LIGO
#Uncomment to run on a specific CPU
#EXTRA_CFLAGS += -DSPECIFIC_CPU=2

EXTRA_CFLAGS += $(shell rtai-config --module-cflags) -I/usr/include -ffast-math

obj-m += setup_shmem.o

#setup_shmem-objs := setup_shmem.o
