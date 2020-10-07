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
#include <drv/gsc20ao8.c>
#include <drv/accesIIRO8.c>
#include <drv/accesIIRO16.c>
#include <drv/accesDio24.c>
#include <drv/contec6464.c>
#include <drv/contec1616.c>
#include <drv/contec32o.c>
#include <drv/vmic5565.c>
#include <drv/symmetricomGps.c>
#include <drv/spectracomGPS.c>
#include <drv/gsc18ai32.c>

// *****************************************************************************
/// \brief Patch to properly handle PEX PCIe chip for newer (PCIe) General
/// Standards
///< DAC modules ie those that are integrated PCIe boards vs. earlier versions
///< built with carrier boards. \n This is extracted from code provided by GSC..
// *****************************************************************************
void
set_8111_prefetch( struct pci_dev* dacdev )
{
    struct pci_dev* dev = dacdev->bus->self;

    printk( "set_8111_prefetch: subsys=0x%x; vendor=0x%x\n",
            dev->device,
            dev->vendor );
    if ( ( dev->device == 0x8111 ) && ( dev->vendor == PLX_VID ) )
    {
        unsigned int reg;
        // Handle PEX 8111 setup, enable prefetch, set pref size to 64
        // These numbers come from reverse engineering the GSC pxe8111 driver
        // and using their prefetch program to enable the prefetch and set pref
        // size to 64
        pci_write_config_dword( dev, 132, 72 );
        pci_read_config_dword( dev, 136, &reg );
        pci_write_config_dword( dev, 136, reg );
        pci_write_config_dword( dev, 132, 72 );
        pci_read_config_dword( dev, 136, &reg );
        pci_write_config_dword( dev, 136, reg | 1 );
        pci_write_config_dword( dev, 132, 12 );
        pci_read_config_dword( dev, 136, &reg );
        pci_write_config_dword( dev, 136, reg | 0x8000000 );
    }
}

