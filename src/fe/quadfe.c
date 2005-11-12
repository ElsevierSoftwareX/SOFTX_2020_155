/*----------------------------------------------------------------------*/
/*                                                                      */
/* Module Name: lsc.c                                                   */
/*                                                                      */
/* Module Description: LSC frontend code, with Lock Acq. incorp.        */
/*                                                                      */
/* Module Arguments:    Code started by 'sp lsc_start'			*/
/*                                                                      */
/* Revision History:                                                    */
/* Rel   Date    Engineer   Comments                                    */
/* 1.0   16Apr02 R. Bork    Initial installation test release. 		*/
/*                                                                      */
/* Documentation References:                                            */
/*      Man Pages:                                                      */
/*      References:                                                     */
/*                                                                      */
/* Author Information:                                                  */
/*      Name          Telephone    Fax          e-mail                  */
/*      Rolf Bork   . (626)3953182 (626)5770424 rolf@ligo.caltech.edu   */
/*                                                                      */
/* Code Compilation and Runtime Specifications:                         */
/*      Code Compiled on: Sun Ultra60  running Solaris2.8               */
/*      Compiler Used: cc386		                                */
/*      Runtime environment: PentiumIII 1.2G running VxWorks 5.4.1      */
/*                                                                      */
/* Hardware Required:
	1) Pentium 1.2GHz CPU w/2 VMIC5579 and one VMIC5565PMC RFM modules
	2) Three Pentek Adc/Dac Modules set to 0xff00, 0xfe00, & 0xfd00 SIO.
	3) Clock Driver Module set to 0x9000 in SIO.
	4) Xycom 220 Module set to 0xc800 SIO.
	5) Latest rev Pentek clock fanout module.			*/
/*                                                                      */
/* Known Bugs, Limitations, Caveats:                                    */
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2002.                      */
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
#define MAX_FILTERS     960     /* Max number of filters to one file 	*/
#define MAX_MODULES     96      /* Max number of modules to one file 	*/
#define INLINE  inline
#define MMAP_SIZE (64*1024*1024 - 5000)
#if 0
#include "drv/vmic5579.h"
#include "drv/vmic5565.h"
#endif
char *_epics_shm;
long _pci_rfm;
long _pci_adc;
long _pci_dac;
extern char *pRfmMem;
sem_t irqsem;

#include "msr.h"
#include "fm10Gen.h"		/* CDS filter module defs and C code	*/
#include "iscNetDsc.h"		/* Lvea control RFM network defs.	*/
#include "daqmap.h"		/* DAQ network layout			*/
#include "gdsLib.h"
#define CPURATE	2800
#define MEM_LOCATOR	0x7

#include "fpvalidate.h"	/* Valid FP number ck	*/
#include "drv/daqLib.c"	/* DAQ connection 	*/
#include "drv/epicsXfer.c"
#include "./quad.c"
#if 0
#include "drv/gdsLib.c"	/* GDS connection 	*/
#endif

DAQ_RANGE daq;

rtl_pthread_t wthread;
int rfd, wfd;

extern long mapcard(int state, int memsize);
extern int rfm5565dma(int rfmMemLocator, int byteCount, int direction);
extern int rfm5565DmaDone();
extern long gsaAdcTrigger();
extern long gsaAdcStop();
extern long mapAdc();
extern int adcDmaDone();
extern int checkAdcRdy(int count);
extern int gsaAdcDma(int byteCount);
extern long mapDac();
extern void trigDac(short dacData[]);
extern int gsaDacDma();
extern int myriNetInit();
extern int myriNetClose();
extern int myriNetCheckCallback();
extern int myriNetDaqSend(int cycle, int subCycle, unsigned int fileCrc, int tpCount, int tpNum[], char *dataBuffer);
extern int myriNetReconnect(int waitReply, int dcuId);
extern int myriNetCheckReconnect();


/* ADC/DAC overflow variables */
int overflowAdc[4][8];;
int overflowDac[2][8];;
int ovHoldAdc[4][8];
int ovHoldDac[2][8];

float *testpoint[20];


CDS_EPICS pLocalEpics;   /* Local mem ptr to EPICS control data	*/
CDS_EPICS *pLocalEpicsRfm; /* Local mem ptr to EPICS control data	*/
CDS_EPICS *plocalEpics;   /* Local mem ptr to EPICS control data	*/


/*  DAQS Info		*/
short daqShort[100];
float daqFloat[100];

/* 1/16 sec cycle counters for DAQS and ISC RFM IPC		*/
int subcycle = 0;			/* Internal cycle counter	*/ 
unsigned long daqCycle;		/* DAQS cycle counter		*/

