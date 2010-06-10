/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched_client.h					*/
/*                                                         		*/
/* Module Description: Remote Scheduler Client API			*/
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
/*			  ANSI			OK			*/
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

#ifndef _GDS_SCHED_CLNT_H
#define _GDS_SCHED_CLNT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Header File List: */
#include "dtt/gdssched.h"
#include "dtt/gdsrsched.h"

/** @name Remote Scheduler Client API
    * This API can be used to interface a scheduler running on an
    other computer. Scheduled tasks scheduled will automatically
    be transfered to the remote computer and put into a locally
    running scheduler there. All other functions of the scheduler
    behave as expected with the difference that the routine is 
    called remotely (using ONC rpc). After creating a scheduler
    descriptor on the local computer (refred to as the remote
    scheduler) an additional bind procedure has to be called to 
    create and initialize a scheduler for the remote computer
    (called a bound scheduler). A remote scheduler can manage
    more than one bound scheduler if needed. This feature is useful 
    to synchronize tasks running on several different computers, 
    since time tags are automatically propagated to all schedulers 
    which were bound with the same remote scheduler. 

    When scheduling a task on a remote computer there are a couple of
    differences: when using the scheduler descriptor returned 
    by the create function the task gets scheduled on all bound
    schedulers, whereas when using scheduler descriptor returned
    by the bind function it gets scheduled only on the specified
    remote computer. Also, the function pointer of a scheduled task
    has to be an integer id which is defined in the dispatch table
    of the remote scheduler server. Arguments are copied to the 
    remote computer when scheduled and the (local) memory can be 
    released immediately after scheduling. On the local computer 
    the free resources function is never called. Important! An 
    additional xdr routine for the argument data has to be provided 
    instead of the freeResources function (see rpcgen on how to create 
    a xdr routine). Indeed specifying a freeResources on the local
    machine has no effect at all.

    The actual functions which are called by the scheduler when their
    scheduling conditions are met have to be defined in the
    remote scheduler server application (see Remote Scheduler Server
    API on how to do this). Since more than one type of remote scheduler
    might be avaialable on a remote computer, they have to be 
    distinguished by a unique id number. This id number has to be 
    specified when calling the bind routine (together with a version
    number). This id number has to be a valid rpc program number (see
    also the "ONC+ developer's guide").
   
    @memo Schedules tasks on remote computers
    @author Written May 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/	


/** @name Constants and flags
    * Constants and flags of the Remote Scheduler Client API.

    @memo Constants and flags
    @author DS, May 98
    @see Scheduler API
************************************************************************/

/*@{*/

/** Flag value identifying a remote scheduler descriptor.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_REMOTE		0x0100


/** Flag value identifying a remote scheduler descriptor on a local
    computer.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_REMOTE_LOCAL	0x0200


/** Flag value identifying a remote scheduler descriptor bound to a 
    scheduler on a remote computer.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_REMOTE_BOUND	0x0400


/** Flag value identifying a remote scheduler descriptor bound to a 
    scheduler on the local computer.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
#define SCHED_LOCAL_BOUND	0x0600


/*@}*/


/** @name Data types
    * Data types of the Remote Scheduler Client API.

    @memo Data types
    @author DS, May 98
    @see Scheduler API
************************************************************************/

/*@{*/


/** Denotes a type representing a remote scheduler descriptor. 
    See rscheduler_info_struct for more details.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   typedef struct rscheduler_info_struct rscheduler_info_t;


/** Structure defining a remote scheduleder descriptor. The information 
    in this structure must not be accessed directly.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   struct rscheduler_info_struct {
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
      /** parent descriptor */
      scheduler_t*	parent;
      /** self descriptor */
      scheduler_t*	self;
      /** list of bound remote schedulers */	
      rscheduler_info_t**	rlist;
      /** maximum length of remote scheduler list */
      int		maxrlist;
      /** */
      struct in_addr	remotehost;
      /** program number of remote scheduler rpc interface for
      bound schedulers */
      unsigned long	remotenum;
      /** version number of remote scheduler rpc interface for
      bound schedulers */
      int		remotever;
      /** remote scheduler descriptor */
      scheduler_r	rd;
      /** client handle */
      CLIENT*		clnt;		
      /** old close function */
      int 	(*closeScheduler) (scheduler_t* sd, tainsec_t timeout);
   
   };


/*@}*/

/** @name Functions
    * Functions of the Remote Scheduler Client API.
   
    @memo Functions
    @author DS, May 98
    @see Scheduler API
   ************************************************************************/

/*@{*/


/** Creates a new loacl scheduler for remote use. This new scheduler can 
    be bound to a scheduler on a remote computer and can be used to 
    synchronize tasks between different computers. It returns a scheduler 
    descriptor if successful, or NULL if failed. This scheduler 
    descriptor has to be used in future calls to bindRemoteScheduler to 
    create and initialize the schedulers on the remote computers.
   
    @param flags specfies the scheduler options
    @return pointer to the new scheduler
    @author DS, May 98
    @see Scheduler API
   ************************************************************************/
   scheduler_t* createRemoteScheduler (int flags);


/** Initializes a scheduler on a remote computer. Binds a new scheduler
    to a remote scheduler. This function
    can be called more than once to synchronize tasks on different
    remote computers. The returned scheduler descriptor identifies the
    remote scheduler and can be used in future calls to the scheduler
    API to identify a specific remote scheduler. (The descriptor 
    returned by the create routine referes to the set of all bound
    schedulers.) The close routine for a 'bound' scheduler 
    descriptor is automatically invoked when the close routine is
    called with the descriptor returned by the create routine.

    When the hostname is NULL or the loop back address (127.0.0.1) the 
    descriptor binds to a scheduler running on the local computer. This 
    is not the same as specifying the valid network name of the local 
    computer, since rpc is not used at all in the first situation.
    It is also different when scheduling a task, since a locally
    bound scheduler behaves exactely the same as any other 
    scheduler created with createScheduler.

    @param sd remote scheduler descriptor
    @param hostname network name of the remote computer
    @param remoteprog rpc program number of remote scheduler
    @param remotever version number of remote scheduler
    @return a descriptor of the remote scheduler, NULL if failed
    @author DS, May 98
    @see Scheduler API
************************************************************************/
   scheduler_t* createBoundScheduler (scheduler_t* rsd, 
                     const char* remotehost,
                     unsigned int remoteprog, int remotever);


/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _GDS_SCHED_CLNT_H */
