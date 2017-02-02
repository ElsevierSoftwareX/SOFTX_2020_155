/*----------------------------------------------------------------------*/
/*                                                                      */
/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2012.                      */
/*                                                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/

///	@file controller.c
///	@brief Main scheduler program for compiled real-time kernal object. \n
/// 	@detail More information can be found in the following DCC document:
///<	<a href="https://dcc.ligo.org/cgi-bin/private/DocDB/ShowDocument?docid=7688">T0900607 CDS RT Sequencer Software</a>
///	@author R.Bork, A.Ivanov
///     @copyright Copyright (C) 2014 LIGO Project      \n
///<    California Institute of Technology              \n
///<    Massachusetts Institute of Technology           \n\n
///     @license This program is free software: you can redistribute it and/or modify
///<    it under the terms of the GNU General Public License as published by
///<    the Free Software Foundation, version 3 of the License.                 \n
///<    This program is distributed in the hope that it will be useful,
///<    but WITHOUT ANY WARRANTY; without even the implied warranty of
///<    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///<    GNU General Public License for more details.


#include <linux/version.h>
#include <linux/init.h>
#undef printf
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <asm/delay.h>
#include <asm/cacheflush.h>

#include <linux/slab.h>
/// Can't use printf in kernel module so redefine to use Linux printk function
#define printf printk
#include <drv/cdsHardware.h>
#include "inlineMath.h"

#include </usr/src/linux/arch/x86/include/asm/processor.h>
#include </usr/src/linux/arch/x86/include/asm/cacheflush.h>

// Code can be run without shutting down CPU by changing this compile flag
#undef NO_CPU_SHUTDOWN
#ifndef NO_CPU_SHUTDOWN
extern int vprintkl(const char*, va_list);
extern int printkl(const char*, ...);
char fmt1[512];
int printk(const char *fmt, ...) {
    va_list args;
    int r;

    strcat(strcpy(fmt1, SYSTEM_NAME_STRING_LOWER), ": ");
    strcat(fmt1, fmt);
    va_start(args, fmt);
    r = vprintkl(fmt1, args);
    va_end(args);
    return r;
}
#endif


#include "fm10Gen.h"		// CDS filter module defs and C code
#include "feComms.h"		// Lvea control RFM network defs.
#include "daqmap.h"		// DAQ network layout
#include "controller.h"

#ifndef NO_DAQ
#include "drv/fb.h"
#include "drv/daqLib.c"		// DAQ/GDS connection software
#endif

#include "drv/map.h"		// PCI hardware defs
#include "drv/epicsXfer.c"	// User defined EPICS to/from FE data transfer function
#include "timing.c"		// timing module / IRIG-B  functions

#include "drv/inputFilterModule.h"		
#include "drv/inputFilterModule1.h"		

#ifdef DOLPHIN_TEST
#include "dolphinNet1.c"
#endif

/// Maintains present cycle count within a one second period.
int cycleNum = 0;
/// Value of readback from DAC FIFO size registers; used in diags for FIFO overflow/underflow.
unsigned int cycle_gps_time = 0; // Time at which ADCs triggered
unsigned int cycle_gps_event_time = 0; // Time at which ADCs triggered
unsigned int   cycle_gps_ns = 0;
unsigned int   cycle_gps_event_ns = 0;
unsigned int   gps_receiver_locked = 0; // Lock/unlock flag for GPS time card
/// GPS time in GPS seconds
unsigned int timeSec = 0;
unsigned int timeSecDiag = 0;
/* 1 - error occured on shmem; 2 - RFM; 3 - Dolphin */
unsigned int ipcErrBits = 0;
int adcTime;			///< Used in code cycle timing
int adcHoldTime;		///< Stores time between code cycles
int adcHoldTimeMax;		///< Stores time between code cycles
int adcHoldTimeEverMax;		///< Maximum cycle time recorded
int adcHoldTimeEverMaxWhen;
int cpuTimeEverMax;		///< Maximum code cycle time recorded
int cpuTimeEverMaxWhen;
int startGpsTime;
int adcHoldTimeMin;
int adcHoldTimeAvg;
int adcHoldTimeAvgPerSec;
int usrTime;			///< Time spent in user app code
int usrHoldTime;		///< Max time spent in user app code
int cardCountErr = 0;
int cycleTime;			///< Current cycle time
int timeHold = 0;			///< Max code cycle time within 1 sec period
int timeHoldHold = 0;			///< Max code cycle time within 1 sec period; hold for another sec
int timeHoldWhen= 0;			///< Cycle number within last second when maximum reached; running
int timeHoldWhenHold = 0;		///< Cycle number within last second when maximum reached

