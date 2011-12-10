#include <linux/types.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/delay.h>

#if 0
#ifndef NO_DAQ
#include <drv/gmnet.h>
#endif
#endif
#include <drv/cdsHardware.h>
#include <drv/map.h>
#include <commData2.h>

int mapRfm(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev, int);
void rfm55DMA(CDS_HARDWARE *pHardware, int card, int offset);

// PCI Device variables
volatile PLX_9056_DMA *adcDma[MAX_ADC_MODULES];	/* DMA struct for GSA ADC */
volatile PLX_9056_DMA *dacDma[MAX_DAC_MODULES];	/* DMA struct for GSA DAC */
volatile PLX_9056_INTCTRL *plxIcr;		/* Ptr to interrupt cntrl reg on PLX chip */
dma_addr_t adc_dma_handle[MAX_ADC_MODULES];	/* PCI add of ADC DMA memory */
dma_addr_t dac_dma_handle[MAX_DAC_MODULES];	/* PCI add of DAC DMA memory */
dma_addr_t rfm_dma_handle[MAX_DAC_MODULES];	/* PCI add of RFM DMA memory */
volatile GSA_ADC_REG *adcPtr[MAX_ADC_MODULES];	/* Ptr to ADC registers */
volatile GSA_FAD_REG *fadcPtr[MAX_ADC_MODULES]; /* Ptr to 2MS/sec ADC registers */
volatile GSA_DAC_REG *dacPtr[MAX_DAC_MODULES];	/* Ptr to DAC registers */
volatile VMIC5565_CSR *p5565Csr;		// VMIC5565 Control/Status Registers
volatile VMIC5565DMA *p5565Dma[2];		// VMIC5565 DMA Engine


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
// Function checks if DMA from DAC module is complete
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
	  adcPtr[ii]->BCR &= ~(GSAI_DMA_DEMAND_MODE);
	  // adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR_DEMAND;
	  adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR | 0x1000;
	  gsaAdcDma2(ii);
	  adcPtr[ii]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
	  adcPtr[ii]->BCR |= GSAI_ENABLE_X_SYNC;
    }else{
	  // This is the 2MS ADC module
          fadcPtr[ii]->BCR |= GSAF_ENABLE_INPUT;
	  // adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR_DEMAND;
	  adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR | 0x1000;
	  gsaAdcDma2(ii);
	  //adcPtr[ii]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
	  //adcPtr[ii]->BCR |= GSAI_ENABLE_X_SYNC;
    }
  }
  return(0);
}

