/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: confinfo						*/
/*                                                         		*/
/* Module Description: Configuration information API			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 29July99 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: confinfo.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
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
/* Caltech				MIT		   		*/
/* LIGO Project MS 51-33		LIGO Project NW-17 161		*/
/* Pasadena CA 91125			Cambridge MA 01239 		*/
/*                                                         		*/
/* LIGO Hanford Observatory		LIGO Livingston Observatory	*/
/* P.O. Box 159				19100 LIGO Lane Rd.		*/
/* Richland WA 99352			Livingston, LA 70754		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef _GDS_CONFINFO_H
#define _GDS_CONFINFO_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include "dtt/conftype.h"

/** @name Configuration Information API
    This API provides information about the configuration of the
    diagnostics test tools.

    @memo Configuration Information API
    @author Written July 1999 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

/** Obtain configuration information. If a negative or zero wait time 
    is specified, the function will first check if it was previously 
    called and if yes, return the previously returned result. If no, 
    the information is obtained with a wait time equal the aboslute 
    value of the specified one.

    A timeout of zero corresponds to the default (1 sec).

    This function is not MT-safe when called with a positive wait
    time. In multi-threaded programs use getConfInfo_r instead. 

    @param id configuration id
    @param wait time to wait for answer
    @return pointer to list of configuration strings or NULL if failed
************************************************************************/
   const char* const* getConfInfo (int id, double wait);


/** Obtain configuration information. This function is MT-safe.
    The buffer must be at least a 1 kByte large.

    @param id configuration id
    @param wait time to wait for answer
    @param buf pointer to a buffer which holds the result
    @param len maximum length of the result buffer
    @return pointer to list of configuration strings or NULL if failed
************************************************************************/
   const char* const* getConfInfo_r (int id, double wait, 
                     char* buf, int len);

/** Parse a configuration information string. Returns interface
    name, interferometer number, id number, host name, port/program
    number, the version number and the sender's address.

    @param info configuration information string
    @param rec configuration information record (return)
    @return 0 if successful, <0 otherwise
************************************************************************/
   int parseConfInfo (const char* info, confinfo_t* rec);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_CONFINFO_H */
