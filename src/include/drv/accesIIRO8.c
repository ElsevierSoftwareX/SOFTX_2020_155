///     \file accesIIRO8.c
///     \brief File contain subroutines for initializing and read/write ops
///<            for Acces I/O 8 bit relay I/O modules. \n
///< For board info, see <a
///< href="http://accesio.com/go.cgi?p=../pcie/pcie-iiro-8.html">PCIe-IIRO-8
///< Manual</a>

#include "accesIIRO8.h"

// *****************************************************************************
/// \brief Routine to initialize ACCESS IIRO-8 Isolated DIO modules
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///     @parm[in] *diodev PCI address information passed by the mapping code in
///     map.c
// *****************************************************************************
int
accesIiro8Init( CDS_HARDWARE* pHardware, struct pci_dev* diodev )
{
    static unsigned int pci_io_addr; /// @param pci_io_addr Bus address of PCI
                                     /// card I/O register.
    int devNum; /// @param devNum Index into CDS_HARDWARE struct for adding
                /// board info.
    int pedStatus; /// @param pedStatus Status return from call to enable
                   /// device.

    /// Get index into CDS_HARDWARE struct based on total number of DIO cards
    /// found by mapping routine in map.c
    devNum = pHardware->doCount;
    /// Enable the module.
    pedStatus = pci_enable_device( diodev );
    /// Find the I/O address space for this module.
    pci_read_config_dword( diodev, PCI_BASE_ADDRESS_2, &pci_io_addr );
    printk( "iiro-8 dio pci2 = 0x%x\n", pci_io_addr );
    /// Write I/O address info into the CDS_HARDWARE structure.
    pHardware->pci_do[ devNum ] = pci_io_addr - 1;
    /// Fill in remaining info into CDS_HARDWARE structure.
    pHardware->doType[ devNum ] = ACS_8DIO;
    pHardware->doInstance[ devNum ] = pHardware->iiroDioCount;
    pHardware->iiroDioCount++;
    printk( "iiro-8 diospace = 0x%x\n", pHardware->pci_do[ devNum ] );
    pHardware->doCount++;
    /// Return device enable status.
    return ( pedStatus );
}

// *****************************************************************************
/// \brief Routine to read ACCESS IIRO-8 Isolated DIO modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
accesIiro8ReadInputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    unsigned int
        data; /// @param data Data read back from the module input register.
    data = inb( pHardware->pci_do[ modNum ] + IIRO_DIO8_INPUT );
    return ( data );
}

// *****************************************************************************
/// \brief Routine to read back output register of ACCESS IIRO-8 Isolated DIO
/// modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
accesIiro8ReadOutputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    unsigned int
        data; /// @param data Data read back from the module input register.
    data = inb( pHardware->pci_do[ modNum ] + IIRO_DIO8_OUTPUT );
    return ( data );
}

// *****************************************************************************
/// \brief Routine to write ACCESS IIRO-8 Isolated DIO modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
///     @param[in] data Data to be written to the module.
// *****************************************************************************
void
accesIiro8WriteOutputRegister( CDS_HARDWARE* pHardware, int modNum, int data )
{
    outb( data & 0xff, pHardware->pci_do[ modNum ] + IIRO_DIO8_OUTPUT );
}
