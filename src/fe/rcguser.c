///	@file moduleLoad.c
///	@brief File contains startup routines for running user app in user
///space.

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

#include "moduleLoadCommon.c"

// These externs and "16" need to go to a header file (mbuf.h)
extern int   fe_start_app_user( );
extern char  daqArea[ 2 * DAQ_DCU_SIZE ]; // Space allocation for daqLib buffers
extern char* addr;

// Scan a double
#if 0
double
simple_strtod(char *start, char **end) {
	int integer;
	if (*start != '.') {
		integer = simple_strtol(start, end, 10);
        	if (*end == start) return 0.0;
		start = *end;
	} else integer = 0;
	if (*start != '.') return integer;
	else {
		start++;
		double frac = simple_strtol(start, end, 10);
        	if (*end == start) return integer;
		int i;
		for (i = 0; i < (*end - start); i++) frac /= 10.0;
		return ((double)integer) + frac;
	}
	// Never reached
}
#endif

void
usage( )
{
    fprintf( stderr, "Usage: rcgUser -m system_name \n" );
    fprintf( stderr, "-h - help\n" );
}

/// Startup function for initialization of kernel module.
int
main( int argc, char** argv )
{
    int  status;
    int  ii, jj, kk; /// @param ii,jj,kk default loop counters
    char fname[ 128 ]; /// @param fname[128] Name of shared mem area to allocate
                       /// for DAQ data
    int cards; /// @param cards Number of PCIe cards found on bus
    int adcCnt; /// @param adcCnt Number of ADC cards found by control model.
    int dacCnt; /// @param dacCnt Number of 16bit DAC cards found by control
                /// model.
    int dac18Cnt; /// @param dac18Cnt Number of 18bit DAC cards found by control
                  /// model.
    int doCnt; /// @param doCnt Total number of digital I/O cards found by control
               /// model.
    int do32Cnt; /// @param do32Cnt Total number of Contec 32 bit DIO cards
                 /// found by control model.
    int doIIRO16Cnt; /// @param doIIRO16Cnt Total number of Acces I/O 16 bit
                     /// relay cards found by control model.
    int doIIRO8Cnt; /// @param doIIRO8Cnt Total number of Acces I/O 8 bit relay
                    /// cards found by control model.
    int cdo64Cnt; /// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit
                  /// output sections mapped by control model.
    int cdi64Cnt; /// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit
                  /// input sections mapped by control model.
    int ret; /// @param ret Return value from various Malloc calls to allocate
             /// memory.
    int   cnt;
    char* sysname;
    char  shm_name[ 64 ];
    int   c;

    while ( ( c = getopt( argc, argv, "m:help" ) ) != EOF )
        switch ( c )
        {
        case 'm':
            sysname = optarg;
            printf( "sysname = %s\n", sysname );
            break;
        case 'help':
        default:
            usage( );
            exit( 1 );
        }

    kk = 0;

    jj = 0;
    // printf("cpu clock %u\n",cpu_khz);

    sprintf( shm_name, "%s", sysname );
    findSharedMemory( sysname );
    _epics_shm = (char*)addr;
    ;
    if ( _epics_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _epics_shm );
        return -1;
    }
    printf( "EPICSM at 0x%lx\n", (long)_epics_shm );

    sprintf( shm_name, "%s", "ipc" );
    findSharedMemory( "ipc" );
    _ipc_shm = (char*)addr;
    if ( _ipc_shm < 0 )
    {
        printf( "mbuf_allocate_area(ipc) failed; ret = %d\n", _ipc_shm );
        return -1;
    }

    printf( "IPC    at 0x%lx\n", (long)_ipc_shm );
    ioMemData = (volatile IO_MEM_DATA*)( ( (char*)_ipc_shm ) + 0x4000 );
    printf(
        "IOMEM  at 0x%lx size 0x%x\n", (long)ioMemData, sizeof( IO_MEM_DATA ) );
    printf( "%d PCI cards found\n", ioMemData->totalCards );

    // If DAQ is via shared memory (Framebuilder code running on same machine or
    // MX networking is used) attach DAQ shared memory location.
    sprintf( shm_name, "%s_daq", sysname );
    findSharedMemory( shm_name );
    _daq_shm = (char*)addr;
    if ( _daq_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _daq_shm );
        return -1;
    }
    // printf("Allocated daq shmem; set at 0x%x\n", _daq_shm);
    printf( "DAQSM at 0x%lx\n", _daq_shm );
    daqPtr = (struct rmIpcStr*)_daq_shm;

    // Open new GDS TP data shared memory in support of ZMQ
    sprintf( shm_name, "%s_gds", sysname );
    findSharedMemory( shm_name );
    _gds_shm = (char*)addr;
    if ( _gds_shm < 0 )
    {
        printf( "mbuf_allocate_area() failed; ret = %d\n", _gds_shm );
        return -1;
    }
    printf( "GDSSM at 0x%lx\n", _gds_shm );

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

    // If running as a control process, I/O card information is via ipc shared
    // memory
    printf( "%d PCI cards found\n", ioMemData->totalCards );
    status = 0;
    adcCnt = 0;
    dacCnt = 0;
    dac18Cnt = 0;
    doCnt = 0;
    do32Cnt = 0;
    cdo64Cnt = 0;
    cdi64Cnt = 0;
    doIIRO16Cnt = 0;
    doIIRO8Cnt = 0;

    // Have to search thru all cards and find desired instance for application
    // Master will map ADC cards first, then DAC and finally DIO
    for ( ii = 0; ii < ioMemData->totalCards; ii++ )
    {
        /*
        printf("Model %d = %d\n",ii,ioMemData->model[ii]);
        */
        for ( jj = 0; jj < cards; jj++ )
        {
            /*
            printf("Model %d = %d, type = %d, instance = %d, dacCnt = %d \n",
                    ii,ioMemData->model[ii],
                    cdsPciModules.cards_used[jj].type,
                    cdsPciModules.cards_used[jj].instance,
                    dacCnt);
                    */
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
                    // printf("Found 6464 DIN CONTEC at %d
                    // 0x%x\n",jj,ioMemData->ipc[ii]);
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
    // This did not quite work for some reason
    // Need to find a way to handle skipped DAC cards in control
    // cdsPciModules.dacCount = ioMemData->dacCount;
    printf( "%d PCI cards found \n", status );
    if ( status < cards )
    {
        printf( " ERROR **** Did not find correct number of cards! Expected %d "
                "and Found %d\n",
                cards,
                status );
        cardCountErr = 1;
    }

    // Print out all the I/O information

    // Following section maps Reflected Memory, both VMIC hardware style and
    // Dolphin PCIe network style. Control units will perform I/O transactions
    // with RFM directly ie MASTER does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to control models.

    // Control app gets RFM module count from MASTER.
    cdsPciModules.rfmCount = ioMemData->rfmCount;
    cdsPciModules.dolphinCount = ioMemData->dolphinCount;
#if 0
    if(cdsPciModules.dolphinCount)
    {
        dolphin_init(&cdsPciModules);
    }
#endif
    for ( ii = 0; ii < cdsPciModules.rfmCount; ii++ )
    {
        cdsPciModules.pci_rfm[ ii ] = ioMemData->pci_rfm[ ii ];
        cdsPciModules.pci_rfm_dma[ ii ] = ioMemData->pci_rfm_dma[ ii ];
    }
    printf( "******************************************************************"
            "*********\n" );
    if ( cdsPciModules.gps )
    {
        printf( "IRIG-B card found %d\n", cdsPciModules.gpsType );
        printf( "**************************************************************"
                "*************\n" );
    }

    print_io_info( &cdsPciModules );

    // Initialize buffer for daqLib.c code
    printf( "Initializing space for daqLib buffers\n" );
    daqBuffer = (long)&daqArea[ 0 ];

    pLocalEpics = (CDS_EPICS*)&( (RFM_FE_COMMS*)_epics_shm )->epicsSpace;
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
    fe_start_app_user( );

    return 0;
}
