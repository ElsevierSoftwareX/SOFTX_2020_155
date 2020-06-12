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

///	@file controllerAppUser.c
///	@brief Main scheduler program for compiled real-time user space object.
///\n
/// 	@detail More information can be found in the following DCC document:
///<	<a
///<href="https://dcc.ligo.org/cgi-bin/private/DocDB/ShowDocument?docid=7688">T0900607
///<CDS RT Sequencer Software</a>
///	@author R.Bork, A.Ivanov
///     @copyright Copyright (C) 2014 LIGO Project      \n
///<    California Institute of Technology              \n
///<    Massachusetts Institute of Technology           \n\n
///     @license This program is free software: you can redistribute it and/or
///     modify
///<    it under the terms of the GNU General Public License as published by
///<    the Free Software Foundation, version 3 of the License. \n This program
///<    is distributed in the hope that it will be useful, but WITHOUT ANY
///<    WARRANTY; without even the implied warranty of MERCHANTABILITY or
///<    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
///<    for more details.

/// Can't use printf in kernel module so redefine to use Linux printk function
#include <linux/version.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <drv/cdsHardware.h>
#include "inlineMath.h"

#include "fm10Gen.h" // CDS filter module defs and C code
#include "feComms.h" // Lvea control RFM network defs.
#include "daqmap.h" // DAQ network layout
#include "cds_types.h"
#include "controller.h"

#ifndef NO_DAQ
#include "drv/fb.h"
#include "drv/daqLib.c" // DAQ/GDS connection software
#endif

#include "drv/epicsXfer.c" // User defined EPICS to/from FE data transfer function
#include "drv/mapuser.h"
#include "timing.c" // timing module / IRIG-B  functions

int dacOF[ MAX_DAC_MODULES ];
#include "drv/inputFilterModule.h"
#include "drv/inputFilterModule1.h"
#include <drv/app_dac_functions.c>

// #include "dolphin_usp.c"

#define BILLION 1000000000L

// Contec 64 input bits plus 64 output bits (Standard for aLIGO)
// /// Contec6464 input register values
unsigned int CDIO6464InputInput[ MAX_DIO_MODULES ]; // Binary input bits
// /// Contec6464 - Last output request sent to module.
unsigned int CDIO6464LastOutState[ MAX_DIO_MODULES ]; // Current requested value
                                                      // of the BO bits
// /// Contec6464 values to be written to the output register
unsigned int CDIO6464Output[ MAX_DIO_MODULES ]; // Binary output bits
//
// // This Contect 16 input / 16 output DIO card is used to control timing receiver
// by IOP
// /// Contec1616 input register values
unsigned int CDIO1616InputInput[ MAX_DIO_MODULES ]; // Binary input bits
// /// Contec1616 output register values read back from the module
unsigned int CDIO1616Input[ MAX_DIO_MODULES ]; // Current value of the BO bits
// /// Contec1616 values to be written to the output register
unsigned int CDIO1616Output[ MAX_DIO_MODULES ]; // Binary output bits
// /// Holds ID number of Contec1616 DIO card(s) used for timing control.

// Include C code modules
#include "rcguser.c"
int             getGpsTime( unsigned int* tsyncSec, unsigned int* tsyncUsec );
struct timespec adcTime; ///< Used in code cycle timing
int             timeHoldHold =
    0; ///< Max code cycle time within 1 sec period; hold for another sec
struct timespec myTimer[ 2 ]; ///< Used in code cycle timing

// Clear DAC channel shared memory map,
// used to keep track of non-overlapping DAC channels among control models
//
void
deallocate_dac_channels( void )
{
    int ii, jj;
    for ( ii = 0; ii < MAX_DAC_MODULES; ii++ )
    {
        int pd = cdsPciModules.dacConfig[ ii ] - ioMemData->adcCount;
        for ( jj = 0; jj < 16; jj++ )
            if ( dacOutUsed[ ii ][ jj ] )
                ioMemData->dacOutUsed[ pd ][ jj ] = 0;
    }
}
unsigned int
getGpsTimeProc( )
{
    FILE*        timef;
    char         line[ 100 ];
    int          status;
    unsigned int mytime;

    timef = fopen( "/sys/kernel/gpstime/time", "r" );
    if ( !timef )
    {
        printf( "Cannot find GPS time \n" );
        return ( 0 );
    }
    fgets( line, 100, timef );
    mytime = atoi( line );
    printf( "GPS TIME is %d\n", mytime );
    fclose( timef );
    return ( mytime );
}

