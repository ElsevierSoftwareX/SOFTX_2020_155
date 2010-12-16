static char *versionId = "Version $Id$" ;


/* Header File List: */
#include <math.h>
#include <fcntl.h>
#include <time.h>       /* General Time definitions */
#include <string.h>     /* VxWorks strings files */
#include <limits.h>
#include <sys/types.h>
#ifdef OS_VXWORKS
#include <vxWorks.h>    /* Generic VxWorks header file */
/*#include <semLib.h>      VxWorks Semaphores */
#include <sysLib.h>     /* VxWorks system library */
#include <stdio.h>      /* VxWorks standard io */
#include <stdlib.h>     /* VxWorks function prototypes */
#include <taskLib.h>    /* VxWorks Task Definitions */
#include <intLib.h>     /* VxWorks Interrupts Definitions */
#include <vme.h>        /* VME access defines */
#include <vxLib.h>
#include <iv.h>		/* Interrupt Vector Header File. */
#include <ioLib.h>
#endif
#include "dtt/hardware.h"   /* DAQ hardware configuration header file */
#ifdef ICS115_0_USE_TIMINGCARD
#include "dtt/timingcard.h"
#endif
#ifdef LIGO_GDS
#include "dtt/gdsheartbeat.h"
#else
#include "dtt/getgps.h"
#include "dtt/gps.h"
#define GPS_BASE_ADDRESS   0x010000
#define GPS_ADRMOD            0x3d 
#endif
#ifdef OS_VXWORKS
#include "dtt/ics115.h"
#include "dtt/gdsics115.h"
#endif
#include "dtt/gdsdac.h"

#ifdef OS_VXWORKS
#define BCD_GPS_SUBSECONDS 1
#define CHN_NUM  ICS115_0_CHN_NUM	/* number of channels */
#define LENGTH   (CHN_NUM * 1024)	/* Programmed length of swing buffer */

   static const int  intlvl = ICS115_0_INT_LEVEL;  /* VME Interrupt level for ICS-115 */
   static const int  ivector = ICS115_0_INT_VEC;   /* VME Interrupt vector for ICS-115 */
   static sem_t* semDAC = NULL;
   static char  *baseAddress;

#ifndef LIGO_GDS
   static unsigned long lastTimeSec;
   static unsigned long lastTimeNSec;
   static unsigned long currentTimeSec;
   static unsigned long currentTimeNSec;
   static unsigned long deltaTime;
   static unsigned short *tm = (short*) 0x1e010000;  /* GPS board used for timing writes*/
#endif

/* The following are structure definitions required for configuring the ICS-115 parameters */
#ifdef OS_VXWORKS
#ifdef PROCESSOR_BAJA47
   static ICS115_CONTROL Control = {
   0,                       /* reserved bits */
   0,                       /* PLL range = ? */
   0,                       /* Conversion not enabled */
   0,                       /* synchronous mode  */
   ICS115_LEVEL,            /* level trigger */
   ICS115_INTERNAL,         /* internal trigger */
   ICS115_EXTERNAL,         /* external clock */
   ICS115_CONT,             /* Continuous mode */
   ICS115_VME,              /* VME supplies the data */
   0,                       /* diagnostics disabled */
   0,                       /* disable DAC interrupts */
   0};                      /* disable VME interrupts */

   static ICS115_CONFIG Config = {
   0,                       /* reserved bits */
   CHN_NUM / 2 - 1,         /* thirty-two packed channels in */
   CHN_NUM - 1,             /* thirty-two channels out */
   0,                       /* reserved bits */
   0,                       /* swing buffer not split */
   LENGTH-1,                /* 64K swing buffer */
   0,                       /* reserved bits */
   0,                       /* no decimation */
   0,                       /* reserved bits */
   0};                      /* frame counter not used */

#elif defined(PROCESSOR_I486)
   static ICS115_CONTROL Control = {
   0,                       /* disable VME interrupts */
   0,                       /* disable DAC interrupts */
   0,                       /* diagnostics disabled */
   ICS115_VME,              /* VME supplies the data */
   ICS115_CONT,             /* Continuous mode */
   ICS115_EXTERNAL,         /* external clock */
   ICS115_INTERNAL,         /* internal trigger */
   ICS115_LEVEL,            /* level trigger */
   0,                       /* synchronous mode  */
   0,                       /* Conversion not enabled */
   0,                       /* PLL range = ? */
   0};                      /* reserved bits */                     

   static ICS115_CONFIG Config = {
   CHN_NUM - 1,             /* thirty-two channels out */
   CHN_NUM / 2 - 1,         /* thirty-two packed channels in */
   0,                       /* reserved bits */
   
   LENGTH-1,                /* 64K swing buffer */
   0,                       /* swing buffer not split */
   0,                       /* reserved bits */

   0,                       /* no decimation */
   0,                       /* reserved bits */
   
   0,                       /* frame counter not used */
   0};                      /* reserved bits */

