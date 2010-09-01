/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: rmapi							*/
/*                                                         		*/
/* Module Description: implements functions for accessing the RM	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef _RM_RANGE_CHECK 
#define _RM_RANGE_CHECK
#endif

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <sysLib.h>
#include <cacheLib.h>
#include <vme.h>
#include <vxLib.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdstask.h"
#include "dtt/hardware.h"
#if defined(OS_VXWORKS) && (RMEM_LAYOUT == 1)
#include "dtt/pci.h"
#endif
#include "dtt/rmapi.h"

#ifdef OS_SOLARIS
#include <rfm2g_api.h>
#include <rfm_io.h>
#include <rfmApi.h>
#include <rfmErrno.h>
#include <unistd.h>
#endif

#ifndef RTLINUX
#include "../drv/mbuf/mbuf.h"
#endif


/* VxWorks BSP functions not prototyped */
#if defined(OS_VXWORKS) && defined (PROCESSOR_BAJA)
   extern STATUS sysVicBlkCopy (char *,char *, int, BOOL, BOOL);
#endif

#define RMAPI_MAXNODE		4

#define VMIC5588_RAM_OFFSET	0x40
#define DMA_THRESHOLD		256*1000000


   static char*		rm[] = {
   (char*) (0),
   (char*) (0),
   (char*) (0),
   (char*) (0)};

   static char*		rmboard[] = {
   (char*) (0),
   (char*) (0),
   (char*) (0),
   (char*) (0)};

   static int		rmsize[] = {0, 0, 0, 0};
   static int 		rmmaster[] = {0, 0, 0, 0};

   struct rmISR_ctrl {
      short		id;
      int		inum;
      int 		level;
      int		vec;
      rmISR		isr;
      void*		arg;
      int		status;
   };
   typedef struct rmISR_ctrl rmISR_ctrl;
   static rmISR_ctrl	rmIntCtrl[RMAPI_MAXNODE][4];
   static rmISR_ctrl*	ctrlISR = NULL;
   static sem_t		semISR;
   static taskID_t	tidISR;
   static int		badData = 0;
   static int		badFIFO = 0;

   char* rmBaseAddress (short ID)
   {
      switch (ID) {
         case 0:
         case 1:
         case 2:
         case 3:
            {
               return rm[ID];
            }
         default:
            {
               return NULL;
            }
      }
   }


   char* rmBoardAddress (short ID)
   {
      switch (ID) {
         case 0:
         case 1:
         case 2:
         case 3:
            {
               return rmboard[ID];
            }
         default:
            {
               return NULL;
            }
      }
   }


   int rmBoardSize (short ID)
   {
      switch (ID) {
         case 0:
         case 1:
         case 2:
         case 3:
            {
               return rmsize[ID];
            }
         default:
            {
               return 0;
            }
      }
   }

#if RMEM_LAYOUT == 0
   static int rmMaster (int ID) 
   {
      switch (ID) {
         case 0:
         case 1:
         case 2:
         case 3:
            {
               return (rmmaster[ID] != 0);
            }
         default:
            {
               return 0;
            }
      }
   
   }
#endif

/*
  Handels interrupts 1,2,3
  records sender and interrupt number
  gives interrupt semaphore
*/
   int rmClearInt (short ID, int* sid, int* inum)
   {
      volatile char* volatile	addr;	/* base address */
   
      addr = rmBoardAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
      /* toggle the damn led */
   #ifdef DEBUG
      ((RMREGS*) addr)->csr ^= 0x80;
   #endif
   
      *sid = ((RMREGS*) addr)->sid1;
      *inum = 0x0007 & ((RMREGS*) addr)->irs;
   
      return 0;
   }

/*
  Turns the FAIL led on or off
  setting the FAIL led will have no effect on the
  operation of the 5588 board.  The 5588 does not
  set or clear the FAIL led under any circumstances.
*/
   int rmLED (short ID, int led_on)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = rmBoardAddress (ID);
      if (addr == NULL) {
         return -1;
      }
      /* set LED */
      if (led_on) {
         ((RMREGS*) addr)->csr |= 0x80;
      }
      else {
         ((RMREGS*) addr)->csr &= ~(0x80);
      }
   
      return 0;
   }

/*
  Bypasses the current Reflected memory Node
  must have external optical switch installed
*/
   int rmNodeBypass (short ID, int bypass)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = rmBoardAddress (ID);
      if (addr == NULL) {
         return -1;
      }
      /* set bypass */
      if (bypass) {
         ((RMREGS*) addr)->irs |= (0x08);
      }
      else {
         ((RMREGS*) addr)->irs &= ~(0x08);
      }
   
      return 0;
   }

#ifndef OS_VXWORKS

