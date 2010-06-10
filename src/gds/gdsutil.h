/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsutil 						*/
/*                                                         		*/
/* Module Description: includes common GDS libraries and headers	*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 2Apr98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdserrmsg.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.5.1		*/
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

#ifndef _GDS_UTIL_H
#define _GDS_UTIL_H

/* Header File List: */

#include "dtt/gdsmain.h"
#include "dtt/gdsprm.h"
#include "dtt/gdserrmsg.h"
#include "dtt/gdserr.h"
#include "tconv.h"
#include "dtt/gdsheartbeat.h"
#include "dtt/gdsstring.h"


/* doc++ stuff */

/** @name Utility Libraries
    Includes common utility libraries and headers. Use the following
    include statement to automatically include all utility libraries. 

    #include "gdsutil.h"
 
    @memo Common utility libraries and headers
    @author Written Mar. 1998 by Daniel Sigg
    @version 0.5
************************************************************************/

/*@{*/		/* utility libraries */


/** @name gdsmain
    Sets up the compiling environment. Include with
    
    #include "gdsmain.h"
    
    @memo Compiling environment
    @see Compiling Environment
************************************************************************/

/** @name cobox
    Interface for cobox (ethernet-to-RS232).
    
    #include "cobox.h"

    @memo Cobox API
    @see Cobox API
************************************************************************/


/** @name gdserr
    Error message API. Implements an interface to write console, error,
    warning and debug messages to the GDS system console. Include with
    
    #include "gdserr.h"

    @memo Error message API
    @see Error Messages
    @see Error Message API
    @see Error Message Log Daemon
************************************************************************/

/** @name gdserrmsg
    Definition of common error messages. Include with
    
    #include "gdserrmsg.h"

    @memo Error messages
    @see Error Messages
    @see Error Message API
    @see Error Message Log Daemon
************************************************************************/

/** @name gdsheartbeat
    System hearbeat. Implements the system heartbeat (typically 16Hz
    for LIGO).
    
    #include "gdsstring.h"

    @memo Heartbeat API
    @see Heartbeat API
************************************************************************/


/** @name gdsprm
    Parameter file API. Implements an interface to read parameter
    files and access its parameter values. Include with
    
    #include "gdsprm.h"

    @memo Parameter file API
    @see Parameter File API
************************************************************************/

/** @name gdsstring
    Additional string and channel handling utilities.
    
    #include "gdsstring.h"

    @memo String Utilities
    @see String Utilities
************************************************************************/


/** @name tconv
    Time conversion API. Converts between coordinate universal time
    (UTC) and international atomic time (TAI).Include with
    
    #include "tconv.h"

    @memo Time Conversion API
    @see Time Conversion API
************************************************************************/


/*@}*/


#endif /*_GDS_UTIL_H */
