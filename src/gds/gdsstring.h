/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsstring 						*/
/*                                                         		*/
/* Module Description: adds additional string handling routines		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 24Apr98  MRP/DS    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdsstring.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/* Mark Pratt    (617) 253-6410			 mrp@mit.edu		*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.5.1		*/
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
#ifndef _GDS_STRING_H
#define _GDS_STRING_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @name String utilities
   Additional string handling routines. Implements string handling
   routines to compare string without case-sensitivity and implements
   routines to deal with the LIGO channel naming convention.

   @memo Additional string handling routines
   @author MRP/DS, Apr. 1998  */

/*@{*/

/** @name Standard string routines
   Implements additional string handling routines.

   @memo String handling routines which were 'forgotten'
   @author MRP/DS, Apr. 1998  */

/*@{*/

/** Rewrite of unix (semi)standard function.

    Case-insensitive version of strcmp().
    @author MRP, Apr. 1998 
*********************************************************************/
   int gds_strcasecmp (const char* s1, const char* s2);

/** Rewrite of unix (semi)standard function.

    Case-insensitive version of strncmp().
    @author MRP, Apr. 1998 
*********************************************************************/
   int gds_strncasecmp (const char* s1, const char* s2, int n);

/** Returns the end of a string.

    @author DS, Apr. 1998 
*********************************************************************/
   char* strend (const char* s);

/** Like strcpy, but returns the end of the string.

    @author DS, Apr. 1998 
*********************************************************************/
   char* strecpy (char* s1, const char* s2);

/** Like strncpy, but returns the end of the string.

    @author DS, Apr. 1998 
*********************************************************************/
   char* strencpy (char* s1, const char* s2, size_t n);

#ifdef OS_VXWORKS
/** Duplicates a string.

    @author DS, Apr. 1998 
*********************************************************************/
   char* strdup (const char* s);
#endif

/*@}*/

/** @name Channel name routines
   Implements string handling routines dealing with channel names.

   @memo String handling routines for channel names
   @author MRP/DS, Apr. 1998  */

/*@{*/ 

/** Returns a non zero value if it is a valid channel name.
    A valid channel name consists of a site prefix, an 
    interferometer prefix, a subsystem name, an identification
    within the susbsystem, a ':' (colon) between the interferometer
    prefix and the subsystem anme and a '-' (dash) between
    the subsystem name and the identification. It's general form is:

    <site prefix><ifo prefix>:<subsystem>-<identification>

    This function returns a non-zero value if the channel name
    is of the above fromat, it doesn't check whether the name 
    actually exists in the channel database of the detector.

    @param name channel name 
    @param site site prefix buffer
    @return pointer to site prefix buffer or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   int chnIsValid (const char* name);


/** Returns the site prefix of a channel.
    Returns NULL on error.

    @param name channel name 
    @param site site prefix buffer
    @return pointer to site prefix buffer or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   char* chnSitePrefix (const char* name, char* site);

/** Returns the interfeormeter prefix of a channel.
    Returns NULL on error.

    @param name channel name 
    @param ifo interferometer prefix buffer
    @return pointer to interferometer prefix buffer or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   char* chnIfoPrefix (const char* name, char* ifo);

/** Returns the subsystem name of a channel.
    Retruns NULL on error.

    @param name channel name 
    @param subsys subsystem name buffer
    @return pointer to subsystem name buffer or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   char* chnSubsystemName (const char* name, char* subsys);

/** Returns the name of a channel without site, interferometer and
    subsystem name. Returns NULL on error.

    @param name channel name 
    @param rem buffer for remaining characters
    @return pointer to remaining character buffer or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   char* chnRemName (const char* name, char* rem);

/** Returns the name of a channel without site and interferometer
    prefix. Returns NULL on error.

    @param name channel name 
    @param sname buffer for short name
    @return pointer to short name buffer or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   char* chnShortName (const char* name, char* sname);

/** Returns a channel name build up from its components. 
    Returns NULL on error. If the ifo parameter is NULL, it is
    assumed that the site parameter consists of both the site and
    the interferometer prefix. If the srem paramater is NULL, it
    is assumed that the sname parameter consists of both the 
    subsystem name and the identification within the subsystem.
    Returns NULL on error.

    @param name channel name buffer 
    @param site site prefix
    @param ifo interferometer prefix
    @param sname subsystem name
    @param srem identification within the subsystem
    @return pointer to channel name or NULL
    @author DS, Apr. 1998 
*********************************************************************/
   char* chnMakeName (char* name, const char* site, const char* ifo,
                     const char* sname, const char* srem);

/*@}*/

#ifdef __cplusplus
}
#include "dtt/gdsstringcc.hh"
#endif

/*@}*/

#endif /* _GDS_STRING_H */
