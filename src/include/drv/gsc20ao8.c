///     \file gsc20ao8.c
///     \brief File contains the initialization routine and various register read/write
///<            operations for the General Standards 20bit, 8 channel DAC modules. \n
///< For board info, see 
///<    <a href="http://www.generalstandards.com/view-products2.php?BD_family=18ao8">GSC 18AO8 Manual</a>

#include "gsc20ao8.h"

// *****************************************************************************
/// \brief Routine to initialize GSC 20AO8 DAC modules.
///     @param[in,out] *pHardware Pointer to global data structure for storing I/O 
///<            register mapping information.
///     @param[in] *dacdev PCI address information passed by the mapping code in map.c
///     @return Status from board enable command.
// *****************************************************************************
int gsc20ao8Init(CDS_HARDWARE *pHardware, struct pci_dev *dacdev)
{
  int devNum;					/// @param devNum Index into CDS_HARDWARE struct for adding board info.
  char *_dac_add;				/// @param *_dac_add DAC register address space
  static unsigned int pci_io_addr;		/// @param pci_io_addr Bus address of PCI card I/O register.
  int pedStatus;				/// @param pedStatus Status return from call to enable device.
  volatile GSA_20BIT_DAC_REG *dac20bitPtr;	/// @param *dac20bitPtr Pointer to DAC control registers.
  int timer = 0;

	  /// Get index into CDS_HARDWARE struct based on total number of DAC cards found by mapping routine
          /// in map.c
          devNum = pHardware->dacCount;
          /// Enable the device, PCI required
          pedStatus = pci_enable_device(dacdev);
          /// Register module as Master capable, required for DMA
          pci_set_master(dacdev);
          /// Get the PLX chip PCI address, it is advertised at address 0
          pci_read_config_dword(dacdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
          printk("pci0 = 0x%x\n",pci_io_addr);
          _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
          /// Set up a pointer to DMA registers on PLX chip
          dacDma[devNum] = (PLX_9056_DMA *)_dac_add;

          /// Get the DAC register address
          pci_read_config_dword(dacdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  // Send some info to dmesg
          printk("dac pci2 = 0x%x\n",pci_io_addr);
          _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	  // Send some info to dmesg
          printk("DAC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_dac_add);

          dac20bitPtr = (GSA_20BIT_DAC_REG *)_dac_add;
          dacPtr[devNum] = (GSC_DAC_REG *)_dac_add;

	  // Send some info to dmesg
          printk("DAC BCR = 0x%x\n",dac20bitPtr->BCR);
          /// Reset the DAC board and wait for it to finish (3msec)

          dac20bitPtr->BCR |= GSAO_20BIT_RESET;

	  timer = 6000;
          do{
		udelay(1000);
		timer -= 1;
          }while((dac20bitPtr->BCR & GSAO_20BIT_RESET) != 0 &&
	  	 	timer > 0 &&
		 	dac20bitPtr->PRIMARY_STATUS == 1);

	  // printk("DAC PSR after init = 0x%x and timer = %d\n",dac20bitPtr->PRIMARY_STATUS,timer);

          /// Enable 2s complement by clearing offset binary bit
          dac20bitPtr->BCR &= ~GSAO_20BIT_OFFSET_BINARY;
	  // Set simultaneous outputs
          dac20bitPtr->BCR |= GSAO_20BIT_SIMULT_OUT;
	  // Send some info to dmesg
          // printk("DAC BCR after init = 0x%x\n",dac20bitPtr->BCR);
          // printk("DAC OUTPUT CONFIG = 0x%x\n",dac20bitPtr->OUTPUT_CONFIG);

          /// Enable 10 volt output range
	  dac20bitPtr->OUTPUT_CONFIG |= GSAO_20BIT_10VOLT_RANGE;
          // Set differential outputs
          dac20bitPtr->OUTPUT_CONFIG |= GSAO_20BIT_DIFF_OUTS;
	  // Enable outputs.
          dac20bitPtr->BCR |= GSAO_20BIT_OUTPUT_ENABLE;
	  udelay(1000);
	  // Set primary status to detect autocal
	  printk("DAC PSR = 0x%x\n",dac20bitPtr->PRIMARY_STATUS);
	  dac20bitPtr->PRIMARY_STATUS = 2;
	   	udelay(1000);
	  printk("DAC PSR after reset = 0x%x\n",dac20bitPtr->PRIMARY_STATUS);

	  // Start Calibration
	  dac20bitPtr->BCR |= GSAO_20BIT_AUTOCAL_SET;
	  // Wait for autocal to complete
	  timer = 0;
          do{
	   	udelay(1000);
	  	// printk("DAC PSR in autocal = 0x%x\n",dac20bitPtr->PRIMARY_STATUS);
		timer += 1;
          }while((dac20bitPtr->BCR &  GSAO_20BIT_AUTOCAL_SET) != 0);

	  printk("DAC after autocal PSR = 0x%x\n",dac20bitPtr->PRIMARY_STATUS);
	  if(dac20bitPtr->BCR & GSAO_20BIT_AUTOCAL_PASS) 
		  printk("DAC AUTOCAL SUCCESS in %d milliseconds \n",timer);
	  else
		  printk("DAC AUTOCAL FAILED in %d milliseconds \n",timer);
	  printk("DAC PSR = 0x%x\n",dac20bitPtr->PRIMARY_STATUS);

	  // If 20bit DAC, need to enable outputs.
          dac20bitPtr->BCR |= GSAO_20BIT_OUTPUT_ENABLE;
          printk("DAC OUTPUT CONFIG after init = 0x%x with BCR = 0x%x\n",dac20bitPtr->OUTPUT_CONFIG, dac20bitPtr->BCR);

          pHardware->pci_dac[devNum] =
                (long) pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
          pHardware->dacConfig[devNum] = (int) (dac20bitPtr->ASY_CONFIG);

	  // Return the device type to main code.
          pHardware->dacType[devNum] = GSC_20AO8;
          pHardware->dacCount ++;

	  /// Call patch in map.c needed to properly write to native PCIe module
          set_8111_prefetch(dacdev);
          return(pedStatus);
}

// *****************************************************************************
/// \brief Function enables DAC modules to begin receiving external clocking signals.
///     @param[in] *pHardware Pointer to global data structure for storing I/O 
///<            register mapping information.
// *****************************************************************************
int gsc20ao8Enable(CDS_HARDWARE *pHardware)
{
   int ii;

   for (ii = 0; ii < pHardware -> dacCount; ii++)
   {
      if (pHardware->dacType[ii] == GSC_20AO8) {
        volatile GSA_20BIT_DAC_REG *dac20bitPtr = (volatile GSA_20BIT_DAC_REG *)(dacPtr[ii]);
        dac20bitPtr->OUTPUT_CONFIG |= GSAO_20BIT_EXT_CLOCK_SRC;
        dac20bitPtr->BUF_OUTPUT_OPS |= GSAO_20BIT_ENABLE_CLOCK;
        // printk("Triggered 20-bit DAC\n");
      }
   }

   return(0);
}


// *****************************************************************************
/// \brief This routine sets up the DAC DMA registers once on code initialization.
///     @param[in] modNum ID number of board to be accessed.
// *****************************************************************************
int gsc20ao8DmaSetup(int modNum)
{
  dacDma[modNum]->DMA1_MODE = GSAI_DMA_MODE_NO_INTR;
  dacDma[modNum]->DMA1_PCI_ADD = (int)dac_dma_handle[modNum];
  dacDma[modNum]->DMA1_LOC_ADD = GSAO_20BIT_DMA_LOCAL_ADDR;
#ifdef OVERSAMPLE_DAC
  dacDma[modNum]->DMA1_BTC = 0x20*OVERSAMPLE_TIMES;
#else
  dacDma[modNum]->DMA1_BTC = 0x20;
#endif
  dacDma[modNum]->DMA1_DESC = 0x0;
  return(1);
}


// *****************************************************************************
/// \brief This routine starts a DMA operation to a DAC module.
///< It must first be setup by the gsc20ao8DmaSetup call.
// *****************************************************************************
void gsc20ao8DmaStart(int modNum)
{
        // dacDma[modNum]->DMA1_PCI_ADD = ((int)dac_dma_handle[modNum]);
        dacDma[modNum]->DMA_CSR = GSAI_DMA1_START;
}


