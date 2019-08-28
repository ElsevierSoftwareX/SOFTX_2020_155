///     \file spectracomGPS.c
///     \brief File contains the initialization routine and various register
///     read/write
///<            operations for the TSync-PCIe IRIG-B receiver module. \n
///< For board info, see
///<    <a
///<    href="http://www.spectracomcorp.com/ProductsServices/TimingSynchronization/BuslevelTiming/PCIexpressslotcards/tabid/1296/Default.aspx">Spectracom
///<    TSync-PCIe Manual</a>

#include "spectracomGPS.h"

// *****************************************************************************
/// \brief Initialize TSYNC GPS card (model BC635PCI-U)
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///	@param *gpsdev PCI address information passed by the mapping code in
///map.c
// *****************************************************************************
static int spectracomGpsInitCheckSync(CDS_HARDWARE *pHardware,
                                      struct pci_dev *gpsdev, int *need_sync) {
  unsigned int i, ii;
  static unsigned int
      pci_io_addr; /// @param pci_io_addr Bus address of PCI card I/O register.
  int pedStatus;
  unsigned int days, hours, min, sec, msec, usec, nanosec, tsync;
  unsigned char *addr1;
  TSYNC_REGISTER *myTime;
  void *TSYNC_FIFO; /// @param *TSYNC_FIFO Pointer to board uP FIFO
  int sync_dummy = 0;

  if (!need_sync)
    need_sync = &sync_dummy;

  pedStatus = pci_enable_device(gpsdev);
  pci_read_config_dword(gpsdev, PCI_BASE_ADDRESS_0, &pci_io_addr);
  pci_io_addr &= 0xfffffff0;
  printk("TSYNC PIC BASE 0 address = %x\n", pci_io_addr);

  addr1 = (unsigned char *)ioremap_nocache((unsigned long)pci_io_addr, 0x30);
  printk("Remapped 0x%p\n", addr1);
  pHardware->gps = (unsigned int *)addr1;
  pHardware->gpsType = TSYNC_RCVR;

  // Spectracom IRIG-B Card does not auto detect Year information from IRIG-B
  // fanout unit This section writes to the module uP CodeExp register to
  // correct this. Delays are required between code lines, as writing to FIFO is
  // slow. Sequence was determined by looking at manufacturer driver code.
  TSYNC_FIFO = (void *)(addr1 + 384);
  iowrite16(0x101, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x1000, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x72a, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x280, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x0, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x800, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x0, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x0, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x0, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0x400, TSYNC_FIFO);
  udelay(1000);
  iowrite16(0xd100, TSYNC_FIFO);
  udelay(10000);
  udelay(10000);
  udelay(10000);
  // End Code exp setup
  // Need following delay to allow module to change time codes
  for (ii = 0; ii < 500; ii++)
    udelay(10000);

  myTime = (TSYNC_REGISTER *)addr1;
  for (ii = 0; ii < 2; ii++) {
    udelay(10000);
    i = myTime->SUPER_SEC_LOW;
    sec = (i & 0xf) + ((i >> 4) & 0xf) * 10;
    min = ((i >> 8) & 0xf) + ((i >> 12) & 0xf) * 10;
    hours = ((i >> 16) & 0xf) + ((i >> 20) & 0xf) * 10;
    days = ((i >> 24) & 0xf) + ((i >> 28) & 0xf) * 10;

    i = myTime->SUPER_SEC_HIGH;
    days += (i & 0xf) * 100;

    i = myTime->SUB_SEC;
    // nanosec = ((i & 0xffff)*5) + (i&0xfff0000);
    nanosec = ((i & 0xfffffff) * 5);
    tsync = (i >> 31) & 1;

    i = myTime->BCD_SEC;
    if (i < 1000000000) {
      printk("TSYNC NOT receiving YEAR info, defaulting to by year patch\n");
      *need_sync = 1;
      /* Historically we would hardwire a offset here.
       * With RCG 3.4 we have moved to configuring this in
       * the symmetricom/gpstime driver.
       *
       * This file is built 2 ways
       * 1. as part of the symmetricom/gpstime driver, in which case
       * it sets the gpsOffset to 0 (and allows the user
       * to configure it).
       * 2. as part of an IOP, in which it gets the offset from
       * the symmetricom/gpstime driver.
       *
       * We are leaving the offsets in as comments so that we have a
       * history/example of what was done
       */
      /* add offsets for leap-seconds - +15 through 2008 */
      /* add offset at end of 2011 (31536000 normal year) */
      /* Add offset for June 30, 2012 leap second */
      /* add offset at end of 2012 (31622400 leap year) */
      /* add offset at end of 2013 (31536000 normal year) */
      /*pHardware->gpsOffset = 31190400 + 15 + 31536000 + 1 + 31622400 +
       * 31536000;*/
      /* add offset at end of 2014 (31536000 normal year) */
      /*pHardware->gpsOffset = pHardware->gpsOffset + 31536000;*/
      /* 2015 had 365 days plus a July 1, 2015 leap second */
      /*pHardware->gpsOffset += 31536000 + 1;*/
      /* 2016 had 366 days plus a Dec 31, 2016/Jan 1, 2017 leap second */
      /*pHardware->gpsOffset += 31622400 + 1;*/
#ifndef IN_LIGO_GPS_KERNEL_DRIVER
      extern long ligo_get_gps_driver_offset(void);
      pHardware->gpsOffset = ligo_get_gps_driver_offset();
#else
      pHardware->gpsOffset = 0;
#endif
    } else {
      *need_sync = 0;
      printk("TSYNC receiving YEAR info\n");
      pHardware->gpsOffset = -315964800;
    }
    sec = i + pHardware->gpsOffset;
    i = myTime->BCD_SUB_SEC;
    printk("date = %d days %2d:%2d:%2d\n", days, hours, min, sec);
    usec = (i & 0xf) + ((i >> 4) & 0xf) * 10 + ((i >> 8) & 0xf) * 100;
    msec = ((i >> 16) & 0xf) + ((i >> 20) & 0xf) * 10 + ((i >> 24) & 0xf) * 100;
    printk("bcd time = %d sec  %d milliseconds %d microseconds  %d nanosec\n",
           sec, msec, usec, nanosec);
    printk("Board sync = %d\n", tsync);
  }
  return (0);
}

