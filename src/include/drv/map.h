#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

void set_8111_prefetch( struct pci_dev* );
int  mapPciModules( CDS_HARDWARE* ); /* Init routine to map adc/dac cards*/

// PCI Device variables
volatile PLX_9056_INTCTRL* plxIcr; /* Ptr to interrupt cntrl reg on PLX chip */
dma_addr_t rfm_dma_handle[ MAX_DAC_MODULES ]; /* PCI add of RFM DMA memory */

#endif
