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

///	@file controllerApp.c
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
#ifdef USE_ZMQ
	#include "drv/daqLibZmq.c"		// DAQ/GDS connection software
#else
	#include "drv/daqLib.c"		// DAQ/GDS connection software
#endif
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
int adcHoldTime = 0;		///< Stores time between code cycles
int adcHoldTimeMax = 0;		///< Stores time between code cycles
int startGpsTime = 0;
int adcHoldTimeMin = 0xffff;
int adcHoldTimeAvg = 0;
int adcHoldTimeAvgPerSec;
int usrTime;			///< Time spent in user app code
int usrHoldTime = 0;		///< Max time spent in user app code
int cardCountErr = 0;
int cycleTime;			///< Current cycle time
int timeHold = 0;			///< Max code cycle time within 1 sec period
int timeHoldHold = 0;			///< Max code cycle time within 1 sec period; hold for another sec
int timeHoldWhen= 0;			///< Cycle number within last second when maximum reached; running
int timeHoldWhenHold = 0;		///< Cycle number within last second when maximum reached

struct rmIpcStr *daqPtr;

int  getGpsTime(unsigned int *tsyncSec, unsigned int *tsyncUsec); 

// Include C code modules
#include "moduleLoadApp.c"
#include "map.c"


char daqArea[2*DAQ_DCU_SIZE];		// Space allocation for daqLib buffers
int cpuId = 1;

#ifdef DUAL_DAQ_DC
	#define MX_OK	15
#else
	#define MX_OK	3
#endif

// Initial diag reset flag
int initialDiagReset = 1;

// Cache flushing mumbo jumbo suggested by Thomas Gleixner, it is probably useless
// Did not see any effect
  char fp [64*1024];


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
  int ii,jj,kk,ll,mm;			// Dummy loop counter variables
  static int clock1Min = 0;		///  @param clockMin Minute counter (Not Used??)
  static int cpuClock[CPU_TIMER_CNT];	///  @param cpuClock[] Code timing diag variables

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
  int diagWord = 0;			/// @param diagWord Code diagnostic bit pattern returned to EPICS
  int system = 0;
  int sampleCount = 1;			/// @param sampleCount Number of ADC samples to take per code cycle
  int syncSource = SYNC_SRC_MASTER;	/// @param syncSource Code startup synchronization source
  int mxStat = 0;			/// @param mxStat Net diags when myrinet express is used
  int mxDiag = 0;
  int mxDiagR = 0;
// ****** Share data
  int ioClockDac = DAC_PRELOAD_CNT;
  int ioMemCntr = 0;
  int ioMemCntrDac = DAC_PRELOAD_CNT;
  int memCtr = 0;
  int ioClock = 0;
  double dac_in =  0.0;			/// @param dac_in DAC value after upsample filtering
  int dac_out = 0;			/// @param dac_out Integer value sent to DAC FIFO

  int feStatus = 0;

  int clk, clk1;			// Used only when run on timer enabled (test mode)

  int cnt = 0;
  unsigned long cpc;

