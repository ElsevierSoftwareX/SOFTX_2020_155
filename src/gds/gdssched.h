/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched.h						*/
/*                                                         		*/
/* Module Description: Scheduler API	 				*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 13Apr98  D. Sigg    	First release		   		*/
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
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.5.1		*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			OK			*/
/*			  POSIX			OK (may use xdr)	*/
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

#ifndef _GDS_SCHED_H
#define _GDS_SCHED_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined (NO_XDR) && !defined (__EXTENSIONS__)
#define __EXTENSIONS__
#endif


/* Header File List: */
#include "dtt/gdsmain.h"
#include "dtt/gdsheartbeat.h"
#include "tconv.h"

#ifndef NO_XDR
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif

#ifdef OS_VXWORKS
#include <semLib.h>

#else
#include <pthread.h>
#endif


/** @name Scheduler API
    * A scheduler is a process which maintains an internal list of 
    tasks to be scheduled at a given time under certain conditions. 
    These task can be called synchronously (scheduler waits until they
    finish) or asynchronously (tasks are started as independent threads).
    A scheduler is synchronized with an external heartbeat through a
    semaphore. Everytime the semaphore is released the scheduler will
    go through its internal list and start executing all tasks which
    met all the scheduling conditions. Scheduling conditions are 
    evaluated in to stages. First, a general wait condition can be
    specified; waits can be relative to an absolute time point or
    relative to the termination of an other task. After the wait 
    condition is met an optional synchronisation condition can be
    used to synchronize the execution of a task with a heartbeat of
    a given epoch (or inverse epoch). A task can be scheduled only
    once or repeatedly. Tasks can be scheduled to repeat forever, for 
    a fixed number of times or until a condition specified by the
    return value of the task is met. The task repeat rate (or better
    the interval between sucessive tasks) can again be specified
    both as a wait period and an optional synchronization condition.

   
    @memo Schedules tasks based on the 16Hz system clock
    @author Written April 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/	


/** @name Constants and flags
    * Constants and flags of the Scheduler API.

    @memo Constants and flags
    @author DS, April 98
    @see Scheduler API
************************************************************************/

/*@{*/


/** Maximum length of a time tag, including the termination \0 - 
    16 characters.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define TIMETAG_LENGTH 17


/** Flag which specifies that the scheduled entry is called only
    once.

    @memo Flag values for scheduled tasks
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_SINGLE	0x00

/** Flag which specifies that the scheduled entry is called repeatedly.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_REPEAT	0x01

/** Flag which specifies that the scheduled entry will not wait for
    another one.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_NOWAIT	0x00

/** Flag which specifies that the scheduled entry is called to wait for
    another one.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT	0x02

/** Flag which specifies that the scheduled entry does not carry a 
    time tag.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_NOTIMETAG	0x00

/** Flag which specifies that the scheduled entry carries a time tag.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_TIMETAG	0x04

/** Flag which specifies that the scheduled entry can wait forever.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_NOTIMEOUT	0x00

/** Flag which specifies that the scheduled entry has a timeout.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_TIMEOUT	0x08

/** Flag which specifies that the scheduled entry will run at the 
    default priority.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_PRIORITY_DEFAULT	0x00

/** Flag which specifies that the scheduled entry will run at a user
    specified priority.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_PRIORITY_USER	0x10

/** Flag which specifies that the scheduled entry will not be
    accepted the estimated execution time lies more than an
    hour in the future.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_MAXTIME_SHORT	0x00

/** Flag which specifies that the scheduled entry can be scheduled
    at any possible time.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_MAXTIME_LONG	0x20

/** Flag which specifies that the scheduled entry will be called
    synchronously.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_SYNC		0x00

/** Flag which specifies that the scheduled entry will be called
    asynchronously.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_ASYNC		0x40

/** Flag which specifies that the tag of the scheduled entry should
    expire after the scheduled function is called for the first time.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_TAG_END		0

/** Flag which specifies that the tag of the scheduled entry should
    expire just prior the scheduled function is called for the first
    time.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_TAG_START		1	

/** Flag which specifies that the tag of the scheduled entry should
    expire after the scheduled entry is called for the last time.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_TAG_LAST		2

/** Flag which specifies that the scheduled entry should synchronize
    with the next coming epoch.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_SYNC_NEXT		0

/** Flag which specifies that the scheduled entry should synchronize
    with the specified epoch. Epoches are numbered from 0 to
    NUMBER_OF_EPOCHS - 1 with the epoch 0 starting at the second
    interval.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_SYNC_EPOCH	1

/** Flag which specifies that the scheduled entry should synchronize
    with the specified inverse epoch. An inverse epoch starts at
    the each TAI second which is a multiple of the specified value.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_SYNC_IEPOCH	2

/** Flag which specifies that the scheduled entry should not wait.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_IMMEDIATE	0

/** Flag which specifies that the scheduled entry should wait a minimum
    time delay specified in nsec.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_TIMEDELAY	1

/** Flag which specifies that the scheduled entry should wait a minimum
    time delay specified in epochs.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_EPOCHDELAY	2

/** Flag which specifies that the scheduled entry should wait until
    the specified time given as TAI (in nsec) has passed.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_STARTTIME	3

/** Flag which specifies that the scheduled entry should wait for a
    time tag to expire.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_TAG		4

/** Flag which specifies that the scheduled entry should wait for a
    time tag to expire plus an additional time delay (given in nsec).
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_TAGDELAYED	5

/** Flag which specifies that the scheduled entry should wait for a
    time tag to expire plus an additional time delay (given in epochs).
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_WAIT_TAGEPOCHDELAYED	6

/** Flag which specifies that the scheduled entry should be repeated
    N times.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_REPEAT_N		0

/** Flag which specifies that the scheduled entry should repeat forever.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_REPEAT_INFINITY	1

/** Flag which specifies that the scheduled entry should repeat until
    a condition is met. Meaning the scheduled function returns 1.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_REPEAT_COND	2

/** Flag which specifies that the repeat rate is specified in
    units of epochs.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_REPEAT_EPOCH	0

/** Flag which specifies that the repeat rate is specified in
    units of inverse epochs.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_REPEAT_IEPOCH	1

/** Flag which specifies that no scheduler process should be spawned
    when the scheduler is created. This option is only useful when 
    a setup routine is specified which does the job instead. This 
    option also prevents the allocation of memory for the task list
    and the time tag list.
  
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SCHED_NOPROC	0x01


/*@}*/