#else
#error "Define ICS115 board parameters"
#endif
#endif

/* The following is the sequencer data for the ICS-115 */
   static unsigned long seq[32] = {
   0x00000000,              /* channel 31 data */
   0x00018000,              /* channel 30 data */
   0x00000001,
   0x00018001,
   0x00000002,
   0x00018002,
   0x00000003,
   0x00018003,
   0x00000004,
   0x00018004,
   0x00000005,
   0x00018005,
   0x00000006,
   0x00018006,
   0x00000007,
   0x00018007,
   0x00000008,
   0x00018008,
   0x00000009,
   0x00018009,
   0x0000000a,
   0x0001800a,
   0x0000000b,
   0x0001800b,
   0x0000000c,
   0x0001800c,
   0x0000000d,
   0x0001800d,
   0x0000000e,
   0x0001800e,
   0x0000000f,
   0x0001800f};             /* Channel 0 data */


/* VxWorks BSP functions not prototyped */
#if defined(OS_VXWORKS) && defined(PROCESSOR_BAJA47)
   extern STATUS sysVicBlkCopy(char *, char *,int, BOOL, BOOL);
#endif
   static void dacIsr (void);

#ifndef LIGO_GDS
   static int initGPS();
   static int hex2dec(int hexNumber);
   static void gps(long *seconds,long *nanoseconds,int YEAR_PERIOD);
