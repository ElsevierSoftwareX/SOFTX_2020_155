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
#include "dolphin.c"
#endif

// Contec 64 input bits plus 64 output bits (Standard for aLIGO)
/// Contec6464 input register values
unsigned int CDIO6464InputInput[MAX_DIO_MODULES]; // Binary input bits
/// Contec6464 - Last output request sent to module.
unsigned int CDIO6464LastOutState[MAX_DIO_MODULES]; // Current requested value of the BO bits
/// Contec6464 values to be written to the output register
unsigned int CDIO6464Output[MAX_DIO_MODULES]; // Binary output bits

// This Contect 16 input / 16 output DIO card is used to control timing slave by IOP
/// Contec1616 input register values
unsigned int CDIO1616InputInput[MAX_DIO_MODULES]; // Binary input bits
/// Contec1616 output register values read back from the module
unsigned int CDIO1616Input[MAX_DIO_MODULES]; // Current value of the BO bits
/// Contec1616 values to be written to the output register
unsigned int CDIO1616Output[MAX_DIO_MODULES]; // Binary output bits
/// Holds ID number of Contec1616 DIO card(s) used for timing control.
int tdsControl[3];	// Up to 3 timing control modules allowed in case I/O chassis are daisy chained
/// Total number of timing control modules found on bus
int tdsCount = 0;


/// Maintains present cycle count within a one second period.
int cycleNum = 0;
unsigned int odcStateWord = 0xffff;
/// Value of readback from DAC FIFO size registers; used in diags for FIFO overflow/underflow.
int out_buf_size = 0; // test checking DAC buffer size
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


struct rmIpcStr *daqPtr;
int dacWatchDog = 0;

int  getGpsTime(unsigned int *tsyncSec, unsigned int *tsyncUsec); 

// Include C code modules
#include "moduleLoad.c"
#include "map.c"


char daqArea[2*DAQ_DCU_SIZE];		// Space allocation for daqLib buffers
int cpuId = 1;

// All systems not running at 64K require up/down sampling to communicate I/O data
// with IOP, which is running at 64K.
// Following defines the filter coeffs for these up/down filters.
#ifdef OVERSAMPLE

#if defined(CORE_BIQUAD)

/* Recalculated filters in biquad form */

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double __attribute__ ((unused)) feCoeff2x[9] =
        {0.053628649721183,
	 0.2568759660371100, -0.3225906481359000, 1.2568801238621801, 1.6774135096891700,
	-0.2061764045745400, -1.0941543149527400, 2.0846376586498803, 2.1966597482716801};

/* RCG V3.0 Coeffs for the 4x downsampling (16K system) filter */
static double __attribute__ ((unused)) feCoeff4x[9] =
	{0.054285975,
	 0.3890221, -0.17645085, -0.0417771600000001, 0.41775916,
	 0.52191125, -0.37884382, 1.52190741336686, 1.69347541336686};

// Pre RCG 3.0 filters
//	{0.014805052402446,  
//	0.7166258547451800, -0.0683289874517300, 0.3031629575762000, 0.5171469569032900,
//	0.6838596423885499, -0.2534855521841101, 1.6838609161411500, 1.7447155374502499};

//
// New Brian Lantz 4k decimation filter
static double __attribute__ ((unused)) feCoeff16x[9] =
        {0.010203728365,
	0.8052941009065100, -0.0241751519071000, 0.3920490703701900, 0.5612099784288400,
	0.8339678987936501, -0.0376022631287799, -0.0131581721533700, 0.1145865116421301};

/* Coeffs for the 32x downsampling filter (2K system) */
#if 0
/* Original Rana coeffs from 40m lab elog */
static double __attribute__ ((unused)) feCoeff32x[9] =
        {0.0001104130574447,
	0.9701834961388200, -0.0010837026165800, -0.0200761119821899, 0.0085463156103800,
	0.9871502388637901, -0.0039246182095299, 3.9871502388637898, 3.9960753817904697};
#endif

/* Coeffs for the 32x downsampling filter (2K system) per Brian Lantz May 5, 2009 */
static double __attribute__ ((unused)) feCoeff32x[9] =
	{0.010581064947739,
	 0.90444302586137004, -0.0063413204375699639, -0.056459743474659874, 0.032075154877300172, 
  	 0.92390910024681006, -0.0097523655540199261, 0.077383808424050127, 0.14238741130302013};

#else

/* Oversamping base rate is 64K */
/* Coeffs for the 2x downsampling (32K system) filter */
static double __attribute__ ((unused)) feCoeff2x[9] =
        {0.053628649721183,
        -1.25687596603711,    0.57946661417301,    0.00000415782507,    1.00000000000000,
        -0.79382359542546,    0.88797791037820,    1.29081406322442,    1.00000000000000};
/* Coeffs for the 4x downsampling (16K system) filter */
static double __attribute__ ((unused)) feCoeff4x[9] =
	{0.014805052402446,  
	-1.71662585474518,    0.78495484219691,   -1.41346289716898,   0.99893884152400,
	-1.68385964238855,    0.93734519457266,    0.00000127375260,   0.99819981588176};
//
// New Brian Lantz 4k decimation filter
static double __attribute__ ((unused)) feCoeff16x[9] =
        {0.010203728365,
        -1.80529410090651,   0.82946925281361,  -1.41324503053632,   0.99863016087226,
        -1.83396789879365,   0.87157016192243,  -1.84712607094702,   0.99931484571793};
//

/* Coeffs for the 32x downsampling filter (2K system) per Brian Lantz May 5, 2009 */
static double __attribute__ ((unused)) feCoeff32x[9] =
	{0.010581064947739,
        -1.90444302586137,    0.91078434629894,   -1.96090276933603,    0.99931924465090,
        -1.92390910024681,    0.93366146580083,   -1.84652529182276,    0.99866506867980};
#endif



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
int run_on_timer = 0;

// Initial diag reset flag
int initialDiagReset = 1;

// Cache flushing mumbo jumbo suggested by Thomas Gleixner, it is probably useless
// Did not see any effect
  char fp [64*1024];


#ifdef ADC_SLAVE
// Clear DAC channel shared memory map,
// used to keep track of non-overlapping DAC channels among slave models
//
void deallocate_dac_channels(void) {
  int ii, jj;
  for (ii = 0; ii < MAX_DAC_MODULES; ii++) {
    int pd = cdsPciModules.dacConfig[ii] - ioMemData->adcCount;
    for (jj = 0; jj < 16; jj++)
	if (dacOutUsed[ii][jj]) 
	   ioMemData->dacOutUsed[pd][jj] = 0;
  }
}
#endif

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
#ifdef ADC_MASTER
  volatile int *packedData;		/// @param *packedData Pointer to ADC PCI data space
  volatile unsigned int *pDacData;	/// @param *pDacData Pointer to DAC PCI data space
  int wtmin,wtmax;			/// @param wtmin Time window for startup on IRIG-B
  int dacEnable = 0;
  int pBits[9] = {1,2,4,8,16,32,64,128,256};	/// @param pBits[] Lookup table for quick power of 2 calcs
  int sync21ppsCycles = 0;		/// @param sync32ppsCycles Number of attempts to sync to 1PPS
#endif
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
#ifdef ADC_SLAVE
  int memCtr = 0;
  int ioClock = 0;
#endif
  double dac_in =  0.0;			/// @param dac_in DAC value after upsample filtering
  int dac_out = 0;			/// @param dac_out Integer value sent to DAC FIFO

  int feStatus = 0;

  int clk, clk1;			// Used only when run on timer enabled (test mode)

#ifdef ADC_MASTER
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

  volatile GSA_18BIT_DAC_REG *dac18bitPtr;	// Pointer to 18bit DAC memory area
  volatile GSA_20BIT_DAC_REG *dac20bitPtr;	// Pointer to 20bit DAC memory area
  volatile GSC_DAC_REG *dac16bitPtr;		// Pointer to 16bit DAC memory area
  unsigned int usec = 0;
  unsigned int offset = 0;
#endif


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

  /// \> Zero out DAC outputs
  for (ii = 0; ii < MAX_DAC_MODULES; ii++)
  {
#ifdef ADC_MASTER
    dacTimingErrorPending[ii] = 0;
#endif
    for (jj = 0; jj < 16; jj++) {
 	dacOut[ii][jj] = 0.0;
 	dacOutUsed[ii][jj] = 0;
#ifdef ADC_MASTER
	dacOutBufSize[ii] = 0;
	// Zero out DAC channel map in the shared memory
	// to be used to check on slaves' channel allocation
	ioMemData->dacOutUsed[ii][jj] = 0;
#endif
    }
  }

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
#ifdef ADC_MASTER
  // Look for DIO card or IRIG-B Card
  // if Contec 1616 BIO present, TDS slave will be used for timing.
  if(cdsPciModules.cDio1616lCount) syncSource = SYNC_SRC_TDS;
  else syncSource = SYNC_SRC_1PPS;
#else
   // SLAVE processes get sync from ADC_MASTER
   syncSource = SYNC_SRC_MASTER;
#endif

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
  for (ii = 0; ii <  cdsPciModules.dacCount; ii++)
	for (jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++) // 16 per DAC regardless of the actual
		testpoint[MAX_DAC_CHN_PER_MOD * ii + jj] = floatDacOut + MAX_DAC_CHN_PER_MOD * ii + jj;

  // Zero out storage
  memset(floatDacOut, 0, sizeof(floatDacOut));

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

