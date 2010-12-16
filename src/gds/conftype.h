/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: conftype						*/
/*                                                         		*/
/* Module Description: Types for the configuration information API	*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 29July99 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: conftype.html					*/
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

#ifndef _GDS_CONFTYPE_H
#define _GDS_CONFTYPE_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */

/** @name Configuration Information Types and Constants
    This header provides types for the configuration information API.

    @memo Configuration Information Types and Constants
    @author Written July 1999 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

/** Service name of arbitrary waveform generator */
#define			CONFIG_SERVICE_AWG	"awg"

/** Service name of test point manager */
#define			CONFIG_SERVICE_TP	"tp"

/** Service name of network data server */
#define			CONFIG_SERVICE_NDS	"nds"

/** Service name of diagnostics test kernel */
#define			CONFIG_SERVICE_TEST	"tst"

/** Service name of error logger */
#define			CONFIG_SERVICE_ERR	"err"

/** Service name of channel information database */
#define			CONFIG_SERVICE_CHN	"chn"

/** Service name of channel information database */
#define			CONFIG_SERVICE_LEAP	"leap"

/** Service name of network time server */
#define			CONFIG_SERVICE_NTP	"ntp"

/** Service name of calibration server */
#define			CONFIG_SERVICE_CAL	"cal"

/** Service name of launch server */
#define			CONFIG_SERVICE_LAUNCH	"launch"

/** Configuration information record.
************************************************************************/
   struct confinfo_t {
      /** interface name */
      char		interface[8];
      /** interferometer number (-1 for *) */
      int		ifo;
      /** interface id number (-1 for *) */
      int		num;
      /** host name */
      char		host[64];
      /** port/program number of interface (-1 for *) */
      int		port_prognum;
      /** interface version (-1 for *) */
      int		progver;
      /** sender's address */
      char		sender[64];
   };
   typedef struct confinfo_t confinfo_t;

   typedef struct confServices confServices;

/** Configuration service callback.
    @param conf pointer to configuration service structure
    @param arg string argument
************************************************************************/
   typedef char* (*confCallback) (const confServices* conf, 
                     const char* arg);

/** Configuration service structure.
************************************************************************/
   struct confServices {
      /** query id */
      int		id;
      /** callback function */
      confCallback	answer;
      /** user argument */
      char*		user;
   };


/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_CONFTYPE_H */
