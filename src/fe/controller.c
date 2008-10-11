/*----------------------------------------------------------------------*/
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2005.                      */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 18-34                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

#include <rtl_time.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <linux/slab.h>
#include <drv/cdsHardware.h>
#include "inlineMath.h"
#include "feSelectHeader.h"
#include <string.h>

#ifndef NUM_SYSTEMS
#define NUM_SYSTEMS 1
#endif

#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)
char *_epics_shm;		/* Ptr to computer shared memory		*/
char *_ipc_shm;			/* Ptr to inter-process communication area */
#if defined(SHMEM_DAQ)
char *_daq_shm;			/* Ptr to frame builder comm shared mem area */
int daq_fd;			/* File descriptor to share memory file */
#endif

long daqBuffer;			/* Address for daq dual buffers in daqLib.c	*/
sem_t irqsem;			/* Semaphore if in IRQ mode.			*/
CDS_HARDWARE cdsPciModules;	/* Structure of hardware addresses		*/
volatile int vmeDone = 0;
volatile int stop_working_threads = 0;

#include "msr.h"
#include "fm10Gen.h"		/* CDS filter module defs and C code	*/
#include "feComms.h"		/* Lvea control RFM network defs.	*/
#include "daqmap.h"		/* DAQ network layout			*/
extern unsigned int cpu_khz;
#define CPURATE	(cpu_khz/1000)
#define ONE_PPS_THRESH 2000

// #include "fpvalidate.h"		/* Valid FP number ck			*/
#ifndef NO_DAQ
#include "drv/gmnet.h"
#include "drv/fb.h"
#include "drv/myri.h"
#include "drv/daqLib.c"		/* DAQ/GDS connection 			*/
#endif
#include "drv/map.h"
#include "drv/epicsXfer.c"	/* Transfers EPICS data to/from shmem	*/

#ifdef SERVO256K
        #define CYCLE_PER_SECOND        (2*131072)
        #define CYCLE_PER_MINUTE        (2*7864320)
        #define DAQ_CYCLE_CHANGE        (2*8000)
        #define END_OF_DAQ_BLOCK        (2*8191)
        #define DAQ_RATE                (2*8192)
        #define NET_SEND_WAIT           (2*655360)
        #define CYCLE_TIME_ALRM         4
#endif
#ifdef SERVO128K
        #define CYCLE_PER_SECOND        131072
        #define CYCLE_PER_MINUTE        7864320
        #define DAQ_CYCLE_CHANGE        8000
        #define END_OF_DAQ_BLOCK        8191
        #define DAQ_RATE                8192
        #define NET_SEND_WAIT           655360
        #define CYCLE_TIME_ALRM         7
#endif
#ifdef SERVO64K
	#define CYCLE_PER_SECOND	(2*32768)
	#define CYCLE_PER_MINUTE	(2*1966080)
	#define DAQ_CYCLE_CHANGE	(2*1540)
	#define END_OF_DAQ_BLOCK	(2*2047)
	#define DAQ_RATE	(DAQ_16K_SAMPLE_SIZE*4)
	#define NET_SEND_WAIT		(2*81920)
	#define CYCLE_TIME_ALRM		15
	#define DAC_START_DELAY		5
#endif
#ifdef SERVO32K
	#define CYCLE_PER_SECOND	32768
	#define CYCLE_PER_MINUTE	1966080
	#define DAQ_CYCLE_CHANGE	1540
	#define END_OF_DAQ_BLOCK	2047
	#define DAQ_RATE	(DAQ_16K_SAMPLE_SIZE*2)
	#define NET_SEND_WAIT		(2*81920)
	#define CYCLE_TIME_ALRM		30
	#define DAC_START_DELAY		10
#endif

#ifdef SERVO16K
	#define CYCLE_PER_SECOND	16384
	#define CYCLE_PER_MINUTE	983040
	#define DAQ_CYCLE_CHANGE	770
	#define END_OF_DAQ_BLOCK	1023
	#define DAQ_RATE	DAQ_16K_SAMPLE_SIZE
	#define NET_SEND_WAIT		81920
	#define CYCLE_TIME_ALRM		60
	#define DAC_START_DELAY		40
#endif
#ifdef SERVO2K
	#define CYCLE_PER_SECOND	2048
	#define CYCLE_PER_MINUTE	122880
	#define DAQ_CYCLE_CHANGE	120
	#define END_OF_DAQ_BLOCK	127
	#define DAQ_RATE	DAQ_2K_SAMPLE_SIZE
	#define NET_SEND_WAIT		10240
	#define CYCLE_TIME_ALRM		487
	#define DAC_START_DELAY		468
#endif

#ifndef NO_DAQ
DAQ_RANGE daq;			/* Range settings for daqLib.c		*/
int numFb = 0;
int fbStat[2] = {0,0};
#endif

rtl_pthread_t wthread;
rtl_pthread_t wthread1;
rtl_pthread_t wthread2;
int wfd, ipc_fd;

/* ADC/DAC overflow variables */
int overflowAdc[4][32];;
int overflowDac[4][16];;
int overflowAcc = 0;

float *testpoint[20];


CDS_EPICS *pLocalEpics;   	/* Local mem ptr to EPICS control data	*/


/* 1/16 sec cycle counters for DAQS and ISC RFM IPC		*/
int subcycle = 0;		/* Internal cycle counter	*/ 
unsigned int daqCycle;		/* DAQS cycle counter		*/

int firstTime;			/* Dummy var for startup sync.	*/

FILT_MOD dsp[NUM_SYSTEMS];			/* SFM structure.		*/
FILT_MOD *dspPtr[NUM_SYSTEMS];		/* SFM structure pointer.	*/
FILT_MOD *pDsp[NUM_SYSTEMS];			/* Ptr to SFM in shmem.		*/
COEF dspCoeff[NUM_SYSTEMS];	/* Local mem for SFM coeffs.	*/
VME_COEF *pCoeff[NUM_SYSTEMS];		/* Ptr to SFM coeffs in shmem		*/
double dWord[MAX_ADC_MODULES][32];
unsigned int dWordUsed[MAX_ADC_MODULES][32];
double dacOut[MAX_DAC_MODULES][16];
unsigned int dacOutUsed[MAX_DAC_MODULES][16];
int dioInput[MAX_DIO_MODULES];
int dioOutput[MAX_DIO_MODULES];
int rioInput[MAX_DIO_MODULES];
int rioOutput[MAX_DIO_MODULES];
int rioInput1[MAX_DIO_MODULES];
int rioOutput1[MAX_DIO_MODULES];
unsigned int CDO32Input[MAX_DIO_MODULES];
unsigned int CDO32Output[MAX_DIO_MODULES];
int clock16K = 0;
double cycle_gps_time = 0.; // Time at which ADCs triggered
double cycle_gps_event_time = 0.; // Time at which ADCs triggered
unsigned int   cycle_gps_ns = 0;
unsigned int   cycle_gps_event_ns = 0;
unsigned int   gps_receiver_unlocked = 1; // Lock/unlock flag for GPS time card