#ifdef ADC_SLAVE
	/// \> Read Dio card initial values
	/// - ---- SLAVE units read/write their own DIO \n
	/// - ---- MASTER units ignore DIO for speed reasons \n 
     for(kk=0;kk<cdsPciModules.doCount;kk++)
     {
	ii = cdsPciModules.doInstance[kk];
	if(cdsPciModules.doType[kk] == ACS_8DIO)
	{
	  rioInputInput[ii] = accesIiro8ReadInputRegister(&cdsPciModules, kk) & 0xff;
	  rioInputOutput[ii] = accesIiro8ReadOutputRegister(&cdsPciModules, kk) & 0xff;
	  rioOutputHold[ii] = -1;
	} else if(cdsPciModules.doType[kk] == ACS_16DIO) {
  	  rioInput1[ii] = accesIiro16ReadInputRegister(&cdsPciModules, kk) & 0xffff;
	  rioOutputHold1[ii] = -1;
	} else if (cdsPciModules.doType[kk] == CON_32DO) {
  	  CDO32Input[ii] = contec32ReadOutputRegister(&cdsPciModules, kk);
	} else if (cdsPciModules.doType[kk] == CON_6464DIO) {
  	  CDIO6464LastOutState[ii] = contec6464ReadOutputRegister(&cdsPciModules, kk);
	  printf ("Initial state of contec6464 number %d = 0x%x \n",ii,CDIO6464LastOutState[ii]);
	} else if (cdsPciModules.doType[kk] == CDI64) {
  	  CDIO6464LastOutState[ii] = contec6464ReadOutputRegister(&cdsPciModules, kk);
	  printf ("Initial state of contec6464 number %d = 0x%x \n",ii,CDIO6464LastOutState[ii]);
	} else if(cdsPciModules.doType[kk] == ACS_24DIO) {
  	  dioInput[ii] = accesDio24ReadInputRegister(&cdsPciModules, kk);
	}
     }
#endif


  // Clear the code exit flag
  vmeDone = 0;

  /// \> Call user application software initialization routine.
  printf("Calling feCode() to initialize\n");
  iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0], (struct CDS_EPICS *)pLocalEpics,1);

/// \> Verify DAC channels defined for this app are not already in use. \n
/// - ---- User apps are allowed to share DAC modules but not DAC channels.
#ifdef ADC_SLAVE
  // See if my DAC channel map overlaps with already running models
  for (ii = 0; ii < cdsPciModules.dacCount; ii++) {
    int pd = cdsPciModules.dacConfig[ii] - ioMemData->adcCount; // physical DAC number
    for (jj = 0; jj < 16; jj++) {
	if (dacOutUsed[ii][jj]) {
	  if (ioMemData->dacOutUsed[pd][jj])  {
    		vmeDone = 1;
   		printf("Failed to allocate DAC channel.\n");
		printf("DAC local %d global %d channel %d is already allocated.\n", ii, pd, jj);
	  }
    	}
    }
  }
  if (vmeDone) {
     	return(0);
  } else {
    for (ii = 0; ii < cdsPciModules.dacCount; ii++) {
      int pd = cdsPciModules.dacConfig[ii] - ioMemData->adcCount; // physical DAC number
      for (jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++) {
	if (dacOutUsed[ii][jj]) {
	  	ioMemData->dacOutUsed[pd][jj] = 1;
		printf("Setting card local=%d global = %d channel=%d dac usage\n", ii, pd, jj);
	}
      }
    }
  }
#endif

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
#ifdef ADC_MASTER
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
	  // Setup the DMA registers
	  status = gsc16ai64DmaSetup(jj);
	  // Preload input memory with dummy variables to test that new ADC data has arrived.
	  packedData = (int *)cdsPciModules.pci_adc[jj];
	  // Write a dummy 0 to first ADC channel location
	  // This location should never be zero when the ADC writes data as it should always
	  // have an upper bit set indicating channel 0.
          *packedData = 0x0;
          if (cdsPciModules.adcType[jj] == GSC_18AISS6C)  packedData += 5;
          else packedData += 31;
	  // Write a number into the last channel which the ADC should never write ie no
	  // upper bits should be set in channel 31.
          *packedData = DUMMY_ADC_VAL;
	  // Set ADC Present Flag
  	  pLocalEpics->epicsOutput.statAdc[jj] = 1;
	  adcRdTimeErr[jj] = 0;
  }
  printf("ADC setup complete \n");
#endif
#ifdef ADC_SLAVE
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
  	  pLocalEpics->epicsOutput.statAdc[jj] = 1;
  }
#endif

#ifdef ADC_MASTER
  /// \> If IOP, Initialize the DAC module variables
  for(jj = 0; jj < cdsPciModules.dacCount; jj++) {
  	pLocalEpics->epicsOutput.statDac[jj] = DAC_FOUND_BIT;
        pDacData = (unsigned int *) cdsPciModules.pci_dac[jj];
	// Arm DAC DMA for full data size
	if(cdsPciModules.dacType[jj] == GSC_16AO16) {
		status = gsc16ao16DmaSetup(jj);
	} else if (cdsPciModules.dacType[jj] == GSC_20AO8){
		status = gsc20ao8DmaSetup(jj);
	} else {
		status = gsc18ao8DmaSetup(jj);
	}
  }
  printf("DAC setup complete \n");

  

/// \> If IOP, find the code syncrhonization source. \n
/// - Standard aLIGO Sync source is the Timing Distribution System (TDS) (SYNC_SRC_TDS). 
  if (!run_on_timer) {
  switch(syncSource)
  {
	/// \>\> For SYNC_SRC_TDS, initialize system for synchronous start on 1PPS mark:
	case SYNC_SRC_TDS:
		/// - ---- Turn off TDS slave unit timing clocks, in turn removing clocks from ADC/DAC modules.
		for(ii=0;ii<tdsCount;ii++)
		{
		CDIO1616Output[ii] = TDS_STOP_CLOCKS;
		CDIO1616Input[ii] = contec1616WriteOutputRegister(&cdsPciModules, tdsControl[ii], CDIO1616Output[ii]);
		printf("writing BIO %d\n",tdsControl[ii]);
		}
		udelay(MAX_UDELAY);
		udelay(MAX_UDELAY);
		/// - ---- Arm ADC modules
    		gsc16ai64Enable(cdsPciModules.adcCount);
		/// - ----  Arm DAC outputs
		gsc18ao8Enable(&cdsPciModules);
		gsc20ao8Enable(&cdsPciModules);
		gsc16ao16Enable(&cdsPciModules);
		// Set synched flag so later code will not check for 1PPS
		sync21pps = 1;
		udelay(MAX_UDELAY);
		udelay(MAX_UDELAY);
		/// - ---- Preload DAC FIFOS\n
		/// - --------- Code runs intrinsically slower first few cycle after startup, so new DAC
		/// values not written until a few cycle into run. \n
		/// - --------- DAC timing diags will later check FIFO sizes to verify synchrounous timing.
		for(jj=0;jj<cdsPciModules.dacCount;jj++)
		{       
			if(cdsPciModules.dacType[jj] == GSC_18AO8)
			{       
				dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
				for(ii=0;ii<GSAO_18BIT_PRELOAD;ii++) dac18bitPtr->OUTPUT_BUF = 0;
			} else if(cdsPciModules.dacType[jj] == GSC_20AO8) {       
				dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
				for(ii=0;ii<GSAO_20BIT_PRELOAD;ii++) dac20bitPtr->OUTPUT_BUF = 0;
			}else{  
				dac16bitPtr = dacPtr[jj];
				for(ii=0;ii<GSAO_16BIT_PRELOAD;ii++) dac16bitPtr->ODB = 0;
			}       
		}       
		/// - ---- Start the timing clocks\n
		/// - --------- Send start command to TDS slave.\n
		/// - --------- TDS slave will begin sending 64KHz clocks synchronous to next 1PPS mark.
		// CDIO1616Output[tdsControl] = 0x7B00000;
		for(ii=0;ii<tdsCount;ii++)
		{
		// CDIO1616Output[ii] = TDS_START_ADC_NEG_DAC_POS;
		CDIO1616Output[ii] = TDS_START_ADC_NEG_DAC_POS | TDS_NO_DAC_DUOTONE;
		CDIO1616Input[ii] = contec1616WriteOutputRegister(&cdsPciModules, tdsControl[ii], CDIO1616Output[ii]);
		}
		break;
	case SYNC_SRC_1PPS:
                printf("Sync src is 1PPS\n");
#ifndef NO_DAC_PRELOAD
		for(jj=0;jj<cdsPciModules.dacCount;jj++)
		{       
			if(cdsPciModules.dacType[jj] == GSC_18AO8)
			{       
				dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
				for(ii=0;ii<GSAO_18BIT_PRELOAD;ii++) dac18bitPtr->OUTPUT_BUF = 0;
			} else if(cdsPciModules.dacType[jj] == GSC_20AO8) {       
				dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
				for(ii=0;ii<GSAO_20BIT_PRELOAD;ii++) dac20bitPtr->OUTPUT_BUF = 0;
			}else{  
				dac16bitPtr = dacPtr[jj];
				printf("writing DAC %d\n",jj);
				for(ii=0;ii<GSAO_16BIT_PRELOAD;ii++) dac16bitPtr->ODB = 0;
			}       
		}       
#endif
		// Arm ADC modules
		// This has to be done sequentially, one at a time.
                kk = 0;
                for(jj=0;jj<cdsPciModules.adcCount;jj++)
                {
                    packedData = (int *)cdsPciModules.pci_adc[0];
                    packedData += 31;
                    gsc16ai64Enable1PPS(jj);
                    rdtscl(cpuClock[jj]);
                    status = gsc16ai64WaitDmaDone(0, packedData);
                    kk ++;
                    udelay(2);
                    for(ii=0;ii<kk;ii++) {
                          gsc16ai64DmaEnable(ii);
                    }
                }
                // Need to do some dummy reads here to allow time for last ADC to arm
                // as it takes two clock cycles past arm to actually deliver data.
                for(ii=0;ii<cdsPciModules.adcCount;ii++)
                {
                    // Want to verify ADC FIFOs are empty to ensure they are in sync.
                    status = gsc16ai64WaitDmaDone(0, packedData);
                    cpuClock[ii] = gsc16ai64CheckAdcBuffer(ii);
                    for(jj=0;jj<cdsPciModules.adcCount;jj++)
                    {
                         gsc16ai64DmaEnable(jj);
                    }
                }
                // Print out the FIFO info to dmesg to verify sync.
                for(jj=0;jj<cdsPciModules.adcCount;jj++) {
                    printf("ADC buffer %d = %d\n",jj,cpuClock[jj]);
                }
		break;
	default: {
		    // IRIG-B card not found, so use CPU time to get close to 1PPS on startup
			// Pause until this second ends
		break;
	}
  }
  }


  if (run_on_timer) {	// NOT normal operating mode; used for test systems without I/O cards/chassis
    printf("*******************************\n");
    printf("*     Running on timer!       *\n");
    printf("*******************************\n");
  } else {
    printf("Triggered the ADC\n");
  }