// The following are for timing histograms written to /proc files
#if defined(SERVO64K) || defined(SERVO32K)
unsigned int cycleHist[32];
unsigned int cycleHistMax[32];
unsigned int cycleHistWhen[32];
unsigned int cycleHistWhenHold[32];
#elif defined(SERVO16K)
unsigned int cycleHist[64];
unsigned int cycleHistMax[64];
unsigned int cycleHistWhen[64];
unsigned int cycleHistWhenHold[64];
#endif


int  getGpsTime(unsigned int *tsyncSec, unsigned int *tsyncUsec); 

// Include C code modules
#include "moduleLoadSwitch.c"
#include "map.c"


char daqArea[2*DAQ_DCU_SIZE];		// Space allocation for daqLib buffers
int cpuId = 1;

// All systems not running at 64K require up/down sampling to communicate I/O data
// with IOP, which is running at 64K.
// Following defines the filter coeffs for these up/down filters.
#ifdef OVERSAMPLE


/* Recalculated filters in biquad form */

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double __attribute__ ((unused)) feCoeff2x[9] =
        {0.053628649721183,
	 0.2568759660371100, -0.3225906481359000, 1.2568801238621801, 1.6774135096891700,
	-0.2061764045745400, -1.0941543149527400, 2.0846376586498803, 2.1966597482716801};

/* Coeffs for the 4x downsampling (16K system) filter */
static double __attribute__ ((unused)) feCoeff4x[9] =
	{0.014805052402446,  
	0.7166258547451800, -0.0683289874517300, 0.3031629575762000, 0.5171469569032900,
	0.6838596423885499, -0.2534855521841101, 1.6838609161411500, 1.7447155374502499};

//
// New Brian Lantz 4k decimation filter
static double __attribute__ ((unused)) feCoeff16x[9] =
        {0.010203728365,
	0.8052941009065100, -0.0241751519071000, 0.3920490703701900, 0.5612099784288400,
	0.8339678987936501, -0.0376022631287799, -0.0131581721533700, 0.1145865116421301};


/* Coeffs for the 32x downsampling filter (2K system) per Brian Lantz May 5, 2009 */
static double __attribute__ ((unused)) feCoeff32x[9] =
	{0.010581064947739,
	 0.90444302586137004, -0.0063413204375699639, -0.056459743474659874, 0.032075154877300172, 
  	 0.92390910024681006, -0.0097523655540199261, 0.077383808424050127, 0.14238741130302013};


// History buffers for oversampling filters
double dHistory[(MAX_ADC_MODULES * 32)][MAX_HISTRY];
double dDacHistory[(MAX_DAC_MODULES * 16)][MAX_HISTRY];
#else

#define OVERSAMPLE_TIMES 1
#endif

#ifdef DUAL_DAQ_DC
	#define MX_OK	15
#else
	#define MX_OK	3
#endif

// Whether run on internal timer (when no ADC cards found)
int run_on_timer = 1;

// Initial diag reset flag
int initialDiagReset = 1;

// Cache flushing mumbo jumbo suggested by Thomas Gleixner, it is probably useless
// Did not see any effect
  char fp [64*1024];