double getGpsTime(unsigned int *);
#include "./feSelectCode.c"

char daqArea[2*DAQ_DCU_SIZE];		/* Space allocation for daqLib buffers	*/


#ifdef OVERSAMPLE

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double feCoeff2x[9] =
        {0.053628649721183,
        -1.25687596603711,    0.57946661417301,    0.00000415782507,    1.00000000000000,
        -0.79382359542546,    0.88797791037820,    1.29081406322442,    1.00000000000000};
/* Coeffs for the 4x downsampling (16K system) filter */
static double feCoeff4x[9] =
	{0.014805052402446,  
	-1.71662585474518,    0.78495484219691,   -1.41346289716898,   0.99893884152400,
	-1.68385964238855,    0.93734519457266,    0.00000127375260,   0.99819981588176};

/* Coeffs for the 32x downsampling filter (2K system) */
static double feCoeff32x[9] =
        {0.0001104130574447,
        -1.97018349613882,    0.97126719875540,   -1.99025960812101,    0.99988962634797,
        -1.98715023886379,    0.99107485707332,    2.00000000000000,    1.00000000000000};


double dHistory[96][40];
double dDacHistory[96][40];

#if 0
#ifdef SERVO2K
#define OVERSAMPLE_TIMES	32
#define FE_OVERSAMPLE_COEFF	feCoeff32x
#elif SERVO16K
#define OVERSAMPLE_TIMES	4
#define FE_OVERSAMPLE_COEFF	feCoeff4x
#elif SERVO32K
#define OVERSAMPLE_TIMES	2
#define FE_OVERSAMPLE_COEFF	feCoeff2x
#else
#error Unsupported system rate when in oversampling mode: only 2K, 16K and 32K are supported
#endif
#endif

#else

#define OVERSAMPLE_TIMES 1
#endif

#define ADC_SAMPLE_COUNT	(0x20 * OVERSAMPLE_TIMES)
#define ADC_DMA_BYTES		(0x80 * OVERSAMPLE_TIMES)

// Whether run on internal timer (when no I/O cards found)
int run_on_timer = 0;

// Initial diag reset flag
int initialDiagReset = 1;

// DIAGNOSTIC_RETURNS_FROM_FE 
#define FE_NO_ERROR		0x0
#define FE_SYNC_ERR		0x1
#define FE_ADC_HOLD_ERR		0x2
#define FE_FB0_NOT_ONLINE	0x4
#define FE_PROC_TIME_ERR	0x8
#define FE_ADC_SYNC_ERR		0xf0
#define FE_FB_AVAIL		0x1
#define FE_FB_ONLINE		0x2
#define FE_MAX_FB_QUE		0x10

#if 0
// **************************************************************************
// Interrupt handler if using interrupts for ADC module
// **************************************************************************
unsigned int intr_handler(unsigned int irq, struct rtl_frame *regs)
{
  unsigned int status;
  unsigned char sid;
        /* pend this IRQ so that the general purpose OS gets it, too */
        rtl_global_pend_irq(irq);


        /*
         * Do any interrupt context handling that must be done right away.
         * This code is not running in a RTLinux thread but in an
         * "interrupt context".
         */

        adcPtr->INTCR = 0x3;



        /*
         * Wake up the thread so it can handle any processing for this irq
         * that can be done in a RTLinux thread and doesn't need to be done in
         * an "interrupt context".
         */
        sem_post( &irqsem );

        /* Reenable the hardware IRQ. (Not the software IRQ the GPOS now has. */
        rtl_hard_enable_irq(irq);
        return 0;
}
#endif


static double getGpsTime1(unsigned int *ns, int event_flag) {
  double the_time = 0.0;

  if (cdsPciModules.gps) {
    // Sample time
    if (event_flag) {
      //cdsPciModules.gps[1] = 1;
    } else {
      cdsPciModules.gps[0] = 1;
    }
    // Read seconds, microseconds, nanoseconds
    unsigned int time0;
    unsigned int time1;
    if (event_flag) {
      time0 = cdsPciModules.gps[SYMCOM_BC635_EVENT0/4];
      time1 = cdsPciModules.gps[SYMCOM_BC635_EVENT1/4];
    } else {
      time0 = cdsPciModules.gps[SYMCOM_BC635_TIME0/4];
      time1 = cdsPciModules.gps[SYMCOM_BC635_TIME1/4];
    }
    gps_receiver_unlocked = !!(time0 & (1<<24));
    unsigned  int msecs = 0xfffff & time0; // microseconds
    unsigned  int nsecs = 0xf & (time0 >> 20); // nsecs * 100
    the_time = ((double) time1) + .000001 * (double) msecs  /* + .0000001 * (double) nsecs*/;
    if (ns) {
	*ns = 100 * nsecs; // Store nanoseconds
    }
  }
  //printf("%f\n", the_time);
  return the_time;
}

// Get current GPS time off the card
// Nanoseconds (hundreds at most) are available as an optional ns parameter
double getGpsTime(unsigned int *ns) {
	return getGpsTime1(ns, 0);
}

// Get current EVENT time off the card
// Nanoseconds (hundreds at most) are available as an optional ns parameter
double getGpsEventTime(unsigned int *ns) {
	return getGpsTime1(ns, 1);
}

// Get Gps card microseconds
unsigned int getGpsUsec() {

  if (cdsPciModules.gps) {
    // Sample time
    cdsPciModules.gps[0] = 1;
    // Read microseconds, nanoseconds
    unsigned int time0 = cdsPciModules.gps[SYMCOM_BC635_TIME0/4];
    unsigned int time1 = cdsPciModules.gps[SYMCOM_BC635_TIME1/4];
    return 0xfffff & time0; // microseconds
  }
  return 0;
}

/************************************************************************/
/* TASK: fe_start()							*/
/* This routine is the skeleton for all front end code			*/
/************************************************************************/
void *fe_start(void *arg)
{

  int ii,jj,kk;
  static int clock1Min = 0;
  static int cpuClock[10];
  int adcData[MAX_ADC_MODULES][32];
  volatile int *packedData;
  volatile unsigned int *pDacData;
  RFM_FE_COMMS *pEpicsComms;
  int cycleTime;
  int timeHold = 0;
  int timeHoldMax = 0;
  int myGmError2 = 0;
  int attemptingReconnect = 0;
  int status;
  float onePps;
  int onePpsHi = 0;
  int dcuId;
  static int adcTime;
  static int adcHoldTime;
  static int usrTime;
  static int usrHoldTime;
  int netRetry;
  float xExc[10];
  static int skipCycle = 0;
  int diagWord = 0;
  int timeDiag = 0;
  int epicsCycle = 0;
  int system = 0;


// Do all of the initalization

  /* Init comms with EPICS processor */
  pEpicsComms = (RFM_FE_COMMS *)_epics_shm;
  pLocalEpics = (CDS_EPICS *)&pEpicsComms->epicsSpace;

#ifdef OVERSAMPLE
  // Zero out filter histories
  memset(dHistory, 0, sizeof(dHistory));
  memset(dDacHistory, 0, sizeof(dDacHistory));
  //printf("Coeff history sizes are %d %d\n", sizeof(dHistory), sizeof(dDacHistory));
#endif

  // Zero out DAC outputs
  for (ii = 0; ii < MAX_DAC_MODULES; ii++)
    for (jj = 0; jj < 16; jj++) {
 	dacOut[ii][jj] = 0.0;
 	dacOutUsed[ii][jj] = 0;
    }

  // Set pointers to SFM data buffers
#if NUM_SYSTEMS > 1
  for (system = 0; system < NUM_SYSTEMS; system++) {
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace[system]);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace[system]);
    dspPtr[system] = dsp + system;
  }