int firstTime;

FILT_MOD dsp;
FILT_MOD *dspPtr[1];
FILT_MOD *pDsp;
COEF dspCoeffMemSpace;
COEF *dspCoeff;
VME_COEF *pCoeff;
int *epicsInAddShm;
int *epicsInAddLoc;
int *epicsOutAddLoc;
int *epicsOutAddShm;
#ifdef MYTEST
int resetCycle = 0;
#endif
char daqArea[0x300000];
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

#if 0

/* Read EPICS data from RFM */
inline void getEpicsData(int *pRfmData, int *pLocalData, int count,int reset)
{
  static UINT32 *rfmPtr;
  static UINT32 *localPtr;
  int ii;

  if(reset==0)
  {
    rfmPtr = (UINT32 *)pRfmData;
    localPtr = (UINT32 *)pLocalData;
  }

  for(ii=0;ii<count;ii++)
  {
    *localPtr = *rfmPtr;
    localPtr ++;
    rfmPtr ++;
  }
}

/* Write EPICS data to RFM */
inline void putEpicsData(int *pRfmData, int *pLocalData, int count,int reset)
{
  static int *rfmPtr;
  static int *localPtr;
  int ii;

  if(reset==0)
  {
    rfmPtr = (int *)pRfmData;
    localPtr = (int *)pLocalData;
  }

  for(ii=0;ii<count;ii++)
  {
    *rfmPtr = *localPtr;
    localPtr ++;
    rfmPtr ++;
  }
}




/************************************************************************/
/* TASK: updateEpics()						*/
/*	Initiates/performs functions required on particular clock cycle	*/
/*	counts. Due to performace reqs of LSC, do not want to do more	*/
/* 	than one housekeeping I/O function per LSC cycle. Resulting 	*/
/* 	update rate to/from EPICS is 16Hz.				*/
/************************************************************************/
inline int updateEpics(){
  
int ii;

  ii = subcycle;
  /* Get EPICS input variables */
  if((ii >= 0) && (ii < (EPICS_IN_SIZE))) 
	  getEpicsData(epicsInAddShm, epicsInAddLoc,1,ii);
  ii -= EPICS_IN_SIZE;

  /* Send EPICS output variables */
  if((ii >= 0) && (ii < (EPICS_OUT_SIZE))) 
	  putEpicsData(epicsOutAddShm, epicsOutAddLoc,1,ii);
  ii -= EPICS_OUT_SIZE;

  /* Check for new filter coeffs or history resets */
  if((ii >= 0) && (ii < MAX_MODULES)) 
	checkFiltReset(ii, &dsp, pDsp, dspCoeff, MAX_MODULES, pCoeff);
  ii -= MAX_MODULES;

  /* Get filter switch settings from EPICS */
  if((ii >= 0) && (ii < MAX_MODULES)) 
	dsp.inputs[ii].opSwitchE = pDsp->inputs[ii].opSwitchE;
  ii -= MAX_MODULES;

  /* Send filter module input data to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES)) 
{
	pDsp->data[ii].filterInput = dsp.data[ii].filterInput;
	pDsp->data[ii].exciteInput = dsp.data[ii].exciteInput;
}
  ii -= MAX_MODULES;

  /* Send filter module 16Hz filtered output to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES)) 
{
	pDsp->data[ii].output16Hz = dsp.data[ii].output16Hz;
	pDsp->data[ii].output = dsp.data[ii].output;
}
  ii -= MAX_MODULES;

  /* Send filter module test point output data to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES)) 
{
	pDsp->data[ii].testpoint = dsp.data[ii].testpoint;
        dsp.inputs[ii].gain_ramp_time = pDsp->inputs[ii].gain_ramp_time;
}
  ii -= MAX_MODULES;

  /* Send filter module switch settings to EPICS */
  if((ii >= 0) && (ii < MAX_MODULES)) 
{
	pDsp->inputs[ii].opSwitchP = dsp.inputs[ii].opSwitchP;
	dsp.inputs[ii].offset = pDsp->inputs[ii].offset;
}
  ii -= MAX_MODULES;

  /* Get filter module gain settings from EPICS */
  if((ii >= 0) && (ii < MAX_MODULES)) 
{
	dsp.inputs[ii].outgain = pDsp->inputs[ii].outgain;
	dsp.inputs[ii].limiter = pDsp->inputs[ii].limiter;
}
  ii -= MAX_MODULES;

     if (pLocalEpics.epicsInput.vmeReset) {
	printf("VME_RESET PUSHED !!! \n");
	gsaAdcStop();
        return(1);
     }
  ii -= 1;

#ifdef TODO
  /* Check for overflow counter reset command */
  if(ii==0) {
	  if(pLocal_epics->overflowReset == 1)
	  {
		pLocal_epics->ovAccum = 0;
	  	pLocalEpicsRfm->overflowReset = 0;
	  }
	if(pLocal_epics->ovAccum > 10000000) pLocal_epics->ovAccum = 0;
  }
#endif

  return(0);

}
#endif


