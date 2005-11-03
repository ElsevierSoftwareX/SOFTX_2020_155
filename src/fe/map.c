#include <linux/types.h>
#include <linux/pci.h>
#include <drv/vmic5579.h>
#include <drv/vmic5565.h>
#include <drv/gsaadc.h>		/* GSA ADC module defs */
#include <drv/gsadac.h>		/* GSA DAC module defs */
#include <drv/plx9056.h>	/* PCI interface chip for GSA products */

#define RFM_WRITE	0x0
#define RFM_READ	0x8

volatile PLX_9056_DMA *adcDma;	/* DMA struct for GSA ADC */
volatile PLX_9056_DMA *dacDma;	/* DMA struct for GSA DAC */
volatile PLX_9056_INTCTRL *plxIcr;	/* Ptr to interrupt cntrl reg on PLX chip */
dma_addr_t adc_dma_handle;		/* PCI add of ADC DMA memory */
dma_addr_t dac_dma_handle;		/* PCI add of ADC DMA memory */
char *_adc_add;				/* ADC register address space */
char *_dac_add;				/* DAC register address space */
volatile GSA_ADC_REG *adcPtr;		/* Ptr to ADC registers */
volatile GSA_DAC_REG *dacPtr;		/* Ptr to DAC registers */


// *****************************************************************************
// Generic function to DMA data to/from VMIC5565 RFM Module
// *****************************************************************************
int rfm5565dma(int rfmMemLocator, int byteCount, int direction)
{
	p5565Dma->DMA0_MODE = 0x1C3;
	p5565Dma->DMA0_PCI_ADD = (int)((rfmDmaHandle & 0xffffffff));
	p5565Dma->DMA0_PCI_DUAL = (int)(((rfmDmaHandle >> 32) & 0xffffffff));
	p5565Dma->DMA0_LOC_ADD = rfmMemLocator;
	p5565Dma->DMA0_BTC = byteCount;
	p5565Dma->DMA0_DESC = direction;
	p5565Dma->DMA_CSR = 0x3;
}

// *****************************************************************************
// Generic function to check DMA complete to/from VMIC5565 RFM Module
// *****************************************************************************
int rfm5565DmaDone()
{
	if((p5565Dma->DMA_CSR & 0x11) == 0x11) return (1);
	else return(0);
}

// *****************************************************************************
// Function checks if DMA from ADC module is complete
// *****************************************************************************
int adcDmaDone()
{
	if(adcDma->DMA_CSR & GSAI_DMA_DONE) return (1);
	else return(0);
}

// *****************************************************************************
// Function clears ADC buffer and starts acquisition via external clock
// *****************************************************************************
int gsaAdcTrigger()
{
#if 0
  adcPtr->INTCR |= GSAI_ISR_ON_SAMPLE;
  plxIcr->INTCSR |= PLX_INT_ENABLE;
  adcPtr->RAG &= ~(GSAI_SAMPLE_START);
#endif
  adcPtr->IDBC = GSAI_CLEAR_BUFFER;
  adcPtr->BCR |= GSAI_ENABLE_X_SYNC;
}

// *****************************************************************************
// Function stops ADC acquisition
// *****************************************************************************
int gsaAdcStop()
{
#if 0
  plxIcr->INTCSR &= ~(PLX_INT_DISABLE);
  adcPtr->INTCR = 0;
  adcPtr->RAG |= GSAI_SAMPLE_START;
#endif
  adcPtr->BCR &= ~(GSAI_ENABLE_X_SYNC);
}


// *****************************************************************************
// Test if ADC has a sample ready
// *****************************************************************************
int checkAdcRdy(int count)
{
  int dataCount;

    do {
        dataCount = adcPtr->BUF_SIZE;
  }while(dataCount < count);
  return(1);

}

// *****************************************************************************
// DMA 32 samples from ADC module
// *****************************************************************************
int gsaAdcDma(int byteCount)
{
  adcDma->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  adcDma->DMA0_PCI_ADD = (int)adc_dma_handle;
  adcDma->DMA0_LOC_ADD = GSAI_DMA_LOCAL_ADDR;
  adcDma->DMA0_BTC = byteCount;
  adcDma->DMA0_DESC = GSAI_DMA_TO_PCI;
  adcDma->DMA_CSR = GSAI_DMA_START;
  return(1);
}

// *****************************************************************************
// DMA 16 samples to DAC module
// *****************************************************************************
int gsaDacDma()
{
  dacDma->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  dacDma->DMA0_PCI_ADD = (int)dac_dma_handle;
  dacDma->DMA0_LOC_ADD = 0x18;
  dacDma->DMA0_BTC = 0x40;
  dacDma->DMA0_DESC = 0x0;
  dacDma->DMA_CSR = GSAI_DMA_START;
  return(1);
}

