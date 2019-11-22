///     \file contec6464.c
///     \brief File contain subroutines for initializing and read/write ops
///<            for Contec 64input/64output digital I/O modules. \n
///< For board info, see <a
///< href="http://www.contec.com/product.php?id=1710">DIO-6464L-PE Manual</a>

#include "contec6464.h"

// *****************************************************************************
/// \brief Routine to initialize CONTEC PCIe 6464 DIO module
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///     @parm[in] *diodev PCI address information passed by the mapping code in
///     map.c
// *****************************************************************************
int
contec6464Init( CDS_HARDWARE* pHardware, struct pci_dev* diodev )
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
    /// Enable the module
    pedStatus = pci_enable_device( diodev );
    /// Find the I/O address space for this module.
    pci_read_config_dword( diodev, PCI_BASE_ADDRESS_0, &pci_io_addr );
    printk( "contec 6464 dio pci2 = 0x%x\n", pci_io_addr );
    pHardware->pci_do[ devNum ] = pci_io_addr - 1;
    printk( "contec32L diospace = 0x%x\n", pHardware->pci_do[ devNum ] );
    /// Read board number switch setting on module
    pci_read_config_dword( diodev, PCI_REVISION_ID, &id );
    printk( "contec dio pci2 card number= 0x%x\n", ( id & 0xf ) );
    /// Break the module into two, 32bit devices ie 64bits will not carry thru
    /// read/write and
    ///< maintain all of the bit information.
    /// Fill in CDS_HARDWARE information for lower 32 bits.
    pHardware->doType[ devNum ] = CON_6464DIO;
    pHardware->doCount++;
    pHardware->doInstance[ devNum ] = pHardware->cDio6464lCount;
    pHardware->cDio6464lCount++;
    /// Fill in CDS_HARDWARE information for upper 32 bits.
    devNum++;
    pHardware->pci_do[ devNum ] = ( pci_io_addr - 1 ) + 4;
    pHardware->doType[ devNum ] = CON_6464DIO;
    pHardware->doCount++;
    pHardware->doInstance[ devNum ] = pHardware->cDio6464lCount;
    pHardware->cDio6464lCount++;
    printk( "contec32H diospace = 0x%x\n", pHardware->pci_do[ devNum ] );
    /// Return board ID number.
    return ( id );
}
// *****************************************************************************
/// \brief Routine to write to CONTEC PCIe-64 DIO modules
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
///     @param[in] data Data to be written to the module.
// *****************************************************************************
unsigned int
contec6464WriteOutputRegister( CDS_HARDWARE* pHardware,
                               int           modNum,
                               unsigned int  data )
{

    outl( data, pHardware->pci_do[ modNum ] + 8 );
    return data;
}

// *****************************************************************************
/// \brief Routine to read back the output register of CONTEC PCIe-64 DIO
/// modules.
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
contec6464ReadOutputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    unsigned int out;
    //
    out = inl( pHardware->pci_do[ modNum ] + 8 );
    return out;
}

// *****************************************************************************
/// \brief Routine to read the input register of CONTEC PCIe-64 DIO modules.
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
///     @param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
contec6464ReadInputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    unsigned int out;
    out = inl( pHardware->pci_do[ modNum ] );
    return out;
}
