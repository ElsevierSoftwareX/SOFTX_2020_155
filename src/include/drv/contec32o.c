///     \file contec32o.c
///     \brief File contain subroutines for initializing and read/write ops
///<            for Contec 32 output digital I/O modules. \n
///< For board info, see <a
///< href="http://www.contec.com/product.php?id=1723">DO-32L-PE Manual</a>
// *****************************************************************************
/// Routine to Initialize CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
int
contec32OutInit( CDS_HARDWARE* pHardware, struct pci_dev* diodev )
{
    static unsigned int pci_io_addr;
    int                 devNum;
    int                 id;
    int                 pedStatus;

    devNum = pHardware->doCount;
    pedStatus = pci_enable_device( diodev );
    pci_read_config_dword( diodev, PCI_BASE_ADDRESS_0, &pci_io_addr );
    printk( "contec dio pci2 = 0x%x\n", pci_io_addr );
    pHardware->pci_do[ devNum ] = pci_io_addr - 1;
    printk( "contec32L diospace = 0x%x\n", pHardware->pci_do[ devNum ] );
    pci_read_config_dword( diodev, PCI_REVISION_ID, &id );
    printk( "contec dio pci2 card number= 0x%x\n", ( id & 0xf ) );
    pHardware->doType[ devNum ] = CON_32DO;
    pHardware->doCount++;
    pHardware->doInstance[ devNum ] = pHardware->cDo32lCount;
    // printk("pHardware count is at %d\n",pHardware->doCount);
    // printk("pHardware cDo32lCount is at %d\n", pHardware->cDo32lCount);
    pHardware->cDo32lCount++;
    return ( 0 );
}

// *****************************************************************************
/// Routine to write CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
unsigned int
contec32WriteOutputRegister( CDS_HARDWARE* pHardware,
                             int           modNum,
                             unsigned int  data )
{
    // printk("writeCDO32l modNum = %d\n",modNum);
    // printk("writeCDO32l data = %d\n",data);
    outl( data, pHardware->pci_do[ modNum ] );
    return ( inl( pHardware->pci_do[ modNum ] ) );
}

// *****************************************************************************
/// Routine to read CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
unsigned int
contec32ReadOutputRegister( CDS_HARDWARE* pHardware, int modNum )
{
    return ( inl( pHardware->pci_do[ modNum ] ) );
}