#else
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace);
    dspPtr[system] = dsp;
#endif

  // Clear the FE reset which comes from Epics
  pLocalEpics->epicsInput.vmeReset = 0;

  // Do not proceed until EPICS has had a BURT restore
  unsigned int ns;
  double time = getGpsTime(&ns);
  printf("Waiting for EPICS BURT at %f and %d ns\n", time, ns);
  do{
	usleep(1000000);
  }while(!pLocalEpics->epicsInput.burtRestore);
  for (system = 0; system < NUM_SYSTEMS; system++)
  {
    for(ii=0;ii<END_OF_DAQ_BLOCK;ii++)
    {
	updateEpics(ii, dspPtr[system], pDsp[system],
		    &dspCoeff[system], pCoeff[system]);
    }
  }

  // Need this FE dcuId to make connection to FB
  dcuId = pLocalEpics->epicsInput.dcuId;

  pLocalEpics->epicsOutput.diagWord = 0;
  pLocalEpics->epicsOutput.timeDiag = 0;


#ifdef PNM
  dcuId = 9;
#endif
#ifndef NO_DAQ
  // Make connections to FrameBuilder
  printf("Waiting for Network connect to FB - %d\n",dcuId);
  netRetry = 0;
  status = cdsDaqNetReconnect(dcuId);
  printf("Reconn status = %d %d\n",status,numFb);
  do{
	usleep(10000);
	status = cdsDaqNetCheckReconnect();
	netRetry ++;
  }while((status < numFb) && (netRetry < 10));
  printf("Reconn Check = %d %d\n",status,numFb);

  if(fbStat[0] & 1) fbStat[0] = 3;
  if(fbStat[1] & 1) fbStat[1] = 3;
#endif

  /* Initialize filter banks */
  for (system = 0; system < NUM_SYSTEMS; system++) {
    for(ii=0;ii<MAX_MODULES;ii++){
      for(jj=0;jj<FILTERS;jj++){
        for(kk=0;kk<MAX_COEFFS;kk++){
          dspCoeff[system].coeffs[ii].filtCoeff[jj][kk] = 0.0;
        }
        dspCoeff[system].coeffs[ii].filtSections[jj] = 0;
      }
    }
  }

  /* Initialize all filter module excitation signals to zero */
  for (system = 0; system < NUM_SYSTEMS; system++)
    for(ii=0;ii<MAX_MODULES;ii++)
       dsp[system].data[ii].exciteInput = 0.0;


  /* Initialize DSP filter bank values */
  for (system = 0; system < NUM_SYSTEMS; system++)
    if (initVars(dsp + system, pDsp[system], dspCoeff + system, MAX_MODULES, pCoeff[system])) {
    	printf("Filter module init failed, exiting\n");
	return 0;
    }

  printf("Initialized servo control parameters.\n");




#ifndef NO_DAQ
  /* Set data range limits for daqLib routine */
#ifdef SERVO2K
  daq.filtExMin = 20001;
  daq.filtTpMin = 30001;
#else
  daq.filtExMin = 1;
  daq.filtTpMin = 10001;
#endif
  daq.filtExMax = daq.filtExMin + MAX_MODULES;
  daq.filtExSize = MAX_MODULES;
  daq.xExMin = -1;
  daq.xExMax = -1;
  daq.filtTpMax = daq.filtTpMin + MAX_MODULES * 3;
  daq.filtTpSize = MAX_MODULES * 3;
  daq.xTpMin = daq.filtTpMax;
  daq.xTpMax = daq.xTpMin + 50;

  printf("DAQ Ex Min/Max = %d %d\n",daq.filtExMin,daq.filtExMax);
  printf("DAQ Tp Min/Max = %d %d\n",daq.filtTpMin,daq.filtTpMax);
  printf("DAQ XTp Min/Max = %d %d\n",daq.xTpMin,daq.xTpMax);

  // Set an xtra TP to read out one pps signal
  testpoint[0] = (float *)&onePps;
#endif
  pLocalEpics->epicsOutput.diags[1] = 0;
  pLocalEpics->epicsOutput.diags[2] = 0;

#if defined(OMC_CODE) && !defined(COMPAT_INITIAL_LIGO)
  if (cdsPciModules.pci_rfm[0] != 0) {
	lscRfmPtr = (float *)(cdsPciModules.pci_rfm[0] + 0x3000);
  } else {
	lscRfmPtr = 0;
  }
#endif

#ifndef NO_DAQ
  // Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0,pLocalEpics->epicsOutput.gdsMon,xExc);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    vmeDone = 1;
#if NUM_SYSTEMS > 1
    feDone();
#endif
    return(0);
  }
