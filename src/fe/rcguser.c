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

    // Have to search thru all cards and find desired instance for application
    // Master will map ADC cards first, then DAC and finally DIO
    for ( ii = 0; ii < ioMemData->totalCards; ii++ )
    {
        for ( jj = 0; jj < cards; jj++ )
        {
            switch ( ioMemData->model[ ii ] )
            {
            case GSC_16AI64SSA:
                if ( ( cdsPciModules.cards_used[ jj ].type == GSC_16AI64SSA ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == adcCnt ) )
                {
                    printf( "Found ADC at %d %d\n", jj, ioMemData->ipc[ ii ] );
                    kk = cdsPciModules.adcCount;
                    cdsPciModules.adcType[ kk ] = GSC_16AI64SSA;
                    cdsPciModules.adcConfig[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.adcCount++;
                    status++;
                }
                break;
            case GSC_16AO16:
                if ( ( cdsPciModules.cards_used[ jj ].type == GSC_16AO16 ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == dacCnt ) )
                {
                    printf( "Found DAC at %d %d\n", jj, ioMemData->ipc[ ii ] );
                    kk = cdsPciModules.dacCount;
                    cdsPciModules.dacType[ kk ] = GSC_16AO16;
                    cdsPciModules.dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.pci_dac[ kk ] =
                        (long)( ioMemData->iodata[ ii ] );
                    cdsPciModules.dacCount++;
                    status++;
                }
                break;
            case GSC_18AO8:
                if ( ( cdsPciModules.cards_used[ jj ].type == GSC_18AO8 ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == dac18Cnt ) )
                {
                    printf( "Found DAC at %d %d\n", jj, ioMemData->ipc[ ii ] );
                    kk = cdsPciModules.dacCount;
                    cdsPciModules.dacType[ kk ] = GSC_18AO8;
                    cdsPciModules.dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.pci_dac[ kk ] =
                        (long)( ioMemData->iodata[ ii ] );
                    cdsPciModules.dacCount++;
                    status++;
                }
                break;
            case GSC_20AO8:
                if ( ( cdsPciModules.cards_used[ jj ].type == GSC_20AO8 ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == dac20Cnt ) )
                {
                    printf( "Found DAC at %d %d\n", jj, ioMemData->ipc[ ii ] );
                    kk = cdsPciModules.dacCount;
                    cdsPciModules.dacType[ kk ] = GSC_20AO8;
                    cdsPciModules.dacConfig[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.pci_dac[ kk ] =
                        (long)( ioMemData->iodata[ ii ] );
                    cdsPciModules.dacCount++;
                    status++;
                }
                break;
            case CON_6464DIO:
                if ( ( cdsPciModules.cards_used[ jj ].type == CON_6464DIO ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == doCnt ) )
                {
                    kk = cdsPciModules.doCount;
                    printf( "Found 6464 DIO CONTEC at %d 0x%x\n",
                            jj,
                            ioMemData->ipc[ ii ] );
                    cdsPciModules.doType[ kk ] = ioMemData->model[ ii ];
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doCount++;
                    cdsPciModules.cDio6464lCount++;
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doInstance[ kk ] = doCnt;
                    status += 2;
                }
                if ( ( cdsPciModules.cards_used[ jj ].type == CDO64 ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == doCnt ) )
                {
                    kk = cdsPciModules.doCount;
                    cdsPciModules.doType[ kk ] = CDO64;
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doCount++;
                    cdsPciModules.cDio6464lCount++;
                    cdsPciModules.doInstance[ kk ] = doCnt;
                    printf( "Found 6464 DOUT CONTEC at %d 0x%x index %d \n",
                            jj,
                            ioMemData->ipc[ ii ],
                            doCnt );
                    cdo64Cnt++;
                    status++;
                }
                if ( ( cdsPciModules.cards_used[ jj ].type == CDI64 ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == doCnt ) )
                {
                    kk = cdsPciModules.doCount;
                    cdsPciModules.doType[ kk ] = CDI64;
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doInstance[ kk ] = doCnt;
                    cdsPciModules.doCount++;
                    printf( "Found 6464 DIN CONTEC at %d 0x%x index %d \n",
                            jj,
                            ioMemData->ipc[ ii ],
                            doCnt );
                    cdsPciModules.cDio6464lCount++;
                    cdi64Cnt++;
                    status++;
                }
                break;
            case CON_32DO:
                if ( ( cdsPciModules.cards_used[ jj ].type == CON_32DO ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == do32Cnt ) )
                {
                    kk = cdsPciModules.doCount;
                    printf( "Found 32 DO CONTEC at %d 0x%x\n",
                            jj,
                            ioMemData->ipc[ ii ] );
                    cdsPciModules.doType[ kk ] = ioMemData->model[ ii ];
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doCount++;
                    cdsPciModules.cDo32lCount++;
                    cdsPciModules.doInstance[ kk ] = do32Cnt;
                    status++;
                }
                break;
            case ACS_16DIO:
                if ( ( cdsPciModules.cards_used[ jj ].type == ACS_16DIO ) &&
                     ( cdsPciModules.cards_used[ jj ].instance ==
                       doIIRO16Cnt ) )
                {
                    kk = cdsPciModules.doCount;
                    printf( "Found Access IIRO-16 at %d 0x%x\n",
                            jj,
                            ioMemData->ipc[ ii ] );
                    cdsPciModules.doType[ kk ] = ioMemData->model[ ii ];
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doCount++;
                    cdsPciModules.iiroDio1Count++;
                    cdsPciModules.doInstance[ kk ] = doIIRO16Cnt;
                    status++;
                }
                break;
            case ACS_8DIO:
                if ( ( cdsPciModules.cards_used[ jj ].type == ACS_8DIO ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == doIIRO8Cnt ) )
                {
                    kk = cdsPciModules.doCount;
                    printf( "Found Access IIRO-8 at %d 0x%x\n",
                            jj,
                            ioMemData->ipc[ ii ] );
                    cdsPciModules.doType[ kk ] = ioMemData->model[ ii ];
                    cdsPciModules.pci_do[ kk ] = ioMemData->ipc[ ii ];
                    cdsPciModules.doCount++;
                    cdsPciModules.iiroDioCount++;
                    cdsPciModules.doInstance[ kk ] = doIIRO8Cnt;
                    status++;
                }
                break;
            default:
                break;
            }
        }
        if ( ioMemData->model[ ii ] == GSC_16AI64SSA )
            adcCnt++;
        if ( ioMemData->model[ ii ] == GSC_16AO16 )
            dacCnt++;
        if ( ioMemData->model[ ii ] == GSC_18AO8 )
            dac18Cnt++;
        if ( ioMemData->model[ ii ] == GSC_20AO8 )
            dac20Cnt++;
        if ( ioMemData->model[ ii ] == CON_6464DIO )
            doCnt++;
        if ( ioMemData->model[ ii ] == CON_32DO )
            do32Cnt++;
        if ( ioMemData->model[ ii ] == ACS_16DIO )
            doIIRO16Cnt++;
        if ( ioMemData->model[ ii ] == ACS_8DIO )
            doIIRO8Cnt++;
    }
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
    print_io_info( &cdsPciModules );

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
    // Start the control loop software
    fe_start_app_user( );

    return 0;
}