#ifdef OS_SOLARIS
#if RMEM_LAYOUT != 1
# error unsupported
#endif
/*
   There are up to four RFM boards installed in a frame builder computer.
   The devices are:

  Board ID | Device File Name    | IFO RFM loop 
  ------------------------------------------------------
	0   /dev/daqd-rfm	  the 4k 5565 card
	1   /dev/daqd-rfm-aux	  the 4k 5579 card
	2   /dev/daqd-rfm1	  the 2k 5565 card
	3   /dev/daqd-rfm-aux1	  the 2k 5579 card


   TPMan node 0 should map into boards 0 and 1
   node 1 into boards 2 and 3
*/

   /* Reflective memory board handles */
   static unsigned int rfmHandle[4] = {0, 0, 0, 0};

   /* 5565 boards' handles */
   RFM2GHANDLE   rh5565[2];

   int rmInit (short ID)
   {
	static char junk[64 * 1024 *1024];
        volatile void *mem;
	char dev_fname[128];

	/* printf("rmInit(%d)\n", ID); */
	if (ID == 0) strcpy(dev_fname, "/dev/daqd-rfm");
 	else if (ID == 1) strcpy(dev_fname, "/dev/daqd-rfm-aux");
	else if (ID == 2) strcpy(dev_fname, "/dev/daqd-rfm1");
 	else if (ID == 3) strcpy(dev_fname, "/dev/daqd-rfm-aux1");
	else  {
           printf ("rmInit: Illegal board ID %d\n", ID);
           exit(-1);
	}

	if (ID & 1) {
	  /* 5579 board initialization */
	  RFMHANDLE rh = 0;

	  if ((rh = rfmOpen (dev_fname)) == 0) {
                printf("Could not open 5579 reflective memory in %s\n", dev_fname);
                exit(-1);
	  }
	  rfmSetSwapping(rh, rfmSwapSize);
	  mem = rfmRfm(rh)->rfm_ram;
	  /*mem = junk;*/
          printf ("VMIC RFM 5579 (%d) found, mapped at 0x%x\n", ID, (int) mem);
          rfmHandle[ID] = rh->rh_fd;
        } else {
	  /* 5565 board initialization */
	  RFM2GHANDLE   rh = 0;

	  if (RFM2gOpen (dev_fname, &rh) != RFM2G_SUCCESS) {
                printf("Could not open 5565 reflective memory in %s\n", dev_fname);
		if (ID == 2) return -1;
                exit(-1);
	  }
	  /*(void)RFM2gUserMemory(rh, (void **)&mem, 0, 1);*/
	  mem = junk;
  	  /* Set DMA bnd I/O byteswapping */
  	  (void)RFM2gSetDMAByteSwap(rh, 1);
  	  (void)RFM2gSetPIOByteSwap(rh, 1);

          printf ("VMIC RFM 5565 (%d) found, mapped at 0x%x\n", ID, (int) mem);
          rfmHandle[ID] = rh->rh_fd;
	  rh5565[ID >> 1] = rh;
       }
       rmboard[ID] = rm[ID] = (char*)mem;
       rmsize[ID] =  64 * 1024 *1024;
       rmmaster[ID] = 0;
       return 0;
   }
#else