/// **********************************************************************************************\n
/// Start Initialization Process \n
/// **********************************************************************************************\n

  /// \> Flush L1 cache
  memset (fp, 0, 64*1024);
  memset (fp, 1, 64*1024);
  clflush_cache_range ((void *)fp, 64*1024);

  fz_daz(); /// \> Kill the denorms!

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
    for (jj = 0; jj < 16; jj++) {
 	dacOut[ii][jj] = 0.0;
 	dacOutUsed[ii][jj] = 0;
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


/// \> Wait for BURT restore.\n
/// - ---- Code will exit if BURT flag not set by EPICS sequencer.
  // Do not proceed until EPICS has had a BURT restore *******************************
  printf("Waiting for EPICS BURT Restore = %d\n", pLocalEpics->epicsInput.burtRestore);
  cnt = 0;
  do{
#ifndef NO_CPU_SHUTDOWN
	udelay(MAX_UDELAY);
        udelay(MAX_UDELAY);
        udelay(MAX_UDELAY);
#else
	msleep(80);
#endif
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


  // Clear the code exit flag
  vmeDone = 0;

  /// \> Call user application software initialization routine.
  printf("Calling feCode() to initialize\n");
  iopDacEnable = feCode(cycleNum,dWord,dacOut,dspPtr[0],&dspCoeff[0], (struct CDS_EPICS *)pLocalEpics,1);

/// \> Verify DAC channels defined for this app are not already in use. \n
/// - ---- User apps are allowed to share DAC modules but not DAC channels.
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
  if (vmeDone == 1) {
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

  /// \> Initialize the ADC status
  for(jj=0;jj<cdsPciModules.adcCount;jj++)
  {
  	  pLocalEpics->epicsOutput.statAdc[jj] = 1;
  }

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
	// Get GPS seconds from MASTER
	timeSec = ioMemData->gpsSecond;
        pLocalEpics->epicsOutput.timeDiag = timeSec;
	// Decrement GPS seconds as it will be incremented on first read cycle.
	timeSec --;

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
#ifdef NO_CPU_SHUTDOWN
	usleep_range(2,4);
#endif


#ifdef NO_CPU_SHUTDOWN
  while(!kthread_should_stop()){
#else
  while(!vmeDone){ 	// Run forever until user hits reset
#endif
	// **********************************************************************************************************
	// NORMAL OPERATION -- Wait for ADC data ready
	// On startup, only want to read one sample such that first cycle
	// coincides with GPS 1PPS. Thereafter, sampleCount will be 
	// increased to appropriate number of 65536 s/sec to match desired
	// code rate eg 32 samples each time thru before proceeding to match 2048 system.
	// **********************************************************************************************************

/// \> On 1PPS mark \n
        if(cycleNum == 0)
        {
	  /// - ---- Check awgtpman status.
	  //printf("awgtpman gps = %d local = %d\n", pEpicsComms->padSpace.awgtpman_gps, timeSec);
	  pLocalEpics->epicsOutput.awgStat = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
	  if(pLocalEpics->epicsOutput.awgStat) feStatus |= FE_ERROR_AWG;
	  /// - ---- Check if DAC outputs are enabled, report error.
	  if(!iopDacEnable || dkiTrip) feStatus |= FE_ERROR_DAC_ENABLE;

	}
#ifdef NO_CPU_SHUTDOWN
        if(vmeDone)usleep_range(2,4);
#endif
        for(ll=0;ll<sampleCount;ll++)
        {
		/// \> SLAVE gets its adc data from MASTER via ipc shared memory\n
		/// \> For each ADC defined:
		for(jj=0;jj<cdsPciModules.adcCount;jj++)
		{
        		mm = cdsPciModules.adcConfig[jj];
	    		rdtscl(cpuClock[CPU_TIME_RDY_ADC]);
			/// - ---- Wait for proper timestamp in shared memory, indicating data ready.
                        do{
		    		rdtscl(cpuClock[CPU_TIME_ADC_WAIT]);
				adcWait = (cpuClock[CPU_TIME_ADC_WAIT] - cpuClock[CPU_TIME_RDY_ADC])/CPURATE;
                        }while((ioMemData->iodata[mm][ioMemCntr].cycle != ioClock) && (adcWait < MAX_ADC_WAIT_SLAVE));
			timeSec = ioMemData->iodata[mm][ioMemCntr].timeSec;
			if (cycle_gps_time == 0) {
				startGpsTime = timeSec;
				pLocalEpics->epicsOutput.startgpstime = startGpsTime;
			}
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
					dWord[jj][ii] = iir_filter_biquad(dWord[jj][ii],FE_OVERSAMPLE_COEFF,2,&dHistory[ii+jj*32][0]);
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
                        }
                }
		/// \> Set counters for next read from ipc memory
                ioClock = (ioClock + 1) % IOP_IO_RATE;
                ioMemCntr = (ioMemCntr + 1) % IO_MEMORY_SLOTS;
        	rdtscl(cpuClock[CPU_TIME_CYCLE_START]);

	}

	// After first synced ADC read, must set to code to read number samples/cycle
        sampleCount = OVERSAMPLE_TIMES;
/// End of ADC Read **************************************************************************************


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

/// END OF DAC WRITE *************************************************


/// BEGIN HOUSEKEEPING ************************************************ \n

        pLocalEpics->epicsOutput.cycle = cycleNum;

/// \> Cycle 18, Send timing info to EPICS at 1Hz
	if(cycleNum ==HKP_TIMING_UPDATES)	
        {
	  pLocalEpics->epicsOutput.cpuMeter = timeHold;
	  pLocalEpics->epicsOutput.cpuMeterMax = timeHoldMax;
          timeHoldHold = timeHold;
          timeHold = 0;
	  timeHoldWhenHold = timeHoldWhen;

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
	  }
	  // Flip the onePPS various once/sec as a watchdog monitor.
	  // pLocalEpics->epicsOutput.onePps ^= 1;
	  pLocalEpics->epicsOutput.diagWord = diagWord;
        }


/// \> Check for requests for filter module clear history requests. This is spread out over a number of cycles.
	// Spread out filter coeff update, but keep updates at 16 Hz
	// here we are rounding up:
	//   x/y rounded up equals (x + y - 1) / y
	//
	static const unsigned int mpc = (MAX_MODULES + (FE_RATE / 16) - 1) / (FE_RATE / 16); // Modules per cycle
	unsigned int smpc = mpc * subcycle; // Start module counter
	unsigned int empc = smpc + mpc; // End module counter
	unsigned int i;
	for (i = smpc; i < MAX_MODULES && i < empc ; i++) 
		checkFiltReset(i, dspPtr[0], pDsp[0], &dspCoeff[0], MAX_MODULES, pCoeff[0]);

	/// \> Check if code exit is requested
	if(cycleNum == MAX_MODULES) 
		vmeDone = stop_working_threads | checkEpicsReset(cycleNum, (struct CDS_EPICS *)pLocalEpics);

/// \> Cycle 10 to number of BIO cards:\n
 /// - ---- Read Dio cards once per second \n

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
/// \> Write Dio cards only on change
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
	  if(ipcErrBits & 0xf) feStatus |= FE_ERROR_IPC;
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

        if(cycleNum == 1)
	{
          pLocalEpics->epicsOutput.timeDiag = timeSec;
	}
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
	// Capture end of cycle time.
        rdtscl(cpuClock[CPU_TIME_CYCLE_END]);

	/// \> Compute code cycle time diag information.
	cycleTime = (cpuClock[CPU_TIME_CYCLE_END] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
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
	}
	adcTime = cpuClock[CPU_TIME_CYCLE_START];
	// Calc the max time of one cycle of the user code
	// For IOP, more interested in time to get thru ADC read code and send to slave apps
	usrTime = (cpuClock[CPU_TIME_USR_END] - cpuClock[CPU_TIME_USR_START])/CPURATE;
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

  deallocate_dac_channels();

  /* System reset command received */
  return (void *)-1;
}