#endif
#ifdef ADC_SLAVE
	// SLAVE needs to sync with MASTER by looking for cycle 0 count in ipc memory
	// Find memory buffer of first ADC to be used in SLAVE application.
        ll = cdsPciModules.adcConfig[0];
        printf("waiting to sync %d\n",ioMemData->iodata[ll][0].cycle);
        rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

 	if (boot_cpu_has(X86_FEATURE_MWAIT)) {
	  for(;;) {
	    if (ioMemData->iodata[ll][0].cycle == 0) break;
	    __monitor((void *)&(ioMemData->iodata[ll][0].cycle), 0, 0);
	    if (ioMemData->iodata[ll][0].cycle == 0) break;
	    __mwait(0, 0);
	  }
 	} else {
	  // Spin until cycle 0 detected in first ADC buffer location.
          do {
		udelay(1);
          } while(ioMemData->iodata[ll][0].cycle != 0);
	}
        timeSec = ioMemData->iodata[ll][0].timeSec;

        rdtscl(cpuClock[CPU_TIME_CYCLE_END]);
        cycleTime = (cpuClock[CPU_TIME_CYCLE_END] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
        printf("Synched %d\n",cycleTime);
	// Need to set sync21pps so code will not try to sync with 1pps pulse later.
	sync21pps=1;
	// Get GPS seconds from MASTER
	timeSec = ioMemData->gpsSecond;
        pLocalEpics->epicsOutput.timeDiag = timeSec;
	// Decrement GPS seconds as it will be incremented on first read cycle.
	timeSec --;

#endif
  onePpsTime = cycleNum;
#ifdef REMOTE_GPS
  timeSec = remote_time((struct CDS_EPICS *)pLocalEpics);
  printf ("Using remote GPS time %d \n",timeSec);
#else
  timeSec = current_time() -1;
#endif

  rdtscl(adcTime);

  /// ******************************************************************************\n
  /// Enter the infinite FE control loop  ******************************************\n

  /// ******************************************************************************\n
  // Calculate how many CPU cycles per code cycle
  cpc = cpu_khz * 1000;
  cpc /= CYCLE_PER_SECOND;


  while(!vmeDone){ 	// Run forever until user hits reset
  	if (run_on_timer) {  // NO ADC present, so run on CPU realtime clock
	  // Pause until next cycle begins
	  if (cycleNum == 0) {
	    	//printf("awgtpman gps = %d local = %d\n", pEpicsComms->padSpace.awgtpman_gps, timeSec);
	  	pLocalEpics->epicsOutput.awgStat = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
	  }
	  // This is local CPU timer (no ADCs)
	  // advance to the next cycle polling CPU cycles and microsleeping
	  rdtscl(clk);
	  clk += cpc;
	  for(;;) {
	  	rdtscl(clk1);
		if (clk1 >= clk) break;
		udelay(1);
	  }
	    ioMemCntr = (cycleNum % IO_MEMORY_SLOTS);
	    for(ii=0;ii<IO_MEMORY_SLOT_VALS;ii++)
	    {
		ioMemData->iodata[0][ioMemCntr].data[ii] = cycleNum/4;
		ioMemData->iodata[1][ioMemCntr].data[ii] = cycleNum/4;
	    }
	    // Write GPS time and cycle count as indicator to slave that adc data is ready
	    ioMemData->gpsSecond = timeSec;
	    ioMemData->iodata[0][ioMemCntr].timeSec = timeSec;;
	    ioMemData->iodata[1][ioMemCntr].timeSec = timeSec;;
	    ioMemData->iodata[0][ioMemCntr].cycle = cycleNum;
	    ioMemData->iodata[1][ioMemCntr].cycle = cycleNum;
	  rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

         if(cycleNum == 0) {

	  // Increment GPS second on cycle 0
          timeSec ++;
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	  }
	} else {
	// **********************************************************************************************************
	// NORMAL OPERATION -- Wait for ADC data ready
	// On startup, only want to read one sample such that first cycle
	// coincides with GPS 1PPS. Thereafter, sampleCount will be 
	// increased to appropriate number of 65536 s/sec to match desired
	// code rate eg 32 samples each time thru before proceeding to match 2048 system.
	// **********************************************************************************************************
#ifdef ADC_MASTER
#ifndef RFM_DIRECT_READ
/// \> If IOP and RFM DMA selected, block transfer data from GeFanuc RFM cards.
// Used in block transfers of data from GEFANUC RFM
/// - ---- Want to start the DMA ASAP, before ADC data starts coming in.
/// - ----  Note that data only xferred every 4th cycle of IOP, so max data rate on RFM is 16K.
	if((cycleNum % 4) == 0)
	{
		if (cdsPciModules.pci_rfm[0]) vmic5565DMA(&cdsPciModules,0,(cycleNum % IPC_BLOCKS));
		if (cdsPciModules.pci_rfm[1]) vmic5565DMA(&cdsPciModules,1,(cycleNum % IPC_BLOCKS));
	}
#endif
#endif

/// \> On 1PPS mark \n
       if(cycleNum == 0)
        {
	  /// - ---- Check awgtpman status.
	  //printf("awgtpman gps = %d local = %d\n", pEpicsComms->padSpace.awgtpman_gps, timeSec);
	  pLocalEpics->epicsOutput.awgStat = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
	  if(pLocalEpics->epicsOutput.awgStat) feStatus |= FE_ERROR_AWG;
	  /// - ---- Check if DAC outputs are enabled, report error.
	  if(!iopDacEnable || dkiTrip) feStatus |= FE_ERROR_DAC_ENABLE;

#ifdef ADC_MASTER
	  /// - ---- If IOP, Increment GPS second
          timeSec ++;
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	  if (cycle_gps_time == 0) startGpsTime = timeSec;
	  cycle_gps_time = timeSec;
#endif
	}
        for(ll=0;ll<sampleCount;ll++)
        {
/// IF IOP *************************** \n
#ifdef ADC_MASTER
// Start of ADC Read *************************************************************************************
	/// \> IF IOP, Wait for ADC data ready
	/// - ---- On startup, only want to read one sample such that first cycle
	/// coincides with GPS 1PPS. Thereafter, sampleCount will be 
	/// increased to appropriate number of 65536 s/sec to match desired
	/// code rate eg 32 samples each time thru before proceeding to match 2048 system.
		// Read ADC data
               for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
		    /// - ---- ADC DATA RDY is detected when last channel in memory no longer contains the
		    /// dummy variable written during initialization and reset after the read.
		    packedData = (int *)cdsPciModules.pci_adc[jj];
                    if (cdsPciModules.adcType[jj] == GSC_18AISS6C) packedData += 5;
               	    else packedData += 31;
		
		    rdtscl(cpuClock[CPU_TIME_RDY_ADC]);

                    do {
			/// - ---- Need to delay if not ready as constant banging of the input register
			/// will slow down the ADC DMA.
			// if(*packedData == DUMMY_ADC_VAL) {
		    		rdtscl(cpuClock[CPU_TIME_ADC_WAIT]);
				adcWait = (cpuClock[CPU_TIME_ADC_WAIT] - cpuClock[CPU_TIME_RDY_ADC])/CPURATE;
			// }
			/// - ---- Allow 1sec for data to be ready (should never take that long).
                    }while((*packedData == DUMMY_ADC_VAL) && (adcWait < MAX_ADC_WAIT));

			/// - ---- Added ADC timing diagnostics to verify timing consistent and all rdy together.
		    if(jj==0)
			    adcRdTime[jj] = (cpuClock[CPU_TIME_ADC_WAIT] - cpuClock[CPU_TIME_CYCLE_START]) / CPURATE;
		    else
			    adcRdTime[jj] = adcWait;
	
		    if(adcRdTime[jj] > adcRdTimeMax[jj]) adcRdTimeMax[jj] = adcRdTime[jj];

		    if((jj==0) && (adcRdTimeMax[jj] > MAX_ADC_WAIT_CARD_0)) 
			adcRdTimeErr[jj] ++;
		    if((jj!=0) && (adcRdTimeMax[jj] > MAX_ADC_WAIT_CARD_S)) 
			adcRdTimeErr[jj] ++;

		    /// - --------- If data not ready in time, abort.
		    /// Either the clock is missing or code is running too slow and ADC FIFO
		    /// is overflowing.
		    if (adcWait >= MAX_ADC_WAIT) {
	  		pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
	  		pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
                        stop_working_threads = 1;
			vmeDone = 1;
                        printf("timeout %d %d \n",jj,adcWait);
			continue;
		    }
		    if(jj == 0) 
		    {
			// Capture cpu clock for cpu meter diagnostics
			rdtscl(cpuClock[CPU_TIME_CYCLE_START]);
			/// \> If first cycle of a new second, capture IRIG-B time. Standard for aLIGO is
			/// TSYNC_RCVR.
			if(cycleNum == 0) 
			{
				// if SymCom type, just do write to lock current time and read later
				// This save a couple three microseconds here
				if(cdsPciModules.gpsType == SYMCOM_RCVR) lockGpsTime();
				if(cdsPciModules.gpsType == TSYNC_RCVR) 
				{
					/// - ---- Reading second info will lock the time register, allowing
					/// nanoseconds to be read later (on next cycle). Two step process used to 
					/// save CPU time here, as each read can take 2usec or more.
					timeSec = getGpsSecTsync();
				}
			}
			
		    }

#ifndef RFM_DIRECT_READ
#ifdef FUTURE_RFM_DMA_CHECK
                    /// \> If RFM cards, verify DMA is complete
                    if(jj == 0 && (cycleNum % 4) == 0 && cdsPciModules.pci_rfm[0])
                                vmic5565DMAdone(0);
                    if(jj == 0 && (cycleNum % 4) == 0 && cdsPciModules.pci_rfm[1])
                                vmic5565DMAdone(1);
#endif
#endif

                    /// \> Read adc data
                    packedData = (int *)cdsPciModules.pci_adc[jj];
		    /// - ---- First, and only first, channel should have upper bit marker set.
		    /// If not, have a channel hopping error.
                    if(!(*packedData & ADC_1ST_CHAN_MARKER)) 
		    {
  			 adcChanErr[jj] = 1;
			 chanHop = 1;
	  	 	 pLocalEpics->epicsOutput.stateWord |= FE_ERROR_ADC;
	 	    }	

                    limit = OVERFLOW_LIMIT_16BIT;
                    if (cdsPciModules.adcType[jj] == GSC_18AISS6C) {
			limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
			offset = GSAF_DATA_CODE_OFFSET; // Data coding offset in 18-bit DAC
			mask = GSAF_DATA_MASK;
			num_outs = GSAF_CHAN_COUNT;
                    } else {
		    	// Various ADC models have different number of channels/data bits
                    	offset = GSAI_DATA_CODE_OFFSET;
                    	mask = GSAI_DATA_MASK;
                    	num_outs = GSAI_CHAN_COUNT;
		    }
		    /// - ---- Determine next ipc memory location to load ADC data
		    ioMemCntr = (cycleNum % IO_MEMORY_SLOTS);
                    /// - ----  Read adc data from PCI mapped memory into local variables
                    for(ii=0;ii<num_outs;ii++)
                    {
			// adcData is the integer representation of the ADC data
			adcData[jj][ii] = (*packedData & mask);
			adcData[jj][ii]  -= offset;
#ifdef DEC_TEST
			if(ii==0)
			{
				adcData[jj][ii] = dspPtr[0]->data[0].exciteInput;
			}
#endif
			// dWord is the double representation of the ADC data
			// This is the value used by the rest of the code calculations.
			dWord[jj][ii] = adcData[jj][ii];
			/// - ----  Load ADC value into ipc memory buffer
			ioMemData->iodata[jj][ioMemCntr].data[ii] = adcData[jj][ii];
			packedData ++;
                    }
		    /// - ---- Write GPS time and cycle count as indicator to slave that adc data is ready
	  	    ioMemData->gpsSecond = timeSec;;
		    ioMemData->iodata[jj][ioMemCntr].timeSec = timeSec;;
		    ioMemData->iodata[jj][ioMemCntr].cycle = cycleNum;

		    /// - ---- Check for ADC overflows
                    for(ii=0;ii<num_outs;ii++)
                    {
			if((adcData[jj][ii] > limit) || (adcData[jj][ii] < -limit))
			  {
				overflowAdc[jj][ii] ++;
				pLocalEpics->epicsOutput.overflowAdcAcc[jj][ii] ++;
				overflowAcc ++;
				adcOF[jj] = 1;
				odcStateWord |= ODC_ADC_OVF;
			  }
                    }

		   /// - ---- Clear out last ADC data read for test on next cycle
                   packedData = (int *)cdsPciModules.pci_adc[jj];
                   *packedData = 0x0;

                   if (cdsPciModules.adcType[jj] == GSC_18AISS6C) packedData += GSAF_CHAN_COUNT_M1;
                   else packedData += GSAI_CHAN_COUNT_M1;

                   *packedData = DUMMY_ADC_VAL;

#ifdef DIAG_TEST
// For DIAGS ONLY !!!!!!!!
// This will change ADC DMA BYTE count
// -- Greater than normal will result in channel hopping.
// -- Less than normal will result in ADC timeout.
// In both cases, real-time kernel code should exit with errors to dmesg
          	   if(pLocalEpics->epicsInput.bumpAdcRd != 0) {
		   	gsc16ai64DmaBump(jj,pLocalEpics->epicsInput.bumpAdcRd);
		   	pLocalEpics->epicsInput.bumpAdcRd = 0;
		   }
#endif
		   /// - ---- Reset ADC DMA Start Flag \n
		   /// - --------- This allows ADC to dump next data set whenever it is ready
		   gsc16ai64DmaEnable(jj);
            }


		  // Try synching to 1PPS on ADC[0][31] if not using IRIG-B or TDS
		  // Only try for 1 sec.
                  if(!sync21pps)
                  {
			// 1PPS signal should rise above 4000 ADC counts if present.
                        if((adcData[0][31] < ONE_PPS_THRESH) && (sync21ppsCycles < (CYCLE_PER_SECOND*OVERSAMPLE_TIMES))) 
			{
				ll = -1;
				sync21ppsCycles ++;
                        }else {
				// Need to start clocking the DAC outputs.
				gsc18ao8Enable(&cdsPciModules);
				gsc20ao8Enable(&cdsPciModules);
				gsc16ao16Enable(&cdsPciModules);
                                sync21pps = 1;
				// 1PPS never found, so indicate NO SYNC to user
				if(sync21ppsCycles >= (CYCLE_PER_SECOND*OVERSAMPLE_TIMES))
				{
					syncSource = SYNC_SRC_NONE;
					printf("NO SYNC SOURCE FOUND %d %d\n",sync21ppsCycles,adcData[0][31]);
				} else {
				// 1PPS found and synched to
					printf("GPS Trigg %d %d\n",adcData[0][31],sync21pps);
					syncSource = SYNC_SRC_1PPS;
				}
				pLocalEpics->epicsOutput.timeErr = syncSource;
                        }
                }
#endif
/// END IF IOP \n

/// IF USER APP  *************************** \n
#ifdef ADC_SLAVE
		/// \> SLAVE gets its adc data from MASTER via ipc shared memory\n
		/// \> For each ADC defined:
               for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
        		mm = cdsPciModules.adcConfig[jj];
                        kk = 0;
		    		rdtscl(cpuClock[CPU_TIME_RDY_ADC]);
			/// - ---- Wait for proper timestamp in shared memory, indicating data ready.
                        do{
                                // usleep(1);
		    		rdtscl(cpuClock[CPU_TIME_ADC_WAIT]);
				adcWait = (cpuClock[CPU_TIME_ADC_WAIT] - cpuClock[CPU_TIME_RDY_ADC])/CPURATE;
                                // kk++;
                        }while((ioMemData->iodata[mm][ioMemCntr].cycle != ioClock) && (adcWait < MAX_ADC_WAIT_SLAVE));
			timeSec = ioMemData->iodata[mm][ioMemCntr].timeSec;
			if (cycle_gps_time == 0) startGpsTime = timeSec;
			cycle_gps_time = timeSec;

			/// - --------- If data not ready in time, set error, release DAC channel reservation and exit the code.
                        if(adcWait >= MAX_ADC_WAIT_SLAVE)
                        {
                                printf ("ADC TIMEOUT %d %d %d %d\n",mm,ioMemData->iodata[mm][ioMemCntr].cycle, ioMemCntr,ioClock);
	  			pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
	  			pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
				deallocate_dac_channels();
  				return (void *)-1;
                        }
                        for(ii=0;ii<MAX_ADC_CHN_PER_MOD;ii++)
                        {
				/// - ---- Read data from shared memory.
                                adcData[jj][ii] = ioMemData->iodata[mm][ioMemCntr].data[ii];
#ifdef FLIP_SIGNALS
                                adcData[jj][ii] *= -1;
#endif
                                dWord[jj][ii] = adcData[jj][ii];
#ifdef OVERSAMPLE
				/// - ---- Downsample ADC data from 64K to rate of user application
				if (dWordUsed[jj][ii]) {
#ifdef CORE_BIQUAD
					dWord[jj][ii] = iir_filter_biquad(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
#else
					dWord[jj][ii] = iir_filter(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
#endif
				}
			/// - ---- Check for ADC data over/under range
			if((adcData[jj][ii] > limit) || (adcData[jj][ii] < -limit))
			  {
				overflowAdc[jj][ii] ++;
				pLocalEpics->epicsOutput.overflowAdcAcc[jj][ii] ++;
				overflowAcc ++;
				adcOF[jj] = 1;
				odcStateWord |= ODC_ADC_OVF;
			  }
#endif
                                // No filter  dWord[kk][ll] = adcData[kk][ll];
                        }
                }
		/// \> Set counters for next read from ipc memory
                ioClock = (ioClock + 1) % IOP_IO_RATE;
                ioMemCntr = (ioMemCntr + 1) % IO_MEMORY_SLOTS;
        	rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

#endif
	}

	// After first synced ADC read, must set to code to read number samples/cycle
        sampleCount = OVERSAMPLE_TIMES;
	}
/// End of ADC Read **************************************************************************************


/// \> Get EPICS setpoint data from /proc if any new values are set. 
/// - ---- This feature was added in RCG V2.7. It allows for synchronous input to EPICS setpoint variables by
/// user scripts or other code on same computer.
///
	// Process future setpoints
	for (jj = 0; jj < MAX_PROC_FUTURES; jj++) {
		if (proc_futures[jj].proc_epics) {
			if (proc_futures[jj].gps <=  cycle_gps_time
				&& proc_futures[jj].cycle <= cycleNum) {
				// It is time to execute the setpoint
				switch (proc_futures[jj].proc_epics->type) {
				  case 0: /* int */
					*((int *)(((void *)pLocalEpics) + proc_futures[jj].proc_epics->idx)) = proc_futures[jj].val;
					break;
				  case 1: /* double */
					((double *)(((void *)pLocalEpics) + proc_futures[jj].proc_epics->idx))[proc_futures[jj].idx] = proc_futures[jj].val;
					break;
				}
				// Invalidate this setpoint: mark as processed
				proc_futures[jj].proc_epics = 0;
			}
		}
	}

/// \> Call the front end specific application  ******************\n
/// - -- This is where the user application produced by RCG gets called and executed. \n\n
        rdtscl(cpuClock[CPU_TIME_USR_START]);
 	iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0],(struct CDS_EPICS *)pLocalEpics,0);
        rdtscl(cpuClock[CPU_TIME_USR_END]);

  	odcStateWord = 0;