#endif
#endif


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: dacInit                                     */
/*                                                                      */
/* Procedure Description: Initializes the ICS115 DAC board for use by   */
/*              	  the arbitrary waveform generator.		*/
/*                                                                      */
/* Procedure Arguments: short ID, sem_t* semIsr.                        */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int dacInit(short ID, sem_t* semIsr)
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
      int 		err;
      int 		status;
      short 		val;
      unsigned long 	muter/*, rseq[32]*/;
   
      union {
         ICS115_CONTROL check;
         unsigned long  control_reg;
      }u;
   
      /* Obtain board's base address. */
      status = sysBusToLocalAdrs(ICS115_0_ADRMOD,
                                (char*)ICS115_0_BASE_ADDRESS,
                                (char**)&baseAddress);
      printf("dacInit: sysBusToLocalAdrs = 0x%x\n", (int) baseAddress);
      if(status != OK) {
         printf("dacInit: Error, sysBusToLocalAdrs\n");
         return(ERROR);
      }
   
      /* Check that ICS115 is present */
      status = vxMemProbe((char *)baseAddress,
                         VX_READ, 4, (char *)&val);
      if (status != OK) {
         printf("dacInit: Error, vxMemProbe\n");
         return(ERROR);
      }
   
      /* Initialize 115 board */
      if(ICS115BoardInit(baseAddress, intlvl, ivector) == ERROR) {
         printf("\nUnable to initialize ICS-115 board");
         return(ERROR);
      }
   
      /* Setup interrupt stuff */
      if (semIsr != 0) {
         semDAC = semIsr;
      }
      /* Connect DAC interrupt to ISR and enable */
      printf("connecting Int Vector 0x%x to dacIsr\n",ivector);
      status = intConnect((VOIDFUNCPTR *)INUM_TO_IVEC(ivector),
                         (VOIDFUNCPTR)dacIsr, 0);
      if (status != OK) {
         printf("dacInit :Error: intConnect semDAC\n");
         return(ERROR);
      }
      printf("enabling Int level %d\n",intlvl);
      status = sysIntEnable(intlvl);
      if (status != OK) {
         printf("dacInit :Error: sysIntEnable out of range %d\n",
               intlvl);
         return(ERROR);
      }
   
      /* Reset board */
      if((err=ICS115BoardReset(baseAddress)) != 0) {
         printf("Board Reset error = %d\n", err);
         return(err);
      }
   
      /* Set up Configuration Register according to Config structure */
      if((err=ICS115ConfigSet(baseAddress, &Config)) != 0) {
         printf("Config Set error = %d\n", err);
         return(err);
      }
      /* Set up the sequencer */
      if((err=ICS115SeqSet(baseAddress, seq)) != 0) {
         printf("Sequence Set error = %d\n", err);
         return(err);
      }
   
      /* Enable all channel outputs */
      muter=0xFFFFFFFF;   
      if((err=ICS115MuteSet(baseAddress, &muter)) != 0) {
         printf("Mute error = %d\n", err);
         return(err);
      }
      printf("Mute Reg = 0x%lx\n", muter);
   
      /* Set up Control Register according to Control structure */
      if((err=ICS115ControlSet(baseAddress, &Control)) != 0) {
         printf("Control Set error = %d\n", err);
         return(err);
      }
      printf("Control Reg = 0x%lx\n", u.control_reg);
   
      /* MUST reset DAC memory before writing to it */
      if((err=ICS115DACReset(baseAddress)) != 0) {
         printf("DAC Reset error = %d\n", err);
         return(err);
      }
   
      /* Initialize timing card */
   #ifdef ICS115_0_USE_TIMINGCARD
      if((status = initTimingCard()) != OK) {
         printf("dacInit: Error, initTimingCard  0x%x\n",status);
             /* Cannot proceed without this hardware */
         return(ERROR);
      }
   #endif 
      /* Initialize GPS card */
   #ifndef LIGO_GDS
      if((status = initGPS()) != OK) {
         printf("dacInit: Error, initGPS  0x%x\n",status);
             /* Cannot proceed without this hardware */
         return(ERROR);
      }
   #endif
   
      {
      int i;
      unsigned long iv, vi, x;
      for (i = 0; i < 1; ++i) {
	 taskDelay(1);
         iv = ivector;
	 vi = intlvl;
	 *(unsigned long*)(baseAddress+SCV64_IVECT) = iv;
	 *(unsigned long*)(baseAddress+SCV64_VINT) = vi;
         iv = *(unsigned long*)(baseAddress+SCV64_IVECT);
         vi = *(unsigned long*)(baseAddress+SCV64_VINT);
         printf ("IVEC = 0x%04lx, VINT = 0x%04lx\n", iv, vi);
	 x = *(unsigned long*)(baseAddress+0x48010);
	 printf ("Base = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+SCV64_MODE);
	 printf ("Mode = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+DAC_STAT_OFFSET);
	 printf ("Status = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+DAC_CTRL_OFFSET);
	 printf ("Control = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+VSB_CTRL_OFFSET);
	 printf ("VSB = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+DAC_CONFIG_OFFSET);
	 printf ("Config 1 = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+DAC_CONFIG_OFFSET+4);
	 printf ("Config 2 = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+DAC_CONFIG_OFFSET+8);
	 printf ("Config 3 = 0x%08lx\n", x);
	 x = *(unsigned long*)(baseAddress+DAC_CONFIG_OFFSET+12);
	 printf ("Config 4 = 0x%08lx\n", x);
      }
      }
      return(OK);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: dacReinit                                   */
/*                                                                      */
/* Procedure Description:  Sets up the DAC and arms the 1Hz GPS clock.  */
/*			   This routine is called at least once after   */
/*			   dacInit and whenever the arbitrary waveform  */
/*			   generator has lost synchronization with the  */
/*			   DAC.  					*/
/*                                                                      */
/* Procedure Arguments: short ID  					*/
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int dacReinit (short ID)
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
   
      int  err;
      unsigned long 	muter;
   
      /* make sure interrupt is disabled */
      ICS115IntDisable(baseAddress);
   
      /* Initialize 115 board */
      if(ICS115BoardInit(baseAddress, intlvl, ivector) == ERROR) {
         printf("\nUnable to initialize ICS-115 board");
         return(ERROR);
      }
   
      /* board reset */
      if((err=ICS115BoardReset(baseAddress)) != 0) {
         printf("Board Reset error = %d\n", err);
         return(err);
      }
   
      /* Set up Configuration Register according to Config structure */
      if((err=ICS115ConfigSet(baseAddress, &Config)) != 0) {
         printf("Config Set error = %d\n", err);
         return(err);
      }
   
       /* Set up the sequencer */
      if((err=ICS115SeqSet(baseAddress, seq)) != 0) {
         printf("Sequence Set error = %d\n", err);
         return(err);
      }
   
      /* Set mute reg */
      muter=0xFFFFFFFF;   /* Enable all channel outputs */
      if((err=ICS115MuteSet(baseAddress, &muter)) != 0) {
         printf("Mute error = %d\n", err);
         return(err);
      }
   
      /* now (re)set control register */
      if((err=ICS115ControlSet(baseAddress, &Control)) != 0) {
         printf("Control Set error = %d\n", err);
         return(err);
      }
   
      /* DAC reset */
      if((err=ICS115DACReset(baseAddress)) != 0) {
         printf("DAC Reset error = %d\n", err);
         return(err);
      }
   
      /* ARM THE TIMING CARD in middle of the current second */
   #ifdef ICS115_0_USE_TIMINGCARD
      resetTimingCard();
      /* wait for an epoch between 3 and 9 */
   #ifdef LIGO_GDS
      {
         taisec_t tai;	/* time of current heartbeat */
         int	  epoch;/* epoch of current heartbeat */
         do {
            syncWithHeartbeatEx (&tai, &epoch);
         } while ((epoch < 3) && (epoch > 9));
      }
   #else /* LIGO_GDS */
      gps(&lastTimeSec,&lastTimeNSec,80);
      while ((lastTimeNSec < 200000000) || (lastTimeNSec > 600000000)) {
         taskDelay(1);
         gps(&lastTimeSec,&lastTimeNSec,80);
      }
   #endif /* LIGO_GDS */
   
      armingTimingCard ();
   #ifdef LIGO_GDS
      printf("Timing Card armed at %ld nSec\n", 
             (unsigned long)(TAInow() % _ONESEC));
   #else /* LIGO_GDS */
      printf("Timing Card armed at %ld nSec\n",lastTimeNSec);
   #endif /* LIGO_GDS */
   #endif /* ICS115_0_USE_TIMINGCARD */
   
      return(OK);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: dacRestart                                  */
/*                                                                      */
/* Procedure Description:  Loads the first two buffers into the DAC and */
/*			   enables the outputs and interrupts.          */
/*			   If no timing board is used, the routine 	*/
/*			   checks the GPS time to synchronize the	*/
/*			   the start of the conversion with the 1PPS.	*/
/*                                                                      */
/* Procedure Arguments: short ID, short* buf0, short* buf1, int len.    */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int dacRestart (short ID, short* buf0, short* buf1, int len)
   
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
   
      int  err;
      int status;
      char *address;
   
      /* Write to first half of swing buffer */
      /* Write 16384 long words of data from outdata to 115 */
      address = (char *)baseAddress;
   #ifdef PROCESSOR_BAJA47
      status = sysVicBlkCopy((char *)buf0,
         (char *)address, 2*len, 1, 0);
      if(status == ERROR){
         printf("Write 1 error = %d\n", ERROR);
         return(status);
      }
   #else
      memcpy (address, buf0, 2*len);
   #endif
      taskDelay (1);
   
      /*Write to second half of swing buffer  */
      /* Write 16384 long words of data from outdata to 115 */
      address = (char *)baseAddress;
   #ifdef PROCESSOR_BAJA47
      status = sysVicBlkCopy((char *)buf1,
         (char *)address, 2*len, 1, 0);
      if(status == ERROR){
         printf("Write 2 error = %d\n", ERROR);
         return(status);
      }
   #else
      memcpy (address, buf1, 2*len);
   #endif
   
      /* if no timing card, use gps hearbeat to synchronize */
   #ifndef ICS115_0_USE_TIMINGCARD
   #ifdef LIGO_GDS
      {
         /* Monitor time, wait until near end of second. */
         taisec_t tai;	/* time of current heartbeat */
         int	  epoch;/* epoch of current heartbeat */
         do {
            syncWithHeartbeatEx (&tai, &epoch);
         } while (epoch > 8);
         if (epoch != 0) {
            /* missed it */
            return ERROR;
         }
      }
   #else /* LIGO_GDS */
      gps(&lastTimeSec,&lastTimeNSec,80);
      while(lastTimeNSec<970000000) {
         gps(&lastTimeSec,&lastTimeNSec,80);
             /* Delay so we dont fill up the VMETRO with GPS time reads. */
         taskDelay(1);
      }
      printf("homing in on start time %ldnsec into second\n",lastTimeNSec);
   
           /* Now get very close to the 1 second point */
      gps(&lastTimeSec,&lastTimeNSec,80);
      while(lastTimeNSec<999986000) {
         gps(&lastTimeSec,&lastTimeNSec,80);
      }
   #endif /* LIGO_GDS */
   #endif /* ICS115_0_USE_TIMINGCARD */
   
      /* Following function call should cause DAC to output 
         selected waveform */
      if((err=ICS115DACEnable(baseAddress)) != 0) {
         printf("Enable error = %d\n", err);
         return(err);
      }
      /* Enable DAC interrupt */
      status = ICS115IntEnable(baseAddress);
      if(status == ERROR) {
         printf("ics115 - DACInt error\n");
         return(ERROR);
      }
   
      return(OK);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: dacCopyData                                 */
/*                                                                      */
/* Procedure Description:  copies data into the DAC buffer		*/
/*                                                                      */
/* Procedure Arguments: short ID, short* buf0, int len.      		*/
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int dacCopyData (short ID, short* buf0, int len)
   
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
   
      int status;
      char *address;
   
      /* Write data from buffer to 115 */
      address = (char *)baseAddress;
   #ifdef PROCESSOR_BAJA47
      status = sysVicBlkCopy ((char*)buf0,
                             (char*)address, 2*len, 1, 0);
      if(status == ERROR) {
         printf("ics115 - BlkCopy error\n");
         return(ERROR);
      }
   #else
      memcpy (address, buf0, 2*len);
   #endif
   
      /* Enable DAC interrupt */
      status = ICS115IntEnable(baseAddress);
      if(status == ERROR) {
         printf("dacCopyData IntEnable error\n");
         return(ERROR);
      }
   
      return(OK);
   #endif
   }


/*---------------------------------------------------------*/
/*                                                         */
/* Internal Procedure Name: dacIsr                         */
/*                                                         */
/* Procedure Description: Interrupt Service Routine for    */
/*                        handling ics115 DAC interrupts   */
/*                        Give dacIsrSem for futher        */
/*                        processing.                      */
/*                                                         */
/* Procedure Arguments:                                    */
/*                                                         */
/* Procedure Returns:                                      */
/*                                                         */
/* Note: This code runs in the ISR context.                */
/*                                                         */
/*---------------------------------------------------------*/
#ifdef OS_VXWORKS
   void dacIsr (void)
   {
      /* disable interrupt and post semaphore */
      ICS115IntDisable(baseAddress);
      sem_post(semDAC);
   }
#endif

/*---------------------------------------------------------*/
/*                                                         */
/* Internal Procedure Name: dacStop                        */
/*                                                         */
/* Procedure Description: Disable the interrupts, close	   */
/*			  the device and reset the board.  */
/*                                                         */
/* Procedure Arguments:                                    */
/*                                                         */
/* Procedure Returns:                                      */
/*                                                         */
/* Note: This code runs in the ISR context.                */
/*                                                         */
/*---------------------------------------------------------*/
   int dacStop (short ID)
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
      ICS115IntDisable(baseAddress);
   
      /* Reset Board */
      ICS115BoardReset(baseAddress);
   
      /* Disable DAC*/
      ICS115DACDisable(baseAddress);
   
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: dacConvertData                              */
/*                                                                      */
/* Procedure Description:  converts data into DAC format		*/
/*                                                                      */
/* Procedure Arguments: DAC buffer, channel array, channel #, length	*/
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int dacConvertData (short* buf, float* data, int chnnum, int len)
   
   {
      int		k;	/* channel data index */
      float		val;	/* data value */
      short*		p;	/* pointer to DAC buffer */
   
      if ((buf == 0) || (data == 0) || (chnnum < 0) || 
         (chnnum >= ICS115_0_CHN_NUM)) {
         return -1;
      }
      for (k = 0, p = buf + chnnum; k < len; k++, p += ICS115_0_CHN_NUM) {
         val = floor (ICS115_0_CONVERSION * data[k]);
         if (fabs(val) >= (float)SHRT_MAX) {
            *p = (val > 0) ? SHRT_MAX : -SHRT_MAX;
         }
         else {
            *p = (short) val;
         }
      }	
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: dacError                                    */
/*                                                                      */
/* Procedure Description: Returns true, if a DAC timing error occured   */
/*                                                                      */
/* Procedure Arguments: short ID.		                        */
/*                                                                      */
/* Procedure Returns: true on error.                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int dacError (short ID)
   {
   #ifndef OS_VXWORKS
      return 0;
   #else
   #ifdef ICS115_0_USE_TIMINGCARD
      return (statusTimingCard () & 0x20) != 0;
   #else
      return 0;
   #endif
   #endif
   }


#ifndef LIGO_GDS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initGPS	        		      	*/
/*                                                         		*/
/* Procedure Description: Initialize GPS card   .                       */
/*                                                         		*/
/* Procedure Arguments: none.						*/
/*                                                         		*/
/* Procedure Returns: OK or ERROR					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int initGPS()
   {
      unsigned short val;
      int status;
      int addrMod;
      int baseAddr;
      unsigned short *tcPtr;   
   
   
      /* Decode GPS base address */
      baseAddr = GPS_BASE_ADDRESS;
      addrMod = GPS_ADRMOD;
   
      status = sysBusToLocalAdrs(addrMod,
                                (char *)(baseAddr),
                                (char **)&tcPtr);
      if (status != OK) {
         printf("initGPS: Error, sysBusToLocalAdrs\n");
         return(ERROR);
      }
   
      /* Check that GPS is present */
      status = vxMemProbe((char *)tcPtr,
                         VX_READ,
                         2,
                         (char *)&val);
      if (status != OK) {
         printf("initGPS: Error, vxMemProbe\n");
         return(ERROR);
      }
   
      return(OK);
   }

/*---------------------------------------------------------*/
/*                                                         */
/* Internal Procedure Name: gps				   */
/*                                                         */
/* Procedure Description: Get GPS timestamp data           */
/*                                                         */
/* Procedure Arguments:					   */
/*                                                         */
/* Procedure Returns:					   */
/*	Time in seconds and nanoseconds + year		   */
/*                                                         */
/* Note: This code runs in the ISR context.		   */
/*                                                         */
/*---------------------------------------------------------*/
   void gps(long *seconds, long *nanoseconds, int YEAR_PERIOD)
   {
      int *ptr0;
      int *ptr1;
      unsigned int timelo;
      unsigned int timehi;
      int year,day,hour,min,sec,msec,usec;
      unsigned int mask;
      unsigned long nsec;
      int j;
   
   
      ptr0 = (unsigned int *)GPS_Sec40_Usec1;
      ptr1 = (unsigned int *)GPS_Year8_Min1;
   
      timelo = (unsigned int)*ptr0;
      timehi = (unsigned int)*ptr1;
   
   
   /* UTC time */
      year = hex2dec((timehi&YEAR_MASK)>>YEAR_SHIFT);
      year += YEAR_PERIOD;
      year += 3;
      day = hex2dec((timehi&DAY_MASK)>>DAY_SHIFT);
      hour = hex2dec((timehi&HOUR_MASK)>>HOUR_SHIFT);
      min = hex2dec(timehi&MIN_MASK);
      sec = hex2dec((timelo&SEC_MASK)>>SEC_SHIFT);
   
      if (BCD_GPS_SUBSECONDS) {
      /* Old style cards, all subseconds in BCD format */
         msec = hex2dec((timelo&MSEC_MASK)>>MSEC_SHIFT);
         usec = hex2dec(timelo&USEC_MASK);
      } 
      else {
      /* New type cards, bits 24-0 represent 2^-1 to 2^-24 */
         nsec=0;
         mask = 0x800000;
      
         for(j=0;j<24;j++) {
            if (timelo&mask) {
               nsec += nsecLookUp[j];
            }
            mask = mask>>1;
         }
      }
        /* transfer time year-1-1-hour-min-second to seconds */
   
      *seconds = Y98;
        /* adding days of the current year */
      *seconds += day*86400;
   /* get GPS time in seconds: GPS=UTC+11  */
      *seconds += hour*3600;
      *seconds += min*60;
      *seconds += sec;
   /* GPS time in nanoseconds  */
      if (BCD_GPS_SUBSECONDS) {
         *nanoseconds = msec*1000000.0+usec*1000.0;
      } 
      else {
         *nanoseconds = nsec;
      
      }
   
      return;
   }


/*---------------------------------------------------------*/
/*                                                         */
/* Internal Procedure Name: hex2dec			   */
/*                                                         */
/* Procedure Description: Convert hex number into decimal  */
/*                                                         */
/* Procedure Arguments:	int hexNumber  Hex number.     	   */
/*                                                         */
/* Procedure Returns:					   */
/*	decimal number                                     */
/*                                                         */
/*                                                         */
/*---------------------------------------------------------*/

   int hex2dec(int hexNumber) {
      int decNumber;
      decNumber=0;
      if(hexNumber>65535)
      {
         decNumber += (1000*(hexNumber/65536));
         hexNumber -= 65536*(hexNumber/65536);
      }
      if(hexNumber>255)
      {
         decNumber += (100*(hexNumber/256));
         hexNumber -= 256*(hexNumber/256);
      }
      if(hexNumber>15)
      {
         decNumber += (10*(hexNumber/16));
         hexNumber -= 16*(hexNumber/16);
      }
      decNumber += hexNumber;
   
      return decNumber;
   }
#endif

