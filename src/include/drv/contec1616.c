///	\file contec1616.c
///     \brief File contain subroutines for initializing and read/write ops
///<            for Contec 16input/16output digital I/O modules. \n
///<		This module is supported solely for use in controlling timing
///<receivers 		modules in I/O chassis ie it is not intended for general use as a
///<		digital I/O board for users. \n
///< For board info, see <a
///< href="http://www.contec.com/product.php?id=1611">DIO-1616L-PE Manual</a>

#include "contec1616.h"

// *****************************************************************************
/// \brief Routine to initialize CONTEC PCIe 1616 DIO modules
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///     @parm[in] *diodev PCI address information passed by the mapping code in
///     map.c
// *****************************************************************************
int
contec1616Init( CDS_HARDWARE* pHardware, struct pci_dev* diodev )
{
    static unsigned int pci_io_addr; /// @param pci_io_addr Bus address of PCI
                                     /// card I/O register.
    int devNum; /// @param devNum Index into CDS_HARDWARE struct for adding
                /// board info.
    int pedStatus; /// @param pedStatus Status return from call to enable
                   /// device.
    int id; /// @param id Card ID number read from switch on Contec module.

    /// Get index into CDS_HARDWARE struct based on total number of DIO cards
    /// found by mapping routine in map.c
    devNum = pHardware->doCount;
    /// Enable the module.
    pedStatus = pci_enable_device( diodev );
    /// Find the I/O address space for this module.
    pci_read_config_dword( diodev, PCI_BASE_ADDRESS_0, &pci_io_addr );
    printk( "contec 1616 dio pci2 = 0x%x\n", pci_io_addr );
    /// Write I/O address info into the CDS_HARDWARE structure.
    pHardware->pci_do[ devNum ] = pci_io_addr - 1;
    printk( "contec 1616 diospace = 0x%x\n", pHardware->pci_do[ devNum ] );
    /// Read board number switch setting on module
    pci_read_config_dword( diodev, PCI_REVISION_ID, &id );
    printk( "contec dio pci2 card number= 0x%x\n", ( id & 0xf ) );
    /// Fill in remaining info into CDS_HARDWARE structure.
    pHardware->doType[ devNum ] = CON_1616DIO;
    pHardware->doCount++;
    pHardware->doInstance[ devNum ] = pHardware->cDio1616lCount;
    pHardware->cDio1616lCount++;
    /// Return board ID number.
    return ( id );
}

// *****************************************************************************
/// \brief Routine to write to CONTEC PCIe-16 DIO modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
///     @param[in] data Data to be written to the module.
// *****************************************************************************
unsigned int
contec1616WriteOutputRegister( CDS_HARDWARE* pHardware,
                               int           modNum,
                               unsigned int  data )
{
    outl( data, pHardware->pci_do[ modNum ] );
    return ( data );
}

// *****************************************************************************
/// \brief Routine to read back data from output register of CONTEC PCIe-16 DIO
/// modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
contec1616ReadOutputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    // The binary output state bits register is at +2
    return ( inl( pHardware->pci_do[ modNum ] + 2 ) );
}

// *****************************************************************************
/// \brief Routine to read data from input register of CONTEC PCIe-16 DIO
/// modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
contec1616ReadInputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    // Reading at +0 gives the input bits
    return ( inl( pHardware->pci_do[ modNum ] ) );
}