//***********************************************************************
// TASK: fe_start()	
// This routine is the skeleton for all front end code	
//***********************************************************************
/// This function is the main real-time sequencer or scheduler for all code built
/// using the RCG. \n
/// There are two primary modes of operation, based on two compile options: \n
///	- ADC_MASTER: Software is compiled as an I/O Processor (IOP). 
///	- ADC_SLAVE: Normal user control process.
/// This code runs in a continuous loop at the rate specified in the RCG model. The
/// loop is synchronized and triggered by the arrival of ADC data, the ADC module in turn
/// is triggered to sample by the 64KHz clock provided by the Timing Distribution System.
///	- 
void *fe_start(void *arg)
{
  int longestWrite2 = 0;
  int tempClock[4];
  int ii,jj,kk,ll,mm;			// Dummy loop counter variables
  static int clock1Min = 0;		///  @param clockMin Minute counter (Not Used??)
  static int cpuClock[CPU_TIMER_CNT];	///  @param cpuClock[] Code timing diag variables
  static int chanHop = 0;		/// @param chanHop Adc channel hopping status

  int adcData[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];	/// @param adcData[][]  ADC raw data
  int adcChanErr[MAX_ADC_MODULES];
  int adcWait = 0;
  int adcOF[MAX_ADC_MODULES];		/// @param adcOF[]  ADC overrange counters

  // int dacChanErr[MAX_DAC_MODULES];
  int dacOF[MAX_DAC_MODULES];		/// @param dacOF[]  DAC overrange counters
  static int dacWriteEnable = 0;	/// @param dacWriteEnable  No DAC outputs until >4 times through code
  					///< Code runs longer for first few cycles on startup as it settles in,
					///< so this helps prevent long cycles during that time.
  int limit = OVERFLOW_LIMIT_16BIT;      /// @param limit ADC/DAC overflow test value
  int mask = GSAI_DATA_MASK;            /// @param mask Bit mask for ADC/DAC read/writes
  int num_outs = MAX_DAC_CHN_PER_MOD;   /// @param num_outs Number of DAC channels variable
  volatile int *packedData;		/// @param *packedData Pointer to ADC PCI data space
  int wtmin,wtmax;			/// @param wtmin Time window for startup on IRIG-B
  int dacEnable = 0;
  int pBits[9] = {1,2,4,8,16,32,64,128,256};	/// @param pBits[] Lookup table for quick power of 2 calcs
  int sync21ppsCycles = 0;		/// @param sync32ppsCycles Number of attempts to sync to 1PPS
  int dkiTrip = 0;
  RFM_FE_COMMS *pEpicsComms;		/// @param *pEpicsComms Pointer to EPICS shared memory space
  int timeHoldMax = 0;			/// @param timeHoldMax Max code cycle time since last diag reset
  int myGmError2 = 0;			/// @param myGmError2 Myrinet error variable
  int status;				/// @param status Typical function return value
  float onePps;				/// @param onePps Value of 1PPS signal, if used, for diagnostics
  int onePpsHi = 0;			/// @param onePpsHi One PPS diagnostic check
  int onePpsTime = 0;			/// @param onePpsTime One PPS diagnostic check
#ifdef DIAG_TEST
  float onePpsTest;			/// @param onePpsTest Value of 1PPS signal, if used, for diagnostics
  int onePpsHiTest[10];			/// @param onePpsHiTest[] One PPS diagnostic check
  int onePpsTimeTest[10];		/// @param onePpsTimeTest[] One PPS diagnostic check
#endif
  int dcuId;				/// @param dcuId DAQ ID number for this process
  static int missedCycle = 0;		/// @param missedCycle Incremented error counter when too many values in ADC FIFO
  int diagWord = 0;			/// @param diagWord Code diagnostic bit pattern returned to EPICS
  int system = 0;
  int sampleCount = 1;			/// @param sampleCount Number of ADC samples to take per code cycle
  int sync21pps = 0;			/// @param sync21pps Code startup sync to 1PPS flag
  int syncSource = SYNC_SRC_NONE;	/// @param syncSource Code startup synchronization source
  int mxStat = 0;			/// @param mxStat Net diags when myrinet express is used
  int mxDiag = 0;
  int mxDiagR = 0;
// ****** Share data
  int ioClockDac = DAC_PRELOAD_CNT;
  int ioMemCntr = 0;
  int ioMemCntrDac = DAC_PRELOAD_CNT;
  double dac_in =  0.0;			/// @param dac_in DAC value after upsample filtering
  int dac_out = 0;			/// @param dac_out Integer value sent to DAC FIFO

  int feStatus = 0;

  static unsigned int clk, clk1;			// Used only when run on timer enabled (test mode)

  static float duotone[IOP_IO_RATE];		// Duotone timing diagnostic variables
  static float duotoneDac[IOP_IO_RATE];
  float duotoneTimeDac;
  float duotoneTime;
  static float duotoneTotal = 0.0;
  static float duotoneMean = 0.0;
  static float duotoneTotalDac = 0.0;
  static float duotoneMeanDac = 0.0;
  static int dacDuoEnable = 0;
  static int dacTimingError = 0;
  static int dacTimingErrorPending[MAX_DAC_MODULES];

  volatile GSA_18BIT_DAC_REG *dac18bitPtr;	// Pointer to 16bit DAC memory area
  volatile GSC_DAC_REG *dac16bitPtr;		// Pointer to 18bit DAC memory area
  unsigned int usec = 0;
  unsigned int offset = 0;


  int cnt = 0;
  unsigned long cpc;

/// **********************************************************************************************\n
/// Start Initialization Process \n
/// **********************************************************************************************\n
  memset (tempClock, 0, sizeof(tempClock));

  /// \> Flush L1 cache
  memset (fp, 0, 64*1024);
  memset (fp, 1, 64*1024);
  clflush_cache_range ((void *)fp, 64*1024);

  fz_daz(); /// \> Kill the denorms!

// Set memory for cycle time history diagnostics
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
  memset(cycleHist, 0, sizeof(cycleHist));
  memset(cycleHistMax, 0, sizeof(cycleHistMax));
  memset(cycleHistWhen, 0, sizeof(cycleHistWhen));
  memset(cycleHistWhenHold, 0, sizeof(cycleHistWhenHold));
#endif

  /// \> Init comms with EPICS processor */
  pEpicsComms = (RFM_FE_COMMS *)_epics_shm;
  pLocalEpics = (CDS_EPICS *)&pEpicsComms->epicsSpace;
  pEpicsDaq = (char *)&(pLocalEpics->epicsOutput);
// printf("Epics at 0x%x and DAQ at 0x%x  size = %d \n",pLocalEpics,pEpicsDaq,sizeof(CDS_EPICS_IN));

#ifdef OVERSAMPLE
  /// \> Zero out filter histories
  memset(dHistory, 0, sizeof(dHistory));
  memset(dDacHistory, 0, sizeof(dDacHistory));
#endif

  /// \> Set pointers to filter module data buffers. \n
  /// - ---- Prior to V2.8, separate local/shared memories for FILT_MOD data.\n
  /// - ---- V2.8 and later, code uses EPICS shared memory only. This was done to: \n
  /// - -------- Allow daqLib.c to retrieve filter module data directly from shared memory. \n
  /// - -------- Avoid copy of filter module data between to memory locations, which was slow. \n
    pDsp[system] = (FILT_MOD *)(&pEpicsComms->dspSpace);
    pCoeff[system] = (VME_COEF *)(&pEpicsComms->coeffSpace);
    dspPtr[system] = (FILT_MOD *)(&pEpicsComms->dspSpace);

  /// \> Clear the FE reset which comes from Epics
  pLocalEpics->epicsInput.vmeReset = 0;

  // Clear input masks
  pLocalEpics->epicsInput.burtRestore_mask = 0;
  pLocalEpics->epicsInput.dacDuoSet_mask = 0;
  memset(proc_futures, 0, sizeof(proc_futures));

/// \> Init code synchronization source.
  // Look for DIO card or IRIG-B Card
  // if Contec 1616 BIO present, TDS slave will be used for timing.
  syncSource = SYNC_SRC_1PPS;

printf("Sync source = %d\n",syncSource);


/// \> Wait for BURT restore.\n
/// - ---- Code will exit if BURT flag not set by EPICS sequencer.
  // Do not proceed until EPICS has had a BURT restore *******************************
  printf("Waiting for EPICS BURT Restore = %d\n", pLocalEpics->epicsInput.burtRestore);
  cnt = 0;
  do{
	udelay(MAX_UDELAY);
        udelay(MAX_UDELAY);
        udelay(MAX_UDELAY);
  	printf("Waiting for EPICS BURT %d\n", cnt++);
	cpu_relax();
  }while(!pLocalEpics->epicsInput.burtRestore);

  printf("BURT Restore Complete\n");

// BURT has completed *******************************************************************

/// < Read in all Filter Module EPICS coeff settings
    for(ii=0;ii<MAX_MODULES;ii++)
    {
	checkFiltReset(ii, dspPtr[0], pDsp[0], &dspCoeff[0], MAX_MODULES, pCoeff[0]);
    }

  // Need this FE dcuId to make connection to FB
  dcuId = pLocalEpics->epicsInput.dcuId;
  pLocalEpics->epicsOutput.dcuId = dcuId;

  // Reset timing diagnostics
  pLocalEpics->epicsOutput.diagWord = 0;
  pLocalEpics->epicsOutput.timeDiag = 0;
  pLocalEpics->epicsOutput.timeErr = syncSource;
  for(ii=0;ii<32;ii++) {
	  pLocalEpics->epicsOutput.gdsMon[ii] = 0;
  }

/// \> Init IIR filter banks
//   Initialize filter banks  *********************************************
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

  /// \> Initialize all filter module excitation signals to zero 
  for (system = 0; system < NUM_SYSTEMS; system++)
    for(ii=0;ii<MAX_MODULES;ii++)
       // dsp[system].data[ii].exciteInput = 0.0;
       pDsp[0]->data[ii].exciteInput = 0.0;


  /// \> Initialize IIR filter bank values
    if (initVars(pDsp[0], pDsp[0], dspCoeff, MAX_MODULES, pCoeff[0])) {
    	printf("Filter module init failed, exiting\n");
	return 0;
    }

  printf("Initialized servo control parameters.\n");

udelay(1000);

/// \> Initialize DAQ variable/software 
#if !defined(NO_DAQ) && !defined(IOP_TASK)
  /// - ---- Set data range limits for daqLib routine 
#if defined(SERVO2K) || defined(SERVO4K)
  daq.filtExMin = GDS_2K_EXC_MIN;
  daq.filtTpMin = GDS_2K_TP_MIN;
#else
  daq.filtExMin = GDS_16K_EXC_MIN;
  daq.filtTpMin = GDS_16K_TP_MIN;
#endif
  daq.filtExMax = daq.filtExMin + MAX_MODULES;
  daq.filtExSize = MAX_MODULES;
  daq.xExMin = daq.filtExMax;
  daq.xExMax = daq.xExMin + GDS_MAX_NFM_EXC;
  daq.filtTpMax = daq.filtTpMin + MAX_MODULES * 3;
  daq.filtTpSize = MAX_MODULES * 3;
  daq.xTpMin = daq.filtTpMax;
  daq.xTpMax = daq.xTpMin + GDS_MAX_NFM_TP;

  printf("DAQ Ex Min/Max = %d %d\n",daq.filtExMin,daq.filtExMax);
  printf("DAQ XEx Min/Max = %d %d\n",daq.xExMin,daq.xExMax);
  printf("DAQ Tp Min/Max = %d %d\n",daq.filtTpMin,daq.filtTpMax);
  printf("DAQ XTp Min/Max = %d %d\n",daq.xTpMin,daq.xTpMax);

  /// - ---- Assign DAC testpoint pointers

#endif
  pLocalEpics->epicsOutput.ipcStat = 0;
  pLocalEpics->epicsOutput.fbNetStat = 0;
  pLocalEpics->epicsOutput.tpCnt = 0;

#if !defined(NO_DAQ) && !defined(IOP_TASK)
  /// - ---- Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0, (int *)(pLocalEpics->epicsOutput.gdsMon),xExc,pEpicsDaq);
  if(status == -1) 
  {
    printf("DAQ init failed -- exiting\n");
    vmeDone = 1;
    return(0);
  }
#endif

  // Clear the code exit flag
  vmeDone = 0;

  /// \> Call user application software initialization routine.
  printf("Calling feCode() to initialize\n");
  iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0], (struct CDS_EPICS *)pLocalEpics,1);

  // Clear timing diags.
  adcHoldTime = 0;
  adcHoldTimeMax = 0;
  adcHoldTimeEverMax = 0;	
  adcHoldTimeEverMaxWhen = 0;	
  cpuTimeEverMax = 0;		
  cpuTimeEverMaxWhen = 0;	
  startGpsTime = 0;
  adcHoldTimeMin = 0xffff;	
  adcHoldTimeAvg = 0;		
  usrHoldTime = 0;		
  missedCycle = 0;

  /// \> If IOP,  Initialize the ADC modules
  printf("ADC setup complete \n");

  /// \> If IOP, Initialize the DAC module variables
  printf("DAC setup complete \n");

  printf("\n*******************************\n");
  printf("*     Running on timer!       *\n");
  printf("*******************************\n");

  onePpsTime = cycleNum;
  timeSec = current_time() -1;

  rdtscl(adcTime);

  /// ******************************************************************************\n
  /// Enter the infinite FE control loop  ******************************************\n

  /// ******************************************************************************\n
  // Calculate how many CPU cycles per code cycle
  cpc = cpu_khz * 1000;
  cpc /= CYCLE_PER_SECOND;
  printf("CPC is %d \n",cpc);

  rdtscll(clk);
  rdtscl(cpuClock[CPU_TIME_CYCLE_START]);
  unsigned long long clkchk = 0;

  while(!vmeDone){ 	// Run forever until user hits reset
  	rdtscll(clk);
	  // Pause until next cycle begins
	  if (cycleNum == 0) {
	  	// pLocalEpics->epicsOutput.awgStat = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
		// if(pLocalEpics->epicsOutput.awgStat) feStatus |= FE_ERROR_AWG;
		  /// - ---- Check if DAC outputs are enabled, report error.
		if(!iopDacEnable || dkiTrip) feStatus |= FE_ERROR_DAC_ENABLE;
		  // Increment GPS second on cycle 0
		  timeSec ++;
		  if (cycle_gps_time == 0) startGpsTime = timeSec;
		            cycle_gps_time = timeSec;
		  pLocalEpics->epicsOutput.timeDiag = timeSec;
		  // printf("Time is %d - clk = %ld\n",timeSec,clk);
	  }
	  // This is local CPU timer (no ADCs)
	  // advance to the next cycle polling CPU cycles and microsleeping
	  // udelay(1);
	  ioMemCntr = (cycleNum % IO_MEMORY_SLOTS);
	  // Write GPS time and cycle count as indicator to slave that adc data is ready
	  // ioMemData->gpsSecond = timeSec;
	  rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

/// End of ADC Read **************************************************************************************


/// \> Call the front end specific application  ******************\n
/// - -- This is where the user application produced by RCG gets called and executed. \n\n
        rdtscl(cpuClock[CPU_TIME_USR_START]);
 	iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0],(struct CDS_EPICS *)pLocalEpics,0);
        rdtscl(cpuClock[CPU_TIME_USR_END]);
