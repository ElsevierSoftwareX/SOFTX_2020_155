/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdserrmsg						*/
/*                                                         		*/
/* Module Description: Defines error messages	 			*/
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

#ifndef _GDS_ERR_MSG_H
#define _GDS_ERR_MSG_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */

#define MAXERRMSG  1024

/* must use negative numbers */
#define GDS_ERR_NONE 0
#define GDS_ERRMSG_NONE "no error"

#define GDS_ERR_PROG -1
#define GDS_ERRMSG_PROG "general program error"

#define GDS_ERR_PRM -2
#define GDS_ERRMSG_PRM "parameter error"

#define GDS_ERR_MEM -3 
#define GDS_ERRMSG_MEM "memory error"

#define GDS_ERR_FILE -4
#define GDS_ERRMSG_FILE "file access error"

#define GDS_ERR_FORMAT -5
#define GDS_ERRMSG_FORMAT "format error"

#define GDS_ERR_MISSING -6
#define GDS_ERRMSG_MISSING "missing argument error"

#define GDS_ERR_VERSION -7
#define GDS_ERRMSG_VERSION "version conflict error"

#define GDS_ERR_MATH -8
#define GDS_ERRMSG_MATH "floating point error"

#define GDS_ERR_CORRUPT -9
#define GDS_ERRMSG_CORRUPT "corrupt resource error"

#define GDS_ERR_ARG -10
#define GDS_ERRMSG_ARG "function argument error"

#define GDS_ERR_UNDEF -12
#define GDS_ERRMSG_UNDEF "undefined error"

#define GDS_ERR_TIME -11
#define GDS_ERRMSG_TIME "timing error error"

#define GDS_ERR_SET { \
     {GDS_ERR_NONE, GDS_ERRMSG_NONE}, \
     {GDS_ERR_PROG, GDS_ERRMSG_PROG}, \
     {GDS_ERR_PRM, GDS_ERRMSG_PRM}, \
     {GDS_ERR_MEM, GDS_ERRMSG_MEM}, \
     {GDS_ERR_FILE, GDS_ERRMSG_FILE}, \
     {GDS_ERR_FORMAT, GDS_ERRMSG_FORMAT}, \
     {GDS_ERR_MISSING, GDS_ERRMSG_MISSING}, \
     {GDS_ERR_VERSION, GDS_ERRMSG_VERSION}, \
     {GDS_ERR_MATH, GDS_ERRMSG_MATH}, \
     {GDS_ERR_CORRUPT, GDS_ERRMSG_CORRUPT}, \
     {GDS_ERR_ARG, GDS_ERRMSG_ARG}, \
     {GDS_ERR_TIME, GDS_ERRMSG_TIME}, \
     {GDS_ERR_UNDEF, GDS_ERRMSG_UNDEF}, \
     {1,""}}

#if 0
/* doc++ stuff */

/**
   @name Error Messages
   * A list of error number and messages. Error numbers are defined
   as GDS_ERR_X, whereas error messages are defined as GDS_ERRMSG_X.

   @memo A list of common error messages
   @author Written Mar. 1998 by Daniel Sigg
   @version 0.5
   @see Error Message Log Daemon, Error Message API
************************************************************************/

/*@{*/		/* subset of GDS Parameter File API documentation */


/** Indicates no error. Error message: no erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_NONE

/** Indicates a program error. Error message: program error.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_PROG

/** Indicates a parameter file error. Error message: parameter error.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_PRM

/** Indicates a memory allocation error. Error message: memory error.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_MEM

/** Indicates file access problem. Error message: file erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_FILE

/** Indicates an illegal string format. Error message: format erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_FORMAT

/** Indicates a missing argument. Error message: missing argument erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_MISSING

/** Indicates a version conflict. Error message: version conflict erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_VERSION

/** Indicates a math error. Error message: floating point erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_MATH

/** Indicates a corrupted resource. Error message: corrupt resource erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_CORRUPT

/** Indicates a timing error. Error message: timing erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_TIME

/** Indicates an error of undefined source. Error message: undefined erorr.  

    @author DS, March 98
    @see Error Message API, Error Message Log Daemon
************************************************************************/
#define GDS_ERR_UNDEF

/*@}*/
#endif

#ifdef __cplusplus
}
#endif

#endif /*_GDS_ERR_MSG_H */