int mbuf_opened = 0;
int mbuf_fd = 0;

   int rmInit (short ID)
   {
      // 0 -- our shared memory
      // 2 -- IPC shared memory
      if (ID == 0 || ID == 2) {
	 int fd;
	 void *addr;
         char fname[128];
	 extern char system_name[PARAM_ENTRY_LEN];

         /*rmboard[ID] = rm[ID] = malloc (rmsize[ID]);*/
         rmmaster[ID] = 0;

#ifndef RTLINUX
	 strcpy(fname, "/dev/mbuf");
	 if (!mbuf_opened) {
	   if ((fd = open (fname, O_RDWR | O_SYNC)) < 0) {
	       fprintf(stderr, "Couldn't open /dev/mbuf read/write\n");
	       _exit(-1);
	   }
	   mbuf_opened = 1;
 	   mbuf_fd = fd;
	 } else fd = mbuf_fd;
	 struct mbuf_request_struct req;
	 if (ID == 0) {
		strcpy(req.name, system_name);
		rmsize[ID] =  64 * 1024 * 1024;
	 } else {
		strcpy(req.name, "ipc");
		rmsize[ID] =  4 * 1024 * 1024;
	 }
	 req.size = rmsize[ID];
         ioctl (fd, IOCTL_MBUF_ALLOCATE, &req);
         //ioctl (fd, IOCTL_MBUF_INFO, &req);

#else
	 rmsize[ID] =  64 * 1024 * 1024 - 5000;
	 if (ID == 0) sprintf(fname, "/rtl_mem_%s", system_name);
	 else strcpy(fname, "/rtl_mem_ipc");

         if ((fd = open(fname, O_RDWR)) < 0) {
           fprintf(stderr, "Couldn't open `%s' read/write\n", fname);
           _exit(-1);
#if 0
	   /* rtl_epics is shared memory partition used for 
	     communication between epics, front end, awg and tpman */
           if ((fd=open("/rtl_epics", O_RDWR))<0) {
                perror("open(\"rtl_epics\")");
                _exit(-1);
           }
#endif
	 }
#endif

         addr = mmap(0, rmsize[ID], PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
         if (addr == MAP_FAILED) {
                printf("return was %d\n",errno);
                perror("mmap");
                _exit(-1);
         }
         printf("%s mmapped address is 0x%lx\n", fname, (long)addr);
         rmboard[ID] = rm[ID] = addr;
      }
      else if (ID == 1) {
         rm[ID] = NULL;
         rmboard[ID] = NULL;
         rmsize[ID] = 0;
         rmmaster[ID] = 0;
      } 
      else {
         printf ("rmInit: Illegal board ID\n");
         return -1;
      } 
   #ifdef DEBUG
      printf("RM%i: Node ID: %i -- Shared memory (%s)\n",
            ID, rm[ID] ? "/gds/" : "");
   #endif
      return 0;
   }
#endif


#elif RMEM_LAYOUT == 1

#define TURN_OFF_5565_FAIL	0x80000000

#define MEM_LOCATOR     0x7
#define DMA_LOCATOR     0x4
#define CSR_LOCATOR     0x6

   struct VMIC5565_CSR {
	unsigned char BRV;	/* 0x0 */
	unsigned char BID;	/* 0x1 */
	unsigned char rsv0;     /* 0x2 */
	unsigned char rsv1;     /* 0x3 */
	unsigned char NID;      /* 0x4 */
	unsigned char rsv2;     /* 0x5 */
	unsigned char rsv3;     /* 0x6 */
	unsigned char rsv4;     /* 0x7 */
	unsigned int LCSR1;	/* 0x8 */
	unsigned int rsv5;	/* 0xC */
	unsigned int LISR;	/* 0x10 */
	unsigned int LIER;	/* 0x14 */
	unsigned int NTD;	/* 0x18 */
	unsigned char NTN;	/* 0x1C */
	unsigned char NIC;	/* 0x1D */
   };

   volatile void *findRfm5565 (unsigned short instance, unsigned short locRegister)
   {
      int busId;
      int status;
      unsigned int pmcAdd;

      status = sysFindPciDevice (0x5565, 0x114A, instance, 0, &busId);
      if (status == -1) return (volatile void *) -1;
      else pmcAdd = sysReadPciConfig (busId, status, 0, locRegister);

      if (locRegister == DMA_LOCATOR) pmcAdd += 0x80;

      return (volatile void*) pmcAdd;
   }

#define TURN_OFF_FAIL_LITE      0x80

struct VMIC5579_MEM_REGISTER {
        unsigned char rsv0;     /* 0x0 */
        unsigned char BID;      /* 0x1 */
        unsigned char rsv2;     /* 0x2 */
        unsigned char rsv3;     /* 0x3 */
        unsigned char NID;      /* 0x4 */
        unsigned char rsv5;     /* 0x5 */
        unsigned char rsv6;     /* 0x6 */
        unsigned char rsv7;     /* 0x7 */
        unsigned char IRS;      /* 0x8 */
        unsigned char CSR1;     /* 0x9 */
        unsigned char rsvA;     /* 0xa */
        unsigned char rsvB;     /* 0xb */
        unsigned char CSR2;     /* 0xc */
        unsigned char CSR3;     /* 0xd */
        unsigned char rsvE;     /* 0xe */
        unsigned char rsvF;     /* 0xf */
        unsigned char CMDND;    /* 0x10 */
        unsigned char CMD;      /* 0x11 */
        unsigned char CDR1;     /* 0x12 */
        unsigned char CDR2;     /* 0x13 */
        unsigned char ICSR;     /* 0x14 */
        unsigned char rsv15;    /* 0x15 */
        unsigned char rsv16;    /* 0x16 */
        unsigned char rsv17;    /* 0x17 */
        unsigned char SID1;     /* 0x18 */
        unsigned char IFR1;     /* 0x19 */
        unsigned short IDR1;    /* 0x1a & 0x1b */
        unsigned char SID2;     /* 0x1c */
        unsigned char IFR2;     /* 0x1d */
        unsigned short IDR2;    /* 0x1e & 0x1f */
        unsigned char SID3;     /* 0x20 */
        unsigned char IFR3;     /* 0x21 */
        unsigned short IDR3;    /* 0x22 & 0x23 */
        unsigned long DADD;     /* 0x24 to 0x27 */
        unsigned char EIS;      /* 0x28 */
        unsigned char ECSR3;    /* 0x29 */
        unsigned char rsv2A;    /* 0x2a */
        unsigned char rsv2B;    /* 0x2b */
        unsigned char rsv2C;    /* 0x2c */
        unsigned char MACR;     /* 0x2d */
        unsigned char rsv2E;    /* 0x2e */
        unsigned char rsv2F;    /* 0x2f */
};


   volatile void *findRfm5579 (unsigned short instance)
   {
      int busId;
      int status;
      unsigned int pmcAdd;

      status = sysFindPciDevice(0x5579, 0x114A, instance, 0, &busId);

      if (status == -1) return (volatile void *) -1;
      else pmcAdd = sysReadPciConfig (busId, status, 0, 5);

      return (volatile void*) pmcAdd;
   }

/*
  Initializes the 5565 PMC reflective memory board, rmAPI
*/
   int rmInit (short ID)
   {
      volatile void *mem;

#if TARGET == (TARGET_L1_GDS_AWG1 + 21)
      if (ID == 1)
#else
      if (ID == 0)
#endif
	{
        volatile struct VMIC5565_CSR *regs;
        regs = (volatile struct VMIC5565_CSR *) findRfm5565 (0, CSR_LOCATOR);
        if (-1 == (int) regs) {
           printf ("rmInit: FATAL ERROR; 5565 PMC not installed\n");
           return (ERROR);
        }
        mem = findRfm5565 (0, MEM_LOCATOR);
        regs -> LCSR1 &= ~TURN_OFF_5565_FAIL;
        rmboard[ID] = rm[ID] = (char*)mem;
        rmsize[ID] =  64 * 1024 *1024;
        rmmaster[ID] = 0;
        printf ("VMIC PMC 5565 (%i MB) installed at 0x%x (node %i)\n",
                 rmBoardSize (0) / (1024 * 1024), (int) mem,
                 regs -> NID);
	}
#if TARGET == (TARGET_L1_GDS_AWG1 + 21)
      else if (ID == 0)
#else
      else if (ID == 1)
#endif
	{
        volatile struct VMIC5579_MEM_REGISTER *csr;
	csr = (volatile struct VMIC5579_MEM_REGISTER *) findRfm5579 (0);
	if (-1 == (int) csr) {
           printf ("rmInit: FATAL ERROR; 5579 PMC not installed\n");
           return (ERROR);
	}
        mem = csr;
        csr->CSR2 = TURN_OFF_FAIL_LITE;
        rmboard[ID] = rm[ID] = (char*)mem;
	rmsize[ID] =  64 * 1024 *1024;
        rmmaster[ID] = 0;
        printf ("VMIC PMC 5579 (%i MB) installed at 0x%x (node %i)\n",
                 rmBoardSize (0) / (1024 * 1024), (int) mem,
                 csr -> NID);
      } else {
         printf ("rmInit: Illegal board ID\n");
         return (ERROR);
      } 
      return 0;
   }

#else

/*
  Initializes the rmAPI
*/
   int rmInit (short ID)
   {
      volatile char* volatile addr;	 /* address */
      int 		baddr;   /* base address */
      int		addrmod; /* address modifier */
      int		ofs;	 /* board offset */
      int		size;	 /* board size */
      int		master;	 /* master driver? */
      int 		status;  /* VME status */
      int		intlevel;/* interrupt level */
      int		intvec;  /* interrupt vector */
      char		test;	 /* VME test byte */
   
      /* get configuration */
      switch (ID) {
         case 0:
            {
               if (VMIVME5588_0_BASE_ADDRESS == 0) {
                  return -1;
               }
               baddr = VMIVME5588_0_BASE_ADDRESS;
               addrmod = VMIVME5588_0_ADRMOD;
               ofs = VMIVME5588_0_BOARD_OFFSET;
               size = VMIVME5588_0_MEM_SIZE;
               master = VMIVME5888_0_MASTER;
               intlevel = VMIVME5588_0_INT_LEVEL;
               intvec = VMIVME5588_0_INT_VEC;
               break;
            }
         case 1:
            {
               if (VMIVME5588_1_BASE_ADDRESS == 0) {
                  return -1;
               }
               baddr = VMIVME5588_1_BASE_ADDRESS;
               addrmod = VMIVME5588_1_ADRMOD;
               ofs = VMIVME5588_1_BOARD_OFFSET;
               size = VMIVME5588_1_MEM_SIZE;
               master = VMIVME5888_1_MASTER;
               intlevel = VMIVME5588_1_INT_LEVEL;
               intvec = VMIVME5588_1_INT_VEC;
               break;
            }
         default:
            {
               return -1;
            }
      }
      rm[ID] = NULL;
      rmboard[ID] = NULL;
      rmsize[ID] = 0;
      rmmaster[ID] = 0;
   
   #ifdef OS_VXWORKS
      /* Obtain board's base address. */
      status = sysBusToLocalAdrs (addrmod, (char*)baddr,
                                (char**)&addr);
      printf("rmInit: sysBusToLocalAdrs = 0x%x (AM 0x%x)\n", (int) baddr, (int) addrmod);
      if(status != OK) {
         printf("rmInit: Error, RM %i sysBusToLocalAdrs\n", ID);
         return(ERROR);
      }
   	
      /* check if board is there */
      printf ("check %X\n", (int) ((char*) addr + ofs));
      if (vxMemProbe ((char*) addr + ofs, VX_READ, 1, &test) == ERROR) {
         printf("rmInit: Error, RM %i not accessable in VME\n", ID);
         return -1;
      }
      else {
         rm[ID] = (char*) addr;
         rmboard[ID] = (char*) addr + ofs;
         rmsize[ID] = size;
         rmmaster[ID] = master;
         printf ("VMIC 5588 (%i MB) installed at %x (node %i)\n",
                 rmBoardSize (ID) / 1048576, (int) addr + ofs, 
                 ((RMREGS*) (addr + ofs))->nid);
      }
   #else
      rm[ID] = baddr;
      rmboard[ID] = baddr + ofs;
      rmsize[ID] = size;
      rmmaster[ID] = master;
   #endif
   
   
      if (!rmMaster (ID)) {
         return 0;
      }
      addr = rmBoardAddress (ID);
   
      /* reset CSR */
      ((RMREGS*) addr)->csr &= 0xF3;
   
      /* Setup Interrupts for */
      /* Flag Auto Clear */
      /* Interrupt Disable */
      ((RMREGS*) addr)->cr0 = 0x48 | intlevel;
      ((RMREGS*) addr)->cr1 = 0x48 | intlevel;
      ((RMREGS*) addr)->cr2 = 0x48 | intlevel;
      ((RMREGS*) addr)->cr3 = 0x48 | intlevel;
   
      /* Set interrupt Vectors */
      ((RMREGS*) addr)->vr0 = intvec;
      ((RMREGS*) addr)->vr1 = intvec + 1;
      ((RMREGS*) addr)->vr2 = intvec + 2;
      ((RMREGS*) addr)->vr3 = intvec + 3;
   
      /* extinguish the Fail LED */
      (void) rmLED (ID, 0);
   
      /* reset latch sync loss bit and switch off node bypass */
      ((RMREGS*) addr)->irs &= ~(0x48);
   
   #ifdef DEBUG
      printf("RM%i: Node ID: %i -- Status Registers (0x%X,0x%X)\n",
            ID, ((RMREGS*) addr)->nid, ((RMREGS*) addr)->irs, 
            ((RMREGS*) addr)->csr);
   #endif
   
      return 0;
   }
#endif /* RMEM_LAYOUT == 0 */


#ifndef __linux__
/*
  Initializes the rmAPI automatically when module is loaded
*/
   __init__ (initReflectiveMemory);
#pragma init (initReflectiveMemory)
#endif

   void initReflectiveMemory (void) 
   {
      memset (rmIntCtrl, 0, sizeof (rmIntCtrl));
      tidISR = 0;
      sem_init (&semISR, 0, 0);
      rmInit (0);
#if TARGET != (TARGET_L1_GDS_AWG1 + 20)
#if TARGET != (TARGET_L1_GDS_AWG1 + 21)
      rmInit (1);
#endif
#endif

      // Open the IPC memory space
      rmInit (2);

#ifdef OS_SOLARIS
      /* See if we have two more RFM cards installed (LHO) */
      if (rmInit (2) >= 0)
        rmInit (3);
      else {
#if 0
	/* point boards 2 and 3 into RAM */
	/* for testing at the 40m */
	static char junk[64 * 1024 *1024];
       rmboard[2] = rm[2] = junk;
       rmboard[3] = rm[3] = junk;
       rmsize[2] = rmsize[3] = 64 * 1024 *1024;
       rmmaster[2] = rmmaster[3] = 0;
#endif
      }
#endif
   }

/*
  generate and interrupt on a node
  num = interrupt number
  node = 0..255 or RMAPI_NODE_ALL
*/
   int rmInt (short ID, int inum, int node)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = rmBoardAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
   #ifdef DEBUG
      if ((inum < 0) || (inum > 3)) {
         printf ("RMAPI: Invalid Interrupt Number\n");
         return -2;
      }
   
      if (((node < 0) || (node > 255)) && (node != RMAPI_NODE_ALL)) {
         printf ("RMAPI: Invalid Node ID\n");
         return -2;
      }
   #endif
   
      /* Issue Interrupt */
      if (node == RMAPI_NODE_ALL) {
         ((RMREGS*) addr)->cmd = (0x43 & inum);
      } 
      else {
         ((RMREGS*) addr)->cmdn = node;
         ((RMREGS*) addr)->cmd = (0x03 & inum);
      }
   
      return 0;
   }