/// \> Send IPC info the gdsMon  ******************\n
#ifdef ADC_MASTER
	if(cycleNum == 5) {
		ioMemData->ipcDetect[0][0] = pLocalEpics->epicsOutput.gdsMon[0];
		ioMemData->ipcDetect[0][1] = pLocalEpics->epicsOutput.gdsMon[1];
		ioMemData->ipcDetect[0][2] = pLocalEpics->epicsOutput.gdsMon[2];
		ioMemData->ipcDetect[0][3] = pLocalEpics->epicsOutput.gdsMon[3];
		ioMemData->ipcDetect[1][0] = pLocalEpics->epicsOutput.gdsMon[4];
		ioMemData->ipcDetect[1][1] = pLocalEpics->epicsOutput.gdsMon[5];
		ioMemData->ipcDetect[1][2] = pLocalEpics->epicsOutput.gdsMon[6];
		ioMemData->ipcDetect[1][3] = pLocalEpics->epicsOutput.gdsMon[7];
	}
#else
	if(cycleNum == 15) {
		pLocalEpics->epicsOutput.gdsMon[10] = ioMemData->ipcDetect[0][0];
		pLocalEpics->epicsOutput.gdsMon[11] = ioMemData->ipcDetect[0][1];
		pLocalEpics->epicsOutput.gdsMon[12] = ioMemData->ipcDetect[0][2];
		pLocalEpics->epicsOutput.gdsMon[13] = ioMemData->ipcDetect[0][3];
		pLocalEpics->epicsOutput.gdsMon[14] = ioMemData->ipcDetect[1][0];
		pLocalEpics->epicsOutput.gdsMon[15] = ioMemData->ipcDetect[1][1];
		pLocalEpics->epicsOutput.gdsMon[16] = ioMemData->ipcDetect[1][2];
		pLocalEpics->epicsOutput.gdsMon[17] = ioMemData->ipcDetect[1][3];
	}
