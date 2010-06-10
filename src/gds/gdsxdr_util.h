/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsxdr_util.h						*/
/*                                                         		*/
/* Module Description: xdr encodeing and decodeing utilities		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 13Apr98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdssched.html					*/
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
/*			  ANSI			OK			*/
/*			  POSIX			OK (may use xdr)	*/
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

#ifndef _GDS_XDR_UTIL_H
#define _GDS_XDR_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined (__EXTENSIONS__)
#define __EXTENSIONS__
#endif


/* Header File List: */
#include <rpc/rpc.h>


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Functions:		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int xdr_encodeArgument (const char* xdr_struct, char** xdr_stream, 
                     unsigned int* xdr_stream_len, xdrproc_t xdr_func);

   int xdr_decodeArgument (char** xdr_struct, unsigned int xdr_struct_len,
                     const char* xdr_stream, unsigned int xdr_stream_len,
                     xdrproc_t xdr_func);

#ifdef __cplusplus
}
#endif

#endif /* _GDS_XDR_UTIL_H */
