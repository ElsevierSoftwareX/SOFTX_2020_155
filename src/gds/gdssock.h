/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssock							*/
/*                                                         		*/
/* Module Description: Socket utility library				*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 22Augy99 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdssock.html						*/
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

#ifndef _GDS_GDSSOCK_H
#define _GDS_GDSSOCK_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#ifndef OS_VXWORKS
#include "sockutil.h"
#else
#include <sockLib.h>
#include <inetLib.h>

/** @name Socket Utlities
    This API provides additional utility functions to deal with
    sockets.

    @memo Socket Utlities
    @author Written August 1999 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

/** Looks up a hostname. This function uses DNS if necessary. If the
    hostname is NULL, the (first) address of the local machine is 
    returned.

    @param hostname name of the host (NULL for self)
    @param addr IP address (return)
    @return 0 if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int nslookup (const char* hostname, struct in_addr* addr);

#ifndef OS_VXWORKS
/** Connects a socket with a timeout. This function is similar to 
    connect, but implements an additional user specified timeout.

    @param sock socket
    @param name address to conenct
    @param size of socket address
    @param timeout connection timeout
    @return 0 if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int connectWithTimeout (int sock, struct sockaddr* name, int size, 
                     struct timeval* timeout);
#endif

/** Pings the given host. This ping will try to establish a connection
    to the echo port of the specified host machine. If a connection
    is established or refused, ping returns with true. If the connection
    attempt aborts with a timeout, the function returns false. A non
    positive timeout value is interpreted as the default (10 sec).

    @param hostname name of the machine to ping
    @param timeout maximum time to wait for response (sec)
    @return true if alive, false otherwise
    @author DS, July 98
************************************************************************/
   int ping (const char* hostname, double timeout);

#endif

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_GDSSOCK_H */