// *****************************************************************************
// Function...
// *****************************************************************************
int gsaDacTrigger(CDS_HARDWARE *pHardware)
{
   int ii;

   for (ii = 0; ii < pHardware -> dacCount; ii++)
   {
#ifdef DAC_INTERNAL_CLOCKING
#error
      dacPtr[ii]->BOR |= (GSAO_ENABLE_CLK);
#else
      if (pHardware->dacType[ii] == GSC_18AO8) {
  	volatile GSA_18BIT_DAC_REG *dac18bitPtr = dacPtr[ii];
	dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_CLOCK_SRC;
	//dac18bitPtr->OUTPUT_CONFIG &= ~GSAO_18BIT_EXT_TRIG_SRC;
	//dac18bitPtr->BUF_OUTPUT_OPS |= (1<<11);
	dac18bitPtr->BUF_OUTPUT_OPS |= GSAO_18BIT_ENABLE_CLOCK;

	//dac18bitPtr->RATE_GEN_C = 40320000/(64*1024);
	//dac18bitPtr->RATE_GEN_D = 40320000/(64*1024);
	//dac18bitPtr->BCR |= (1 << 21);
	//dac18bitPtr->BCR |= (1 << 22);
	//dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_TRIG_SRC;
	printk("Triggered 18-bit DAC\n");
      } else {
      	dacPtr[ii]->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
      }
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
                        dataCount = adcPtr[numAdc]->BUF_SIZE;
#if 0
                  do {
                        dataCount = adcPtr[numAdc]->BUF_SIZE;
                        if (i == 1000000) {
                                break;
                        }
                        i++;
                  }while(dataCount < count);
#endif
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
          if (i >= 1000000) return -1;
	  // gsaAdcDma2(numAdc);
  return(dataCount);
}

// *****************************************************************************
// Check number of samples in ADC FIFO.
// *****************************************************************************
int checkAdcBuffer(int numAdc)
{
  int dataCount;

                dataCount = adcPtr[numAdc]->BUF_SIZE;
  return(dataCount);

}
//
// *****************************************************************************
// Check number of samples in DAC FIFO.
// *****************************************************************************
int checkDacBuffer(int numDac)
{
  int dataCount;
     dataCount = (dacPtr[numDac]->BOR >> 12) & 0xf;
     return(dataCount);
}

// *****************************************************************************
// Clear DAC FIFO.
// *****************************************************************************
int clearDacBuffer(int numDac)
{
	 dacPtr[numDac]->BOR |=  GSAO_CLR_BUFFER;
	 return(0);
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
        adcDma[modNum]->DMA0_BTC = 24; //byteCount/8;
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
  if (dacType == GSC_18AO8) {
          dacDma[modNum]->DMA1_MODE = GSAI_DMA_MODE_NO_INTR;
          dacDma[modNum]->DMA1_PCI_ADD = (int)dac_dma_handle[modNum];
          dacDma[modNum]->DMA1_LOC_ADD = GSAF_DAC_DMA_LOCAL_ADDR;
#ifdef OVERSAMPLE_DAC
          dacDma[modNum]->DMA1_BTC = 0x20*OVERSAMPLE_TIMES;
#else
          dacDma[modNum]->DMA1_BTC = 0x20;
#endif
          dacDma[modNum]->DMA1_DESC = 0x0;
  } else {
	  dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
	  dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
	  dacDma[modNum]->DMA0_LOC_ADD = 0x18;
#ifdef OVERSAMPLE_DAC
         dacDma[modNum]->DMA0_BTC = 0x40*OVERSAMPLE_TIMES;
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
void gsaDacDma2(int modNum, int dacType, int offset)
{
  if(dacType == GSC_18AO8) 
  {
        dacDma[modNum]->DMA1_PCI_ADD = ((int)dac_dma_handle[modNum] + offset);
        dacDma[modNum]->DMA_CSR = GSAI_DMA1_START;
  	//volatile GSA_18BIT_DAC_REG *dac18bitPtr = dacPtr[0];
	//dac18bitPtr->BCR |= (1 << 20);
  } else {
  	dacDma[modNum]->DMA0_PCI_ADD = ((int)dac_dma_handle[modNum] + offset);
	dacDma[modNum]->DMA_CSR = GSAI_DMA_START;
  }
}

// *****************************************************************************
// Routine to read ACCESS DIO modules
// *****************************************************************************
unsigned int readDio(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int status;
	status = inb(pHardware->pci_do[modNum]);
	return(status);
}

// *****************************************************************************
// Routine to write ACCESS DIO modules
// *****************************************************************************
void writeDio(CDS_HARDWARE *pHardware, int modNum, int data)
{
	outb(data & 0xff,pHardware->pci_do[modNum]+DIO_C_REG);
}

// *****************************************************************************
// Routine to initialize ACCESS DIO modules
// *****************************************************************************
int mapDio(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;
  int pedStatus;;

	  devNum = pHardware->dioCount;
	  pedStatus = pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_2,&pci_io_addr);
	  printk("dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = (pci_io_addr - 1);
	  printk("diospace = 0x%x\n",pHardware->pci_do[devNum]);
	  outb_p(DIO_C_OUTPUT,pHardware->pci_do[devNum]+DIO_CTRL_REG);
	  outb(0x00,pHardware->pci_do[devNum]+DIO_C_REG);
          pHardware->doType[devNum] = ACS_24DIO;
          pHardware->doInstance[devNum]  = pHardware->dioCount;
          pHardware->dioCount ++;
	  pHardware->doCount ++;
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
  int pedStatus;

	  devNum = pHardware->doCount;
	  pedStatus = pci_enable_device(diodev);
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
  int pedStatus;

	  devNum = pHardware->doCount;
	  pedStatus = pci_enable_device(diodev);
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
// Initialize CONTEC PCIe 6464 DIO module
// *****************************************************************************
int mapContec6464dio(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;
  int id;
  int pedStatus;

	  devNum = pHardware->doCount;
	  pedStatus = pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_0,&pci_io_addr);
	  printk("contec 6464 dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = pci_io_addr-1;
	  printk("contec32L diospace = 0x%x\n",pHardware->pci_do[devNum]);
	  pci_read_config_dword(diodev,PCI_REVISION_ID,&id);
	  printk("contec dio pci2 card number= 0x%x\n",(id & 0xf));
	  pHardware->doType[devNum] = CON_6464DIO;
	  pHardware->doCount ++;
	  pHardware->doInstance[devNum]  = pHardware->cDio6464lCount;
	  pHardware->cDio6464lCount ++;
	  devNum ++;
	  pHardware->pci_do[devNum] = (pci_io_addr-1) + 4;
	  pHardware->doType[devNum] = CON_6464DIO;
	  pHardware->doCount ++;
	  pHardware->doInstance[devNum]  = pHardware->cDio6464lCount;
	  pHardware->cDio6464lCount ++;
	  printk("contec32H diospace = 0x%x\n",pHardware->pci_do[devNum]);
	  return(0);
}

// *****************************************************************************
// Initialize CONTEC PCIe 1616 DIO module
// *****************************************************************************
int mapContec1616dio(CDS_HARDWARE *pHardware, struct pci_dev *diodev)
{
  static unsigned int pci_io_addr;
  int devNum;
  int id;
  int pedStatus;

	  devNum = pHardware->doCount;
	  pedStatus = pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_0,&pci_io_addr);
	  printk("contec 1616 dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = pci_io_addr-1;
	  printk("contec 1616 diospace = 0x%x\n",pHardware->pci_do[devNum]);
	  pci_read_config_dword(diodev,PCI_REVISION_ID,&id);
	  printk("contec dio pci2 card number= 0x%x\n",(id & 0xf));
	  pHardware->doType[devNum] = CON_1616DIO;
	  pHardware->doCount ++;
	  pHardware->doInstance[devNum]  = pHardware->cDio1616lCount;
	  pHardware->cDio1616lCount ++;
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
  int pedStatus;

	  devNum = pHardware->doCount;
	  pedStatus = pci_enable_device(diodev);
	  pci_read_config_dword(diodev,PCI_BASE_ADDRESS_0,&pci_io_addr);
	  printk("contec dio pci2 = 0x%x\n",pci_io_addr);
	  pHardware->pci_do[devNum] = pci_io_addr-1;
	  printk("contec32L diospace = 0x%x\n",pHardware->pci_do[devNum]);
	  pci_read_config_dword(diodev,PCI_REVISION_ID,&id);
	  printk("contec dio pci2 card number= 0x%x\n",(id & 0xf));
	  pHardware->doType[devNum] = CON_32DO;
	  pHardware->doCount ++;
	  pHardware->doInstance[devNum]  = pHardware->cDo32lCount;
	  //printk("pHardware count is at %d\n",pHardware->doCount);
	  //printk("pHardware cDo32lCount is at %d\n", pHardware->cDo32lCount);
	  pHardware->cDo32lCount ++;
	  return(0);
}

// *****************************************************************************
// Routine to write CONTEC PCIe-32 Isolated DO modules
// *****************************************************************************
unsigned int writeCDO32l(CDS_HARDWARE *pHardware, int modNum, unsigned int data)
{
	//printk("writeCDO32l modNum = %d\n",modNum);
	//printk("writeCDO32l data = %d\n",data);
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
// Routines to write to CONTEC PCIe-16 DIO modules
// *****************************************************************************
unsigned int writeCDIO1616l(CDS_HARDWARE *pHardware, int modNum, unsigned int data)
{
        outl(data,pHardware->pci_do[modNum]);
	// The binary output state bits register is at +2
        // return(inl(pHardware->pci_do[modNum] + 2));
	return(data);
}

unsigned int readCDIO1616l(CDS_HARDWARE *pHardware, int modNum)
{
	// The binary output state bits register is at +2
        return(inl(pHardware->pci_do[modNum] + 2));
}

unsigned int readInputCDIO1616l(CDS_HARDWARE *pHardware, int modNum)
{
	// Reading at +0 gives the input bits
        return(inl(pHardware->pci_do[modNum]));
}

// *****************************************************************************
// Routine to write to CONTEC PCIe-64 DIO modules
// *****************************************************************************
unsigned int writeCDIO6464l(CDS_HARDWARE *pHardware, int modNum, unsigned int data)
{

        outl(data,pHardware->pci_do[modNum] + 8);
	return data;
}

// *****************************************************************************
// Routine to read CONTEC PCIe-64 DIO modules - the output register
// *****************************************************************************
unsigned int readCDIO6464l(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int out;
	//
        out = inl(pHardware->pci_do[modNum] + 8);
	return out;
}

// *****************************************************************************
// Routine to read CONTEC PCIe-64 DIO modules
// *****************************************************************************
unsigned int readInputCDIO6464l(CDS_HARDWARE *pHardware, int modNum)
{
  unsigned int out;
        out = inl(pHardware->pci_do[modNum]);
	return out;
}

void set_8111_prefetch(struct pci_dev *dacdev) {
	struct pci_dev *dev = dacdev->bus->self;

	printk("set_8111_prefetch: subsys=0x%x; vendor=0x%x\n", dev->device, dev->vendor);
	if ((dev->device == 0x8111) && (dev->vendor == PLX_VID)) {
		unsigned int reg;
	  	// Handle PEX 8111 setup, enable prefetch, set pref size to 64
	  	// These numbers come from reverse engineering the GSC pxe8111 driver
	  	// and using their prefetch program to enable the prefetch and set pref size to 64
	  	pci_write_config_dword(dev,132, 72);
	  	pci_read_config_dword(dev,136, &reg);
	  	pci_write_config_dword(dev,136, reg);
	  	pci_write_config_dword(dev,132, 72);
	  	pci_read_config_dword(dev,136, &reg);
	  	pci_write_config_dword(dev,136, reg | 1);
	  	pci_write_config_dword(dev,132, 12);
	  	pci_read_config_dword(dev,136, &reg);
	  	pci_write_config_dword(dev,136, reg | 0x8000000);
	}
}

// *****************************************************************************
// Routine to initialize DAC modules
// *****************************************************************************
int mapDac(CDS_HARDWARE *pHardware, struct pci_dev *dacdev)
{
  int devNum;
  char *_dac_add;				/* DAC register address space */
  static unsigned int pci_io_addr;
  int pedStatus;

	  devNum = pHardware->dacCount;
          // Enable the device, PCI required
          pedStatus = pci_enable_device(dacdev);
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
	  // dacPtr[devNum]->BOR = GSAO_FIFO_16;
	  dacPtr[devNum]->BOR = GSAO_FIFO_256;
#endif
	  dacPtr[devNum]->BOR |=  GSAO_EXTERN_CLK;
	  printk("DAC BOR = 0x%x\n",dacPtr[devNum]->BOR);
	  pHardware->pci_dac[devNum] = 
		(long) pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
  	  pHardware->dacConfig[devNum] = (int) (dacPtr[devNum]->ASSC);
  	  pHardware->dacType[devNum] = GSC_16AO16;
	  pHardware->dacCount ++;

	  set_8111_prefetch(dacdev);
	  return(0);
}

int map18bitDac(CDS_HARDWARE *pHardware, struct pci_dev *dacdev)
{
  int devNum;
  char *_dac_add;				/* DAC register address space */
  static unsigned int pci_io_addr;
  int pedStatus;
  volatile GSA_18BIT_DAC_REG *dac18bitPtr;
  unsigned int reg;

	  devNum = pHardware->dacCount;
          // Enable the device, PCI required
          pedStatus = pci_enable_device(dacdev);
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

  	  dac18bitPtr = (GSA_18BIT_DAC_REG *)_dac_add;
	  dacPtr[devNum] = (GSA_DAC_REG *)_dac_add;

	  printk("DAC BCR = 0x%x\n",dac18bitPtr->BCR);
	  // Reset the DAC board and wait for it to finish (3msec)

	  dac18bitPtr->BCR |= GSAO_18BIT_RESET;
	
	  do{
	  }while((dac18bitPtr->BCR & GSAO_18BIT_RESET) != 0);

	  // Following setting will also enable 2s complement by clearing offset binary bit
	  dac18bitPtr->BCR &= ~GSAO_18BIT_OFFSET_BINARY;
	  dac18bitPtr->BCR |= GSAO_18BIT_SIMULT_OUT;
	  printk("DAC BCR after init = 0x%x\n",dac18bitPtr->BCR);
	  printk("DAC OUTPUT CONFIG = 0x%x\n",dac18bitPtr->OUTPUT_CONFIG);

	  // Enable 10 volt output range
	  dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_10VOLT_RANGE;
	  // Various bits setup
	  //dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_ENABLE_EXT_CLOCK;
	  //dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_CLOCK_SRC;
	  //dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_EXT_TRIG_SRC;
	  dac18bitPtr->OUTPUT_CONFIG |= GSAO_18BIT_DIFF_OUTS;

	  printk("DAC OUTPUT CONFIG after init = 0x%x\n",dac18bitPtr->OUTPUT_CONFIG);
#if 0
#ifdef OVERSAMPLE_DAC
	  // Larger buffer required when in oversampling mode 
	  dacPtr[devNum]->BOR = GSAO_FIFO_1024;
#else
	  // dacPtr[devNum]->BOR = GSAO_FIFO_16;
	  dacPtr[devNum]->BOR = GSAO_FIFO_1024;
#endif
	  dacPtr[devNum]->BOR |=  GSAO_EXTERN_CLK;
	  printk("DAC BOR = 0x%x\n",dacPtr[devNum]->BOR);
#endif

	// TODO: maybe dac_dma_handle should be a separate variable for 18-bit board?
	  pHardware->pci_dac[devNum] = 
		(long) pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
  	  pHardware->dacConfig[devNum] = (int) (dac18bitPtr->ASY_CONFIG);
  	  pHardware->dacType[devNum] = GSC_18AO8;
	  pHardware->dacCount ++;

	  set_8111_prefetch(dacdev);
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
  int pedStatus;


  devNum = pHardware->adcCount;
  pedStatus = pci_enable_device(adcdev);
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
  // adcPtr[devNum]->IDBC = (GSAI_CLEAR_BUFFER | 0xff);
  adcPtr[devNum]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
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
// Routine to initialize GSA PMC66_18AISS6C ADC modules 
// *****************************************************************************
int mapFadc(CDS_HARDWARE *pHardware,
            struct pci_dev *adcdev)
{
  static unsigned int pci_io_addr;
  int devNum;
  char *_adc_add;                               /* ADC register address space */
  int pedStatus;

  devNum = pHardware->adcCount;
  pedStatus = pci_enable_device(adcdev);
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
  printk("after reset BCR = 0x%x\n",fadcPtr[devNum]->BCR);
  printk("after reset INPUT CONFIG = 0x%x\n",fadcPtr[devNum]->IN_CONF);

  fadcPtr[devNum]->IN_CONF |= 1 << 9;
  fadcPtr[devNum]->BCR |=  1 << 12; // enable buffer
  fadcPtr[devNum]->BCR |=  1 << 13;
  printk("final BCR = 0x%x\n",fadcPtr[devNum]->BCR);
  printk("final INPUT CONFIG = 0x%x\n",fadcPtr[devNum]->IN_CONF);
  pHardware->adcConfig[devNum] = fadcPtr[devNum]->ASSC;
  printk("final ASSEMBLY CONFIG = 0x%x\n",pHardware->adcConfig[devNum]);

  pHardware->pci_adc[devNum] =
                (long)pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
  pHardware->adcType[devNum] = GSC_18AISS6C;
  pHardware->adcCount ++;
  return(0);
}


// *****************************************************************************
// Routine to find and map PCI adc/dac modules
// *****************************************************************************
int mapSymComGps(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev);
int mapTsyncGps(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev);
int mapPciModules(CDS_HARDWARE *pCds)
{
  static struct pci_dev *dacdev;
  int status;
  int i;
  int modCount = 0;
  int adc_cnt = 0;
  int fast_adc_cnt = 0;
  int dac_cnt = 0;
  int dac_18bit_cnt = 0;
  int bo_cnt = 0;
  int use_it;
  unsigned int board_type;

  dacdev = NULL;
  status = 0;

  // Search system for any module with PLX-9056 and PLX id
  while((dacdev = pci_get_device(PLX_VID, PLX_TID, dacdev))) {
	// Check if this is an 18bit DAC from General Standards
	if ((dacdev->subsystem_device == DAC_18BIT_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
	{
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == GSC_18AO8
				    && pCds->cards_used[i].instance == dac_18bit_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("18-bit dac card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
                  status = map18bitDac(pCds,dacdev);
		  modCount ++;
		}
		dac_18bit_cnt++;
	}
	// if found, check if it is a DAC module
        if((dacdev->subsystem_device == DAC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
		  printk("DAC card on bus %x; device %x prim %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn),
			dacdev->bus->secondary);
			/* See if ought to use this one or not */
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
#ifndef ADC_SLAVE
	if((dacdev->subsystem_device == ADC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
	{
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
		  printk("ADC card on bus %x; device %x prim %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn),
			dacdev->bus->secondary);
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == GSC_16AI64SSA
				    && pCds->cards_used[i].instance == adc_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
		  printk("adc card on bus %x; device %x prim %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn),
			dacdev->bus->secondary);
		  status = mapAdc(pCds,dacdev);
		  modCount ++;
		}
		adc_cnt++;
	}
#endif
        // if found, check if it is a Fast ADC module
    	// TODO: for the time of testing of the 18-bit board, it returned same PCI device number as the 16-bit fast GS board
	// This number will most likely change in the future.
        if((dacdev->subsystem_device == FADC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == GSC_18AISS6C
				    && pCds->cards_used[i].instance == fast_adc_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                   printk("fast adc card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
                   status = mapFadc(pCds, dacdev);
                   modCount ++;
		}
		fast_adc_cnt++;
        }
  }

  dacdev = NULL;
  status = 0;
  bo_cnt = 0;
  // Search for ACCESS PCI-DIO  modules
  while((dacdev = pci_get_device(ACC_VID, ACC_TID, dacdev))) {
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == ACS_24DIO
				    && pCds->cards_used[i].instance == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Access 24 BIO card on bus %x; device %x vendor 0x%x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn),
			dacdev->device);
		  status = mapDio(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }

  dacdev = NULL;
  status = 0;
  bo_cnt = 0;
  // Search for ACCESS PCI-IIRO-8 isolated I/O modules
  while((dacdev = pci_get_device(ACC_VID, PCI_ANY_ID, dacdev))) {
		if (dacdev->device != ACC_IIRO_TID && dacdev->device != ACC_IIRO_TID_OLD)
			continue;
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == ACS_8DIO
				    && pCds->cards_used[i].instance == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Access 8 BIO card on bus %x; device %x vendor 0x%x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn),
			dacdev->device);
		  status = mapIiroDio(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }
		
  dacdev = NULL;
  status = 0;
  bo_cnt = 0;
  // Search for ACCESS PCI-IIRO-16 isolated I/O modules
  while((dacdev = pci_get_device(ACC_VID, PCI_ANY_ID, dacdev))) {
		if (dacdev->device != ACC_IIRO_TID1 && dacdev->device != ACC_IIRO_TID1_OLD)
			continue;
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
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

  // Search for Contec C_DIO_6464L_PE isolated I/O modules
  while((dacdev = pci_get_device(CONTEC_VID, C_DIO_6464L_PE, dacdev))) {
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == CON_6464DIO
				    && (pCds->cards_used[i].instance * 2) == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Contec 6464 DIO card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		  status = mapContec6464dio(pCds,dacdev);
		  modCount ++;
		  modCount ++;
		}
		bo_cnt ++;
		bo_cnt ++;
  }


  dacdev = NULL;
  status = 0;
  bo_cnt = 0;

  // Search for Contec C_DIO_1616L_PE isolated I/O modules
  while((dacdev = pci_get_device(CONTEC_VID, C_DIO_1616L_PE, dacdev))) {
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
			for (i = 0; i < pCds->cards; i++) {
				if (pCds->cards_used[i].type == CON_1616DIO
				    && pCds->cards_used[i].instance == bo_cnt) {
					use_it = 1;
					break;
				}
			}
		}
		if (use_it) {
                  printk("Contec 1616 DIO card on bus %x; device %x\n",
                        dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		  status = mapContec1616dio(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }

  dacdev = NULL;
  status = 0;
  bo_cnt = 0;

  // Search for Contec C_DO_32L_PE isolated I/O modules
  while((dacdev = pci_get_device(CONTEC_VID, C_DO_32L_PE, dacdev))) {
		use_it = 0;
		if (pCds->cards) {
			use_it = 0;
			/* See if ought to use this one or not */
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
  while((dacdev = pci_get_device(VMIC_VID, VMIC_TID, dacdev))) {
		printk("5565 RFM card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		status = mapRfm(pCds,dacdev,0x5565);
		modCount ++;
  }

#if 0
  dacdev = NULL;
  status = 0;

  // Search system for 5579 VMIC RFM modules
  while((dacdev = pci_get_device(VMIC_VID, VMIC_TID_5579, dacdev))) {
		printk("5579 RFM card on bus %x; device %x\n",
			dacdev->bus->number,
			PCI_SLOT(dacdev->devfn));
		status = mapRfm(pCds,dacdev,0x5579);
		modCount ++;
  }
#endif

  dacdev = NULL;
  status = 0;
  pCds->gps = 0;
  pCds->gpsType = 0;
  // Look for Symmetricom GPS board
  if ((dacdev = pci_get_device(SYMCOM_VID, SYMCOM_BC635_TID, dacdev))) {
            	printk("Symmetricom GPS card on bus %x; device %x\n",
                   	dacdev->bus->number,
		   	PCI_SLOT(dacdev->devfn));
		status = mapSymComGps(pCds,dacdev);
		if (status == 0) {
		  // GPS board initialized and mapped
		  modCount ++;
		}
  }

  dacdev = NULL;
  status = 0;
  // Look for TSYNC GPS board
  if ((dacdev = pci_get_device(TSYNC_VID, TSYNC_TID, dacdev))) {
            	printk("TSYNC GPS card on bus %x; device %x\n",
                   	dacdev->bus->number,
		   	PCI_SLOT(dacdev->devfn));
		status = mapTsyncGps(pCds,dacdev);
		if (status == 0) {
		  // GPS board initialized and mapped
		  modCount ++;
		}
  }

  return(modCount);
}

// *****************************************************************************
// Routine to initialize VMIC RFM modules
// Support provided is only for use of RFM with RCG IPC components
// *****************************************************************************
int mapRfm(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev, int kind)
{
  static unsigned int pci_io_addr;
  int devNum;
  static char *csrAddr;
  static char *dmaAddr;
  static unsigned int csrAddress;
  static unsigned int dmaAddress;
  int pedStatus;
  int status;

  devNum = pHardware->rfmCount;
  pedStatus = pci_enable_device(rfmdev);
  // Register module as Master capable, required for DMA
  pci_set_master(rfmdev);

  if (kind == 0x5565) {
    // Find the reflected memory base address
    pci_read_config_dword(rfmdev, 
        		  PCI_BASE_ADDRESS_3,
                 	  &pci_io_addr);
    // Map full 128 MByte new cards only
    pHardware->pci_rfm[devNum] = (long)ioremap_nocache((unsigned long)pci_io_addr, 128*1024*1024);
    // Allocate local memory for IPC DMA xfers from RFM module
    pHardware->pci_rfm_dma[devNum] = (long) pci_alloc_consistent(rfmdev,IPC_BUFFER_SIZE,&rfm_dma_handle[devNum]);
    printk("RFM address is 0x%lx\n",pci_io_addr);

    // Find the RFM control/status register
    pci_read_config_dword(rfmdev, PCI_BASE_ADDRESS_2, &csrAddress);
    printk("CSR address is 0x%x\n", csrAddress);
    csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

    p5565Csr = (VMIC5565_CSR *)csrAddr;
    pHardware->rfm_reg[devNum] = p5565Csr;
    p5565Csr->LCSR1 &= ~TURN_OFF_5565_FAIL;
    p5565Csr->LCSR1 &= !1; // Turn off own data light

    printk("Board id = 0x%x\n",p5565Csr->BID);
    pHardware->rfmConfig[devNum] = p5565Csr->NID;
    //
    // Find DMA Engine controls in RFM Local Configuration Table 
    pci_read_config_dword(rfmdev,
		 PCI_BASE_ADDRESS_0,
		 &dmaAddress);
    printk("DMA address is 0x%lx\n",dmaAddress);
    dmaAddr = ioremap_nocache(dmaAddress,0x200);
    p5565Dma[devNum] = (VMIC5565DMA *)dmaAddr;
    pHardware->rfm_dma[devNum] = p5565Dma[devNum];
    printk("5565DMA at 0x%lx\n",(int)p5565Dma[devNum]);
    printk("5565 INTCR = 0x%lx\n",p5565Dma[devNum]->INTCSR);
    p5565Dma[devNum]->INTCSR = 0;	// Disable interrupts from this card
    printk("5565 INTCR = 0x%lx\n",p5565Dma[devNum]->INTCSR);
    printk("5565 MODE = 0x%lx\n",p5565Dma[devNum]->DMA0_MODE);
    printk("5565 DESC = 0x%lx\n",p5565Dma[devNum]->DMA0_DESC);
    // Preload some DMA settings
    // Only important items here are BTC and DESC fields, which are used 
    // later by DMA routine.
    p5565Dma[devNum]->DMA0_PCI_ADD = pHardware->pci_rfm_dma[0];
    p5565Dma[devNum]->DMA0_LOC_ADD = IPC_BASE_OFFSET;	// Start at RFM + 0x100 offset
    p5565Dma[devNum]->DMA0_BTC = IPC_RFM_XFER_SIZE;	// Set byte xfer to 256 bytes
    p5565Dma[devNum]->DMA0_DESC = VMIC_DMA_READ;	// Set RFM to local memory

// Legacy VMIC 5579 code, which is not fully functional and should be deleted soon.
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
// *****************************************************************************
// RFM DMA Read in support of commData
// *****************************************************************************
void rfm55DMA(CDS_HARDWARE *pHardware, int card, int offset)
{
int stats;
int myAddress;
int rfmAddress;

    rfmAddress = IPC_BASE_OFFSET + (offset * IPC_RFM_XFER_SIZE);
    myAddress = (pHardware->pci_rfm_dma[card] + (offset * IPC_RFM_XFER_SIZE));

    p5565Dma[card]->DMA_CSR = VMIC_DMA_CLR;	// Clear DMA DONE
    p5565Dma[card]->DMA0_PCI_ADD = myAddress;	// Computer address space
    p5565Dma[card]->DMA0_LOC_ADD = rfmAddress;	// RFM card offset from base
    // These were set during initialization, so don't need them again (save time)
    // p5565Dma[card]->DMA0_BTC = 0x400;	// Set byte xfer to 1024 bytes
    // p5565Dma[card]->DMA0_DESC = 0x8;	// Set RFM to local memory
    p5565Dma[card]->DMA_CSR = VMIC_DMA_START;	// Start DMA
}
// *****************************************************************************
// Routine to check RFM done bit and clear for next DMA
// *****************************************************************************
int rfm55DMAdone(int card)
{
int status;
int ii;
    ii = 0;
    do{
    status = p5565Dma[card]->DMA_CSR;	// Check DMA DONE bit (4)
    if((status & 0x10) == 0) udelay(1);
    ii ++;
    }while(((status & 0x10) == 0) && (ii < 10));
    p5565Dma[card]->DMA_CSR = 0x8;	// Clear DMA DONE
    return (ii);
}

// *****************************************************************************
// Initialize Symmetricom GPS card (model BC635PCI-U)
// *****************************************************************************
int mapSymComGps(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev)
{
  int i;
  static unsigned int pci_io_addr;
  int pedStatus;
  unsigned char *addr1;
  unsigned char *addr3;
  unsigned char *addr4;
  unsigned char *addr5;
  unsigned int *cmd;
  unsigned int *dramRead;
  unsigned int time0;
  unsigned char inPut[8];
  SYMCOM_REGISTER *timeReg;

  pedStatus = pci_enable_device(gpsdev);
  pci_read_config_dword(gpsdev, PCI_BASE_ADDRESS_2, &pci_io_addr);
  pci_io_addr &= 0xfffffff0;
  printk("PIC BASE 2 address = %x\n", pci_io_addr);

  addr1 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x40);
  printk("Remapped 0x%p\n", addr1);
  pHardware->gps = addr1;
  pHardware->gpsType = SYMCOM_RCVR;
  timeReg = (TSYNC_REGISTER *) addr1;;

  pci_read_config_dword(gpsdev, PCI_BASE_ADDRESS_3, &pci_io_addr);
  pci_io_addr &= 0xfffffff0;
  addr3 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("PIC BASE 3 address = 0x%x\n", pci_io_addr);
  printk("PIC BASE 3 address = 0x%p\n", addr3);
  dramRead = (unsigned int *)(addr3 + 0x82);
  cmd = (unsigned int *)(addr3 + 0x102);
  //
  // Set write and wait *****************************
  *cmd = 0xf6; // Request model ID
	i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
	udelay(1000);
        timeReg->ACK = 0x80;  // Trigger module to capture time
	do{
	udelay(1000);
	i++;
	}while((timeReg->ACK == 0) &&(i<20));
	if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
	printk("Model = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
	*cmd = 0x4915; // Request model ID
	i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
	udelay(1000);
        timeReg->ACK = 0x80;  // Trigger module to capture time
	do{
	udelay(1000);
	i++;
	}while((timeReg->ACK == 0) &&(i<20));
	if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
	printk("Model = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
	*cmd = 0x4416; // Request model ID
	i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
	udelay(1000);
        timeReg->ACK = 0x80;  // Trigger module to capture time
	do{
	udelay(1000);
	i++;
	}while((timeReg->ACK == 0) &&(i<20));
	if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
	printk("Model = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
	*cmd = 0x1519; // Request model ID
	i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
	udelay(1000);
        timeReg->ACK = 0x80;  // Trigger module to capture time
	do{
	udelay(1000);
	i++;
	}while((timeReg->ACK == 0) &&(i<20));
	if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
	printk("New Time COde Format = 0x%x\n",*dramRead);
// End Wait ****************************************
  //
  // Set write and wait *****************************
	*cmd = 0x1619; // Request model ID
	i=0;
        timeReg->ACK = 0x1;  // Trigger module to capture time
	udelay(1000);
        timeReg->ACK = 0x80;  // Trigger module to capture time
	do{
	udelay(1000);
	i++;
	}while((timeReg->ACK == 0) &&(i<20));
	if(timeReg->ACK) printk("SysCom ack received ID %d !!! 0x%x\n",timeReg->ACK,i);
	printk("New TC Modulation = 0x%x\n",*dramRead);
// End Wait ****************************************
	
  for (i = 0; i < 10; i++) {
    pHardware->gps[0] = 1;
    printk("Current time %ds %dus %dns \n", (pHardware->gps[0x34/4]-252806386), 0xfffff & pHardware->gps[0x30/4], 100 * ((pHardware->gps[0x30/4] >> 20) & 0xf));
  }
  pHardware->gps[0] = 1;
  time0 = pHardware->gps[0x30/4];
  if (time0 & (1<<24)) printk("Flywheeling, unlocked...\n");
  else printk ("Locked!\n");
  return(0);
}

// *****************************************************************************
// Initialize TSYNC GPS card (model BC635PCI-U)
// *****************************************************************************
int mapTsyncGps(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev)
{
  unsigned int i,ii;
  static unsigned int pci_io_addr;
  int pedStatus;
  unsigned int days,hours,min,sec,msec,usec,nanosec,tsync;
  unsigned char *addr1;
  TSYNC_REGISTER *myTime;

  pedStatus = pci_enable_device(gpsdev);
  pci_read_config_dword(gpsdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
  pci_io_addr &= 0xfffffff0;
  printk("TSYNC PIC BASE 0 address = %x\n", pci_io_addr);

  addr1 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x30);
  printk("Remapped 0x%p\n", addr1);
  pHardware->gps = addr1;
  pHardware->gpsType = TSYNC_RCVR;

  myTime = (TSYNC_REGISTER *)addr1;
for(ii=0;ii<2;ii++)
{
  i = myTime->SUPER_SEC_LOW;
  sec = (i&0xf) + ((i>>4)&0xf) * 10;
  min = ((i>>8)&0xf) + ((i>>12)&0xf)*10;
  hours = ((i>>16)&0xf) + ((i>>20)&0xf)*10;
  days = ((i>>24)&0xf) + ((i>>28)&0xf)*10;

  i = myTime->SUPER_SEC_HIGH;
  days += (i&0xf)*100;

  i = myTime->SUB_SEC;
  // nanosec = ((i & 0xffff)*5) + (i&0xfff0000);
  nanosec = ((i & 0xfffffff)*5);
  tsync = (i>>31) & 1;

  i = myTime->BCD_SEC;
  sec = i - 315964819;
  i = myTime->BCD_SUB_SEC;
  printk("date = %d days %2d:%2d:%2d\n",days,hours,min,sec);
  usec = (i&0xf) + ((i>>4)&0xf) *10 + ((i>>8)&0xf) * 100;
  msec = ((i>>16)&0xf) + ((i>>20)&0xf) *10 + ((i>>24)&0xf) * 100;
  printk("bcd time = %d sec  %d milliseconds %d microseconds  %d nanosec\n",sec,msec,usec,nanosec);
  printk("Board sync = %d\n",tsync);
}
  return(0);
}
