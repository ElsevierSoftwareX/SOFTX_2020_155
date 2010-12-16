/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ntp							*/
/*                                                         		*/
/* Module Description: 	This routines are used to interface  	  	*/
/*			a NTP server					*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 1.0	 12Jan04  D. Sigg    	Initial release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	References: none						*/
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

#ifndef _GDS_NTP_H
#define _GDS_NTP_H

#ifdef __cplusplus
extern "C" { 
#endif


/* Header File List: */

/** @name NTP API
    This routines are used to interface a NTP server.

    @memo Interface to the NTP
    @author Written January 2004 by Daniel Sigg
************************************************************************/

/*@{*/


/** Synchronizes the internal clock. 

    @param hostname NTP server address 
    @return 0 if successful, <0 otherwise
************************************************************************/
   int sntp_sync (const char* hostname);

/** Synchronizes the internal clock and returns the year information. 

    @param ID board ID 
    @param hostname NTP server address 
    @param year year information (return)
    @return 0 if successful, <0 otherwise
************************************************************************/
   int sntp_sync_year (const char* hostname, int* year);


/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_NTP_H */
