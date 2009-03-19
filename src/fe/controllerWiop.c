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
#ifdef USE_IO_DRIVER
CDS_HARDWARE *cdsPciMod;	/* Structure of hardware addresses		*/
#endif
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
int clock64K = 0;
double cycle_gps_time = 0.; // Time at which ADCs triggered
double cycle_gps_event_time = 0.; // Time at which ADCs triggered
unsigned int   cycle_gps_ns = 0;
unsigned int   cycle_gps_event_ns = 0;
unsigned int   gps_receiver_unlocked = 1; // Lock/unlock flag for GPS time card

double getGpsTime(unsigned int *);
#include "./feSelectCode.c"

char daqArea[2*DAQ_DCU_SIZE];		/* Space allocation for daqLib buffers	*/



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

#if 0
static double feCoeff32x[13] =
	{0.010175080706269,
        -1.85347342989536,    0.86652823013428,   -1.90194120908885,    1.00000000000000,
        -1.92214573995746,    0.95974748650870,   -1.56965924485826,    1.00000000000000,
        -1.92395145633000,    0.96164389369609,   -1.96035842905592,    0.99876412118456};
#endif


double dHistory[96][40];
double dDacHistory[96][40];



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

#ifdef USE_IRIG_B
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
#endif

/************************************************************************/
/* TASK: fe_start()							*/
/* This routine is the skeleton for all front end code			*/
/************************************************************************/
void *fe_start(void *arg)
{

  int ii,jj,kk,ll,mm;
  int ioClock = 0;
  int ioMemCntr = 0;
  int ioMemCntrDac = 0;
  static int clock1Min = 0;
  static int cpuClock[10];
  volatile IO_MEM_DATA *ioMemData;
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
  double dac_in;
  int dac_out;
  int sampleCount = 1;


// Do all of the initalization

  /* Init comms with EPICS processor */
  pEpicsComms = (RFM_FE_COMMS *)_epics_shm;
  pLocalEpics = (CDS_EPICS *)&pEpicsComms->epicsSpace;
  ioMemData = (IO_MEM_DATA *)(_ipc_shm+ 0x3000);

  // Zero out filter histories
  memset(dHistory, 0, sizeof(dHistory));
  memset(dDacHistory, 0, sizeof(dDacHistory));
  //printf("Coeff history sizes are %d %d\n", sizeof(dHistory), sizeof(dDacHistory));

  // Zero out DAC outputs
  for (ii = 0; ii < MAX_DAC_MODULES; ii++)
    for (jj = 0; jj < 16; jj++) {
 	dacOut[ii][jj] = 0.0;
 	dacOutUsed[ii][jj] = 0;
    }

  // Set pointers to SFM data buffers
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace);
    dspPtr[system] = dsp;

  // Clear the FE reset which comes from Epics
  pLocalEpics->epicsInput.vmeReset = 0;

  // Do not proceed until EPICS has had a BURT restore
#ifndef USE_IO_DRIVER
  unsigned int ns;
  double time = getGpsTime(&ns);
  printf("Waiting for EPICS BURT at %f and %d ns\n", time, ns);
