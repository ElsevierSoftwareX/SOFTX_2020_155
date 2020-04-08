///	@file moduleLoadIop.c
///	@brief File contains startup routines for real-time IOP code.

#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/spinlock_types.h>
#include <proc.h>

// These externs and "16" need to go to a header file (mbuf.h)
extern void* kmalloc_area[ 16 ];
extern int   mbuf_allocate_area( char* name, int size, struct file* file );
extern void* fe_start_iop( void* arg );
extern char  daqArea[ 2 * DAQ_DCU_SIZE ]; // Space allocation for daqLib buffers
struct task_struct* sthread;

// MAIN routine: Code starting point
// ****************************************************************
int need_to_load_IOP_first;
EXPORT_SYMBOL( need_to_load_IOP_first );

extern void set_fe_code_idle( void* ( *ptr )(void*), unsigned int cpu );
extern void msleep( unsigned int );

#include "moduleLoadCommon.c"

/// Startup function for initialization of kernel module.
int
rt_iop_init( void )
{
    int  status;
    int  ii, jj, kk; /// @param ii,jj,kk default loop counters
    int cards; /// @param cards Number of PCIe cards found on bus
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
    /// Verify requested core is free.
    if ( is_cpu_taken_by_rcg_model( CPUID ) )
    {
        printk( KERN_ALERT "Error: CPU %d already taken\n", CPUID );
        return -1;
    }
#endif

#ifdef DOLPHIN_TEST
    /// Initialize the Dolphin interface
    status = init_dolphin( 2 );
    if ( status != 0 )
    {
	    printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: Dolphin Network initialization failed; ret = %d\n", ret );
        return -6;
    }
#endif

    jj = 0;

    ret = attach_shared_memory();
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: mbuf_allocate_area failed; ret = %d\n", ret );
        return ret;
    }

    pLocalEpics->epicsOutput.fe_status = 1;
    /// Find and initialize all PCIe I/O modules
    // Following I/O card info is from feCode
    cards = sizeof( cards_used ) / sizeof( cards_used[ 0 ] );
    cdsPciModules.cards = cards;
    cdsPciModules.cards_used = cards_used;
    cdsPciModules.adcCount = 0;
    cdsPciModules.dacCount = 0;
    cdsPciModules.dioCount = 0;
    cdsPciModules.doCount = 0;

    /// Call PCI initialization routine in map.c file.
    status = mapPciModules( &cdsPciModules );
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

#ifdef REQUIRE_IO_CNT
    if(cardCountErr)
    {
        pLocalEpics->epicsOutput.fe_status = IO_CONFIG_ERROR;
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: Exit on incorrect card count \n");
        return -5;
    }
#endif

    /// Wirte PCIe card info to mbuf for use by userapp models
    // Clear out card model info in IO_MEM
    for ( ii = 0; ii < MAX_IO_MODULES; ii++ )
    {
        ioMemData->model[ ii ] = -1;
    }

    /// Master send module counts to SLAVE via ipc shm
    ioMemData->totalCards = status;
    ioMemData->adcCount = cdsPciModules.adcCount;
    ioMemData->dacCount = cdsPciModules.dacCount;
    ioMemData->bioCount = cdsPciModules.doCount;
    // kk will act as ioMem location counter for mapping modules
    kk = cdsPciModules.adcCount;
    for ( ii = 0; ii < cdsPciModules.adcCount; ii++ )
    {
        // MASTER maps ADC modules first in ipc shm for SLAVES
        ioMemData->model[ ii ] = cdsPciModules.adcType[ ii ];
        ioMemData->ipc[ ii ] =
            ii; // ioData memory buffer location for SLAVE to use
    }
    for ( ii = 0; ii < cdsPciModules.dacCount; ii++ )
    {
        // Pass DAC info to SLAVE processes
        ioMemData->model[ kk ] = cdsPciModules.dacType[ ii ];
        ioMemData->ipc[ kk ] = kk;
        // Following used by MASTER to point to ipc memory for inputting DAC
        // data from SLAVES
        cdsPciModules.dacConfig[ ii ] = kk;
        kk++;
    }
    // MASTER sends DIO module information to SLAVES
    // Note that for DIO, SLAVE modules will perform the I/O directly and
    // therefore need to know the PCIe address of these modules.
    ioMemData->bioCount = cdsPciModules.doCount;
    for ( ii = 0; ii < cdsPciModules.doCount; ii++ )
    {
        // MASTER needs to find Contec 1616 I/O card to control timing slave.
        if ( cdsPciModules.doType[ ii ] == CON_1616DIO )
        {
            tdsControl[ tdsCount ] = ii;
            tdsCount++;
        }
        ioMemData->model[ kk ] = cdsPciModules.doType[ ii ];
        // Unlike ADC and DAC, where a memory buffer number is passed, a PCIe
        // address is passed for DIO cards.
        ioMemData->ipc[ kk ] = cdsPciModules.pci_do[ ii ];
        kk++;
    }
    // Following section maps Reflected Memory, both VMIC hardware style and
    // Dolphin PCIe network style. Slave units will perform I/O transactions
    // with RFM directly ie MASTER does not do RFM I/O. Master unit only maps
    // the RFM I/O space and passes pointers to SLAVES.

    /// Map VMIC RFM cards, if any
    ioMemData->rfmCount = cdsPciModules.rfmCount;
    for ( ii = 0; ii < cdsPciModules.rfmCount; ii++ )
    {
        // Master sends RFM memory pointers to SLAVES
        ioMemData->pci_rfm[ ii ] = cdsPciModules.pci_rfm[ ii ];
        ioMemData->pci_rfm_dma[ ii ] = cdsPciModules.pci_rfm_dma[ ii ];
    }
