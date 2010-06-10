/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsdac							*/
/*                                                         		*/
/* Module Description: gps time conversion				*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   6Aug98  ST        						*/
/*                                                         		*/
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Serap Tilav   							*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			OK			*/
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

#ifndef _GDS_GPSTIME_H
#define _GDS_GPSTIME_H

#ifdef __cplusplus 
extern "C" {
#endif

#ifdef OS_VXWORKS
   typedef unsigned long long uint64_t;
#else
#include <stdlib.h>
#ifdef linux
#include <stdint.h>
#endif
#endif

   struct tais_t {
      uint64_t		x;
      uint64_t		s;
   } ;
   typedef struct tais_t tais_t;

   struct gpstime_t {
      long		year;
      long		yearday;
      int		hour;
      int		minute;
      int		second;
   };
   typedef struct gpstime_t gpstime_t;

   struct caldate_t {
      long		year;
      int		month;
      int		day;
   };
   typedef struct caldate_t caldate_t;

   extern void gpstime_to_gpssec (const gpstime_t* gt, tais_t* t);
   extern long caldate_mjd (const caldate_t* cd);


#ifdef __cplusplus
}
#endif

#endif /*_GDS_GPSTIME_H */