/*
  Resets the given node
  node = 0..255 or RMAPI_NODE_ALL
*/
   int rmResetNode (short ID, int node)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* get base address */
      addr = rmBoardAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
   #ifdef DEBUG
      if ((node < 0 || node > 255) && node != RMAPI_NODE_ALL) {
         printf("RMAPI: Invalid Node ID\n");
         return -2;
      }
   #endif
   
      /* Issue reset */
      if (node == RMAPI_NODE_ALL) {
         ((RMREGS*) addr)->cmd = (0x40);
      } 
      else {
         ((RMREGS*) addr)->cmdn = node;
         ((RMREGS*) addr)->cmd = (0x00);
      }
      return 0;
   }


/*
  Checks the validity of a reflective memory address range
  NOTE: offset must also be 4-byte aligned 
*/

   int rmCheck (short ID, int offset, int size) 
   {
#if RMEM_LAYOUT == 0
      char*	addr;		/* base address */
      char* 	board;		/* board base address */
      int	window1;	/* offset to memory window start */
      int	window2;	/* offset to memory window end */
   
      /* get base and board address; calculate valid memory window */
      addr = rmBaseAddress (ID);
      board = rmBoardAddress (ID);
      if ((addr == NULL) || (board == NULL)) {
         return 0;
      }
      window1 = (int) board - (int) addr + VMIC5588_RAM_OFFSET;
      window2 = (int) board - (int) addr + rmBoardSize (ID);
   
      /* check that offset is above register area */
      if ((offset < window1) || (offset + size > window2)) {
         gdsDebug ("RMAPI: Invalid Memory Range");
         return 0;
      }
   
      /* check that offset mod 4 = 0 */
      if (offset % 4 != 0) {
         gdsDebug ("RMAPI: Invalid Memory Alignment");
         return 0;
      }
#endif
      return 1;
   }