/** @name Data types
    * Data types of the Scheduler API.

    @memo Data types
    @author DS, April 98
    @see Scheduler API
************************************************************************/

/*@{*/


#ifndef OS_VXWORKS
/** Denotes a type representing a task id. A pthread_t on UNIX and int 
    on VxWorks.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
   typedef pthread_t schedTID_t;
#else
   typedef int       schedTID_t;
#endif


#ifndef OS_VXWORKS
/** Denotes a type representing a mutex. A pthread_mutex_t on UNIX and 
    an SEM_ID on VxWorks.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
   typedef pthread_mutex_t	schedmutex_t;
#else
   typedef SEM_ID		schedmutex_t;
#endif


/** Denotes a type representing a scheduler task. See 
    schedulertask_struct for more details.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
   typedef struct schedulertask_struct schedulertask_t;


/** Structure defining a scheduled task. The structure specifies
    a call-back function which is called whenever specified
    conditions are met. Functions can be called repeatedly or only once;
    they may wait for another event before they are called the first
    time or the may proceed immediatley. Scheduled functions can be
    synchronized with specified epochs and they can be called in order
    of priority. Scheduled entries may wait forever until their 
    condition is met or exit after a timeout has expired. They can
    run either synchronously or asynchronously.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
   struct schedulertask_struct
   {
      /** Flag of schedule entry. Can be one of the following
          (or'ed together):
      
          SCHED_SINGLE | SCHED_REPEAT : single shot (default) or 
          repeated call
      
          SCHED_WAIT | SCHED_NOWAIT : wait for an other event to 
          happen or not (default)
      
          SCHED_TIMETAG | SCHED_NOTIMETAG : determines whether the 
          current entry carries a time tag 
      
          SCHED_TIMEOUT | SCHED_NOTIMEOUT : determines whether the
          task should wait forever (default) or timeout after a
          certain time. 
   
          SCHED_PRIORITY_USER | SCHED_PRIORITY_DEFAULT : determines
          wether task are scheduled with the default priority or
          with a user specified priority level.
   
          SCHED_MAXTIME_LONG | SCHED_MAXTIME_SHORT : determines
          whether tasks can be scheduled long before they actually
          run. By default task which have an aboslute time or relative
          delay which points more than an hour into the future, will
          not be scheduled and the scheduleTask will return with an 
          error. Default is short. 
   
          SCHED_SYNC | SCHED_ASYNC : determines whtether the user
          function is called synchronously (default) or asynchronously. 
          When called asynchronously the function will have its own 
          task context and the scheduler will not wait unitl it is 
          terminates before scheduling other tasks. */
      int	flag;
      /** Task priority. Specifies the priority of the scheduled task */
      int	priority;
      /** Timeout of the entry in nsec. */
      tainsec_t	timeout;
      /** Type of time tag. Specifies when the tag is expired:
      
          SCHED_TAG_START | SCHED_TAG_END | SCHED_TAG_LAST : 
          specifiyes when the time tag corresponding to the current
          entry expires; just before the entry is scheduled, after it
          is scheduled for the first time (default), or after it is
          scheduled for the last time (only). */
      int	tagtype; 
      /** Name of time tag. NULL terminated string */
      char	timetag [TIMETAG_LENGTH];
      /** Synchronization flag. Describes how the scheduleded task gets 
          synchronized with the 16Hz system clock. It can be one of the 
          following:
      
          SCHED_SYNC_EPOCH | SCHED_SYNC_IEPOCH | SCHED_SYNC_NEXT : 
          synchronizes with the next epoch specified in syncval, the
          next inverse epoch specified in syncval, or just with the next 
          epoch (default) */
      int	synctype;
      /** Synchronization value. Spcifies the epoch or the inverse 
          epoch if synchtype is set accordingly. */
      int	syncval;
      /** Wait flag. Specifies what to wait for if scheduled task has
          to wait for an event. Can be one of the following:
      
          SCHED_WAIT_TAG | SCHED_WAIT_TAGDELAYED | 
          SCHED_WAIT_TAGEPOCHDELAYED | SCHED_WAIT_IMMEDIATE | 
          SCHED_WAIT_TIMEDELAY | SCHED_WAIT_EPOCHDELAY | 
          SCHED_WAIT_STARTTIME : 
          Wait periods can be specified relative to a time tag, 
          relative to the current time or absolute in time. 
          The wait period can be zero, or a time specified in units
          of nsec or epochs, respectively. Default is 
          SCHED_WAIT_IMMEDIATE. */
      int	waittype;
      /** Wait time. Depending on waittype either specifies a time
          delay in nsec or epochs; or specifies the aboslute
          start time as atomic time (TAI). */
      tainsec_t	waitval;
      /** Name of time tag to wait for. Only relevant if wait type
          is SCHED_WAIT_TAGxxx */
      char	waittag [TIMETAG_LENGTH];
      /** Specifies how often an entry is scheduled. Can be one
          of the following:
      
          SCHED_REPEAT_N | SCHED_REPEAT_INFINITY | SCHED_REPEAT_COND :
          repetas the number of times specified with repeatval,
          repeats infinitely (default), or repeats until a condition
          is met */
      int	repeattype;
      /** Number of repeats if repeattype is SCHED_REPEAT_N */
      int	repeatval;
      /** Specifies the units of the repeat rate. Can be one of 
          the following:
      
          SCHED_REPEAT_EPOCH | SCHED_REPEAT_IEPOCH : repeat rate is
          given in units of epochs (default) or inverse epochs. */
      int	repeatratetype;
      /** Repeat rate. Or better, the interval in between. */
      int	repeatrate;
      /** Synchronization flag for repeated entries. Describes how the 
          repeated scheduleded task gets  synchronized with the heartbeat.
          It can be one of the following:
      
          SCHED_SYNC_EPOCH | SCHED_SYNC_IEPOCH | SCHED_SYNC_NEXT : 
          synchronizes with the next epoch specified in syncval, the
          next inverse epoch specified in syncval, or just with the next 
          epoch (default). This flag is only used when the repeatratetype
          is equal to SCHED_REPEAT_IEPOCH */
      int	repeatsynctype;
      /** Repeat synchronization value. Specifies the epoch or the 
          inverse epoch if synchtype is set accordingly. */
      int	repeatsyncval;
      /** Optional argument. This pointer is passed to the schedules 
          task as an additional argument. */
      void*	arg;
      /** Scheduled task. When all the condiitons of the schduler
          entry are met, this function will be called synchronously
          or asynchronously, depending on the settings in flag. */
      int (*func) (schedulertask_t* info,
                  taisec_t time, int epoch, void* arg);
      /** Exit function. This routine is called to free memory 
          allocated for the task arguments and to do clean up work. 
          This function is called asynchronously and the scheduler will
          not wait for its return. This function has to be set to
          NULL if it is not used. */
      void (*freeResources) (taisec_t time, int epoch, void* arg);
      #ifndef NO_XDR
      /** XDR function. If this function is not NULL it is called 
          when the task is added to the scheduler to convert the argument 
          data into an external data representation; the it is called
          again to make a copy of the argument data. This function is
          called again to free the copy of the argument data when the 
          task retires. (freeResources must not be used to free the
          arguments in this case!) Since the argument is copied, the
          process which calls scheduleTask can free or reuse the memory 
          of the argument data immediately after the call returns.
          freeResources is called before xdr_arg.
          This function has to be set to NULL if it is not used. */
      xdrproc_t	xdr_arg;
      /** Size of argument. Whenever an XDR function is used, the
          length (in bytes) of the data structure pointed to by arg
          has to be specified. The sizeof function should be used to
          determine the size of this data structure. */
      size_t	arg_sizeof;
      #endif
   };


