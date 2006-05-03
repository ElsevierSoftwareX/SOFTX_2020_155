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
#ifdef HEPI_CODE
	#include "hepi.h"	/* User code for HEPI control.		*/
#elif defined(SUS_CODE)
	#include "sus.h"	/* User code for quad control.		*/
#elif defined(PNM)
	#include "pnm.h"	/* User code for Ponderomotive control. */
#else
	#error
#endif

#ifndef NUM_SYSTEMS
#define NUM_SYSTEMS 1
#endif

#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)
char *_epics_shm;		/* Ptr to computer shared memory		*/
long daqBuffer;			/* Address for daq dual buffers in daqLib.c	*/
sem_t irqsem;			/* Semaphore if in IRQ mode.			*/
CDS_HARDWARE cdsPciModules;	/* Structure of hardware addresses		*/
volatile int vmeDone = 0;
volatile int stop_working_threads = 0;

#include "msr.h"
#include "fm10Gen.h"		/* CDS filter module defs and C code	*/
#include "feComms.h"		/* Lvea control RFM network defs.	*/
#include "daqmap.h"		/* DAQ network layout			*/
extern unsigned long cpu_khz;
#define CPURATE	(cpu_khz/1000)


// #include "fpvalidate.h"		/* Valid FP number ck			*/
#include "drv/daqLib.c"		/* DAQ/GDS connection 			*/
#include "drv/epicsXfer.c"	/* Transfers EPICS data to/from shmem	*/
#ifdef SERVO128K
        #define CYCLE_PER_SECOND        131072
        #define CYCLE_PER_MINUTE        7864320
        #define DAQ_CYCLE_CHANGE        8000
        #define END_OF_DAQ_BLOCK        8191
        #define DAQ_RATE                8192
        #define NET_SEND_WAIT           655360
        #define CYCLE_TIME_ALRM         7
#endif

#ifdef SERVO16K
	#define CYCLE_PER_SECOND	16384
	#define CYCLE_PER_MINUTE	983040
	#define DAQ_CYCLE_CHANGE	770
	#define END_OF_DAQ_BLOCK	1023
	#define DAQ_RATE	DAQ_16K_SAMPLE_SIZE
	#define NET_SEND_WAIT		81920
	#define CYCLE_TIME_ALRM		60
#endif
#ifdef SERVO2K
	#define CYCLE_PER_SECOND	2048
	#define CYCLE_PER_MINUTE	122880
	#define DAQ_CYCLE_CHANGE	120
	#define END_OF_DAQ_BLOCK	127
	#define DAQ_RATE	DAQ_2K_SAMPLE_SIZE
	#define NET_SEND_WAIT		10240
	#define CYCLE_TIME_ALRM		480
#endif

DAQ_RANGE daq;			/* Range settings for daqLib.c		*/

rtl_pthread_t wthread;
rtl_pthread_t wthread1;
rtl_pthread_t wthread2;
int rfd, wfd;

extern int mapPciModules(CDS_HARDWARE *);	/* Init routine to map adc/dac cards	*/
extern long gsaAdcTrigger(int);			/* Starts ADC acquisition.		*/
extern int adcDmaDone(int,int *);		/* Checks if ADC DMA complete.		*/
extern int checkAdcRdy(int,int);		/* Checks if ADC has samples avail.	*/
extern int gsaAdcDma(int,int);			/* Send data to ADC via DMA.		*/
extern int gsaAdcDma1(int,int);			/* Send data to ADC via DMA.		*/
extern int gsaDacDma(int);			/* Send data to DAC via DMA.		*/
extern int gsaDacDma1(int);			/* Send data to DAC via DMA.		*/
extern void gsaDacDma2(int);			/* Send data to DAC via DMA.		*/
extern int myriNetInit(int);			/* Initialize myrinet card.		*/
extern int myriNetClose();			/* Clean up myrinet on exit.		*/
extern int myriNetCheckCallback();		/* Check for messages on myrinet.	*/
extern int myriNetReconnect(int);		/* Make connects to FB.			*/
extern int myriNetCheckReconnect();		/* Check FB net connected.		*/
extern int myriNetDrop();		/* Check FB net connected.		*/
extern int cdsNetStatus;
extern unsigned int readDio(CDS_HARDWARE *,int);