#ifdef DOLPHIN_TEST
    /// Send Dolphin addresses to user app processes
    // dolphinCount is number of segments
    ioMemData->dolphinCount = cdsPciModules.dolphinCount;
    // dolphin read/write 0 is for local PCIe network traffic
    ioMemData->dolphinRead[ 0 ] = cdsPciModules.dolphinRead[ 0 ];
    ioMemData->dolphinWrite[ 0 ] = cdsPciModules.dolphinWrite[ 0 ];
    // dolphin read/write 1 is for long range PCIe (RFM) traffic
    ioMemData->dolphinRead[ 1 ] = cdsPciModules.dolphinRead[ 1 ];
    ioMemData->dolphinWrite[ 1 ] = cdsPciModules.dolphinWrite[ 1 ];

#else
    // Clear Dolphin pointers so the slave sees NULLs
    ioMemData->dolphinCount = 0;
    ioMemData->dolphinRead[ 0 ] = 0;
    ioMemData->dolphinWrite[ 0 ] = 0;
    ioMemData->dolphinRead[ 1 ] = 0;
    ioMemData->dolphinWrite[ 1 ] = 0;
#endif

    // Initialize buffer for daqLib.c code
    daqBuffer = (long)&daqArea[ 0 ];

    // wait to ensure EPICS is running before proceeding
    msleep(5000);
    pLocalEpics->epicsOutput.fe_status = 2;
    printk( "Waiting for EPICS BURT Restore = %d\n",
            pLocalEpics->epicsInput.burtRestore );
    /// Ensure EPICS running else exit
    for ( cnt = 0; cnt < 10 && pLocalEpics->epicsInput.burtRestore == 0; cnt++ )
    {
        msleep( 1000 );
    }
    if ( cnt == 10 || cdsPciModules.adcCount == 0 )
    {
	printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: EPICS restore not set - exiting\n");
	pLocalEpics->epicsOutput.fe_status = BURT_RESTORE_ERROR;
        // Cleanup
#ifdef DOLPHIN_TEST
        finish_dolphin( );
#endif
        return -6;
    }

    pLocalEpics->epicsInput.vmeReset = 0;

    udelay( 2000 );

    /// Start the controller thread
#ifdef NO_CPU_SHUTDOWN
    sthread = kthread_create( fe_start_iop, 0, "fe_start_iop/%d", CPUID );
    if ( IS_ERR( sthread ) )
    {
        printk( "Failed to kthread_create()\n" );
        return -1;
    }
    kthread_bind( sthread, CPUID );
    wake_up_process( sthread );
#endif

#ifndef NO_CPU_SHUTDOWN
    pLocalEpics->epicsOutput.fe_status = 3;
    printk( "" SYSTEM_NAME_STRING_LOWER ": Locking CPU core %d\n", CPUID );
    // The code runs on the disabled CPU
    set_fe_code_idle( fe_start_iop, CPUID );
    msleep( 100 );
    cpu_down( CPUID );

#endif
    return 0;
}

/// Kernel module cleanup function
void
rt_iop_cleanup( void )
{
#ifndef NO_CPU_SHUTDOWN
    extern int cpu_up( unsigned int cpu );

    /// Unset the code callback
    set_fe_code_idle( 0, CPUID );
#endif

    // printk("Setting stop_working_threads to 1\n");
    // Stop the code and wait
#ifdef NO_CPU_SHUTDOWN
    int ret;
    ret = kthread_stop( sthread );
#endif
    stop_working_threads = 1;
    msleep( 1000 );

#ifdef DOLPHIN_TEST
    /// Cleanup Dolphin card connections
    finish_dolphin( );
#endif

#ifndef NO_CPU_SHUTDOWN

    /// Bring the CPU core back on line
    // Unset the code callback
    set_fe_code_idle( 0, CPUID );
    // printkl("Will bring back CPU %d\n", CPUID);
    msleep( 1000 );
    // Bring the CPU back up
    cpu_up( CPUID );
    msleep( 1000 );
#endif

    // Print out any error messages from FE code on exit
    print_exit_messages(fe_status_return, fe_status_return_subcode);
}

module_init( rt_iop_init );
module_exit( rt_iop_cleanup );

MODULE_DESCRIPTION( "Control system" );
MODULE_AUTHOR( "LIGO" );
MODULE_LICENSE( "Dual BSD/GPL" );
