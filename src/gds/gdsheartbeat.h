/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsheartbeat.h						*/
/*                                                         		*/
/* Module Description: Heartbeat API	 				*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 13Apr98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdsheartbeat.html					*/
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

#ifndef _GDS_HEARTBEAT_H
#define _GDS_HEARTBEAT_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include "tconv.h"

/** @name Heartbeat API
    * The heartbeat API provides a global semaphore which is periodically 
    released to all waiting tasks by the system heartbeat (in LIGO: 
    16Hz GPS synchronized clock). It also provides the necessary interrupt 
    service routines and a set of tools to utilize this heartbeat.
    Careful: on sparc platforms this will create a multi-threaded program
    and has to be compiled with the "-D_POSIC_C_SOURCE=199506L" and 
    linked with "-lposix4 -lpthread", respectively.
   
    @memo Synchronization tools for the system heartbeat
    @author Written April 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/	


/** @name Constants and flags
    * Constants and flags of the heartbeat API.

    @memo Constants and flags
    @author DS, April 98
    @see Heartbeat API
************************************************************************/

/*@{*/

/*@}*/


/** @name Functions
    * Functions of the heartbeat API.

    @memo Functions
    @author DS, April 98
    @see Heartbeat API
************************************************************************/

/*@{*/


/** Installs an interrupt service routine on the system clock which
    provides the heartbeat. If a NULL pointer is passed to the routine,
    the default ISR is used. The ISR must reset board and system specific
    interrupt registers before it calls doHeartbeat. This routine will
    fail if the hearbeat sempahore is already setup. DO NOT call
    initHeartbeat before this routine.

    @param ISR interrupt service routine
    @return 0 if successful, negative error number otherwise
    @author DS, April 98
    @see Heartbeat API
************************************************************************/
   int installHeartbeat (void ISR (void));


/** Blocks until the next heartbeat is detected. This function
    can be called from more than one task concurrently. For VxWorks
    this is equivalent to semTake (sem).

    @param void
    @return 0 if successful, negative error number otherwise
    @author DS, April 98
    @see Heartbeat API
************************************************************************/
   int syncWithHeartbeat (void);


/** Blocks until the next heartbeat is detected. This function
    can be called from more than one task concurrently. For VxWorks
    this is equivalent to semTake (sem). If successful, it will 
    return TAI and epoch of the heartbeat.

    @param tai TAI of heartbeat in sec (pointer)
    @param epoch Epoch of hearttbeat (pointer)
    @return 0 if successful, negative error number otherwise
    @author DS, April 98
    @see Heartbeat API
************************************************************************/
   int syncWithHeartbeatEx (taisec_t* tai, int* epoch);


/** Sets up the hearbeat. This function creates the semaphore which is
    used to provide the heartbeat. For VxWorks this is equivalent 
    to semBCreate(SEM_Q_FIFO, SEM_EMPTY). This function should be called
    before either doHeartbeat or syncWithHeartbeat. However, it
    is automatically called by installHeartbeat and should not be
    called directly when installHeartbeat is used.

    @param void
    @return 0 if successful, negative error number otherwise
    @author DS, April 98
    @see Heartbeat API
************************************************************************/
   int setupHeartbeat (void);


/** Does one heartbeat. This function is used by the heartbeat
    interrupt server routine to release a semaphore. It should never be
    called directy. For VxWorks this is equivalent to semFlush(sem).

    @param void
    @return 0 if successful, negative error number otherwise
    @author DS, April 98
    @see Heartbeat API
************************************************************************/
   int doHeartbeat (void);


/** Returns the number of heartbeats. Everytime doHeartbeat is called
    the count of heartbeats is increased by one. This function should
    be used with care, since heartbeats can be lost when the system
    is overloaded.

    @param void
    @return number of heartbeats since heartbeat was installed
    @author DS, April 98
    @see Heartbeat API
************************************************************************/
   unsigned long getHeartbeatCount (void);


/*@}*/

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /* _GDS_HEARTBEAT_H */