#endif
	//Read Dio card initial values
     for(kk=0;kk<cdsPciModules.doCount;kk++)
     {
	ii = cdsPciModules.doInstance[kk];
	if(cdsPciModules.doType[kk] == ACS_8DIO)
	{
  	  rioInput[ii] = readIiroDio(&cdsPciModules, kk) & 0xffff;
	}
	if(cdsPciModules.doType[kk] == ACS_16DIO)
	{
  	  rioInput1[ii] = readIiroDio1(&cdsPciModules, kk) & 0xffff;
	}
	if(cdsPciModules.doType[kk] == CON_32DO)
	{
  	  CDO32Input[ii] = readCDO32l(&cdsPciModules, kk);
	}
     }

  // Clear the startup sync counter
  firstTime = 0;

  // Clear the code exit flag
  vmeDone = 0;
	feCode(clock16K,dWord,dacOut,dspPtr[0],&dspCoeff[0],pLocalEpics,1);

  printf("entering the loop\n");

  // Clear a couple of timing diags.
  adcHoldTime = 0;
  usrHoldTime = 0;

  // Initialize the ADC/DAC DMA registers
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
	  status = gsaAdcDma1(jj,cdsPciModules.adcType[jj],ADC_DMA_BYTES);
	  packedData = (int *)cdsPciModules.pci_adc[jj];
	  *packedData = 0x0;
	  if (cdsPciModules.adcType[jj] == GSC_16AISS8AO4
	      || cdsPciModules.adcType[jj] == GSC_18AISS8AO8) { 
		 packedData += 3;
	  } else packedData += 31;
	  *packedData = 0x110000;
  }

  for(jj = 0; jj < cdsPciModules.dacCount; jj++) {
	status = gsaDacDma1(jj, cdsPciModules.dacType[jj]);

        pDacData = (unsigned int *) cdsPciModules.pci_dac[jj];

 	// Preload DAC values
#ifdef DAC_OVER2
        for(ii = 0; ii < (OVERSAMPLE_TIMES * 32); ii++) {
#else
        for(ii = 0; ii < (OVERSAMPLE_TIMES * 16); ii++) {
#endif
 	    *pDacData = 0; // (unsigned int) 10000; // for testing with the scope
	    pDacData ++;
        }

        gsaDacDma2(jj, cdsPciModules.dacType[jj]);
  }

#ifndef NO_SYNC
  if (run_on_timer) {
#endif
   // See if GPS card present
   cycle_gps_time = getGpsTime(&cycle_gps_ns);
   if (cycle_gps_time != 0.0) {
   	unsigned int usec;
	// Enable external event GPS time capture
        cdsPciModules.gps[SYMCOM_BC635_CONTROL/4] = 8;

	// Sleep a second and a half
	usleep(1500000);
	
	// Get the 1PPS time stamp
        cycle_gps_event_time = getGpsEventTime(&cycle_gps_event_ns);
        printf("Initial 1PPS time stamp is %d\n", cycle_gps_event_time);

        usec = 1000000 * (cycle_gps_event_time - (unsigned int) cycle_gps_event_time);
	if (usec > 500000) usec -= 1000000;
	//pLocalEpics->epicsOutput.onePps = usec;

	//while ((usec = getGpsUsec()) < 999978);// Wait until 22us before sec
 	printf("1PPS usec delay is %d\n", usec);
	int delay = 10 + usec;
	int wt = 1000000 - delay;
        if (wt > 1000000) wt = 2000000 - wt;
	printf("Waiting until usec = %d to start the ADCs\n", wt);
	while ((usec = getGpsUsec()) < wt);

        printf("Running time %d us\n", usec);
    } else {
        // Pause until this second ends
        struct timespec next;
        clock_gettime(CLOCK_REALTIME, &next);
        printf("Start time %ld s %ld ns\n", next.tv_sec, next.tv_nsec);
        next.tv_nsec = 0;
        next.tv_sec += 1;
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
        clock_gettime(CLOCK_REALTIME, &next);
        printf("Running time %ld s %ld ns\n", next.tv_sec, next.tv_nsec);
    }
#ifndef NO_SYNC
  }
#endif

  if (!run_on_timer) {
    // Trigger the ADC to start running
    gsaAdcTrigger(cdsPciModules.adcCount,cdsPciModules.adcType);
#ifdef SERVO128K
    gsaDacTrigger(cdsPciModules.dacCount);
#endif
#ifdef SERVO64K
    gsaDacTrigger(cdsPciModules.dacCount);
#endif
#ifdef SERVO32K
    gsaDacTrigger(cdsPciModules.dacCount);
#endif
#ifdef NO_SYNC
    gsaDacTrigger(cdsPciModules.dacCount);
  printf("Triggered the DAC\n");
#endif
  } else {
    printf("*******************************\n");
    printf("* Running with RTLinux timer! *\n");
    printf("*******************************\n");
  }

  printf("Triggered the ADC\n");
#ifdef OMC_CODE
  cdsPciModules.gps = 0;
#endif
  cdsPciModules.gps = 0;

  // Total number of various binary I/O modules
  unsigned int total_bio_boards = cdsPciModules.dioCount
			+ cdsPciModules.iiroDioCount
			+ cdsPciModules.iiroDio1Count
			+ cdsPciModules.cDo32lCount;
  // 0 to total_bio_boards*2 counter
  int cur_bio_card = 0; // Current binary I/O module we read or write

  // Enter the coninuous FE control loop  **********************************************************
  while(!vmeDone){

        rdtscl(cpuClock[2]);
  	if (run_on_timer) {
	  // Pause until next cycle begins
    	  struct timespec next;
    	  clock_gettime(CLOCK_REALTIME, &next);
	  if (clock16K == 0) {
 	    next.tv_nsec = 0;
	    next.tv_sec += 1;
	  } else {
    	    next.tv_nsec = 1000000000 / CYCLE_PER_SECOND * clock16K;
	  }
          clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
	} else {
	  // Wait for data ready from first ADC module.
	  if (cdsPciModules.adcType[0] == GSC_16AISS8AO4
	      || cdsPciModules.adcType[0] == GSC_18AISS8AO8)
	  {
		status = checkAdcRdy(ADC_SAMPLE_COUNT,0);
		if (status == -1) {
			stop_working_threads = 1;
			printf("timeout 0\n");
		}
	  } else {
		// ADC is running in DEMAND DMA MODE
		packedData = (int *)cdsPciModules.pci_adc[0];
		kk = 0;
		do {
			kk ++;
		}while((*packedData == 0) && (kk < 10000000));
		if (kk == 10000000) {
			stop_working_threads = 1;
			printf("timeout 0\n");
		}
	  }

	  usleep(0);
	}

#if 0
	if ((firstTime == 200) && (clock16K == (CYCLE_PER_SECOND - 3)))
	{
	    printf("firstTime=%d clock16K = %d\n", firstTime, clock16K);

	    gsaDacTrigger(cdsPciModules.dacCount);
	    firstTime = 300;
	}
#endif
	// Read CPU clock for timing info
        rdtscl(cpuClock[0]);

        // Get the time from the GPS board
        cycle_gps_time = getGpsTime(&cycle_gps_ns);
        cycle_gps_event_time = getGpsEventTime(&cycle_gps_event_ns);
//   	printf("%f\n", cycle_gps_time - cycle_gps_event_time);
	timeDiag |= gps_receiver_unlocked;
	
  	if (!run_on_timer) {
  	  for(jj=0;jj<cdsPciModules.adcCount;jj++) {
		if(cdsPciModules.adcType[jj] == GSC_16AISS8AO4
		   || cdsPciModules.adcType[jj] == GSC_18AISS8AO8)
		{
			gsaAdcDma2(jj);
		}
	  }
	  // Wait until all ADC channels have been updated
  	  for(jj=0;jj<cdsPciModules.adcCount;jj++) {
		packedData = (int *)cdsPciModules.pci_adc[jj];
		if(cdsPciModules.adcType[jj] == GSC_16AISS8AO4) packedData += 3;
		else if(cdsPciModules.adcType[jj] == GSC_18AISS8AO8) packedData += 3;
		else packedData += 31;
		kk = 0;
		// This seems to work for 18-bit board, not sure this is
		// completely correct
		if (cdsPciModules.adcType[jj] == GSC_18AISS8AO8) {
		   do {
			kk ++;
		   }while(((*packedData & 0x040000) > 0) && (kk < 10000000));
		} else {
		   do {
			kk ++;
		   }while(((*packedData & 0x110000) > 0) && (kk < 10000000));
		}
		if (kk == 10000000) {
			stop_working_threads = 1;
			printf("Adc %d timeout 1\n", jj);
		}
	  }
	}

        // Update internal cycle counters
        if((firstTime != 0) && (!skipCycle))
        {
          clock16K += 1;
          clock16K %= CYCLE_PER_SECOND;
	  clock1Min += 1;
	  clock1Min %= CYCLE_PER_MINUTE;
          if(subcycle == DAQ_CYCLE_CHANGE) daqCycle = (daqCycle + 1) % 16;
          if(subcycle == END_OF_DAQ_BLOCK) /*we have reached the 16Hz second barrier*/
            {
              /* Reset the data cycle counter */
              subcycle = 0;
  
            }
          else{
            /* Increment the internal cycle counter */
            subcycle ++;                                                
          }
        }
        if((subcycle == 0) && (daqCycle == 15))
        {
	  pLocalEpics->epicsOutput.cpuMeter = timeHold;
	  pLocalEpics->epicsOutput.cpuMeterMax = timeHoldMax;
          timeHold = 0;
	  pLocalEpics->epicsOutput.adcWaitTime = adcHoldTime;
	  if(adcHoldTime > CYCLE_TIME_ALRM) diagWord |= FE_ADC_HOLD_ERR;
	  if(timeHoldMax > CYCLE_TIME_ALRM) diagWord |= FE_PROC_TIME_ERR;
  	  if(pLocalEpics->epicsInput.diagReset || initialDiagReset)
	  {
		initialDiagReset = 0;
		pLocalEpics->epicsInput.diagReset = 0;
		adcHoldTime = 0;
		timeHoldMax = 0;
		printf("DIAG RESET\n");
	  }
	  pLocalEpics->epicsOutput.diagWord = diagWord;
	  diagWord = 0;
	  pLocalEpics->epicsOutput.timeDiag = timeDiag;
	  timeDiag = 0;
        }

	/* Update Epics variables */
#if MAX_MODULES > END_OF_DAQ_BLOCK
	epicsCycle = (epicsCycle + 1) % (MAX_MODULES + 2);
#else
	epicsCycle = subcycle;
#endif
  	for (system = 0; system < NUM_SYSTEMS; system++)
		updateEpics(epicsCycle, dspPtr[system], pDsp[system],
			    &dspCoeff[system], pCoeff[system]);

        vmeDone = stop_working_threads | checkEpicsReset(epicsCycle, pLocalEpics);
	// usleep(1);
  	if (!run_on_timer) {
	  // Wait for completion of DMA of Adc data
  	  for(kk=0;kk<cdsPciModules.adcCount;kk++) {
		// Read adc data into local variables
		packedData = (int *)cdsPciModules.pci_adc[kk];
		// Return 0x10 if first ADC channel does not have sync bit set
		if(*packedData & 0xf0000) status = 0;
		else status = 16;
		jj = kk + 1;
		diagWord |= status * jj;

		int limit = 32000;
		int offset = 0x8000;
		int mask = 0xffff;
		int num_outs = 32;
		if (cdsPciModules.adcType[kk] == GSC_18AISS8AO8) {
			limit *= 4; // 18 bit limit
			offset = 0x20000; // Data coding offset in 18-bit DAC
			mask = 0x3ffff;
			num_outs = 8;
		}
		if (cdsPciModules.adcType[kk] == GSC_16AISS8AO4) {
			num_outs = 4;
		}
#ifdef OVERSAMPLE
		for (jj=0; jj < OVERSAMPLE_TIMES; jj++)
		{
			for(ii=0;ii<num_outs;ii++)
			{
				adcData[kk][ii] = (*packedData & mask);
				adcData[kk][ii]  -= offset;
				if (ii == 31) {
				   if (jj == 0 || dWord[kk][31] < adcData[kk][31])
					dWord[kk][31] = adcData[kk][31];
				} else {
				  if (dWordUsed[kk][ii]) {
				  	dWord[kk][ii] = iir_filter((double)adcData[kk][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+kk*32][0]);
				  }
				}
				packedData ++;
			}
		}
		//dWord[kk][31] = adcData[kk][31];
#else
		for(ii=0;ii<num_outs;ii++)
		{
			adcData[kk][ii] = (*packedData & mask);
			adcData[kk][ii] -= offset;
                        packedData ++;
			dWord[kk][ii] = adcData[kk][ii];
		}
#endif
		for(ii=0;ii<32;ii++)
		{
			if((adcData[kk][ii] > limit) || (adcData[kk][ii] < -limit))
			  {
				overflowAdc[kk][ii] ++;
				// pLocalEpics->epicsOutput.ovAccum ++;
				overflowAcc ++;
				diagWord |= 0x100 *  jj;
			  }
		}
		// Clear out last ADC data read for test on next cycle
        	packedData = (int *)cdsPciModules.pci_adc[kk];
  		*packedData = 0x0;
		if(cdsPciModules.adcType[kk] == GSC_16AISS8AO4) packedData += 3;
		else if(cdsPciModules.adcType[kk] == GSC_18AISS8AO8) packedData += 3;
  		else packedData += 31;
  		*packedData = 0x110000;
	  }
	}

	// Assign chan 32 to onePps 
	onePps = dWord[0][31];

	// For startup sync to 1pps, loop here
	if(firstTime == 0)
	{
		if(onePps > ONE_PPS_THRESH) 
		 {
			firstTime = 100;
#ifdef SERVO64K
			firstTime = 200;
#endif
			onePpsHi = 0;
		 }
#ifdef NO_SYNC
		firstTime = 200;
			onePpsHi = 0;
#endif
		/* Do not do 1PPS sync when running on timer */
		if (run_on_timer) {
			firstTime = 200;
			onePpsHi = 0;
		}
	}

	if((onePps > ONE_PPS_THRESH) && (onePpsHi == 0))  
	{
#ifdef NO_SYNC
	        unsigned int usec = 1000000 * (cycle_gps_event_time - (unsigned int) cycle_gps_event_time);
		if (usec > 500000) usec -= 1000000;
		pLocalEpics->epicsOutput.onePps = usec;
#else
		pLocalEpics->epicsOutput.onePps = clock16K;
#endif
		onePpsHi = 1;
	}
	if(onePps < ONE_PPS_THRESH) onePpsHi = 0;  

	// Display sample 0 GPS microseconds
	if (clock16K == 0) {
		unsigned int nsec = 1000000000.0 * (cycle_gps_time - (unsigned int) cycle_gps_time);
		if (nsec > 500000000) nsec -= 1000000000;
		pLocalEpics->epicsOutput.timeErr = nsec;
	}

	// Check if front end continues to be in sync with 1pps
	// If not, set sync error flag
	if(pLocalEpics->epicsOutput.onePps > 20) diagWord |= FE_SYNC_ERR;

	if(!skipCycle)
 	{
	// Call the front end specific software
        rdtscl(cpuClock[4]);


#if (NUM_SYSTEMS > 1) && !defined(PNM)
  	for (system = 0; system < 1; system++)
	  feCode(clock16K,dWord,dacOut,dspPtr[system],dspCoeff + system,pLocalEpics, system);
#else
	feCode(clock16K,dWord,dacOut,dspPtr[0],&dspCoeff[0],pLocalEpics,0);
#endif
        rdtscl(cpuClock[5]);
  	// pLocalEpics->epicsOutput.diags[1]  = readDio(&cdsPciModules,0);

#if defined(OMC_CODE) && !defined(COMPAT_INITIAL_LIGO)
	if (lscRfmPtr != 0) {
		*lscRfmPtr = dspPtr[0]->data[LSC_DRIVE].output;
	}
#endif

	// Write out data to DAC modules
	if((firstTime >= 200))
	{
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
	{
	   // Check Dac output overflow and write to DMA buffer
	   pDacData = (unsigned int *)cdsPciModules.pci_dac[jj];
#ifdef OVERSAMPLE_DAC
#ifdef DAC_OVER2
	   for (kk=0; kk < (OVERSAMPLE_TIMES*2); kk++) {
#else
	   for (kk=0; kk < OVERSAMPLE_TIMES; kk++) {
#endif
#endif
		int limit = 32000;
		int offset = 0; //0x8000;
		int mask = 0xffff;
		int num_outs = 16;
		if (cdsPciModules.dacType[jj] == GSC_18AISS8AO8) {
			limit *= 4; // 18 bit limit
			offset = 0x20000; // Data coding offset in 18-bit DAC
			mask = 0x3ffff;
			num_outs = 8;
		}
		if (cdsPciModules.dacType[jj] == GSC_16AISS8AO4) {
		  	num_outs = 4;
		}
		for (ii=0; ii < num_outs; ii++)
		{
#ifdef OVERSAMPLE_DAC
			if (dacOutUsed[jj][ii]) {
			  double dac_in =  kk == 0? (double)dacOut[jj][ii]: 0.0;
#ifdef DAC_OVER2
		 	  dacOut[jj][ii] = iir_filter(dac_in,&feCoeff4x,2,&dDacHistory[ii+jj*16][0]);
			  dacOut[jj][ii] *= (OVERSAMPLE_TIMES * 2);
#else
		 	  dacOut[jj][ii] = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*16][0]);
			  dacOut[jj][ii] *= OVERSAMPLE_TIMES;
#endif
			}
#endif
			if(dacOut[jj][ii] > limit) 
			{
				dacOut[jj][ii] = limit;
				overflowDac[jj][ii] ++;
				// pLocalEpics->epicsOutput.ovAccum ++;
				overflowAcc ++;
				diagWord |= 0x1000 *  (jj+1);
			}
			if(dacOut[jj][ii] < -limit) 
			{
				dacOut[jj][ii] = -limit;
				overflowDac[jj][ii] ++;
				// pLocalEpics->epicsOutput.ovAccum ++;
				overflowAcc ++;
				diagWord |= 0x1000 *  (jj+1);
			}
			  int dac_out = dacOut[jj][ii];
			  dac_out += offset;
			//if (ii == 0) printf("%d\n", dac_out);
			  *pDacData =  (unsigned int)(dac_out & mask);
			  pDacData ++;
		}
#ifdef OVERSAMPLE_DAC
  	   }
#endif
  	   // DMA out dac values
	   gsaDacDma2(jj,cdsPciModules.dacType[jj]);
	}
	}

	// Write/Read Dio cards
	for(kk=0;kk<cdsPciModules.doCount;kk++)
	{
		ii = cdsPciModules.doInstance[kk];
		if(cdsPciModules.doType[kk] == ACS_8DIO)
		{
			if (rioInput[ii] != rioOutput[ii]) {
			  writeIiroDio(&cdsPciModules, kk, rioOutput[ii]);
			  rioInput[ii] = readIiroDio(&cdsPciModules, kk) & 0xff;
			}
		}
		if(cdsPciModules.doType[kk] == ACS_16DIO)
		{
			if (rioInput1[ii] != rioOutput1[ii]) {
			  writeIiroDio1(&cdsPciModules, kk, rioOutput1[ii]);
			  rioInput1[ii] = readIiroDio1(&cdsPciModules, kk) & 0xffff;
			}
		}
		if(cdsPciModules.doType[kk] == CON_32DO)
		{
			if (CDO32Input[ii] != CDO32Output[ii]) {
			  CDO32Input[ii] = writeCDO32l(&cdsPciModules, kk, CDO32Output[ii]);
			}
		}
	}

#ifndef NO_DAQ
	// Write DAQ and GDS values once we are synched to 1pps
  	if(firstTime != 0) 
	{
		// Call daqLib
		pLocalEpics->epicsOutput.diags[3] = 
			daqWrite(1,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],myGmError2,pLocalEpics->epicsOutput.gdsMon,xExc);
		// Check if any messages received and get xmit que count.
		status = cdsDaqNetCheckCallback();
		// If callbacks pending count is high, FB must not be responding.
		// Check FB0
		if(((status & 0xff) > FE_MAX_FB_QUE) && (fbStat[0] & FE_FB_ONLINE))
		{
			printf("Net fail to fb0 \n");
			// Set error flag
			attemptingReconnect = 1;
			// Send FB status to on net, but not online.
			fbStat[0] = 0x1;
		}
		// Check FB1
		if((((status >> 8) & 0xff) > FE_MAX_FB_QUE) && (fbStat[1] & FE_FB_ONLINE))
		{
			printf("Net fail to fb1 - recon try\n");
			// Set error flag
			attemptingReconnect = 1;
			// Send FB status to on net, but not online.
			fbStat[1] = 0x1;
		}

		// Check if any FB have returned to net as indicated by reduced send que count
		if((attemptingReconnect) && (clock16K == 1000)) 
		{
			// Check FB0
			if(((status & 0xff) < FE_MAX_FB_QUE) && (fbStat[0] == FE_FB_AVAIL))
			{
				// Set fbStat to include recon bit needed by myri.c code
				fbStat[0] = 5;
				// Send recon message to FB
				status = cdsDaqNetReconnect(dcuId);
				// printf("Send recon command fb0\n");
			}
			// Check FB1
			if((((status >> 8) & 0xff) < FE_MAX_FB_QUE) && (fbStat[1] == FE_FB_AVAIL))
			{
				// Set fbStat to include recon bit needed by myri.c code
				fbStat[1] = 5;
				// Send recon message to FB
				status = cdsDaqNetReconnect(dcuId);
				// printf("Send recon command fb1\n");
			}
		}
		// Once per second, check if FB are back on line
		if((attemptingReconnect) && (clock16K == 0)) 
		{
			// Clear error flag
			attemptingReconnect = 0;
			if((fbStat[0] & FE_FB_AVAIL) && ((fbStat[0] & FE_FB_ONLINE) == 0))
				// Set error flag
				attemptingReconnect = 1;
			if((fbStat[1] & FE_FB_AVAIL) && ((fbStat[1] & FE_FB_ONLINE) == 0))
				// Set error flag
				attemptingReconnect = 1;
		}
	}
#endif
	}

	skipCycle = 0;
	usleep(1);
	// Reset all ADC DMA GO bits
  	for(jj=0;jj<cdsPciModules.adcCount;jj++)
  	{
                if(cdsPciModules.adcType[jj] == GSC_16AI64SSA)
                {
  			gsaAdcDma2(jj);
		}
	}

	// Measure time to complete 1 cycle
        rdtscl(cpuClock[1]);

	// Compute max time of one cycle.
	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
	//if (clock16K < 2) printf("cycle %d time %d\n", clock16K, cycleTime);
