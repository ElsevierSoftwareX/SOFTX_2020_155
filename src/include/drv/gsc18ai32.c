///	\file gsc16ai64.c
///	\brief File contains the initialization routine and various register read/write
///<		operations for the General Standards 16bit, 32 channel ADC modules. \n
///< For board info, see 
///<    <a href="http://www.generalstandards.com/view-products2.php?BD_family=16ai64ssc">GSC 16AI64SSC Manual</a>

#include "gsc18ai32.h"


// *****************************************************************************
/// \brief Routine to initialize GSC 16bit, 32 channel ADC modules
///     @param[in,out] *pHardware Pointer to global data structure for storing I/O 
///<            register mapping information.
///     @param[in] *adcdev PCI address information passed by the mapping code in map.c
///	@return Status from board enable command.
// *****************************************************************************
int gsc18ai32Init(CDS_HARDWARE *pHardware, struct pci_dev *adcdev)
{
  static unsigned int pci_io_addr;	/// @param pci_io_addr Bus address of PCI card I/O register.
  int devNum;				/// @param devNum Index into CDS_HARDWARE struct for adding board info.
  char *_adc_add;       		/// @param *_adc_add ADC register address space
  int pedStatus;			/// @param pedStatus Status return from call to enable device.
  volatile GSA_ADC_18BIT_REG *adc18Ptr;


  	/// Get index into CDS_HARDWARE struct based on total number of ADC cards found by mapping routine
        /// in map.c
	devNum = pHardware->adcCount;
	/// Enable the module.
	pedStatus = pci_enable_device(adcdev);
	/// Enable device to be DMA master.
	pci_set_master(adcdev);
	/// Get the PLX chip address
	pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
	printk("pci0 = 0x%x\n",pci_io_addr);
	/// Map module DMA space directly to computer memory space.
	_adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	/// Map the module DMA control registers via PLX chip registers
	adcDma[devNum] = (PLX_9056_DMA *)_adc_add;
	if(devNum == 0) plxIcr = (PLX_9056_INTCTRL *)_adc_add;

	/// Get the ADC register address
	pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	printk("pci2 = 0x%x\n",pci_io_addr);
	/// Map the module control register so local memory space.
	_adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	printk("ADC 18 I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_adc_add);
	/// Set global ptr to control register memory space.
	adc18Ptr = (GSA_ADC_18BIT_REG *)_adc_add;
    adcPtr[devNum] = (GSA_ADC_REG *)_adc_add;
        printk("ADC  pointer init  at 0x%lx\n",(long)adcPtr[devNum]);

	printk("BCR = 0x%x\n",adc18Ptr->BCR);
	/// Reset the ADC board
	adc18Ptr->BCR |= GSAF_RESET;
	do{
	  }while((adc18Ptr->BCR & GSAF_RESET) != 0);

	/// Write in a sync word
	adc18Ptr->SMUW = 0x0000;
	adc18Ptr->SMLW = 0x0000;

	/// Set ADC to 64 channel = 32 differential channels
    // Don't need this for this board

	#ifdef ADC_EXTERNAL_SYNC
      // This is for the external connector use
	  adc18Ptr->BCR |= (GSAF_ENABLE_X_SYNC); // 0x80
	  adc18Ptr->AUX_SIO |= 0x80;
	#endif


	/// Set sample rate close to 16384Hz
	/// Unit runs with external clock, so this probably not necessary
	adc18Ptr->RAG = 0x117D8;
	printk("RAG = 0x%x\n",adc18Ptr->RAG);
	printk("BCR = 0x%x\n",adc18Ptr->BCR);
	adc18Ptr->RAG &= ~(GSAF_SAMPLE_START);  //  0x10000
	/// Initiate board calibration
	adc18Ptr->BCR |= GSAF_AUTO_CAL;
	/// Wait for internal calibration to complete.
	do {
	  }while((adc18Ptr->BCR & GSAF_AUTO_CAL) != 0);  // 0x2000
	adc18Ptr->RAG |= GSAF_SAMPLE_START;  // 0x10000
	adc18Ptr->IDBC = (GSAF_CLEAR_BUFFER | GSAF_THRESHOLD);  // 0x40000 | 0x001f
	adc18Ptr->SSC = (GSAF_32_CHANNEL); // 0x5  Sets 32 channels and external clock
	printk("SSC = 0x%x\n",adc18Ptr->SSC);
	printk("IDBC = 0x%x\n",adc18Ptr->IDBC);
	/// Fill in CDS_HARDWARE structure with ADC information.
	pHardware->pci_adc[devNum] = (long) pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
	pHardware->adcType[devNum] = GSC_18AI32SSC1M;
	pHardware->adcConfig[devNum] = adc18Ptr->ASSC;
	pHardware->adcCount ++;
	/// Return board enable status.
	return(pedStatus);


/*
SSC Register 
    - Byte 0 = Active channels + lower sample source bit
        - 0x5
    - Byte 1 = upper sample source + enable clocking + enable clock divider + burst busy
        - 0x20
*/

}

// *****************************************************************************
/// \brief Function checks status of DMA DONE bit for an ADC module.
/// 	@param[in] module ID of ADC board to read.
///	@return Status of ADC module DMA DONE bit (0=not complete, 1= complete)
// *****************************************************************************
int gsc18ai32CheckDmaDone(int module)
{
	// Return 0 if DMA not complete
        if((adcDma[module]->DMA_CSR & GSAF_DMA_DONE) == 0) return(0);
	// Return 1 if DMA is complete
        else return(1);
}
// *****************************************************************************
/// \brief Function if DMA from ADC module is complete. 
///< Code will remain in loop until DMA is complete.
///     @param[in] module ID of ADC board to read.
///	@param[out] data Status of DMA DONE bit.
///	@return ADC DMA Status (0=not complete, 16=complete
/// Note: This function not presently used.
// *****************************************************************************
int gsc18ai32WaitDmaDone(int module, int *data)
{
        do{
        }while((adcDma[module]->DMA_CSR & GSAF_DMA_DONE) == 0);
        // First channel should be marked with an upper bit set
        if (*data == 0) return 0; else return 16;
}


// *****************************************************************************
/// \brief Function clears ADC buffer and starts acquisition via external clock.
///< Also sets up ADC for Demand DMA mode and set GO bit in DMA Mode Register.
///< NOTE: In normal operation, this code should only be called while the clocks from
///< the timing slave are turned OFF ie during initialization process.
///	@param[in] adcCount Total number of ADC modules to start DMA.
// *****************************************************************************
int gsc18ai32Enable(CDS_HARDWARE *pHardware)
{
  int ii;
  volatile GSA_ADC_18BIT_REG *adc18Ptr;
  for(ii=0;ii<pHardware->adcCount;ii++)
  {
    if(pHardware->adcType[ii] == GSC_18AI32SSC1M)
    {
        adc18Ptr = (volatile GSA_ADC_18BIT_REG *) adcPtr[ii];
	  /// Enable demand DMA mode ie auto DMA data to computer memory when 
	  ///< GSAI_THRESHOLD data points in ADC FIFO.
          adc18Ptr->BCR &= ~(GSAF_DMA_DEMAND_MODE);
	  /// Set DMA mode and direction in PLX controller chip on module.
          adcDma[ii]->DMA0_MODE = GSAF_DMA_MODE_NO_INTR | 0x1000;
	  /// Enable DMA
          adcDma[ii]->DMA_CSR = GSAF_DMA_START;
	  /// Clear the FIFO and set demand DMA to start after all 32 channels have 1 sample
          adc18Ptr->IDBC = (GSAF_CLEAR_BUFFER | GSAF_THRESHOLD);
	  /// Enable sync via external clock input.
          adc18Ptr->SSC |= GSAF_ENABLE_CLOCK;
    }
  }
  return(0);
}

#if 0
// *****************************************************************************
int gsc18ai32Enable1PPS(CDS_HARDWARE *pHardware, int ii)
{
/// Enable demand DMA mode ie auto DMA data to computer memory when 
///< GSAI_THRESHOLD data points in ADC FIFO.
  volatile GSA_ADC_18BIT_REG *adc18Ptr;
  if(pHardware->adcType[ii] == GSC_18AI32SSC1M)
  {
        adc18Ptr = (volatile GSA_ADC_18BIT_REG *) adcPtr[ii];
          adc18Ptr->BCR &= ~(GSAF_DMA_DEMAND_MODE);
          /// Set DMA mode and direction in PLX controller chip on module.
          adcDma->DMA0_MODE = GSAF_DMA_MODE_NO_INTR | 0x1000;
          /// Enable DMA
          adcDma->DMA_CSR = GSAF_DMA_START;
          /// Clear the FIFO and set demand DMA to start after all 32 channels have 1 sample
          adc18Ptr->IDBC = (GSAF_CLEAR_BUFFER | GSAF_THRESHOLD);
          /// Enable sync via external clock input.
          adc18Ptr->BCR |= GSAF_ENABLE_X_SYNC;
  }
         return(0);
}

// *****************************************************************************
/// \brief Function stops ADC acquisition by removing the clocking signal.
// *****************************************************************************
int gsc18ai32AdcStop()
{
  adc18Ptr[0]->BCR &= ~(GSAF_ENABLE_X_SYNC);
  return(0);
}
#endif

// *****************************************************************************
/// \brief Routine reads number of samples in ADC FIFO.
///	@param[in] numAdc The ID number of the ADC module to read.
///	@return The number of samples presently in the ADC FIFO.
// *****************************************************************************
int gsc18ai32CheckAdcBuffer(CDS_HARDWARE *pHardware, int numAdc)
{
  volatile GSA_ADC_18BIT_REG *adc18Ptr;
  int dataCount = 0;

  if(pHardware->adcType[numAdc] == GSC_18AI32SSC1M)
  {
        adc18Ptr = (volatile GSA_ADC_18BIT_REG *) adcPtr[numAdc];
        dataCount = adc18Ptr->BUF_SIZE;
  }
  return(dataCount);

}


// *****************************************************************************
/// \brief This routine sets up the ADC DMA registers once on code initialization.
///	@param[in] modNum The ID number of the ADC module to read.
// *****************************************************************************
int gsc18ai32DmaSetup(int modNum)
{
  /// Set DMA mode such that completion does not cause interrupt on bus.
  adcDma[modNum]->DMA0_MODE = GSAF_DMA_MODE_NO_INTR;
  /// Load PCI address (remapped local memory) to which data is to be delivered.
  adcDma[modNum]->DMA0_PCI_ADD = (int)adc_dma_handle[modNum];
  /// Set the PCI address of board where data will be transferred from.
  adcDma[modNum]->DMA0_LOC_ADD = GSAF_DMA_LOCAL_ADDR;
  /// Set the number of bytes to be transferred.
  adcDma[modNum]->DMA0_BTC = GSAF_DMA_BYTE_COUNT;
  /// Set the DMA direction ie ADC to computer memory.
  adcDma[modNum]->DMA0_DESC = GSAF_DMA_TO_PCI;
  return(1);
}

// *****************************************************************************
/// \brief This routine sets up the ADC DMA registers once on code initialization.
///	@param[in] modNum The ID number of the ADC module to read.
// *****************************************************************************
#if 0
int gsc18ai32DmaSetup32(int modNum)
{
  /// Set DMA mode such that completion does not cause interrupt on bus.
  adcDma[modNum]->DMA0_MODE = GSAF_DMA_MODE_NO_INTR;
  /// Load PCI address (remapped local memory) to which data is to be delivered.
  adcDma[modNum]->DMA0_PCI_ADD = (int)adc_dma_handle[modNum];
  /// Set the PCI address of board where data will be transferred from.
  adcDma[modNum]->DMA0_LOC_ADD = GSAF_DMA_LOCAL_ADDR;
  /// Set the number of bytes to be transferred.
  adcDma[modNum]->DMA0_BTC = 0x100;
  /// Set the DMA direction ie ADC to computer memory.
  adcDma[modNum]->DMA0_DESC = GSAF_DMA_TO_PCI;
  return(1);
}
#endif


// *****************************************************************************
/// \brief This routine starts an ADC DMA operation. It must first be setup by 
///< the gsc16ai64DmaSetup routine.
///	@param[in] modNum The ID number of the ADC module to read.
// *****************************************************************************
void gsc18ai32DmaEnable(int modNum)
{
  adcDma[modNum]->DMA_CSR = GSAF_DMA_START;
}

