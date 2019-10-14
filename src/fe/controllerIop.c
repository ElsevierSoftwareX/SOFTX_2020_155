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

///	@file controllerIop.c
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


#include "controllerko.h"


#ifdef DOLPHIN_TEST
#include "dolphin.c"
#endif


#if defined (TIME_MASTER) || defined (TIME_SLAVE)
TIMING_SIGNAL *pcieTimer;
#endif



// Duotone diags struct
duotone_diag_t dt_diag;

/// Maintains present cycle count within a one second period.
int adcCycleNum = 0;
// Variables for setting IOP->APP I/O
int ioClockDac = DAC_PRELOAD_CNT;
// int ioMemCntr = 0;
int ioMemCntrDac = DAC_PRELOAD_CNT;
// DAC variable
int dacEnable = 0;
int dacWriteEnable = 0;	/// @param dacWriteEnable  No DAC outputs until >4 times through code
int dacTimingErrorPending[MAX_DAC_MODULES];
static int dacTimingError = 0;
int dacWatchDog = 0;
int dac_out = 0;

int pBits[9] = {1,2,4,8,16,32,64,128,256};

int  getGpsTime(unsigned int *tsyncSec, unsigned int *tsyncUsec); 

// Include C code modules
#include "moduleLoadIop.c"

#ifdef TIME_SLAVE
#include "mapVirtual.c"
#include <drv/time_slave_io.c>
#else
#include "map.c"
#include <drv/iop_adc_functions.c>
#include <drv/iop_dac_functions.c>
#endif

#include <drv/dac_info.c>
#include <drv/adc_info.c>