#if defined(SERVO16K) || defined(SERVO2K)
	if ((firstTime == 100) && (clock16K == (CYCLE_PER_SECOND - 2)))
	{
	    printf("DT firstTime=%d clock16K = %d\n", firstTime, clock16K);
	    if((DAC_START_DELAY-cycleTime) > 0)
	    	usleep((DAC_START_DELAY-cycleTime));
	    gsaDacTrigger(cdsPciModules.dacCount);
	    firstTime = 200;
	}
#endif
	// Hold the max cycle time over the last 1 second
	if(cycleTime > timeHold) timeHold = cycleTime;
	// Hold the max cycle time since last diag reset
	if(cycleTime > timeHoldMax) timeHoldMax = cycleTime;
	adcTime = (cpuClock[0] - cpuClock[2])/CPURATE;
	if(adcTime > adcHoldTime) adcHoldTime = adcTime;
	// Calc the max time of one cycle of the user code
	usrTime = (cpuClock[5] - cpuClock[4])/CPURATE;
	if(usrTime > usrHoldTime) usrHoldTime = usrTime;

        if((subcycle == 0) && (daqCycle == 14))
        {
	  pLocalEpics->epicsOutput.diags[0] = usrHoldTime;
	  // Create FB status word for return to EPICS
#ifdef USE_GM
  	  pLocalEpics->epicsOutput.diags[2] = (fbStat[1] & 3) * 4 + (fbStat[0] & 3);
#else
	  // There is no frame builder to front-end feedback at the moment when
	  // not using GM (using shmem), perhaps it needs to be added
  	  pLocalEpics->epicsOutput.diags[2] = 3;
#endif
	  usrHoldTime = 0;
  	  if(pLocalEpics->epicsInput.syncReset)
	  {
		pLocalEpics->epicsInput.syncReset = 0;
		skipCycle = 1;
	  }
  	  if((pLocalEpics->epicsInput.overflowReset) || (overflowAcc > 0x1000000))
	  {

#ifdef ROLLING_OVERFLOWS
                if (pLocalEpics->epicsInput.overflowReset) {
                   for (ii = 0; ii < 16; ii++) {
                      for (jj = 0; jj < cdsPciModules.adcCount; jj++) {
                         overflowAdc[jj][ii] = 0;
                         overflowAdc[jj][ii + 16] = 0;
                      }

                      for (jj = 0; jj < cdsPciModules.dacCount; jj++) {
                         overflowDac[jj][ii] = 0;
                      }
                   }
                }
#endif

		pLocalEpics->epicsInput.overflowReset = 0;
		pLocalEpics->epicsOutput.ovAccum = 0;
		overflowAcc = 0;
	  }
        }
        if(clock16K == 200)
        {
		pLocalEpics->epicsOutput.ovAccum = overflowAcc;
	  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  {
	    for(ii=0;ii<32;ii++)
	    {
		pLocalEpics->epicsOutput.overflowAdc[jj][ii] = overflowAdc[jj][ii];

#ifdef ROLLING_OVERFLOWS
                if (overflowAdc[jj][ii] > 0x1000000) {
		   overflowAdc[jj][ii] = 0;
                }
#else
		overflowAdc[jj][ii] = 0;
#endif

	    }
	  }
	  for(jj=0;jj<cdsPciModules.dacCount;jj++)
	  {
	    for(ii=0;ii<16;ii++)
	    {
		pLocalEpics->epicsOutput.overflowDac[jj][ii] = overflowDac[jj][ii];

#ifdef ROLLING_OVERFLOWS
                if (overflowDac[jj][ii] > 0x1000000) {
		   overflowDac[jj][ii] = 0;
                }
#else
		overflowDac[jj][ii] = 0;
#endif

	    }
	  }
        }

  }