/* ADC/DAC overflow variables */
int overflowAdc[4][32];;
int overflowDac[4][16];;

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
int dacOut[MAX_DAC_MODULES][16];

#ifdef HEPI_CODE
	#include "drv/seiwd.c"	/* User code for HEPI control.		*/
	#include "hepi/hepi.c"	/* User code for HEPI control.		*/
#elif defined(SUS_CODE)
	#include "sus/sus.c"	/* User code for quad control.		*/
#elif defined(PNM)
	#include "pnm/pnm.c"	/* User code for Ponderomotive control. */
#else
	#error
#endif

char daqArea[0x400000];		/* Space allocation for daqLib buffers	*/

#ifdef OVERSAMPLE
#define ADC_SAMPLE_COUNT	0x100
#define ADC_DMA_BYTES		0x400
double feCoeff8x[13] =
        {0.0019451746049824,
        -1.75819687682033,    0.77900344926987,   -1.84669761259482,    0.99885145868275,
        -1.81776674036645,    0.86625937590562,   -1.73730291821706,    0.97396693941237,
        -1.89162859406079,    0.96263319997793,   -0.81263245399030,    0.83542699550059};
double dHistory[96][40];
#else
#define ADC_SAMPLE_COUNT	0x20
#define ADC_DMA_BYTES		0x80
#endif

int clock16K = 0;

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


/************************************************************************/
/* TASK: fe_start()							*/
/* This routine is the skeleton for all front end code			*/
/************************************************************************/
void *fe_start(void *arg)
{

  int ii,jj,kk;
  int clock1Min = 0;
  int cpuClock[10];
  short adcData[MAX_ADC_MODULES][32];
  int *packedData;
  unsigned int *pDacData;
  RFM_FE_COMMS *pEpicsComms;
  int cycleTime;
  int timeHold = 0;
  int timeHoldMax = 0;
  int myGmError2 = 0;
  int attemptingReconnect = 0;
  int netRestored = 0;
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
  static int dropSends = 0;
  int diagWord = 0;
  int epicsCycle = 0;
  int system = 0;


// Do all of the initalization

  /* Init comms with EPICS processor */
  pEpicsComms = (RFM_FE_COMMS *)_epics_shm;
  pLocalEpics = (CDS_EPICS *)&pEpicsComms->epicsSpace;


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
  printf("Waiting for EPICS BURT\n");
  do{
	usleep(1000000);
  }while(!pLocalEpics->epicsInput.burtRestore);

  // Need this FE dcuId to make connection to FB
  dcuId = pLocalEpics->epicsInput.dcuId;

#ifndef NO_DAQ
  // Make connections to FrameBuilder
  printf("Waiting for Network connect to FB - %d\n",dcuId);
  netRetry = 0;
  status = myriNetReconnect(dcuId);
  do{
	usleep(10000);
	status = myriNetCheckReconnect();
	netRetry ++;
  }while((status != 0) && (netRetry < 10));

  // If there is no answer from FB for connection request, continue,
  // but mark net as bad and needs retry later.
  if(netRetry >= 10)
  {
  	printf("Net Connect to FB FAILED!!! - %d\n",dcuId);
	// Set error flag, which will cause daqLib to quit sending
	// data to FB until problem is fixed.
	myGmError2 = 1;
	attemptingReconnect = 2;
	rdtscl(cpuClock[8]);
	netRestored = 0;
	printf("Net fail - recon try\n");
	pLocalEpics->epicsOutput.diagWord |= 4;
  }
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




  /* Set data range limits for daqLib routine */
  daq.filtExMin = (dcuId-5) * GDS_TP_MAX_FE;
  daq.filtExMax = daq.filtExMin + MAX_MODULES;
  daq.filtExSize = MAX_MODULES;
  daq.xExMin = -1;
  daq.xExMax = -1;
  daq.filtTpMin = daq.filtExMin + 10000;
  daq.filtTpMax = daq.filtTpMin + MAX_MODULES * 3;
  daq.filtTpSize = MAX_MODULES * 3;
  daq.xTpMin = daq.filtTpMax;
  daq.xTpMax = daq.xTpMin + 50;

  printf("DAQ Ex Min/Max = %d %d\n",daq.filtExMin,daq.filtExMax);
  printf("DAQ Tp Min/Max = %d %d\n",daq.filtTpMin,daq.filtTpMax);
  printf("DAQ XTp Min/Max = %d %d\n",daq.xTpMin,daq.xTpMax);

  // Set an xtra TP to read out one pps signal
  testpoint[0] = (float *)&onePps;
  pLocalEpics->epicsOutput.diagWord = 0;
  pLocalEpics->epicsOutput.diags[1] = 0;
  pLocalEpics->epicsOutput.diags[2] = 0;


#ifndef NO_DAQ
  // Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0,pLocalEpics->epicsOutput.gdsMon,xExc);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    vmeDone = 1;
    return(0);
  }