/** Scheduler function type.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   typedef int (*schedfunc_t) (schedulertask_t*, taisec_t, int, void*);


/** Denotes a type representing a scheduler. See scheduler_struct 
    for more details.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
   typedef struct scheduler_scruct scheduler_t;


/** Structure defining a scheduleder. The information in this structure
    must not be accessed directly.

    @author DS, April 98
    @see Scheduler API
************************************************************************/
   struct scheduler_scruct {
      /** Scheduler task list. Pointer to a list of pointers describing
          all scheduled tasks. Last pointer of list must be NULL.
          (private) */
      schedulertask_t** tasklist;
      /** size of task list (private) */
      int	maxtasklist;
      /** mutex for accessing scheduler resources (private) */
      schedmutex_t	sem;
      /** task ID of scheduler (private) */
      schedTID_t scheduler_tid;
      /** flags of scheduler */
      int	scheduler_flags;
      /** hearbeat rate (private) */
      int	heartbeatsPerSec;
      /** marked for termination (private) */
      int	markedForTermination;
      /** timetag list (private) */
      char**	timetaglist;
      /** size of tag list (private) */
      int	maxtimetaglist;
      /** data for extensions (private) */
      void*	data;
      /** synchronization function (private) */
      int	(*sync) (void);
      /** function which returns the current time (private) */
      tainsec_t	(*timenow) (void);
      /** close function (private) */
      int 	(*closeScheduler) (scheduler_t* sd, tainsec_t timeout);
      /** schedule function (private) */
      int 	(*scheduleTask) (scheduler_t* sd, 
                        const schedulertask_t* newtask);
      /** get function (private) */
      int 	(*getScheduledTask) (scheduler_t* sd, int id, 
                        schedulertask_t* task);
      /** remove function (private) */
      int 	(*removeScheduledTask) (scheduler_t* sd, int id, 
                        int terminate);
      /** wait function (private) */
      int 	(*waitForSchedulerToFinish) (scheduler_t* sd, 
                        tainsec_t timeout);
      /** tag function (private) */
      int 	(*setTagNotify) (scheduler_t* sd, const char* tag,
                        taisec_t tai, int epoch);
   };