#endif

/// WRITE DAC OUTPUTS ***************************************** \n

/// START OF IOP DAC WRITE ***************************************** \n
        /// \> If DAC FIFO error, always output zero to DAC modules. \n
        /// - -- Code will require restart to clear.
        // COMMENT OUT NEX LINE FOR TEST STAND w/bad DAC cards. 
        // Write out data to DAC modules
	dkiTrip = 0;
	/// \> Loop thru all DAC modules

/// END OF DAC WRITE *************************************************


/// BEGIN HOUSEKEEPING ************************************************ \n

        pLocalEpics->epicsOutput.cycle = cycleNum;
// The following, to endif, is all duotone timing diagnostics.
/// \> Cycle 0: \n
/// - ---- Read IRIGB time if symmetricom card (this is not standard, but supported for earlier cards. \n


/// \> Cycle 18, Send timing info to EPICS at 1Hz
	if(cycleNum ==HKP_TIMING_UPDATES)	
        {
	  pLocalEpics->epicsOutput.cpuMeter = timeHold;
	  pLocalEpics->epicsOutput.cpuMeterMax = timeHoldMax;
  	  // pLocalEpics->epicsOutput.dacEnable = dacEnable;
          timeHoldHold = timeHold;
          timeHold = 0;
	  timeHoldWhenHold = timeHoldWhen;

	  if(timeHoldMax > CYCLE_TIME_ALRM) 
	  {
	  	diagWord |= FE_PROC_TIME_ERR;
		feStatus |= FE_ERROR_TIMING;
	  }
	  // pLocalEpics->epicsOutput.stateWord = feStatus;
  	  feStatus = 0;
  	  if(pLocalEpics->epicsInput.diagReset || initialDiagReset)
	  {
		initialDiagReset = 0;
		pLocalEpics->epicsInput.diagReset = 0;
		pLocalEpics->epicsInput.ipcDiagReset = 1;
  		// pLocalEpics->epicsOutput.diags[1] = 0;
		timeHoldMax = 0;
	  	diagWord = 0;
		ipcErrBits = 0;
		
		// feStatus = 0;
	  }
	  // Flip the onePPS various once/sec as a watchdog monitor.
	  // pLocalEpics->epicsOutput.onePps ^= 1;
	  pLocalEpics->epicsOutput.diagWord = diagWord;
        }


	/// \> Check if code exit is requested
	if(cycleNum == MAX_MODULES) 
		vmeDone = stop_working_threads | checkEpicsReset(cycleNum, (struct CDS_EPICS *)pLocalEpics);


