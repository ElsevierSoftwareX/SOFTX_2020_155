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
#define MAX_FILTERS     960     /* Max number of filters to one file 	*/
#define MAX_MODULES     96      /* Max number of modules to one file 	*/
#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)
char *_epics_shm;		/* Ptr to computer shared memory		*/
long daqBuffer;			/* Address for daq dual buffers in daqLib.c	*/
sem_t irqsem;			/* Semaphore if in IRQ mode.			*/
CDS_HARDWARE cdsPciModules;	/* Structure of hardware addresses		*/

#include "msr.h"
#include "fm10Gen.h"		/* CDS filter module defs and C code	*/
#include "iscNetDsc.h"		/* Lvea control RFM network defs.	*/
#include "daqmap.h"		/* DAQ network layout			*/
#define CPURATE	2800

#include "fpvalidate.h"		/* Valid FP number ck			*/
#include "drv/daqLib.c"		/* DAQ/GDS connection 			*/
#include "drv/epicsXfer.c"	/* Transfers EPICS data to/from shmem	*/
#include "./quad.c"		/* User code for quad control.		*/

DAQ_RANGE daq;			/* Range settings for daqLib.c		*/

rtl_pthread_t wthread;
int rfd, wfd;

extern int mapPciModules(CDS_HARDWARE *);	/* Init routine to map adc/dac cards	*/
extern long gsaAdcTrigger(int);			/* Starts ADC acquisition.		*/
extern int adcDmaDone();			/* Checks if ADC DMA complete.		*/
extern int checkAdcRdy(int count);		/* Checks if ADC has samples avail.	*/
extern int gsaAdcDma(int,int);			/* Send data to ADC via DMA.		*/
extern int gsaDacDma(int);			/* Send data to DAC via DMA.		*/
extern int myriNetInit();			/* Initialize myrinet card.		*/
extern int myriNetClose();			/* Clean up myrinet on exit.		*/
extern int myriNetCheckCallback();		/* Check for messages on myrinet.	*/
extern int myriNetReconnect(int);		/* Make connects to FB.			*/
extern int myriNetCheckReconnect();		/* Check FB net connected.		*/
extern int myriNetDrop();		/* Check FB net connected.		*/
extern int cdsNetStatus;


/* ADC/DAC overflow variables */
int overflowAdc[4][32];;
int overflowDac[4][16];;

float *testpoint[20];


CDS_EPICS pLocalEpics;   	/* Local mem ptr to EPICS control data	*/
CDS_EPICS *pLocalEpicsRfm; 	/* Local mem ptr to EPICS control data	*/
CDS_EPICS *plocalEpics;   	/* Local mem ptr to EPICS control data	*/


/* 1/16 sec cycle counters for DAQS and ISC RFM IPC		*/
int subcycle = 0;		/* Internal cycle counter	*/ 
unsigned int daqCycle;		/* DAQS cycle counter		*/

int firstTime;			/* Dummy var for startup sync.	*/

FILT_MOD dsp;			/* SFM structure.		*/
FILT_MOD *dspPtr[1];		/* SFM structure pointer.	*/
FILT_MOD *pDsp;			/* Ptr to SFM in shmem.		*/
COEF dspCoeffMemSpace;		/* Local mem for SFM coeffs.	*/
COEF *dspCoeff;			/* Ptr to SFM coeffs in local mem.	*/
VME_COEF *pCoeff;		/* Ptr to SFM coeffs in shmem		*/
int *epicsInAddShm;		/* Ptr to epics inputs in shmem		*/
int *epicsInAddLoc;		/* Ptr to epics inputs in local mem	*/
int *epicsOutAddLoc;		/* Ptr to epics outputs in local mem	*/
int *epicsOutAddShm;		/* Ptr to epics outputs in shmem.	*/
char daqArea[0x400000];		/* Space allocation for daqLib buffers	*/

#ifdef OVERSAMPLE
#define ADC_SAMPLE_COUNT	0x100
#define ADC_DMA_BYTES		0x400
double dCoeff8x[9] =
	{0.0020893039272331,
	-1.76081044819067, 0.79178857629531, -1.33077060017586, 1.0,
	-1.81299681613251, 0.91908202908703, 0.09475575216002,1.0};