/** Scheduler set tag notify function type.

    @author DS, May 98
    @see Scheduler API
************************************************************************/
   typedef int (*settagfunc_t) (scheduler_t*, const char*, taisec_t, int);


/*@}*/


/** @name Functions
    * Functions of the Scheduler API.

    @memo Functions
    @author DS, April 98
    @see Scheduler API
************************************************************************/

/*@{*/


/** Creates a new scheduler. Creates a new scheduler and starts a
    scheduler process which synchronizes with the system heartbeat.
    Returns the scheduler descriptor if successful, or NULL if failed.
    An optional function argument can be used to specify a setup 
    routine which is called after the descritor is initialized, but
    before the scheduler process is spawned. NULL means no setup
    routine. The setup routine has to return 0 if successfull.
    An optional data structure can be passed which will be attached
    to the data field of the scheduler. This data structure will be
    freed automatically when the scheduler closes.

    @param flags specfies the scheduler options
    @param setup function for doing additional setup
    @param data pointer to an optional user data structure
    @return pointer to the new scheduler
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   scheduler_t* createScheduler (int flags, 
                     int (*setup) (scheduler_t* sd), void* data);


/** Closes a scheduler. Removes all scheduled tasks from
    the task list and terminates any still running tasks after a
    certain timeout. A timeout of zero will terminate any running
    tasks immediately.

    Error codes:

	-1: invalid scheduler
	
	-2: timeout

    @param sd scheduler descriptor
    @param timeout maximum time to wait before terminating running
           tasks
    @return zero if successful, a negative error number otherwise
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   int closeScheduler (scheduler_t* sd, tainsec_t timeout);


/** Schedules a new task. It requires a pointer to a strcture describing
    the scheduling conditions and the entry point and arguements of the 
    call-back routine. Returns a (positive) id number for the scheduled 
    task if successful, and a negative number otherwise.

    Error codes:

	-1: invalid scheduler
	
	-2: scheduler busy
	
	-3: insufficient memory
	
	-4: unable to create mutex
	
	-10: illegal timeout value
	
	-11: timeout too long (may need SCHED_MAXTIME_LONG)
	
	-12: illegal priority value
	
	-13: illegal wait type
	
	-14: illegal wait value
	
	-15: wait too long (may need SCHED_MAXTIME_LONG)
	
	-16: illegal wait tag
	
	-17: illegal synchronization type
	
	-18: illegal synchronization value
	
	-19: illegal time tag
	
	-20: illegal time tag type
	
	-21: illegal repeat type
	
	-22: illegal repeat value
	
	-23; illegal repeat rate type
	
	-24: illegal repeat synchronization type
	
	-25: illegal repeat synchronization value
	
	-26: illeagl function reference

    @param sd scheduler descriptor
    @param newtask pointer to scheduled task information  
    @return id of scheduled task or a neagtive error number
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   int scheduleTask (scheduler_t* sd, const schedulertask_t* newtask);


/** Gets information about a scheduled task. It needs the scheduled
    task id and returns the next higher valid id, if successful.
    Returns a negative number, if failed. This function can be used
    to cycle through all scheduler entries staring with an id of -1. 
    The last call will then return the lowest valied id. No info
    structure will be returned if task is NULL.

    Error codes:

	-1: invalid scheduler
	
	-2: scheduler busy

	-5: entry not found
	
    @param sd scheduler descriptor
    @param id scheduled task number 
    @param task pointer to store the returned information  
    @return next higher valid id or a negative error number
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   int getScheduledTask (scheduler_t* sd, int id, schedulertask_t* task);


/** Removes a scheduled task from the scheduler list. Will terminate 
    already running tasks by brute force if terminate is 
    SCHED_NOWAIT, or will wait if SCHED_WAIT. An negative id will 
    remove all scheduled tasks at once. This routine will mark
    tasks for termination and send a cancel signal id terminate 
    id SCHED_NOWAIT, however it will not wait until all the clean
    up is done. When all tasks were deleted and one would like
    to wait until the cleanup is finished, one has to call
    waitForSchedulerToFinish. 

    Error codes:

	-1: invalid scheduler
	
	-2: scheduler busy
	
	-6: scheduler entry busy

    @param sd scheduler descriptor
    @param id scheduled task number or -1 for all
    @param terminate specifies how to terminate running tasks
    @return 0 if succesful, a negative error number otherwise
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   int removeScheduledTask (scheduler_t* sd, int id, int terminate);


/** Waits until all scheduled task have finished. A timeout can be
    specified to prevent this function from blocking forever. A 
    timeout value of -1 will wait forever. A timeout value of -2
    will wait until all tasks which should finish in a finite time
    have been scheduled and terminated. A timeout of zero will
    return immediately. A negative returned value inicates an error
    and can be used in conjunction with a timeout of zero to test
    whether the scheduler is empty.

    Error codes:
	
	-7: scheduler still active
	
    @param sd scheduler descriptor
    @param timeout maximum time to wait
    @return zero if succesful, a negative error number otherwise
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   int waitForSchedulerToFinish (scheduler_t* sd, tainsec_t timeout);


/** Sets the time tag. This function sets a time tag which can be used
    to synchronize scheduled tasks. This function is called whenever
    a scheduled task fulfills the condition which releases the time
    tag. If the specified time is zero the actual time will be
    used instead. The nonotify flag can be used when non-zero to 
    prevent the setTagNotify from beeing called automatically.

    Error codes:

	-1: invalid scheduler
	
	-2: scheduler busy
	
	-8: time tag invalid
	
    @param sd scheduler descriptor
    @param tag name of time tag
    @param time tag time
    @param nonotify if non-zero the notification routine is not called
    @return zero if successful, a negative error number otherwise
    @author DS, April 98
    @see Scheduler API
************************************************************************/
   int setSchedulerTag (scheduler_t* sd, const char* tag,
                     tainsec_t time, int nonotify);


