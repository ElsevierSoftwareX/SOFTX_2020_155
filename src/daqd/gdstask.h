/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdstask.h						*/
/*                                                         		*/
/* Module Description: task and sempahore routines 			*/
/* Used to fix include file discrepancies between UNIX and VxWorks	*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 1.0	 9Jul98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdstask.html						*/
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
/*			  POSIX			OK (for UNIX)		*/
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
#ifndef _GDS_TASK_H
#define _GDS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OS_VXWORKS
#include <semLib.h>
#include <taskLib.h>

#else
#include <pthread.h>
#endif

/** @name Task and Semaphore API
    This library hides the difference between the multi-threading
    environment of UNIX and VxWorks. It exports types, macros and
    functions which will map into POSIX 1.c calls on UNIX and into
    native real-time functions on VxWorks.

    @memo Routines for handling semaphores, mutex and tasks
    @author Written July 1998 by Daniel Sigg
    @version 1.0
************************************************************************/

/*@{*/

/** @name Constants and flags
    Constants and flags of the Task and Semaphore API.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/

/*@{*/

/*@}*/

/** @name Data types
    Data types of the Task and Semaphore API.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/

/*@{*/

#ifndef OS_VXWORKS
/** Denotes a type representing a task id. A pthread_t on UNIX and int
    on VxWorks.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
typedef pthread_t taskID_t;
#else
typedef int    taskID_t;
#endif

#ifndef OS_VXWORKS
/** Denotes a type representing a mutex. A pthread_mutex_t on UNIX and
    an SEM_ID on VxWorks.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
typedef pthread_mutex_t mutexID_t;
#else
typedef SEM_ID mutexID_t;
#endif

#ifndef OS_VXWORKS
/** Denotes a type representing the return argument of a thread. A
    (void*) pointer on UNIX and (void) on VxWorks.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
typedef void* taskretarg_t;
#else
typedef void   taskretarg_t;
#endif

/** Denotes a type representing the argument of a thread. A (void*)
    pointer on both UNIX and VxWorks.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
typedef void* taskarg_t;

/** Denotes a type representing a function prototype of a thread.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
typedef taskretarg_t ( *taskfunc_t )( taskarg_t );

/*@}*/

/** @name Macros
    Macros of the Task and Semaphore API.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/

/*@{*/

#ifndef OS_VXWORKS
/** Denotes a macro to create a mutex. pthread_mutex_init on UNIX and
    semMCreate on VxWorks.

    @param sem mutex variable
    @return 0 if successfull, non-zero otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
#define MUTEX_CREATE( sem ) pthread_mutex_init( &( sem ), NULL )
#else
#define MUTEX_CREATE( sem )                                                    \
    ( ( ( sem = semMCreate( SEM_Q_PRIORITY | SEM_INVERSION_SAFE |              \
                            SEM_DELETE_SAFE ) ) == NULL )                      \
          ? 1                                                                  \
          : 0 )
#endif

#ifndef OS_VXWORKS
/** Denotes a macro to get a mutex. pthread_mutex_lock on UNIX and
    semTake (sem, WAIT_FOREVER) on VxWorks.

    @param sem mutex variable
    @return 0 if successfull, non-zero otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
#define MUTEX_GET( sem ) pthread_mutex_lock( &( sem ) )
#else
#define MUTEX_GET( sem ) semTake( sem, WAIT_FOREVER )
#endif

#ifndef OS_VXWORKS
/** Denotes a macro which tries to get a mutex. pthread_mutex_trylock on
    UNIX and semTake (sem, NO_WAIT) on VxWorks.

    @param sem mutex variable
    @return 0 if successfull, non-zero otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
#define MUTEX_TRY( sem ) pthread_mutex_trylock( &( sem ) )
#else
#define MUTEX_TRY( sem ) semTake( sem, NO_WAIT )
#endif

#ifndef OS_VXWORKS
/** Denotes a macro to release a mutex. pthread_mutex_unlock on
    UNIX and semGive on VxWorks.

    @param sem mutex variable
    @return 0 if successfull, non-zero otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
#define MUTEX_RELEASE( sem ) pthread_mutex_unlock( &( sem ) )
#else
#define MUTEX_RELEASE( sem ) semGive( sem )
#endif

#ifndef OS_VXWORKS
/** Denotes a macro to destroy a mutex. pthread_mutex_destroy on
    UNIX and semDelete on VxWorks.

    @param sem mutex variable
    @return 0 if successfull, non-zero otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
#define MUTEX_DESTROY( sem ) pthread_mutex_destroy( &( sem ) )
#else
#define MUTEX_DESTROY( sem ) semDelete( sem )
#endif

/*@}*/

/** @name Functions
    Functions of the Task and Semaphore API.

    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/

/*@{*/

/** Creates a task (VxWorks) or a thread (UNIX). This function uses
    either taskSpawn or pthread_create, respectively. The function
    accepts the task/thread attributes, the task priority, the task
    function and a pointer to the task argument. It returns a status
    flag and the task ID if successful. The task attributes are
    specific to the environment (see manual page of taskSpawn and
    pthread_create, respectively).

    @param attr task creation attributes
    @param priority task priority
    @param taskID pointer to a task ID (return argument)
    @param taskname name of task (can be NULL)
    @param task function pointer to task/thread
    @param arg pointer to task arguments
    @return 0 if successfull, negative otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
int taskCreate( int         attr,
                int         priority,
                taskID_t*   taskID,
                const char* taskname,
                taskfunc_t  task,
                taskarg_t   arg );

/** Cancels a task (VxWorks) or a thread (UNIX). This function
    terminates the task/thread and sets the taskID to zero. A
    task ID of zero is ignored (no suicide!).

    @param taskID pointer to a task ID (return argument)
    @return 0 if successfull, negative otherwise
    @author DS, July 98
    @see Task and Semaphore API
************************************************************************/
int taskCancel( taskID_t* taskID );

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _GDS_TASK_H */