//***********************************************************************
// TASK: fe_start_app_user()
// This routine is the skeleton for all front end code
//***********************************************************************
/// This function is the main real-time sequencer or scheduler for all code
/// built using the RCG. \n There are two primary modes of operation, based on
/// two compile options: \n
///	- IOP_MODEL: Software is compiled as an I/O Processor (IOP).
///	- CONTROL_MODEL: Normal user control process.
/// This code runs in a continuous loop at the rate specified in the RCG model.
/// The loop is synchronized and triggered by the arrival of ADC data, the ADC
/// module in turn is triggered to sample by the 64KHz clock provided by the
/// Timing Distribution System.
///	-
int
fe_start_app_user( )
{
    int        longestWrite2 = 0;
    int        tempClock[ 4 ];
    int        ii, jj, kk, ll, mm; // Dummy loop counter variables
    static int clock1Min = 0; ///  @param clockMin Minute counter (Not Used??)
    struct timespec tp;
    static struct timespec
        cpuClock[ CPU_TIMER_CNT ]; ///  @param cpuClock[] Code timing diag
                                   ///  variables
    // int dacOF[MAX_DAC_MODULES];

    adcInfo_t adcInfo;
    dacInfo_t dacInfo;

    static int dacWriteEnable =
        0; /// @param dacWriteEnable  No DAC outputs until >4 times through code
           ///< Code runs longer for first few cycles on startup as it settles
           ///< in, so this helps prevent long cycles during that time.
    int limit =
        OVERFLOW_LIMIT_16BIT; /// @param limit ADC/DAC overflow test value
    int mask = GSAI_DATA_MASK; /// @param mask Bit mask for ADC/DAC read/writes
    int num_outs =
        MAX_DAC_CHN_PER_MOD; /// @param num_outs Number of DAC channels variable
    int           dkiTrip = 0;
    RFM_FE_COMMS* pEpicsComms; /// @param *pEpicsComms Pointer to EPICS shared
                               /// memory space
    // int timeHoldMax = 0;			/// @param timeHoldMax Max code cycle time since
    // last diag reset
    int myGmError2 = 0; /// @param myGmError2 Myrinet error variable
    int status; /// @param status Typical function return value
    float
        onePps; /// @param onePps Value of 1PPS signal, if used, for diagnostics
    int onePpsHi = 0; /// @param onePpsHi One PPS diagnostic check
    int onePpsTime = 0; /// @param onePpsTime One PPS diagnostic check
#ifdef DIAG_TEST
    float onePpsTest; /// @param onePpsTest Value of 1PPS signal, if used, for
                      /// diagnostics
    int onePpsHiTest[ 10 ]; /// @param onePpsHiTest[] One PPS diagnostic check
    int onePpsTimeTest[ 10 ]; /// @param onePpsTimeTest[] One PPS diagnostic
                              /// check
#endif
    int dcuId; /// @param dcuId DAQ ID number for this process
    int diagWord =
        0; /// @param diagWord Code diagnostic bit pattern returned to EPICS
    int system = 0;
    int sampleCount =
        1; /// @param sampleCount Number of ADC samples to take per code cycle
    int sync21pps = 0; /// @param sync21pps Code startup sync to 1PPS flag
    int syncSource = SYNC_SRC_MASTER; /// @param syncSource Code startup
                                      /// synchronization source
    int mxStat = 0; /// @param mxStat Net diags when myrinet express is used
    int mxDiag = 0;
    int mxDiagR = 0;
    // ****** Share data
    int    ioClockDac = DAC_PRELOAD_CNT;
    int    ioMemCntr = 0;
    int    ioMemCntrDac = DAC_PRELOAD_CNT;
    int    memCtr = 0;
    int    ioClock = 0;
    double dac_in = 0.0; /// @param dac_in DAC value after upsample filtering
    int    dac_out = 0; /// @param dac_out Integer value sent to DAC FIFO

    int feStatus = 0;

    int clk, clk1; // Used only when run on timer enabled (test mode)

    int           cnt = 0;
    unsigned long cpc;

    /// **********************************************************************************************\n
    /// Start Initialization Process \n
    /// **********************************************************************************************\n
    memset( tempClock, 0, sizeof( tempClock ) );

    /// \> Flush L1 cache
    memset( fp, 0, 64 * 1024 );
    memset( fp, 1, 64 * 1024 );

    fz_daz( ); /// \> Kill the denorms!

    /// \> Init comms with EPICS processor */
    pEpicsComms = (RFM_FE_COMMS*)_epics_shm;
    pLocalEpics = (CDS_EPICS*)&pEpicsComms->epicsSpace;
    pEpicsDaq = (char*)&( pLocalEpics->epicsOutput );
    // printf("Epics at 0x%x and DAQ at 0x%x  size = %d
    // \n",pLocalEpics,pEpicsDaq,sizeof(CDS_EPICS_IN));

#ifdef OVERSAMPLE
    /// \> Zero out filter histories
    memset( dHistory, 0, sizeof( dHistory ) );
    memset( dDacHistory, 0, sizeof( dDacHistory ) );
#endif

    /// \> Zero out DAC outputs
    for ( ii = 0; ii < MAX_DAC_MODULES; ii++ )
    {
        for ( jj = 0; jj < 16; jj++ )
        {
            dacOut[ ii ][ jj ] = 0.0;
            dacOutUsed[ ii ][ jj ] = 0;
        }
    }

    /// \> Set pointers to filter module data buffers. \n
    /// - ---- Prior to V2.8, separate local/shared memories for FILT_MOD
    /// data.\n
    /// - ---- V2.8 and later, code uses EPICS shared memory only. This was done
    /// to: \n
    /// - -------- Allow daqLib.c to retrieve filter module data directly from
    /// shared memory. \n
    /// - -------- Avoid copy of filter module data between to memory locations,
    /// which was slow. \n
    pDsp[ system ] = (FILT_MOD*)( &pEpicsComms->dspSpace );
    pCoeff[ system ] = (VME_COEF*)( &pEpicsComms->coeffSpace );
    dspPtr[ system ] = (FILT_MOD*)( &pEpicsComms->dspSpace );

    /// \> Clear the FE reset which comes from Epics
    pLocalEpics->epicsInput.vmeReset = 0;

    // Clear input masks
    pLocalEpics->epicsInput.burtRestore_mask = 0;
    pLocalEpics->epicsInput.dacDuoSet_mask = 0;

    /// < Read in all Filter Module EPICS coeff settings
    for ( ii = 0; ii < MAX_MODULES; ii++ )
    {
        checkFiltReset( ii,
                        dspPtr[ 0 ],
                        pDsp[ 0 ],
                        &dspCoeff[ 0 ],
                        MAX_MODULES,
                        pCoeff[ 0 ] );
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
    for ( system = 0; system < NUM_SYSTEMS; system++ )
    {
        for ( ii = 0; ii < MAX_MODULES; ii++ )
        {
            for ( jj = 0; jj < FILTERS; jj++ )
            {
                for ( kk = 0; kk < MAX_COEFFS; kk++ )
                {
                    dspCoeff[ system ].coeffs[ ii ].filtCoeff[ jj ][ kk ] = 0.0;
                }
                dspCoeff[ system ].coeffs[ ii ].filtSections[ jj ] = 0;
            }
        }
    }

    /// \> Initialize all filter module excitation signals to zero
    for ( system = 0; system < NUM_SYSTEMS; system++ )
        for ( ii = 0; ii < MAX_MODULES; ii++ )
            // dsp[system].data[ii].exciteInput = 0.0;
            pDsp[ 0 ]->data[ ii ].exciteInput = 0.0;

    /// \> Initialize IIR filter bank values
    if ( initVars( pDsp[ 0 ], pDsp[ 0 ], dspCoeff, MAX_MODULES, pCoeff[ 0 ] ) )
    {
        printf( "Filter module init failed, exiting\n" );
        return 0;
    }

    printf( "Initialized servo control parameters.\n" );

    usleep( 1000 );

/// \> Initialize DAQ variable/software
#if !defined( NO_DAQ ) && !defined( IOP_TASK )
    /// - ---- Set data range limits for daqLib routine
#if defined( SERVO2K ) || defined( SERVO4K )
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

    printf( "DAQ Ex Min/Max = %d %d\n", daq.filtExMin, daq.filtExMax );
    printf( "DAQ XEx Min/Max = %d %d\n", daq.xExMin, daq.xExMax );
    printf( "DAQ Tp Min/Max = %d %d\n", daq.filtTpMin, daq.filtTpMax );
    printf( "DAQ XTp Min/Max = %d %d\n", daq.xTpMin, daq.xTpMax );

    /// - ---- Assign DAC testpoint pointers
    for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
        for ( jj = 0; jj < MAX_DAC_CHN_PER_MOD;
              jj++ ) // 16 per DAC regardless of the actual
            testpoint[ MAX_DAC_CHN_PER_MOD * ii + jj ] =
                floatDacOut + MAX_DAC_CHN_PER_MOD * ii + jj;

    // Zero out storage
    memset( floatDacOut, 0, sizeof( floatDacOut ) );

#endif
    pLocalEpics->epicsOutput.ipcStat = 0;
    pLocalEpics->epicsOutput.fbNetStat = 0;
    pLocalEpics->epicsOutput.tpCnt = 0;

#if !defined( NO_DAQ ) && !defined( IOP_TASK )
    /// - ---- Initialize DAQ function
    status = daqWrite( 0,
                       dcuId,
                       daq,
                       DAQ_RATE,
                       testpoint,
                       dspPtr[ 0 ],
                       0,
                       (int*)( pLocalEpics->epicsOutput.gdsMon ),
                       xExc,
                       pEpicsDaq );
    if ( status == -1 )
    {
        printf( "DAQ init failed -- exiting\n" );
        vmeDone = 1;
        return ( 0 );
    }
#endif

    // Clear the code exit flag
    vmeDone = 0;

    /// \> Call user application software initialization routine.
    printf( "Calling feCode() to initialize\n" );
    iopDacEnable = feCode( cycleNum,
                           dWord,
                           dacOut,
                           dspPtr[ 0 ],
                           &dspCoeff[ 0 ],
                           (struct CDS_EPICS*)pLocalEpics,
                           1 );

    /// \> Verify DAC channels defined for this app are not already in use. \n
    /// - ---- User apps are allowed to share DAC modules but not DAC channels.
    // See if my DAC channel map overlaps with already running models
    for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
    {
        int pd = cdsPciModules.dacConfig[ ii ] -
            ioMemData->adcCount; // physical DAC number
        for ( jj = 0; jj < 16; jj++ )
        {
            if ( dacOutUsed[ ii ][ jj ] )
            {
                if ( ioMemData->dacOutUsed[ pd ][ jj ] )
                {
                    // vmeDone = 1;
                    printf( "Failed to allocate DAC channel.\n" );
                    printf( "DAC local %d global %d channel %d is already "
                            "allocated.\n",
                            ii,
                            pd,
                            jj );
                }
            }
        }
    }
    if ( vmeDone )
    {
        return ( 0 );
    }
    else
    {
        for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
        {
            int pd = cdsPciModules.dacConfig[ ii ] -
                ioMemData->adcCount; // physical DAC number
            for ( jj = 0; jj < MAX_DAC_CHN_PER_MOD; jj++ )
            {
                if ( dacOutUsed[ ii ][ jj ] )
                {
                    ioMemData->dacOutUsed[ pd ][ jj ] = 1;
                    printf( "Setting card local=%d global = %d channel=%d dac "
                            "usage\n",
                            ii,
                            pd,
                            jj );
                }
            }
        }
    }

    // Initialize ADC status
    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        pLocalEpics->epicsOutput.statAdc[ jj ] = 1;
    }

    // Control model needs to sync with MASTER by looking for cycle 0 count in ipc
    // memory Find memory buffer of first ADC to be used in control application.
    pLocalEpics->epicsOutput.fe_status = INIT_SYNC;
    ll = cdsPciModules.adcConfig[ 0 ];
    printf( "waiting to sync %d\n", ioMemData->iodata[ ll ][ 0 ].cycle );
    clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_CYCLE_START ] );

    // Spin until cycle 0 detected in first ADC buffer location.
    do
    {
        usleep( 1 );
    } while ( ioMemData->iodata[ ll ][ 0 ].cycle != 0 );
    timeSec = ioMemData->iodata[ ll ][ 0 ].timeSec;

    clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_CYCLE_END ] );
    timeinfo.cycleTime = BILLION *
            ( cpuClock[ CPU_TIME_CYCLE_END ].tv_sec -
              cpuClock[ CPU_TIME_CYCLE_START ].tv_sec ) +
        cpuClock[ CPU_TIME_CYCLE_END ].tv_nsec -
        cpuClock[ CPU_TIME_CYCLE_START ].tv_nsec;
    timeinfo.cycleTime /= 1000;
    printf( "Synched %d\n", timeinfo.cycleTime );
    // Need to set sync21pps so code will not try to sync with 1pps pulse later.
    sync21pps = 1;
    // Get GPS seconds from MASTER
    timeSec = ioMemData->gpsSecond;
    pLocalEpics->epicsOutput.timeDiag = timeSec;
    // Decrement GPS seconds as it will be incremented on first read cycle.
    timeSec--;

    onePpsTime = cycleNum;
    // timeSec = current_time() -1;
    timeSec = ioMemData->gpsSecond;
    // timeSec = ioMemData->iodata[0][0].timeSec;
    printf( "Using local GPS time %d \n", timeSec );
    pLocalEpics->epicsOutput.fe_status = NORMAL_RUN;

    clock_gettime( CLOCK_MONOTONIC, &adcTime );

    /// ******************************************************************************\n
    /// Enter the infinite FE control loop
    /// ******************************************\n

    /// ******************************************************************************\n
    // Calculate how many CPU cycles per code cycle
    // cpc = cpu_khz * 1000;
    // cpc /= CYCLE_PER_SECOND;

    while ( !vmeDone )
    { // Run forever until user hits reset
        // **********************************************************************************************************
        // NORMAL OPERATION -- Wait for ADC data ready
        // On startup, only want to read one sample such that first cycle
        // coincides with GPS 1PPS. Thereafter, sampleCount will be
        // increased to appropriate number of 65536 s/sec to match desired
        // code rate eg 32 samples each time thru before proceeding to match
        // 2048 system.
        // **********************************************************************************************************

        /// \> On 1PPS mark \n
        if ( cycleNum == 0 )
        {
            /// - ---- Check awgtpman status.
            // printf("awgtpman gps = %d local = %d\n",
            // pEpicsComms->padSpace.awgtpman_gps, timeSec);
            pLocalEpics->epicsOutput.awgStat =
                ( pEpicsComms->padSpace.awgtpman_gps != timeSec );
            if ( pLocalEpics->epicsOutput.awgStat )
                feStatus |= FE_ERROR_AWG;
            /// - ---- Check if DAC outputs are enabled, report error.
            if ( !iopDacEnable || dkiTrip )
                feStatus |= FE_ERROR_DAC_ENABLE;
        }
        for ( ll = 0; ll < sampleCount; ll++ )
        {
            /// \> Control model gets its adc data from MASTER via ipc shared memory\n
            for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
            {
                mm = cdsPciModules.adcConfig[ jj ];
                kk = 0;
                clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_RDY_ADC ] );
                /// - ---- Wait for proper timestamp in shared memory,
                /// indicating data ready.
                do
                {
                    clock_gettime( CLOCK_MONOTONIC,
                                   &cpuClock[ CPU_TIME_ADC_WAIT ] );
                    adcInfo.adcWait = BILLION *
                            ( cpuClock[ CPU_TIME_ADC_WAIT ].tv_sec -
                              cpuClock[ CPU_TIME_RDY_ADC ].tv_sec ) +
                        cpuClock[ CPU_TIME_ADC_WAIT ].tv_nsec -
                        cpuClock[ CPU_TIME_RDY_ADC ].tv_nsec;
                    adcInfo.adcWait /= 1000;
                } while (
                    ( ioMemData->iodata[ mm ][ ioMemCntr ].cycle != ioClock ) &&
                    ( adcInfo.adcWait < MAX_ADC_WAIT_CONTROL ) );
                timeSec = ioMemData->iodata[ mm ][ ioMemCntr ].timeSec;
                if ( cycle_gps_time == 0 )
                {
                    timeinfo.startGpsTime = timeSec;
                    pLocalEpics->epicsOutput.startgpstime =
                        timeinfo.startGpsTime;
                }
                cycle_gps_time = timeSec;

                /// - --------- If data not ready in time, set error, release
                /// DAC channel reservation and exit the code.
                if ( adcInfo.adcWait >= MAX_ADC_WAIT_CONTROL )
                {
                    printf( "ADC TIMEOUT %d %d %d %d\n",
                            mm,
                            ioMemData->iodata[ mm ][ ioMemCntr ].cycle,
                            ioMemCntr,
                            ioClock );
                    pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
                    pLocalEpics->epicsOutput.diagWord |= ADC_TIMEOUT_ERR;
                    deallocate_dac_channels( );
                    return (void*)-1;
                }
                for ( ii = 0; ii < MAX_ADC_CHN_PER_MOD; ii++ )
                {
                    /// - ---- Read data from shared memory.
                    adcInfo.adcData[ jj ][ ii ] =
                        ioMemData->iodata[ mm ][ ioMemCntr ].data[ ii ];
#ifdef FLIP_SIGNALS
                    adcInfo.adcData[ jj ][ ii ] *= -1;
#endif
                    dWord[ jj ][ ii ] = adcInfo.adcData[ jj ][ ii ];
#ifdef OVERSAMPLE
                    /// - ---- Downsample ADC data from 64K to rate of user
                    /// application
                    if ( dWordUsed[ jj ][ ii ] )
                    {
                        dWord[ jj ][ ii ] =
                            iir_filter_biquad( dWord[ jj ][ ii ],
                                               FE_OVERSAMPLE_COEFF,
                                               2,
                                               &dHistory[ ii + jj * 32 ][ 0 ] );
                    }
                    /// - ---- Check for ADC data over/under range
                    if ( ( adcInfo.adcData[ jj ][ ii ] > limit ) ||
                         ( adcInfo.adcData[ jj ][ ii ] < -limit ) )
                    {
                        adcInfo.overflowAdc[ jj ][ ii ]++;
                        pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ]++;
                        overflowAcc++;
                        adcInfo.adcOF[ jj ] = 1;
                        odcStateWord |= ODC_ADC_OVF;
                    }
#endif
                    // No filter  dWord[kk][ll] = adcData[kk][ll];
                }
            }
            /// \> Set counters for next read from ipc memory
            ioClock = ( ioClock + 1 ) % IOP_IO_RATE;
            ioMemCntr = ( ioMemCntr + 1 ) % IO_MEMORY_SLOTS;
            clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_CYCLE_START ] );
        }

        // After first synced ADC read, must set to code to read number
        // samples/cycle
        sampleCount = OVERSAMPLE_TIMES;
        /// End of ADC Read
        /// **************************************************************************************

        /// \> Call the front end specific application  ******************\n
        /// - -- This is where the user application produced by RCG gets called
        /// and executed. \n\n
        clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_USR_START ] );
        iopDacEnable = feCode( cycleNum,
                               dWord,
                               dacOut,
                               dspPtr[ 0 ],
                               &dspCoeff[ 0 ],
                               (struct CDS_EPICS*)pLocalEpics,
                               0 );
        clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_USR_END ] );

        odcStateWord = 0;
        /// WRITE DAC OUTPUTS ***************************************** \n

        /// Writing of DAC outputs is dependent on code compile option: \n
        /// - -- IOP (IOP_MODEL) reads DAC output values from memory shared
        /// with user apps and writes to DAC hardware. \n
        /// - -- USER APP (CONTROL_MODEL) sends output values to memory shared with
        /// IOP. \n

        /// START OF USER APP DAC WRITE
        /// *****************************************
        // Write out data to DAC modules
        /// \> Loop thru all DAC modules
        status = app_dac_write( ioMemCntrDac, ioClockDac, &dacinfo );
        /// \> Increment DAC memory block pointers for next cycle
        ioClockDac = ( ioClockDac + OVERSAMPLE_TIMES ) % IOP_IO_RATE;
        ioMemCntrDac = ( ioMemCntrDac + OVERSAMPLE_TIMES ) % IO_MEMORY_SLOTS;
        if ( dacWriteEnable < 10 )
            dacWriteEnable++;

        /// END OF DAC WRITE *************************************************

        /// BEGIN HOUSEKEEPING ************************************************
        /// \n

        pLocalEpics->epicsOutput.cycle = cycleNum;

        // *******************************************************************
        /// \> Cycle 18, Send timing info to EPICS at 1Hz
        // *******************************************************************
        if ( cycleNum == HKP_TIMING_UPDATES )
        {
            sendTimingDiags2Epics( pLocalEpics, &timeinfo, &adcinfo );

            if ( ( adcinfo.adcHoldTime > CYCLE_TIME_ALRM_HI ) ||
                 ( adcinfo.adcHoldTime < CYCLE_TIME_ALRM_LO ) )
            {
                diagWord |= FE_ADC_HOLD_ERR;
                feStatus |= FE_ERROR_TIMING;
            }
            if ( timeinfo.timeHoldMax > CYCLE_TIME_ALRM )
            {
                diagWord |= FE_PROC_TIME_ERR;
                feStatus |= FE_ERROR_TIMING;
            }
            pLocalEpics->epicsOutput.stateWord = feStatus;
            feStatus = 0;
            if ( pLocalEpics->epicsInput.diagReset || initialDiagReset )
            {
                initialDiagReset = 0;
                pLocalEpics->epicsInput.diagReset = 0;
                pLocalEpics->epicsInput.ipcDiagReset = 1;
                // pLocalEpics->epicsOutput.diags[1] = 0;
                timeinfo.timeHoldMax = 0;
                diagWord = 0;
                ipcErrBits = 0;

                // feStatus = 0;
            }
            // Flip the onePPS various once/sec as a watchdog monitor.
            // pLocalEpics->epicsOutput.onePps ^= 1;
            pLocalEpics->epicsOutput.diagWord = diagWord;
        }

        /// \> Check for requests for filter module clear history requests. This
        /// is spread out over a number of cycles.
        // Spread out filter coeff update, but keep updates at 16 Hz
        // here we are rounding up:
        //   x/y rounded up equals (x + y - 1) / y
        //
        static const unsigned int mpc = ( MAX_MODULES + ( FE_RATE / 16 ) - 1 ) /
            ( FE_RATE / 16 ); // Modules per cycle
        unsigned int smpc = mpc * subcycle; // Start module counter
        unsigned int empc = smpc + mpc; // End module counter
        unsigned int i;
        for ( i = smpc; i < MAX_MODULES && i < empc; i++ )
            checkFiltReset( i,
                            dspPtr[ 0 ],
                            pDsp[ 0 ],
                            &dspCoeff[ 0 ],
                            MAX_MODULES,
                            pCoeff[ 0 ] );

        /// \> Check if code exit is requested
        if ( cycleNum == MAX_MODULES )
            vmeDone = stop_working_threads |
                checkEpicsReset( cycleNum, (struct CDS_EPICS*)pLocalEpics );

