///     \file gsc16ao16.c
///     \brief File contains the initialization routine and various register
///     read/write
///<            operations for the General Standards 16bit, 16 channel DAC
///<            modules. \n
///< For board info, see
///<    <a
///<    href="http://www.generalstandards.com/view-products2.php?BD_family=16ao16">GSC
///<    16AO16 Manual</a>

#include "gsc16ao16.h"

// *****************************************************************************
/// \brief Routine to initialize GSC 16AO16 DAC modules.
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///     @param[in] *dacdev PCI address information passed by the mapping code in
///     map.c
///     @return Status from board enable command.
// *****************************************************************************
int gsc16ao16Init(CDS_HARDWARE *pHardware, struct pci_dev *dacdev) {
  int devNum; /// @param devNum Index into CDS_HARDWARE struct for adding board
              /// info.
  char *_dac_add; /// @param *_dac_add DAC register address space
  static unsigned int
      pci_io_addr; /// @param pci_io_addr Bus address of PCI card I/O register.
  int pedStatus; /// @param pedStatus Status return from call to enable device.

  /// Get index into CDS_HARDWARE struct based on total number of DAC cards
  /// found by mapping routine in map.c
  devNum = pHardware->dacCount;
  /// Enable the device, PCI required
  pedStatus = pci_enable_device(dacdev);
  /// Register module as Master capable, required for DMA
  pci_set_master(dacdev);
  /// Get the PLX chip PCI address, it is advertised at address 0
  pci_read_config_dword(dacdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
  printk("pci0 = 0x%x\n", pci_io_addr);
  _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  /// Set up a pointer to DMA registers on PLX chip
  dacDma[devNum] = (PLX_9056_DMA *)_dac_add;

  /// Get the DAC register address
  pci_read_config_dword(dacdev, PCI_BASE_ADDRESS_2, &pci_io_addr);
  printk("dac pci2 = 0x%x\n", pci_io_addr);
  _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("DAC I/O address=0x%x  0x%lx\n", pci_io_addr, (long)_dac_add);
  dacPtr[devNum] = (GSC_DAC_REG *)_dac_add;

  printk("DAC BCR = 0x%x\n", dacPtr[devNum]->BCR);
  /// Reset the DAC board and wait for it to finish (3msec)
  dacPtr[devNum]->BCR |= GSAO_RESET;

  do {
  } while ((dacPtr[devNum]->BCR & GSAO_RESET) != 0);

  /// Enable 2s complement by clearing offset binary bit
  dacPtr[devNum]->BCR = (GSAO_OUT_RANGE_10 | GSAO_SIMULT_OUT);
  printk("DAC BCR after init = 0x%x\n", dacPtr[devNum]->BCR);
  printk("DAC CSR = 0x%x\n", dacPtr[devNum]->CSR);

#ifdef OVERSAMPLE_DAC
  // Larger buffer required when in oversampling mode
#ifdef SERVO32K
  dacPtr[devNum]->BOR = GSAO_FIFO_64;
#endif
#ifdef SERVO16K
  dacPtr[devNum]->BOR = GSAO_FIFO_128;
#endif
#ifdef SERVO2K
  dacPtr[devNum]->BOR = GSAO_FIFO_1024;
#endif
#else
  /// Set DAC FIFO buffer size. Set to match DAC timing diagnostics.
  dacPtr[devNum]->BOR = GSAO_FIFO_256;
#endif
  dacPtr[devNum]->BOR |= GSAO_EXTERN_CLK;
  printk("DAC BOR = 0x%x\n", dacPtr[devNum]->BOR);
  pHardware->pci_dac[devNum] =
      (long)pci_alloc_consistent(dacdev, 0x200, &dac_dma_handle[devNum]);
  pHardware->dacConfig[devNum] = (int)(dacPtr[devNum]->ASSC);
  pHardware->dacType[devNum] = GSC_16AO16;
  pHardware->dacCount++;

  /// Call patch in map.c needed to properly write to native PCIe module version
  /// of 16AO16
  set_8111_prefetch(dacdev);
  return (pedStatus);
}

// *****************************************************************************
/// \brief Function checks if DMA from DAC module is complete. \n
///< Note: This function not presently used.
///     @param[in] module ID number of board to be accessed.
// *****************************************************************************
int gsc16ao16DacDmaDone(int module) {
  do {
  } while ((dacDma[module]->DMA_CSR & GSAI_DMA_DONE) == 0);
  return (1);
}

// *****************************************************************************
/// \brief Function enables all 16AO16 DAC modules to begin receiving external
/// clocking signals.
///     @param[in] *pHardware Pointer to global data structure for storing I/O
///<            register mapping information.
// *****************************************************************************
int gsc16ao16Enable(CDS_HARDWARE *pHardware) {
  int ii;

  for (ii = 0; ii < pHardware->dacCount; ii++) {
    if (pHardware->dacType[ii] == GSC_16AO16) {
      dacPtr[ii]->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
    }
  }

  return (0);
}

//
// *****************************************************************************
/// \brief Diagnostic to check number of samples in DAC FIFO.
///     @param[in] numDac ID number of board to be accessed.
///	@return DAC FIFO size information.
// *****************************************************************************
int gsc16ao16CheckDacBuffer(int numDac) {
  int dataCount;
  dataCount = (dacPtr[numDac]->BOR >> 12) & 0xf;
  return (dataCount);
}

// *****************************************************************************
/// \brief Clear DAC FIFO.
///     @param[in] numDac ID number of board to be accessed.
// *****************************************************************************
int gsc16ao16ClearDacBuffer(int numDac) {
  dacPtr[numDac]->BOR |= GSAO_CLR_BUFFER;
  return (0);
}

// *****************************************************************************
/// \brief This routine sets up the DAC DMA registers.
///< It is only called once to set up the number of bytes to be preloaded into
///< the DAC prior to fe code realtime loop.
///     @param[in] modNum ID number of board to be accessed.
///     @param[in] samples Number DAC values to be transferred on each DMA
///     cycle..
// *****************************************************************************
int gsc16ao16DmaPreload(int modNum, int samples) {
  dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
  dacDma[modNum]->DMA0_LOC_ADD = 0x18;
  dacDma[modNum]->DMA0_BTC = 0x20 * samples;
  dacDma[modNum]->DMA0_DESC = 0x0;
  return (1);
}

// *****************************************************************************
/// \brief This routine sets up the DAC DMA registers once on code
/// initialization.
///     @param[in] modNum ID number of board to be accessed.
// *****************************************************************************
int gsc16ao16DmaSetup(int modNum) {
  dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
  dacDma[modNum]->DMA0_LOC_ADD = 0x18;
#ifdef OVERSAMPLE_DAC
  dacDma[modNum]->DMA0_BTC = 0x40 * OVERSAMPLE_TIMES;
#else
  dacDma[modNum]->DMA0_BTC = 0x40;
#endif
  dacDma[modNum]->DMA0_DESC = 0x0;
  return (1);
}

// *****************************************************************************
/// \brief This routine starts a DMA operation to a DAC module.
///< It must first be setup by the gsc16ao16DmaSetup call.
///     @param[in] modNum ID number of board to be accessed.
// *****************************************************************************
void gsc16ao16DmaStart(int modNum) {
  // dacDma[modNum]->DMA0_PCI_ADD = ((int)dac_dma_handle[modNum]);
  dacDma[modNum]->DMA_CSR = GSAI_DMA_START;
}
