#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#ifdef RTAI_BUILD
#include <rtai_shm.h>
#include <rtai_nam2num.h>
#endif


#ifdef SOLARIS
#include <rfmApi.h>

static char	*rfmFn = "/dev/rfm0";
int rfm_size = 0;
volatile unsigned char *rm_mem_ptr;
static RFMHANDLE	rh;			/* Where RFM gets opened	 */

unsigned int findRfmCard(unsigned int bn)
{
  static initFlag = 1;
  if (initFlag) {
    if ((rh = rfmOpen (rfmFn)) == 0) {
      fprintf(stderr," can't open RFM device `%s'; errno=%d", rfmFn, errno);
      exit (1);
    }
    /* Determine how much memory is associated with this device	 */
    rfm_size = rfmSize(rh);

    printf("%d=0x%x bytes of VMIC Reflected Memory in `%s'\n", rfm_size, rfm_size, rfmFn);
    rm_mem_ptr = rfmRfm(rh)->rfm_ram;
    if (! rm_mem_ptr) {
      fprintf(stderr, "failed to map VMIC Reflected Memory into the address space");
      exit(1);
    }
    initFlag = 0;
  }
  return (unsigned int) rm_mem_ptr;
}

#else 

#ifdef RFM_5579

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/page.h>
#include "rmn_api.h"
#include "rmn_version.h"

#define DEFAULT_DEVICE   "/dev/rmnet0"
#define PROCFILE         "/proc/rmnet"

static  RMNHANDLE Handle;
int rfm_size;

unsigned int findRfmCard(unsigned int bn)
{
    int  result;
    static char *outbuffer;           /* Pointer mapped to RFM area          */
    static initFlag = 1;

    if (initFlag) {
    /* Open the Reflective Memory device */
    result = rmnOpen( DEFAULT_DEVICE, &Handle, 0 );
    if( result != RMN_SUCCESS )
    {
        printf( "ERROR: rmnOpen() failed.\n" );
        exit(1);
    }

#ifdef RFM5579_SWAP_ON_SIZE
    {
      RMN_UINT8 mode = SWAP_ON_SIZE;
      rmnSetSwapMode(Handle, mode);
    }
#endif

    result = rmnMapMemory(Handle, (void **)(&outbuffer), 0, (rfm_size = Handle->Config.MemorySize) / PAGE_SIZE);
    if( result != RMN_SUCCESS )
    {
        printf( "ERROR: rmnMapMemory() failed.\n" );
        exit(1);
    }
    printf("5579 RFM card base address=0x%x\n", outbuffer);
    initFlag = 0;
    }
    return (unsigned int) outbuffer;
}
#elif defined(RFM_5565)

#include <asm/page.h>
#include "rfm2g_api.h"
int rfm_size;

#define DEVICE "/dev/rfm2g0"
static  RFM2GHANDLE Handle;

unsigned int findRfmCard(unsigned int bn)
{
    RFM2G_STATUS  result;              /* Return codes from RFM2Get API calls */
    static RFM2G_UINT32 *outbuffer;           /* Pointer mapped to RFM area          */
    static initFlag = 1;


    if (initFlag) {
    /* Open the Reflective Memory device */
    result = RFM2gOpen( DEVICE, &Handle );
    if( result != RFM2G_SUCCESS )
    {
        printf( "ERROR: RFM2gOpen() failed.\n" );
        printf( "ERROR MSG: %s\n", RFM2gErrorMsg(result));
        exit(1);
    }

    result = RFM2gUserMemory(Handle, (void **)(&outbuffer), 0, (rfm_size = Handle->Config.MemorySize) / PAGE_SIZE);
    if( result != RFM2G_SUCCESS )
    {
        printf( "ERROR: RFM2gUserMemory() failed.\n" );
        printf( "ERROR MSG: %s\n", RFM2gErrorMsg(result));
        exit(1);
    }
    printf("5565 RFM card base address=0x%x\n", outbuffer);
    initFlag = 0;
    }
    return (unsigned int) outbuffer;
}
#else

unsigned char *addr = 0;



/*
  Search shared memory device file names in /rtl_mem_*
  These are like /rtl_mem_das for DAS system,
  /rtl_mem_pde for PDE system.
*/
void *
findSharedMemory(char *sys_name)
{
	char *s;
        int fd;
	char sys[128];
	char fname[128];
	strcpy(sys, sys_name);
	for(s = sys; *s; s++) *s=tolower(*s);

#ifdef RTAI_BUILD
	printf("nam2num(%s) returned %d\n", sys, nam2num(sys));
    	addr = (unsigned char *)rtai_malloc(nam2num(sys), 64*1024*1024);
	if (addr == NULL) {
		printf("rtai_malloc() failed (maybe /dev/rtai_shm is missing)!\n");
		return 0;
    	}

#else
	sprintf(fname, "/rtl_mem_%s", sys);

        if ((fd=open(fname, O_RDWR))<0) {
		fprintf(stderr, "Couldn't open `%s' read/write\n", fname);
                return 0;
        }

        addr = mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
        }
	printf("mmapped address is 0x%lx\n", (long)addr);
#endif
        return addr;
}


void *
findRfmCard(unsigned int bn)
{
    if (!addr) {
        int fd;

        if ((fd=open("/rtl_epics", O_RDWR))<0) {
                perror("open(\"rtl_epics\")");
                _exit(-1);
        }

        addr = mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
        }
	printf("mmapped address is 0x%lx\n", (long)addr);
    }
    return addr;
}
#endif
#endif
