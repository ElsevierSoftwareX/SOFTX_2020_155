#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#ifdef USE_VMIC_RFM
#include <drv/vmic5579.h>
#include <drv/vmic5565.h>
#define RFM_WRITE	0x0
#define RFM_READ	0x8
#endif
#ifndef NO_DAQ
#include <drv/gmnet.h>
#endif
#include <drv/cdsHardware.h>
#include <drv/map.h>

int mapRfm(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev, int);

// PCI Device variables
volatile PLX_9056_DMA *adcDma[MAX_ADC_MODULES];	/* DMA struct for GSA ADC */
volatile PLX_9056_DMA *dacDma[MAX_DAC_MODULES];	/* DMA struct for GSA DAC */
volatile PLX_9056_INTCTRL *plxIcr;		/* Ptr to interrupt cntrl reg on PLX chip */
dma_addr_t adc_dma_handle[MAX_ADC_MODULES];	/* PCI add of ADC DMA memory */
dma_addr_t dac_dma_handle[MAX_DAC_MODULES];	/* PCI add of ADC DMA memory */
volatile GSA_ADC_REG *adcPtr[MAX_ADC_MODULES];	/* Ptr to ADC registers */
volatile GSA_FAD_REG *fadcPtr[MAX_ADC_MODULES]; /* Ptr to 2MS/sec ADC registers */
volatile GSA_DAC_REG *dacPtr[MAX_DAC_MODULES];	/* Ptr to DAC registers */
volatile VMIC5565_CSR *p5565Csr;


int checkAdcDmaDone(int module)
{
	if((adcDma[module]->DMA_CSR & GSAI_DMA_DONE) == 0) return(0);
	else return(1);
}
// *****************************************************************************
// Function checks if DMA from ADC module is complete
// Note: This function not presently used.
// *****************************************************************************
int adcDmaDone(int module, int *data)
{
	do{
	}while((adcDma[module]->DMA_CSR & GSAI_DMA_DONE) == 0);
	// First channel should be marked with an upper bit set
	if (*data == 0) return 0; else return 16;
}
// *****************************************************************************
// Function checks if DMA from ADC module is complete
// Note: This function not presently used.
// *****************************************************************************
int dacDmaDone(int module)
{
	do{
	}while((dacDma[module]->DMA_CSR & GSAI_DMA_DONE) == 0);
	return(1);
}

// *****************************************************************************
// Function clears ADC buffer and starts acquisition via external clock
// Also sets up ADC for Demand DMA mode and set GO bit in DMA Mode Register
// *****************************************************************************
int gsaAdcTrigger(int adcCount, int adcType[])
{
  int ii;
  for(ii=0;ii<adcCount;ii++)
  {
     if(adcType[ii] == GSC_16AI64SSA)
     {
	  // adcPtr[ii]->BCR &= ~(GSAI_DMA_DEMAND_MODE);
	  adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
	  // gsaAdcDma2(ii);
	  adcPtr[ii]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
	  adcPtr[ii]->BCR |= GSAI_ENABLE_X_SYNC;
    }else{
	  // This is the 2MS ADC module
          fadcPtr[ii]->BCR |= GSAF_ENABLE_INPUT;
    }
  }
  return(0);
}

// *****************************************************************************
// Function...
// *****************************************************************************
int gsaDacTrigger(int dacCount)
{
   int ii;

   for (ii = 0; ii < dacCount; ii++)
   {
#ifdef DAC_INTERNAL_CLOCKING
#error
      dacPtr[ii]->BOR |= (GSAO_ENABLE_CLK);
#else
      dacPtr[ii]->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
#endif
   }

   return(0);
}

// *****************************************************************************
// Function stops ADC acquisition
// *****************************************************************************
int gsaAdcStop()
{
  adcPtr[0]->BCR &= ~(GSAI_ENABLE_X_SYNC);
  return(0);
}


// *****************************************************************************
// Test if ADC has a sample ready
// Only to be used with 2MS/sec ADC module.
// *****************************************************************************
int checkAdcRdy(int count,int numAdc,int type)
{
  int dataCount;

          int i = 0;
          switch(type)
          {
#if 0
                case GSC_18AI32SSC1M:
                  do {
                        dataCount = adcPtr[0]->BUF_SIZE;
                        if (i == 1000000) {
                                break;
                        }
                        i++;
                  }while(dataCount < count);
                  break;
#endif
                case GSC_16AI64SSA:
                  do {
                        dataCount = adcPtr[0]->BUF_SIZE;
                        if (i == 1000000) {
                                break;
                        }
                        i++;
                  }while(dataCount < count);
                  break;
                // Default is GSC 2MHz ADC
                default:
                  do {
                        dataCount = fadcPtr[0]->IN_BUF_SIZE;
                        if (i == 1000000) {
                                break;
                        }
                        i++;
                  }while(dataCount < (count / 8));
                break;
          }
          // If timeout, return error
          if (i == 1000000) return -1;
  return(dataCount);
}