//***********************************************************************
// TASK: fe_start_iop()	
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
void *fe_start_iop(void *arg)
{
  int ii,jj,kk,ll;			// Dummy loop counter variables
  static int clock1Min = 0;		///  @param clockMin Minute counter (Not Used??)
  static int cpuClock[CPU_TIMER_CNT];	///  @param cpuClock[] Code timing diag variables

  int sync21ppsCycles = 0;		/// @param sync32ppsCycles Number of attempts to sync to 1PPS
  RFM_FE_COMMS *pEpicsComms;		/// @param *pEpicsComms Pointer to EPICS shared memory space
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
  int sync21pps = 0;			/// @param sync21pps Code startup sync to 1PPS flag
  int syncSource = SYNC_SRC_NONE;	/// @param syncSource Code startup synchronization source
  int mxStat = 0;			/// @param mxStat Net diags when myrinet express is used
  int mxDiag = 0;
  int mxDiagR = 0;

  int feStatus = 0;
  int dkiTrip = 0;

  unsigned int usec = 0;
  unsigned long cpc;
  float duotoneTimeDac;
  float duotoneTime;

  int usloop=1;
  double adcval[MAX_ADC_MODULES][MAX_ADC_CHN_PER_MOD];
  



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

adcInfo_t *padcinfo = (adcInfo_t *)&adcinfo;
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

/// \> Init code synchronization source.
  // Look for DIO card or IRIG-B Card
  // if Contec 1616 BIO present, TDS slave will be used for timing.
  if(cdsPciModules.cDio1616lCount) syncSource = SYNC_SRC_TDS;
  else syncSource = SYNC_SRC_1PPS;

#ifdef NO_SYNC
  syncSource = SYNC_SRC_NONE;
#endif


#ifdef TIME_MASTER
  pcieTimer = (TIMING_SIGNAL *) ((volatile char *)(cdsPciModules.dolphinWrite[0]) + IPC_PCIE_TIME_OFFSET);
#endif

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
       pDsp[0]->data[ii].exciteInput = 0.0;


  /// \> Initialize IIR filter bank values
  if (initVars(pDsp[0], pDsp[0], dspCoeff, MAX_MODULES, pCoeff[0])) {
    pLocalEpics->epicsOutput.fe_status = FILT_INIT_ERROR;
    return 0;
  }


  udelay(1000);

/// \> Initialize DAQ variable/software 
#if !defined(NO_DAQ) && !defined(IOP_TASK)
  /// - ---- Set data range limits for daqLib routine 
  daq.filtExMin = GDS_16K_EXC_MIN;
  daq.filtTpMin = GDS_16K_TP_MIN;
  daq.filtExMax = daq.filtExMin + MAX_MODULES;
  daq.filtExSize = MAX_MODULES;
  daq.xExMin = daq.filtExMax;
  daq.xExMax = daq.xExMin + GDS_MAX_NFM_EXC;
  daq.filtTpMax = daq.filtTpMin + MAX_MODULES * 3;
  daq.filtTpSize = MAX_MODULES * 3;
  daq.xTpMin = daq.filtTpMax;
  daq.xTpMax = daq.xTpMin + GDS_MAX_NFM_TP;
  
  /// - ---- Initialize DAQ function
  status = daqWrite(0,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0, (int *)(pLocalEpics->epicsOutput.gdsMon),xExc,pEpicsDaq);
  if(status == -1) 
  {
    pLocalEpics->epicsOutput.fe_status = DAQ_INIT_ERROR;
    vmeDone = 1;
    return(0);
  }

#endif

  /// - ---- Assign DAC testpoint pointers
  for (ii = 0; ii <  cdsPciModules.dacCount; ii++)
    for (jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++) // 16 per DAC regardless of the actual
        testpoint[MAX_DAC_CHN_PER_MOD * ii + jj] = floatDacOut + MAX_DAC_CHN_PER_MOD * ii + jj;

  // Zero out storage
  memset(floatDacOut, 0, sizeof(floatDacOut));

  pLocalEpics->epicsOutput.ipcStat = 0;
  pLocalEpics->epicsOutput.fbNetStat = 0;
  pLocalEpics->epicsOutput.tpCnt = 0;

  // Clear the code exit flag
  vmeDone = 0;

  /// \> Call user application software initialization routine.
  iopDacEnable = feCode(cycleNum,adcval,dacOut,dspPtr[0],&dspCoeff[0], (struct CDS_EPICS *)pLocalEpics,1);

  // Initialize timing info variables
  initializeTimingDiags(&timeinfo);
  missedCycle = 0;

  // Initialize duotone measurement signals
  initializeDuotoneDiags(&dt_diag);

  /// \> Initialize the ADC modules *************************************
  pLocalEpics->epicsOutput.fe_status = INIT_ADC_MODS;
  status = iop_adc_init(padcinfo);

  /// \> Initialize the DAC module variables  **********************************
  pLocalEpics->epicsOutput.fe_status = INIT_DAC_MODS;
  status = iop_dac_init(dacTimingErrorPending);

  pLocalEpics->epicsOutput.fe_status = INIT_SYNC;

#ifdef TIME_SLAVE
syncSource = SYNC_SRC_DOLPHIN;
#else

/// \> Find the code syncrhonization source. \n
/// - Standard aLIGO Sync source is the Timing Distribution System (TDS) (SYNC_SRC_TDS). 
  switch(syncSource)
  {
    /// \>\> For SYNC_SRC_TDS, initialize system for synchronous start on 1PPS mark:
    case SYNC_SRC_TDS:
      /// - ---- Turn off TDS slave unit timing clocks, in turn removing clocks from ADC/DAC modules.
      for(ii=0;ii<tdsCount;ii++)
      {
        CDIO1616Output[ii] = TDS_STOP_CLOCKS;
        CDIO1616Input[ii] = contec1616WriteOutputRegister(&cdsPciModules, tdsControl[ii], CDIO1616Output[ii]);
      }
      udelay(MAX_UDELAY);
      udelay(MAX_UDELAY);
      /// - ---- Arm ADC modules
      gsc16ai64Enable(&cdsPciModules);
      gsc18ai32Enable(&cdsPciModules);
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
      // #ifndef NO_DAC_PRELOAD
#if !defined  (NO_DAC_PRELOAD) && !defined (TIME_SLAVE)
      status = iop_dac_preload(dacPtr);
#endif
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
// #ifndef NO_DAC_PRELOAD
#if !defined  (NO_DAC_PRELOAD) && !defined (TIME_SLAVE)
      gsc16ai64Enable(&cdsPciModules);
      status = iop_dac_preload(dacPtr);
#endif
      // Arm ADC modules
      // This has to be done sequentially, one at a time.
      // status = sync_adc_2_1pps();
      sync21pps = 1;
      gsc16ai64Enable(&cdsPciModules);
      gsc18ai32Enable(&cdsPciModules);
      break;
    case SYNC_SRC_NONE:
      gsc16ai64Enable(&cdsPciModules);
      gsc18ai32Enable(&cdsPciModules);
      sync21pps = 1;
      break;
    case SYNC_SRC_DOLPHIN:
      sync21pps = 1;
      break;
    default: {
      // IRIG-B card not found, so use CPU time to get close to 1PPS on startup
      // Pause until this second ends
      gsc16ai64Enable(&cdsPciModules);
      gsc18ai32Enable(&cdsPciModules);
      sync21pps = 1;
      break;
    }
  }
#endif

//     for(jj=0;jj<cdsPciModules.adcCount;jj++) gsc18ai32DmaEnable(jj);
  pLocalEpics->epicsOutput.fe_status = NORMAL_RUN;

  onePpsTime = cycleNum;
#ifdef REMOTE_GPS
  timeSec = remote_time((struct CDS_EPICS *)pLocalEpics);
#elif TIME_SLAVE
  timeSec = sync2master(pcieTimer);
  sync21pps = 1;
#else
  timeSec = current_time_fe() -1;
#endif

  adcinfo.adcTime = rdtsc_ordered();

  /// ******************************************************************************\n
  /// Enter the infinite FE control loop  ******************************************\n

  /// ******************************************************************************\n
  // Calculate how many CPU cycles per code cycle
  cpc = cpu_khz * 1000;
  cpc /= CYCLE_PER_SECOND;


#ifdef NO_CPU_SHUTDOWN
  while(!kthread_should_stop() && !vmeDone){
#else
  while(!vmeDone){ 	// Run forever until user hits reset
#endif
// *****************************************************************************************
// NORMAL OPERATION -- Wait for ADC data ready
// *****************************************************************************************
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

/// \> On 1PPS mark \n
    if(cycleNum == 0)
    {
      /// - ---- Check awgtpman status.
      pLocalEpics->epicsOutput.awgStat = (pEpicsComms->padSpace.awgtpman_gps != timeSec);
      if(pLocalEpics->epicsOutput.awgStat) feStatus |= FE_ERROR_AWG;
      /// - ---- Check if DAC outputs are enabled, report error.
      if(!iopDacEnable || dkiTrip) feStatus |= FE_ERROR_DAC_ENABLE;

      /// - ---- If IOP, Increment GPS second
#ifndef TIME_SLAVE
      timeSec ++;
#endif
      pLocalEpics->epicsOutput.timeDiag = timeSec;
      if (cycle_gps_time == 0) {
        timeinfo.startGpsTime = timeSec;
        pLocalEpics->epicsOutput.startgpstime = timeinfo.startGpsTime;
      }
      cycle_gps_time = timeSec;
    }
#ifdef NO_CPU_SHUTDOWN
    if((cycleNum % 65536) == 0)  {
        usleep_range(1,3);
        printk("cycleNum = %d\n",cycleNum);
    }
#endif
// Start of ADC Read **********************************************************************
    // Read ADC data
    status = iop_adc_read (padcinfo, cpuClock);
    // Try synching to 1PPS on ADC[0][31] if not using IRIG-B or TDS
    // Only try for 1 sec.
    if(!sync21pps)
    {
      // 1PPS signal should rise above 4000 ADC counts if present.
      if((adcinfo.adcData[0][31] < ONE_PPS_THRESH) && (sync21ppsCycles < (CYCLE_PER_SECOND*OVERSAMPLE_TIMES))) 
      {
        ll = -1;
        sync21ppsCycles ++;
      }else {
        // Need to start clocking the DAC outputs.
        gsc18ao8Enable(&cdsPciModules);
	gsc16ao16Enable(&cdsPciModules);
        sync21pps = 1;
	// 1PPS never found, so indicate NO SYNC to user
	if(sync21ppsCycles >= (CYCLE_PER_SECOND*OVERSAMPLE_TIMES))
	{
          syncSource = SYNC_SRC_NONE;
        } else {
          // 1PPS found and synched to
          syncSource = SYNC_SRC_1PPS;
        }
        pLocalEpics->epicsOutput.timeErr = syncSource;
      }
    }


for(usloop=0;usloop<UNDERSAMPLE;usloop++)
{

// **************************************************************************************
/// \> Call the front end specific application  ******************\n
/// - -- This is where the user application produced by RCG gets called and executed. \n\n
    // 
    for(ii=0;ii<cdsPciModules.adcCount;ii++)
    {
        for(jj=0;jj<32;jj++)
        {
            adcval[ii][jj] = dWord[ii][jj][usloop];
        }
    }
    cpuClock[CPU_TIME_USR_START] = rdtsc_ordered();
    iopDacEnable = feCode(cycleNum,adcval,dacOut,dspPtr[0],&dspCoeff[0],(struct CDS_EPICS *)pLocalEpics,0);
    cpuClock[CPU_TIME_USR_END] = rdtsc_ordered();
// **************************************************************************************
//
//
// **************************************************************************************
    /// - ---- Reset ADC DMA Start Flag \n
    /// - --------- This allows ADC to dump next data set whenever it is ready
    // for(jj=0;jj<cdsPciModules.adcCount;jj++) gsc16ai64DmaEnable(jj);
    // for(jj=0;jj<cdsPciModules.adcCount;jj++) gsc18ai32DmaEnable(jj);
    odcStateWord = 0;
//
// ********************************************************************
/// WRITE DAC OUTPUTS ***************************************** \n
// ********************************************************************

#ifndef DAC_WD_OVERRIDE
    // If a DAC module has bad timing then quit writing outputs
    // COMMENT OUT NEXT LINE FOR TEST STAND w/bad DAC cards. 
    if(dacTimingError) iopDacEnable = 0;
#endif
    // Write out data to DAC modules
    if(usloop == 0) dkiTrip = iop_dac_write();


// ***********************************************************************
/// BEGIN HOUSEKEEPING ************************************************ \n
// ***********************************************************************

    pLocalEpics->epicsOutput.cycle = cycleNum;
// *****************************************************************
/// \> Cycle 0: \n
/// - ---- Read IRIGB time if symmetricom card (this is not standard, but supported for earlier cards. \n
// *****************************************************************
    if(cycleNum == HKP_READ_SYMCOM_IRIGB)
    {
      if(cdsPciModules.gpsType == SYMCOM_RCVR) 
      {
        // Retrieve time set in at adc read and report offset from 1PPS
        gps_receiver_locked = getGpsTime(&timeSec,&usec);
        pLocalEpics->epicsOutput.irigbTime = usec;
      }
      if((usec > MAX_IRIGB_SKEW || usec < MIN_IRIGB_SKEW) && cdsPciModules.gpsType != 0) 
      {
        diagWord |= TIME_ERR_IRIGB;;
        feStatus |= FE_ERROR_TIMING;;
      }
/// - ---- Calc duotone diagnostic mean values for past second and reset.
      dt_diag.meanAdc = dt_diag.totalAdc/CYCLE_PER_SECOND;
      dt_diag.totalAdc = 0.0;
      dt_diag.meanDac = dt_diag.totalDac/CYCLE_PER_SECOND;
      dt_diag.totalDac = 0.0;
    }

// *****************************************************************
/// \> Cycle 1 and Spectricom IRIGB (standard), get IRIG-B time information.
// *****************************************************************
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

#if 0
/// \> Update duotone diag information
    dt_diag.dac[(cycleNum + DT_SAMPLE_OFFSET) % CYCLE_PER_SECOND] = dWord[ADC_DUOTONE_BRD][DAC_DUOTONE_CHAN][usloop];
    dt_diag.totalDac += dWord[ADC_DUOTONE_BRD][DAC_DUOTONE_CHAN][usloop];
    dt_diag.adc[(cycleNum + DT_SAMPLE_OFFSET) % CYCLE_PER_SECOND] = dWord[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN][usloop];
    dt_diag.totalAdc += dWord[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN][usloop];

// *****************************************************************
/// \> Cycle 16, perform duotone diag calcs.
// *****************************************************************
    if(cycleNum == HKP_DT_CALC)
    {
      duotoneTime = duotime(DT_SAMPLE_CNT, dt_diag.meanAdc, dt_diag.adc);
      pLocalEpics->epicsOutput.dtTime = duotoneTime;
      if(((duotoneTime < MIN_DT_DIAG_VAL) || (duotoneTime > MAX_DT_DIAG_VAL)) &&
        syncSource != SYNC_SRC_1PPS) 
          feStatus |= FE_ERROR_TIMING;
        duotoneTimeDac = duotime(DT_SAMPLE_CNT, dt_diag.meanDac, dt_diag.dac);
        pLocalEpics->epicsOutput.dacDtTime = duotoneTimeDac;
    }

// *****************************************************************
/// \> Cycle 17, set/reset DAC duotone switch if request has changed.
// *****************************************************************
    if(cycleNum == HKP_DAC_DT_SWITCH)
    {
      if(dt_diag.dacDuoEnable != pLocalEpics->epicsInput.dacDuoSet)
      {
        dt_diag.dacDuoEnable = pLocalEpics->epicsInput.dacDuoSet;
        if(dt_diag.dacDuoEnable)
          CDIO1616Output[0] = TDS_START_ADC_NEG_DAC_POS;
        else CDIO1616Output[0] = TDS_START_ADC_NEG_DAC_POS | TDS_NO_DAC_DUOTONE;
        CDIO1616Input[0] = contec1616WriteOutputRegister(&cdsPciModules, tdsControl[0], CDIO1616Output[0]);
      }
    }
#endif

// *****************************************************************
/// \> Cycle 18, Send timing info to EPICS at 1Hz
// *****************************************************************
    if(cycleNum ==HKP_TIMING_UPDATES)	
    {
      sendTimingDiags2Epics(pLocalEpics, &timeinfo, &adcinfo);

      pLocalEpics->epicsOutput.dacEnable = dacEnable;

      if((adcinfo.adcHoldTime > CYCLE_TIME_ALRM_HI) || (adcinfo.adcHoldTime < CYCLE_TIME_ALRM_LO)) 
      {
        diagWord |= FE_ADC_HOLD_ERR;
        feStatus |= FE_ERROR_TIMING;
      }
      if(timeinfo.timeHoldMax > CYCLE_TIME_ALRM) 
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
        timeinfo.timeHoldMax = 0;
        diagWord = 0;
        ipcErrBits = 0;
      	for(jj=0;jj<cdsPciModules.adcCount;jj++) adcinfo.adcRdTimeMax[jj] = 0;
      }
      pLocalEpics->epicsOutput.diagWord = diagWord;
      for(jj=0;jj<cdsPciModules.adcCount;jj++) {
        if(adcinfo.adcRdTimeErr[jj] > MAX_ADC_WAIT_ERR_SEC)
          pLocalEpics->epicsOutput.stateWord |= FE_ERROR_ADC;
          adcinfo.adcRdTimeErr[jj] = 0;
      }
    }


// *****************************************************************
/// \> Check for requests for filter module clear history requests. 
/// This is spread out over a number of cycles.
// Spread out filter coeff update, but keep updates at 16 Hz
// here we are rounding up:
//   x/y rounded up equals (x + y - 1) / y
//
// *****************************************************************
    { 
      static const unsigned int mpc = (MAX_MODULES + (FE_RATE / 16) - 1) / (FE_RATE / 16); // Modules per cycle
      unsigned int smpc = mpc * subcycle; // Start module counter
      unsigned int empc = smpc + mpc; // End module counter
      unsigned int i;
      for (i = smpc; i < MAX_MODULES && i < empc ; i++) 
        checkFiltReset(i, dspPtr[0], pDsp[0], &dspCoeff[0], MAX_MODULES, pCoeff[0]);
    }

// *****************************************************************
    /// \> Check if code exit is requested
    if(cycleNum == MAX_MODULES) 
      vmeDone = stop_working_threads | checkEpicsReset(cycleNum, (struct CDS_EPICS *)pLocalEpics);

#if 0
// *****************************************************************
	// If synced to 1PPS on startup, continue to check that code
	// is still in sync with 1PPS.
	// This is NOT normal aLIGO mode.
    if(syncSource == SYNC_SRC_1PPS)
    {
      // Assign chan 32 to onePps 
      onePps = adcinfo.adcData[ADC_DUOTONE_BRD][ADC_DUOTONE_CHAN];
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
#endif

// Following is only used on automated test system
#ifdef DIAG_TEST
    for(ii=0;ii<10;ii++)
    {
      if(ii<5) onePpsTest = adcinfo.adcData[0][ii];
      else onePpsTest = adcinfo.adcData[1][(ii-5)];
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

// *****************************************************************
/// \>  Write data to DAQ.
// *****************************************************************
#ifndef NO_DAQ
		
    // Call daqLib
    pLocalEpics->epicsOutput.daqByteCnt = 
    daqWrite(1,dcuId,daq,DAQ_RATE,testpoint,dspPtr[0],0,(int *)(pLocalEpics->epicsOutput.gdsMon),xExc,pEpicsDaq);
    // Send the current DAQ block size to the awgtpman for TP number checking
    pEpicsComms->padSpace.feDaqBlockSize = curDaqBlockSize;
    pLocalEpics->epicsOutput.tpCnt = tpPtr->count & 0xff;
    feStatus |= (FE_ERROR_EXC_SET & tpPtr->count);
    if (FE_ERROR_EXC_SET & tpPtr->count) odcStateWord |= ODC_EXC_SET;
    else odcStateWord &= ~(ODC_EXC_SET);
    if(pLocalEpics->epicsOutput.daqByteCnt > DAQ_DCU_RATE_WARNING) 
      feStatus |= FE_ERROR_DAQ;
#endif

// *****************************************************************
/// \> Cycle 19, write updated diag info to EPICS
// *****************************************************************
    if(cycleNum == HKP_DIAG_UPDATES)	
    {
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
      if(mxStat != MX_OK) feStatus |= FE_ERROR_DAQ;;
      if(pLocalEpics->epicsInput.overflowReset)
      {
        if (pLocalEpics->epicsInput.overflowReset) {
          for (ii = 0; ii < 16; ii++) {
            for (jj = 0; jj < cdsPciModules.adcCount; jj++) {
              adcinfo.overflowAdc[jj][ii] = 0;
              adcinfo.overflowAdc[jj][ii + 16] = 0;
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

// *****************************************************************
/// \> Cycle 20, Update latest DAC output values to EPICS
// *****************************************************************
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

// *****************************************************************
/// \> Cycle 21, Update ADC/DAC status to EPICS.
// *****************************************************************
    if(cycleNum == HKP_ADC_DAC_STAT_UPDATES)
    {
      pLocalEpics->epicsOutput.ovAccum = overflowAcc;
      feStatus |= adc_status_update(&adcinfo);
      feStatus |= dac_status_update(&dacinfo); 
      // If ADC channels not where they should be, we have no option but to exit
      // from the RT code ie loops would be working with wrong input data.
      if (adcinfo.chanHop) {
        pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
        pLocalEpics->epicsOutput.fe_status = CHAN_HOP_ERROR;
#if 0
        stop_working_threads = 1;
        vmeDone = 1;
        continue;    
#endif
      } else {
        pLocalEpics->epicsOutput.fe_status = NORMAL_RUN;
      }
    }

// *****************************************************************
/// \> Cycle 300, If IOP and RFM cards, check own data diagnostics
// *****************************************************************
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

// *****************************************************************
/// \> Cycle 400 to 400 + numDacModules, write DAC heartbeat to AI chassis (only for 18 bit DAC modules)
// DAC WD Write for 18 bit DAC modules
// Check once per second on code cycle 400 to dac count
// Only one write per code cycle to reduce time
// *****************************************************************
    if (cycleNum >= HKP_DAC_WD_CLK && cycleNum < (HKP_DAC_WD_CLK + cdsPciModules.dacCount)) 
    {
      if (cycleNum == HKP_DAC_WD_CLK) dacWatchDog ^= 1;
      jj = cycleNum - HKP_DAC_WD_CLK;
      if(cdsPciModules.dacType[jj] == GSC_18AO8)
      {
        volatile GSA_18BIT_DAC_REG *dac18bitPtr;
        dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[jj]);
        if(iopDacEnable && !dacChanErr[jj])
          dac18bitPtr->digital_io_ports = (dacWatchDog | GSAO_18BIT_DIO_RW);
      }
      if(cdsPciModules.dacType[jj] == GSC_20AO8)
      {
        volatile GSA_20BIT_DAC_REG *dac20bitPtr;
        dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[jj]);
        if(iopDacEnable && !dacChanErr[jj]) 
          dac20bitPtr->digital_io_ports = (dacWatchDog | GSAO_20BIT_DIO_RW);
      }
    }

// *****************************************************************
/// \> Cycle 500 to 500 + numDacModules, read back watchdog from AI chassis (18 bit DAC only)
// AI Chassis WD CHECK for 18 bit DAC modules
// Check once per second on code cycle HKP_DAC_WD_CHK to dac count
// Only one read per code cycle to reduce time
// *****************************************************************
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
          pLocalEpics->epicsOutput.statDac[jj] &= ~(DAC_WD_BIT);
        else
          pLocalEpics->epicsOutput.statDac[jj] |= DAC_WD_BIT;
      }
    }

// *****************************************************************
/// \> Cycle 600 to 600 + numDacModules, Check DAC FIFO Sizes to determine if DAC modules are synched to code 
/// - ---- 18bit DAC reads out FIFO size dirrectly, but 16bit module only has a 4 bit register 
/// area for FIFO empty, quarter full, etc. So, to make these bits useful in 16 bit module,
/// code must set a proper FIFO size in map.c code.
// This code runs once per second.
// *****************************************************************
// #ifndef NO_DAC_PRELOAD
#if !defined (NO_DAC_PRELOAD) && !defined (TIME_SLAVE)
   if (cycleNum >= HKP_DAC_FIFO_CHK && cycleNum < (HKP_DAC_FIFO_CHK + cdsPciModules.dacCount)) 
   {
      status = check_dac_buffers(cycleNum);
   }
   if (dacTimingError) feStatus |= FE_ERROR_DAC;

#endif

// *****************************************************************
// Update end of cycle information
// *****************************************************************
if(usloop == 0)
{
    // Capture end of cycle time.
    cpuClock[CPU_TIME_CYCLE_END] = rdtsc_ordered();

    /// \> Compute code cycle time diag information.
    captureEocTiming(cycleNum, cycle_gps_time, &timeinfo, &adcinfo);
    timeinfo.cycleTime = (cpuClock[CPU_TIME_CYCLE_END] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
    adcinfo.adcHoldTime = (cpuClock[CPU_TIME_CYCLE_START] - adcinfo.adcTime)/CPURATE;
    adcinfo.adcTime = cpuClock[CPU_TIME_CYCLE_START];
    // Calc the max time of one cycle of the user code
    // For IOP, more interested in time to get thru ADC read code and send to slave apps
    timeinfo.usrTime = (cpuClock[CPU_TIME_USR_START] - cpuClock[CPU_TIME_CYCLE_START])/CPURATE;
    if(timeinfo.usrTime > timeinfo.usrHoldTime) timeinfo.usrHoldTime = timeinfo.usrTime;
}

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
    } else {
      /* Increment the internal cycle counter */
      subcycle ++;                                                
    }
  }

/// \> If not exit request, then continue INFINITE LOOP  *******
  }

  pLocalEpics->epicsOutput.cpuMeter = 0;


  /* System reset command received */
  return (void *)-1;
}
