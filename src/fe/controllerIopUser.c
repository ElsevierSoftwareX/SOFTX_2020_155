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

///	@file controllerIopUser.c
///	@brief Main scheduler program for compiled user space IOP  object. \n
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

#include "drv/inputFilterModule.h"
#include "drv/inputFilterModule1.h"
#include <drv/dac_info.c>

#ifdef DOLPHIN_TEST
#include "dolphin.c"
#endif

#define BILLION 1000000000L

// Contec 64 input bits plus 64 output bits (Standard for aLIGO)
/// Contec6464 input register values
unsigned int CDIO6464InputInput[ MAX_DIO_MODULES ]; // Binary input bits
/// Contec6464 - Last output request sent to module.
unsigned int CDIO6464LastOutState[ MAX_DIO_MODULES ]; // Current requested value
                                                      // of the BO bits
/// Contec6464 values to be written to the output register
unsigned int CDIO6464Output[ MAX_DIO_MODULES ]; // Binary output bits

// This Contect 16 input / 16 output DIO card is used to control timing control by
// IOP
/// Contec1616 input register values
unsigned int CDIO1616InputInput[ MAX_DIO_MODULES ]; // Binary input bits
/// Contec1616 output register values read back from the module
unsigned int CDIO1616Input[ MAX_DIO_MODULES ]; // Current value of the BO bits
/// Contec1616 values to be written to the output register
unsigned int CDIO1616Output[ MAX_DIO_MODULES ]; // Binary output bits
/// Holds ID number of Contec1616 DIO card(s) used for timing control.
int tdsControl[ 3 ]; // Up to 3 timing control modules allowed in case I/O
                     // chassis are daisy chained
/// Total number of timing control modules found on bus
int tdsCount = 0;

int startGpsTime;
int cycleOffset = 0;

struct timespec myTimer[ 3 ]; ///< Used in code cycle timing
int             getGpsTime( unsigned int* tsyncSec, unsigned int* tsyncUsec );

// Include C code modules
#include "rcguserIop.c"

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
    mytime -= 1;
    return ( mytime );
}
void
initAdcModules( adcInfo_t* adcinfo )
{
    int status;
    int jj;

    for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
    {
        // Set ADC Present Flag
        pLocalEpics->epicsOutput.statAdc[ jj ] = 1;
        adcinfo->adcRdTimeErr[ jj ] = 0;
    }
    printf( "ADC setup complete \n" );
}
void
initDacModules( void )
{
    int jj;
    int status;

    for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
    {
        pLocalEpics->epicsOutput.statDac[ jj ] = DAC_FOUND_BIT;
    }
    printf( "DAC setup complete \n" );
}

