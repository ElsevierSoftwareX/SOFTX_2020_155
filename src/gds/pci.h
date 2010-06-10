/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: pci							*/
/*                                                         		*/
/* Module Description: PCI configuration utility functions		*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Engineer   Comments			   		*/
/* 0.1   10/30/02 AI        						*/
/*                                                         		*/
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*								 	*/
/*									*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1996.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/* LIGO Hanford Observatory				   		*/
/* P.O. Box 1970 S9-02					   		*/
/* Richland WA 99352					   		*/
/*                                                         		*/
/* LIGO Livingston Observatory		   				*/
/* 19100 LIGO Lane Rd.					   		*/
/* Livingston, LA 70754					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef _GDS_PCI_H
#define _GDS_PCI_H

#ifdef __cplusplus
extern "C" {
#endif


/** @name PCI Utility Functions
    This routines are used to find PCI boards and their configuration.

    @memo PCI Utility Functions
    @author Written June 2002 by Alex Ivanov
    @version 1.0
 ************************************************************************/

/*@{*/

/** This function reads the designated register of the <function> number 
    on the <device> instance on the specified bus number, and returns 
    the value (4 bytes).

    @param busNumber PCI bus number, 0 for local PCI bus
    @param device Device number on the bus
    @param function Function number on the device
    @param registerNumber Register number, i.e. long word index
    @return Long word from configuration data register
 ************************************************************************/
unsigned int sysReadPciConfig (unsigned char busNumber,
                               unsigned char device,
                               unsigned char function,
                               unsigned char registerNumber);

/** This function searches the local PCI bus for the specified <instance>
    of the device, and returns the device number if found.

    @param deviceId Vendor specified device identifier
    @param vendorId Vendor identifier, i.e. VMIC is 0x114A
    @param instance Which instance of the device to find
    @param function Function number on multifunction devices
    @param busNumber Bus that the device was found on
    @return Device number if device found, -1 otherwise
 ************************************************************************/
int sysFindPciDevice (unsigned short deviceId,
                      unsigned short vendorId,
                      unsigned short instance,
                      unsigned short function,
                      int* busNo);


/** This function asserts a VME bus reset on a pentium CPU.

    @return void
 ************************************************************************/
void vmeBusReset(void);

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_PCI_H */
