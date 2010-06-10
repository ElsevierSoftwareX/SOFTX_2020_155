/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: cobox							*/
/*                                                         		*/
/* Module Description: Driver for cobox (ethernet-to-RS232)		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 30Apr98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: cobox.html						*/
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

#ifndef _GDS_COBOX_H
#define _GDS_COBOX_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */


/* definitions */

/* main program */

/**
   @name Cobox API
   * The cobox API establishes a connection to an RS232 port on a cobox.
   Only one function is provided by the API to open a connection.
   This open function returns a handle which can be used by successive
   read and write function calls. To terminate a connection the close
   function should be used.

   Present limitations: on UNIX machines this routine will not return
   if no cobox can be reached under the specified network address.

   @memo Interface to a cobox (ethernet-to-RS232)
   @author Written April 1998 by Daniel Sigg
   @version 0.1
************************************************************************/

/*@{*/


/** Opens a socket and establishes an TCP/IP connection to a cobox.

    @param netaddr network address of the cobox
    @param serialport specifies which serial port to use (either 1 or 2)
    @return socket handle
    @author DS, April 98
************************************************************************/
   int openCobox (const char* netaddr, int serialport);


/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_COBOX_H */
