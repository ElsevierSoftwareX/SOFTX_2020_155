/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: channel_server						*/
/*                                                         		*/
/* Module Description: channel database server				*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 22Aug99  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: channel_server.html					*/
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

#ifndef _GDS_CHANNEL_SERVER_H
#define _GDS_CHANNEL_SERVER_H

/** @name Channel Database Server
    Remote procedure service for the gds channel database. This rpc
    server should run on a unix platform as an inet daemon. The server
    provides channel information for clients.

    @memo rpc server for the gds channel database
    @author Written August 1999 by Daniel Sigg
    @see gdschannel.h
    @version 0.1
************************************************************************/

/*@{*/

/** Starts the rpc service for the gds channel database. This routine
    should be called from the channel database daemon with a list
    of channel configuration files as its argument. This function
    shall not return if successful.

    The default program/version number combination is 0x31001005/1.

    @param config list of configuration files
    @return never if successful, false otherwise
    @author DS, August 99
************************************************************************/
   bool channel_server (const char* const* config);

/*@}*/

#endif /* _GDS_CHANNEL_SERVER_H */