#if (NUM_SYSTEMS > 1) && !defined(PNM)
  feDone();
#endif

  /* System reset command received */
  return (void *)-1;
}

int main(int argc, char **argv)
{
        pthread_attr_t attr;
 	int status;
	int ii,jj;
	char fname[128];

	jj = 0;
	printf("cpu clock %ld\n",cpu_khz);

        /*
         * Create the shared memory area.  By passing a non-zero value
         * for the mode, this means we also create a node in the GPOS.
         */       

	/* See if we can open new-style shared memory file */
	sprintf(fname, "/rtl_mem_%s", SYSTEM_NAME_STRING_LOWER);
        wfd = shm_open(fname, RTL_O_RDWR, 0666);
	if (wfd == -1) {
          //printf("Warning, couldn't open `%s' read/write (errno=%d)\n", fname, errno);
          wfd = shm_open("/rtl_epics", RTL_O_RDWR, 0666);
          if (wfd == -1) {
                printf("open failed for write on /rtl_epics (%d)\n",errno);
                rtl_perror("shm_open()");
                return -1;
          }
	}

        _epics_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,wfd,0);
        if (_epics_shm == MAP_FAILED) {
                printf("mmap failed for epics shared memory area\n");
                rtl_perror("mmap()");
                return -1;
        }

	// See if IPC area is available, open and map it
	strcpy(fname, "/rtl_mem_ipc");
        ipc_fd = shm_open(fname, RTL_O_RDWR, 0666);
	if (ipc_fd == -1) {
          //printf("Warning, couldn't open `%s' read/write (errno=%d)\n", fname, errno);
	} else {
          _ipc_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,ipc_fd,0);
          if (_ipc_shm == MAP_FAILED) {
                printf("mmap failed for IPC shared memory area\n");
                rtl_perror("mmap()");
                return -1;
	  }
        }

