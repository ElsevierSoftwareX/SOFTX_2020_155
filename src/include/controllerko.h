#include <linux/version.h>
#include <linux/init.h>
#undef printf
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <asm/delay.h>
#include <asm/cacheflush.h>

#include <linux/slab.h>
/// Can't use printf in kernel module so redefine to use Linux printk function
#define printf printk
#include <drv/cdsHardware.h>
#include "inlineMath.h"

#include <asm/processor.h>
#include <asm/cacheflush.h>

// Code can be run without shutting down CPU by changing this compile flag
#ifndef NO_CPU_SHUTDOWN
// extern long ligo_get_gps_driver_offset(void);
#endif

#include "fm10Gen.h" // CDS filter module defs and C code
#include "feComms.h" // Lvea control RFM network defs.
#include "daqmap.h" // DAQ network layout
#include "cds_types.h"
#include "controller.h"

#ifndef NO_DAQ
#include "drv/fb.h"
#include "drv/daqLib.c" // DAQ/GDS connection software
#endif

#include "drv/epicsXfer.c" // User defined EPICS to/from FE data transfer function
#include "../fe/timing.c" // timing module / IRIG-B  functions

#include "drv/inputFilterModule.h"
#include "drv/inputFilterModule1.h"

// Contec 64 input bits plus 64 output bits (Standard for aLIGO)
/// Contec6464 input register values
unsigned int CDIO6464InputInput[ MAX_DIO_MODULES ]; // Binary input bits
/// Contec6464 - Last output request sent to module.
unsigned int CDIO6464LastOutState[ MAX_DIO_MODULES ]; // Current requested value
                                                      // of the BO bits
/// Contec6464 values to be written to the output register
unsigned int CDIO6464Output[ MAX_DIO_MODULES ]; // Binary output bits

// This Contect 16 input / 16 output DIO card is used to control timing receiver by
// IOP
/// Contec1616 input register values
unsigned int CDIO1616InputInput[ MAX_DIO_MODULES ]; // Binary input bits
/// Contec1616 output register values read back from the module
unsigned int CDIO1616Input[ MAX_DIO_MODULES ]; // Current value of the BO bits
/// Contec1616 values to be written to the output register
unsigned int CDIO1616Output[ MAX_DIO_MODULES ]; // Binary output bits
/// Holds ID number of Contec1616 DIO card(s) used for timing control.
int tdsControl[ 3 ]; // Up to 3 timing control modules allowed in case I/O
                     // chassis are daisy chained
/// Total number of timing control modules found on bus
int tdsCount = 0;