/*
  Actual Interrupt service routine
*/
#ifdef OS_VXWORKS
   static void rmISRfunc (rmISR_ctrl* ctrl)
   {
      volatile char* volatile	addr;	/* base address */
   
      /* check ctrl */
      if (ctrl == NULL) {
         return;
      }
   
      /* get base address */
      addr = rmBoardAddress (ctrl->id);
      if (addr == NULL) {
         return;
      }
   
      /* get status */
      switch (ctrl->inum) {
         case 0:
            ctrl->status = ((RMREGS*) addr)->int_csr;
            ((RMREGS*) addr)->int_csr = ((RMREGS*) addr)->int_csr & 0xEE;
            ((RMREGS*) addr)->csr = ((RMREGS*) addr)->csr & 0xF7;
            if ((ctrl->status & 0x10) != 0) {
               badData++;
            }
            if ((ctrl->status & 0x01) != 0) {
               badFIFO++;
            }
            break;
         case 1:
            ctrl->status = ((RMREGS*) addr)->sid1;
            break;
         case 2:
            ctrl->status = ((RMREGS*) addr)->sid2;
            break;
         case 3:
            ctrl->status = ((RMREGS*) addr)->sid3;
            break;
      }
   
      /* release semaphore */
      ctrlISR = ctrl;
      sem_post (&semISR);
   
      /* reenable interrupt */
      switch (ctrl->inum) {
         case 0:
            ((RMREGS*) addr)->cr0 = ((RMREGS*) addr)->cr0 | 0x10;
            break;
         case 1:
            ((RMREGS*) addr)->cr1 = ((RMREGS*) addr)->cr1 | 0x10;
            break;
         case 2:
            ((RMREGS*) addr)->cr2 = ((RMREGS*) addr)->cr2 | 0x10;
            break;
         case 3:
            ((RMREGS*) addr)->cr3 = ((RMREGS*) addr)->cr3 | 0x10;
            break;
      }
   }


