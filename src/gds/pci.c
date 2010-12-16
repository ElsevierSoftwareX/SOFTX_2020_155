static char *versionId = "Version $Id$" ;
#ifdef OS_VXWORKS
#include <vxWorks.h>            /* Generic VxWorks header file          */
#include <sysLib.h>             /* VxWorks system library               */
#else
typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
#endif
#include "dtt/pci.h"
#if defined(PROCESSOR_BAJA47)
#include <drv/pci/pci.h>
#endif

#define PCI_CONFIGURATION_ADDR    0x0CF8  /* Triton CONFADDR register */
#define PCI_CONFIGURATION_DATA    0x0CFC  /* Triton CONFDATA register */
#define PCI_DEV_ID_VNDR_ID        0x00

/******************************************************************************
*
* sysReadPciConfig - reads 4 bytes from PCI config space
*
* This function reads the designated register of the <function> number on the
* <device> instance on the specified bus number, and returns the value.
*
* RETURNS: long word from configuration data register
*
* SEE ALSO: sysWritePciConfig(), and sysFindPciDevice()
*/

UINT32 sysReadPciConfig
    (
    UINT8 busNumber,      /* PCI bus number, 0 for local PCI bus   */
    UINT8 device,         /* device number on the bus              */
    UINT8 function,       /* function number on the device         */
    UINT8 registerNumber  /* register number, i.e. long word index */
    )
    {
#ifndef OS_VXWORKS
    return 0;
#else
#if !defined(OS_VXWORKS) || defined(PROCESSOR_BAJA47)
    PCI_CONFIG_DESC *pci_config_desc;
#if 0
typedef struct /* PCI_CONFIG_DESC */
    {
    UINT16        vendorId;        /* PCI vendor ID */
    UINT16        deviceId;        /* PCI device ID */
    UINT8         revisionId;      /* PCI revision ID */
    UINT8         function;        /* PCI function */
    VOIDFUNCPTR * vector;          /* vector to feed sysPciIntConnect() */
    char *        pBaseAdrs[6];    /* local ptrs to device register files */
    char *        romBaseAdrs;     /* local ptr to ROM image */
    char *        pciMemRamBaseAdrs;
                                   /* pci memory space ptr to base of RAM */
    } PCI_CONFIG_DESC;
#endif
    /* This is fudged here to work on Baja and find 5579 RFM board's base */
    pci_config_desc = sysPciConfigGet (0x114a, 0x5579, 0);
    if (pci_config_desc == 0) return 0;
    return (UINT32) (pci_config_desc->pBaseAdrs[1]);
#else
    UINT32 pattern = 0x80000000;  /* set the config enable bit */

    pattern |= (((UINT32)busNumber & 0x0FF) << 16);
    pattern |= (((UINT32)device & 0x1F) << 11);
    pattern |= (((UINT32)function & 0x07) << 8);
    pattern |= (((UINT32)registerNumber & 0x3F) << 2);

    sysOutLong(PCI_CONFIGURATION_ADDR, (long)pattern);
    return sysInLong(PCI_CONFIGURATION_DATA);
#endif
#endif
    }

/******************************************************************************
*
* sysFindPciDevice - find a PCI device
*
* This function searches the local PCI bus for the specified <instance> of the
* device, and returns the device number if found.
*
* RETURNS: device number if device found, -1 otherwise
*
* SEE ALSO: sysReadPciConfig(), and sysWritePciConfig()
*/

INT32 sysFindPciDevice
    (
    UINT16 deviceId,  /* vendor specified device identifier       */
    UINT16 vendorId,  /* vendor identifier, i.e. VMIC is 0x114A   */
    UINT16 instance,  /* which instance of the device to find     */
    UINT16 function,  /* function number on multifunction devices */
    int *busNo       /* bus that the device was found on */
    )
    {
#ifndef OS_VXWORKS
    return 0;
#else
#if !defined(OS_VXWORKS) || defined(PROCESSOR_BAJA47)
    int dev_cnt;
    sysPciInit();
    dev_cnt = sysPciDevCount(vendorId, deviceId);
    if (dev_cnt < 1) return -1; else return 0;
#else
    INT32 i, x;
    UINT32 searchPattern;
    INT32 instCtr;
    INT32 device = -1;

    instCtr = instance;

    searchPattern = deviceId;
    searchPattern <<= 16;
    searchPattern |= vendorId;

    for(x=0; x<0xff; x++)
        for (i=0; i<=20;i++)
            {
            if ((sysReadPciConfig(x, i, function, PCI_DEV_ID_VNDR_ID) == searchPattern) && !(instCtr--))
                {
                device = i;
                *busNo=x;
                break;
                }
            }
    return device;
#endif
#endif
    }

static UINT32 findU232Chip
   (
   UINT16 instance
   )
{
#ifndef OS_VXWORKS
    return 0;
#else
#if !defined(OS_VXWORKS) || defined(PROCESSOR_BAJA47)
    return (UINT32)-1;
#else
INT32 busId;
int status;
UINT32 pmcAdd;

   status = sysFindPciDevice(0x0,0x10E3,instance,0,&busId);
   pmcAdd = sysReadPciConfig(busId,status,0,0x4);
   return(pmcAdd);
#endif
#endif
}

/* Write bit 22 into the U232 chip to initiate VME BUS RESET */
void vmeBusReset(void) 
{ 
#if defined(OS_VXWORKS) && !defined(PROCESSOR_BAJA47)
    *(int *)(findU232Chip(0) + 0x404) = 0x400000; 
#endif
}

