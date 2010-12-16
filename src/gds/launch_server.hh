/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: launch_server						*/
/*                                                         		*/
/* Module Description: launch server					*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 22Apr02  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: launch_server.html					*/
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
/* LIGO Project NW37-278				   		*/
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

#ifndef _GDS_LAUNCH_SERVER_H
#define _GDS_LAUNCH_SERVER_H

#include "dtt/gdsstring.h"

namespace diag {

/** @name Launch Server
    Remote procedure service for launching programs on an other
    machine. This rpc server should run on a unix platform as an 
    inet daemon. The server provides information about the available
    programs and implements a remote exec command. Launch servers
    also install a configuration information server about their
    services.

    @memo rpc server for launching programs
    @author Written April 2002 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

/** Starts the rpc service for the launch server. This routine
    should be called from the launch server daemon with a configuration 
    file as its argument. This function shall not return if successful.

    The default program/version number combination is 0x31001007/1.

    @param config launcher configuration file
    @return never if successful, false otherwise
    @author DS, August 99
************************************************************************/
   bool launch_server (const string& config);

/*@}*/

}

#endif /* _GDS_LAUNCH_SERVER_H */
