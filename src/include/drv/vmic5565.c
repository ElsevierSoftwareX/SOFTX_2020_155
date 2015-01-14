///     \file vmic5565.c
///     \brief File contains the initialization routine and various register read/write
///<            operations for the Gefanuc PCIe-5565RC reflected memory  modules. \n
///< For board info, see 
///<    <a href="http://defense.ge-ip.com/products/pcie-5565rc/p2239">PCIe-5565RC Manual</a>

#include "vmic5565.h"

// *****************************************************************************
/// \brief Routine to initialize VMIC RFM modules. \n
///< Support provided is only for use of RFM with RCG IPC components.
// *****************************************************************************
int vmic5565Init(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev)
{
  static unsigned int pci_io_addr;
  int devNum;
  static char *csrAddr;
  static char *dmaAddr;
  static unsigned int csrAddress;
  static unsigned int dmaAddress;
  int pedStatus;

  devNum = pHardware->rfmCount;
  pedStatus = pci_enable_device(rfmdev);
  // Register module as Master capable, required for DMA
  pci_set_master(rfmdev);

    // Find the reflected memory base address
    pci_read_config_dword(rfmdev,
                          PCI_BASE_ADDRESS_3,
                          &pci_io_addr);
    // Map full 128 MByte new cards only
    pHardware->pci_rfm[devNum] = (long)ioremap_nocache((unsigned long)pci_io_addr, 128*1024*1024);
    // Allocate local memory for IPC DMA xfers from RFM module
    pHardware->pci_rfm_dma[devNum] = (long) pci_alloc_consistent(rfmdev,IPC_BUFFER_SIZE,&rfm_dma_handle[devNum]);
    printk("RFM address is 0x%ux\n",pci_io_addr);

    // Find the RFM control/status register
    pci_read_config_dword(rfmdev, PCI_BASE_ADDRESS_2, &csrAddress);
    printk("CSR address is 0x%x\n", csrAddress);
    csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

    p5565Csr[devNum] = (VMIC5565_CSR *)csrAddr;
    // pHardware->rfm_reg[devNum] = p5565Csr;
    p5565Csr[devNum]->LCSR1 &= ~TURN_OFF_5565_FAIL;
    p5565Csr[devNum]->LCSR1 &= !1; // Turn off own data light

    printk("Board id = 0x%x\n",p5565Csr[devNum]->BID);
    pHardware->rfmConfig[devNum] = p5565Csr[devNum]->NID;

    // Check switches and such
    if(p5565Csr[devNum]->LCSR1 & VMIC_5565_REDUNDANT_MODE) printk("VMIC5565 set to redundant transfers\n");
    else printk("VMIC5565 set to single transfers\n");
    if(p5565Csr[devNum]->LCSR1 & VMIC_5565_ROGUE_MASTER1) printk("VMIC5565 ROGUE MASTER 1 = ON\n");
    else printk("VMIC5565 ROGUE MASTER 1 = OFF\n");
    if(p5565Csr[devNum]->LCSR1 & VMIC_5565_ROGUE_MASTER0) printk("VMIC5565 ROGUE MASTER 0 = ON\n");
    else printk("VMIC5565 ROGUE MASTER 0 = OFF\n");
    if(p5565Csr[devNum]->LCSR1 & VMIC_5565_MEM_SIZE) printk("VMIC5565 Memory size = 128MBytes\n");
    else printk("VMIC5565 Memory size = 64MBytes\n");
    //
    // Find DMA Engine controls in RFM Local Configuration Table 
    pci_read_config_dword(rfmdev,
                 PCI_BASE_ADDRESS_0,
                 &dmaAddress);
    printk("DMA address is 0x%ux\n",dmaAddress);
    dmaAddr = ioremap_nocache(dmaAddress,0x200);
    p5565Dma[devNum] = (VMIC5565DMA *)dmaAddr;
    // pHardware->rfm_dma[devNum] = p5565Dma[devNum];
    printk("5565DMA at 0x%lx\n",(unsigned long int)p5565Dma[devNum]);
    printk("5565 INTCR = 0x%ux\n",p5565Dma[devNum]->INTCSR);
    p5565Dma[devNum]->INTCSR = 0;       // Disable interrupts from this card
    printk("5565 INTCR = 0x%ux\n",p5565Dma[devNum]->INTCSR);
    printk("5565 MODE = 0x%ux\n",p5565Dma[devNum]->DMA0_MODE);
    printk("5565 DESC = 0x%ux\n",p5565Dma[devNum]->DMA0_DESC);
    // Preload some DMA settings
    // Only important items here are BTC and DESC fields, which are used 
    // later by DMA routine.
    p5565Dma[devNum]->DMA0_PCI_ADD = pHardware->pci_rfm_dma[0];
    p5565Dma[devNum]->DMA0_LOC_ADD = IPC_BASE_OFFSET;   // Start at RFM + 0x100 offset
    p5565Dma[devNum]->DMA0_BTC = IPC_RFM_XFER_SIZE;     // Set byte xfer to 256 bytes
    p5565Dma[devNum]->DMA0_DESC = VMIC_DMA_READ;        // Set RFM to local memory


  pHardware->rfmType[devNum] = 0x5565;
  pHardware->rfmCount ++;
  return(0);
}
// *****************************************************************************
/// RFM DMA Read in support of commData
// *****************************************************************************
void vmic5565DMA(CDS_HARDWARE *pHardware, int card, int offset)
{
int myAddress;
int rfmAddress;

    rfmAddress = IPC_BASE_OFFSET + (offset * IPC_RFM_BLOCK_SIZE);
    myAddress = (pHardware->pci_rfm_dma[card] + (offset * IPC_RFM_BLOCK_SIZE));

    // p5565Dma[card]->DMA_CSR = VMIC_DMA_CLR;  // Clear DMA DONE
    p5565Dma[card]->DMA0_PCI_ADD = myAddress;   // Computer address space
    p5565Dma[card]->DMA0_LOC_ADD = rfmAddress;  // RFM card offset from base
    // These were set during initialization, so don't need them again (save time)
    // p5565Dma[card]->DMA0_BTC = 0x400;        // Set byte xfer to 1024 bytes
    // p5565Dma[card]->DMA0_DESC = 0x8; // Set RFM to local memory
    p5565Dma[card]->DMA_CSR = VMIC_DMA_START;   // Start DMA
}
// *****************************************************************************
/// Routine to clear RFM DMA done bit
// *****************************************************************************
void vmic5565DMAclr(CDS_HARDWARE *pHardware, int card)
{
    p5565Dma[card]->DMA_CSR = VMIC_DMA_CLR;     // Clear DMA DONE
}
// *****************************************************************************
/// Routine to check RFM done bit and clear for next DMA
// *****************************************************************************
int vmic5565DMAdone(int card)
{
int status;
int ii;
    ii = 0;
    do{
    status = p5565Dma[card]->DMA_CSR;   // Check DMA DONE bit (4)
    if((status & 0x10) == 0) udelay(1);
    ii ++;
    }while(((status & 0x10) == 0) && (ii < 10));
    p5565Dma[card]->DMA_CSR = 0x8;      // Clear DMA DONE
    return (ii);
}

// *****************************************************************************
/// \brief Routine to check RFM Own Data received bit.
/// @param[in] card ID of card to read
/// @return Status of Own Data Bit
// *****************************************************************************
int vmic5565CheckOwnDataRcv(int card)
{
	return (p5565Csr[card]->LCSR1 & 1);
}

// *****************************************************************************
/// \brief Routine to reset RFM Own Data received bit.
/// @param[in] card ID of card to read
// *****************************************************************************
void vmic5565ResetOwnDataLight(int card)
{
	p5565Csr[card]->LCSR1 &= ~1; // Turn off own data light

}


