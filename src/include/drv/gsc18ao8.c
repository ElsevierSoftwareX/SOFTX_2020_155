///     \file gsc18ao8.c
///     \brief File contains the initialization routine and various register read/write
///<            operations for the General Standards 18bit, 8 channel DAC modules. \n
///< For board info, see 
///<    <a href="http://www.generalstandards.com/view-products2.php?BD_family=18ao8">GSC 18AO8 Manual</a>

#include "gsc18ao8.h"

// *****************************************************************************
/// \brief Routine to initialize GSC 18AO8 DAC modules.
///     @param[in,out] *pHardware Pointer to global data structure for storing I/O 
///<            register mapping information.
///     @param[in] *dacdev PCI address information passed by the mapping code in map.c
///     @return Status from board enable command.
// *****************************************************************************
int gsc18ao8Init(CDS_HARDWARE *pHardware, struct pci_dev *dacdev)
{
  int devNum;					/// @param devNum Index into CDS_HARDWARE struct for adding board info.
  char *_dac_add;				/// @param *_dac_add DAC register address space
  static unsigned int pci_io_addr;		/// @param pci_io_addr Bus address of PCI card I/O register.
  int pedStatus;				/// @param pedStatus Status return from call to enable device.
  volatile GSA_18BIT_DAC_REG *dac18bitPtr;	/// @param *dac18bitPtr Pointer to DAC control registers.
#ifdef DAC_AUTOCAL
  int timer = 0;
#endif

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
          printk("dac pci2 = 0x%x\n",pci_io_addr);
          _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
          printk("DAC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_dac_add);

          dac18bitPtr = (GSA_18BIT_DAC_REG *)_dac_add;
          dacPtr[devNum] = (GSC_DAC_REG *)_dac_add;

          printk("DAC BCR = 0x%x\n",dac18bitPtr->BCR);
          /// Reset the DAC board and wait for it to finish (3msec)

          dac18bitPtr->BCR |= GSAO_18BIT_RESET;

          do{
          }while((dac18bitPtr->BCR & GSAO_18BIT_RESET) != 0);

          /// Enable 2s complement by clearing offset binary bit
          dac18bitPtr->BCR &= ~GSAO_18BIT_OFFSET_BINARY;
          dac18bitPtr->BCR |= GSAO_18BIT_SIMULT_OUT;
          printk("DAC BCR after init = 0x%x\n",dac18bitPtr->BCR);
          printk("DAC OUTPUT CONFIG = 0x%x\n",dac18bitPtr->OUTPUT_CONFIG);

          /// Enable 10 volt output range
          dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_10VOLT_RANGE;
          // Various bits setup
          //dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_ENABLE_EXT_CLOCK;
          //dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_CLOCK_SRC;
          //dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_TRIG_SRC;
          dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_DIFF_OUTS;

#ifdef DAC_AUTOCAL
	  // Start Calibration
	  dac18bitPtr->BCR |= GSAO_18BIT_AUTOCAL_SET;
          do{
	   	udelay(1000);
		timer += 1;
          }while((dac18bitPtr->BCR &  GSAO_18BIT_AUTOCAL_SET) != 0);

	  if(dac18bitPtr->BCR & GSAO_18BIT_AUTOCAL_PASS) 
		  printk("DAC AUTOCAL SUCCESS in %d milliseconds \n",timer);
	  else
		  printk("DAC AUTOCAL FAILED in %d milliseconds \n",timer);
          printk("DAC OUTPUT CONFIG after init = 0x%x with BCR = 0x%x\n",dac18bitPtr->OUTPUT_CONFIG, dac18bitPtr->BCR);
#else
	printk("DAC OUTPUT CONFIG after init = 0x%x\n",dac18bitPtr->OUTPUT_CONFIG);
#endif

        // TODO: maybe dac_dma_handle should be a separate variable for 18-bit board?
          pHardware->pci_dac[devNum] =
                (long) pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
          pHardware->dacConfig[devNum] = (int) (dac18bitPtr->ASY_CONFIG);
          pHardware->dacType[devNum] = GSC_18AO8;
          pHardware->dacCount ++;

	  /// Call patch in map.c needed to properly write to native PCIe module version of 16AO16
          set_8111_prefetch(dacdev);
          return(pedStatus);
}

// *****************************************************************************
/// \brief Function enables DAC modules to begin receiving external clocking signals.
///     @param[in] *pHardware Pointer to global data structure for storing I/O 
///<            register mapping information.
// *****************************************************************************
int gsc18ao8Enable(CDS_HARDWARE *pHardware)
{
   int ii;

   for (ii = 0; ii < pHardware -> dacCount; ii++)
   {
      if (pHardware->dacType[ii] == GSC_18AO8) {
        volatile GSA_18BIT_DAC_REG *dac18bitPtr = (volatile GSA_18BIT_DAC_REG *)(dacPtr[ii]);
        dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_CLOCK_SRC;
        dac18bitPtr->BUF_OUTPUT_OPS |= GSAO_18BIT_ENABLE_CLOCK;
        printk("Triggered 18-bit DAC\n");
      }
   }

   return(0);
}


// *****************************************************************************
/// \brief This routine sets up the DAC DMA registers once on code initialization.
///     @param[in] modNum ID number of board to be accessed.
// *****************************************************************************
int gsc18ao8DmaSetup(int modNum)
{
  dacDma[modNum]->DMA1_MODE = GSAI_DMA_MODE_NO_INTR;
  dacDma[modNum]->DMA1_PCI_ADD = (int)dac_dma_handle[modNum];
  dacDma[modNum]->DMA1_LOC_ADD = GSAO_18BIT_DMA_LOCAL_ADDR;
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
///< It must first be setup by the gsc18ao8DmaSetup call.
// *****************************************************************************
void gsc18ao8DmaStart(int modNum)
{
        // dacDma[modNum]->DMA1_PCI_ADD = ((int)dac_dma_handle[modNum]);
        dacDma[modNum]->DMA_CSR = GSAI_DMA1_START;
}