#if 0
double dCoeff8x[9] =
	{0.0012231229599977,
	-1.88212816674877, 0.89023589750808, -1.81125517371268, 1.0,
	-1.93089044909424, 0.95813764806104, -1.14714341559299, 1.0};
#endif
double dHistory[64][40];
#else
#define ADC_SAMPLE_COUNT	0x20
#define ADC_DMA_BYTES		0x80
#endif

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

  static int vmeDone;
  int ii,jj,kk;
  int clock16K = 0;
  int clock1Min = 0;
  int cpuClock[10];
  short adcData[4][32];
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
  short dacData[16];
  double dWord[64];
  int dacOut[32];
  int dcuId;
  static int adcTime;
  static int adcHoldTime;
  static int usrTime;
  static int usrHoldTime;
  int netRetry;
  float xExc[10];
  static int skipCycle = 0;
  static int dropSends = 0;


// Do all of the initalization

  /* Init comms with EPICS processor */
  pEpicsComms = (RFM_FE_COMMS *)_epics_shm;
  plocalEpics = (CDS_EPICS *)&pLocalEpics;
  pLocalEpicsRfm = (CDS_EPICS *)&pEpicsComms->epicsSpace;

  // Get addresses of epics in shared memory
  epicsInAddShm = (int *)&pEpicsComms->epicsSpace.epicsShm.epicsInput;
  epicsOutAddShm = (int *)&pEpicsComms->epicsSpace.epicsShm.epicsOutput;
  epicsInAddLoc = (int *)&plocalEpics->epicsInput;
  epicsOutAddLoc = (int *)&plocalEpics->epicsOutput;

  printf("Local epics 0x%lx 0x%lx 0x%lx\n",(long)plocalEpics, epicsInAddLoc, epicsOutAddLoc);

  // Set pointers to SFM data buffers
  pDsp = (FILT_MOD *)(&pEpicsComms->dspSpace);
  pCoeff = (VME_COEF *)(&pEpicsComms->coeffSpace);
  dspCoeff = (COEF *)&dspCoeffMemSpace;
  dspPtr[0] = (FILT_MOD *)&dsp;

  // Clear the FE reset which comes from Epics
  pLocalEpicsRfm->epicsInput.vmeReset = 0;

  // Do not proceed until EPICS has had a BURT restore
  printf("Waiting for EPICS BURT\n");
  do{
	usleep(1000000);
  }while(!pLocalEpicsRfm->epicsInput.burtRestore);

  /* Get all epics input data from SHM */
  getEpicsData(epicsInAddShm, epicsInAddLoc,EPICS_IN_SIZE,0);

  // Need this FE dcuId to make connection to FB
  dcuId = pLocalEpicsRfm->epicsInput.dcuId;

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
	pLocalEpics.epicsOutput.diagWord |= 4;
  }

  /* Initialize filter banks */
  for(ii=0;ii<MAX_MODULES;ii++){
    for(jj=0;jj<FILTERS;jj++){
      for(kk=0;kk<MAX_COEFFS;kk++){
        dspCoeff->coeffs[ii].filtCoeff[jj][kk] = 0.0;
      }
      dspCoeff->coeffs[ii].filtSections[jj] = 0;
    }
  }

  /* Initialize all filter module excitation signals to zero */
  for(ii=0;ii<MAX_MODULES;ii++)
        dsp.data[ii].exciteInput = 0.0;


    /* Initialize DSP filter bank values */
  initVars(&dsp,pDsp,dspCoeff,MAX_MODULES,pCoeff);

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
  pLocalEpics.epicsOutput.diagWord = 0;
  pLocalEpics.epicsOutput.diags[1] = 0;
  pLocalEpics.epicsOutput.diags[2] = 0;


  // Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_16K_SAMPLE_SIZE,testpoint,dspPtr,0,pLocalEpics.epicsOutput.gdsMon,xExc);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    return(0);
  }

  // Clear the startup sync counter
  firstTime = 0;

  // Clear the code exit flag
  vmeDone = 0;

  printf("entering the loop\n");
  // Trigger the ADC to start running
  gsaAdcTrigger(1);

  // Clear a couple of timing diags.
  adcHoldTime = 0;
  usrHoldTime = 0;

  // Enter the coninuous FE control loop  **********************************************************
  while(!vmeDone){

  	pLocalEpics.epicsOutput.diagWord = 0;
        rdtscl(cpuClock[2]);
	// Check ADC has data ready; hang here until it does
	status = checkAdcRdy(ADC_SAMPLE_COUNT);
	// Read CPU clock for timing info
        rdtscl(cpuClock[0]);
	// Start reading ADC data
	status = gsaAdcDma(0,ADC_DMA_BYTES);

	// Wait for completion of DMA of Adc data
	do{
		status = adcDmaDone();
	}while(!status);

    
        // Update internal cycle counters
        if((firstTime != 0) && (!skipCycle))
        {
          clock16K += 1;
          clock16K %= 16384;
	  clock1Min += 1;
	  clock1Min %= 983040;
          if(subcycle == 770) daqCycle = (daqCycle + 1) % 16;
          if(subcycle == 1023) /*we have reached the 16Hz second barrier*/
            {
              /* Reset the data cycle counter */
              subcycle = 0;
  
            }
          else{
            /* Increment the internal cycle counter */
            subcycle ++;                                                
          }
        }

	// Read adc data into local variables
	packedData = (int *)cdsPciModules.pci_adc[0];
#ifdef OVERSAMPLE
	for(jj=0;jj<8;jj++)
	{
	for(ii=0;ii<32;ii++)
	{
		adcData[0][ii] = (*packedData & 0xffff);
		dWord[ii] = iir_filter((double)adcData[0][ii],&dCoeff8x[0],2,&dHistory[ii][0]);
		packedData ++;
	}
	}
#else
	for(ii=0;ii<32;ii++)
	{
		adcData[0][ii] = (*packedData & 0xffff);
		dWord[ii] = adcData[0][ii];
		packedData ++;
	}
#endif
	for(jj=0;jj<cdsPciModules.adcCount;jj++)
	{
		for(ii=0;ii<32;ii++)
		{
			if((adcData[jj][ii] > 32000) || (adcData[jj][ii] < -32000))
			  {
				overflowAdc[jj][ii] ++;
				pLocalEpics.epicsOutput.ovAccum ++;
			  }
		}
	}

	// Assign chan 32 to onePps 
	onePps = dWord[31];

	// For startup sync to 1pps, loop here
	if(firstTime == 0)
	{
		if(onePps < 1000) firstTime += 100;
	}

	if(onePps < 1000)  pLocalEpics.epicsOutput.onePps = clock16K;
	// Check if front end continues to be in sync with 1pps
	// If not, set sync error flag
	if(pLocalEpics.epicsOutput.onePps > 4) pLocalEpics.epicsOutput.diagWord |= 1;

	if(!skipCycle)
 	{
	// Call the front end specific software
        rdtscl(cpuClock[4]);
	feCode(dWord,dacOut,dspPtr[0],dspCoeff,plocalEpics);
        rdtscl(cpuClock[5]);

	// Write out data to DAC modules
	for(jj=0;jj<cdsPciModules.dacCount;jj++)
	{
		// Check Dac output overflow and write to DMA buffer
		pDacData = (unsigned int *)cdsPciModules.pci_dac[jj];
		for(ii=0;ii<16;ii++)
		{
			if(dacOut[ii] > 32000) 
			{
				dacOut[ii] = 32000;
				overflowDac[jj][ii] ++;
				pLocalEpics.epicsOutput.ovAccum ++;
			}
			if(dacOut[ii] < -32000) 
			{
				dacOut[ii] = -32000;
				overflowDac[jj][ii] ++;
				pLocalEpics.epicsOutput.ovAccum ++;
			}
			*pDacData = (short)(dacOut[ii] & 0xffff);
			pDacData ++;
		}
		// DMA out dac values
		status = gsaDacDma(jj);
	}

	// Write DAQ and GDS values once we are synched to 1pps
  	if(firstTime != 0) 
	{
		// Call daqLib
		pLocalEpics.epicsOutput.diags[3] = 
			daqWrite(1,dcuId,daq,DAQ_16K_SAMPLE_SIZE,testpoint,dspPtr,myGmError2,pLocalEpics.epicsOutput.gdsMon,xExc);
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
				pLocalEpics.epicsOutput.diagWord |= 4;
        			rdtscl(cpuClock[8]);
			}
		}
		// If net reconnect requested, check for receipt of return message from FB
		if(attemptingReconnect == 2) 
		{
			pLocalEpics.epicsOutput.diagWord |= 4;
			status = myriNetCheckReconnect();
			if(status == 0)
			{
				netRestored = 65000;
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
				pLocalEpics.epicsOutput.diags[2] = 0;
			}
		}
	}
	}

	skipCycle = 0;

	/* Update Epics variables */
	vmeDone = updateEpics(subcycle,epicsInAddShm,epicsOutAddShm,epicsInAddLoc,epicsOutAddLoc,
				dspPtr[0],pDsp,dspCoeff,pCoeff,plocalEpics);	

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

        if((subcycle == 0) && (daqCycle == 15))
        {
	  pLocalEpics.epicsOutput.cpuMeter = timeHold;
	  pLocalEpics.epicsOutput.cpuMeterMax = timeHoldMax;
          timeHold = 0;
	  pLocalEpics.epicsOutput.adcWaitTime = adcHoldTime;
	  if(adcHoldTime > 60) pLocalEpics.epicsOutput.diagWord |= 2;
	  if(timeHoldMax > 60) pLocalEpics.epicsOutput.diagWord |= 8;
  	  if(pLocalEpicsRfm->epicsInput.diagReset)
	  {
		pLocalEpicsRfm->epicsInput.diagReset = 0;
		adcHoldTime = 0;
		timeHoldMax = 0;
		printf("DIAG RESET\n");
	  }
        }
        if((subcycle == 0) && (daqCycle == 14))
        {
	  pLocalEpics.epicsOutput.diags[0] = usrHoldTime;
	  usrHoldTime = 0;
	  if(attemptingReconnect && ((cdsNetStatus != 0) || (dropSends != 0)))
	  {
		status = myriNetReconnect(dcuId);
		cdsNetStatus = 0;
		pLocalEpics.epicsOutput.diags[2] ++;
		dropSends = 0;
	  }
  	  if(pLocalEpicsRfm->epicsInput.syncReset)
	  {
		pLocalEpicsRfm->epicsInput.syncReset = 0;
		skipCycle = 1;
	  }
  	  if((pLocalEpicsRfm->epicsInput.overflowReset) || (pLocalEpics.epicsOutput.ovAccum > 0x1000000))
	  {
		pLocalEpicsRfm->epicsInput.overflowReset = 0;
		pLocalEpics.epicsOutput.ovAccum = 0;
	  }
        }
        if((subcycle == 0) && (daqCycle == 13))
        {
	  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  {
	    for(ii=0;ii<32;ii++)
	    {
		pLocalEpics.epicsOutput.overflowAdc[jj][ii] = overflowAdc[jj][ii];
		overflowAdc[jj][ii] = 0;
	    }
	  }
	  for(jj=0;jj<cdsPciModules.dacCount;jj++)
	  {
	    for(ii=0;ii<16;ii++)
	    {
		pLocalEpics.epicsOutput.overflowDac[jj][ii] = overflowDac[jj][ii];
		overflowDac[jj][ii] = 0;
	    }
	  }
        }

  }

  /* System reset command received */
  return (void *)-1;
}
int main(int argc, char **argv)
{
        pthread_attr_t attr;
 	int status;

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

	printf("Initializing space for daqLib buffers\n");
	daqBuffer = (long)&daqArea[0];
 
	printf("Initializing Network\n");
	status = myriNetInit();
#if 0
        /* initialize the semaphore */
        sem_init (&irqsem, 1, 0);
#endif



        rtl_pthread_attr_init(&attr);
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

	status = myriNetClose();
        /* kill the threads */
        rtl_pthread_cancel(wthread);
        rtl_pthread_join(wthread, NULL);

        /* unmap the shared memory areas */
        munmap(_epics_shm, MMAP_SIZE);

        /* Note that this is a shared area created with shm_open() - we close
         * it with close(), but use shm_unlink() to actually destroy the area
         */
        close(wfd);

        return 0;
}