/// \> Cycle 19, write updated diag info to EPICS
	if(cycleNum == HKP_DIAG_UPDATES)	
        {
	  pLocalEpics->epicsOutput.userTime = usrHoldTime;
        }

	// Capture end of cycle time.
        rdtscl(cpuClock[CPU_TIME_CYCLE_END]);

	/// \> Compute code cycle time diag information.
	cycleTime = (cpuClock[CPU_TIME_CYCLE_END] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
	if (longestWrite2 < ((tempClock[3]-tempClock[2])/CPURATE)) longestWrite2 = (tempClock[3]-tempClock[2])/CPURATE;
	if (cycleTime > (1000000/CYCLE_PER_SECOND))  {
		// printf("cycle %d time %d; adcWait %d; write1 %d; write2 %d; longest write2 %d\n", cycleNum, cycleTime, adcWait, (tempClock[1]-tempClock[0])/CPURATE, (tempClock[3]-tempClock[2])/CPURATE, longestWrite2);
	}
	// Hold the max cycle time over the last 1 second
	if(cycleTime > timeHold) { 
		timeHold = cycleTime;
		timeHoldWhen = cycleNum;
	}
	// Hold the max cycle time since last diag reset
	if(cycleTime > timeHoldMax) timeHoldMax = cycleTime;
	adcHoldTime = (cpuClock[CPU_TIME_CYCLE_START] - adcTime)/CPURATE;
	// Avoid calculating the max hold time for the first few seconds
	if (cycleNum != 0 && (startGpsTime+3) < cycle_gps_time) {
		if(adcHoldTime > adcHoldTimeMax) adcHoldTimeMax = adcHoldTime;
		if(adcHoldTime < adcHoldTimeMin) adcHoldTimeMin = adcHoldTime;
		adcHoldTimeAvg += adcHoldTime;
		if (adcHoldTimeMax > adcHoldTimeEverMax)  {
			adcHoldTimeEverMax = adcHoldTimeMax;
			adcHoldTimeEverMaxWhen = cycle_gps_time;
			//printf("Maximum adc hold time %d on cycle %d gps %d\n", adcHoldTimeMax, cycleNum, cycle_gps_time);
		}
		if (timeHoldMax > cpuTimeEverMax)  {
			cpuTimeEverMax = timeHoldMax;
			cpuTimeEverMaxWhen = cycle_gps_time;
		}
	}
	adcTime = cpuClock[CPU_TIME_CYCLE_START];
	// Calc the max time of one cycle of the user code
	// For IOP, more interested in time to get thru ADC read code and send to slave apps
	usrTime = (cpuClock[CPU_TIME_USR_START] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
	if(usrTime > usrHoldTime) usrHoldTime = usrTime;

        /// \> Update internal cycle counters
          cycleNum += 1;
          cycleNum %= CYCLE_PER_SECOND;
	  clock1Min += 1;
	  clock1Min %= CYCLE_PER_MINUTE;
          if(subcycle == DAQ_CYCLE_CHANGE) 
	  {
		daqCycle = (daqCycle + 1) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;
		if(!(daqCycle % 2)) pLocalEpics->epicsOutput.epicsSync = daqCycle;
	  }
          if(subcycle == END_OF_DAQ_BLOCK) /*we have reached the 16Hz second barrier*/
            {
              /* Reset the data cycle counter */
              subcycle = 0;
  
            }
          else{
            /* Increment the internal cycle counter */
            subcycle ++;                                                
          }

/// \> If not exit request, then continue INFINITE LOOP  *******
  }

  printf("exiting from fe_code()\n");
  pLocalEpics->epicsOutput.cpuMeter = 0;

  /* System reset command received */
  return (void *)-1;
}