/// \>  Write data to DAQ.
#ifndef NO_DAQ

        // Call daqLib
        pLocalEpics->epicsOutput.daqByteCnt =
            daqWrite( 1,
                      dcuId,
                      daq,
                      DAQ_RATE,
                      testpoint,
                      dspPtr[ 0 ],
                      myGmError2,
                      (int*)( pLocalEpics->epicsOutput.gdsMon ),
                      xExc,
                      pEpicsDaq );
        // Send the current DAQ block size to the awgtpman for TP number
        // checking
        pEpicsComms->padSpace.feDaqBlockSize = curDaqBlockSize;
        pLocalEpics->epicsOutput.tpCnt = tpPtr->count & 0xff;
        feStatus |= ( FE_ERROR_EXC_SET & tpPtr->count );
        if ( FE_ERROR_EXC_SET & tpPtr->count )
            odcStateWord |= ODC_EXC_SET;
        else
            odcStateWord &= ~( ODC_EXC_SET );
        if ( pLocalEpics->epicsOutput.daqByteCnt > DAQ_DCU_RATE_WARNING )
            feStatus |= FE_ERROR_DAQ;
#endif

        // *******************************************************************
        /// \> Cycle 19, write updated diag info to EPICS
        // *******************************************************************
        if ( cycleNum == HKP_DIAG_UPDATES )
        {
            pLocalEpics->epicsOutput.userTime = timeinfo.usrHoldTime;
            pLocalEpics->epicsOutput.ipcStat = ipcErrBits;
            if ( ipcErrBits & 0xf )
                feStatus |= FE_ERROR_IPC;
            // Create FB status word for return to EPICS
            mxStat = 0;
            mxDiagR = daqPtr->reqAck;
            if ( ( mxDiag & 1 ) != ( mxDiagR & 1 ) )
                mxStat = 1;
            if ( ( mxDiag & 2 ) != ( mxDiagR & 2 ) )
                mxStat += 2;
#ifdef DUAL_DAQ_DC
            if ( ( mxDiag & 4 ) != ( mxDiagR & 4 ) )
                mxStat += 4;
            if ( ( mxDiag & 8 ) != ( mxDiagR & 8 ) )
                mxStat += 8;
#endif
            pLocalEpics->epicsOutput.fbNetStat = mxStat;
            mxDiag = mxDiagR;
            if ( mxStat != MX_OK )
                feStatus |= FE_ERROR_DAQ;
            ;
            timeinfo.usrHoldTime = 0;
            if ( pLocalEpics->epicsInput.overflowReset )
            {
                for ( ii = 0; ii < 16; ii++ )
                {
                    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
                    {
                        adcInfo.overflowAdc[ jj ][ ii ] = 0;
                        adcInfo.overflowAdc[ jj ][ ii + 16 ] = 0;
                        pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ] = 0;
                        pLocalEpics->epicsOutput
                            .overflowAdcAcc[ jj ][ ii + 16 ] = 0;
                    }

                    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
                    {
                        pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ] = 0;
                    }
                }
            }
            if ( ( pLocalEpics->epicsInput.overflowReset ) ||
                 ( overflowAcc > OVERFLOW_CNTR_LIMIT ) )
            {
                pLocalEpics->epicsInput.overflowReset = 0;
                pLocalEpics->epicsOutput.ovAccum = 0;
                overflowAcc = 0;
            }
        }

        // *******************************************************************
        // *******************************************************************
        if ( cycleNum == 1 )
        {
            pLocalEpics->epicsOutput.timeDiag = timeSec;
        }
        // *******************************************************************
        /// \> Cycle 20, Update latest DAC output values to EPICS
        // *******************************************************************
        if ( subcycle == HKP_DAC_EPICS_UPDATES )
        {
            // Send DAC output values at 16Hzfb
            for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
            {
                for ( ii = 0; ii < MAX_DAC_CHN_PER_MOD; ii++ )
                {
                    pLocalEpics->epicsOutput.dacValue[ jj ][ ii ] =
                        dacInfo.dacOutEpics[ jj ][ ii ];
                }
            }
        }

        // *******************************************************************
        /// \> Cycle 21, Update ADC/DAC status to EPICS.
        // *******************************************************************
        if ( cycleNum == HKP_ADC_DAC_STAT_UPDATES )
        {
            pLocalEpics->epicsOutput.ovAccum = overflowAcc;
            for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
            {
                // SET/CLR Channel Hopping Error
                if ( adcInfo.adcChanErr[ jj ] )
                {
                    pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( 2 );
                    feStatus |= FE_ERROR_ADC;
                    ;
                }
                else
                    pLocalEpics->epicsOutput.statAdc[ jj ] |= 2;
                adcInfo.adcChanErr[ jj ] = 0;
                // SET/CLR Overflow Error
                if ( adcInfo.adcOF[ jj ] )
                {
                    pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( 4 );
                    feStatus |= FE_ERROR_OVERFLOW;
                    ;
                }
                else
                    pLocalEpics->epicsOutput.statAdc[ jj ] |= 4;
                adcInfo.adcOF[ jj ] = 0;
                for ( ii = 0; ii < 32; ii++ )
                {
                    if ( pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ] >
                         OVERFLOW_CNTR_LIMIT )
                    {
                        pLocalEpics->epicsOutput.overflowAdcAcc[ jj ][ ii ] = 0;
                    }
                    pLocalEpics->epicsOutput.overflowAdc[ jj ][ ii ] =
                        adcInfo.overflowAdc[ jj ][ ii ];
                    adcInfo.overflowAdc[ jj ][ ii ] = 0;
                }
            }
            for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
            {
                if ( dacOF[ jj ] )
                {
                    pLocalEpics->epicsOutput.statDac[ jj ] &=
                        ~( DAC_OVERFLOW_BIT );
                    feStatus |= FE_ERROR_OVERFLOW;
                    ;
                }
                else
                    pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_OVERFLOW_BIT;
                dacOF[ jj ] = 0;
                if ( dacChanErr[ jj ] )
                {
                    pLocalEpics->epicsOutput.statDac[ jj ] &=
                        ~( DAC_TIMING_BIT );
                }
                else
                    pLocalEpics->epicsOutput.statDac[ jj ] |= DAC_TIMING_BIT;
                dacChanErr[ jj ] = 0;
                for ( ii = 0; ii < MAX_DAC_CHN_PER_MOD; ii++ )
                {
                    if ( pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ] >
                         OVERFLOW_CNTR_LIMIT )
                    {
                        pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ] = 0;
                    }
                    pLocalEpics->epicsOutput.overflowDac[ jj ][ ii ] =
                        dacInfo.overflowDac[ jj ][ ii ];
                    dacInfo.overflowDac[ jj ][ ii ] = 0;
                }
            }
        }
        // *********************************************************************
        // Capture end of cycle time.
        // *********************************************************************

        clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_CYCLE_END ] );
        /// \> Compute code cycle time diag information.
        timeinfo.cycleTime = BILLION *
                ( cpuClock[ CPU_TIME_CYCLE_END ].tv_sec -
                  cpuClock[ CPU_TIME_CYCLE_START ].tv_sec ) +
            cpuClock[ CPU_TIME_CYCLE_END ].tv_nsec -
            cpuClock[ CPU_TIME_CYCLE_START ].tv_nsec;
        timeinfo.cycleTime /= 1000;
        // BILLION
        adcinfo.adcHoldTime = BILLION *
                ( cpuClock[ CPU_TIME_CYCLE_START ].tv_sec - adcTime.tv_sec ) +
            cpuClock[ CPU_TIME_CYCLE_START ].tv_nsec - adcTime.tv_nsec;
        adcinfo.adcHoldTime /= 1000;
        // Calc the max time of one cycle of the user code
        timeinfo.usrTime = BILLION *
                ( cpuClock[ CPU_TIME_USR_END ].tv_sec -
                  cpuClock[ CPU_TIME_CYCLE_START ].tv_sec ) +
            cpuClock[ CPU_TIME_USR_END ].tv_nsec -
            cpuClock[ CPU_TIME_CYCLE_START ].tv_nsec;
        timeinfo.usrTime /= 1000;

        // Calculate timing max/mins/etc.
        captureEocTiming( cycleNum, cycle_gps_time, &timeinfo, &adcinfo );

        pLocalEpics->epicsOutput.startgpstime = timeinfo.startGpsTime;
        adcTime.tv_sec = cpuClock[ CPU_TIME_CYCLE_START ].tv_sec;
        adcTime.tv_nsec = cpuClock[ CPU_TIME_CYCLE_START ].tv_nsec;

        /// \> Update internal cycle counters
        cycleNum += 1;
        cycleNum %= CYCLE_PER_SECOND;
        clock1Min += 1;
        clock1Min %= CYCLE_PER_MINUTE;
        if ( subcycle == DAQ_CYCLE_CHANGE )
        {
            daqCycle = ( daqCycle + 1 ) % DAQ_NUM_DATA_BLOCKS_PER_SECOND;
            if ( !( daqCycle % 2 ) )
                pLocalEpics->epicsOutput.epicsSync = daqCycle;
        }
        if ( subcycle ==
             END_OF_DAQ_BLOCK ) /*we have reached the 16Hz second barrier*/
        {
            /* Reset the data cycle counter */
            subcycle = 0;
        }
        else
        {
            /* Increment the internal cycle counter */
            subcycle++;
        }

        /// \> If not exit request, then continue INFINITE LOOP  *******
    }

    printf( "exiting from fe_code()\n" );
    pLocalEpics->epicsOutput.cpuMeter = 0;

    deallocate_dac_channels( );

    /* System reset command received */
    return (void*)-1;
}