#ifndef NO_XDR
/** Initializes a task info structure. This macro initializes all
    elements of a task info structure to a known initial value.
    It does the following

	info->flag = SCHED_SINGLE | SCHED_NOWAIT;
	
	info->timeout = 0;
	
	info->priority = 50;
	
	info->tagtype = SCHED_TAG_END;
	
	memset (info->timetag, 0, sizeof (info->timetag));
	
	info->synctype = SCHED_SYNC_NEXT;
	
	info->syncval = 0;
	
	info->waittype = SCHED_WAIT_IMMEDIATE;
	
	info->waitval = 0;
	
	memset (info->waittag, 0, sizeof (info->waittag));
	
	info->repeattype = SCHED_REPEAT_INFINITY;
	
	info->repeatval = 0;
	
	info->repeatratetype = SCHED_REPEAT_EPOCH;
	
	info->repeatrate = 1;
	
	info->repeatsynctype = SCHED_SYNC_NEXT;
	
	info->repeatsyncval = 0;
	
	info->arg = NULL;
	
	info->func = NULL;
	
	info->freeResources = NULL;
	
	info->xdr_arg = NULL;
			
    @param info pointer to task info structure
    @return void
    @author DS, April 98
    @see Scheduler API
************************************************************************/
#define SET_TASKINFO_ZERO(info) \
   { \
	(info)->flag = SCHED_SINGLE | SCHED_NOWAIT; \
	(info)->timeout = 0; \
	(info)->priority = 50; \
	(info)->tagtype = SCHED_TAG_END; \
	memset ((info)->timetag, 0, sizeof ((info)->timetag)); \
	(info)->synctype = SCHED_SYNC_NEXT; \
	(info)->syncval = 0; \
	(info)->waittype = SCHED_WAIT_IMMEDIATE; \
	(info)->waitval = 0; \
	memset ((info)->waittag, 0, sizeof ((info)->waittag)); \
	(info)->repeattype = SCHED_REPEAT_INFINITY; \
	(info)->repeatval = 0; \
	(info)->repeatratetype = SCHED_REPEAT_EPOCH; \
	(info)->repeatrate = 1; \
	(info)->repeatsynctype = SCHED_SYNC_NEXT; \
	(info)->repeatsyncval = 0; \
	(info)->arg = NULL; \
	(info)->func = NULL; \
	(info)->freeResources = NULL; \
	(info)->xdr_arg = NULL; \
   }

#else

#define SET_TASKINFO_ZERO(info) \
   { \
	(info)->flag = SCHED_SINGLE | SCHED_NOWAIT; \
	(info)->timeout = 0; \
	(info)->priority = 50; \
	(info)->tagtype = SCHED_TAG_END; \
	memset ((info)->timetag, 0, sizeof ((info)->timetag)); \
	(info)->synctype = SCHED_SYNC_NEXT; \
	(info)->syncval = 0; \
	(info)->waittype = SCHED_WAIT_IMMEDIATE; \
	(info)->waitval = 0; \
	memset ((info)->waittag, 0, sizeof ((info)->waittag)); \
	(info)->repeattype = SCHED_REPEAT_INFINITY; \
	(info)->repeatval = 0; \
	(info)->repeatratetype = SCHED_REPEAT_EPOCH; \
	(info)->repeatrate = 1; \
	(info)->repeatsynctype = SCHED_SYNC_NEXT; \
	(info)->repeatsyncval = 0; \
	(info)->arg = NULL; \
	(info)->func = NULL; \
	(info)->freeResources = NULL; \
   }

#endif

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _GDS_SCHED_H */