#endif

  // Clear the startup sync counter
  firstTime = 0;

  // Clear the code exit flag
  vmeDone = 0;
	feCode(clock16K,dWord,dacOut,dspPtr[0],&dspCoeff[0],pLocalEpics,1);

  printf("entering the loop\n");
  // Trigger the ADC to start running
  gsaAdcTrigger(cdsPciModules.adcCount);

  // Clear a couple of timing diags.
  adcHoldTime = 0;
  usrHoldTime = 0;

  // Initialize the ADC/DAC DMA registers
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  status = gsaAdcDma1(jj,ADC_DMA_BYTES);
  for(jj=0;jj<cdsPciModules.dacCount;jj++)
		status = gsaDacDma1(jj);

  // Enter the coninuous FE control loop  **********************************************************
  while(!vmeDone){

  	// diagWord = 0;
        rdtscl(cpuClock[2]);
	// Following call blocks until ADC has correct number of samples ready
	// The call then starts the DMA input of all ADC modules.
	status = checkAdcRdy(ADC_SAMPLE_COUNT,cdsPciModules.adcCount);
	// status = checkAdcRdy(ADC_SAMPLE_COUNT,1);
	// Read CPU clock for timing info
// usleep(0);
        rdtscl(cpuClock[0]);
	// Start reading ADC data
  	for(jj=0;jj<cdsPciModules.adcCount;jj++)
		gsaAdcDma2(jj);
    
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
	  if(adcHoldTime > CYCLE_TIME_ALRM) diagWord |= 2;
	  if(timeHoldMax > CYCLE_TIME_ALRM) diagWord |= 8;
  	  if(pLocalEpics->epicsInput.diagReset)
	  {
		pLocalEpics->epicsInput.diagReset = 0;
		adcHoldTime = 0;
		timeHoldMax = 0;
		printf("DIAG RESET\n");
	  }
	  pLocalEpics->epicsOutput.diagWord = diagWord;
	  diagWord = 0;
        }

	/* Update Epics variables */
#ifdef HEPI_CODE
	epicsCycle = (epicsCycle + 1) % (MAX_MODULES + 2);
#else
	epicsCycle = subcycle;