/// WRITE DAC OUTPUTS ***************************************** \n

/// Writing of DAC outputs is dependent on code compile option: \n
/// - -- IOP (ADC_MASTER) reads DAC output values from memory shared with user apps and writes to DAC hardware. \n
/// - -- USER APP (ADC_SLAVE) sends output values to memory shared with IOP. \n

/// START OF IOP DAC WRITE ***************************************** \n
#ifdef ADC_MASTER
        /// \> If DAC FIFO error, always output zero to DAC modules. \n
        /// - -- Code will require restart to clear.
        // COMMENT OUT NEX LINE FOR TEST STAND w/bad DAC cards. 
#ifndef DAC_WD_OVERRIDE
        if(dacTimingError) iopDacEnable = 0;
#endif
        // Write out data to DAC modules
	dkiTrip = 0;
	/// \> Loop thru all DAC modules
        for(jj=0;jj<cdsPciModules.dacCount;jj++)
        {
           	/// - -- Point to DAC memory buffer
           	pDacData = (unsigned int *)(cdsPciModules.pci_dac[jj]);
        	/// - -- locate the proper DAC memory block
        	mm = cdsPciModules.dacConfig[jj];
        	/// - -- Determine if memory block has been set with the correct cycle count by Slave app.
		if(ioMemData->iodata[mm][ioMemCntrDac].cycle == ioClockDac)
		{
			dacEnable |= pBits[jj];
		}else {
			dacEnable &= ~(pBits[jj]);
			dacChanErr[jj] += 1;
		}
		/// - -- Set overflow limits, data mask, and chan count based on DAC type
                limit = OVERFLOW_LIMIT_16BIT;
                mask = GSAO_16BIT_MASK;
                num_outs = GSAO_16BIT_CHAN_COUNT;
                if (cdsPciModules.dacType[jj] == GSC_18AO8) {
                        limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
                        mask = GSAO_18BIT_MASK;
                        num_outs = GSAO_18BIT_CHAN_COUNT;
                }
		if (cdsPciModules.dacType[jj] == GSC_20AO8) {
                        limit = OVERFLOW_LIMIT_20BIT; // 20 bit limit
                        mask = GSAO_20BIT_MASK;
                        num_outs = GSAO_20BIT_CHAN_COUNT;
                }
		/// - -- For each DAC channel
                for (ii=0; ii < num_outs; ii++)
                {
#ifdef FLIP_SIGNALS
                        dacOut[jj][ii] *= -1;
#endif
			/// - ---- Read DAC output value from shared memory and reset memory to zero
                        if((!dacChanErr[jj]) && (iopDacEnable)) {
                                dac_out = ioMemData->iodata[mm][ioMemCntrDac].data[ii];
                                /// - --------- Zero out data in case user app dies by next cycle
                                /// when two or more apps share same DAC module.
                                ioMemData->iodata[mm][ioMemCntrDac].data[ii] = 0;
                        } else {
				dac_out = 0;
				dkiTrip = 1;
			}
                        /// - ----  Write out ADC duotone signal to first DAC, last channel, 
			/// if DAC duotone is enabled.
                        if((dacDuoEnable) && (ii==(num_outs-1)) && (jj == 0))
                        {
                                dac_out = adcData[0][ADC_DUOTONE_CHAN];
                        }
// Code below is only for use in DAQ test system.
#ifdef DIAG_TEST
                        if((ii==0) && (jj == 0))
                        {
                                if(cycleNum < 100) dac_out = limit / 20;
                                else dac_out = 0;
                        }
                        if((ii==0) && (jj == 6))
                        {
                                if(cycleNum < 100) dac_out = limit / 20;
                                else dac_out = 0;
                        }
#endif
                        /// - ---- Check output values are within range of DAC \n
                        /// - --------- If overflow, clip at DAC limits and report errors
                        if(dac_out > limit || dac_out < -limit)
                        {
                                overflowDac[jj][ii] ++;
				pLocalEpics->epicsOutput.overflowDacAcc[jj][ii] ++;
                                overflowAcc ++;
                                dacOF[jj] = 1;
				odcStateWord |= ODC_DAC_OVF;;
				if(dac_out > limit) dac_out = limit;
				else dac_out = -limit;
                        }
                        /// - ---- If DAQKILL tripped, set output to zero.
                        if(!iopDacEnable) dac_out = 0;
                        /// - ---- Load last values to EPICS channels for monitoring on GDS_TP screen.
                        dacOutEpics[jj][ii] = dac_out;

                        /// - ---- Load DAC testpoints
                        floatDacOut[16*jj + ii] = dac_out;

                        /// - ---- Write to DAC local memory area, for later xmit to DAC module
                        *pDacData =  (unsigned int)(dac_out & mask);
                        pDacData ++;
		}
                /// - -- Mark cycle count as having been used -1 \n
                /// - --------- Forces slaves to mark this cycle or will not be used again by Master
                ioMemData->iodata[mm][ioMemCntrDac].cycle = -1;
                /// - -- DMA Write data to DAC module
                if(dacWriteEnable > 4) {
                        if(cdsPciModules.dacType[jj] == GSC_16AO16) {
                                gsc16ao16DmaStart(jj);
                        } else if(cdsPciModules.dacType[jj] == GSC_20AO8) {
                                gsc20ao8DmaStart(jj);
                        } else {
                                gsc18ao8DmaStart(jj);
                        }
                }
	}
        /// \> Increment DAC memory block pointers for next cycle
        ioClockDac = (ioClockDac + 1) % IOP_IO_RATE;
        ioMemCntrDac = (ioMemCntrDac + 1) % IO_MEMORY_SLOTS;
        if(dacWriteEnable < 10) dacWriteEnable ++;
