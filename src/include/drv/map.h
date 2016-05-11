#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

void set_8111_prefetch(struct pci_dev *);
int mapPciModules(CDS_HARDWARE *);/* Init routine to map adc/dac cards*/

// PCI Device variables
volatile PLX_9056_INTCTRL *plxIcr;              /* Ptr to interrupt cntrl reg on PLX chip */
dma_addr_t rfm_dma_handle[MAX_DAC_MODULES];     /* PCI add of RFM DMA memory */


#ifdef OVERSAMPLE
#ifdef SERVO2K
#define OVERSAMPLE_TIMES        32
#define FE_OVERSAMPLE_COEFF     feCoeff32x
#elif defined(SERVO4K)
#define OVERSAMPLE_TIMES        16
#define FE_OVERSAMPLE_COEFF     feCoeff16x
#elif defined(SERVO16K)
#define OVERSAMPLE_TIMES        4
#define FE_OVERSAMPLE_COEFF     feCoeff4x
#elif defined(SERVO32K)
#define OVERSAMPLE_TIMES        2
#define FE_OVERSAMPLE_COEFF     feCoeff2x
#elif defined(SERVO256K)
#define OVERSAMPLE_TIMES        1
#else
#error Unsupported system rate when in oversampling mode: only 2K, 16K and 32K are supported
#endif
#endif

#endif