#endif
  	for (system = 0; system < NUM_SYSTEMS; system++)
		updateEpics(epicsCycle, dspPtr[system], pDsp[system],
			    &dspCoeff[system], pCoeff[system]);

        vmeDone = stop_working_threads | checkEpicsReset(epicsCycle, pLocalEpics);
	// Wait for completion of DMA of Adc data
  	for(kk=0;kk<cdsPciModules.adcCount;kk++)
	{
		// Waits for DMA complete
		// Return 0x10 if first ADC channel does not have sync bit set
		status = adcDmaDone(kk,(int *)cdsPciModules.pci_adc[kk]);
		jj = kk +1;
		diagWord |= status * jj;
		// if(jj<cdsPciModules.adcCount) gsaAdcDma2(jj);

		// Read adc data into local variables
		packedData = (int *)cdsPciModules.pci_adc[kk];
#ifdef OVERSAMPLE
		for(jj=0;jj<8;jj++)
		{
			for(ii=0;ii<32;ii++)
			{
				adcData[kk][ii] = (*packedData & 0xffff);
				dWord[kk][ii] = iir_filter((double)adcData[kk][ii],&feCoeff8x[0],3,&dHistory[ii+kk*32][0]);
				packedData ++;
			}
		}
#else
		for(ii=0;ii<32;ii++)
		{
			adcData[kk][ii] = (*packedData & 0xffff);
			dWord[kk][ii] = adcData[kk][ii];
			packedData ++;
		}
#endif
		for(ii=0;ii<32;ii++)
		{
			if((adcData[kk][ii] > 32000) || (adcData[kk][ii] < -32000))
			  {
				overflowAdc[kk][ii] ++;
				pLocalEpics->epicsOutput.ovAccum ++;
				diagWord |= 0x100 *  jj;
			  }
		}
	}


	// Assign chan 32 to onePps 
	onePps = dWord[0][31];
#ifdef HEPI_CODE
	if(cdsPciModules.adcCount < 3)
	{           
		for(ii=0;ii<32;ii++) dWord[1][ii] = dWord[0][ii];
		for(ii=0;ii<32;ii++) dWord[2][ii] = dWord[0][ii];
	}
#endif

	// For startup sync to 1pps, loop here
	if(firstTime == 0)
	{
		if(onePps > 6000) 
		 {
			firstTime += 100;
			onePpsHi = 0;
		 }
#ifdef NO_SYNC
		firstTime += 100;
			onePpsHi = 0;
#endif
	}

	if((onePps > 6000) && (onePpsHi == 0))  
	{
		pLocalEpics->epicsOutput.onePps = clock16K;
		onePpsHi = 1;
	}
	if(onePps < 6000) onePpsHi = 0;  
	// Check if front end continues to be in sync with 1pps
	// If not, set sync error flag
	if(pLocalEpics->epicsOutput.onePps > 4) diagWord |= 1;

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

	// Write out data to DAC modules
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
	{
		// Check Dac output overflow and write to DMA buffer
		pDacData = (unsigned int *)cdsPciModules.pci_dac[jj];
		for(ii=0;ii<16;ii++)
		{
			if(dacOut[jj][ii] > 32000) 
			{
				dacOut[jj][ii] = 32000;
				overflowDac[jj][ii] ++;
				pLocalEpics->epicsOutput.ovAccum ++;
				diagWord |= 0x1000 *  (jj+1);
			}
			if(dacOut[jj][ii] < -32000) 
			{
				dacOut[jj][ii] = -32000;
				overflowDac[jj][ii] ++;
				pLocalEpics->epicsOutput.ovAccum ++;
				diagWord |= 0x1000 *  (jj+1);
			}
			*pDacData = (short)(dacOut[jj][ii] & 0xffff);
			pDacData ++;
		}
		// DMA out dac values
		gsaDacDma2(jj);
	}

#ifndef NO_DAQ
	// Write DAQ and GDS values once we are synched to 1pps
  	if(firstTime != 0) 
	{
		// Call daqLib
		pLocalEpics->epicsOutput.diags[3] = 
			daqWrite(1,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],myGmError2,pLocalEpics->epicsOutput.gdsMon,xExc);
		if(!attemptingReconnect)
		{
			// Check and clear network callbacks.
			status = myriNetCheckCallback();
			dropSends = 0;
			// If callbacks pending count is high, try to fix net connection.
			if(status > 4)
			{
				// Set error flag, which will cause daqLib to quit sending
				// data to FB until problem is fixed.
				myGmError2 = 1;
				attemptingReconnect = 2;
				// Send a reconnect request to FB.
				status = myriNetReconnect(dcuId);
				netRestored = 0;
				printf("Net fail - recon try\n");
				diagWord |= 4;
        			rdtscl(cpuClock[8]);
			}
		}
		// If net reconnect requested, check for receipt of return message from FB
		if(attemptingReconnect == 2) 
		{
			diagWord |= 4;
			status = myriNetCheckReconnect();
			if(status == 0)
			{
				netRestored = NET_SEND_WAIT;
				attemptingReconnect = 1;
				printf("Net recon established\n");
				dropSends = 0;
			}
			if((status != 0) && (!clock1Min))
			{
					dropSends = 1;
			}
		}
		// If net successfully reconnected, wait 4-5sec before restoring xmission of DAQ data.
		// This is done to make sure that the callback counter is cleared below the
		// error set point.
		if(attemptingReconnect == 1) 
		{
			netRestored --;
			// Reduce the callback counter.
			status = myriNetCheckCallback();
			// Go back to sending DAQ data.
			if(netRestored <= 0)
			{
				netRestored = 0;
				attemptingReconnect = 0;		
				myGmError2 = 0;
				printf("Continue sending DAQ data\n");
				pLocalEpics->epicsOutput.diags[2] = 0;
			}
		}
	}
