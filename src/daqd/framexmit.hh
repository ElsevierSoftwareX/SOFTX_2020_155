/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: framexmit						*/
/*                                                         		*/
/* Module Description: API for broadcasting frames			*/
/*		       implements a reliable UDP/IP broadcast for	*/
/*                     large data sets over high speed links		*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 1.0	 10Aug99  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: doc/index.html (use doc++)				*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.7			*/
/*	Compiler Used: egcs-1.1.2					*/
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
/* LIGO Project MS NW17-161				   		*/
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

#ifndef _GDS_FRAMEXMIT_H
#define _GDS_FRAMEXMIT_H

// include files
#include "framesend.hh"
#include "framerecv.hh"

/** @name Frame broadcast API
    This API implements a reliable UDP/IP broadcast transfer. This API
    is intended to broadcast frame data from a frame builder machine
    to the on-line Data Monitoring Tools over gigabit ethernet.
    The main features are:
    \begin{verbatim}
    - UDP/IP broadcast protocol using propritary packets
    - optmized for high speed links and bulk data transfer
    - reliable; uses UDP/IP retransmission packets
    - quality of service implemented in the receiver
    - simple buffer management
    - one transmitter / any number of receivers
    - all handshaking hidden from the user; join at any time
    - no complicated start-up procedures
    \end{verbatim}

    @memo Frame broadcast API
    @author Written August 1999, by Daniel Sigg
    @version 3.0
************************************************************************/

/*@{*/

//@Include: framexmittypes.hh
//@Include: framesend.hh
//@Include: framerecv.hh

/** @name Examples
    Example of a transmitter:
    \URL[sndtest.cc]{sndtest.ps}

    Example of a receiver:
    \URL[rcvtest.cc]{rcvtest.ps}

    @memo Sender and receiver examples
************************************************************************/

/** @name Download

    Get version 2.0 from \URL[here]{framexmit-2.0.tar.gz}

    Get version 3.0 from \URL[here]{framexmit-3.0.tar.gz}

    @memo Download the distribition
************************************************************************/

/*@}*/

#endif // _GDS_FRAMEXMIT_H
