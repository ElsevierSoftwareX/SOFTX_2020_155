///	@file rcguserIop.c
///	@brief File contains startup routines for IOP running in user space.

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

#include "rcguserCommon.c"

// These externs and "16" need to go to a header file (mbuf.h)
extern int   fe_start_iop_user( );
extern char  daqArea[ 2 * DAQ_DCU_SIZE ]; // Space allocation for daqLib buffers
extern char* addr;
extern int   cycleOffset;

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
    int  ii, jj, kk, mm; /// @param ii,jj,kk default loop counters
    char fname[ 128 ]; /// @param fname[128] Name of shared mem area to allocate
                       /// for DAQ data
    int cards; /// @param cards Number of PCIe cards found on bus
    int do32Cnt = 0; /// @param do32Cnt Total number of Contec 32 bit DIO cards
                 /// found by control model.
    int doIIRO16Cnt = 0; /// @param doIIRO16Cnt Total number of Acces I/O 16 bit
                     /// relay cards found by control model.
    int doIIRO8Cnt = 0; /// @param doIIRO8Cnt Total number of Acces I/O 8 bit relay
                    /// cards found by control model.
    int ret; /// @param ret Return value from various Malloc calls to allocate
             /// memory.
    int   cnt;
    char* sysname;
    char  shm_name[ 64 ];
    int   c;
    char* modelname;

    cycleOffset = 0;

#if 0
    while ( ( c = getopt( argc, argv, "m:t:help" ) ) != EOF )
        switch ( c )
        {
        case 'm':
            sysname = optarg;
            printf( "sysname = %s\n", sysname );
            break;
        case 't':
            cycleOffset = atoi( optarg );
            printf( "cycle offset = %d\n", cycleOffset );
            break;
        case 'help':
        default:
            usage( );
            exit( 1 );
        }
