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
struct task_struct* sthread;

extern void         set_fe_code_idle( void* ( *ptr )(void*), unsigned int cpu );
extern void         msleep( unsigned int );
extern int need_to_load_IOP_first;

#include "moduleLoadCommon.c"


// MAIN routine: Code starting point
// ****************************************************************
/// Startup function for initialization of kernel module.
int
rt_fe_init( void )
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
    // Verify requested CPU core is free
    if ( is_cpu_taken_by_rcg_model( CPUID ) )
    {
        printk( KERN_ALERT "Error: CPU %d already taken\n", CPUID );
        return -1;
    }
#endif

    need_to_load_IOP_first = 0;

    jj = 0;

    ret = attach_shared_memory();
    if ( ret < 0 )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: mbuf_allocate_area failed; ret = %d\n", ret );
        return ret;
    }

    // Find and initialize all PCI I/O modules
    // ******************************************************* Following I/O
    // card info is from feCode
    pLocalEpics->epicsOutput.fe_status = FIND_MODULES;
    cards = sizeof( cards_used ) / sizeof( cards_used[ 0 ] );
    cdsPciModules.cards = cards;
    cdsPciModules.cards_used = cards_used;
    cdsPciModules.adcCount = 0;
    cdsPciModules.dacCount = 0;
    cdsPciModules.dioCount = 0;
    cdsPciModules.doCount = 0;

     /// Call PCI initialization routine in mapApp.c file
    status = mapPciModules(&cdsPciModules);

    // If no ADC cards were found, then SLAVE cannot run
    if ( !cdsPciModules.adcCount )
    {
        printk( "" SYSTEM_NAME_STRING_LOWER ": ERROR: No ADC cards found - exiting\n" );
        return -5;
    }

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

    // Initialize buffer for daqLib.c code
    daqBuffer = (long)&daqArea[ 0 ];

    // wait to ensure EPICS is running before proceeding
    pLocalEpics->epicsOutput.fe_status = WAIT_BURT;
    msleep(5000);
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

    /// Start the controller thread
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
    pLocalEpics->epicsOutput.fe_status = LOCKING_CORE;
    printk( "" SYSTEM_NAME_STRING_LOWER ": Locking CPU core %d\n", CPUID );

    set_fe_code_idle( fe_start_app, CPUID );
    msleep( 100 );
    cpu_down( CPUID );

    // The code runs on the disabled CPU
#endif
    return 0;
}

/// Kernel module cleanup function
void
rt_fe_cleanup( void )
{
    int        i;

#ifndef NO_CPU_SHUTDOWN
    extern int cpu_up( unsigned int cpu );

    // Unset the code callback
    set_fe_code_idle( 0, CPUID );
#endif

    //printk( "Setting stop_working_threads to 1\n" );
    // Stop the code and wait
#ifdef NO_CPU_SHUTDOWN
    int        ret;
    ret = kthread_stop( sthread );
#endif
    stop_working_threads = 1;
    msleep( 1000 );

#ifndef NO_CPU_SHUTDOWN

    /// Bring the CPU core back on line
    // Unset the code callback
    set_fe_code_idle( 0, CPUID );
    // printk("Will bring back CPU %d\n", CPUID);
    msleep( 1000 );
    // Bring the CPU back up
    cpu_up( CPUID );
    msleep(1000);
#endif

    // Print out any error messages from FE code on exit
    print_exit_messages(fe_status_return, fe_status_return_subcode);
}

module_init( rt_fe_init );
module_exit( rt_fe_cleanup );
MODULE_DESCRIPTION( "Control system" );
MODULE_AUTHOR( "LIGO" );
MODULE_LICENSE( "Dual BSD/GPL" );
