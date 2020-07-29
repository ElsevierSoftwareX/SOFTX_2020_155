///	@file src/drv/rfm.c
/// 	@brief Routines for finding shared memory locations.

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "mbuf/mbuf.h"

/// Pointer to start of shared memory segment for a selected system.
volatile unsigned char *addr = 0;

///  Search shared memory device file names in /rtl_mem_*
///	@param[in] *sys_name	Name of system, required to attach shared memory.
///	@return			Pointer to start of shared memory segment for this system.
volatile void *
findSharedMemory(char *sys_name)
{
	char *s;
        int fd;
	char sys[128];
	char fname[128];
	strcpy(sys, sys_name);
	for(s = sys; *s; s++) *s=tolower(*s);

	sprintf(fname, "/rtl_mem_%s", sys);

	int ss = 64*1024*1024;
  	if (!strcmp(sys_name, "ipc")) ss = 32*1024*1024;
  	if (!strcmp(sys_name, "shmipc")) ss = 16*1024*1024;

       if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
                return 0;
       }
       struct mbuf_request_struct req;
       req.size = ss;
       strcpy(req.name, sys);
       ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
       ioctl (fd, IOCTL_MBUF_INFO, &req);
	
        addr = (volatile unsigned char *)mmap(0, ss, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
        }
	printf(" %s mmapped address is 0x%lx\n", sys,(long)addr);
        return addr;
}

/// Find and return pointer to shared memory. This is old, from when EPICS memory was on
/// VMIC RFM cards.
/// This function should be deleted in favor just having findSharedMemory return pointer,
/// but need to find/change all code that calls this function first.
/// Note that this routine does nothing but return addr, which was found in findSharedMemory().
///	@param[in] bn	RFM board number (0,1) - should now always be zero
///	@return			Pointer to start of shared memory segment for this system.
void *
findRfmCard(unsigned int bn)
{
    if (!addr) {
        int fd;

        if ((fd=open("/rtl_epics", O_RDWR))<0) {
                perror("open(\"rtl_epics\")");
                _exit(-1);
        }

        addr = (volatile unsigned char *)mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
        }
	printf("mmapped address is 0x%lx\n", (long)addr);
    }
    return (void *)addr;
}
volatile void *
findSharedMemorySize(char *sys_name, int size)
{
	char *s;
        int fd;
	char sys[128];
	char fname[128];
	strcpy(sys, sys_name);
	for(s = sys; *s; s++) *s=tolower(*s);

	sprintf(fname, "/rtl_mem_%s", sys);

	int ss = size*1024*1024;
	printf("Making mbuff area %s with size %d\n",sys,ss);

       if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
                return 0;
       }
       struct mbuf_request_struct req;
       req.size = ss;
       strcpy(req.name, sys);
       ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
       ioctl (fd, IOCTL_MBUF_INFO, &req);
	
        addr = (volatile unsigned char *)mmap(0, ss, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
        }
	printf(" %s mmapped address is 0x%lx\n", sys,(long)addr);
        return addr;
}

