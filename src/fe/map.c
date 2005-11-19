#include <linux/types.h>
#include <linux/pci.h>
#ifdef USE_VMIC_RFM
#include <drv/vmic5579.h>
#include <drv/vmic5565.h>
#define RFM_WRITE	0x0
#define RFM_READ	0x8
#endif
#include <drv/gsaadc.h>		/* GSA ADC module defs */
#include <drv/gsadac.h>		/* GSA DAC module defs */
#include <drv/plx9056.h>	/* PCI interface chip for GSA products */
#include <drv/gmnet.h>
#include <drv/cdsHardware.h>


// PCI Device variables
volatile PLX_9056_DMA *adcDma[MAX_ADC_MODULES];	/* DMA struct for GSA ADC */
volatile PLX_9056_DMA *dacDma[MAX_DAC_MODULES];	/* DMA struct for GSA DAC */
volatile PLX_9056_INTCTRL *plxIcr;		/* Ptr to interrupt cntrl reg on PLX chip */
dma_addr_t adc_dma_handle[MAX_ADC_MODULES];	/* PCI add of ADC DMA memory */
dma_addr_t dac_dma_handle[MAX_DAC_MODULES];	/* PCI add of ADC DMA memory */
volatile GSA_ADC_REG *adcPtr[MAX_ADC_MODULES];	/* Ptr to ADC registers */
volatile GSA_DAC_REG *dacPtr[MAX_DAC_MODULES];	/* Ptr to DAC registers */

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


#ifdef USE_VMIC_RFM
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
#endif

// *****************************************************************************
// Function checks if DMA from ADC module is complete
// *****************************************************************************
int adcDmaDone()
{
	if(adcDma[0]->DMA_CSR & GSAI_DMA_DONE) return (1);
	else return(0);
}

// *****************************************************************************
// Function clears ADC buffer and starts acquisition via external clock
// *****************************************************************************
int gsaAdcTrigger(int adcCount)
{
int ii;
#if 0
  adcPtr->INTCR |= GSAI_ISR_ON_SAMPLE;
  plxIcr->INTCSR |= PLX_INT_ENABLE;
  adcPtr->RAG &= ~(GSAI_SAMPLE_START);
#endif
  for(ii=0;ii<adcCount;ii++)
  {
	  adcPtr[ii]->IDBC = GSAI_CLEAR_BUFFER;
	  adcPtr[ii]->BCR |= GSAI_ENABLE_X_SYNC;
  }
  return(0);
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
  adcPtr[0]->BCR &= ~(GSAI_ENABLE_X_SYNC);
  return(0);
}


// *****************************************************************************
// Test if ADC has a sample ready
// *****************************************************************************
int checkAdcRdy(int count)
{
  int dataCount;

    do {
        dataCount = adcPtr[0]->BUF_SIZE;
  }while(dataCount < count);
  return(1);

}

// *****************************************************************************
// DMA 32 samples from ADC module
// *****************************************************************************
int gsaAdcDma(int modNum, int byteCount)
{
  adcDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  adcDma[modNum]->DMA0_PCI_ADD = (int)adc_dma_handle[modNum];
  adcDma[modNum]->DMA0_LOC_ADD = GSAI_DMA_LOCAL_ADDR;
  adcDma[modNum]->DMA0_BTC = byteCount;
  adcDma[modNum]->DMA0_DESC = GSAI_DMA_TO_PCI;
  adcDma[modNum]->DMA_CSR = GSAI_DMA_START;
  return(1);
}

// *****************************************************************************
// DMA 16 samples to DAC module
// *****************************************************************************
int gsaDacDma(int modNum)
{
  dacDma[modNum]->DMA0_MODE = GSAI_DMA_MODE_NO_INTR;
  dacDma[modNum]->DMA0_PCI_ADD = (int)dac_dma_handle[modNum];
  dacDma[modNum]->DMA0_LOC_ADD = 0x18;
  dacDma[modNum]->DMA0_BTC = 0x40;
  dacDma[modNum]->DMA0_DESC = 0x0;
  dacDma[modNum]->DMA_CSR = GSAI_DMA_START;
  return(1);
}

