///	@file moduleLoadApp.c
///	@brief File contains startup routines for real-time user app code.

#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/spinlock_types.h>

// These externs and "16" need to go to a header file (mbuf.h)
extern void* kmalloc_area[ 16 ];
extern int   mbuf_allocate_area( char* name, int size, struct file* file );
extern void* fe_start_app( void* arg );
extern char  daqArea[ 2 * DAQ_DCU_SIZE ]; // Space allocation for daqLib buffers

// MAIN routine: Code starting point
// ****************************************************************
extern int need_to_load_IOP_first;

extern void         set_fe_code_idle( void* ( *ptr )(void*), unsigned int cpu );
extern void         msleep( unsigned int );
struct task_struct* sthread;

#include "moduleLoadCommon.c"

/// Startup function for initialization of kernel module.
int
rt_fe_init( void )
{
    int  status;
    int  ii, jj, kk; /// @param ii,jj,kk default loop counters
    char fname[ 128 ]; /// @param fname[128] Name of shared mem area to allocate
                       /// for DAQ data
    int cards; /// @param cards Number of PCIe cards found on bus
    int adcCnt; /// @param adcCnt Number of ADC cards found by slave model.
    int dacCnt; /// @param dacCnt Number of 16bit DAC cards found by slave
                /// model.
    int dac18Cnt; /// @param dac18Cnt Number of 18bit DAC cards found by slave
                  /// model.
    int dac20Cnt; /// @param dac20Cnt Number of 20bit DAC cards found by slave
                  /// model.
    int doCnt; /// @param doCnt Total number of digital I/O cards found by slave
               /// model.
    int do32Cnt; /// @param do32Cnt Total number of Contec 32 bit DIO cards
                 /// found by slave model.
    int doIIRO16Cnt; /// @param doIIRO16Cnt Total number of Acces I/O 16 bit
                     /// relay cards found by slave model.
    int doIIRO8Cnt; /// @param doIIRO8Cnt Total number of Acces I/O 8 bit relay
                    /// cards found by slave model.
    int cdo64Cnt; /// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit
                  /// output sections mapped by slave model.
    int cdi64Cnt; /// @param cdo64Cnt Total number of Contec 6464 DIO card 32bit
                  /// input sections mapped by slave model.
    int ret; /// @param ret Return value from various Malloc calls to allocate
             /// memory.
    int        cnt;
    extern int cpu_down( unsigned int ); /// @param cpu_down CPU shutdown call.
    extern int is_cpu_taken_by_rcg_model(
        unsigned int cpu ); /// @param is_cpu_taken_by_rcg_model Check to verify
                            /// CPU availability for shutdown.

    kk = 0;
#ifdef SPECIFIC_CPU
#define CPUID SPECIFIC_CPU
#else
#define CPUID 1
#endif

#ifndef NO_CPU_SHUTDOWN
    // See if our CPU core is free
    if ( is_cpu_taken_by_rcg_model( CPUID ) )
    {
        printk( KERN_ALERT "Error: CPU %d already taken\n", CPUID );
        return -1;
    }
#endif

    need_to_load_IOP_first = 0;

    jj = 0;

    // Allocate EPICS shmem area
    ret = mbuf_allocate_area( SYSTEM_NAME_STRING_LOWER, 64 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: mbuf_allocate_area(epics) failed; ret = %d\n", ret );
        return -12;
    }
    _epics_shm = (unsigned char*)( kmalloc_area[ ret ] );
    // Allocate IPC shmem area
    ret = mbuf_allocate_area( "ipc", 16 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: mbuf_allocate_area(ipc) failed; ret = %d\n", ret );
        return -12;
    }
    _ipc_shm = (unsigned char*)( kmalloc_area[ ret ] );

    // Point to IOP/APP comms shmem area
    ioMemData = (IO_MEM_DATA*)( _ipc_shm + 0x4000 );

    // Allocate DAQ shmem area
    sprintf( fname, "%s_daq", SYSTEM_NAME_STRING_LOWER );
    ret = mbuf_allocate_area( fname, 64 * 1024 * 1024, 0 );
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR:mbuf_allocate_area() failed; ret = %d\n", ret );
        return -12;
    }
    _daq_shm = (unsigned char*)( kmalloc_area[ ret ] );
    daqPtr = (struct rmIpcStr*)_daq_shm;

    // Find and initialize all PCI I/O modules
    // ******************************************************* Following I/O
    // card info is from feCode
    cards = sizeof( cards_used ) / sizeof( cards_used[ 0 ] );
    cdsPciModules.cards = cards;
    cdsPciModules.cards_used = cards_used;
    // return -1;
    cdsPciModules.adcCount = 0;
    cdsPciModules.dacCount = 0;
    cdsPciModules.dioCount = 0;
    cdsPciModules.doCount = 0;

    // If running as a slave process, I/O card information is via ipc shared
    // memory
    status = 0;
    adcCnt = 0;
    dacCnt = 0;
    dac18Cnt = 0;
    dac20Cnt = 0;
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
        for ( jj = 0; jj < cards; jj++ )
        {
            switch ( ioMemData->model[ ii ] )
            {
            case GSC_16AI64SSA:
                if ( ( cdsPciModules.cards_used[ jj ].type == GSC_16AI64SSA ) &&
                     ( cdsPciModules.cards_used[ jj ].instance == adcCnt ) )
                {
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
                if ( cdsPciModules.cards_used[ jj ].type == GSC_20AO8 &&
                     ( cdsPciModules.cards_used[ jj ].instance == dac20Cnt ) )
                {
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
    // If no ADC cards were found, then SLAVE cannot run
    if ( !cdsPciModules.adcCount )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: No ADC cards found - exiting\n" );
        return -5;
    }
    // This did not quite work for some reason
    // Need to find a way to handle skipped DAC cards in slaves
    // cdsPciModules.dacCount = ioMemData->dacCount;
    if ( status < cards )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: Did not find correct number of cards! Expected %d "
                "and Found %d\n",
                cards,
                status );
        cardCountErr = 1;
    }

    // Print out all the I/O information
    // Following routine is in moduleLoadCommon.c
    print_io_info( &cdsPciModules );

    // Following section maps Reflected Memory, both VMIC hardware style and
    // Dolphin PCIe network style. Slave units will perform I/O transactions
    // with RFM directly ie MASTER does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to SLAVES.

    // Slave gets RFM module count from MASTER.
    cdsPciModules.rfmCount = ioMemData->rfmCount;
    // dolphinCount is number of segments
    cdsPciModules.dolphinCount = ioMemData->dolphinCount;
    // dolphin read/write 0 is for local PCIe network traffic
    cdsPciModules.dolphinRead[ 0 ] = ioMemData->dolphinRead[ 0 ];
    cdsPciModules.dolphinWrite[ 0 ] = ioMemData->dolphinWrite[ 0 ];
    // dolphin read/write 1 is for long range PCIe (RFM) traffic
    cdsPciModules.dolphinRead[ 1 ] = ioMemData->dolphinRead[ 1 ];
    cdsPciModules.dolphinWrite[ 1 ] = ioMemData->dolphinWrite[ 1 ];
    for ( ii = 0; ii < cdsPciModules.rfmCount; ii++ )
    {
        cdsPciModules.pci_rfm[ ii ] = ioMemData->pci_rfm[ ii ];
        cdsPciModules.pci_rfm_dma[ ii ] = ioMemData->pci_rfm_dma[ ii ];
    }
    // User APP does not access IRIG-B cards
    cdsPciModules.gps = 0;
    cdsPciModules.gpsType = 0;

    // Initialize buffer for daqLib.c code
    daqBuffer = (long)&daqArea[ 0 ];

    // Set Pointer to EPICS data
    pLocalEpics = (CDS_EPICS*)&( (RFM_FE_COMMS*)_epics_shm )->epicsSpace;
    pLocalEpics->epicsOutput.fe_status = WAIT_BURT;
    // Ensure EPICS is running
    for ( cnt = 0; cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++ )
    {
        msleep( 1000 );
    }
    // If EPICS not running, EXIT
    if ( cnt == 10 )
    {
	printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: EPICS restore not set - exiting\n");
        pLocalEpics->epicsOutput.fe_status = BURT_RESTORE_ERROR;
        // Cleanup
        return -6;
    }

    pLocalEpics->epicsInput.vmeReset = 0;
    udelay( 2000 );

#ifdef NO_CPU_SHUTDOWN
    sthread = kthread_create( fe_start_app, 0, "fe_start_app/%d", CPUID );
    if ( IS_ERR( sthread ) )
    {
        printk( "Failed to kthread_create()\n" );
        return -1;
    }
    kthread_bind( sthread, CPUID );
    wake_up_process( sthread );
#endif

    pLocalEpics->epicsOutput.fe_status = LOCKING_CORE;

#ifndef NO_CPU_SHUTDOWN
    set_fe_code_idle( fe_start_app, CPUID );
    msleep( 100 );

    cpu_down( CPUID );

    // The code runs on the disabled CPU
#endif
    return 0;
}

void
rt_fe_cleanup( void )
{
    int        i;
    int        ret;
    extern int cpu_up( unsigned int cpu );

#ifndef NO_CPU_SHUTDOWN
    // Unset the code callback
    set_fe_code_idle( 0, CPUID );
#endif

    printk( "Setting stop_working_threads to 1\n" );
    // Stop the code and wait
#ifdef NO_CPU_SHUTDOWN
    ret = kthread_stop( sthread );
#endif
    stop_working_threads = 1;
    msleep( 1000 );

#ifndef NO_CPU_SHUTDOWN

    // Unset the code callback
    set_fe_code_idle( 0, CPUID );
    // printk("Will bring back CPU %d\n", CPUID);
    msleep( 1000 );
    // Bring the CPU back up
    cpu_up( CPUID );
    // msleep(1000);
#endif

    // Print out any error messages from FE code on exit
    print_exit_messages(fe_status_return, fe_status_return_subcode);
}

module_init( rt_fe_init );
module_exit( rt_fe_cleanup );
MODULE_DESCRIPTION( "Control system" );
MODULE_AUTHOR( "LIGO" );
MODULE_LICENSE( "Dual BSD/GPL" );