// *****************************************************************************
// This routine sets up the DMA registers once on code initialization.
// *****************************************************************************
int gsaAdcDma1(int modNum, int adcType, int byteCount)
{
  adcDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  adcDma[modNum]->DMA0_PCI_ADD = (int)adc_dma_handle[modNum];
  if(adcType == GSC_16AI64SSA)
  {
	adcDma[modNum]->DMA0_LOC_ADD = GSAI_DMA_LOCAL_ADDR;
	adcDma[modNum]->DMA0_BTC = byteCount;
  } else  {
        adcDma[modNum]->DMA0_LOC_ADD = GSAF_DMA_LOCAL_ADDR;
        adcDma[modNum]->DMA0_BTC = byteCount/8;
  }
  adcDma[modNum]->DMA0_DESC = GSAI_DMA_TO_PCI;
  return(1);
}

// *****************************************************************************
// This routine starts a DMA operation.
// It must first be setup by the Dma1 call above.
// *****************************************************************************
void gsaAdcDma2(int modNum)
{
  adcDma[modNum]->DMA_CSR = GSAI_DMA_START;
}

// *****************************************************************************
// This routine sets up the DAC DMA registers.
// It is only called once to set up the number of bytes to be preloaded into
// the DAC prior to fe code realtime loop.
// *****************************************************************************
int dacDmaPreload(int modNum, int samples)
{
          dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
          dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
          dacDma[modNum]->DMA0_LOC_ADD = 0x18;
          dacDma[modNum]->DMA0_BTC = 0x20 * samples;
          dacDma[modNum]->DMA0_DESC = 0x0;
  return(1);
}


// *****************************************************************************
// This routine sets up the DAC DMA registers once on code initialization.
// *****************************************************************************
int gsaDacDma1(int modNum, int dacType)
{
  if(dacType == GSC_16AISS8AO4 || dacType == GSC_18AISS8AO8)
  {
          dacDma[modNum]->DMA1_MODE = GSAI_DMA_MODE_NO_INTR;
          dacDma[modNum]->DMA1_PCI_ADD = (int)dac_dma_handle[modNum];
          dacDma[modNum]->DMA1_LOC_ADD = GSAF_DAC_DMA_LOCAL_ADDR;
#ifdef OVERSAMPLE_DAC
          dacDma[modNum]->DMA1_BTC = 0x10*OVERSAMPLE_TIMES;
#else
          dacDma[modNum]->DMA1_BTC = 0x10;
#endif
          dacDma[modNum]->DMA1_DESC = 0x0;
  } else {
	  dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
	  dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
	  dacDma[modNum]->DMA0_LOC_ADD = 0x18;
#ifdef OVERSAMPLE_DAC
#ifdef DAC_OVER2
         dacDma[modNum]->DMA0_BTC = 0x40*OVERSAMPLE_TIMES*2;
#else
         dacDma[modNum]->DMA0_BTC = 0x40*OVERSAMPLE_TIMES;
#endif
#else
	  dacDma[modNum]->DMA0_BTC = 0x40;
#endif
	  dacDma[modNum]->DMA0_DESC = 0x0;
  }
  return(1);
}

// *****************************************************************************
// This routine starts a DMA operation to a DAC module.
// It must first be setup by the Dma1 call above.
// *****************************************************************************
void gsaDacDma2(int modNum, int dacType)
{
  if(dacType == GSC_16AISS8AO4 || dacType == GSC_18AISS8AO8)
  {
        dacDma[modNum]->DMA_CSR = GSAI_DMA1_START;
  } else {
	dacDma[modNum]->DMA_CSR = GSAI_DMA_START;
  }
}

// *****************************************************************************
// Routine to read ACCESS DIO modules
// *****************************************************************************
unsigned int readDio(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int status;
	status = inb(pHardware->pci_dio[modNum]);
	return(status);
}

// *****************************************************************************
// Routine to write ACCESS DIO modules
// *****************************************************************************
void writeDio(CDS_HARDWARE *pHardware, int modNum, int data)
{
	outb(data & 0xff,pHardware->pci_dio[modNum]+DIO_C_REG);
}