/// END OF IOP DAC WRITE *************************************************

#endif
#ifdef ADC_SLAVE
/// START OF USER APP DAC WRITE *****************************************
        // Write out data to DAC modules
	/// \> Loop thru all DAC modules
        for(jj=0;jj<cdsPciModules.dacCount;jj++)
        {
		/// - -- locate the proper DAC memory block
           	mm = cdsPciModules.dacConfig[jj];
		/// - -- Set overflow limits, data mask, and chan count based on DAC type
                limit = OVERFLOW_LIMIT_16BIT;
                mask = GSAO_16BIT_MASK;
                num_outs = GSAO_16BIT_CHAN_COUNT;
                if (cdsPciModules.dacType[jj] == GSC_18AO8) {
                        limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
                        mask = GSAO_18BIT_MASK;
                        num_outs = GSAO_18BIT_CHAN_COUNT;
                }
		if (cdsPciModules.dacType[jj] == GSC_20AO8) {
                        limit = OVERFLOW_LIMIT_20BIT; // 20 bit limit
                        mask = GSAO_20BIT_MASK;
                        num_outs = GSAO_20BIT_CHAN_COUNT;
                }
		/// - -- If user app < 64k rate (typical), need to upsample from code rate to IOP rate
           	for (kk=0; kk < OVERSAMPLE_TIMES; kk++) {
		     /// - -- For each DAC channel
                    for (ii=0; ii < num_outs; ii++)
                    {
#ifdef FLIP_SIGNALS
                        dacOut[jj][ii] *= -1;
#endif
			/// - ---- Read in DAC value, if channel is used by the application,  and run thru upsample filter\n
                        if (dacOutUsed[jj][ii]) {
#ifdef OVERSAMPLE_DAC
#ifdef NO_ZERO_PAD
				/// - --------- If set to NO_ZERO_PAD (not standard setting), then DAC output value is 
				/// repeatedly run thru the filter OVERSAMPE_TIMES.\n
                          	dac_in = dacOut[jj][ii];
#ifdef CORE_BIQUAD
                          	dac_in = iir_filter_biquad(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#else
                          	dac_in = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#endif
#else
				/// - --------- Otherwise zero padding is used (standard), 
				/// ie DAC output value is applied to filter on first
				/// interation, thereafter zeros are applied.
                          	dac_in =  kk == 0? (double)dacOut[jj][ii]: 0.0;
#ifdef CORE_BIQUAD
                          	dac_in = iir_filter_biquad(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#else
                          	dac_in = iir_filter(dac_in,FE_OVERSAMPLE_COEFF,2,&dDacHistory[ii+jj*MAX_DAC_CHN_PER_MOD][0]);
#endif
                          	dac_in *= OVERSAMPLE_TIMES;
#endif
#else
                          	dac_in = dacOut[jj][ii];
#endif
                        }
			/// - ---- If channel is not used, then set value to zero.
                        else 
				dac_in = 0.0;
                        /// - ---- Smooth out some of the double > short roundoff errors
                        if(dac_in > 0.0) dac_in += 0.5;
                        else dac_in -= 0.5;
                        dac_out = dac_in;

                        /// - ---- Check output values are within range of DAC \n
                        /// - --------- If overflow, clip at DAC limits and report errors
                        if(dac_out > limit || dac_out < -limit)
                        {
                                overflowDac[jj][ii] ++;
				pLocalEpics->epicsOutput.overflowDacAcc[jj][ii] ++;
                                overflowAcc ++;
                                dacOF[jj] = 1;
				odcStateWord |= ODC_DAC_OVF;;
				if(dac_out > limit) dac_out = limit;
				else dac_out = -limit;
                        }
			/// - ---- If DAQKILL tripped, set output to zero.
                        if(!iopDacEnable) dac_out = 0;
			/// - ---- Load last values to EPICS channels for monitoring on GDS_TP screen.
                        dacOutEpics[jj][ii] = dac_out;

                        /// - ---- Load DAC testpoints
                        floatDacOut[16*jj + ii] = dac_out;
                        /// - ---- Determine shared memory location for new DAC output data 
                        memCtr = (ioMemCntrDac + kk) % IO_MEMORY_SLOTS;
			/// - ---- Write DAC output to shared memory. \n
                        /// - --------- Only write to DAC channels being used to allow two or more
                        /// slaves to write to same DAC module.
                        if (dacOutUsed[jj][ii])
                                ioMemData->iodata[mm][memCtr].data[ii] = dac_out;
		    }
                /// - ---- Write cycle count to make DAC data complete
                if(iopDacEnable)
                ioMemData->iodata[mm][memCtr].cycle = (ioClockDac + kk) % IOP_IO_RATE;
           }
	}
        /// \> Increment DAC memory block pointers for next cycle
        ioClockDac = (ioClockDac + OVERSAMPLE_TIMES) % IOP_IO_RATE;
        ioMemCntrDac = (ioMemCntrDac  + OVERSAMPLE_TIMES) % IO_MEMORY_SLOTS;
        if(dacWriteEnable < 10) dacWriteEnable ++;
/// END OF USER APP DAC WRITE *************************************************

#endif
/// END OF DAC WRITE *************************************************


/// BEGIN HOUSEKEEPING ************************************************ \n

        pLocalEpics->epicsOutput.cycle = cycleNum;
#ifdef ADC_MASTER
// The following, to endif, is all duotone timing diagnostics.
/// \> Cycle 0: \n
/// - ---- Read IRIGB time if symmetricom card (this is not standard, but supported for earlier cards. \n
        if(cycleNum == HKP_READ_SYMCOM_IRIGB)
        {
		if(cdsPciModules.gpsType == SYMCOM_RCVR) 
		{
		// Retrieve time set in at adc read and report offset from 1PPS
			gps_receiver_locked = getGpsTime(&timeSec,&usec);
			pLocalEpics->epicsOutput.irigbTime = usec;
		}
		if((usec > MAX_IRIGB_SKEW) || (usec < MIN_IRIGB_SKEW)) 
		{
			diagWord |= TIME_ERR_IRIGB;;
			feStatus |= FE_ERROR_TIMING;;
	        }
/// - ---- Calc duotone diagnostic mean values for past second and reset.
                duotoneMean = duotoneTotal/CYCLE_PER_SECOND;
                duotoneTotal = 0.0;
                duotoneMeanDac = duotoneTotalDac/CYCLE_PER_SECOND;
                duotoneTotalDac = 0.0;
        }

/// \> Cycle 1 and Spectricom IRIGB (standard), get IRIG-B time information.
	if(cycleNum == HKP_READ_TSYNC_IRIBB)
	{
	        if(cdsPciModules.gpsType == TSYNC_RCVR)
	        {
	                gps_receiver_locked = getGpsuSecTsync(&usec);
	                pLocalEpics->epicsOutput.irigbTime = usec;
/// - ---- If not in acceptable range, generate error.
	                if((usec > MAX_IRIGB_SKEW) || (usec < MIN_IRIGB_SKEW)) 
			{
				feStatus |= FE_ERROR_TIMING;;
				diagWord |= TIME_ERR_IRIGB;;
			}
	        }
        }

/// \> Update duotone diag information
        duotoneDac[(cycleNum + DT_SAMPLE_OFFSET) % CYCLE_PER_SECOND] = dWord[ADC_DUOTONE_BRD][DAC_DUOTONE_CHAN];
        duotoneTotalDac += dWord[ADC_DUOTONE_BRD][DAC_DUOTONE_CHAN];
        duotone[(cycleNum + DT_SAMPLE_OFFSET) % CYCLE_PER_SECOND] = dWord[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];
        duotoneTotal += dWord[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];

/// \> Cycle 16, perform duotone diag calcs.
        if(cycleNum == HKP_DT_CALC)
        {
		duotoneTime = duotime(DT_SAMPLE_CNT, duotoneMean, duotone);
		pLocalEpics->epicsOutput.dtTime = duotoneTime;
		if(((duotoneTime < MIN_DT_DIAG_VAL) || (duotoneTime > MAX_DT_DIAG_VAL)) &&
		   syncSource != SYNC_SRC_1PPS)
			feStatus |= FE_ERROR_TIMING;
		duotoneTimeDac = duotime(DT_SAMPLE_CNT, duotoneMeanDac, duotoneDac);
                pLocalEpics->epicsOutput.dacDtTime = duotoneTimeDac;
        }

/// \> Cycle 17, set/reset DAC duotone switch if request has changed.
	if(cycleNum == HKP_DAC_DT_SWITCH)
	{
		if(dacDuoEnable != pLocalEpics->epicsInput.dacDuoSet)
		{
			dacDuoEnable = pLocalEpics->epicsInput.dacDuoSet;
			// printf("duo set = %d\n",dacDuoEnable);
			if(dacDuoEnable)
				CDIO1616Output[0] = TDS_START_ADC_NEG_DAC_POS;
			else CDIO1616Output[0] = TDS_START_ADC_NEG_DAC_POS | TDS_NO_DAC_DUOTONE;
			CDIO1616Input[0] = contec1616WriteOutputRegister(&cdsPciModules, tdsControl[0], CDIO1616Output[0]);
		}
	}
#endif


/// \> Cycle 18, Send timing info to EPICS at 1Hz
	if(cycleNum ==HKP_TIMING_UPDATES)	
        {
	  pLocalEpics->epicsOutput.cpuMeter = timeHold;
	  pLocalEpics->epicsOutput.cpuMeterMax = timeHoldMax;
#ifdef ADC_MASTER
  	  pLocalEpics->epicsOutput.dacEnable = dacEnable;
#endif
          timeHoldHold = timeHold;
          timeHold = 0;
	  timeHoldWhenHold = timeHoldWhen;

#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
	  memcpy(cycleHistMax, cycleHist, sizeof(cycleHist));
	  memset(cycleHist, 0, sizeof(cycleHist));
	  memcpy(cycleHistWhenHold, cycleHistWhen, sizeof(cycleHistWhen));
	  memset(cycleHistWhen, 0, sizeof(cycleHistWhen));
#endif
	  if (timeSec % 4 == 0) pLocalEpics->epicsOutput.adcWaitTime = adcHoldTimeMin;
	  else if (timeSec % 4 == 1)
		pLocalEpics->epicsOutput.adcWaitTime =  adcHoldTimeMax;
	  else
	  	pLocalEpics->epicsOutput.adcWaitTime = adcHoldTimeAvg/CYCLE_PER_SECOND;
	  adcHoldTimeAvgPerSec = adcHoldTimeAvg/CYCLE_PER_SECOND;
	  adcHoldTimeMax = 0;
	  adcHoldTimeMin = 0xffff;
	  adcHoldTimeAvg = 0;
	  if((adcHoldTime > CYCLE_TIME_ALRM_HI) || (adcHoldTime < CYCLE_TIME_ALRM_LO)) 
	  {
	  	diagWord |= FE_ADC_HOLD_ERR;
		feStatus |= FE_ERROR_TIMING;
	  
	  }
	  if(timeHoldMax > CYCLE_TIME_ALRM) 
	  {
	  	diagWord |= FE_PROC_TIME_ERR;
		feStatus |= FE_ERROR_TIMING;
	  }
	  pLocalEpics->epicsOutput.stateWord = feStatus;
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
#ifdef ADC_MASTER
               for(jj=0;jj<cdsPciModules.adcCount;jj++) adcRdTimeMax[jj] = 0;
#endif
	  }
	  // Flip the onePPS various once/sec as a watchdog monitor.
	  // pLocalEpics->epicsOutput.onePps ^= 1;
	  pLocalEpics->epicsOutput.diagWord = diagWord;
#ifdef ADC_MASTER
       	  for(jj=0;jj<cdsPciModules.adcCount;jj++) {
		if(adcRdTimeErr[jj] > MAX_ADC_WAIT_ERR_SEC)
			pLocalEpics->epicsOutput.stateWord |= FE_ERROR_ADC;
		adcRdTimeErr[jj] = 0;
	  }
#endif
        }


/// \> Check for requests for filter module clear history requests. This is spread out over a number of cycles.
	// Spread out filter coeff update, but keep updates at 16 Hz
	// here we are rounding up:
	//   x/y rounded up equals (x + y - 1) / y
	//
	{ 
		static const unsigned int mpc = (MAX_MODULES + (FE_RATE / 16) - 1) / (FE_RATE / 16); // Modules per cycle
		unsigned int smpc = mpc * subcycle; // Start module counter
		unsigned int empc = smpc + mpc; // End module counter
		unsigned int i;
		for (i = smpc; i < MAX_MODULES && i < empc ; i++) 
			checkFiltReset(i, dspPtr[0], pDsp[0], &dspCoeff[0], MAX_MODULES, pCoeff[0]);
	}

	/// \> Check if code exit is requested
	if(cycleNum == MAX_MODULES) 
		vmeDone = stop_working_threads | checkEpicsReset(cycleNum, (struct CDS_EPICS *)pLocalEpics);

	// If synced to 1PPS on startup, continue to check that code
	// is still in sync with 1PPS.
	// This is NOT normal aLIGO mode.
	if(syncSource == SYNC_SRC_1PPS)
	{

		// Assign chan 32 to onePps 
		onePps = adcData[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];
		if((onePps > ONE_PPS_THRESH) && (onePpsHi == 0))  
		{
			onePpsTime = cycleNum;
			onePpsHi = 1;
		}
		if(onePps < ONE_PPS_THRESH) onePpsHi = 0;  

		// Check if front end continues to be in sync with 1pps
		// If not, set sync error flag
		if(onePpsTime > 1) pLocalEpics->epicsOutput.timeErr |= TIME_ERR_1PPS;
	}
#ifdef DIAG_TEST
	for(ii=0;ii<10;ii++)
	{
		if(ii<5) onePpsTest = adcData[0][ii];
		else onePpsTest = adcData[1][(ii-5)];
		if((onePpsTest > 400) && (onePpsHiTest[ii] == 0))  
		{
			onePpsTimeTest[ii] = cycleNum;
			onePpsHiTest[ii] = 1;
			if((ii == 0) || (ii == 5)) pLocalEpics->epicsOutput.timingTest[ii] = cycleNum * 15.26;
			// Slaves do not see 1pps until after IOP signal loops around and back into ADC channel 0,
			// therefore, need to subtract IOP loop time.
			else pLocalEpics->epicsOutput.timingTest[ii] = (cycleNum * 15.26) - pLocalEpics->epicsOutput.timingTest[0];
		}
		// Reset the diagnostic for next cycle
		if(cycleNum > 2000) onePpsHiTest[ii] = 0;  
	}
#endif

#ifdef ADC_SLAVE
/// \> Cycle 10 to number of BIO cards:\n
 /// - ---- If User App, Read Dio cards once per second \n
 /// - ---- IOP does not handle binary I/O module traffic, as it can take too long

	if(cdsPciModules.doCount)
        // if((cycleNum < (HKP_READ_DIO + cdsPciModules.doCount)) && (cycleNum >= HKP_READ_DIO))
        {
                // kk = cycleNum - HKP_READ_DIO;
		kk = cycleNum % cdsPciModules.doCount;
                ii = cdsPciModules.doInstance[kk];
                if(cdsPciModules.doType[kk] == ACS_8DIO)
                {
	  		rioInputInput[ii] = accesIiro8ReadInputRegister(&cdsPciModules, kk) & 0xff;
	  		rioInputOutput[ii] = accesIiro8ReadOutputRegister(&cdsPciModules, kk) & 0xff;
                }
                if(cdsPciModules.doType[kk] == ACS_16DIO)
                {
                        rioInput1[ii] = accesIiro16ReadInputRegister(&cdsPciModules, kk) & 0xffff;
                }
		if(cdsPciModules.doType[kk] == ACS_24DIO)
		{
		  dioInput[ii] = accesDio24ReadInputRegister(&cdsPciModules, kk);
		}
		if (cdsPciModules.doType[kk] == CON_6464DIO) {
	 		CDIO6464InputInput[ii] = contec6464ReadInputRegister(&cdsPciModules, kk);
		}
		if (cdsPciModules.doType[kk] == CDI64) {
	 		CDIO6464InputInput[ii] = contec6464ReadInputRegister(&cdsPciModules, kk);
		}
        }
/// \> If user app, write Dio cards only on change
        for(kk=0;kk < cdsPciModules.doCount;kk++)
        {
                ii = cdsPciModules.doInstance[kk];
                if((cdsPciModules.doType[kk] == ACS_8DIO) && (rioOutput[ii] != rioOutputHold[ii]))
                {
                        accesIiro8WriteOutputRegister(&cdsPciModules, kk, rioOutput[ii]);
                        rioOutputHold[ii] = rioOutput[ii];
                } else 
                if((cdsPciModules.doType[kk] == ACS_16DIO) && (rioOutput1[ii] != rioOutputHold1[ii]))
                {
                        accesIiro16WriteOutputRegister(&cdsPciModules, kk, rioOutput1[ii]);
                        rioOutputHold1[ii] = rioOutput1[ii];
                } else 
                if(cdsPciModules.doType[kk] == CON_32DO)
                {
                        if (CDO32Input[ii] != CDO32Output[ii]) {
                          CDO32Input[ii] = contec32WriteOutputRegister(&cdsPciModules, kk, CDO32Output[ii]);
                        }
		} else if (cdsPciModules.doType[kk] == CON_6464DIO) {
			if (CDIO6464LastOutState[ii] != CDIO6464Output[ii]) {
			  CDIO6464LastOutState[ii] = contec6464WriteOutputRegister(&cdsPciModules, kk, CDIO6464Output[ii]);
			}
		} else if (cdsPciModules.doType[kk] == CDO64) {
			if (CDIO6464LastOutState[ii] != CDIO6464Output[ii]) {
			  CDIO6464LastOutState[ii] = contec6464WriteOutputRegister(&cdsPciModules, kk, CDIO6464Output[ii]);
			}
                } else
                if((cdsPciModules.doType[kk] == ACS_24DIO) && (dioOutputHold[ii] != dioOutput[ii]))
		{
                        accesDio24WriteOutputRegister(&cdsPciModules, kk, dioOutput[ii]);
			dioOutputHold[ii] = dioOutput[ii];
		}
        }

#endif

/// \>  Write data to DAQ.
#ifndef NO_DAQ
		
		// Call daqLib
		pLocalEpics->epicsOutput.daqByteCnt = 
			daqWrite(1,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],myGmError2,(int *)(pLocalEpics->epicsOutput.gdsMon),xExc,pEpicsDaq);
		// Send the current DAQ block size to the awgtpman for TP number checking
	  	pEpicsComms->padSpace.feDaqBlockSize = curDaqBlockSize;
	  	pLocalEpics->epicsOutput.tpCnt = tpPtr->count & 0xff;
		feStatus |= (FE_ERROR_EXC_SET & tpPtr->count);
		if (FE_ERROR_EXC_SET & tpPtr->count) odcStateWord |= ODC_EXC_SET;
		else odcStateWord &= ~(ODC_EXC_SET);
		if(pLocalEpics->epicsOutput.daqByteCnt > DAQ_DCU_RATE_WARNING) 
			feStatus |= FE_ERROR_DAQ;
