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
#include <drv/gsc18ai6.c>

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
    int                    i,ii;
    int                    ret;
    int                    modCount = 0;
//  int fast_adc_cnt = 0;
#ifndef CONTROL_MODEL
    int adc_cnt = 0;
#endif
    int            dac_cnt = 0;
    int            dac_18bit_cnt = 0;
    int            dac_20bit_cnt = 0;
    int            bo_cnt = 0;
    int            use_it;
    char           fname[ 128 ];
    unsigned char* _device_shm;
    int*           data;

    dacdev = NULL;
    status = 0;

    for ( i = 0; i < pCds->cards; i++ )
    {
        if ( pCds->cards_used[ i ].type == GSC_18AO8 )
        {
            sprintf( fname, "%s_%d\n", "IO_DEV_", i );
            ret = mbuf_allocate_area( fname, 8 * 4 * 65536, 0 );
            if ( ret < 0 )
            {
                printf( "mbuf_allocate_area() failed; ret = %d\n", ret );
                return -1;
            }
            _device_shm = (unsigned char*)( kmalloc_area[ ret ] );
            pCds->pci_dac[ dac_cnt ] = (long)_device_shm;
            pCds->dacType[ dac_cnt ] = GSC_18AO8;
            pCds->dacCount++;
            dac_cnt++;
            dac_18bit_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_16AO16 )
        {
            sprintf( fname, "%s_%d\n", "IO_DEV_", i );
            ret = mbuf_allocate_area( fname, 16 * 4 * 65536, 0 );
            if ( ret < 0 )
            {
                printf( "mbuf_allocate_area() failed; ret = %d\n", ret );
                return -1;
            }
            _device_shm = (unsigned char*)( kmalloc_area[ ret ] );
            pCds->pci_dac[ dac_cnt ] = (long)_device_shm;
            pCds->dacType[ dac_cnt ] = GSC_16AO16;
            pCds->dacCount++;
            dac_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_20AO8 )
        {
            sprintf( fname, "%s_%d\n", "IO_DEV_", i );
            ret = mbuf_allocate_area( fname, 8 * 4 * 65536, 0 );
            if ( ret < 0 )
            {
                printf( "mbuf_allocate_area() failed; ret = %d\n", ret );
                return -1;
            }
            _device_shm = (unsigned char*)( kmalloc_area[ ret ] );
            pCds->pci_dac[ dac_cnt ] = (long)_device_shm;
            pCds->dacType[ dac_cnt ] = GSC_20AO8;
            pCds->dacCount++;
            dac_cnt++;
            dac_20bit_cnt++;
        }
        if ( pCds->cards_used[ i ].type == GSC_16AI64SSA )
        {
            sprintf( fname, "%s_%d\n", "IO_DEV_", i );
            ret = mbuf_allocate_area( fname, 32 * 4 * 128, 0 );
            if ( ret < 0 )
            {
                printf( "mbuf_allocate_area() failed; ret = %d\n", ret );
                return -1;
            }
            _device_shm = (unsigned char*)( kmalloc_area[ ret ] );
            pCds->pci_adc[ adc_cnt ] = (long)_device_shm;
            pCds->adcType[ adc_cnt ] = GSC_16AI64SSA;
            pCds->adcChannels[ adc_cnt ] = 32;
            pCds->adcCount++;
            data = (int *) pCds->pci_adc[ adc_cnt ];
            for(ii=0;ii<64;ii++) {
                *data = ii;
                data ++;
            }
            adc_cnt++;
        }
        modCount++;
    }
    return ( modCount );
}
