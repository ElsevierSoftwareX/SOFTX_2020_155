/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awgapi_server						*/
/*                                                         		*/
/* Module Description: API for controlling an arbitrary function 	*/
/*		       generator					*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 4July98 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: awgapi_server.html					*/
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

#ifndef _GDS_AWGAPI_SERVER_H
#define _GDS_AWGAPI_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif


/**
   @name Server for Arbitrary Waveform Generator
   * Remote procedure service for controlling an arbitrary waveform 
   generator. This rpc server has to run on the same VME CPU as the
   corresponding AWG. It exports routines to set and query waveforms.

   @memo rpc server for arbitrary waveform generators
   @author Written July 1998 by Daniel Sigg
   @see Arbitrary Waveform Generator API
   @version 0.1
************************************************************************/

/*@{*/

/** Starts the rpc service for the arbitrary waveform generator interface. 
    This routine has to be called during startup by the AWG cpu as part of 
    the initialization. This routine does not return under normal 
    conditions.

    @return 0 if successful, <0 error number otherwise
    @see Arbitrary Waveform Generator API
    @author DS, July 98
************************************************************************/
   int awg_server (void);


/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_AWGAPI_SERVER_H */