#endif

/// \> Cycle 19, write updated diag info to EPICS
	if(cycleNum == HKP_DIAG_UPDATES)	
        {
	  pLocalEpics->epicsOutput.userTime = usrHoldTime;
	  pLocalEpics->epicsOutput.ipcStat = ipcErrBits;
	  if(ipcErrBits) feStatus |= FE_ERROR_IPC;
	  // Create FB status word for return to EPICS
	  mxStat = 0;
	  mxDiagR = daqPtr->reqAck;
	  if((mxDiag & 1) != (mxDiagR & 1)) mxStat = 1;
	  if((mxDiag & 2) != (mxDiagR & 2)) mxStat += 2;
#ifdef DUAL_DAQ_DC
	  if((mxDiag & 4) != (mxDiagR & 4)) mxStat += 4;
	  if((mxDiag & 8) != (mxDiagR & 8)) mxStat += 8;
#endif
	  pLocalEpics->epicsOutput.fbNetStat = mxStat;
  	  mxDiag = mxDiagR;
	  if(mxStat != MX_OK)
		feStatus |= FE_ERROR_DAQ;;
	  usrHoldTime = 0;
  	  if(pLocalEpics->epicsInput.overflowReset)
	  {
                if (pLocalEpics->epicsInput.overflowReset) {
                   for (ii = 0; ii < 16; ii++) {
                      for (jj = 0; jj < cdsPciModules.adcCount; jj++) {
                         overflowAdc[jj][ii] = 0;
                         overflowAdc[jj][ii + 16] = 0;
			pLocalEpics->epicsOutput.overflowAdcAcc[jj][ii] = 0;
			pLocalEpics->epicsOutput.overflowAdcAcc[jj][ii + 16] = 0;
                      }

                      for (jj = 0; jj < cdsPciModules.dacCount; jj++) {
                         pLocalEpics->epicsOutput.overflowDacAcc[jj][ii] = 0;
                      }
                   }
                }
	  }
  	  if((pLocalEpics->epicsInput.overflowReset) || (overflowAcc > OVERFLOW_CNTR_LIMIT))
	  {
		pLocalEpics->epicsInput.overflowReset = 0;
		pLocalEpics->epicsOutput.ovAccum = 0;
		overflowAcc = 0;
	  }
        }

