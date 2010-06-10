/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched_server.h					*/
/*                                                         		*/
/* Module Description: Remote Scheduler Server API			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 04May98  D. Sigg    	First release		   		*/
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
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			uses sockets & rpc	*/
/*						(otherwise OK)		*/
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

#ifndef _GDS_SCHED_SRVR_H
#define _GDS_SCHED_SRVR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Header File List: */
#include "dtt/gdssched.h"
#include "dtt/gdsrsched.h"


/** @name Remote Scheduler Server API
    TBD.
   
    @memo Schedules tasks originating from an other computer
    @author Written May 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/	


/** @name Constants and flags
    * Constants and flags of the Remote Scheduler Server API.

    @memo Constants and flags
    @author DS, May 98
    @see Scheduler API
************************************************************************/

/*@{*/


/** Flag value identifying a remote scheduler descriptor on a remote
    computer acting as a server.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_REMOTE_SERVER	0x1000


/** Flag value representing a single threaded scheduler server.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_MT_SINGLE		0


/** Flag value representing a fully multi threaded scheduler server.
    Threads are started automatically for each client request. (Only
    works for rpcbind monitors and not for portmap monitors.)

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_MT_AUTO		1


/*@}*/


/** @name Data types
    * Data types of the Remote Scheduler Server API.

    @memo Data types
    @author DS, May 98
    @see Scheduler API
************************************************************************/

/*@{*/


/** Denotes a type representing a scheduler dispatch table entry. 
    See scheduler_dtentry_struct for more details.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   typedef struct scheduler_dtentry_struct scheduler_dtentry_t;


/** Denotes a type representing a scheduler dispatch table. 
    See scheduler_dt_struct for more details.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   typedef struct scheduler_dt_struct scheduler_dt_t;


/** Denotes a structure representing a scheduler dispatch table entry.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   struct scheduler_dtentry_struct {
      /** id of scheduling task. Used by the client to specify the
          scheduling function, resource deallocation function and the
          argument xdr function. */
      int		id;
      /** scheduling function */
      int (*func) (schedulertask_t* info,
                  taisec_t time, int epoch, void* arg);
      /** resource deallocation function */
      void (*freeResources) (taisec_t time, int epoch, void* arg);
      /** xdr function for the argument */
      xdrproc_t		xdr_arg;
   };


/** Denotes a structure representing a scheduler dispatch table. Each
    scheduler class requires a separate dispatch table. A dispatch table
    translates function id's provided by the remote machine into local
    fuction pointers.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   struct scheduler_dt_struct {
      /** rpc program number associated with the scheduler class */
      int			prognum;
      /** rpc version number associated with the scheduler class */
      int 			progver;
      /** number of entries in the dispatch table */
      int			numentries;
      /** actual dispatch table. Pointer to an array of dispatch table 
          entries. The number of valid entries is specified with 
          numentries. */
      scheduler_dtentry_t*	dt;
   };


/** Denotes a type representing a remote server scheduler descriptor. 
    See scheduler_info_struct for more details.
   
    @author DS, May 98
    @see Scheduler API
   ************************************************************************/
   typedef struct sscheduler_info_struct sscheduler_info_t;


/** Structure defining a remote server scheduleder descriptor. The 
    information in this structure must not be accessed directly.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   struct sscheduler_info_struct {
      /** mutex for this data structure. The elements of this
      structure can only accessed after inUse is increased; and
      they can only be changed when the task owns the semaphore
      and inUse is zero. The mutex is also needed to increase/
      decrease the inUse count. */
      schedmutex_t	sem;
      /** counts how many tasks are using this structure. This
      count can only be increased or decreased if the task owns the
      mutex. */
      int		inUse;
      /** pointer to dispatch table */
      scheduler_dt_t*	dispatch;
      /** program number of rpc service interface */
      u_long		prognum;
      /** version number of rpc service */
      int		progver;
      /** network address of client scheduler */
      struct in_addr	clienthost;
      /** program number of rpc callback */
      u_long		callbacknum;
      /** version number of rpc callback */
      int		callbackver;
      /** scheduler descriptor for callback */
      scheduler_r	rd;
      /** old close function */
      int 	(*closeScheduler) (scheduler_t* sd, tainsec_t timeout);   
   };


/*@}*/

/** @name Functions
    * Functions of the Remote Scheduler Server API.
   
    @memo Functions
    @author DS, May 98
    @see Scheduler API
   ************************************************************************/

/*@{*/


/** Register a new scheduler class. Each different type of scheduler must
    have its own rpc program number (the version number is usually one.)
    The dispatch table is used when scheduleTask is called from a remote
    client to determine which task has to be scheduled on the local machine.
    This function makes a copy of the provided scheduler class information.
    Allocated memory can be freed immediately after the function returns.
   
    @param schedclass pointer to scheduler class
    @return 0 if successful, <0 if failed
    @author DS, May 98
    @see Scheduler API
   ************************************************************************/
   int registerSchedulerClass (scheduler_dt_t* schedclass);


/** Starts the scheduler server. This routine should be called after all 
    scheduler classes have been registers. The server can be started by the 
    inetd daemon. This function will return 120s after all locally running 
    server schedulers were closed. On machines which are using the portmap 
    monitor the server will not be multi-tasked. On machines using a 
    rpcbind monitor a new thread can be started automatically with each 
    new client request.
   
    @param mode specifies whether a new thread is started with each 
           client call
    @return void
    @author DS, May 98
    @see Scheduler API
   ************************************************************************/
   void runSchedulerService (int mode);


/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _GDS_SCHED_SRVR_H */
