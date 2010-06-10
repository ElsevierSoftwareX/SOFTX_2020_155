/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: epics							*/
/*                                                         		*/
/* Module Description: gds driver for EPICS channels			*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   10Oct99 DS        						*/
/*                                                         		*/
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 5.0				*/
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
/* LIGO Project MS NW17-137				   		*/
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

#ifndef _GDS_EPICS_H
#define _GDS_EPICS_H

#ifdef __cplusplus
extern "C" {
#endif


/** @name EPICS API
    This routines are used to access epics channels.

    @memo Interface to EPICS
    @author Written October 1999 by Daniel Sigg
    @see EZCA
    @version 0.1
************************************************************************/

/*@{*/

/** Gets the value of an EPICS channel.

    @param chnname channel name
    @param value value of channel (return)
    @return 0 if successful, <0 otherwise
    @author DS, Oct 99
************************************************************************/
   int epicsGet (const char* chnname, double* value);

/** Sets the value of an epics channel.

    @param chnname channel name
    @param value value of channel (return)
    @return 0 if successful, <0 otherwise
    @author DS, Oct 99
************************************************************************/
   int epicsPut (const char* chnname, double value);


/** Sets the timeout and retry limit for channel access.

    @param timeout timeout of channel access (sec)
    @param retry number of retry attemps
    @return 0 if successful, <0 otherwise
    @author DS, Oct 99
************************************************************************/
   int epicsTimeout (double timeout, int retry);

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_EPICS_H */