#endif
	}

	skipCycle = 0;

	// Measure time to complete 1 cycle
        rdtscl(cpuClock[1]);

	// Compute max time of one cycle.
	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
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
	  usrHoldTime = 0;
#ifndef NO_DAQ
	  if(attemptingReconnect && ((cdsNetStatus != 0) || (dropSends != 0)))
	  {
		status = myriNetReconnect(dcuId);
		cdsNetStatus = 0;
		pLocalEpics->epicsOutput.diags[2] ++;
		dropSends = 0;
	  }
#endif
  	  if(pLocalEpics->epicsInput.syncReset)
	  {
		pLocalEpics->epicsInput.syncReset = 0;
		skipCycle = 1;
	  }
  	  if((pLocalEpics->epicsInput.overflowReset) || (pLocalEpics->epicsOutput.ovAccum > 0x1000000))
	  {
		pLocalEpics->epicsInput.overflowReset = 0;
		pLocalEpics->epicsOutput.ovAccum = 0;
	  }
        }
        if((subcycle == 0) && (daqCycle == 13))
        {
	  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  {
	    for(ii=0;ii<32;ii++)
	    {
		pLocalEpics->epicsOutput.overflowAdc[jj][ii] = overflowAdc[jj][ii];
		overflowAdc[jj][ii] = 0;
	    }
	  }
	  for(jj=0;jj<cdsPciModules.dacCount;jj++)
	  {
	    for(ii=0;ii<16;ii++)
	    {
		pLocalEpics->epicsOutput.overflowDac[jj][ii] = overflowDac[jj][ii];
		overflowDac[jj][ii] = 0;
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
	printf("cpu clock %ld\n",cpu_khz);

        /*
         * Create the shared memory area.  By passing a non-zero value
         * for the mode, this means we also create a node in the GPOS.
         */       
        wfd = shm_open("/rtl_epics", RTL_O_RDWR, 0666);
        if (wfd == -1) {
                printf("open failed for write on /rtl_epics (%d)\n",errno);
                rtl_perror("shm_open()");
                return -1;
        }

        _epics_shm = (unsigned char *)rtl_mmap(NULL,MMAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,wfd,0);
        if (_epics_shm == MAP_FAILED) {
                printf("mmap failed for writer\n");
                rtl_perror("mmap()");
                return (void *)(-1);
        }

	printf("Initializing PCI Modules\n");
	status = mapPciModules(&cdsPciModules);
	printf("%d PCI cards found\n",status);
	printf("%d ADC cards found\n",cdsPciModules.adcCount);
	printf("%d DAC cards found\n",cdsPciModules.dacCount);
	printf("%d DIO cards found\n",cdsPciModules.dioCount);

	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
	printf("Initializing Network\n");
#ifndef NO_DAQ
	status = myriNetInit(2);
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
#endif

#ifdef RESERVE_CPU3
        rtl_pthread_attr_setcpu_np(&attr, 3);
        rtl_pthread_attr_setreserve_np(&attr, 1);
        rtl_pthread_create(&wthread2, &attr, cpu3_start, 0);
#endif

        rtl_pthread_attr_setcpu_np(&attr, 1);
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
#endif

 out:

#ifndef NO_DAQ
	status = myriNetClose();
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


