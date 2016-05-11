///	\file map.c
///	\brief This file contains the software to find PCIe devices on the bus.

#include <linux/types.h>
#include <linux/kernel.h>
#undef printf
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#define printf printk
#include <drv/cdsHardware.h>
#include <drv/map.h>
#include <commData3.h>

// Include driver code for all supported I/O cards
#include <drv/gsc16ai64.c>
#include <drv/gsc16ao16.c>
#include <drv/gsc18ao8.c>
#include <drv/accesIIRO8.c>
#include <drv/accesIIRO16.c>
#include <drv/accesDio24.c>
#include <drv/contec6464.c>
#include <drv/contec1616.c>
#include <drv/contec32o.c>
#include <drv/vmic5565.c>
#include <drv/symmetricomGps.c>
#include <drv/spectracomGPS.c>
#include <drv/gsc18ai6.c>



// *****************************************************************************
/// \brief Patch to properly handle PEX PCIe chip for newer (PCIe) General Standards
///< DAC modules ie those that are integrated PCIe boards vs. earlier versions
///< built with carrier boards. \n This is extracted from code provided by GSC..
// *****************************************************************************
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
/// Routine to find PCI modules and call the appropriate driver initialization software.
// *****************************************************************************
int mapPciModules(CDS_HARDWARE *pCds)
{
  static struct pci_dev *dacdev;
  int status;
  int i;
  int modCount = 0;
//  int fast_adc_cnt = 0;
#ifndef ADC_SLAVE
  int adc_cnt = 0;
#endif
  int dac_cnt = 0;
  int dac_18bit_cnt = 0;
  int bo_cnt = 0;
  int use_it;

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
                  status = gsc18ao8Init(pCds,dacdev);
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
                  status = gsc16ao16Init(pCds,dacdev);
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
		  status = gsc16ai64Init(pCds,dacdev);
		  modCount ++;
		}
		adc_cnt++;
	}
#endif
        // if found, check if it is a Fast ADC module
    	// TODO: for the time of testing of the 18-bit board, it returned same PCI device number as the 16-bit fast GS board
	// This number will most likely change in the future.
	#if 0
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
	#endif
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
		  status = accesDio24Init(pCds,dacdev);
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
		  status = accesIiro8Init(pCds,dacdev);
		  modCount ++;
		}
		bo_cnt ++;
  }
		
  dacdev = NULL;
  status = 0;
  bo_cnt = 0;
  // Search for ACCESS PCI-IIRO-16 isolated I/O modules
  while((dacdev = pci_get_device(ACC_VID, PCI_ANY_ID, dacdev))) {
		if (dacdev->device != ACC_IIRO_TID16 && dacdev->device != ACC_IIRO_TID16_OLD)
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
		  status = accesIiro16Init(pCds,dacdev);
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
		  status = contec6464Init(pCds,dacdev);
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
		  status = contec1616Init(pCds,dacdev);
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
		  status = contec32OutInit(pCds,dacdev);
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
		status = vmic5565Init(pCds,dacdev);
		modCount ++;
  }

  dacdev = NULL;
  status = 0;
  pCds->gps = 0;
  pCds->gpsType = 0;
  // Look for Symmetricom GPS board
#if 0
  if ((dacdev = pci_get_device(SYMCOM_VID, SYMCOM_BC635_TID, dacdev))) {
            	printk("Symmetricom GPS card on bus %x; device %x\n",
                   	dacdev->bus->number,
		   	PCI_SLOT(dacdev->devfn));
		status = symmetricomGpsInit(pCds,dacdev);
		if (status == 0) {
		  // GPS board initialized and mapped
		  modCount ++;
		}
  }
#endif
  dacdev = NULL;
  status = 0;
  // Look for TSYNC GPS board
  if ((dacdev = pci_get_device(TSYNC_VID, TSYNC_TID, dacdev))) {
            	printk("TSYNC GPS card on bus %x; device %x\n",
                   	dacdev->bus->number,
		   	PCI_SLOT(dacdev->devfn));
		status = spectracomGpsInit(pCds,dacdev);
		if (status == 0) {
		  // GPS board initialized and mapped
		  modCount ++;
		}
  }

  return(modCount);
}
