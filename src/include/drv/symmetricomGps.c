#include "symmetricomGps.h"

// *****************************************************************************
/// Initialize Symmetricom GPS card (model BC635PCI-U)
// *****************************************************************************
int
symmetricomGpsInit( CDS_HARDWARE* pHardware, struct pci_dev* gpsdev )
{
    int                 i;
    static unsigned int pci_io_addr;
    int                 pedStatus;
    unsigned char*      addr1;
    unsigned char*      addr3;
    unsigned int*       cmd;
    unsigned int*       dramRead;
    unsigned int        time0;
    SYMCOM_REGISTER*    timeReg;

    pedStatus = pci_enable_device( gpsdev );
    pci_read_config_dword( gpsdev, PCI_BASE_ADDRESS_2, &pci_io_addr );
    pci_io_addr &= 0xfffffff0;
    printk( "PIC BASE 2 address = %x\n", pci_io_addr );

    addr1 = (unsigned char*)ioremap_nocache( (unsigned long)pci_io_addr, 0x40 );
    printk( "Remapped 0x%p\n", addr1 );
    pHardware->gps = (unsigned int*)addr1;
    pHardware->gpsType = SYMCOM_RCVR;
    timeReg = (SYMCOM_REGISTER*)addr1;
    ;

    pci_read_config_dword( gpsdev, PCI_BASE_ADDRESS_3, &pci_io_addr );
    pci_io_addr &= 0xfffffff0;
    addr3 =
        (unsigned char*)ioremap_nocache( (unsigned long)pci_io_addr, 0x200 );
    printk( "PIC BASE 3 address = 0x%x\n", pci_io_addr );
    printk( "PIC BASE 3 address = 0x%p\n", addr3 );
    dramRead = (unsigned int*)( addr3 + 0x82 );
    cmd = (unsigned int*)( addr3 + 0x102 );
    //
    // Set write and wait *****************************
    *cmd = 0xf6; // Request model ID
    i = 0;
    timeReg->ACK = 0x1; // Trigger module to capture time
    udelay( 1000 );
    timeReg->ACK = 0x80; // Trigger module to capture time
    do
    {
        udelay( 1000 );
        i++;
    } while ( ( timeReg->ACK == 0 ) && ( i < 20 ) );
    if ( timeReg->ACK )
        printk( "SysCom ack received ID %d !!! 0x%x\n", timeReg->ACK, i );
    printk( "Model = 0x%x\n", *dramRead );
    // End Wait ****************************************
    //
    // Set write and wait *****************************
    *cmd = 0x4915; // Request model ID
    i = 0;
    timeReg->ACK = 0x1; // Trigger module to capture time
    udelay( 1000 );
    timeReg->ACK = 0x80; // Trigger module to capture time
    do
    {
        udelay( 1000 );
        i++;
    } while ( ( timeReg->ACK == 0 ) && ( i < 20 ) );
    if ( timeReg->ACK )
        printk( "SysCom ack received ID %d !!! 0x%x\n", timeReg->ACK, i );
    printk( "Model = 0x%x\n", *dramRead );
    // End Wait ****************************************
    //
    // Set write and wait *****************************
    *cmd = 0x4416; // Request model ID
    i = 0;
    timeReg->ACK = 0x1; // Trigger module to capture time
    udelay( 1000 );
    timeReg->ACK = 0x80; // Trigger module to capture time
    do
    {
        udelay( 1000 );
        i++;
    } while ( ( timeReg->ACK == 0 ) && ( i < 20 ) );
    if ( timeReg->ACK )
        printk( "SysCom ack received ID %d !!! 0x%x\n", timeReg->ACK, i );
    printk( "Model = 0x%x\n", *dramRead );
    // End Wait ****************************************
    //
    // Set write and wait *****************************
    *cmd = 0x1519; // Request model ID
    i = 0;
    timeReg->ACK = 0x1; // Trigger module to capture time
    udelay( 1000 );
    timeReg->ACK = 0x80; // Trigger module to capture time
    do
    {
        udelay( 1000 );
        i++;
    } while ( ( timeReg->ACK == 0 ) && ( i < 20 ) );
    if ( timeReg->ACK )
        printk( "SysCom ack received ID %d !!! 0x%x\n", timeReg->ACK, i );
    printk( "New Time COde Format = 0x%x\n", *dramRead );
    // End Wait ****************************************
    //
    // Set write and wait *****************************
    *cmd = 0x1619; // Request model ID
    i = 0;
    timeReg->ACK = 0x1; // Trigger module to capture time
    udelay( 1000 );
    timeReg->ACK = 0x80; // Trigger module to capture time
    do
    {
        udelay( 1000 );
        i++;
    } while ( ( timeReg->ACK == 0 ) && ( i < 20 ) );
    if ( timeReg->ACK )
        printk( "SysCom ack received ID %d !!! 0x%x\n", timeReg->ACK, i );
    printk( "New TC Modulation = 0x%x\n", *dramRead );
    // End Wait ****************************************

    for ( i = 0; i < 10; i++ )
    {
        pHardware->gps[ 0 ] = 1;
        printk( "Current time %ds %dus %dns \n",
                ( pHardware->gps[ 0x34 / 4 ] - 252806386 ),
                0xfffff & pHardware->gps[ 0x30 / 4 ],
                100 * ( ( pHardware->gps[ 0x30 / 4 ] >> 20 ) & 0xf ) );
    }
    pHardware->gps[ 0 ] = 1;
    time0 = pHardware->gps[ 0x30 / 4 ];
    if ( time0 & ( 1 << 24 ) )
        printk( "Flywheeling, unlocked...\n" );
    else
        printk( "Locked!\n" );
    return ( 0 );
}

inline void
lockGpsTime( void )
{
    SYMCOM_REGISTER* timeRead;
    timeRead = (SYMCOM_REGISTER*)cdsPciModules.gps;
    timeRead->TIMEREQ = 1; // Trigger module to capture time
}

//***********************************************************************
// Function to read time from Symmetricom IRIG-B Module ***********************
//***********************************************************************
inline int
getGpsTime( unsigned int* tsyncSec, unsigned int* tsyncUsec )
{
    SYMCOM_REGISTER* timeRead;
    unsigned int     timeSec, timeNsec, sync;

    if ( cdsPciModules.gps )
    {
        timeRead = (SYMCOM_REGISTER*)cdsPciModules.gps;
        timeSec = timeRead->TIME1;
        timeNsec = timeRead->TIME0;
        *tsyncSec = timeSec - 315964800;
        *tsyncUsec = ( timeNsec & 0xfffff );
        // Read seconds, microseconds, nanoseconds
        sync = !( timeNsec & ( 1 << 24 ) );
        return sync;
    }
    return ( 0 );
}