#ifdef ADC_SLAVE
        if(cycleNum == 1)
	{
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	}
#endif
/// \> Cycle 20, Update latest DAC output values to EPICS
        if(subcycle == HKP_DAC_EPICS_UPDATES)
	{
	      // Send DAC output values at 16Hzfb
	      for(jj=0;jj<cdsPciModules.dacCount;jj++)
	      {
	    	for(ii=0;ii<MAX_DAC_CHN_PER_MOD;ii++)
	    	{
			pLocalEpics->epicsOutput.dacValue[jj][ii] = dacOutEpics[jj][ii];
		}
	      }
	}

/// \> Cycle 21, Update ADC/DAC status to EPICS.
        if(cycleNum == HKP_ADC_DAC_STAT_UPDATES)
        {
	  pLocalEpics->epicsOutput.ovAccum = overflowAcc;
	  for(jj=0;jj<cdsPciModules.adcCount;jj++)
	  {
	    // SET/CLR Channel Hopping Error
	    if(adcChanErr[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statAdc[jj] &= ~(2);
		feStatus |= FE_ERROR_ADC;;
	    }
 	    else pLocalEpics->epicsOutput.statAdc[jj] |= 2;
	    adcChanErr[jj] = 0;
	    // SET/CLR Overflow Error
	    if(adcOF[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statAdc[jj] &= ~(4);
		feStatus |= FE_ERROR_OVERFLOW;;
	    }
 	    else pLocalEpics->epicsOutput.statAdc[jj] |= 4;
	    adcOF[jj] = 0;
	    for(ii=0;ii<32;ii++)
	    {

                if (pLocalEpics->epicsOutput.overflowAdcAcc[jj][ii] > OVERFLOW_CNTR_LIMIT) {
		   pLocalEpics->epicsOutput.overflowAdcAcc[jj][ii] = 0;
                }
		pLocalEpics->epicsOutput.overflowAdc[jj][ii] = overflowAdc[jj][ii];
		overflowAdc[jj][ii] = 0;

	    }
	  }
	  // If ADC channels not where they should be, we have no option but to exit
	  // from the RT code ie loops would be working with wrong input data.
	  if (chanHop) {
	  	pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
	         stop_working_threads = 1;
	         vmeDone = 1;
	         printf("Channel Hopping Detected on one or more ADC modules !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	         printf("Check GDSTP screen ADC status bits to id affected ADC modules\n");
	         printf("Code is exiting ..............\n");
	 	 continue;    
	  }
	  for(jj=0;jj<cdsPciModules.dacCount;jj++)
	  {
	    if(dacOF[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_OVERFLOW_BIT);
		feStatus |= FE_ERROR_OVERFLOW;;
	    }
 	    else pLocalEpics->epicsOutput.statDac[jj] |= DAC_OVERFLOW_BIT;
	    dacOF[jj] = 0;
	    if(dacChanErr[jj]) 
	    {
	    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_TIMING_BIT);
	    }
 	    else pLocalEpics->epicsOutput.statDac[jj] |= DAC_TIMING_BIT;
	    dacChanErr[jj] = 0;
	    for(ii=0;ii<MAX_DAC_CHN_PER_MOD;ii++)
	    {

                if (pLocalEpics->epicsOutput.overflowDacAcc[jj][ii] > OVERFLOW_CNTR_LIMIT) {
		   pLocalEpics->epicsOutput.overflowDacAcc[jj][ii] = 0;
                }
		pLocalEpics->epicsOutput.overflowDac[jj][ii] = overflowDac[jj][ii];
		overflowDac[jj][ii] = 0;

	    }
	  }
        }
/// \> Cycle 300, If IOP and RFM cards, check own data diagnostics
#ifdef ADC_MASTER
	// Deal with the own-data bits on the VMIC 5565 rfm cards
	if (cdsPciModules.rfmCount > 0) {
        	if (cycleNum >= HKP_RFM_CHK_CYCLE && cycleNum < (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount)) {
			int mod = cycleNum - HKP_RFM_CHK_CYCLE;
			status = vmic5565CheckOwnDataRcv(mod);
			if(!status) ipcErrBits |= 4 + (mod * 4);
		}
		if (cycleNum >= (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount) && cycleNum < (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount*2)) {
			int mod = cycleNum - HKP_RFM_CHK_CYCLE - cdsPciModules.rfmCount;
			vmic5565ResetOwnDataLight(mod);
		}
		if (cycleNum >= (HKP_RFM_CHK_CYCLE + 2*cdsPciModules.rfmCount) && cycleNum < (HKP_RFM_CHK_CYCLE + cdsPciModules.rfmCount*3)) {
			int mod = cycleNum - HKP_RFM_CHK_CYCLE - cdsPciModules.rfmCount*2;
				// Write data out to the RFM to trigger the light
	  			((volatile long *)(cdsPciModules.pci_rfm[mod]))[2] = 0;
		}
	}
/// \> Cycle 400 to 400 + numDacModules, write DAC heartbeat to AI chassis (only for 18 bit DAC modules)
// DAC WD Write for 18 bit DAC modules
// Check once per second on code cycle 400 to dac count
// Only one write per code cycle to reduce time
       	if (cycleNum >= HKP_DAC_WD_CLK && cycleNum < (HKP_DAC_WD_CLK + cdsPciModules.dacCount)) 
	{
		if (cycleNum == HKP_DAC_WD_CLK) dacWatchDog ^= 1;
		jj = cycleNum - HKP_DAC_WD_CLK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			// static int dacWatchDog = 0;
			volatile GSA_18BIT_DAC_REG *dac18bitPtr;
			// if (cycleNum == HKP_DAC_WD_CLK) dacWatchDog ^= 1;
			dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			if(iopDacEnable && !dacChanErr[jj]) {
				dac18bitPtr->digital_io_ports = (dacWatchDog | GSAO_18BIT_DIO_RW);
				}

		}
		if(cdsPciModules.dacType[jj] == GSC_20AO8)
		{
			volatile GSA_20BIT_DAC_REG *dac20bitPtr;
			dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
			if(iopDacEnable && !dacChanErr[jj]) {
				dac20bitPtr->digital_io_ports = (dacWatchDog | GSAO_20BIT_DIO_RW);
				}

		}
	}
/// \> Cycle 500 to 500 + numDacModules, read back watchdog from AI chassis (18 bit DAC only)
// AI Chassis WD CHECK for 18 bit DAC modules
// Check once per second on code cycle HKP_DAC_WD_CHK to dac count
// Only one read per code cycle to reduce time
       	if (cycleNum >= HKP_DAC_WD_CHK && cycleNum < (HKP_DAC_WD_CHK + cdsPciModules.dacCount)) 
	{
		jj = cycleNum - HKP_DAC_WD_CHK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			static int dacWDread = 0;
			volatile GSA_18BIT_DAC_REG *dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			dacWDread = dac18bitPtr->digital_io_ports;
			if(((dacWDread >> 8) & 1) > 0)
			    {
			    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_WD_BIT);
			    }
			    else 
			    	pLocalEpics->epicsOutput.statDac[jj] |= DAC_WD_BIT;

		}
		if(cdsPciModules.dacType[jj] == GSC_20AO8)
		{
			static int dacWDread = 0;
			volatile GSA_20BIT_DAC_REG *dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
			dacWDread = dac20bitPtr->digital_io_ports;
			if(((dacWDread >> 8) & 1) > 0)
			    {
			    	pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_WD_BIT);
			    }
			    else 
			    	pLocalEpics->epicsOutput.statDac[jj] |= DAC_WD_BIT;

		}
	}

