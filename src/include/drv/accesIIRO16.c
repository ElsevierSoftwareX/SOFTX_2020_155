///	\file accesIIRO16.c
///	\brief File contain subroutines for initializing and read/write ops
///<		for Acces I/O 16 bit relay I/O modules. \n
///< For board info, see <a
///< href="http://accesio.com/go.cgi?p=../pcie/pcie-iiro-16.html">PCIe-IIRO-16
///< Manual</a>

#include "accesIIRO16.h"

// *****************************************************************************
/// \brief Routine to initialize ACCESS IIRO-16 Isolated DIO modules
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///     @parm[in] *diodev PCI address information passed by the mapping code in
///     map.c
// *****************************************************************************
int
accesIiro16Init( CDS_HARDWARE* pHardware, struct pci_dev* diodev )
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
    printk( "iiro-16 dio pci2 = 0x%x\n", pci_io_addr );
    /// Fill module data into the CDS_HARDWARE structure.
    pHardware->pci_do[ devNum ] = pci_io_addr - 1;
    pHardware->doType[ devNum ] = ACS_16DIO;
    pHardware->doInstance[ devNum ] = pHardware->iiroDio1Count;
    pHardware->iiroDio1Count++;
    printk( "iiro-16 diospace = 0x%x\n", pHardware->pci_do[ devNum ] );

    pHardware->doCount++;
    /// Return device enable status.
    return ( pedStatus );
}

// *****************************************************************************
/// \brief Routine to read input register of ACCESS IIRO-16 Isolated DIO modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
accesIiro16ReadInputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    unsigned int v, v1; /// @param v,v1 data returned from read operation.
    /// Read lower byte via standard PCI I/O space.
    v = inb( pHardware->pci_do[ modNum ] + IIRO_DIO16_INPUT );
    /// Read upper byte via standard PCI I/O space.
    v1 = inb( pHardware->pci_do[ modNum ] + 4 + IIRO_DIO16_INPUT );
    /// Return data as 16bit word.
    return v | ( v1 << 8 );
    return ( v );
}

// *****************************************************************************
/// \brief Routine to write ACCESS IIRO-16 Isolated DIO modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
///     @param[in] data Data to be written to the module.
// *****************************************************************************
void
accesIiro16WriteOutputRegister( CDS_HARDWARE* pHardware, int modNum, int data )
{
    /// Write data lower byte to module via standard PCI I/O space.
    outb( data & 0xff, pHardware->pci_do[ modNum ] + IIRO_DIO16_OUTPUT );
    /// Write data upper byte to module via standard PCI I/O space.
    outb( ( data >> 8 ) & 0xff,
          pHardware->pci_do[ modNum ] + 4 + IIRO_DIO16_OUTPUT );
}