// *****************************************************************************
// Routine to initialize ACCESS DIO modules
// *****************************************************************************
int mapDio(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;

	  devNum = pHardware->dioCount;
	  pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  printk("dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_dio[devNum] = (pci_io_addr - 1);
	  printk("diospace = 0x%x\n",pHardware->pci_dio[devNum]);
	  outb_p(DIO_C_OUTPUT,pHardware->pci_dio[devNum]+DIO_CTRL_REG);
	  outb(0x00,pHardware->pci_dio[devNum]+DIO_C_REG);

	  pHardware->dioCount ++;
	  return(0);
}

// *****************************************************************************
// Routine to read ACCESS IIRO-8 Isolated DIO modules
// *****************************************************************************
unsigned int readIiroDio(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int status;
	status = inb(pHardware->pci_do[modNum] + IIRO_DIO_INPUT);
	return(status);
}

// *****************************************************************************
// Routine to read ACCESS IIRO-8 Isolated DIO modules outputs from register
// *****************************************************************************
unsigned int readIiroDioOutput(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int status;
	status = inb(pHardware->pci_do[modNum] + IIRO_DIO_OUTPUT);
	return(status);
}

// *****************************************************************************
// Routine to write ACCESS IIRO-8 Isolated DIO modules
// *****************************************************************************
void writeIiroDio(CDS_HARDWARE *pHardware, int modNum, int data)
{
	outb(data & 0xff, pHardware->pci_do[modNum] + IIRO_DIO_OUTPUT);
}

// *****************************************************************************
// Routine to initialize ACCESS IIRO-8 Isolated DIO modules
// *****************************************************************************
int mapIiroDio(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;
	  devNum = pHardware->doCount;
	  pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  printk("iiro-8 dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = pci_io_addr-1;
	  pHardware->doType[devNum] = ACS_8DIO;
	  pHardware->doInstance[devNum]  = pHardware->iiroDioCount;
	  pHardware->iiroDioCount ++;
	  printk("iiro-8 diospace = 0x%x\n",pHardware->pci_do[devNum]);

	  pHardware->doCount ++;

	  return(0);
}

// *****************************************************************************
// Routine to read ACCESS IIRO-16 Isolated DIO modules
// *****************************************************************************
unsigned int readIiroDio1(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int v, v1;
  v = inb(pHardware->pci_do[modNum] + IIRO_DIO_INPUT);
  v1 = inb(pHardware->pci_do[modNum] + 4 + IIRO_DIO_INPUT);
  return v | (v1 << 8);
return (v);
}

// *****************************************************************************
// Routine to write ACCESS IIRO-16 Isolated DIO modules
// *****************************************************************************
void writeIiroDio1(CDS_HARDWARE *pHardware, int modNum, int data)
{
	outb(data & 0xff, pHardware->pci_do[modNum] + IIRO_DIO_OUTPUT);
	outb((data >> 8) & 0xff, pHardware->pci_do[modNum] + 4 + IIRO_DIO_OUTPUT);
}

// *****************************************************************************
// Routine to initialize ACCESS IIRO-16 Isolated DIO modules
// *****************************************************************************
int mapIiroDio1(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;

	  devNum = pHardware->doCount;
	  pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  printk("iiro-16 dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = pci_io_addr-1;
	  pHardware->doType[devNum] = ACS_16DIO;
	  pHardware->doInstance[devNum]  = pHardware->iiroDio1Count;
	  pHardware->iiroDio1Count ++;
	  printk("iiro-16 diospace = 0x%x\n",pHardware->pci_do[devNum]);

	  pHardware->doCount ++;
	  return(0);
}


// *****************************************************************************
// Routine to Initialize CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
int mapContec32out(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;
  int id;

	  devNum = pHardware->doCount;
	  pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_0,&pci_io_addr);
	  printk("contec dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = pci_io_addr-1;
	  printk("contec32L diospace = 0x%x\n",pHardware->pci_do[devNum]);
	  pci_read_config_dword(diodev,PCI_REVISION_ID,&id);
	  printk("contec dio pci2 card number= 0x%x\n",(id & 0xf));
	  pHardware->doType[devNum] = CON_32DO;
	  pHardware->doCount ++;
	  pHardware->doInstance[devNum]  = pHardware->cDo32lCount;
	  pHardware->cDo32lCount ++;
	  return(0);
}

// *****************************************************************************
// Routine to write CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
unsigned int writeCDO32l(CDS_HARDWARE *pHardware, int modNum, unsigned int data)
{
	outl(data,pHardware->pci_do[modNum]);
	return(inl(pHardware->pci_do[modNum]));
}

// *****************************************************************************
// Routine to read CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
unsigned int readCDO32l(CDS_HARDWARE *pHardware, int modNum)
{
	return(inl(pHardware->pci_do[modNum]));
}


// *****************************************************************************
// Routine to initialize DAC modules
// *****************************************************************************
int mapDac(CDS_HARDWARE *pHardware, struct pci_dev *dacdev)
{
  int devNum;
  char *_dac_add;				/* DAC register address space */
  static unsigned int pci_io_addr;

	  devNum = pHardware->dacCount;
          // Enable the device, PCI required
          pci_enable_device(dacdev);
          // Register module as Master capable, required for DMA
          pci_set_master(dacdev);
          // Get the PLX chip PCI address, it is advertised at address 0
          pci_read_config_dword(dacdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
          printk("pci0 = 0x%x\n",pci_io_addr);
          _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
          // Set up a pointer to DMA registers on PLX chip
          dacDma[devNum] = (PLX_9056_DMA *)_dac_add;

	  // Get the DAC register address
	  pci_read_config_dword(dacdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  printk("dac pci2 = 0x%x\n",pci_io_addr);
	  _dac_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
	  printk("DAC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_dac_add);
	  dacPtr[devNum] = (GSA_DAC_REG *)_dac_add;

	  printk("DAC BCR = 0x%x\n",dacPtr[devNum]->BCR);
	  // Reset the DAC board and wait for it to finish (3msec)
	  dacPtr[devNum]->BCR |= GSAO_RESET;
	

	  do{
	  }while((dacPtr[devNum]->BCR & GSAO_RESET) != 0);

	  // Following setting will also enable 2s complement by clearing offset binary bit
	  dacPtr[devNum]->BCR = (GSAO_OUT_RANGE_10 | GSAO_SIMULT_OUT);
	  printk("DAC BCR after init = 0x%x\n",dacPtr[devNum]->BCR);
	  printk("DAC CSR = 0x%x\n",dacPtr[devNum]->CSR);

#ifdef OVERSAMPLE_DAC
	  // Larger buffer required when in oversampling mode 
	  dacPtr[devNum]->BOR = GSAO_FIFO_1024;
#else
	  dacPtr[devNum]->BOR = GSAO_FIFO_16;
#endif
//ifdef DAC_INTERNAL_CLOCKING
//	  dacPtr[devNum]->BOR |= (GSAO_ENABLE_CLK);
//else
//	  dacPtr[devNum]->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
//endif
	  printk("DAC BOR = 0x%x\n",dacPtr[devNum]->BOR);
	  pHardware->pci_dac[devNum] = 
		(long) pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
  	  pHardware->dacConfig[devNum] = (int) (dacPtr[devNum]->ASSC);
  	  pHardware->dacType[devNum] = GSC_16AO16;
	  pHardware->dacCount ++;
	  return(0);
}

// *****************************************************************************
// Routine to initialize GSA ADC modules
// *****************************************************************************
int mapAdc(CDS_HARDWARE *pHardware, struct pci_dev *adcdev)
{
  static unsigned int pci_io_addr;
  int devNum;
  char *_adc_add;				/* ADC register address space */


  devNum = pHardware->adcCount;
  pci_enable_device(adcdev);
  pci_set_master(adcdev);
 // Get the PLX chip address
  pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
  printk("pci0 = 0x%x\n",pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  adcDma[devNum] = (PLX_9056_DMA *)_adc_add;
  if(devNum == 0) plxIcr = (PLX_9056_INTCTRL *)_adc_add;

  // Get the ADC register address
  pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
  printk("pci2 = 0x%x\n",pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("ADC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_adc_add);
  adcPtr[devNum] = (GSA_ADC_REG *)_adc_add;

  printk("BCR = 0x%x\n",adcPtr[devNum]->BCR);
  // Reset the ADC board
  adcPtr[devNum]->BCR |= GSAI_RESET;
  do{
  }while((adcPtr[devNum]->BCR & GSAI_RESET) != 0);

  // Write in a sync word
  adcPtr[devNum]->SMUW = 0x0000;
  adcPtr[devNum]->SMLW = 0x0000;

  // Set ADC to 64 channel = 32 differential channels
  adcPtr[devNum]->BCR |= (GSAI_FULL_DIFFERENTIAL);

#ifdef ADC_EXTERNAL_SYNC
  adcPtr[devNum]->BCR |= (GSAI_ENABLE_X_SYNC);
  adcPtr[devNum]->AUX_SIO |= 0x80;
#endif

#if 0
  // Do not do that, it makes noise worse, not better!
  // Clear range bits to select lowest range 2.5V
  adcPtr[devNum]->BCR &= ~(GSAI_IN_RANGE_BITS);
#endif
  // Do not enable 2S compliment coding
  //adcPtr[devNum]->BCR &= ~(GSAI_SET_2S_COMP);

  // Set sample rate close to 16384Hz
  // Unit runs with external clock, so this probably not necessary
  adcPtr[devNum]->RAG = 0x117D8;
  printk("RAG = 0x%x\n",adcPtr[devNum]->RAG);
  printk("BCR = 0x%x\n",adcPtr[devNum]->BCR);
  adcPtr[devNum]->RAG &= ~(GSAI_SAMPLE_START);
  // Perform board calibration
  adcPtr[devNum]->BCR |= GSAI_AUTO_CAL;
  do {
  }while((adcPtr[devNum]->BCR & GSAI_AUTO_CAL) != 0);
  adcPtr[devNum]->RAG |= GSAI_SAMPLE_START;
  adcPtr[devNum]->IDBC = (GSAI_CLEAR_BUFFER | 0xff);
  // adcPtr[devNum]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
  adcPtr[devNum]->SSC = (GSAI_64_CHANNEL | GSAI_EXTERNAL_SYNC);
  printk("SSC = 0x%x\n",adcPtr[devNum]->SSC);
  printk("IDBC = 0x%x\n",adcPtr[devNum]->IDBC);
  pHardware->pci_adc[devNum] = (long) pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
  pHardware->adcType[devNum] = GSC_16AI64SSA;
  pHardware->adcConfig[devNum] = adcPtr[devNum]->ASSC;
  pHardware->adcCount ++;
  return(0);

}
// *****************************************************************************
// Routine to initialize GSA 2MS ADC modules and also 18-bit GSA modules
// The difference is indicated with the last argument
// *****************************************************************************
int mapFadc(CDS_HARDWARE *pHardware,
            struct pci_dev *adcdev,
            unsigned int board_type)
{
  static unsigned int pci_io_addr;
  int devNum;
  char *_adc_add;                               /* ADC register address space */

  devNum = pHardware->adcCount;
  pci_enable_device(adcdev);
  pci_set_master(adcdev);
 // Get the PLX chip address
  pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_0,&pci_io_addr);
  printk("pci0 = 0x%x\n",pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  adcDma[devNum] = (PLX_9056_DMA *)_adc_add;
  dacDma[pHardware->dacCount] = (PLX_9056_DMA *)_adc_add;
  if(devNum == 0) plxIcr = (PLX_9056_INTCTRL *)_adc_add;

  // Get the ADC register address
  pci_read_config_dword(adcdev,PCI_BASE_ADDRESS_2,&pci_io_addr);
  printk("pci2 = 0x%x\n",pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("ADC I/O address=0x%x  0x%lx\n", pci_io_addr,(long)_adc_add);
  fadcPtr[devNum] = (GSA_FAD_REG *)_adc_add;

  printk("BCR = 0x%x\n",fadcPtr[devNum]->BCR);
  printk("INPUT CONFIG = 0x%x\n",fadcPtr[devNum]->IN_CONF);
  // Reset the ADC board
  fadcPtr[devNum]->BCR |= GSAF_RESET;
  do{
  }while((fadcPtr[devNum]->BCR & GSAF_RESET) != 0);

  // Only enable two's compliment for the 16-bit board
  // DO NOT
  //if (board_type == GSC_16AISS8AO4) {
    //fadcPtr[devNum]->BCR &= ~(GSAF_SET_2S_COMP);
  //}

  fadcPtr[devNum]->RAG = GSAF_RATEA_32K;
  if (board_type == GSC_16AISS8AO4) {
    fadcPtr[devNum]->IN_CONF = 0x4f000400;
  } else {
    fadcPtr[devNum]->AO_03 = 0x00002a0f;
  }
  fadcPtr[devNum]->RGENC = GSAF_RATEC_1MHZ;
  //fadcPtr[devNum]->RGEND = GSAF_RATEC_1MHZ;
  fadcPtr[devNum]->BCR |= GSAF_ENABLE_BUFF_OUT;
  fadcPtr[devNum]->BCR |= GSAF_DAC_SIMULT;
  if (board_type == GSC_18AISS8AO8) {
    // External triggering
    // Eanble all 8 output channels
    fadcPtr[devNum]->OUT_CONF = 0x00002aff;
  }
  fadcPtr[devNum]->DAC_BUFF_OPS |= (GSAF_DAC_CLK_INIT | GSAF_DAC_4CHAN);
  fadcPtr[devNum]->DAC_BUFF_OPS |= GSAF_DAC_ENABLE_CLK;
  fadcPtr[devNum]->DAC_BUFF_OPS |= GSAF_DAC_CLR_BUFFER;
  if (board_type == GSC_16AISS8AO4) {
    fadcPtr[devNum]->BCR |= GSAF_DAC_ENABLE_RGC;
  }

  printk("RAG = 0x%x\n",fadcPtr[devNum]->RAG);
  printk("BCR = 0x%x\n",fadcPtr[devNum]->BCR);
  printk("BOO = 0x%x\n",fadcPtr[devNum]->DAC_BUFF_OPS);
  printk("INPUT CONFIG = 0x%x\n",fadcPtr[devNum]->IN_CONF);
  pHardware->adcConfig[devNum] = fadcPtr[devNum]->ASSC;
  pHardware->dacConfig[pHardware->dacCount] = fadcPtr[devNum]->ASSC;
  printk("ASSEMBLY CONFIG = 0x%x\n",pHardware->adcConfig[devNum]);

  pHardware->pci_adc[devNum] =
                (long)pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
  pHardware->pci_dac[pHardware->dacCount] =
                (long)pci_alloc_consistent(adcdev,0x2000,&dac_dma_handle[devNum]);
  pHardware->adcType[devNum] = board_type;
  pHardware->dacType[pHardware->dacCount] = board_type;
  pHardware->adcCount ++;
  pHardware->dacCount ++;
  return(0);
}


// *****************************************************************************
// Routine to find and map PCI adc/dac modules
// *****************************************************************************
int mapSymComGps(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev);
int mapPciModules(CDS_HARDWARE *pCds)
{
  static struct pci_dev *dacdev;
  int status;
  int i;
  int modCount = 0;
  int adc_cnt = 0;
  int fast_adc_cnt = 0;
  int dac_cnt = 0;
  int bo_cnt = 0;

  dacdev = NULL;
  status = 0;

  // Search system for any module with PLX-9056 and PLX id
  while((dacdev = pci_find_device(PLX_VID, PLX_TID, dacdev))) {
	// if found, check if it is a DAC module
        if((dacdev->subsystem_device == DAC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
		int use_it = 1;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			int i;
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == GSC_16AO16
				    && pCds->cards_used[i].instance == dac_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("dac card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
                  status = mapDac(pCds,dacdev);
		  modCount ++;
		}
		dac_cnt++;
        }
	// if found, check if it is an ADC module
	if((dacdev->subsystem_device == ADC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
	{
		int use_it = 1;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			int i;
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == GSC_16AI64SSA
				    && pCds->cards_used[i].instance == adc_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
		  printk("adc card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		  status = mapAdc(pCds,dacdev);
		  modCount ++;
		}
		adc_cnt++;
	}
        // if found, check if it is a Fast ADC module
    	// TODO: for the time of testing of the 18-bit board, it returned same PCI device number as the 16-bit fast GS board
	// This number will most likely change in the future.
        if((dacdev->subsystem_device == FADC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
		int use_it = 1;
		unsigned int board_type = -1;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			int i;
			for (i = 0; i < pCds->cards; i++) {
				if ((pCds->cards_used[i].type == GSC_16AISS8AO4 || pCds->cards_used[i].type == GSC_18AISS8AO8)
				    && pCds->cards_used[i].instance == fast_adc_cnt) {
					use_it = 1;
					board_type = pCds->cards_used[i].type;
					break;
				}
			}
		}
		if (use_it) {
                   printk("fast adc card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
                   status = mapFadc(pCds, dacdev, board_type);
                   modCount ++;
		}
		fast_adc_cnt++;
        }
  }

  dacdev = NULL;
  status = 0;
  // Search system for Digital I/O modules
  while((dacdev = pci_find_device(ACC_VID, ACC_TID, dacdev))) {
		printk("dio card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		status = mapDio(pCds,dacdev);
		modCount ++;
  }

  dacdev = NULL;
  status = 0;
  bo_cnt = 0;
  // Search for ACCESS PCI-IIRO-8 isolated I/O modules
  while((dacdev = pci_find_device(ACC_VID, ACC_IIRO_TID, dacdev))) {
		int use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			int i;
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == ACS_8DIO
				    && pCds->cards_used[i].instance == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Access BIO card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		  status = mapIiroDio(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }
		
  dacdev = NULL;
  status = 0;
  bo_cnt = 0;
  // Search for ACCESS PCI-IIRO-16 isolated I/O modules
  while((dacdev = pci_find_device(ACC_VID, ACC_IIRO_TID1, dacdev))) {
		int use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			int i;
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == ACS_16DIO
				    && pCds->cards_used[i].instance == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Access BIO-16 card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		  status = mapIiroDio1(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }

  dacdev = NULL;
  status = 0;
  bo_cnt = 0;

  // Search for Contec C_DO_32L_PE isolated I/O modules
  while((dacdev = pci_find_device(CONTEC_VID, C_DO_32L_PE, dacdev))) {
		int use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			int i;
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == CON_32DO
				    && pCds->cards_used[i].instance == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Contec BO card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		  status = mapContec32out(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }

  dacdev = NULL;
  status = 0;

  for (i = 0; i < MAX_RFM_MODULES; i++) {
  	pCds->pci_rfm[i] = 0;
  }

  // Search system for 5565 VMIC RFM modules
  while((dacdev = pci_find_device(VMIC_VID, VMIC_TID, dacdev))) {
		printk("5565 RFM card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		status = mapRfm(pCds,dacdev,0x5565);
		modCount ++;
  }

  dacdev = NULL;
  status = 0;

  // Search system for 5579 VMIC RFM modules
  while((dacdev = pci_find_device(VMIC_VID, VMIC_TID_5579, dacdev))) {
		printk("5579 RFM card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		status = mapRfm(pCds,dacdev,0x5579);
		modCount ++;
  }

  dacdev = NULL;
  status = 0;
  pCds->vmeBridgeCount = 0;
  pCds->vme[0] = 0;

#ifndef RTAI_BUILD
  // Look for SBS Technologies VME Bridges
  while((dacdev = pci_find_device(SBS_618_VID, SBS_618_TID, dacdev))) {
		printk("SBS 618 VME Bridge card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		status = mapSbsVmeBridge(pCds,dacdev);
		if (status == 0) {
		  // VME link initialized and mapped
		  modCount ++;
		}
  }
#endif

  dacdev = NULL;
  status = 0;
  pCds->gps = 0;
  // Look for Symmetricom GPS board
  if ((dacdev = pci_find_device(SYMCOM_VID, SYMCOM_BC635_TID, dacdev))) {
            	printk("Symmetricom GPS card on bus %x; device %x\n",
                   	dacdev->bus->number,
		   	PCI_SLOT(dacdev->devfn));
		status = mapSymComGps(pCds,dacdev);
		if (status == 0) {
		  // GPS board initialized and mapped
		  modCount ++;
		}
  }

  return(modCount);
}

// *****************************************************************************
// Routine to initialize VMIC RFM modules
// *****************************************************************************
int mapRfm(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev, int kind)
{
  static unsigned int pci_io_addr;
  int devNum;
  static char *csrAddr;
  static unsigned int csrAddress;

  devNum = pHardware->rfmCount;
  pci_enable_device(rfmdev);

  if (kind == 0x5565) {
    pci_read_config_dword(rfmdev, 
        		  PCI_BASE_ADDRESS_3,
                 	  &pci_io_addr);
    pHardware->pci_rfm[devNum] = (long)ioremap_nocache((unsigned long)pci_io_addr, 64*1024*1024);

    pci_read_config_dword(rfmdev, PCI_BASE_ADDRESS_2, &csrAddress);
    printk("CSR address is 0x%x\n", csrAddress);
    csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

    p5565Csr = (VMIC5565_CSR *)csrAddr;
    p5565Csr->LCSR1 &= ~TURN_OFF_5565_FAIL;
    printk("Board id = 0x%x\n",p5565Csr->BID);
    pHardware->rfmConfig[devNum] = p5565Csr->NID;
  } else if (kind == 0x5579) {
    struct VMIC5579_MEM_REGISTER *pciRegPtr;
    int nodeId;

    pci_read_config_dword(rfmdev, 
        		  PCI_BASE_ADDRESS_1,
                 	  &pci_io_addr);
    pHardware->pci_rfm[devNum] = (long)ioremap_nocache((unsigned long)pci_io_addr, 64*1024*1024);


    pciRegPtr = (struct VMIC5579_MEM_REGISTER *)pHardware->pci_rfm[devNum];
    pciRegPtr->CSR2 = TURN_OFF_5579_FAIL;

    nodeId = pciRegPtr->NID;
    printk("Board id = 0x%x\n", nodeId);
    pHardware->rfmConfig[devNum] = nodeId;
  }

  pHardware->rfmType[devNum] = kind;
  pHardware->rfmCount ++;
  return(0);
}

#ifdef USE_VMIC_RFM
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
#endif

#ifndef RTAI_BUILD
// *****************************************************************************
// Initialize SBS Technologies VME Bridge Model 618
// *****************************************************************************
int mapSbsVmeBridge(CDS_HARDWARE *pHardware, struct pci_dev *sbsdev)
{
  static unsigned int pci_io_addr, pci_io_addr1, pci_io_addr2, pci_io_addr3;
  int devNum;
  static char *csrAddr;
  static unsigned int csrAddress;

	//devNum = pHardware->vmeBridgeCount;
  	pci_enable_device(sbsdev);
  	pci_read_config_dword(sbsdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
	pci_io_addr &= 0xfffffff0;
	printk("VME bridge PCI BASE 0 address = %x\n", pci_io_addr);
	//unsigned char *addr1 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 64*1024);
	// Local status register
	unsigned char lsr = inb(pci_io_addr + SBS_618_LOCAL_STATUS_REGISTER);
	printk("Local status register = 0x%x\n", lsr);
	if (lsr & 1) {
	  printk("Remote power is off or the fiber cable is disconnected\n");
	  return 1;
	}
	printk("Remote power is on and the fiber cable is connected\n");
	// Read the Remote Status Register and discard the results
	inb(pci_io_addr + SBS_618_REMOTE_STATUS_REGISTER);

	// Set bit 7 of the Local Command Register
	unsigned char lcr = inb(pci_io_addr + SBS_618_LOCAL_COMMAND_REGISTER);
	outb(lcr & 0x80, pci_io_addr + SBS_618_LOCAL_COMMAND_REGISTER);
	  
	lsr = inb(pci_io_addr + SBS_618_LOCAL_STATUS_REGISTER);
	printk("Local status register = 0x%x\n", lsr);
	// make sure no error bits are set
	if (lsr & 128) {
	  printk("Fiber-Optic Interface Data Error\n");
	  return 1;
	}
	if (lsr & 64) {
	  printk("Remote Bus Error\n");
	  return 1;
	}

	// Map VME window memory (32 megs, base address 3)
	pci_read_config_dword(sbsdev, PCI_BASE_ADDRESS_1, &pci_io_addr1);
	pci_io_addr1 &= 0xfffffff0;
	pci_read_config_dword(sbsdev, PCI_BASE_ADDRESS_2, &pci_io_addr2);
	pci_io_addr2 &= 0xfffffff0;
	pci_read_config_dword(sbsdev, PCI_BASE_ADDRESS_3, &pci_io_addr3);
	pci_io_addr3 &= 0xfffffff0;
 	volatile unsigned int *vme_mapping_reg = (unsigned int *)ioremap_nocache((unsigned long)pci_io_addr2, 64*1024);
 	volatile unsigned int *pci_reg = (unsigned int *)ioremap_nocache((unsigned long)pci_io_addr1, 64*1024);
 	volatile unsigned int *vme_mem = (unsigned int *)ioremap_nocache((unsigned long)pci_io_addr3, 32*1024*1024);
 	printk("VME mapping registers =(0x%x, 0x%x), VME memory =(0x%x, 0x%x)\n", pci_io_addr2, vme_mapping_reg, pci_io_addr3, vme_mem);

	// Program first regsiter to point us to VME 5588 card (at 0x50000000)
	*vme_mapping_reg = 0x50000360;

	// Program DMA mapping regs to our buffer
	volatile unsigned int *pci_dma_mapping_reg = vme_mapping_reg + SBS_618_MAPPING_REGS_DMA_PCI/sizeof(int);
	pHardware->buf = kmalloc(40*1024, GFP_KERNEL);
	memset(pHardware->buf,0,40*1024);
	*pci_dma_mapping_reg = virt_to_phys(pHardware->buf);
	printk("DMA buffer address (kmalloc) 0x%x, physical 0x%x\n", pHardware->buf, *pci_dma_mapping_reg);

 	// Load Local DMA command register; do long word transfers
	outb(0x10, pci_io_addr + SBS_618_DMA_COMMAND);
	// Load PCI addresses
	outb(0, pci_io_addr + SBS_618_DMA_PCI_ADDRESS0);
	outb(0, pci_io_addr + SBS_618_DMA_PCI_ADDRESS1);
	outb(0, pci_io_addr + SBS_618_DMA_PCI_ADDRESS2);
	// Load remote DMA address with starting VME address
	outb(0x10, pci_io_addr + SBS_618_DMA_VME_ADDRESS0);
	outb(0, pci_io_addr + SBS_618_DMA_VME_ADDRESS1);
	outb(0, pci_io_addr + SBS_618_DMA_VME_ADDRESS2);
	outb(0x50, pci_io_addr + SBS_618_DMA_VME_ADDRESS3);
	// Load remainder count (size of the last chunk, DMA goes in chunks of 256 bytes)
	outb(32, pci_io_addr + SBS_618_DMA_REMAINDER_COUNT);
	outb(32, pci_io_addr + SBS_618_DMA_REMOTE_REMAINDER_COUNT);
	// Load load DMA packet count (256 bytes per packet)
	outb(0, pci_io_addr + SBS_618_DMA_PACKET_COUNT0);
	outb(0, pci_io_addr + SBS_618_DMA_PACKET_COUNT1);
	// Disable VME interrupt in remote command register 2
	// Set for block mode operation as well
	outb(0x30, pci_io_addr + SBS_618_REMOTE_COMMAND_REGISTER2);
	// Program VME address modifier
	outb(0xf, pci_io_addr + SBS_618_REMOTE_VME_ADDRES_MODIFIER);
	
	vme_mem[0x10] = 0xbeef0000;
#if 0
	printk("0x%x 0x%x\n", vme_mem[0], vme_mem[1]);
	printk("0x%x 0x%x\n", vme_mem[2], vme_mem[3]);
	printk("0x%x 0x%x\n", vme_mem[4], vme_mem[5]);
	printk("0x%x 0x%x\n", vme_mem[6], vme_mem[7]);
	vme_mem[0x10] = 0xbeef0000;
	printk("0x%x\n", vme_mem[0x10]);

	iounmap ((void *)vme_mapping_reg);
	iounmap ((void *)vme_mem);
#endif

  	//pci_read_config_dword(rfmdev, PCI_BASE_ADDRESS_2, &csrAddress);
  	//printk("CSR address is 0x%x\n", csrAddress);
  	//csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

	//p5565Csr = (VMIC5565_CSR *)csrAddr;
	//p5565Csr->LCSR1 &= ~TURN_OFF_5565_FAIL;
	//printk("Board id = 0x%x\n",p5565Csr->BID);
  	//pHardware->rfmConfig[devNum] = p5565Csr->NID;

	pHardware->vme[pHardware->vmeBridgeCount] = vme_mem;
	pHardware->vme_reg[pHardware->vmeBridgeCount] = pci_reg;
	pHardware->vmeBridgeCount++;
	return(0);
}
#endif



// *****************************************************************************
// Initialize Symmetricom GPS card (model BC635PCI-U)
// *****************************************************************************
int mapSymComGps(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev)
{
  int i;
  static unsigned int pci_io_addr;

  pci_enable_device(gpsdev);
  pci_read_config_dword(gpsdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
  pci_io_addr &= 0xfffffff0;
  printk("PIC BASE 0 address = %x\n", pci_io_addr);

  unsigned char *addr1 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x40);
  printk("Remapped 0x%x\n", addr1);
  pHardware->gps = addr1;

  for (i = 0; i < 10; i++) {
    pHardware->gps[0] = 1;
    printk("Current time %ds %dus %dns \n", pHardware->gps[0x34/4], 0xfffff & pHardware->gps[0x30/4], 100 * ((pHardware->gps[0x30/4] >> 20) & 0xf));
  }
  pHardware->gps[0] = 1;
  unsigned int time0 = pHardware->gps[0x30/4];
  if (time0 & (1<<24)) printk("Flywheeling, unlocked...\n");
  else printk ("Locked!\n");
  return(0);
}