#endif

    kk = 0;

    jj = 0;

    attach_shared_memory(SYSTEM_NAME_STRING_LOWER);
    pLocalEpics = (CDS_EPICS*)&( (RFM_FE_COMMS*)_epics_shm )->epicsSpace;

    // Find and initialize all PCI I/O modules
    // ******************************************************* Following I/O
    // card info is from feCode
    cards = sizeof( cards_used ) / sizeof( cards_used[ 0 ] );
    printf( "configured to use %d cards\n", cards );
    cdsPciModules.cards = cards;
    cdsPciModules.cards_used = cards_used;
    // return -1;
    printf( "Initializing PCI Modules for IOP\n" );
    for ( jj = 0; jj < cards; jj++ )
        printf(
            "Card %d type = %d\n", jj, cdsPciModules.cards_used[ jj ].type );
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
    printf( "%d PCI cards found\n", cards );
    status = 0;

    ioMemData->totalCards = cards;

    // Have to search thru all cards and find desired instance for application
    // Master will map ADC cards first, then DAC and finally DIO
    for ( jj = 0; jj < cards; jj++ )
    {
        /*
        printf("Model %d = %d, type = %d, instance = %d, dacCnt = %d \n",
                ii,ioMemData->model[ii],
                cdsPciModules.cards_used[jj].type,
                cdsPciModules.cards_used[jj].instance,
                dacCnt);
                */
        switch ( cdsPciModules.cards_used[ jj ].type )
        {
        case GSC_16AI64SSA:
            kk = cdsPciModules.adcCount;
            cdsPciModules.adcType[ kk ] = GSC_16AI64SSA;
            cdsPciModules.adcCount++;
            status++;
            break;
        case GSC_16AO16:
            kk = cdsPciModules.dacCount;
            cdsPciModules.dacType[ kk ] = GSC_16AO16;
            cdsPciModules.dacCount++;
            status++;
            break;
        case GSC_18AO8:
            kk = cdsPciModules.dacCount;
            cdsPciModules.dacType[ kk ] = GSC_18AO8;
            cdsPciModules.dacCount++;
            status++;
            break;
        case GSC_20AO8:
            kk = cdsPciModules.dacCount;
            cdsPciModules.dacType[ kk ] = GSC_20AO8;
            cdsPciModules.dacCount++;
            status++;
            break;
        case CON_6464DIO:
            kk = cdsPciModules.doCount;
            cdsPciModules.doType[ kk ] = CON_6464DIO;
            cdsPciModules.doCount++;
            cdsPciModules.cDio6464lCount++;
            // cdsPciModules.doInstance[kk] = cDio6464lCount;
            status += 2;
            break;
        case CON_32DO:
            kk = cdsPciModules.doCount;
            cdsPciModules.doType[ kk ] = CON_32DO;
            cdsPciModules.doCount++;
            cdsPciModules.cDo32lCount++;
            cdsPciModules.doInstance[ kk ] = do32Cnt;
            status++;
            break;
        case ACS_16DIO:
            kk = cdsPciModules.doCount;
            cdsPciModules.doCount++;
            cdsPciModules.iiroDio1Count++;
            cdsPciModules.doInstance[ kk ] = doIIRO16Cnt;
            status++;
            break;
        case ACS_8DIO:
            kk = cdsPciModules.doCount;
            cdsPciModules.doCount++;
            cdsPciModules.iiroDioCount++;
            cdsPciModules.doInstance[ kk ] = doIIRO8Cnt;
            status++;
            break;
        default:
            break;
        }
    }
    // If no ADC cards were found, then control models cannot run
    if ( !cdsPciModules.adcCount )
    {
        printf( "No ADC cards found - exiting\n" );
        return -1;
    }
    // This did not quite work for some reason
    // Need to find a way to handle skipped DAC cards in controllers
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
    printf( "******************************************************************"
            "*********\n" );
    // Master send module counds to control models via ipc shm
    ioMemData->totalCards = status;
    ioMemData->adcCount = cdsPciModules.adcCount;
    ioMemData->dacCount = cdsPciModules.dacCount;
    ioMemData->bioCount = cdsPciModules.doCount;
    // kk will act as ioMem location counter for mapping modules
    kk = cdsPciModules.adcCount;
    printf( "%d ADC cards found\n", cdsPciModules.adcCount );
    for ( ii = 0; ii < cdsPciModules.adcCount; ii++ )
    {
        // MASTER maps ADC modules first in ipc shm for control models
        ioMemData->model[ ii ] = cdsPciModules.adcType[ ii ];
        ioMemData->ipc[ ii ] = ii; 
        // ioData memory buffer for virtual I/O cards
        ioMemDataIop->model[ ii ] = cdsPciModules.adcType[ ii ];
        ioMemDataIop->ipc[ ii ] = ii; 
        // ioData fill virtual ADC mem with dummy data (65536 x 32 chans)
        for ( jj = 0; jj < 65536; jj++ )
        {
            for ( mm = 0; mm < 32; mm++ )
            {
                ioMemDataIop->iodata[ ii ][ jj ].data[ mm ] = jj - 32767;
            }
        }
        if ( cdsPciModules.adcType[ ii ] == GSC_16AI64SSA )
        {
            printf( "\tADC %d is a GSC_16AI64SSA module\n", ii );
            if ( ( cdsPciModules.adcConfig[ ii ] & 0x10000 ) > 0 )
                jj = 32;
            else
                jj = 64;
            printf( "\t\tChannels = %d \n", jj );
            printf( "\t\tFirmware Rev = %d \n\n",
                    ( cdsPciModules.adcConfig[ ii ] & 0xfff ) );
        }
    }
    printf( "******************************************************************"
            "*********\n" );
    printf( "%d DAC cards found\n", cdsPciModules.dacCount );
    for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
    {
        if ( cdsPciModules.dacType[ ii ] == GSC_18AO8 )
        {
            printf( "\tDAC %d is a GSC_18AO8 module\n", ii );
        }
        if ( cdsPciModules.dacType[ ii ] == GSC_16AO16 )
        {
            printf( "\tDAC %d is a GSC_16AO16 module\n", ii );
            if ( ( cdsPciModules.dacConfig[ ii ] & 0x10000 ) == 0x10000 )
                jj = 8;
            if ( ( cdsPciModules.dacConfig[ ii ] & 0x20000 ) == 0x20000 )
                jj = 12;
            if ( ( cdsPciModules.dacConfig[ ii ] & 0x30000 ) == 0x30000 )
                jj = 16;
            printf( "\t\tChannels = %d \n", jj );
            if ( ( cdsPciModules.dacConfig[ ii ] & 0xC0000 ) == 0x0000 )
            {
                printf( "\t\tFilters = None\n" );
            }
            if ( ( cdsPciModules.dacConfig[ ii ] & 0xC0000 ) == 0x40000 )
            {
                printf( "\t\tFilters = 10kHz\n" );
            }
            if ( ( cdsPciModules.dacConfig[ ii ] & 0xC0000 ) == 0x80000 )
            {
                printf( "\t\tFilters = 100kHz\n" );
            }
            if ( ( cdsPciModules.dacConfig[ ii ] & 0x100000 ) == 0x100000 )
            {
                printf( "\t\tOutput Type = Differential\n" );
            }
            printf( "\t\tFirmware Rev = %d \n\n",
                    ( cdsPciModules.dacConfig[ ii ] & 0xfff ) );
        }
        // Pass DAC info to control processes
        ioMemData->model[ kk ] = cdsPciModules.dacType[ ii ];
        ioMemData->ipc[ kk ] = kk;
        // Following used by IOP to point to ipc memory for inputting DAC
        // data from control models
        cdsPciModules.dacConfig[ ii ] = kk;
        printf( "MASTER DAC SLOT %d %d\n", ii, cdsPciModules.dacConfig[ ii ] );
        kk++;
    }
    printf( "******************************************************************"
            "*********\n" );
    printf( "%d DIO cards found\n", cdsPciModules.dioCount );
    printf( "******************************************************************"
            "*********\n" );
    printf( "%d IIRO-8 Isolated DIO cards found\n",
            cdsPciModules.iiroDioCount );
    printf( "******************************************************************"
            "*********\n" );
    printf( "%d IIRO-16 Isolated DIO cards found\n",
            cdsPciModules.iiroDio1Count );
    printf( "******************************************************************"
            "*********\n" );
    printf( "%d Contec 32ch PCIe DO cards found\n", cdsPciModules.cDo32lCount );
    printf( "%d Contec PCIe DIO1616 cards found\n",
            cdsPciModules.cDio1616lCount );
    printf( "%d Contec PCIe DIO6464 cards found\n",
            cdsPciModules.cDio6464lCount );
    printf( "%d DO cards found\n", cdsPciModules.doCount );
    // IOP sends DIO module information to control models
    // Note that for DIO, control modules will perform the I/O directly and
    // therefore need to know the PCIe address of these modules.
    ioMemData->bioCount = cdsPciModules.doCount;
    for ( ii = 0; ii < cdsPciModules.doCount; ii++ )
    {
        // MASTER needs to find Contec 1616 I/O card to control timing receiver.
        if ( cdsPciModules.doType[ ii ] == CON_1616DIO )
        {
            tdsControl[ tdsCount ] = ii;
            printf( "TDS controller %d is at %d\n", tdsCount, ii );
            tdsCount++;
        }
        ioMemData->model[ kk ] = cdsPciModules.doType[ ii ];
        // Unlike ADC and DAC, where a memory buffer number is passed, a PCIe
        // address is passed for DIO cards.
        ioMemData->ipc[ kk ] = kk;
        kk++;
    }
    printf( "Total of %d I/O modules found and mapped\n", kk );
    printf( "******************************************************************"
            "*********\n" );
    print_io_info(&cdsPciModules,1);
    // Following section maps Reflected Memory, both VMIC hardware style and
    // Dolphin PCIe network style. Control units will perform I/O transactions
    // with RFM directly ie IOP does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to control models.

    ioMemData->rfmCount = 9;
#ifdef DOLPHIN_TEST
    ioMemData->dolphinCount = cdsPciModules.dolphinCount;
    ioMemData->dolphinRead[ 0 ] = cdsPciModules.dolphinRead[ 0 ];
    ioMemData->dolphinWrite[ 0 ] = cdsPciModules.dolphinWrite[ 0 ];

#else
    // Clear Dolphin pointers so the control app sees NULLs
    ioMemData->dolphinCount = 0;
    ioMemData->dolphinRead[ 0 ] = 0;
    ioMemData->dolphinWrite[ 0 ] = 0;
#endif

    // Initialize buffer for daqLib.c code
    printf( "Initializing space for daqLib buffers\n" );
    daqBuffer = (long)&daqArea[ 0 ];

    // Wait for SDF restore
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
    fe_start_iop_user( );

    return 0;
}