#if defined(SHMEM_DAQ)
	// See if frame builder DAQ comm area available
	sprintf(fname, "/rtl_mem_%s_daq", SYSTEM_NAME_STRING_LOWER);
        daq_fd = shm_open(fname, RTL_O_RDWR, 0666);
	if (daq_fd == -1) {
          printf("Error: couldn't open `%s' read/write (errno=%d)\n", fname, errno);
	  return -1;
	} else {
          _daq_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,daq_fd,0);
          if (_daq_shm == MAP_FAILED) {
                printf("mmap failed for DAQ shared memory area\n");
                rtl_perror("mmap()");
                return -1;
	  }
        }
#endif

	{
	  int cards = sizeof(cards_used)/sizeof(cards_used[0]);

	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	}
	printf("Initializing PCI Modules\n");
	status = mapPciModules(&cdsPciModules);
	printf("%d PCI cards found\n",status);
#ifdef ONE_ADC
	cdsPciModules.adcCount = cdsPciModules.dacCount = 1;
#endif
#ifdef DAC_COUNT
	cdsPciModules.dacCount = DAC_COUNT;
#endif
        printf("***************************************************************************\n");
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	for(ii=0;ii<cdsPciModules.adcCount;ii++)
        {
                if(cdsPciModules.adcType[ii] == GSC_18AISS8AO8)
                {
                        printf("\tADC %d is a GSC_18AISS8AO8 module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 4;
                        else jj = 8;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
                if(cdsPciModules.adcType[ii] == GSC_16AISS8AO4)
                {
                        printf("\tADC %d is a GSC_16AISS8AO4 module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 4;
                        else jj = 8;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
                if(cdsPciModules.adcType[ii] == GSC_16AI64SSA)
                {
                        printf("\tADC %d is a GSC_16AI64SSA module\n",ii);
                        if((cdsPciModules.adcConfig[ii] & 0x10000) > 0) jj = 32;
                        else jj = 64;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.adcConfig[ii] & 0xfff));
                }
        }
        printf("***************************************************************************\n");
	printf("%d DAC cards found\n",cdsPciModules.dacCount);
	for(ii=0;ii<cdsPciModules.dacCount;ii++)
        {
                if(cdsPciModules.dacType[ii] == GSC_18AISS8AO8)
                {
                        printf("\tDAC %d is a GSC_18AISS8AO8 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x20000) > 0) jj = 0;
                        else jj = 4;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
                if(cdsPciModules.dacType[ii] == GSC_16AISS8AO4)
                {
                        printf("\tDAC %d is a GSC_16AISS8AO4 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x20000) > 0) jj = 0;
                        else jj = 4;
                        printf("\t\tChannels = %d \n",jj);
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
                if(cdsPciModules.dacType[ii] == GSC_16AO16)
                {
                        printf("\tDAC %d is a GSC_16AO16 module\n",ii);
                        if((cdsPciModules.dacConfig[ii] & 0x10000) == 0x10000) jj = 8;
                        if((cdsPciModules.dacConfig[ii] & 0x20000) == 0x20000) jj = 12;
                        if((cdsPciModules.dacConfig[ii] & 0x30000) == 0x30000) jj = 16;
                        printf("\t\tChannels = %d \n",jj);
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x0000)
			{
                        	printf("\t\tFilters = None\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x40000)
			{
                        	printf("\t\tFilters = 10kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0xC0000) == 0x80000)
			{
                        	printf("\t\tFilters = 100kHz\n");
			}
                        if((cdsPciModules.dacConfig[ii] & 0x100000) == 0x100000)
			{
                        	printf("\t\tOutput Type = Differential\n");
			}
                        printf("\t\tFirmware Rev = %d \n\n",(cdsPciModules.dacConfig[ii] & 0xfff));
                }
	}
        printf("***************************************************************************\n");
	printf("%d DIO cards found\n",cdsPciModules.dioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-8 Isolated DIO cards found\n",cdsPciModules.iiroDioCount);
        printf("***************************************************************************\n");
	printf("%d IIRO-16 Isolated DIO cards found\n",cdsPciModules.iiroDio1Count);
        printf("***************************************************************************\n");
	printf("%d Contec 32ch PCIe DO cards found\n",cdsPciModules.cDo32lCount);
	printf("%d DO cards found\n",cdsPciModules.doCount);
        printf("***************************************************************************\n");
	printf("%d RFM cards found\n",cdsPciModules.rfmCount);
	for(ii=0;ii<cdsPciModules.rfmCount;ii++)
        {
                 printf("\tRFM %d is a VMIC_%x module with Node ID %d\n", ii, cdsPciModules.rfmType[ii], cdsPciModules.rfmConfig[ii]);
	}
        printf("***************************************************************************\n");

	//cdsPciModules.adcCount = 0;
	//cdsPciModules.dacCount = 0;

	if (cdsPciModules.adcCount == 0 && cdsPciModules.dacCount == 0) {
		printf("No ADC and no DAC modules found, running on timer\n");
		run_on_timer = 1;
        	//munmap(_epics_shm, MMAP_SIZE);
        	//close(wfd);
        	//return 0;
	}
#ifdef ONE_ADC
	cdsPciModules.adcCount = 1;
#endif

	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
#ifndef NO_DAQ
	printf("Initializing Network\n");
	numFb = cdsDaqNetInit(2);
	if (numFb <= 0) {
		printf("Couldn't initialize Myrinet network connection\n");
		return 0;
	}
	printf("Found %d frameBuilders on network\n",numFb);
#endif
#if 0
        /* initialize the semaphore */
        sem_init (&irqsem, 1, 0);
#endif



        rtl_pthread_attr_init(&attr);
#ifdef RESERVE_CPU2
        rtl_pthread_attr_setcpu_np(&attr, 2);
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread1, &attr, cpu2_start, 0);
	printf("Started cpu2 task\n");
	usleep(1000000);
#endif

#ifdef RESERVE_CPU3
        rtl_pthread_attr_setcpu_np(&attr, 3);
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread2, &attr, cpu3_start, 0);
	printf("Started cpu3 task\n");
	usleep(1000000);
#endif

#ifdef SPECIFIC_CPU
        rtl_pthread_attr_setcpu_np(&attr, SPECIFIC_CPU);
#else
        rtl_pthread_attr_setcpu_np(&attr, 1);
#endif
        /* mark this CPU as reserved - only RTLinux runs on it */
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread, &attr, fe_start, 0);


#if 0
        /* install a handler for this IRQ */
        if ( (ret = rtl_request_irq( 24, intr_handler )) != 0 ) {
                printf("failed to get irq %d\n", 40);
                ret = -1;
                goto out;
        }
#endif


        /* wait for us to be removed or killed */
        rtl_main_wait();

	        /* free this IRQ */
#if 0
        rtl_free_irq(40);
out:
#endif

#ifndef NO_DAQ
	status = cdsDaqNetClose();
#endif

	printf("Killing work threads\n");
	stop_working_threads = 1;

        /* kill the threads */
        rtl_pthread_cancel(wthread);
        rtl_pthread_join(wthread, NULL);

#ifdef RESERVE_CPU2
        rtl_pthread_cancel(wthread1);
        rtl_pthread_join(wthread1, NULL);
#endif
#ifdef RESERVE_CPU3
        rtl_pthread_cancel(wthread2);
        rtl_pthread_join(wthread2, NULL);
#endif

        /* unmap the shared memory areas */
        munmap(_epics_shm, MMAP_SIZE);

        /* Note that this is a shared area created with shm_open() - we close
         * it with close(), but use shm_unlink() to actually destroy the area
         */
        close(wfd);

        return 0;
}

