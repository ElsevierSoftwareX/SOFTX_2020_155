#include "gsc18ai6.h"

volatile GSA_FAD_REG *fadcPtr[MAX_ADC_MODULES];
// *****************************************************************************
/// Routine to initialize GSA PMC66_18AISS6C ADC modules
// *****************************************************************************
int mapFadc(CDS_HARDWARE *pHardware, struct pci_dev *adcdev) {
  static unsigned int pci_io_addr;
  int devNum;
  char *_adc_add; /* ADC register address space */
  int pedStatus;

  devNum = pHardware->adcCount;
  pedStatus = pci_enable_device(adcdev);
  pci_set_master(adcdev);
  // Get the PLX chip address
  pci_read_config_dword(adcdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
  printk("pci0 = 0x%x\n", pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  adcDma[devNum] = (PLX_9056_DMA *)_adc_add;
  dacDma[pHardware->dacCount] = (PLX_9056_DMA *)_adc_add;
  if (devNum == 0)
    plxIcr = (PLX_9056_INTCTRL *)_adc_add;

  // Get the ADC register address
  pci_read_config_dword(adcdev, PCI_BASE_ADDRESS_2, &pci_io_addr);
  printk("pci2 = 0x%x\n", pci_io_addr);
  _adc_add = ioremap_nocache((unsigned long)pci_io_addr, 0x200);
  printk("ADC I/O address=0x%x  0x%lx\n", pci_io_addr, (long)_adc_add);
  fadcPtr[devNum] = (GSA_FAD_REG *)_adc_add;

  printk("BCR = 0x%x\n", fadcPtr[devNum]->BCR);
  printk("INPUT CONFIG = 0x%x\n", fadcPtr[devNum]->IN_CONF);

  // Reset the ADC board
  fadcPtr[devNum]->BCR |= GSAF_RESET;
  do {
  } while ((fadcPtr[devNum]->BCR & GSAF_RESET) != 0);
  printk("after reset BCR = 0x%x\n", fadcPtr[devNum]->BCR);
  printk("after reset INPUT CONFIG = 0x%x\n", fadcPtr[devNum]->IN_CONF);

  fadcPtr[devNum]->IN_CONF |= 1 << 9;
  fadcPtr[devNum]->BCR |= 1 << 12; // enable buffer
  fadcPtr[devNum]->BCR |= 1 << 13;
  printk("final BCR = 0x%x\n", fadcPtr[devNum]->BCR);
  printk("final INPUT CONFIG = 0x%x\n", fadcPtr[devNum]->IN_CONF);
  pHardware->adcConfig[devNum] = fadcPtr[devNum]->ASSC;
  printk("final ASSEMBLY CONFIG = 0x%x\n", pHardware->adcConfig[devNum]);

  pHardware->pci_adc[devNum] =
      (long)pci_alloc_consistent(adcdev, 0x2000, &adc_dma_handle[devNum]);
  pHardware->adcType[devNum] = GSC_18AISS6C;
  pHardware->adcCount++;
  return (0);
}

// *****************************************************************************
/// Test if ADC has a sample ready.
/// Only to be used with 2MS/sec ADC module.
// *****************************************************************************
int checkAdcRdy(int count, int numAdc, int type) {
  int dataCount;

  int i = 0;
  switch (type) {
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
    } while (dataCount < (count / 8));
    break;
  }
  // If timeout, return error
  if (i >= 1000000)
    return -1;
  // gsaAdcDma2(numAdc);
  return (dataCount);
}
