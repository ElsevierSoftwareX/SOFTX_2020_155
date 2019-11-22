///	\file accesDio24.c
///	\brief File contains subroutines for initializing and read/write ops
///<	for Acces I/O 24bit DIO modules.

// *****************************************************************************
/// \brief Routine to initialize ACCESS 24bit DIO modules
///	@param[in,out] *pHardware Pointer to global data structure for storing
///I/O
///<		register mapping information.
///	@parm[in] *diodev PCI address information passed by the mapping code in
///map.c
// *****************************************************************************
int
accesDio24Init( CDS_HARDWARE* pHardware, struct pci_dev* diodev )
{
    static unsigned int pci_io_addr;
    int devNum; /// @param devNum Index into CDS_HARDWARE struct for adding
                /// board info.
    int pedStatus;
    ; /// @param pedStatus Status return from call to enable device.

    /// Get index into CDS_HARDWARE struct based on total number of DIO cards
    /// found by mapping routine in map.c
    devNum = pHardware->dioCount;
    /// Enable the module.
    pedStatus = pci_enable_device( diodev );
    /// Find the I/O address space for this module.
    pci_read_config_dword( diodev, PCI_BASE_ADDRESS_2, &pci_io_addr );
    printk( "dio pci2 = 0x%x\n", pci_io_addr );
    /// Write I/O address info into the CDS_HARDWARE structure.
    pHardware->pci_do[ devNum ] = ( pci_io_addr - 1 );
    printk( "diospace = 0x%x\n", pHardware->pci_do[ devNum ] );
    outb_p( DIO_C_OUTPUT, pHardware->pci_do[ devNum ] + DIO_CTRL_REG );
    /// Clear the present output from the module.
    outb( 0x00, pHardware->pci_do[ devNum ] + DIO_C_REG );
    /// Fill in remaining info into CDS_HARDWARE structure.
    pHardware->doType[ devNum ] = ACS_24DIO;
    pHardware->doInstance[ devNum ] = pHardware->dioCount;
    pHardware->dioCount++;
    pHardware->doCount++;
    /// Return device enable status.
    return ( pedStatus );
}

// *****************************************************************************
/// \brief Routine to read ACCESS 24bit DIO modules
///	@param[in] *pHardware Pointer to global data structure for storing I/O
///<		register mapping information.
///	@param[in] modNum Which instance of the module is to be addressed.
// *****************************************************************************
unsigned int
accesDio24ReadInputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    unsigned int
        data; /// @param data Data read back from the module input register.
    /// Read module data via standard PCI I/O space.
    data = inb( pHardware->pci_do[ modNum ] );
    /// Return the status of the read operation.
    return ( data );
}

// *****************************************************************************
/// \brief Routine to write ACCESS 24bit DIO modules
///	@param[in] *pHardware Pointer to global data structure for storing I/O
///<		register mapping information.
///	@param[in] modNum Which instance of the module is to be addressed.
///	@param[in] data Data to be written to the module.
// *****************************************************************************
void
accesDio24WriteOutputRegister( CDS_HARDWARE* pHardware, int modNum, int data )
{
    /// Write module data via standard PCI I/O space.
    outb( data & 0xff, pHardware->pci_do[ modNum ] + DIO_C_REG );
}
