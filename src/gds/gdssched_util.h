/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched_util.h						*/
/*                                                         		*/
/* Module Description: Scheduler Client/Server Utilities		*/
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

#ifndef _GDS_SCHED_UTIL_H
#define _GDS_SCHED_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined (__EXTENSIONS__)
#define __EXTENSIONS__
#endif


/* Header File List: */
#include "dtt/gdssched.h"
#include "dtt/gdsrsched.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _schedproc_t	return argument of a thread			*/
/*        _schedarg_t   argument pointer passed to task/thread function	*/
/*        _schedtask_t  prototype for task/thread function		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
   typedef void 	_schedproc_t;
#else
   typedef void*	_schedproc_t;
#endif

   typedef void* _schedarg_t;
   typedef _schedproc_t (*_schedtask_t) (_schedarg_t);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: _direction_t	direction of in use count change		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   enum _direction_enum {INCREASE, DECREASE, GET, RELEASE};
   typedef enum _direction_enum _direction_t;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Macros: SCHEDMUTEX_CREATE	create a mutex	 			*/
/* 	   SCHEDMUTEX_GET	get a mutex (lock, take)		*/
/* 	   SCHEDMUTEX_RELEASE	release a mutex (unlock, give)		*/
/* 	   SCHEDMUTEX_DESTROY	destroy a mutex				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS

#ifndef EDEADLK
#define EDEADLK 45
#endif

/*
#define _SCHEDMUTEX_CREATE(sem) \
	(sem = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE | \
			   SEM_DELETE_SAFE)) */
	
#define _SCHEDMUTEX_CREATE(sem) \
	(((sem = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE | \
			   SEM_DELETE_SAFE)) == NULL) ? 1 : 0)
	
#define _SCHEDMUTEX_GET(sem) \
	semTake (sem, WAIT_FOREVER)
	
#define _SCHEDMUTEX_RELEASE(sem) \
	semGive (sem)
	
#define _SCHEDMUTEX_DESTROY(sem) \
	semDelete (sem)

#define _SCHEDMUTEX_TRY(sem) \
	semTake (sem, NO_WAIT)

#else

#define _SCHEDMUTEX_CREATE(sem) \
	pthread_mutex_init (&(sem), NULL)

#define _SCHEDMUTEX_GET(sem) \
	pthread_mutex_lock (&(sem))

#define _SCHEDMUTEX_RELEASE(sem) \
	pthread_mutex_unlock (&(sem))

#define _SCHEDMUTEX_DESTROY(sem) \
	pthread_mutex_destroy (&(sem))

#define _SCHEDMUTEX_TRY(sem) \
	pthread_mutex_trylock (&(sem))

#endif



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Functions: _threadSpawn	spawns a thread				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int _threadSpawn (int attr, int priority, schedTID_t* taskID, 
                    _schedtask_t task, _schedarg_t arg);

#ifdef __cplusplus
}
#endif

#endif /* _GDS_SCHED_UTIL_H */
