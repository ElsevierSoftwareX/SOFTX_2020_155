static char *versionId = "Version $Id$" ;
/* #define PORTMAP */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>


#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <signal.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdsheartbeat.h"
#include "dtt/gdssched_util.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _NETID		  net protocol used for rpc		*/
/*            _DEFAULT_XDR_SIZE   max. length of an XDR'd task arg	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _NETID			"tcp"
#define _DEFAULT_XDR_SIZE	100000	/* 100kByte */


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: threadSpawn					*/
/*                                                         		*/
/* Procedure Description: spawns a new task				*/
/*                                                         		*/
/* Procedure Arguments: attr - 	thread attr. detached/process (UNIX);	*/
/*				all task attr. (VxWorks)		*/
/* 			priority - thread/task priority			*/
/* 			taskIF - pointer to TID (return value)		*/
/* 			task - thread/task function			*/
/* 			arg - argument passed to the task		*/
/*                                                         		*/
/* Procedure Returns: 0 if succesful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int _threadSpawn (int attr, int priority, schedTID_t* taskID, 
                    _schedtask_t task, _schedarg_t arg)
   {
   #ifdef OS_VXWORKS
      /* VxWorks task */
      *taskID = taskSpawn ("trpcSched", priority, attr, 10000, (FUNCPTR) task, 
                              (int) arg, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      if (*taskID == ERROR) {
         return -1;
      }
   #else
   
      /* POSIX task */
      {
         pthread_attr_t		tattr;
         struct sched_param	schedprm;
         int			status;
      
      	/* set thread parameters: joinable & system scope */
         if (pthread_attr_init (&tattr) != 0) {
            return -1;
         }
         pthread_attr_setdetachstate (&tattr, 
                              attr & PTHREAD_CREATE_DETACHED);
         pthread_attr_setscope (&tattr, attr & PTHREAD_SCOPE_SYSTEM);
      	 /* set priority */
         pthread_attr_getschedparam (&tattr, &schedprm);
         schedprm.sched_priority = priority;
         pthread_attr_setschedparam (&tattr, &schedprm);
      
         /* create thread */
         status = pthread_create (taskID, &tattr, task, (void*) arg);
         pthread_attr_destroy (&tattr);
         if (status != 0) {
            return -1;
         }
      }
   #endif
      return 0;
   }