//***********************************************************************
// TASK: fe_start_iop_user()
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
fe_start_iop_user( )
{
    int        longestWrite2 = 0;
    int        tempClock[ 4 ];
    int        ii, jj, kk, ll, mm; // Dummy loop counter variables
    static int clock1Min = 0; ///  @param clockMin Minute counter (Not Used??)
    struct timespec tp;
    static struct timespec
        cpuClock[ CPU_TIMER_CNT ]; ///  @param cpuClock[] Code timing diag
                                   ///  variables
    static int chanHop = 0; /// @param chanHop Adc channel hopping status

    int adcData[ MAX_ADC_MODULES ]
               [ MAX_ADC_CHN_PER_MOD ]; /// @param adcData[][]  ADC raw data
    int adcChanErr[ MAX_ADC_MODULES ];
    // int adcWait = 0;
    adcInfo_t adcInfo;
    dacInfo_t dacinfo;
    int adcOF[ MAX_ADC_MODULES ]; /// @param adcOF[]  ADC overrange counters

    static int dacWriteEnable =
        0; /// @param dacWriteEnable  No DAC outputs until >4 times through code
           ///< Code runs longer for first few cycles on startup as it settles
           ///< in, so this helps prevent long cycles during that time.
    int limit =
        OVERFLOW_LIMIT_16BIT; /// @param limit ADC/DAC overflow test value
    int mask = GSAI_DATA_MASK; /// @param mask Bit mask for ADC/DAC read/writes
    int num_outs =
        MAX_DAC_CHN_PER_MOD; /// @param num_outs Number of DAC channels variable
    volatile int*
        packedData; /// @param *packedData Pointer to ADC PCI data space
    volatile unsigned int*
        pDacData; /// @param *pDacData Pointer to DAC PCI data space
    int wtmin, wtmax; /// @param wtmin Time window for startup on IRIG-B
    int dacEnable = 0;
    int pBits[ 9 ] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256
    }; /// @param pBits[] Lookup table for quick power of 2 calcs
    int sync21ppsCycles =
        0; /// @param sync32ppsCycles Number of attempts to sync to 1PPS
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
    int        dcuId; /// @param dcuId DAQ ID number for this process
    static int missedCycle = 0; /// @param missedCycle Incremented error counter
                                /// when too many values in ADC FIFO
    int diagWord =
        0; /// @param diagWord Code diagnostic bit pattern returned to EPICS
    int system = 0;
    int sampleCount =
        1; /// @param sampleCount Number of ADC samples to take per code cycle
    int sync21pps = 0; /// @param sync21pps Code startup sync to 1PPS flag
    int syncSource =
        SYNC_SRC_NONE; /// @param syncSource Code startup synchronization source
    int mxStat = 0; /// @param mxStat Net diags when myrinet express is used
    int mxDiag = 0;
    int mxDiagR = 0;
    // ****** Share data
    int    ioClockDac = DAC_PRELOAD_CNT;
    int    ioMemCntr = 0;
    int    ioMemCntrDac = DAC_PRELOAD_CNT;
    double dac_in = 0.0; /// @param dac_in DAC value after upsample filtering
    int    dac_out = 0; /// @param dac_out Integer value sent to DAC FIFO

    int feStatus = 0;

    int clk, clk1; // Used only when run on timer enabled (test mode)

    static float duotone[ IOP_IO_RATE ]; // Duotone timing diagnostic variables
    static float duotoneDac[ IOP_IO_RATE ];
    float        duotoneTimeDac;
    float        duotoneTime;
    static float duotoneTotal = 0.0;
    static float duotoneMean = 0.0;
    static float duotoneTotalDac = 0.0;
    static float duotoneMeanDac = 0.0;
    static int   dacDuoEnable = 0;
    static int   dacTimingError = 0;
    static int   dacTimingErrorPending[ MAX_DAC_MODULES ];

    volatile GSA_18BIT_DAC_REG* dac18bitPtr; // Pointer to 16bit DAC memory area
    volatile GSC_DAC_REG*       dac16bitPtr; // Pointer to 18bit DAC memory area
    unsigned int                usec = 0;
    unsigned int                offset = 0;

    int           cnt = 0;
    unsigned long cpc;

    static int cyclensec = 15259;
    int        pdiff = 0;
    static int nextstep = 0;

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

    /// \> Zero out DAC outputs
    for ( ii = 0; ii < MAX_DAC_MODULES; ii++ )
    {
        dacTimingErrorPending[ ii ] = 0;
        for ( jj = 0; jj < 16; jj++ )
        {
            dacOut[ ii ][ jj ] = 0.0;
            dacOutUsed[ ii ][ jj ] = 0;
            dacinfo.dacOutBufSize[ ii ] = 0;
            // Zero out DAC channel map in the shared memory
            // to be used to check on control models' channel allocation
            ioMemData->dacOutUsed[ ii ][ jj ] = 0;
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

    /// \> Init code synchronization source.
    // Look for DIO card or IRIG-B Card
    // if Contec 1616 BIO present, TDS receiver will be used for timing.
    syncSource = SYNC_SRC_TIMER;

    printf( "Sync source = %d\n", syncSource );

    /// \> Wait for BURT restore.\n
    /// - ---- Code will exit if BURT flag not set by EPICS sequencer.
    // Do not proceed until EPICS has had a BURT restore
    // *******************************
    printf( "Waiting for EPICS BURT Restore = %d\n",
            pLocalEpics->epicsInput.burtRestore );
    cnt = 0;
    do
    {
        usleep( 80000 );
        printf( "Waiting for EPICS BURT %d\n", cnt++ );
    } while ( !pLocalEpics->epicsInput.burtRestore );

    printf( "BURT Restore Complete\n" );

    // BURT has completed
    // *******************************************************************

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

    // Clear timing diags.
    initializeTimingDiags( &timeinfo );
    missedCycle = 0;

    /// \> If IOP,  Initialize the ADC modules
    initAdcModules( &adcInfo );

    /// \> If IOP, Initialize the DAC module variables
    initDacModules( );

    pLocalEpics->epicsOutput.fe_status = INIT_SYNC;
    printf( "*******************************\n" );
    printf( "*     Running on timer!       *\n" );
    printf( "*******************************\n" );
    int timeoff = ( BILLION - ( 60000000 * cycleOffset ) ) % BILLION;
    /// Sync up to the 1Hz boundary
    do
    {
        usleep( 1 );
        clock_gettime( CLOCK_REALTIME, &myTimer[ 0 ] );
        timeinfo.cycleTime = myTimer[ 0 ].tv_nsec;
    } while ( timeinfo.cycleTime > timeoff &&
              timeinfo.cycleTime < ( timeoff - 10000 ) );
    printf( "cycle time = %d\n", timeinfo.cycleTime );
    timeSec = getGpsTimeProc( );
    startGpsTime = timeSec;
    pLocalEpics->epicsOutput.startgpstime = startGpsTime;
    printf(
        "Triggered the ADC at %d and %d usec\n", timeSec, timeinfo.cycleTime );
    onePpsTime = cycleNum;

    nextstep = timeoff + 20000;

    /// ******************************************************************************\n
    /// Enter the infinite FE control loop
    /// ******************************************\n

    /// ******************************************************************************\n

    pLocalEpics->epicsOutput.fe_status = NORMAL_RUN;

    while ( !vmeDone )
    { // Run forever until user hits reset
        // This is local CPU timer (no ADCs)
        // advance to the next cycle polling CPU cycles and microsleeping
        do
        {
            clock_gettime( CLOCK_MONOTONIC, &myTimer[ 1 ] );
            clk = myTimer[ 1 ].tv_nsec;
        } while ( clk < nextstep );
        adcinfo.adcHoldTime =
            ( myTimer[ 1 ].tv_nsec - myTimer[ 0 ].tv_nsec ) / 1000;
        if ( adcinfo.adcHoldTime < 0 )
            adcinfo.adcHoldTime += 1000000;
        myTimer[ 0 ].tv_sec = myTimer[ 1 ].tv_sec;
        myTimer[ 0 ].tv_nsec = myTimer[ 1 ].tv_nsec;

        ioMemCntr = ( cycleNum % IO_MEMORY_SLOTS );
        // Write GPS time and cycle count as indicator to control app that adc data is
        // ready
        for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
        {
            for ( ii = 0; ii < IO_MEMORY_SLOT_VALS; ii++ )
            {
                ioMemData->iodata[ jj ][ ioMemCntr ].data[ ii ] =
                    ioMemDataIop->iodata[ jj ][ cycleNum ].data[ ii ];
                dWord[ jj ][ ii ] =
                    ioMemData->iodata[ jj ][ ioMemCntr ].data[ ii ];
                ioMemDataIop->iodata[ jj ][ cycleNum ].data[ ii ] = 0;
            }
            ioMemData->iodata[ jj ][ ioMemCntr ].timeSec = timeSec;
            ;
            ioMemData->iodata[ jj ][ ioMemCntr ].cycle = cycleNum;
            ioMemDataIop->gpsSecond = timeSec;
            ioMemDataIop->cycleNum = cycleNum;
        }
        ioMemData->gpsSecond = timeSec;
        clock_gettime( CLOCK_MONOTONIC, &cpuClock[ CPU_TIME_CYCLE_START ] );

        if ( cycleNum == 0 )
        {
            pLocalEpics->epicsOutput.awgStat =
                ( pEpicsComms->padSpace.awgtpman_gps != timeSec );
            if ( pLocalEpics->epicsOutput.awgStat )
                feStatus |= FE_ERROR_AWG;
            // Increment GPS second on cycle 0
            timeSec++;
            pLocalEpics->epicsOutput.timeDiag = timeSec;
            int pll =
                myTimer[ 0 ].tv_nsec / 1000 + ( 1000000 - ( timeoff / 1000 ) );
            pll %= 1000000;
            if ( pll > 500000 )
                pdiff = ( 1000000 - pll ) * -1;
            else
                pdiff = pll;
            pLocalEpics->epicsOutput.irigbTime = pdiff + 10;
            // pLocalEpics->epicsOutput.irigbTime = (pdiff + (timeoff / 1000) +
            // 11) ;
            cycle_gps_time = timeSec;
        }

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
/// - -- IOP (IOP_MODEL) reads DAC output values from memory shared with user
/// apps and writes to DAC hardware. \n
/// - -- USER APP (CONTROL_MODEL) sends output values to memory shared with IOP. \n

/// START OF IOP DAC WRITE ***************************************** \n
/// \> If DAC FIFO error, always output zero to DAC modules. \n
/// - -- Code will require restart to clear.
// COMMENT OUT NEX LINE FOR TEST STAND w/bad DAC cards.
#ifndef DAC_WD_OVERRIDE
        if ( dacTimingError )
            iopDacEnable = 0;
#endif
        // Write out data to DAC modules
        dkiTrip = 0;
        /// \> Loop thru all DAC modules
        for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
        {
            /// - -- Point to DAC memory buffer
            // pDacData = (unsigned int *)(cdsPciModules.pci_dac[jj]);
            /// - -- locate the proper DAC memory block
            mm = cdsPciModules.dacConfig[ jj ];
            /// - -- Determine if memory block has been set with the correct
            /// cycle count by control app.
            if ( ioMemData->iodata[ mm ][ ioMemCntrDac ].cycle == ioClockDac )
            {
                dacEnable |= pBits[ jj ];
            }
            else
            {
                dacEnable &= ~( pBits[ jj ] );
                dacChanErr[ jj ] += 1;
            }
            /// - -- Set overflow limits, data mask, and chan count based on DAC
            /// type
            limit = OVERFLOW_LIMIT_16BIT;
            mask = GSAO_16BIT_MASK;
            num_outs = GSAO_16BIT_CHAN_COUNT;
            if ( cdsPciModules.dacType[ jj ] == GSC_18AO8 )
            {
                limit = OVERFLOW_LIMIT_18BIT; // 18 bit limit
                mask = GSAO_18BIT_MASK;
                num_outs = GSAO_18BIT_CHAN_COUNT;
            }
            /// - -- For each DAC channel
            for ( ii = 0; ii < num_outs; ii++ )
            {
#ifdef FLIP_SIGNALS
                dacOut[ jj ][ ii ] *= -1;
#endif
                /// - ---- Read DAC output value from shared memory and reset
                /// memory to zero
                if ( ( !dacChanErr[ jj ] ) && ( iopDacEnable ) )
                {
                    dac_out =
                        ioMemData->iodata[ mm ][ ioMemCntrDac ].data[ ii ];
                    /// - --------- Zero out data in case user app dies by next
                    /// cycle when two or more apps share same DAC module.
                    ioMemData->iodata[ mm ][ ioMemCntrDac ].data[ ii ] = 0;
                }
                else
                {
                    dac_out = 0;
                    dkiTrip = 1;
                }
                /// - ----  Write out ADC duotone signal to first DAC, last
                /// channel, if DAC duotone is enabled.
                if ( ( dacDuoEnable ) && ( ii == ( num_outs - 1 ) ) &&
                     ( jj == 0 ) )
                {
                    dac_out = adcData[ 0 ][ ADC_DUOTONE_CHAN ];
                }
                /// - ---- Check output values are within range of DAC \n
                /// - --------- If overflow, clip at DAC limits and report
                /// errors
                if ( dac_out > limit || dac_out < -limit )
                {
                    dacinfo.overflowDac[ jj ][ ii ]++;
                    pLocalEpics->epicsOutput.overflowDacAcc[ jj ][ ii ]++;
                    overflowAcc++;
                    dacinfo.dacOF[ jj ] = 1;
                    odcStateWord |= ODC_DAC_OVF;
                    ;
                    if ( dac_out > limit )
                        dac_out = limit;
                    else
                        dac_out = -limit;
                }
                /// - ---- If DAQKILL tripped, set output to zero.
                if ( !iopDacEnable )
                    dac_out = 0;
                /// - ---- Load last values to EPICS channels for monitoring on
                /// GDS_TP screen.
                dacinfo.dacOutEpics[ jj ][ ii ] = dac_out;

                /// - ---- Load DAC testpoints
                floatDacOut[ 16 * jj + ii ] = dac_out;

                /// - ---- Write to DAC local memory area, for later xmit to DAC
                /// module
                // *pDacData =  (unsigned int)(dac_out & mask);
                // pDacData ++;
            }
            /// - -- Mark cycle count as having been used -1 \n
            /// - --------- Forces control apps to mark this cycle or will not be used
            /// again by Master
            ioMemData->iodata[ mm ][ ioMemCntrDac ].cycle = -1;
            /// - -- DMA Write data to DAC module
        }
        /// \> Increment DAC memory block pointers for next cycle
        ioClockDac = ( ioClockDac + 1 ) % IOP_IO_RATE;
        ioMemCntrDac = ( ioMemCntrDac + 1 ) % IO_MEMORY_SLOTS;
        if ( dacWriteEnable < 10 )
            dacWriteEnable++;
        /// END OF IOP DAC WRITE
        /// *************************************************

        /// BEGIN HOUSEKEEPING ************************************************
        /// \n

        pLocalEpics->epicsOutput.cycle = cycleNum;

        /// \> Cycle 18, Send timing info to EPICS at 1Hz
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
                for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
                    adcInfo.adcRdTimeMax[ jj ] = 0;
            }
            // Flip the onePPS various once/sec as a watchdog monitor.
            // pLocalEpics->epicsOutput.onePps ^= 1;
            pLocalEpics->epicsOutput.diagWord = diagWord;
            for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
            {
                if ( adcInfo.adcRdTimeErr[ jj ] > MAX_ADC_WAIT_ERR_SEC )
                    pLocalEpics->epicsOutput.stateWord |= FE_ERROR_ADC;
                adcInfo.adcRdTimeErr[ jj ] = 0;
            }
        }

        /// \> Check for requests for filter module clear history requests. This
        /// is spread out over a number of cycles.
        // Spread out filter coeff update, but keep updates at 16 Hz
        // here we are rounding up:
        //   x/y rounded up equals (x + y - 1) / y
        //
        {
            static const unsigned int mpc =
                ( MAX_MODULES + ( FE_RATE / 16 ) - 1 ) /
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
        }

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
            pLocalEpics->epicsOutput.fbNetStat = mxStat;
            mxDiag = mxDiagR;
            if ( mxStat != MX_OK )
                feStatus |= FE_ERROR_DAQ;
            ;
            timeinfo.usrHoldTime = 0;
            if ( pLocalEpics->epicsInput.overflowReset )
            {
                if ( pLocalEpics->epicsInput.overflowReset )
                {
                    for ( ii = 0; ii < 16; ii++ )
                    {
                        for ( jj = 0; jj < cdsPciModules.adcCount; jj++ )
                        {
                            adcInfo.overflowAdc[ jj ][ ii ] = 0;
                            adcInfo.overflowAdc[ jj ][ ii + 16 ] = 0;
                            pLocalEpics->epicsOutput
                                .overflowAdcAcc[ jj ][ ii ] = 0;
                            pLocalEpics->epicsOutput
                                .overflowAdcAcc[ jj ][ ii + 16 ] = 0;
                        }

                        for ( jj = 0; jj < cdsPciModules.dacCount; jj++ )
                        {
                            pLocalEpics->epicsOutput
                                .overflowDacAcc[ jj ][ ii ] = 0;
                        }
                    }
                }
            }
            if ( ( pLocalEpics->epicsInput.overflowReset ) ||
                 ( adcInfo.overflowAdc > OVERFLOW_CNTR_LIMIT ) )
            {
                pLocalEpics->epicsInput.overflowReset = 0;
                pLocalEpics->epicsOutput.ovAccum = 0;
                overflowAcc = 0;
            }
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
                        dacinfo.dacOutEpics[ jj ][ ii ];
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
                if ( adcChanErr[ jj ] )
                {
                    pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( 2 );
                    feStatus |= FE_ERROR_ADC;
                    ;
                }
                else
                    pLocalEpics->epicsOutput.statAdc[ jj ] |= 2;
                adcChanErr[ jj ] = 0;
                // SET/CLR Overflow Error
                if ( adcOF[ jj ] )
                {
                    pLocalEpics->epicsOutput.statAdc[ jj ] &= ~( 4 );
                    feStatus |= FE_ERROR_OVERFLOW;
                    ;
                }
                else
                    pLocalEpics->epicsOutput.statAdc[ jj ] |= 4;
                adcOF[ jj ] = 0;
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
            // If ADC channels not where they should be, we have no option but
            // to exit from the RT code ie loops would be working with wrong
            // input data.
            if ( chanHop )
            {
                pLocalEpics->epicsOutput.stateWord = FE_ERROR_ADC;
                stop_working_threads = 1;
                vmeDone = 1;
                printf( "Channel Hopping Detected on one or more ADC modules "
                        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
                printf( "Check GDSTP screen ADC status bits to id affected ADC "
                        "modules\n" );
                printf( "Code is exiting ..............\n" );
                continue;
            }
            feStatus |= dac_status_update( &dacinfo );
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
        pLocalEpics->epicsOutput.startgpstime = startGpsTime;
        // Calc the max time of one cycle of the user code
        // For IOP, more interested in time to get thru ADC read code and send
        // to control apps
        timeinfo.usrTime = BILLION *
                ( cpuClock[ CPU_TIME_USR_END ].tv_sec -
                  cpuClock[ CPU_TIME_CYCLE_START ].tv_sec ) +
            cpuClock[ CPU_TIME_USR_END ].tv_nsec -
            cpuClock[ CPU_TIME_CYCLE_START ].tv_nsec;
        timeinfo.usrTime /= 1000;

        // Calculate timing max/mins/etc.
        captureEocTiming( cycleNum, cycle_gps_time, &timeinfo, &adcinfo );

        /// \> Update internal cycle counters
        cycleNum += 1;
        cycleNum %= CYCLE_PER_SECOND;
        nextstep = ( ( cycleNum * cyclensec ) + timeoff ) % 1000000000;
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

    /* System reset command received */
    return (void*)-1;
}