// *****************************************************************************
/// \brief Initialize TSYNC GPS card (model BC635PCI-U)
///     @param[in,out] *pHardware Pointer to global data structure for storing
///     I/O
///<            register mapping information.
///	@param *gpsdev PCI address information passed by the mapping code in
///map.c
// *****************************************************************************
int spectracomGpsInit(CDS_HARDWARE *pHardware, struct pci_dev *gpsdev) {
  int need_sync_dummy = 0;
  return spectracomGpsInitCheckSync(pHardware, gpsdev, &need_sync_dummy);
}

//***********************************************************************
/// \brief  Get current GPS time from TSYNC IRIG-B Rcvr
/// @param[out] *tsyncSec Pointer to register with seconds information.
/// @param[out] *tsyncUsec Pointer to register with microsecond information.
/// @return GPS Sync bit (0=No sync 1=Sync)
//***********************************************************************
inline int getGpsTimeTsync(unsigned int *tsyncSec, unsigned int *tsyncUsec) {
  TSYNC_REGISTER *timeRead;
  unsigned int timeSec, timeNsec, sync;

  if (cdsPciModules.gps) {
    timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
    timeSec = timeRead->BCD_SEC;
    timeSec += cdsPciModules.gpsOffset;
    *tsyncSec = timeSec;
    timeNsec = timeRead->SUB_SEC;
    *tsyncUsec = ((timeNsec & 0xfffffff) * 5) / 1000;
    sync = ((timeNsec >> 31) & 0x1) + 1;
    return (sync);
  }
  return (0);
}

//***********************************************************************
/// \brief Get current GPS seconds from TSYNC IRIG-B Rcvr
//***********************************************************************
inline unsigned int getGpsSecTsync(void) {
  TSYNC_REGISTER *timeRead;
  unsigned int timeSec;

  if (cdsPciModules.gps) {
    timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
    timeSec = timeRead->BCD_SEC;
    timeSec += cdsPciModules.gpsOffset;
    return (timeSec);
  }
  return (0);
}

//***********************************************************************
/// \brief Get current GPS useconds from TSYNC IRIG-B Rcvr
/// @param[out] *tsyncUsec Pointer to register with microsecond information.
/// @return GPS Sync bit (0=No sync 1=Sync)
//***********************************************************************
inline int getGpsuSecTsync(unsigned int *tsyncUsec) {
  TSYNC_REGISTER *timeRead;
  unsigned int timeNsec, sync;

  if (cdsPciModules.gps) {
    timeRead = (TSYNC_REGISTER *)cdsPciModules.gps;
    timeNsec = timeRead->SUB_SEC;
    *tsyncUsec = ((timeNsec & 0xfffffff) * 5) / 1000;
    sync = ((timeNsec >> 31) & 0x1) + 1;
    return (sync);
  }
  return (0);
}