/************************************************************************/
/* TASK: fe_start()							*/
/* This routine is used to start up the lscfe. It calls all of the 	*/
/* initialization routines, spawns the primary servo loop code, and 	*/
/* then exits.								*/
/************************************************************************/
void *fe_start(void *arg)
{

  static int vmeDone;
  int ii,jj,kk;
  int clock16K = 0;
  int cpuClock[6];
  short adcData[32];
  int *packedData;
  unsigned int *pDacData;
  RFM_FE_COMMS *pEpicsComms;
  int cycleTime;
  int timeHold = 0;
  int timeHoldMax = 0;
  int myGmError2 = 0;
  int attemptingReconnect = 0;
  int netRestored = 0;
#if 0
  double fmIn;
  double m0SenOut[6];
  double m0DofOut[6];
  double m0Out[6];
  double r0SenOut[6];
  double r0DofOut[6];
  double r0Out[6];
  double l1SenOut[6];
  double l1DofOut[6];
  double l1Out[6];
  double l2SenOut[6];
  double l2DofOut[6];
  double l2Out[6];
  double l3SenOut[6];
  double l3DofOut[6];
  double l3Out[6];
#endif
  int status;
  float onePps;
  short dacData[16];
  double dWord[64];
  int dacOut[32];
  int dcuId;
  static int adcTime;
  static int adcHoldTime;
  static int gdsMon[2][32];
  int netRetry;



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

  pDsp = (FILT_MOD *)(&pEpicsComms->dspSpace);
  pCoeff = (VME_COEF *)(&pEpicsComms->coeffSpace);
  dspCoeff = (COEF *)&dspCoeffMemSpace;
  dspPtr[0] = (FILT_MOD *)&dsp;

  pLocalEpicsRfm->epicsInput.vmeReset = 0;

  printf("Waiting for EPICS BURT\n");
  do{
	usleep(1000000);
  }while(!pLocalEpicsRfm->epicsInput.burtRestore);

  /* Get all epics input data from SHM */
  getEpicsData(epicsInAddShm, epicsInAddLoc,EPICS_IN_SIZE,0);

  dcuId = pLocalEpicsRfm->epicsInput.dcuId;
  printf("Waiting for Network connect to FB - %d\n",dcuId);
  netRetry = 0;
  status = myriNetReconnect(0,dcuId);
  do{
	usleep(10000);
	status = myriNetCheckReconnect();
	netRetry ++;
  }while((status != 0) && (netRetry < 10));

  if(netRetry >= 10)
  {
  	printf("Net Connect to FB FAILED!!! - %d\n",dcuId);
	// Set error flag, which will cause daqLib to quit sending
	// data to FB until problem is fixed.
	myGmError2 = 1;
	attemptingReconnect = 2;
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
  daq.filtTpMin = GDS_SUS1_VALID_TP_MIN;
  daq.filtTpMax = GDS_SUS1_VALID_TP_MAX;
  daq.filtTpSize = GDS_SUS1_TP_SIZE;
  daq.xTpMin = GDS_SUS1_VALID_XTP_LOW;
  daq.xTpMax = GDS_SUS1_VALID_XTP_HI;

  // Set an xtra TP to read out one pps signal
  testpoint[0] = (float *)&onePps;
  pLocalEpics.epicsOutput.diagWord = 0;


  // Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_16K_SAMPLE_SIZE,testpoint,dspPtr,0,pLocalEpics.epicsOutput.gdsMon);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    return(0);
  }

  firstTime = 0; /* Marks that next Pentek intr will be the first */

  vmeDone = 0;

  printf("entering the loop\n");
  // Trigger the ADC to start running
  gsaAdcTrigger();
  adcHoldTime = 0;
  /*  Run in continuous loop */
  while(!vmeDone){

  	pLocalEpics.epicsOutput.diagWord = 0;
        rdtscl(cpuClock[2]);
	// Check ADC has data ready; hang here until it does
	status = checkAdcRdy(ADC_SAMPLE_COUNT);
	// Read CPU clock for timing info
        rdtscl(cpuClock[0]);
	// Start reading ADC data
	status = gsaAdcDma(ADC_DMA_BYTES);
    
        // Update internal cycle counters
        if(firstTime != 0)
        {
          clock16K += 1;
          clock16K %= 16384;
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

	// Wait for completion of DMA of Adc data
	do{
		status = adcDmaDone();
	}while(!status);

	// Read adc data into local variables
	packedData = (int *)_pci_adc;
#ifdef OVERSAMPLE
	for(jj=0;jj<8;jj++)
	{
	for(ii=0;ii<32;ii++)
	{
		adcData[ii] = (*packedData & 0xffff);
		dWord[ii] = iir_filter((double)adcData[ii],&dCoeff8x[0],2,&dHistory[ii][0]);
		packedData ++;
	}
	}
#else
	for(ii=0;ii<32;ii++)
	{
		adcData[ii] = (*packedData & 0xffff);
		dWord[ii] = adcData[ii];
		packedData ++;
	}
#endif


	// Assign chan 32 to onePps 
	onePps = dWord[31];

	// For startup sync to 1pps, loop here
	if(firstTime == 0)
	{
		if(onePps < 1000) firstTime += 100;
	}
	if(onePps < 1000)  pLocalEpics.epicsOutput.onePps = clock16K;
	if(pLocalEpics.epicsOutput.onePps > 4) pLocalEpics.epicsOutput.diagWord |= 1;

	feCode(dWord,dacOut,dspPtr[0],dspCoeff,plocalEpics);

	// Check Dac output overflow and write to DMA buffer
	pDacData = (unsigned int *)_pci_dac;
	for(ii=0;ii<16;ii++)
	{
		if(dacOut[ii] > 32000) dacOut[ii] = 32000;
		if(dacOut[ii] < -32000) dacOut[ii] = -32000;
		dacData[ii] = (short)dacOut[ii];
		*pDacData = dacData[ii] & 0xffff;
		pDacData ++;
	}
	// DMA out dac values
	status = gsaDacDma();

	// Write daq values once we are synched to 1pps
  	if(firstTime != 0) 
	{
		// Call daqLib
		status = daqWrite(1,dcuId,daq,DAQ_16K_SAMPLE_SIZE,testpoint,dspPtr,myGmError2,pLocalEpics.epicsOutput.gdsMon);
		if(!attemptingReconnect)
		{
			// Check and clear network callbacks.
			status = myriNetCheckCallback();
			// If callbacks pending count is high, try to fix net connection.
			if(status > 6)
			{
				// Set error flag, which will cause daqLib to quit sending
				// data to FB until problem is fixed.
				myGmError2 = 1;
				attemptingReconnect = 2;
				// Send a reconnect request to FB.
				status = myriNetReconnect(0,dcuId);
				netRestored = 0;
				printf("Net fail - recon try\n");
				pLocalEpics.epicsOutput.diagWord |= 4;
			}
		}
		// If net reconnect requested, check for receipt of return message from FB
		if(attemptingReconnect == 2) 
		{
			pLocalEpics.epicsOutput.diagWord |= 4;
			status = myriNetCheckReconnect();
			if(status == 0)
			{
				netRestored = 100000;
				attemptingReconnect = 1;
				printf("Net recon established\n");
			}
		}
		// If net successfully reconnected, wait 4-5sec before storing xmission of DAQ data.
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
			}
		}
	}

	/* Update Epics variables */
	vmeDone = updateEpics(subcycle,epicsInAddShm,epicsOutAddShm,epicsInAddLoc,epicsOutAddLoc,
				dspPtr[0],pDsp,dspCoeff,pCoeff,plocalEpics);	

	// Measure time to complete 1 cycle
        rdtscl(cpuClock[1]);


	cycleTime = (cpuClock[1] - cpuClock[0])/CPURATE;
	if(cycleTime > timeHold) timeHold = cycleTime;
	if(cycleTime > timeHoldMax) timeHoldMax = cycleTime;
	adcTime = (cpuClock[0] - cpuClock[2])/CPURATE;
	if(adcTime > adcHoldTime) adcHoldTime = adcTime;

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

  }

  /* System reset command received */
  return (void *)-1;
}
int main(int argc, char **argv)
{
        pthread_attr_t attr;
  	int ret;
 	int status;
	char *daqMem;

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
                return (void *)-1;
        }

	printf("Initializing ADC\n");
        _pci_adc = mapAdc();

	printf("Initializing DAC\n");
	_pci_dac = mapDac();

	printf("Initializing space for daqLib buffers\n");
	_pci_rfm = (long)&daqArea[0];
 
	printf("Initializing Network\n");
	status = myriNetInit();
#if 0
	_pci_rfm = &daqArea[0];
	pRfmMem = _epics_shm;

	printf("Local IO addr ptr = 0x%lx\n",(long)pRfmMem);

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


