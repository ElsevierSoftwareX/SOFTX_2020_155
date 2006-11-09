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

#ifndef NO_DAQ
// Myrinet Variables
static void *netOutBuffer;			/* Buffer for outbound myrinet messages */
static void *netInBuffer;			/* Buffer for incoming myrinet messages */
static void *netDmaBuffer;			/* Buffer for outbound myrinet DMA messages */
gm_remote_ptr_t directed_send_addr;		/* Pointer to FB memory block */
gm_remote_ptr_t directed_send_subaddr[16];	/* Pointers to FB 1/16 sec memory blocks */
gm_recv_event_t *event;

int my_board_num = 0;                 		/* Default board_num is 0 */
gm_u32_t receiver_node_id;
gm_u32_t receiver_global_id;
gm_status_t main_status;
void *recv_buffer;
unsigned int size;
gm_s_e_id_message_t *id_message;
gm_u32_t recv_length;
static int expected_callbacks = 0;
static struct gm_port *netPort = 0;
daqMessage *daqSendMessage;
daqData *daqSendData;
int cdsNetStatus = 0;
#endif

// Prototypes
void gsaAdcDma2(int);


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
	  adcPtr[ii]->BCR &= ~(GSAI_DMA_DEMAND_MODE);
	  adcDma[ii]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR | 0x1000;
	  gsaAdcDma2(ii);
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
int checkAdcRdy(int count,int numAdc)
{
  int dataCount;
  int ii;

  // This is for the 2MS adc module
  {
          do {
                dataCount = fadcPtr[0]->IN_BUF_SIZE;
          }while(dataCount < (count / 8));
  }
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
// This routine sets up the DAC DMA registers once on code initialization.
// *****************************************************************************
int gsaDacDma1(int modNum, int dacType)
{
  if(dacType == GSC_16AISS8AO4)
  {
          dacDma[modNum]->DMA1_MODE = GSAI_DMA_MODE_NO_INTR;
          dacDma[modNum]->DMA1_PCI_ADD = (int)dac_dma_handle[modNum];
          dacDma[modNum]->DMA1_LOC_ADD = GSAF_DAC_DMA_LOCAL_ADDR;
          dacDma[modNum]->DMA1_BTC = 0x10;
          dacDma[modNum]->DMA1_DESC = 0x0;
  } else {
	  dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
	  dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
	  dacDma[modNum]->DMA0_LOC_ADD = 0x18;
	  dacDma[modNum]->DMA0_BTC = 0x40;
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
  if(dacType == GSC_16AISS8AO4)
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

	  dacPtr[devNum]->BOR = GSAO_FIFO_16;
	  dacPtr[devNum]->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
	  // dacPtr[devNum]->BOR |= (GSAO_ENABLE_CLK);
	  printk("DAC BOR = 0x%x\n",dacPtr[devNum]->BOR);
	  pHardware->pci_dac[devNum] = 
		pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
  	  pHardware->dacConfig[devNum] = dacPtr[devNum]->ASSC;
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
  adcPtr[devNum]->BCR &= ~(GSAI_SET_2S_COMP);

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
  adcPtr[devNum]->IDBC = (GSAI_CLEAR_BUFFER | GSAI_THRESHOLD);
  adcPtr[devNum]->SSC = (GSAI_64_CHANNEL | GSAI_EXTERNAL_SYNC);
  printk("SSC = 0x%x\n",adcPtr[devNum]->SSC);
  printk("IDBC = 0x%x\n",adcPtr[devNum]->IDBC);
  pHardware->pci_adc[devNum] = pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
  pHardware->adcType[devNum] = GSC_16AI64SSA;
  pHardware->adcConfig[devNum] = adcPtr[devNum]->ASSC;
  pHardware->adcCount ++;
  return(0);

}
// *****************************************************************************
// Routine to initialize GSA 2MS ADC modules
// *****************************************************************************
int mapFadc(CDS_HARDWARE *pHardware, struct pci_dev *adcdev)
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

  fadcPtr[devNum]->BCR &= ~(GSAF_SET_2S_COMP);
  fadcPtr[devNum]->RAG = GSAF_RATEA_32K;
  fadcPtr[devNum]->IN_CONF = 0x4f000400;
  fadcPtr[devNum]->RGENC = GSAF_RATEC_1MHZ;
  fadcPtr[devNum]->BCR |= GSAF_ENABLE_BUFF_OUT;
  fadcPtr[devNum]->BCR |= GSAF_DAC_SIMULT;
  fadcPtr[devNum]->DAC_BUFF_OPS |= (GSAF_DAC_CLK_INIT | GSAF_DAC_4CHAN);
  fadcPtr[devNum]->DAC_BUFF_OPS |= GSAF_DAC_ENABLE_CLK;
  fadcPtr[devNum]->DAC_BUFF_OPS |= GSAF_DAC_CLR_BUFFER;
  fadcPtr[devNum]->BCR |= GSAF_DAC_ENABLE_RGC;

  printk("RAG = 0x%x\n",fadcPtr[devNum]->RAG);
  printk("BCR = 0x%x\n",fadcPtr[devNum]->BCR);
  printk("BOO = 0x%x\n",fadcPtr[devNum]->DAC_BUFF_OPS);
  printk("INPUT CONFIG = 0x%x\n",fadcPtr[devNum]->IN_CONF);
  pHardware->adcConfig[devNum] = fadcPtr[devNum]->ASSC;
  pHardware->dacConfig[pHardware->dacCount] = fadcPtr[devNum]->ASSC;
  printk("ASSEMBLY CONFIG = 0x%x\n",pHardware->adcConfig[devNum]);

  pHardware->pci_adc[devNum] =
                pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
  pHardware->pci_dac[pHardware->dacCount] =
                pci_alloc_consistent(adcdev,0x2000,&dac_dma_handle[devNum]);
  pHardware->adcType[devNum] = GSC_16AISS8AO4;
  pHardware->dacType[pHardware->dacCount] = GSC_16AISS8AO4;
  pHardware->adcCount ++;
  pHardware->dacCount ++;
  return(0);
}


// *****************************************************************************
// Routine to find and map PCI adc/dac modules
// *****************************************************************************
int mapPciModules(CDS_HARDWARE *pCds)
{
  static struct pci_dev *dacdev;
  int status;
  int modCount = 0;

  dacdev = NULL;
  status = 0;

  // Search system for any module with PLX-9056 and PLX id
  while((dacdev = pci_find_device(PLX_VID, PLX_TID, dacdev))) {
	// if found, check if it is a DAC module
        if((dacdev->subsystem_device == DAC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
                printk("dac card on bus %d; device %d\n",
                        dacdev->bus->number,
                        dacdev->devfn);
                status = mapDac(pCds,dacdev);
		modCount ++;
        }
	// if found, check if it is an ADC module
	if((dacdev->subsystem_device == ADC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
	{
		printk("adc card on bus %d; device %d\n",
			dacdev->bus->number,
			dacdev->devfn);
		status = mapAdc(pCds,dacdev);
		modCount ++;
	}
        // if found, check if it is a Fast ADC module
        if((dacdev->subsystem_device == FADC_SS_ID) && (dacdev->subsystem_vendor == PLX_VID))
        {
                printk("fast adc card on bus %d; device %d\n",
                        dacdev->bus->number,
                        dacdev->devfn);
                status = mapFadc(pCds,dacdev);
                modCount ++;
        }
  }

  dacdev = NULL;
  status = 0;
  // Search system for Digital I/O modules
  while((dacdev = pci_find_device(ACC_VID, ACC_TID, dacdev))) {
		printk("dio card on bus %d; device %d\n",
			dacdev->bus->number,
			dacdev->devfn);
		status = mapDio(pCds,dacdev);
		modCount ++;
  }

  dacdev = NULL;
  status = 0;
  pCds->pci_rfm[0] = 0;
  // Search system for VMIC RFM modules
  while((dacdev = pci_find_device(VMIC_VID, VMIC_TID, dacdev))) {
		printk("RFM card on bus %d; device %d\n",
			dacdev->bus->number,
			dacdev->devfn);
		status = mapRfm(pCds,dacdev);
		modCount ++;
  }
  return(modCount);
}

// *****************************************************************************
// Routine to initialize VMIC RFM modules
// *****************************************************************************
int mapRfm(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev)
{
  static unsigned int pci_io_addr;
  int devNum;
  static char *csrAddr;
  static unsigned int csrAddress;

	devNum = pHardware->rfmCount;
  	pci_enable_device(rfmdev);

  	pci_read_config_dword(rfmdev, 
        		PCI_BASE_ADDRESS_3,
                 	&pci_io_addr);
  	pHardware->pci_rfm[devNum] = ioremap_nocache((unsigned long)pci_io_addr, 0x8000);

  	pci_read_config_dword(rfmdev, PCI_BASE_ADDRESS_2, &csrAddress);
  	printk("CSR address is 0x%lx\n",csrAddress);
  	csrAddr = ioremap_nocache((unsigned long)csrAddress, 0x40);

	p5565Csr = (VMIC5565_CSR *)csrAddr;
	p5565Csr->LCSR1 &= ~TURN_OFF_5565_FAIL;
	printk("Board id = 0x%x\n",p5565Csr->BID);
  	pHardware->rfmConfig[devNum] = p5565Csr->NID;

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

#ifndef NO_DAQ
// *****************************************************************
// Called by gm_unknown().
// Decriments expected_callbacks variable ((*(int *)the_context)--),
// which is a network health monitor for the FE software.
// *****************************************************************
static void
my_send_callback (struct gm_port *port, void *the_context,
                  gm_status_t the_status)
{         
  /* One pending callback has been received */
  (*(int *)the_context)--;
          
  switch (the_status)
    {                                
    case GM_SUCCESS:
      break;
        
    case GM_SEND_DROPPED:
      // cdsNetStatus = (int)the_status;
      // printk ("**** Send dropped!\n");
      break;
          
    default:
	cdsNetStatus = (int)the_status;
  	gm_drop_sends 	(netPort,
                         GM_DAQ_PRIORITY,
                         receiver_node_id,
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks);
      // gm_perror ("Send completed with error", the_status);
    }
}

int myriNetDrop()
{
gm_drop_sends   (netPort,
                         GM_DAQ_PRIORITY,
                         receiver_node_id,
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks);
return(0);
}


// *****************************************************************
// Initialize the myrinet connection to Framebuilder.
int myriNetInit(int fbId)
{
  char receiver_nodename[64];

  // Initialize interface
  gm_init();

#if 0
  /* Open a port on our local interface. */
  switch(fbId)
  {
	case GWAVE111:
		gm_strncpy (receiver_nodename, "fb", sizeof (receiver_nodename) - 1);
	  	break;
	case GWAVE83:
		gm_strncpy (receiver_nodename, "gwave-83", sizeof (receiver_nodename) - 1);
	  	break;
	case GWAVE122:
		gm_strncpy (receiver_nodename, "gwave-211", sizeof (receiver_nodename) - 1);
	  	break;
	default:
		return(-1);
		break;
  }
#endif
  gm_strncpy (receiver_nodename, "fb", sizeof (receiver_nodename) - 1);

  main_status = gm_open (&netPort, my_board_num,
                         GM_PORT_NUM_SEND,
                         "gm_simple_example_send",
                         (enum gm_api_version) GM_API_VERSION_1_1);
  if (main_status == GM_SUCCESS)
    {
      printk ("[send] Opened board %d port %d\n",
                 my_board_num, GM_PORT_NUM_SEND);
    }
  else
    {
      gm_perror ("[send] Couldn't open GM port", main_status);
    }

  /* Allocate DMAable message buffers. */

  netOutBuffer = gm_dma_calloc (netPort, GM_RCV_BUFFER_COUNT,
                              GM_RCV_BUFFER_LENGTH);
  if (netOutBuffer == 0)
    {
      printk ("[send] Couldn't allocate out_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
    }

  netInBuffer = gm_dma_calloc (netPort, GM_RCV_BUFFER_COUNT,
                             GM_RCV_BUFFER_LENGTH);
  if (netInBuffer == 0)
    {
      printk ("[send] Couldn't allocate netInBuffer\n");
      main_status = GM_OUT_OF_MEMORY;
    }

  netDmaBuffer = gm_dma_calloc (netPort,
                                        GM_16HZ_BUFFER_COUNT,
                                        GM_16HZ_SEND_BUFFER_LENGTH);
  if (netDmaBuffer == 0)
    {
      printk ("[send] Couldn't allocate directed_send_buffer\n");
      main_status = GM_OUT_OF_MEMORY;
    }

  /* Tell GM where our receive buffer is */

  gm_provide_receive_buffer (netPort, netInBuffer, GM_RCV_MESSAGE_SIZE,
                             GM_DAQ_PRIORITY);


  main_status = gm_host_name_to_node_id_ex
    (netPort, 10000000, receiver_nodename, &receiver_node_id);
  if (main_status == GM_SUCCESS)
    {
      printk ("[send] receiver node ID is %d\n", receiver_node_id);
    }
  else
    {
      printk ("[send] Conversion of nodename %s to node id failed\n",
                 receiver_nodename);
      gm_perror ("[send]", main_status);
    }

#if 0
  // Send message to FB requesting DMA memory location for DAQ data.
  status = myriNetReconnect();
  // Wait for return message from FB.
  do{
    status = myriNetCheckReconnect();
  }while(status != 0);
#endif

return(1);


}

// *****************************************************************
// Cleanup the myrinet connection when code is killed.
// *****************************************************************
int myriNetClose()
{
  main_status = GM_SUCCESS;

  gm_dma_free (netPort, netOutBuffer);
  gm_dma_free (netPort, netInBuffer);
  gm_dma_free (netPort, netDmaBuffer);
  gm_close (netPort);
  gm_finalize();

  //gm_exit (main_status);

  return(0);

}

// *****************************************************************
// Network health check.
// If expected_callbacks get too high, FE will know to take
// corrective action.
// *****************************************************************
int myriNetCheckCallback()
{
  if (expected_callbacks)
    {
      event = gm_receive (netPort);

      switch (GM_RECV_EVENT_TYPE(event))
        {
        case GM_RECV_EVENT:
        case GM_PEER_RECV_EVENT:
        case GM_FAST_PEER_RECV_EVENT:
          // printk ("[send] Receive Event (UNEXPECTED)\n");

          recv_buffer = gm_ntohp (event->recv.buffer);
          size = (unsigned int)gm_ntoh_u8 (event->recv.size);
          gm_provide_receive_buffer (netPort, recv_buffer, size,
                                     GM_DAQ_PRIORITY);
          main_status = GM_FAILURE; /* Unexpected incoming message */

        case GM_NO_RECV_EVENT:
          break;

        default:
          gm_unknown (netPort, event);  /* gm_unknown calls the callback */
	}
   }
  return(expected_callbacks);

}

// *****************************************************************
// Send a connection message to the Framebuilder requesting its
// DMA memory location for the writing of DAQ data.
// *****************************************************************
int myriNetReconnect(int dcuId)
{
  unsigned long send_length;

  daqSendMessage = (daqMessage *)netOutBuffer;
  sprintf (daqSendMessage->message, "STT");
  daqSendMessage->dcuId = htonl(dcuId);
  daqSendMessage->channelCount = htonl(16);
  daqSendMessage->fileCrc = htonl(0x3879d7b);
  daqSendMessage->dataBlockSize = htonl(GM_DAQ_XFER_BYTE);

  send_length = (unsigned long) sizeof(*daqSendMessage);
  gm_send_with_callback (netPort,
                         netOutBuffer,
                         GM_RCV_MESSAGE_SIZE,
                         send_length,
                         GM_DAQ_PRIORITY,
                         receiver_node_id,
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks);
  expected_callbacks ++;
  return(expected_callbacks);
}

// *****************************************************************
// Check if the connection request message has been received.
// Returns (0) if success, (-1) if fail.
// *****************************************************************
int myriNetCheckReconnect()
{

int eMessage;

  eMessage = -1;
  int ii;

      event = gm_receive (netPort);
      recv_length = gm_ntoh_u32 (event->recv.length);

      switch (GM_RECV_EVENT_TYPE(event))
        {
        case GM_RECV_EVENT:
        case GM_PEER_RECV_EVENT:
        case GM_FAST_PEER_RECV_EVENT:
          if (recv_length != sizeof (gm_s_e_id_message_t))
            {
              printk ("[send] *** ERROR: incoming message length %d "
                         "incorrect; should be %ld\n",
                         recv_length, sizeof (gm_s_e_id_message_t));
              main_status = GM_FAILURE; /* Unexpected incoming message */
            }


          id_message = gm_ntohp (event->recv.message);
          receiver_global_id = gm_ntoh_u32(id_message->global_id);
          directed_send_addr =
            gm_ntoh_u64(id_message->directed_recv_buffer_addr);
          for(ii=0;ii<16;ii++) directed_send_subaddr[ii] = directed_send_addr + GM_DAQ_XFER_BYTE * ii;
          main_status = gm_global_id_to_node_id(netPort,
                                                receiver_global_id,
                                                &receiver_node_id);
          if (main_status != GM_SUCCESS)
            {
              gm_perror ("[send] Couldn't convert global ID to node ID",
                         main_status);
            }

          eMessage = 0;

          /* Return the buffer for reuse */

          recv_buffer = gm_ntohp (event->recv.buffer);
          size = (unsigned int)gm_ntoh_u8 (event->recv.size);
          gm_provide_receive_buffer (netPort, recv_buffer, size,
                                     GM_DAQ_PRIORITY);
          break;

        case GM_NO_RECV_EVENT:
          break;

        default:
          gm_unknown (netPort, event);  /* gm_unknown calls the callback */
        }
  return(eMessage);
}


// *****************************************************************
// Send a 1/256 sec block of data from FE to DAQ Framebuilder.
// *****************************************************************
int myriNetDaqSend(	int dcuId, 
			int cycle, 
			int subCycle, 
			unsigned int fileCrc, 
			unsigned int blockCrc,
			int crcSize,
			int tpCount,
			int tpNum[],
			int xferSize,
			char *dataBuffer)
{

  unsigned int *daqDataBuffer;
  int send_length;
  int kk;
  int xferWords;

  // Load data into xmit buffer from daqLib buffer.
  daqSendData = (daqData *)netDmaBuffer;
  daqDataBuffer = (unsigned int *)dataBuffer;
  xferWords = xferSize / 4;
  daqDataBuffer += subCycle * xferWords;
  for(kk=0;kk<xferWords;kk++) 
  {
	daqSendData->data[kk] = *daqDataBuffer;
	daqDataBuffer ++;
  }
  directed_send_subaddr[0] = directed_send_addr + xferSize * subCycle;

  // Send data directly to designated memory space in Framebuilder.
  gm_directed_send_with_callback (netPort,
                                  netDmaBuffer,
                                  (gm_remote_ptr_t) (directed_send_subaddr[0] + GM_DAQ_BLOCK_SIZE * cycle),
                                  (unsigned long)
                                  xferSize,
                                  GM_DAQ_PRIORITY,
                                  receiver_node_id,
                                  GM_PORT_NUM_RECV,
                                  my_send_callback,
                                  &expected_callbacks);
  expected_callbacks++;


// Once every 1/16 second, send a message to signal FB that data is ready.
if(subCycle == 15) {
  sprintf (daqSendMessage->message, "DAT");
  daqSendMessage->dcuId = htonl(dcuId);
  if(cycle > 0)  {
    int mycycle = cycle -1;
    daqSendMessage->cycle = htonl(mycycle);
  }
  if(cycle == 0) daqSendMessage->cycle = htonl(15);
  daqSendMessage->offset = htonl(subCycle);
  daqSendMessage->fileCrc = htonl(fileCrc);
  daqSendMessage->blockCrc = htonl(blockCrc);
  daqSendMessage->dataCount = htonl(crcSize);
  daqSendMessage->tpCount = htonl(tpCount);
  for(kk=0;kk<20;kk++) daqSendMessage->tpNum[kk] = htonl(tpNum[kk]);
  send_length = (unsigned long) sizeof(*daqSendMessage) + 1;
  gm_send_with_callback (netPort,
                         netOutBuffer,
                         GM_RCV_MESSAGE_SIZE,
                         send_length,
                         GM_DAQ_PRIORITY,
                         receiver_node_id,
                         GM_PORT_NUM_RECV,
                         my_send_callback,
                         &expected_callbacks);
  expected_callbacks++;
}
  return(0);
}
#endif