/*
  Delayed Interrupt service routine
*/
   static void rmWaitInt (void) 
   {
      rmISR_ctrl* 		ctrl;
   
      while (sem_wait (&semISR) == 0) {
         ctrl = ctrlISR;
         if ((ctrlISR != NULL) && (ctrl->isr != NULL)) {
            ctrl->isr (ctrl->id, ctrl->inum, ctrl->status, ctrl->arg);
         }
      }
   }
#endif
/*
  Installs an interrupt service routine
*/
   int rmIntInstall (short ID, int inum, rmISR isr, void* arg)
   {
   #ifndef OS_VXWORKS
      return -10;
   #else
      volatile char* volatile	addr;	/* base address */
      rmISR_ctrl*	ctrl;
   
      /* get base address */
      addr = rmBoardAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
      /* check inum */
      if ((inum < 0) || (inum > 3)) {
         return -2;
      }
      ctrl = &rmIntCtrl[ID][inum];
   
      /* remove previuosly installed ISR? */
      if (ctrl->isr != NULL) {
      	 /* remove */
         sysIntDisable (ctrl->level & 0x07);
         switch (inum) {
            case 0:
               ((RMREGS*) addr)->cr0 = ((RMREGS*) addr)->cr0 & 0xEF;
               break;
            case 1:
               ((RMREGS*) addr)->cr1 = ((RMREGS*) addr)->cr1 & 0xEF;
               break;
            case 2:
               ((RMREGS*) addr)->cr2 = ((RMREGS*) addr)->cr2 & 0xEF;
               break;
            case 3:
               ((RMREGS*) addr)->cr3 = ((RMREGS*) addr)->cr3 & 0xEF;
               break;
         }
         intConnect ((VOIDFUNCPTR*) INUM_TO_IVEC(ctrl->vec),
                    (VOIDFUNCPTR) NULL, 0);
         ctrl->isr = NULL;
      }
      /* done? */	
      if (isr == NULL) {
         return 0;
      }
   
      /* install ISR */
      ctrl->id = ID;
      ctrl->inum = inum;
      ctrl->isr = isr;
      switch (inum) {
         case 0:
            ctrl->level = ((RMREGS*) addr)->cr0 & 0x07;
            ctrl->vec = ((RMREGS*) addr)->vr0;
            ((RMREGS*) addr)->int_csr = ((RMREGS*) addr)->int_csr & 0xEE;
            break;
         case 1:
            ctrl->level = ((RMREGS*) addr)->cr1 & 0x07;
            ctrl->vec = ((RMREGS*) addr)->vr1;
            break;
         case 2:
            ctrl->level = ((RMREGS*) addr)->cr2 & 0x07;
            ctrl->vec = ((RMREGS*) addr)->vr2;
            break;
         case 3:
            ctrl->level = ((RMREGS*) addr)->cr3 & 0x07;
            ctrl->vec = ((RMREGS*) addr)->vr3;
            break;
      }
      ctrl->arg = arg;
   
      /* connect ISR */
      if (intConnect ((VOIDFUNCPTR*) INUM_TO_IVEC(ctrl->vec),
         (VOIDFUNCPTR) rmISRfunc, (int) ctrl) != OK) {
         return -3;
      }
      /* enable interrupt */
      switch (inum) {
         case 0:
            ((RMREGS*) addr)->cr0 = ((RMREGS*) addr)->cr0 | 0x58;
            break;
         case 1:
            ((RMREGS*) addr)->sid1 = 0;
            ((RMREGS*) addr)->cr1 = ((RMREGS*) addr)->cr1 | 0x58;
            break;
         case 2:
            ((RMREGS*) addr)->sid2 = 0;
            ((RMREGS*) addr)->cr2 = ((RMREGS*) addr)->cr2 | 0x58;
            break;
         case 3:
            ((RMREGS*) addr)->sid3 = 0;
            ((RMREGS*) addr)->cr3 = ((RMREGS*) addr)->cr3 | 0x58;
            break;
      }
      if (sysIntEnable (ctrl->level & 0x07) != OK) {
         return -4;
      }
   #ifdef DEBUG
      printf ("Installed ISR on %i (vec 0x%x/level 0x%x)\n", 
             inum, ctrl->vec, ctrl->level & 0x07);   
   #endif
   
      /* install ISR processing task */
      if (tidISR == 0) {
      #ifdef OS_VXWORKS
         int attr = VX_FP_TASK;
      #else
         int attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_SYSTEM;
      #endif
         if (taskCreate (attr, 75, &tidISR, "tRmInt", 
            (taskfunc_t) rmWaitInt, NULL) < 0) {
            return -5;
         }
      }
      return 0;
   #endif
   }