#if 0
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
#endif
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
	  dacPtr[devNum]->BCR = (GSAO_OUT_RANGE_05 | GSAO_SIMULT_OUT);
	  printk("DAC BCR after init = 0x%x\n",dacPtr[devNum]->BCR);
	  printk("DAC CSR = 0x%x\n",dacPtr[devNum]->CSR);

	  dacPtr[devNum]->BOR = GSAO_FIFO_16;
	  // dacPtr->BOR |= (GSAO_ENABLE_CLK | GSAO_EXTERN_CLK);
	  dacPtr[devNum]->BOR |= (GSAO_ENABLE_CLK);
	  printk("DAC BOR = 0x%x\n",dacPtr[devNum]->BOR);
	  pHardware->pci_dac[devNum] = 
		pci_alloc_consistent(dacdev,0x200,&dac_dma_handle[devNum]);
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
  adcPtr[devNum]->SMUW = 0x0002;
  adcPtr[devNum]->SMLW = 0x0001;

  // Set ADC to 64 channel = 32 differential channels
  adcPtr[devNum]->SSC = (GSAI_64_CHANNEL);
  // adcPtr->SSC = GSAI_64_CHANNEL;
  printk("SSC = 0x%x\n",adcPtr[devNum]->SSC);
  adcPtr[devNum]->BCR |= (GSAI_FULL_DIFFERENTIAL);
  adcPtr[devNum]->BCR &= ~(GSAI_SET_2S_COMP);

  // Set sample rate close to 16384Hz
  adcPtr[devNum]->RAG = 0x10BEC;
  printk("RAG = 0x%x\n",adcPtr[devNum]->RAG);
  printk("BCR = 0x%x\n",adcPtr[devNum]->BCR);
  adcPtr[devNum]->RAG &= ~(GSAI_SAMPLE_START);
  adcPtr[devNum]->BCR |= GSAI_AUTO_CAL;
  do {
  }while((adcPtr[devNum]->BCR & GSAI_AUTO_CAL) != 0);
  adcPtr[devNum]->RAG |= GSAI_SAMPLE_START;
  adcPtr[devNum]->IDBC = GSAI_CLEAR_BUFFER;
  adcPtr[devNum]->SSC = (GSAI_64_CHANNEL | GSAI_EXTERNAL_SYNC);
  pHardware->pci_adc[devNum] = pci_alloc_consistent(adcdev,0x2000,&adc_dma_handle[devNum]);
  pHardware->adcCount ++;
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
  }
  return(modCount);
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
int myriNetInit()
{
  char receiver_nodename[64];

  // Initialize interface
  gm_init();

  /* Open a port on our local interface. */
  gm_strncpy (receiver_nodename,  /* Mandatory 1st parameter */
              "gwave-108",
              sizeof (receiver_nodename) - 1);

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

  gm_exit (main_status);

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
          printk ("[send] Receive Event (UNEXPECTED)\n");

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
  daqSendMessage->dcuId = dcuId;
  daqSendMessage->channelCount = 16;
  daqSendMessage->fileCrc = 0x3879d7b;
  daqSendMessage->dataBlockSize = GM_DAQ_XFER_BYTE;

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
  daqSendMessage->dcuId = dcuId;
  if(cycle > 0) 
    daqSendMessage->cycle = cycle - 1;
  if(cycle == 0) daqSendMessage->cycle = 15;
  daqSendMessage->offset = subCycle;
  daqSendMessage->fileCrc = fileCrc;
  daqSendMessage->blockCrc = blockCrc;
  daqSendMessage->dataCount = crcSize;
  daqSendMessage->tpCount = tpCount;
  for(kk=0;kk<20;kk++) daqSendMessage->tpNum[kk] = tpNum[kk];
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