// *****************************************************************************
/// Routine to find PCI modules and call the appropriate driver initialization
/// software.
// *****************************************************************************
int
mapPciModules( CDS_HARDWARE* pCds )
{
    static struct pci_dev* dacdev;
    int                    status;
    int                    i;
    int                    modCount = 0;
#ifndef CONTROL_MODEL
    int fast_adc_cnt = 0;
    int adc_cnt = 0;
#endif
    int dac_cnt = 0;
    int dac_18bit_cnt = 0;
    int dac_20bit_cnt = 0;
    int bo_cnt = 0;
    int use_it;

    dacdev = NULL;
    status = 0;

    // Search for ADC cards
    for ( i = 0; i < pCds->cards; i++ )
    {
        adc_cnt = 0;
        fast_adc_cnt = 0;
        dac_cnt = 0;
        dac_18bit_cnt = 0;
        dac_20bit_cnt = 0;
        // Search system for any module with PLX-9056 and PLX id
        while ( ( dacdev = pci_get_device( PLX_VID, PLX_TID, dacdev ) ) )
        {
            // Check if it is an ADC module
            if ( ( dacdev->subsystem_device == ADC_SS_ID ) &&
                 ( dacdev->subsystem_vendor == PLX_VID ) )
            {
                if ( pCds->cards_used[ i ].instance == adc_cnt &&
                     pCds->cards_used[ i ].type == GSC_16AI64SSA )
                {
                    status = gsc16ai64Init( pCds, dacdev );
                    modCount++;
                    printk( "adc card on bus %x; device %x status %d\n",
                            dacdev->bus->number,
                            PCI_SLOT( dacdev->devfn ),
                            status );
                }
                adc_cnt++;
            }
            // Check if it is a 1M ADC module
            if ( ( dacdev->subsystem_device == GSC_18AI32SSC1M ) &&
                 ( dacdev->subsystem_vendor == PLX_VID ) )
            {
                if ( pCds->cards_used[ i ].instance == fast_adc_cnt &&
                     pCds->cards_used[ i ].type == GSC_18AI32SSC1M )
                {
                    status = gsc18ai32Init( pCds, dacdev );
                    modCount++;
                    printk( "fast adc card on bus %x; device %x\n",
                            dacdev->bus->number,
                            PCI_SLOT( dacdev->devfn ) );
                }
                fast_adc_cnt++;
            }

            // Search for DAC16 cards
            // Search system for any module with PLX-9056 and PLX id
            // Check if it is a DAC16 module
            if ( ( dacdev->subsystem_device == DAC_SS_ID ) &&
                 ( dacdev->subsystem_vendor == PLX_VID ) )
            {
                if ( pCds->cards_used[ i ].instance == dac_cnt &&
                     pCds->cards_used[ i ].type == GSC_16AO16 )
                {
                    status = gsc16ao16Init( pCds, dacdev );
                    modCount++;
                    printk( "16 bit dac card on bus %x; device %x status %d\n",
                            dacdev->bus->number,
                            PCI_SLOT( dacdev->devfn ),
                            status );
                }
                dac_cnt++;
            }

            // Search system for any module with PLX-9056 and PLX id
            // Check if it is a DAC16 module
            if ( ( dacdev->subsystem_device == DAC_18BIT_SS_ID ) &&
                 ( dacdev->subsystem_vendor == PLX_VID ) )
            {
                if ( pCds->cards_used[ i ].instance == dac_18bit_cnt &&
                     pCds->cards_used[ i ].type == GSC_18AO8 )
                {
                    status = gsc18ao8Init( pCds, dacdev );
                    modCount++;
                    printk( "18-bit dac card on bus %x; device %x status %d\n",
                            dacdev->bus->number,
                            PCI_SLOT( dacdev->devfn ),
                            status );
                }
                dac_18bit_cnt++;
            }

            // Check if it is a DAC20 module
            if ( ( dacdev->subsystem_device == DAC_20BIT_SS_ID ) &&
                 ( dacdev->subsystem_vendor == PLX_VID ) )
            {
                if ( pCds->cards_used[ i ].instance == dac_20bit_cnt &&
                     pCds->cards_used[ i ].type == GSC_20AO8 )
                {
                    status = gsc20ao8Init( pCds, dacdev );
                    modCount++;
                    printk( "20-bit dac card on bus %x; device %x status %d\n",
                            dacdev->bus->number,
                            PCI_SLOT( dacdev->devfn ),
                            status );
                }
                dac_20bit_cnt++;
            }
        } // end of while
    } // end of pci_cards used

    dacdev = NULL;
    status = 0;
    bo_cnt = 0;
    // Search for ACCESS PCI-DIO  modules
    while ( ( dacdev = pci_get_device( ACC_VID, ACC_TID, dacdev ) ) )
    {
        use_it = 0;
        if ( pCds->cards )
        {
            use_it = 0;
            /* See if ought to use this one or not */
            for ( i = 0; i < pCds->cards; i++ )
            {
                if ( pCds->cards_used[ i ].type == ACS_24DIO &&
                     pCds->cards_used[ i ].instance == bo_cnt )
                {
                    use_it = 1;
                    break;
                }
            }
        }
        if ( use_it )
        {
            printk( "Access 24 BIO card on bus %x; device %x vendor 0x%x\n",
                    dacdev->bus->number,
                    PCI_SLOT( dacdev->devfn ),
                    dacdev->device );
            status = accesDio24Init( pCds, dacdev );
            modCount++;
        }
        bo_cnt++;
    }

    dacdev = NULL;
    status = 0;
    bo_cnt = 0;
    // Search for ACCESS PCI-IIRO-8 isolated I/O modules
    while ( ( dacdev = pci_get_device( ACC_VID, PCI_ANY_ID, dacdev ) ) )
    {
        if ( dacdev->device != ACC_IIRO_TID &&
             dacdev->device != ACC_IIRO_TID_OLD )
            continue;
        use_it = 0;
        if ( pCds->cards )
        {
            use_it = 0;
            /* See if ought to use this one or not */
            for ( i = 0; i < pCds->cards; i++ )
            {
                if ( pCds->cards_used[ i ].type == ACS_8DIO &&
                     pCds->cards_used[ i ].instance == bo_cnt )
                {
                    use_it = 1;
                    break;
                }
            }
        }
        if ( use_it )
        {
            printk( "Access 8 BIO card on bus %x; device %x vendor 0x%x\n",
                    dacdev->bus->number,
                    PCI_SLOT( dacdev->devfn ),
                    dacdev->device );
            status = accesIiro8Init( pCds, dacdev );
            modCount++;
        }
        bo_cnt++;
    }

    dacdev = NULL;
    status = 0;
    bo_cnt = 0;
    // Search for ACCESS PCI-IIRO-16 isolated I/O modules
    while ( ( dacdev = pci_get_device( ACC_VID, PCI_ANY_ID, dacdev ) ) )
    {
        if ( dacdev->device != ACC_IIRO_TID16 &&
             dacdev->device != ACC_IIRO_TID16_OLD )
            continue;
        use_it = 0;
        if ( pCds->cards )
        {
            use_it = 0;
            /* See if ought to use this one or not */
            for ( i = 0; i < pCds->cards; i++ )
            {
                if ( pCds->cards_used[ i ].type == ACS_16DIO &&
                     pCds->cards_used[ i ].instance == bo_cnt )
                {
                    use_it = 1;
                    break;
                }
            }
        }
        if ( use_it )
        {
            printk( "Access BIO-16 card on bus %x; device %x\n",
                    dacdev->bus->number,
                    PCI_SLOT( dacdev->devfn ) );
            status = accesIiro16Init( pCds, dacdev );
            modCount++;
        }
        bo_cnt++;
    }

    dacdev = NULL;
    status = 0;
    bo_cnt = 0;

    // Search for Contec C_DIO_6464L_PE isolated I/O modules
    while ( ( dacdev = pci_get_device( CONTEC_VID, C_DIO_6464L_PE, dacdev ) ) )
    {
        use_it = 0;
        if ( pCds->cards )
        {
            use_it = 0;
            /* See if ought to use this one or not */
            for ( i = 0; i < pCds->cards; i++ )
            {
                if ( pCds->cards_used[ i ].type == CON_6464DIO &&
                     ( pCds->cards_used[ i ].instance * 2 ) == bo_cnt )
                {
                    use_it = 1;
                    break;
                }
            }
        }
        if ( use_it )
        {
            printk( "Contec 6464 DIO card on bus %x; device %x\n",
                    dacdev->bus->number,
                    PCI_SLOT( dacdev->devfn ) );
            status = contec6464Init( pCds, dacdev );
            modCount++;
            modCount++;
        }
        bo_cnt++;
        bo_cnt++;
    }

    dacdev = NULL;
    status = 0;
    bo_cnt = 0;

    // Search for Contec C_DIO_1616L_PE isolated I/O modules
    while ( ( dacdev = pci_get_device( CONTEC_VID, C_DIO_1616L_PE, dacdev ) ) )
    {
        use_it = 0;
        if ( pCds->cards )
        {
            use_it = 0;
            /* See if ought to use this one or not */
            for ( i = 0; i < pCds->cards; i++ )
            {
                if ( pCds->cards_used[ i ].type == CON_1616DIO &&
                     pCds->cards_used[ i ].instance == bo_cnt )
                {
                    use_it = 1;
                    break;
                }
            }
        }
        if ( use_it )
        {
            printk( "Contec 1616 DIO card on bus %x; device %x\n",
                    dacdev->bus->number,
                    PCI_SLOT( dacdev->devfn ) );
            status = contec1616Init( pCds, dacdev );
            modCount++;
        }
        bo_cnt++;
    }

    dacdev = NULL;
    status = 0;
    bo_cnt = 0;

    // Search for Contec C_DO_32L_PE isolated I/O modules
    while ( ( dacdev = pci_get_device( CONTEC_VID, C_DO_32L_PE, dacdev ) ) )
    {
        use_it = 0;
        if ( pCds->cards )
        {
            use_it = 0;
            /* See if ought to use this one or not */
            for ( i = 0; i < pCds->cards; i++ )
            {
                if ( pCds->cards_used[ i ].type == CON_32DO &&
                     pCds->cards_used[ i ].instance == bo_cnt )
                {
                    use_it = 1;
                    break;
                }
            }
        }
        if ( use_it )
        {
            printk( "Contec BO card on bus %x; device %x\n",
                    dacdev->bus->number,
                    PCI_SLOT( dacdev->devfn ) );
            status = contec32OutInit( pCds, dacdev );
            modCount++;
        }
        bo_cnt++;
    }

    dacdev = NULL;
    status = 0;

    for ( i = 0; i < MAX_RFM_MODULES; i++ )
    {
        pCds->pci_rfm[ i ] = 0;
    }

    dacdev = NULL;
    status = 0;
    pCds->gps = 0;
    pCds->gpsType = 0;
    dacdev = NULL;
    status = 0;
    // Look for TSYNC GPS board
    if ( ( dacdev = pci_get_device( TSYNC_VID, TSYNC_TID, dacdev ) ) )
    {
        printk( "TSYNC GPS card on bus %x; device %x\n",
                dacdev->bus->number,
                PCI_SLOT( dacdev->devfn ) );
        status = spectracomGpsInit( pCds, dacdev );
    }

    return ( modCount );
}
