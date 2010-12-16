/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpoint_server					*/
/*                                                         		*/
/* Module Description: API for handling testpoints			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 25June98 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: testpoint_server.html				*/
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

#ifndef _GDS_TESTPOINT_SERVER_H
#define _GDS_TESTPOINT_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif


/**
   @name Test Point Server
   Remote procedure service for test point interface. This rpc
   server should run on the DAQ system controller. It exports routines
   to set and clear test points, as well as to query the interface.

   @memo rpc server for test point interface
   @author Written June 1998 by Daniel Sigg
   @see Test Point API
   @version 0.1
************************************************************************/

/*@{*/

/** Starts the rpc service for the test point interface. This routine
    has to be called by the DAQ/GDS system controller as part of its 
    initialization after the reflective memory is setup.

    On VxWorks it has to be started as a separate task, since it does not 
    return. The rpc server parameters are read in from a parameter file 
    located at "param/init/<site>/testpoint.par". The format of the file 
    is a follows:
    \begin{verbatim}
    [node0]
    hostname = 10.1.0.18
    prognum = 0x31001001
    progver = 1
    \end{verbatim}
    Each testpoint node must have its one section. A test point server
    can serve multiple nodes. Upon initialization the srever scans 
    through the parameter file and looks for its entry. It then registers
    an rpc server with the given program number and version and waits for
    requests. This parameter file is also used by a test point client to 
    lookup the available test point servers. The default program/version 
    number combination is 0x31001001/1.

    IMPORTANT! In order to answer to query requests the test point 
    client interface (testpoint.c) has to be compiled with 
    _TESTPOINT_DIRECT enabled for the nodes served by this server.
    Also _NO_TESTPOINTS must not be defined during compilation.

    @param void
    @return 0 if successful, <0 error number otherwise
    @see Test Point API
    @author DS, June 98
************************************************************************/
   int testpoint_server (void);


/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_TESTPOINT_SERVER_H */
