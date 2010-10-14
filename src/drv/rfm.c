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

#ifdef NO_RTL
#include "mbuf/mbuf.h"
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

volatile unsigned char *addr = 0;



/*
  Search shared memory device file names in /rtl_mem_*
  These are like /rtl_mem_das for DAS system,
  /rtl_mem_pde for PDE system.
*/
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


#ifdef NO_RTL
	int ss = 64*1024*1024;
  	if (!strcmp(sys_name, "ipc")) ss = 4*1024*1024;
#if 0
       #include <sys/mman.h>
       #include <sys/stat.h>        /* For mode constants */
       #include <fcntl.h>           /* For O_* constants */
       //int shm_open(const char *name, int oflag, mode_t mode);
       if ((fd=shm_open(fname, O_RDWR | O_CREAT, 0777)) < 0) {
                fprintf(stderr, "Couldn't shm_open `%s' read/write\n", fname);
                return 0;

       }
       ftruncate(fd, ss);
#endif

       if ((fd = open ("/dev/mbuf", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
                return 0;
       }
       struct mbuf_request_struct req;
       req.size = ss;
       strcpy(req.name, sys);
       ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
       ioctl (fd, IOCTL_MBUF_INFO, &req);
#else
const int ss = 64*1024*1024-5000;
        if ((fd=open(fname, O_RDWR))<0) {
		fprintf(stderr, "Couldn't open `%s' read/write\n", fname);
                return 0;
        }
#endif

	
        addr = (volatile unsigned char *)mmap(0, ss, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
        }
	printf("mmapped address is 0x%lx\n", (long)addr);
	memset((void *)addr, 0, ss);
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
#endif
#endif
