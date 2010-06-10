/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: confserver						*/
/*                                                         		*/
/* Module Description: Configuration information server			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 29July99 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: confservers.html					*/
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

#ifndef _GDS_CONFSERVER_H
#define _GDS_CONFSERVER_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include "dtt/conftype.h"

/** @name Configuration Server API
    This API provides an implementation of a configuration information
    server.

    @memo Configuration Server API
    @author Written July 1999 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

/** Starts a configuration information server. It requires a list
    of configuration services as an argument. If successful this 
    function will start a task which listens at the configuration
    server port for requests. This function will not return if started
    with async set to false and no error is encountered. The following
    flag values are supported: 0 - start server and do not return, 
    1 - start server as independent task, 2 - start server as port monitor.

    @param confs list of configuration services
    @param num number of configuration services
    @param flag determines how listener is started
    @return 0 if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int conf_server (const confServices confs[], int num, int flag);


/** Standard answer function. This function is an answer callback 
    function which interprets the user argument of conf as a char*
    and returns a copy of this string. The argument is ignored.

    @param conf configuration service
    @param arg argument
    @return answer
    @author DS, July 98
************************************************************************/
   char* stdAnswer (const confServices* conf, const char* arg);


/** Standard answer function with ping. This function is similar to
    the standard answer function, but checks if the host is alive 
    before replying. If the host isn't alive, the answer is NULL.

    @param conf configuration service
    @param arg argument
    @return answer
    @author DS, July 98
************************************************************************/
   char* stdPingAnswer (const confServices* conf, const char* arg);


/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_CONFSERVER_H */