#endif
  printf("Waiting for EPICS BURT \n");
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
	  rioInput[ii] = ioMemData->digOut[ii] & 0xff;
	}
	if(cdsPciModules.doType[kk] == ACS_16DIO)
	{
	  rioInput1[ii] = ioMemData->digOut[ii] & 0xff;
	}
	if(cdsPciModules.doType[kk] == CON_32DO)
	{
	  CDO32Input[ii] = ioMemData->digOut[ii] & 0xff;
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
  ioMemCntrDac = 0;
  // ioMemCntrDac = (OVERSAMPLE_TIMES * 2) % 64;
  // clock64K += OVERSAMPLE_TIMES*2;
  ioMemCntrDac = 8;
  clock64K = 8;
printf ("oversmaple = %d\n",OVERSAMPLE_TIMES);


#ifndef NO_SYNC
  if (run_on_timer) {
#endif
	printf("waiting to sync %d\n",ioMemData->adcCycle[0][0]);
        rdtscl(cpuClock[0]);
	do{
		usleep(1);
	}while(ioMemData->adcCycle[0][0] != 0);
        rdtscl(cpuClock[1]);
	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
	printf("Synched %d\n",cycleTime);
	// return();

  // Enter the coninuous FE control loop  **********************************************************
  while(!vmeDone){

        rdtscl(cpuClock[2]);
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
  	for(ii=0;ii<sampleCount;ii++) {
		for(kk=0;kk<cdsPciModules.adcCount;kk++) {
			jj = 0;
			do{
				usleep(1);
				jj++;
			}while((ioMemData->adcCycle[kk][ioMemCntr] != ioClock) && (jj < 1000));
			if(jj >= 1000)
			{
				printf ("ADC TIMEOUT %d %d %d %d\n",kk,ioMemData->adcCycle[kk][ioMemCntr], ioMemCntr,ioClock);
printf("got here %d %d %d\n",clock16K,clock64K,ioClock);
				return(-1);
			}
			for(ll=0;ll<32;ll++)
			{
				adcData[kk][ll] = ioMemData->adcVal[kk][ioMemCntr][ll];
				dWord[kk][ll] = iir_filter((double)adcData[kk][ll],FE_OVERSAMPLE_COEFF,2,&dHistory[ll+kk*32][0]);
				// No filter  dWord[kk][ll] = adcData[kk][ll];
			}
		}
		ioClock = (ioClock + 1) % 65536;
		ioMemCntr = (ioMemCntr + 1) % 64;
	}
        rdtscl(cpuClock[0]);

	// Assign chan 32 to onePps 
	onePps = dWord[0][31];
	firstTime = 200;

	// Call the front end specific software
        rdtscl(cpuClock[4]);


	feCode(clock16K,dWord,dacOut,dspPtr[0],&dspCoeff[0],pLocalEpics,0);
        rdtscl(cpuClock[5]);
  	// pLocalEpics->epicsOutput.diags[1]  = readDio(&cdsPciModules,0);

#if defined(OMC_CODE) && !defined(COMPAT_INITIAL_LIGO)
	if (lscRfmPtr != 0) {
		*lscRfmPtr = dspPtr[0]->data[LSC_DRIVE].output;
	}
#endif

	// Write out data to DAC modules
		int limit = 32000;
		int dacNum = 0; //0x8000;
		int mask = 0xffff;
		int num_outs = 16;
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
	{
		dacNum = cdsPciModules.pci_dac[jj];
		ll = ioMemCntrDac;
		mm = clock64K;
	   for (kk=0; kk < OVERSAMPLE_TIMES; kk++) {
		for (ii=0; ii < num_outs; ii++)
		{
			if (dacOutUsed[jj][ii]) {
			  	// dac_in =  kk == 0? (double)dacOut[jj][ii]: 0.0;
			  	dac_in =  (double)dacOut[jj][ii];
		 	  	dac_out = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*16][0]);
			  	// dac_out *= OVERSAMPLE_TIMES;
			  	// No filter dac_out = dac_in;
				if(dac_out > limit) 
				{
					dac_out = limit;
					overflowDac[jj][ii] ++;
					pLocalEpics->epicsOutput.ovAccum ++;
					overflowAcc ++;
					diagWord |= 0x1000 *  (jj+1);
				}
				if(dac_out < -limit) 
				{
					dac_out = -limit;
					overflowDac[jj][ii] ++;
					pLocalEpics->epicsOutput.ovAccum ++;
					overflowAcc ++;
					diagWord |= 0x1000 *  (jj+1);
				}
			}else{
				dac_out = 0;
			}
			ioMemData->dacVal[dacNum][ll][ii] = dac_out; 
		}
		ioMemData->dacCycle[dacNum][ll] = mm; 
		ll = (ll + 1) % 64;
		mm = (mm + 1) % 65536;
	   }
	}
   ioMemCntrDac = (ioMemCntrDac + OVERSAMPLE_TIMES) % 64;
   clock64K = (clock64K + OVERSAMPLE_TIMES) % 65536;
   sampleCount = OVERSAMPLE_TIMES;

	// Write/Read Dio cards
	for(kk=0;kk<cdsPciModules.doCount;kk++)
	{
		ii = cdsPciModules.doInstance[kk];
		if(cdsPciModules.doType[kk] == ACS_8DIO)
		{
// printf("write ACS %d\n",ii);
			if (rioInput[ii] != rioOutput[ii]) {
			  ioMemData->digOut[ii] = rioOutput[ii];
			  rioInput[ii] = ioMemData->digOut[ii] & 0xff;
			}
		}
		if(cdsPciModules.doType[kk] == ACS_16DIO)
		{
			if (rioInput1[ii] != rioOutput1[ii]) {
			  ioMemData->digOut[ii] = rioOutput1[ii];
			  rioInput1[ii] = ioMemData->digOut[ii] & 0xffff;
			}
		}
		if(cdsPciModules.doType[kk] == CON_32DO)
		{
printf("write CON\n");
			if (CDO32Input[ii] != CDO32Output[ii]) {
			  ioMemData->digOut[ii] = CDO32Output[ii];
			  CDO32Input[ii] = ioMemData->digOut[ii];
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


	// Measure time to complete 1 cycle
        rdtscl(cpuClock[1]);

	// Compute max time of one cycle.
	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
	//if (clock16K < 2) printf("cycle %d time %d\n", clock16K, cycleTime);
	// Hold the max cycle time over the last 1 second
	if(cycleTime > timeHold) timeHold = cycleTime;
	// Hold the max cycle time since last diag reset
	if(cycleTime > timeHoldMax) timeHoldMax = cycleTime;
	adcTime = (cpuClock[0] - cpuClock[2])/CPURATE;
	// if(adcTime > adcHoldTime) adcHoldTime = adcTime;
	adcHoldTime = adcTime;
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
	int ii,jj,kk,ll;
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

	  int cards = sizeof(cards_used)/sizeof(cards_used[0]);

	  printf("configured to use %d cards\n", cards);
	  cdsPciModules.cards = cards;
	  cdsPciModules.cards_used = cards_used;
          //return -1;
	printf("Initializing PCI Modules\n");
	cdsPciMod = (CDS_HARDWARE *)_ipc_shm;
	status = 0;
				cdsPciModules.doCount = 0;
	for(ii=0;ii<cards;ii++)
	{
		for(jj=0;jj<cdsPciMod->adcCount;jj++)
		{
			if((cards_used[ii].type == cdsPciMod->adcType[jj]) && (cards_used[ii].instance == jj))
			{
				kk = cdsPciModules.adcCount;
				cdsPciModules.pci_adc[kk] = jj;
				cdsPciModules.adcType[kk] = cdsPciMod->adcType[jj];
				cdsPciModules.adcConfig[kk] = cdsPciMod->adcConfig[jj];
				cdsPciModules.adcCount ++;
	printf("\tfound ADC card instance %d %d\n", cards_used[ii].instance,jj);
			}
		}
		for(jj=0;jj<cdsPciMod->dacCount;jj++)
		{
			if((cards_used[ii].type == cdsPciMod->dacType[jj]) && (cards_used[ii].instance == jj))
			{
				kk = cdsPciModules.dacCount;
				cdsPciModules.pci_dac[kk] = jj;
				cdsPciModules.dacType[kk] = cdsPciMod->dacType[jj];
				cdsPciModules.dacConfig[kk] = cdsPciMod->dacConfig[jj];
				cdsPciModules.dacCount ++;
	printf("\tfound DAC card instance %d %d\n", cards_used[ii].instance,jj);
			}
		}
		for(jj=0;jj<cdsPciMod->doCount;jj++)
		{
			if((cards_used[ii].type == cdsPciMod->doType[jj]) && (cards_used[ii].instance == jj))
			{
				kk = cdsPciModules.doCount;
				cdsPciModules.pci_do[kk] = jj;
				cdsPciModules.doType[kk] = cdsPciMod->doType[jj];
				cdsPciModules.doInstance[kk] = cdsPciMod->doInstance[jj];
				if(cdsPciModules.doType[kk] == ACS_8DIO) cdsPciModules.iiroDioCount ++;
				else printf("found %d %d %d\n",cdsPciMod->doType[jj],ii,jj);
				cdsPciModules.doCount ++;
	printf("\tfound DIO card instance %d %d %d\n", cards_used[ii].instance,jj,cdsPciModules.doCount);
			}
		}
	}
#if 0
	for(ii=0;ii<cards;ii++)
	{
		printf("found card type %d instance %d\n",cards_used[ii].type, cards_used[ii].instance);
		for(jj=0;jj<cdsPciMod->cards;jj++)
		{
			if((cdsPciMod->cards_used[jj].type == cards_used[ii].type) && 
			   (cdsPciMod->cards_used[jj].instance == cards_used[ii].instance))
			{
				status ++;
				ll = cards_used[ii].instance;
                		if(cdsPciMod->cards_used[jj].type == GSC_16AI64SSA)
				{
					kk = cdsPciModules.adcCount;
					cdsPciModules.pci_adc[kk] = ll;
					cdsPciModules.adcType[kk] = cdsPciMod->adcType[ll];
					cdsPciModules.adcConfig[kk] = cdsPciMod->adcConfig[ll];
					cdsPciModules.adcCount ++;
		printf("\tfound ADC card instance %d\n", cards_used[ii].instance);
				}
                		if(cdsPciMod->cards_used[jj].type == GSC_16AO16)
				{
					kk = cdsPciModules.dacCount;
					cdsPciModules.pci_dac[kk] = ll;
					cdsPciModules.dacType[kk] = cdsPciMod->dacType[ll];
					cdsPciModules.dacConfig[kk] = cdsPciMod->dacConfig[ll];
					cdsPciModules.dacCount ++;
		printf("\tfound DAC card instance %d\n", cards_used[ii].instance);
				}
                		if(cdsPciMod->cards_used[jj].type == ACS_8DIO)
				{
					kk = cdsPciModules.doCount;
					cdsPciModules.pci_do[kk] = ll;
					cdsPciModules.doType[kk] = cdsPciMod->doType[ll];
					cdsPciModules.doInstance[kk] = cdsPciMod->doInstance[ll];
					cdsPciModules.iiroDioCount ++;
					cdsPciModules.doCount ++;
		printf("\tfound BIO8 card instance %d\n", cards_used[ii].instance);
				}
			}
		}
	}
#endif
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
                        printf("\tADC %d is a GSC_16AI64SSA module in slot %d\n",ii,cdsPciModules.pci_adc[ii]);
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
                        printf("\tDAC %d is a GSC_16AI64SSA module in slot %d\n",ii,cdsPciModules.pci_dac[ii]);
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
  // return (void *)-1;

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

// return(1);


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

