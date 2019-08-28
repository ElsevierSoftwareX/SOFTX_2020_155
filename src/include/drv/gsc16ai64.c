///	\file gsc16ai64.c
///	\brief File contains the initialization routine and various register
///read/write
///<		operations for the General Standards 16bit, 32 channel ADC modules.
///<\n
///< For board info, see
///<    <a
///<    href="http://www.generalstandards.com/view-products2.php?BD_family=16ai64ssc">GSC
///<    16AI64SSC Manual</a>

#include "gsc16ai64.h"

volatile PLX_9056_DMA *adcDma[MAX_ADC_MODULES]; ///< DMA struct for GSA ADC
dma_addr_t adc_dma_handle[MAX_ADC_MODULES];     ///< PCI add of ADC DMA memory
volatile GSA_ADC_REG *adcPtr[MAX_ADC_MODULES];  ///< Ptr to ADC registers */

// *****************************************************************************
/// \brief Routine to initialize GSC 16bit, 32 channel ADC modules
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///     @param[in] *adcdev PCI address information passed by the mapping code in
///     map.c
///	@return Status from board enable command.
// *****************************************************************************
int gsc16ai64Init(CDS_HARDWARE *pHardware, struct pci_dev *adcdev) {
  static unsigned int
      pci_io_addr; /// @param pci_io_addr Bus address of PCI card I/O register.
  int devNum; /// @param devNum Index into CDS_HARDWARE struct for adding board
              /// info.
  char *_adc_add; /// @param *_adc_add ADC register address space
  int pedStatus;  /// @param pedStatus Status return from call to enable device.

  /// Get index into CDS_HARDWARE struct based on total number of ADC cards
  /// found by mapping routine in map.c
  devNum = pHardware->adcCount;
  /// Enable the module.
  pedStatus = pci_enable_device(adcdev);
  /// Enable device to be DMA master.
  pci_set_master(adcdev);
  /// Get the PLX chip address
  pci_read_config_dword(adcdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
  printk("pci0 = 0x%x\n", pci_io_addr);
  /// Map module DMA space directly to computer memory space.
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  /// Map the module DMA control registers via PLX chip registers
  adcDma[devNum] = (PLX_9056_DMA *)_adc_add;
  if (devNum == 0)
    plxIcr = (PLX_9056_INTCTRL *)_adc_add;

  /// Get the ADC register address
  pci_read_config_dword(adcdev, PCI_BASE_ADDRESS_2, &pci_io_addr);
  printk("pci2 = 0x%x\n", pci_io_addr);
  /// Map the module control register so local memory space.
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("ADC I/O address=0x%x  0x%lx\n", pci_io_addr, (long)_adc_add);
  /// Set global ptr to control register memory space.
  adcPtr[devNum] = (GSA_ADC_REG *)_adc_add;

  printk("BCR = 0x%x\n", adcPtr[devNum]->BCR);
  /// Reset the ADC board
  adcPtr[devNum]->BCR |= GSAI_RESET;
  do {
  } while ((adcPtr[devNum]->BCR & GSAI_RESET) != 0);

  /// Write in a sync word
  adcPtr[devNum]->SMUW = 0x0000;
  adcPtr[devNum]->SMLW = 0x0000;

  /// Set ADC to 64 channel = 32 differential channels
  adcPtr[devNum]->BCR |= (GSAI_FULL_DIFFERENTIAL);

#ifdef ADC_EXTERNAL_SYNC
  adcPtr[devNum]->BCR |= (GSAI_ENABLE_X_SYNC);
  adcPtr[devNum]->AUX_SIO |= 0x80;
#endif

  /// Set sample rate close to 16384Hz
  /// Unit runs with external clock, so this probably not necessary
  adcPtr[devNum]->RAG = 0x117D8;
  printk("RAG = 0x%x\n", adcPtr[devNum]->RAG);
  printk("BCR = 0x%x\n", adcPtr[devNum]->BCR);
  adcPtr[devNum]->RAG &= ~(GSAI_SAMPLE_START);
  /// Initiate board calibration
  adcPtr[devNum]->BCR |= GSAI_AUTO_CAL;
  /// Wait for internal calibration to complete.
  do {
  } while ((adcPtr[devNum]->BCR & GSAI_AUTO_CAL) != 0);
  adcPtr[devNum]->RAG |= GSAI_SAMPLE_START;
  adcPtr[devNum]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
  adcPtr[devNum]->SSC = (GSAI_64_CHANNEL | GSAI_EXTERNAL_SYNC);
  printk("SSC = 0x%x\n", adcPtr[devNum]->SSC);
  printk("IDBC = 0x%x\n", adcPtr[devNum]->IDBC);
  /// Fill in CDS_HARDWARE structure with ADC information.
  pHardware->pci_adc[devNum] =
      (long)pci_alloc_consistent(adcdev, 0x2000, &adc_dma_handle[devNum]);
  pHardware->adcType[devNum] = GSC_16AI64SSA;
  pHardware->adcConfig[devNum] = adcPtr[devNum]->ASSC;
  pHardware->adcCount++;
  /// Return board enable status.
  return (pedStatus);
}

// *****************************************************************************
/// \brief Function checks status of DMA DONE bit for an ADC module.
/// 	@param[in] module ID of ADC board to read.
///	@return Status of ADC module DMA DONE bit (0=not complete, 1= complete)
// *****************************************************************************
int gsc16ai64CheckDmaDone(int module) {
  // Return 0 if DMA not complete
  if ((adcDma[module]->DMA_CSR & GSAI_DMA_DONE) == 0)
    return (0);
  // Return 1 if DMA is complete
  else
    return (1);
}
// *****************************************************************************
/// \brief Function if DMA from ADC module is complete.
///< Code will remain in loop until DMA is complete.
///     @param[in] module ID of ADC board to read.
///	@param[out] data Status of DMA DONE bit.
///	@return ADC DMA Status (0=not complete, 16=complete
/// Note: This function not presently used.
// *****************************************************************************
int gsc16ai64WaitDmaDone(int module, int *data) {
  do {
  } while ((adcDma[module]->DMA_CSR & GSAI_DMA_DONE) == 0);
  // First channel should be marked with an upper bit set
  if (*data == 0)
    return 0;
  else
    return 16;
}

// *****************************************************************************
/// \brief Function clears ADC buffer and starts acquisition via external clock.
///< Also sets up ADC for Demand DMA mode and set GO bit in DMA Mode Register.
///< NOTE: In normal operation, this code should only be called while the clocks
///< from the timing slave are turned OFF ie during initialization process.
///	@param[in] adcCount Total number of ADC modules to start DMA.
// *****************************************************************************
int gsc16ai64Enable(int adcCount) {
  int ii;
  for (ii = 0; ii < adcCount; ii++) {
    /// Enable demand DMA mode ie auto DMA data to computer memory when
    ///< GSAI_THRESHOLD data points in ADC FIFO.
    adcPtr[ii]->BCR &= ~(GSAI_DMA_DEMAND_MODE);
    /// Set DMA mode and direction in PLX controller chip on module.
    adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR | 0x1000;
    /// Enable DMA
    adcDma[ii]->DMA_CSR = GSAI_DMA_START;
    /// Clear the FIFO and set demand DMA to start after all 32 channels have 1
    /// sample
    adcPtr[ii]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
    /// Enable sync via external clock input.
    adcPtr[ii]->BCR |= GSAI_ENABLE_X_SYNC;
  }
  return (0);
}

// *****************************************************************************
int gsc16ai64Enable1PPS(int ii) {
  /// Enable demand DMA mode ie auto DMA data to computer memory when
  ///< GSAI_THRESHOLD data points in ADC FIFO.
  adcPtr[ii]->BCR &= ~(GSAI_DMA_DEMAND_MODE);
  /// Set DMA mode and direction in PLX controller chip on module.
  adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR | 0x1000;
  /// Enable DMA
  adcDma[ii]->DMA_CSR = GSAI_DMA_START;
  /// Clear the FIFO and set demand DMA to start after all 32 channels have 1
  /// sample
  adcPtr[ii]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
  /// Enable sync via external clock input.
  adcPtr[ii]->BCR |= GSAI_ENABLE_X_SYNC;
  return (0);
}

// *****************************************************************************
/// \brief Function stops ADC acquisition by removing the clocking signal.
// *****************************************************************************
int gsc16ai64AdcStop() {
  adcPtr[0]->BCR &= ~(GSAI_ENABLE_X_SYNC);
  return (0);
}

// *****************************************************************************
/// \brief Routine reads number of samples in ADC FIFO.
///	@param[in] numAdc The ID number of the ADC module to read.
///	@return The number of samples presently in the ADC FIFO.
// *****************************************************************************
int gsc16ai64CheckAdcBuffer(int numAdc) {
  int dataCount;

  dataCount = adcPtr[numAdc]->BUF_SIZE;
  return (dataCount);
}

// *****************************************************************************
/// \brief This routine sets up the ADC DMA registers once on code
/// initialization.
///	@param[in] modNum The ID number of the ADC module to read.
// *****************************************************************************
int gsc16ai64DmaSetup(int modNum) {
  /// Set DMA mode such that completion does not cause interrupt on bus.
  adcDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  /// Load PCI address (remapped local memory) to which data is to be delivered.
  adcDma[modNum]->DMA0_PCI_ADD = (int)adc_dma_handle[modNum];
  /// Set the PCI address of board where data will be transferred from.
  adcDma[modNum]->DMA0_LOC_ADD = GSAI_DMA_LOCAL_ADDR;
  /// Set the number of bytes to be transferred.
  adcDma[modNum]->DMA0_BTC = GSAI_DMA_BYTE_COUNT;
  /// Set the DMA direction ie ADC to computer memory.
  adcDma[modNum]->DMA0_DESC = GSAI_DMA_TO_PCI;
  return (1);
}

// *****************************************************************************
/// \brief This routine sets up the ADC DMA registers once on code
/// initialization.
///	@param[in] modNum The ID number of the ADC module to read.
// *****************************************************************************
int gsc16ai64DmaSetup32(int modNum) {
  /// Set DMA mode such that completion does not cause interrupt on bus.
  adcDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  /// Load PCI address (remapped local memory) to which data is to be delivered.
  adcDma[modNum]->DMA0_PCI_ADD = (int)adc_dma_handle[modNum];
  /// Set the PCI address of board where data will be transferred from.
  adcDma[modNum]->DMA0_LOC_ADD = GSAI_DMA_LOCAL_ADDR;
  /// Set the number of bytes to be transferred.
  adcDma[modNum]->DMA0_BTC = 0x100;
  /// Set the DMA direction ie ADC to computer memory.
  adcDma[modNum]->DMA0_DESC = GSAI_DMA_TO_PCI;
  return (1);
}

// *****************************************************************************
/// \brief This routine starts an ADC DMA operation. It must first be setup by
///< the gsc16ai64DmaSetup routine.
///	@param[in] modNum The ID number of the ADC module to read.
// *****************************************************************************
void gsc16ai64DmaEnable(int modNum) {
  adcDma[modNum]->DMA_CSR = GSAI_DMA_START;
}

#ifdef DIAG_TEST
// *****************************************************************************
/// \brief Test Code
///< For DIAGS ONLY !!!!!!!!
///< This will change ADC DMA BYTE count

///< -- Greater than normal will result in channel hopping.

///< -- Less than normal will result in ADC timeout.

///< In both cases, real-time kernel code should exit with errors to dmesg
// *****************************************************************************
void gsc16ai64DmaBump(int modNum, int bump) {
  adcDma[modNum]->DMA0_BTC = GSAI_DMA_BYTE_COUNT + bump;
}
#endif