/*
  interrupt service routine for watch
*/
   static void watchInt (short ID, int inum, int cause, void* arg)
   {
      char		buf[1024];
      sprintf (buf, "RM interrupt (%i) by board %i: ", inum, ID);
      if (inum == 0) {
         sprintf (strend (buf), "status 0x%x ", cause);
         if ((cause & 0x10) != 0) {
            sprintf (strend (buf), "(data bad %i, ", badData);
         }
         else {
            sprintf (strend (buf), "(data ok, ");
         }
         if ((cause & 0x01) != 0) {
            sprintf (strend (buf), "FIFO bad %i)\n", badFIFO);
         }
         else {
            sprintf (strend (buf), "FIFO ok)\n");
         }
      }
      else {
         sprintf (strend (buf), "node %i\n", cause);
      }
      printf ("%s", buf);
      gdsWarningMessage (buf);
   }

/*
  Installs an interrupt service routine
*/
   int rmWatchInt (short ID, int inum)
   {
      if (inum >= 0) {
         return rmIntInstall (ID, inum, watchInt, NULL);
      }
      else {
         return rmIntInstall (ID, 0, watchInt, NULL) +
            rmIntInstall (ID, 1, watchInt, NULL) +
            rmIntInstall (ID, 2, watchInt, NULL) +
            rmIntInstall (ID, 3, watchInt, NULL);
      }
   }


/*
  Copies from Reflected memory to local memory
  using Baja as DMA controller
  pData = pointer to local memory buffer
  offset = offset into reflected to read
  size = number of bytes to read

  NOTE: offset must be 4-byte aligned 
*/
   int rmRead (short ID, char* pData, int offset, int size, int flag)
   {
      int 		status;
      volatile char* volatile	addr;	/* base address */
   
   #ifdef _RM_RANGE_CHECK
      if (!rmCheck (ID, offset, size)) {
         return -2;
      }
   #endif
   
      /* printf ("rmRead (%i) at ofs=0x%x, size=%i\n", ID, offset, size); */
   
      /* get base address */
      addr = rmBaseAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
      /* Use Baja as DMA controller */
   #if defined(OS_VXWORKS) && defined (PROCESSOR_BAJA)
      if (size > DMA_THRESHOLD) {
         cacheInvalidate (DATA_CACHE, pData, size);
         status = sysVicBlkCopy (pData, (char*) addr + offset, size, 0, 0);
      }
      else {
   #endif
#if defined(OS_SOLARIS)
#if 0
      pread (rfmHandle[ID], pData, size, offset);
#endif

     /* 5579 board */
     if (ID & 1) {
        pread (rfmHandle[ID], pData, size, offset);
     } else {
	/* Do not use read() on 5565 board */
	int i;
	size /= 2;
	for (i = 0; i < size; i++)
	   RFM2gPeek16(rh5565[ID >> 1], offset + 2*i, ((unsigned short *)pData) + i);
     }

#else
      memcpy ((void*) pData, (void*) (addr + offset), size);
#endif
      status = 0;
   #if defined(OS_VXWORKS) && defined (PROCESSOR_BAJA)
      }
   #endif
   
   #ifdef DEBUG
      if(status != 0)
         gdsDebug ("RMAPI: Error sysVicBlkCopy");
   #endif
   
      return status;
   }


/*
  Copies to Reflected memory from local memory
  using Baja as DMA controller
  pData = pointer to local memory buffer
  offset = offset into reflected to read
  size = number of bytes to read

  NOTE: offset must be 4-byte aligned 
*/
   int rmWrite (short ID, char* pData, int offset, int size, int flag)
   {
      int status;
      volatile char* volatile	addr;	/* base address */
   
   #ifdef _RM_RANGE_CHECK
      if (!rmCheck (ID, offset, size)) {
         return -2;
      }
   #endif
   
      /*printf ("rmWrite (%i) at ofs=0x%x, size=%i\n", ID, offset, size); */
   
      /* get base address */
      addr = rmBaseAddress (ID);
      if (addr == NULL) {
         return -1;
      }
   
      /* Use baja as DMA controller */
   #if defined(OS_VXWORKS) && defined (PROCESSOR_BAJA)
      if (size > DMA_THRESHOLD) {
         cacheFlush (DATA_CACHE, pData, size);
         status = sysVicBlkCopy (pData, (char*) addr + offset, size, 1, 0);
         if (status == ERROR) {
            printf("BlkCopy error\n");
            printf ("rmWrite (%i) at ofs=0x%x, size=%i\n", ID, offset, size);
            status = sysVicBlkCopy (pData, (char*) addr + offset, size, 1, 0);
         }
         if (status == ERROR) {
            printf("BlkCopy error on second attempt\n");
            return -3;
         }
      }
      else {
   #endif
   #if defined(OS_VXWORKS) && !defined (PROCESSOR_BAJA)
      /* In VMIC Pentium boards this is important to avoid writing single
	 bytes to an RFM board; memcpy() seems to do just that for size 4 */
      if (size == 4) 
	*((int *)(addr + offset)) = *((int *)pData);
      else 
   #endif
#if defined(OS_SOLARIS)

#if 1
      /* 5565 driver only likes 4 bytes even writes */
      if ((ID & 1) == 0) size += size%4? 4 - size%4: 0;
	
      lockf(rfmHandle[ID], F_LOCK, 0);
      int nwritten = 0;
      if ((ID & 1) == 0) {
	nwritten = pwrite(rfmHandle[ID], pData, size, offset);
      } else {
	int i;
        size /= 2;
        for (i = 0; i < size; i++) {
           ((short *)(addr + offset))[i^1]  = ((short *)pData)[i];
	}
	nwritten = size;
      }
      lockf(rfmHandle[ID], F_ULOCK, 0);
      if (nwritten != size) {
	printf("pwrite(%d) failed; nwritten=%d; errno=%d\n", rfmHandle[ID], nwritten, errno);
      }
#else
 
      if (ID & 1) {
        int nwritten = pwrite(rfmHandle[ID], pData, size, offset);
        if (nwritten != size) {
	  printf("pwrite(%d) failed; nwritten=%d; errno=%d\n", rfmHandle[ID], nwritten, errno);
        }
      } else {
	/* Do not use write() on 5565 board */
	int i;
	size /= 2;
	for (i = 0; i < size; i++)
	   RFM2gPoke16(rh5565[ID >> 1], offset + 2*i, ((short *)pData)[i]);
      }
#endif

#else
      memcpy ((void*) (addr + offset), (void*) pData, size);
#endif
      status =0;
   #if defined(OS_VXWORKS) && defined (PROCESSOR_BAJA)
      }
   #endif	
   #ifdef DEBUG
      if(status != 0)
         gdsDebug ("RMAPI: Error  sysVicBlkCopy.\n");
   #endif
   
      return status;
   }