/// \> Cycle 600 to 600 + numDacModules, Check DAC FIFO Sizes to determine if DAC modules are synched to code 
/// - ---- 18bit DAC reads out FIFO size dirrectly, but 16bit module only has a 4 bit register 
/// area for FIFO empty, quarter full, etc. So, to make these bits useful in 16 bit module,
/// code must set a proper FIFO size in map.c code.
// This code runs once per second.
       	if (cycleNum >= HKP_DAC_FIFO_CHK && cycleNum < (HKP_DAC_FIFO_CHK + cdsPciModules.dacCount) && !dacTimingError) 
	{
		jj = cycleNum - HKP_DAC_FIFO_CHK;
		if(cdsPciModules.dacType[jj] == GSC_18AO8)
		{
			volatile GSA_18BIT_DAC_REG *dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
			out_buf_size = dac18bitPtr->OUT_BUF_SIZE;
			dacOutBufSize[jj] = out_buf_size;
			if((out_buf_size < 8) || (out_buf_size > 24))
			{
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
			    if(dacTimingErrorPending[jj]) dacTimingError = 1;
  			    dacTimingErrorPending[jj] = 1;
			} else {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
  			    dacTimingErrorPending[jj] = 0;
			}

		}
		if(cdsPciModules.dacType[jj] == GSC_20AO8)
		{
			volatile GSA_20BIT_DAC_REG *dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
			out_buf_size = dac20bitPtr->OUT_BUF_SIZE;
			dacOutBufSize[jj] = out_buf_size;
			if((out_buf_size > 24))
			{
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
			    if(dacTimingErrorPending[jj]) dacTimingError = 1;
  			    dacTimingErrorPending[jj] = 1;
			} else {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
  			    dacTimingErrorPending[jj] = 0;
			}

		}
		if(cdsPciModules.dacType[jj] == GSC_16AO16)
		{
			status = gsc16ao16CheckDacBuffer(jj);
			dacOutBufSize[jj] = status;
			if(status != 2)
			{
			    pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_FIFO_BIT);
			    if(dacTimingErrorPending[jj]) dacTimingError = 1;
  			    dacTimingErrorPending[jj] = 1;
			} else {
			    pLocalEpics->epicsOutput.statDac[jj] |= DAC_FIFO_BIT;
  			    dacTimingErrorPending[jj] = 0;
			}
		}
	}
	if (dacTimingError) feStatus |= FE_ERROR_DAC;

#endif
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
#if defined(SERVO64K) || defined(SERVO32K) || defined(SERVO16K)
// This produces cycle time histogram in /proc file
	{
#if defined(SERVO64K) || defined(SERVO32K)
		static const int nb = 31;
#elif defined(SERVO16K)
		static const int nb = 63;
#endif

		cycleHist[cycleTime<nb?cycleTime:nb]++;
		cycleHistWhen[cycleTime<nb?cycleTime:nb] = cycleNum;
	}
#endif
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
#ifdef ADC_MASTER
	usrTime = (cpuClock[CPU_TIME_USR_START] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
#else
	usrTime = (cpuClock[CPU_TIME_USR_END] - cpuClock[CPU_TIME_USR_START])/CPURATE;
#endif
	if(usrTime > usrHoldTime) usrHoldTime = usrTime;

        /// \> Update internal cycle counters
          cycleNum += 1;
#ifdef DIAG_TEST
          if(pLocalEpics->epicsInput.bumpCycle != 0) {
	  	cycleNum += pLocalEpics->epicsInput.bumpCycle;
		pLocalEpics->epicsInput.bumpCycle = 0;
	  }
#endif
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

#ifdef ADC_SLAVE
  deallocate_dac_channels();
#endif

  /* System reset command received */
  return (void *)-1;
}