// *****************************************************************************
// Load DAC one value at a time - not normally used
// *****************************************************************************
void trigDac(short dacData[])
{
int ii;

  for(ii=0;ii<16;ii++)
  {
	dacPtr->ODB[0] = dacData[ii];
  }
  // dacPtr->BOR |= (GSAO_SFT_TRIG);
}

// *****************************************************************************
// Routine to find and map GSA DAC modules
// *****************************************************************************
long mapDac()
{
  static struct pci_dev *dacdev;
  static unsigned int pci_io_addr;
  int status;
  static long cpu_addr;

  dacdev = NULL;
  status = 0;

  // Search system for any module with PLX-9056 and PLX id
  while((dacdev = pci_find_device(PLX_VID, PLX_TID, dacdev))) {
	// if found, verify it is a DAC module
        if((dacdev->subsystem_device == DAC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
                printk("dac card on bus %d; device %d\n",
                        dacdev->bus->number,
                        dacdev->devfn);
                status = 1;
        }
        if(status) break;
  }
  if(status)
  {
	  // Enable the device, PCI required
	  pci_enable_device(dacdev);
	  // Register module as Master capable, required for DMA
	  pci_set_master(dacdev);
	  // Get the PLX chip PCI address, it is advertised at address 0
	  pci_read_config_dword(dacdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
	  printk("pci0 = 0x%x\n",pci_io_addr);
	  _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	  // Set up a pointer to DMA registers on PLX chip
	  dacDma = (PLX_9056_DMA *)_dac_add;

	  // Get the DAC register address
	  pci_read_config_dword(dacdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  printk("dac pci2 = 0x%x\n",pci_io_addr);
	  _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	  printk("DAC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_dac_add);
	  dacPtr = (GSA_DAC_REG *)_dac_add;

	  printk("DAC BCR = 0x%x\n",dacPtr->BCR);
	  // Reset the DAC board and wait for it to finish (3msec)
	  dacPtr->BCR |= GSAO_RESET;

	  do{
	  }while((dacPtr->BCR & GSAO_RESET) != 0);

	  // Following setting will also enable 2s complement by clearing offset binary bit
	  dacPtr->BCR = (GSAO_OUT_RANGE_05 | GSAO_SIMULT_OUT);
	  printk("DAC BCR after init = 0x%x\n",dacPtr->BCR);
	  printk("DAC CSR = 0x%x\n",dacPtr->CSR);

	  dacPtr->BOR = GSAO_FIFO_16;
	  // dacPtr->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
	  dacPtr->BOR |= (GSAO_ENABLE_CLK);
	  printk("DAC BOR = 0x%x\n",dacPtr->BOR);
	  cpu_addr = pci_alloc_consistent(dacdev,0x200,&dac_dma_handle);
	  return(cpu_addr);
  }
  else {
	return (-1);
  }

}

// *****************************************************************************
// Routine to find and map GSA ADC modules
// *****************************************************************************
long mapAdc()
{
  static struct pci_dev *adcdev;
  static unsigned int pci_io_addr;
  int status;
  static long cpu_addr;

  adcdev = NULL;
  status = 0;
  while((adcdev = pci_find_device(PLX_VID, PLX_TID, adcdev))) {
	if((adcdev->subsystem_device == ADC_SS_ID) && (adcdev->subsystem_vendor == PLX_VID))
	{
		printk("adc card on bus %d; device %d\n",
			adcdev->bus->number,
			adcdev->devfn);
		status = 1;
	}
	if(status) break;
  }
  pci_enable_device(adcdev);
  pci_set_master(adcdev);
 // Get the PLX chip address
  pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
  printk("pci0 = 0x%x\n",pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  adcDma = (PLX_9056_DMA *)_adc_add;
  plxIcr = (PLX_9056_INTCTRL *)_adc_add;

  // Get the ADC register address
  pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
  printk("pci2 = 0x%x\n",pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("ADC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_adc_add);
  adcPtr = (GSA_ADC_REG *)_adc_add;

  printk("BCR = 0x%x\n",adcPtr->BCR);
  // Reset the ADC board
  adcPtr->BCR |= GSAI_RESET;
  do{
  }while((adcPtr->BCR & GSAI_RESET) != 0);

  // Write in a sync word
  adcPtr->SMUW = 0x0002;
  adcPtr->SMLW = 0x0001;

  // Set ADC to 64 channel = 32 differential channels
  adcPtr->SSC = (GSAI_64_CHANNEL);
  // adcPtr->SSC = GSAI_64_CHANNEL;
  printk("SSC = 0x%x\n",adcPtr->SSC);
  adcPtr->BCR |= (GSAI_FULL_DIFFERENTIAL);
  adcPtr->BCR &= ~(GSAI_SET_2S_COMP);

  // Set sample rate close to 16384Hz
  adcPtr->RAG = 0x10BEC;
  printk("RAG = 0x%x\n",adcPtr->RAG);
  printk("BCR = 0x%x\n",adcPtr->BCR);
  adcPtr->RAG &= ~(GSAI_SAMPLE_START);
  adcPtr->BCR |= GSAI_AUTO_CAL;
  do {
  }while((adcPtr->BCR & GSAI_AUTO_CAL) != 0);
  adcPtr->RAG |= GSAI_SAMPLE_START;
  adcPtr->IDBC = GSAI_CLEAR_BUFFER;
  adcPtr->SSC = (GSAI_64_CHANNEL | GSAI_EXTERNAL_SYNC);
  cpu_addr = pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle);
  return(cpu_addr);

}

// *****************************************************************************
// Routine to find and map VMIC RFM modules
// *****************************************************************************
long mapcard(int state, int memsize) {

  static struct pci_dev *pcidev;
  static unsigned int pci_bus;
  static unsigned int pci_device_fn;
  static unsigned int pci_io_addr;
  static char *dmaAdd;
  static char *csrAddr;
  static int using_dac;
  static long cpu_addr;
  static unsigned int dmaAddress;
  static unsigned int csrAddress;
  int *dataOut;
  int ii;

  if(state == 1)
  {
  pcidev = 0;
  if ((pcidev = pci_find_device(0x114a, PCI_ANY_ID, 0))) {
	pci_bus = pcidev->bus->number;
	pci_device_fn = pcidev->devfn;
	printk("Found RFM CARD\n");
	rfmType = pcidev->device;
	printk("\tCard Type = 0x%x\n",rfmType);
        printk("0x%x card on bus %d; device %d\n", rfmType,
		pci_bus,
		pci_device_fn);
  } else {
        printk("VMIC RFM is not found.\n");
        return 0;
  }



  pci_enable_device(pcidev);
  pci_set_master(pcidev);

  pci_read_config_dword(pcidev, 
                 rfmType == 0x5565? PCI_BASE_ADDRESS_3:PCI_BASE_ADDRESS_1,
                 &pci_io_addr);
  printk("VMIC%x PCI bus address=0x%x\n", rfmType, pci_io_addr);

        if (!pci_set_dma_mask(pcidev, DMA_64BIT_MASK)) {
                using_dac = 1;
		pci_set_consistent_dma_mask(pcidev, DMA_64BIT_MASK);
		printk("RFM is 64 bit capable\n");
        } else if (!pci_set_dma_mask(pcidev, DMA_32BIT_MASK)) {
                using_dac = 0;
		pci_set_consistent_dma_mask(pcidev, DMA_32BIT_MASK);
		printk("RFM is 32 bit capable\n");
        } else {
                printk(
                       "mydev: No suitable DMA available.\n");
        }


  pRfmMem = ioremap_nocache((unsigned long)pci_io_addr, memsize);
  printk("VMIC%x protected address=0x%x\n", rfmType, (int)pRfmMem);

  if(rfmType == 0x5565)
  {
	  pci_read_config_dword(pcidev,
			 rfmType == 0x5565? PCI_BASE_ADDRESS_0:PCI_BASE_ADDRESS_1,
			 &dmaAddress);

	  printk("DMA address is 0x%lx\n",dmaAddress);

	  dmaAdd = ioremap_nocache(dmaAddress,0x200);
	  p5565Dma = (VMIC5565DMA *)dmaAdd;
	  p5565Rtr = (VMIC5565RTR *)dmaAdd;
	  printk("5565DMA at 0x%x\n",(int)p5565Dma);
  }


  pci_read_config_dword(pcidev,
                 rfmType == 0x5565? PCI_BASE_ADDRESS_2:PCI_BASE_ADDRESS_1,
                 &csrAddress);
  printk("CSR address is 0x%lx\n",csrAddress);
  csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

  if(rfmType == 0x5565)
  {
	  p5565Csr = (VMIC5565_CSR *)csrAddr;
	  p5565Csr->LCSR1 &= ~TURN_OFF_5565_FAIL;
	  printk("Board id = 0x%x\n",p5565Csr->BID);
	  printk("Node id = 0x%x\n",p5565Csr->NID);
  }
  else
  {
	  p5579Csr = (struct VMIC5579_MEM_REGISTER *)pRfmMem;
	  p5579Csr->CSR2 = TURN_OFF_FAIL_LITE;
	  printk("Node id = 0x%x\n",p5579Csr->NID);
  }

  if(rfmType == 0x5565)
{
  cpu_addr = pci_alloc_consistent(pcidev,0x2000,&rfmDmaHandle);
  printk("cpu addr = 0x%lx  dma addr = 0x%lx\n",cpu_addr,rfmDmaHandle);
  return cpu_addr;
}
else return(0);


}
else {
	iounmap ((void *)pRfmMem);
	iounmap ((void *)dmaAdd);
	iounmap ((void *)csrAddr);
  pci_free_consistent(pcidev,0x2000,cpu_addr,rfmDmaHandle);
  return(0);
}
}