/*
   void RMTest1()
   {
      char*	buffer; 
   
   #ifdef OS_VXWORKS
      buffer  = (char*) cacheDmaMalloc (51);
   #else
      buffer  = (char*) malloc(51);
   #endif
   
      strcpy (buffer, "rmTest-cache-delay");
      rmWrite (0, buffer, 0x150, strlen (buffer) + 1, 0);
      taskDelay (10);
      printf ("WROTE: %s\n", buffer);
   
      rmRead (1, buffer, 0x150, 50, 0);
      taskDelay (10);
      buffer[50] = "\0";
      printf("READ: %s\n", buffer); 
   
   #ifdef OS_VXWORKS
      cacheDmaFree (buffer);
   #else
      free (buffer);
   #endif
      return;
   }


   void RMTest2()
   {
      char *buffer; 
      buffer  = malloc(51);
   
      strcpy(buffer, "rmTest-malloc-delay");
      rmWrite(buffer, 0x50,strlen(buffer));
      taskDelay(90);
      printf("WROTE: %s\n",buffer);
   
      rmRead(buffer, 0x50, 50);
      taskDelay(90);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      free(buffer); 
      return;
   }

   void RMTest3()
   {
      char  buffer[51]; 
   
      strcpy(buffer, "rmTest-stack-delay");
      rmWrite(buffer, 0x50,strlen(buffer));
      taskDelay(90);
      printf("WROTE: %s\n",buffer); 
   
      rmRead(buffer, 0x50, 50);
      taskDelay(90);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      return;
   }

   void RMTest4()
   {
      char *buffer; 
      buffer  = (char*) cacheDmaMalloc(51);
   
      strcpy(buffer, "rmTest-cache-nodelay");
      rmWrite(buffer, 0x50,strlen(buffer));
      printf("WROTE: %s\n",buffer);
   
      rmRead(buffer, 0x50, 50);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      cacheDmaFree(buffer); 
      return;
   }

   void RMTest5()
   {
      char *buffer; 
      buffer  = malloc(51);
   
      strcpy(buffer, "rmTest-malloc-nodelay");
      rmWrite(buffer, 0x50,strlen(buffer));
      printf("WROTE: %s\n",buffer);
   
      rmRead(buffer, 0x50, 50);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      free(buffer); 
      return;
   }

   void RMTest6()
   {
      char  buffer[51]; 
   
      strcpy(buffer, "rmTest-stack-nodelay");
      rmWrite(buffer, 0x50,strlen(buffer));
      printf("WROTE: %s\n",buffer);
   
      rmRead(buffer, 0x50, 50);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      return;
   }

   RMTest7()
   {
      char *buffer; 
      buffer  = malloc(51);
   
      strcpy(buffer, "rmTest-try-to-overwrite");
      rmWrite(buffer, 0x50,strlen(buffer));
   
      buffer[0] = 'P';
      buffer[1] = 'G';
   
      printf("Trying to overwrite buffer during DMA...\n");
      taskDelay(90);
   
      rmRead(buffer, 0x50, 50);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      free(buffer); 
      return;   
   }

   RMTest8()
   {
      char *buffer; 
      buffer  = (char*) cacheDmaMalloc(51);
   
      strcpy(buffer, "rmTest-try-to-overwrite");
      rmWrite(buffer, 0x50,strlen(buffer));
   
      buffer[0] = 'P';
      buffer[1] = 'G';
   
      printf("Trying to overwrite buffer during DMA...\n");
      taskDelay(90);
   
      rmRead(buffer, 0x50, 50);
      buffer[50] = 0;
      printf("READ: %s\n",buffer); 
   
      cacheDmaFree(buffer);
      return;   
   }

   void rmtest()
   {
      RMTest1();
      RMTest2();
      RMTest3();
      RMTest4();
      RMTest5();
      RMTest6();
      RMTest7();
      RMTest8();
   }

*/




