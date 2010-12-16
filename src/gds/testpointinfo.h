/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpointinfo						*/
/*                                                         		*/
/* Module Description: utility routines for handling testpoints		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 25June98 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: testpointinfo.html					*/
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
/*			  ANSI			OK			*/
/*			  POSIX			OK			*/
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
/* LIGO Project NW17-145				   		*/
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

#ifndef _GDS_TESTPOINTINFO_H
#define _GDS_TESTPOINTINFO_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include "tconv.h"
#include "dtt/gdschannel.h"


/**
   @name Test Point Information
   A set of utility routines to obtain information about test points.

   @memo Test point utilities
   @author Written June 1998 by Daniel Sigg
   @version 0.1
************************************************************************/

/*@{*/

/** @name Constants and flags.
    * Constants and flags of the test point API.

    @memo Constants and flags
    @author DS, June 98
    @see Test point API
************************************************************************/

/*@{*/


/*@}*/


/** @name Data types.
    * Data types of the test point API.

    @memo Data types
    @author DS, June 98
    @see Test point API
************************************************************************/

/*@{*/

#ifndef __TESTPOINT_T
#define __TESTPOINT_T
/** Defines the type of a test point.

    @author DS, June 98
    @see Test point API
************************************************************************/
#if defined(_ADVANCED_LIGO) && !defined(COMPAT_INITIAL_LIGO)
   typedef unsigned int testpoint_t;
#else
   typedef unsigned short testpoint_t;
#endif
#endif

/** Type of a test point.

    @author DS, June 98
    @see Test point API
************************************************************************/
   enum testpointtype {
   /** not a test point */
   tpInvalid = 0,
   /** LSC excitation test point */
   tpLSCExc = 1,
   /** LSC test point */
   tpLSC = 2,
   /** ASC excitation test point */
   tpASCExc = 3,
   /** ASC test point */
   tpASC = 4,
   /** digital-to-analog converter channel*/
   tpDAC = 5,
   /** digital signal generator channel */
   tpDSG = 6
   };
   typedef enum testpointtype testpointtype;

/*@}*/

/** @name Functions.
    * Functions of the test point API.

    @memo Functions
    @author DS, June 98
    @see Test point API
************************************************************************/

/*@{*/

/** Returns TRUE if channel is a test point. This function takes a 
    channel info as input. The node id and the test point number are 
    returned if the corersponding return argument pointers aren't NULL.

    @param chn channel info
    @param node return argument for node id
    @param tp return argument for test point value
    @return 1 if test point, 0 otherwise
    @author DS, June 98
************************************************************************/
   int tpIsValid (const gdsChnInfo_t* chn, int* node, testpoint_t* tp);

/** Returns TRUE if channel is a test point. This function takes a 
    channel name and queries the channel database to find out if it
    is a test point. The node id and the test point number are returned 
    if the corersponding return argument pointers aren't NULL.

    @param chnname channel name
    @param node return argument for node id
    @param tp return argument for test point value
    @return 1 if test point, 0 otherwise
    @author DS, June 98
************************************************************************/
   int tpIsValidName (const char* chnname, int* node, testpoint_t* tp);

/** Returns the type of a test point. If not a test point, tpInvalid
    is returned.

    @param chn channel info
    @return test point type
    @author DS, June 98
************************************************************************/
   testpointtype tpType (const gdsChnInfo_t* chn);

/** Returns the type of a test point. If not a test point, tpInvalid
    is returned.

    @param chnname channel name
    @return test point type
    @author DS, June 98
************************************************************************/
   testpointtype tpTypeName (const char* chnname);

/** Returns the readback channel information of an excitation channel.
    In some cases, e.g. DAC channels, the read back channel is 
    different form the excitation channel.

    @param chn channel info
    @param rb channel info of readback channel (return)
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpReadback (const gdsChnInfo_t* chn, gdsChnInfo_t* rb);

/** Returns the name of the readback channel of an excitation channel.
    In some cases, e.g. DAC channels, the read back channel is 
    different form the excitation channel.

    @param chnname channel name
    @param rbname name of readback channel (return)
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int tpReadbackName (const char* chnname, char* rbname);

/*@}*/

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_TESTPOINTINFO_H */
