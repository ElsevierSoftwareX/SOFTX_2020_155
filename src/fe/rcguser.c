///	@file rcguser.c
///	@brief File contains startup routines for running user app in user
///space.

// TODO:
//  - Release DAC channels
//  - Verify Dolphin operation

#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "rcguserCommon.c"

// These externs and "16" need to go to a header file (mbuf.h)
extern int   fe_start_app_user( );
extern char  daqArea[ 2 * DAQ_DCU_SIZE ]; // Space allocation for daqLib buffers
extern char* addr;

/// Startup function for initialization of user space module.
int
main( int argc, char** argv )
{
    int  status = 0;
    int  ii, jj, kk; /// @param ii,jj,kk default loop counters
    char fname[ 128 ]; /// @param fname[128] Name of shared mem area to allocate
                       /// for DAQ data
    int cards; /// @param cards Number of PCIe cards found on bus
    int adcCnt = 0; /// @param adcCnt Number of ADC cards found by control model.
    int dacCnt = 0; /// @param dacCnt Number of 16bit DAC cards found by control
                /// model.
    int dac18Cnt = 0; /// @param dac18Cnt Number of 18bit DAC cards found by control
                  /// model.
    int dac20Cnt = 0; /// @param dac20Cnt Number of 20bit DAC cards found by control
                  /// model.
    int doCnt = 0; /// @param doCnt Total number of digital I/O cards found by control
               /// model.
    int do32Cnt = 0; /// @param do32Cnt Total number of Contec 32 bit DIO cards
                 /// found by control model.
    int doIIRO16Cnt = 0; /// @param doIIRO16Cnt Total number of Acces I/O 16 bit
                     /// relay cards found by control model.
    int doIIRO8Cnt = 0; /// @param doIIRO8Cnt Total number of Acces I/O 8 bit relay
                    /// cards found by control model.
    int cdo64Cnt = 0; /// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit
                  /// output sections mapped by control model.
    int cdi64Cnt = 0; /// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit
                  /// input sections mapped by control model.
    int ret; /// @param ret Return value from various Malloc calls to allocate
             /// memory.
    int   cnt;


    // Connect and allocate mbuf memory spaces
    attach_shared_memory(SYSTEM_NAME_STRING_LOWER);
    // Set pointer to EPICS shared memory
    pLocalEpics = (CDS_EPICS*)&( (RFM_FE_COMMS*)_epics_shm )->epicsSpace;

    // Find and initialize all PCI I/O modules
    // ******************************************************* Following I/O
    // card info is from feCode
    cards = sizeof( cards_used ) / sizeof( cards_used[ 0 ] );
    printf( "configured to use %d cards\n", cards );
    cdsPciModules.cards = cards;
    cdsPciModules.cards_used = cards_used;
    // return -1;
    printf( "Initializing PCI Modules\n" );
    cdsPciModules.adcCount = 0;
    cdsPciModules.dacCount = 0;
    cdsPciModules.dioCount = 0;
    cdsPciModules.doCount = 0;
    cdsPciModules.iiroDioCount = 0;
    cdsPciModules.iiroDio1Count = 0;
    cdsPciModules.cDo32lCount = 0;
    cdsPciModules.cDio1616lCount = 0;
    cdsPciModules.cDio6464lCount = 0;
    cdsPciModules.rfmCount = 0;
    cdsPciModules.dolphinCount = 0;

    // If running as a control process, I/O card information is via ipc shared
    // memory
    printf( "%d PCI cards found\n", ioMemData->totalCards );

    initmap( &cdsPciModules );
    /// Call PCI initialization routine in map.c file.
    status = mapPciModules( &cdsPciModules );

    // If no ADC cards were found, then control cannot run
    if ( !cdsPciModules.adcCount )
    {
        printf( "No ADC cards found - exiting\n" );
        return -1;
    }
    printf( "%d PCI cards found \n", status );
    if ( status < cards )
    {
        printf( " ERROR **** Did not find correct number of cards! Expected %d "
                "and Found %d\n",
                cards,
                status );
        cardCountErr = 1;
    }


    // Control app gets RFM module count from MASTER.
    cdsPciModules.rfmCount = ioMemData->rfmCount;
    cdsPciModules.dolphinCount = ioMemData->dolphinCount;
#if 0
    if(cdsPciModules.dolphinCount)
    {
        dolphin_init(&cdsPciModules);
    }
#endif
    // Print out all the I/O information
    print_io_info( &cdsPciModules,0 );

    // Initialize buffer for daqLib.c code
    printf( "Initializing space for daqLib buffers\n" );
    daqBuffer = (long)&daqArea[ 0 ];

    for ( cnt = 0; cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++ )
    {
        printf( "Epics burt restore is %d\n",
                pLocalEpics->epicsInput.burtRestore );
        usleep( 1000000 );
    }
    if ( cnt == 10 )
    {
        return -1;
    }

    pLocalEpics->epicsInput.vmeReset = 0;
    // Setup signal handler to catch Control C
    signal( SIGINT, intHandler );
    signal( SIGTERM, intHandler );
    sleep( 1 );

    // Start the control loop software
    fe_start_app_user( );

    return 0;
}
