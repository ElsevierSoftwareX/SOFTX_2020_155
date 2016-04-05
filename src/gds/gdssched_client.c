static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched_client						*/
/*                                                         		*/
/* Module Description: implements functions for scheduling tasks	*/
/* on remote computers							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

/* #define PORTMAP */

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <taskVarLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <signal.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdsheartbeat.h"
#include "dtt/gdssched.h"
#include "dtt/gdssched_client.h"
#include "dtt/gdsrsched.h"
#include "dtt/gdsxdr_util.h"
#include "dtt/gdssched_util.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _INIT_RLIST 	  length of remote sched list at start	*/
/*            _DEFAULT_XDR_SIZE   max. length of an XDR'd task arg	*/
/*            _CALLBACK_PRIORITY  priority of the callback process	*/
/*            _CALLBACK_VERSION	  version of the rpc callback 		*/
/*            _NETID		  net protocol used for rpc		*/
/*            _INIT_SLEEP	  when waiting for rpc init		*/
/*            _WAIT_SLEEP	  when waiting for sched to finish	*/
/*            _SEM_SLEEP	  when waiting for semaphore		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _INIT_RLIST		100
#define _DEFAULT_XDR_SIZE	100000	/* 100kByte */
#ifdef OS_VXWORKS
#define _CALLBACK_PRIORITY	95
#else
#define _CALLBACK_PRIORITY	19
#endif
#define _CALLBACK_VERSION	1
#define _NETID			"tcp"
#define _INIT_SLEEP		1000000UL	/*   1 ms */
#define _WAIT_SLEEP		10000000UL	/*  10 ms */
#define _SEM_SLEEP		100000000UL	/* 100 ms */


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: initCallback	whether calback client was already init.*/
/*          callbackSem		mutex to protect callback globals 	*/
/*          callbackProgNum	rpc callback program number	 	*/
/*          callbackVersNum	rpc callback version number		*/
/*          callbackTransport	rpc transport handle for callback	*/
/*          callbackTID		TID of callback service task		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int			initCallback = 0;
#ifdef OS_VXWORKS
   static schedmutex_t		callbackSem = NULL;
#else
   static schedmutex_t		callbackSem = PTHREAD_MUTEX_INITIALIZER;
#endif
   static int			callbackInUse = 0;
   static unsigned long		callbackProgNum = 0;
   static unsigned long		callbackVersNum = 0;
   static SVCXPRT*		callbackTransport = NULL;
   static schedTID_t		callbackTID;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	remoteSetup		setup for remote scheduler		*/
/*	remoteSetup_b		setup for bound scheduler		*/
/*      initCallback		intializes callback server		*/
/*      registerCallback	registers a callback rpc interface	*/
/*      svcProcess		task which creates scheduler and waits	*/
/*      			rpc callbacks				*/
/*      dataUsage		in/decreases get/release inUse count	*/
/*      								*/
/*----------------------------------------------------------------------*/
   extern void gdsschedulercallback_1 (struct svc_req *rqstp, 
                     register SVCXPRT *transp);
   static int _remoteSetup (scheduler_t* sd);
   static int _remoteSetup_b (scheduler_t* sd);
   static int initCallbackSVC (unsigned int cbver);
   static _schedproc_t svcProcess (void);
   static void _dataUsage (rscheduler_info_t* data, _direction_t dir);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: createRemoteScheduler			*/
/*                                                         		*/
/* Procedure Description: creates a remote scheduler and initializes it	*/
/*                                                         		*/
/* Procedure Arguments: flags, setup function				*/
/*                                                         		*/
/* Procedure Returns: scheduler_t if successful, NULL when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   scheduler_t* createRemoteScheduler (int flags)
   {
      scheduler_t*		sd;	/* new scheduler */
      rscheduler_info_t*	data;	/* rsched info */
   
      /* test if mutex is initialized */
   #ifdef OS_VXWORKS
      if (callbackSem == NULL) {
         if (_SCHEDMUTEX_CREATE (callbackSem) != 0) {
            return NULL;
         }
      }
   #endif
   
      /* allocate extension data structure: 
         will be initialized with the remoteSetup routine */
      data = malloc (sizeof (rscheduler_info_t));
      if (data == NULL) {
         return NULL;
      }
   
      /* now create the scheduler */
      sd = createScheduler (flags | SCHED_REMOTE | SCHED_REMOTE_LOCAL |
                           SCHED_NOPROC, _remoteSetup, data);
      if (sd == NULL) {
         free (data);
         return NULL;
      }
   
      /* test if callback was already initialized */
      _SCHEDMUTEX_GET (callbackSem);
      if (initCallback == 0) {
         if (initCallbackSVC (_CALLBACK_VERSION) != 0) {
            closeScheduler (sd, 0);
            return NULL;
         }
      }
   
      /* increase callback in use count */
      callbackInUse++;
      _SCHEDMUTEX_RELEASE (callbackSem);
   
      return sd;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: createBoundScheduler			*/
/*                                                         		*/
/* Procedure Description: creates a bound scheduler and initializes it	*/
/*                                                         		*/
/* Procedure Arguments: rsd - remote scheduler, remotehost DNS/IP name	*/
/*                      remoteprog - rpc prog #, remotever - rpc ver #	*/
/*                                                         		*/
/* Procedure Returns: scheduler_t if successful, NULL when failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   scheduler_t* createBoundScheduler (scheduler_t* rsd, 
                     const char* remotehost,
                     unsigned int remoteprog, int remotever)
   {
      scheduler_t*		sd; 	/* bound scheduler descriptor */ 
      rscheduler_info_t*	data;	/* bound sched info */
      scheduler_r		cbd;	/* callback sched. descr. */
      int			flags;	/* bound sched flags */
      enum clnt_stat 		retval;	/* return value of rpc call */
      remotesched_r		rsched;	/* return arg of rpc call */
   
      /* allocate extension data structure: 
         will be initialized with the remoteSetup routine */
      data = malloc (sizeof (rscheduler_info_t));
      if (data == NULL) {
         return NULL;
      }
      /* copy remote parameters into this data */
      data->remotenum = remoteprog;
      data->remotever = remotever;
      data->parent = rsd;
   
      /* get remote host address */
      if (rpcGetHostaddress (remotehost, &data->remotehost) != 0) {
         free (data);
         return NULL;
      }
   
      /* determine flags for bound scheduler */
      if (data->remotehost.s_addr == inet_addr ("127.0.0.1")) {
         flags = SCHED_LOCAL_BOUND;
      }
      else {
         flags = SCHED_REMOTE | SCHED_REMOTE_BOUND | SCHED_NOPROC;
      }
   
      /* now create the bound scheduler */
      sd = createScheduler (flags, _remoteSetup_b, data);
      if (sd == NULL) {
         free (data);
         return NULL;
      }
   
      /* need to create scheduler on remote CPU */
      if ((flags & SCHED_LOCAL_BOUND) == SCHED_REMOTE_BOUND) {
         memcpy (&cbd, &data->parent, sizeof (scheduler_t*));
      
         retval = 
            connectscheduler_1 (cbd, callbackProgNum, 
                              callbackVersNum, &rsched, data->clnt);
         if ((retval != RPC_SUCCESS) || (rsched.status != 0)) {
            closeScheduler (sd, 0);
            return NULL;
         }
         data->rd = rsched.sd;
      }
   
      return sd;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _closeScheduler				*/
/*                                                         		*/
/* Procedure Description: remote scheduler close function		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, timeout			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _closeScheduler (scheduler_t* sd, tainsec_t timeout)
   {
      rscheduler_info_t*	data;	/* rsched info */
      scheduler_t*		bsd;	/* bound scheduler */
      tainsec_t		timeleft;	/* time left from timeout */
      tainsec_t		start;		/* start time */
      int		retval;		/* return value of r. close */ 
      int		result;		/* err number if failed */
   
      if (sd == NULL) {
         return 0;
      }
   
      /* set result to success */
      data = (rscheduler_info_t*) sd->data;
      result = 0;
   
      /* close all bound remote schedulers: don't get mutex yet! */
      start = sd->timenow();
      timeleft = timeout;
   
      /* Remember: their close function will remove themselves 
         from rlist! */
      while (data->rlist[0] != NULL) {
         bsd = data->rlist[0]->self;
         retval = closeScheduler (bsd, timeleft);
      
         if (result == 0) {
            result = retval;
         }
         /* adjust time left of timeout */
         if (timeleft > 0) {
            timeleft = timeout - (sd->timenow() - start);
            if (timeleft < 0) {
               timeleft = 0;
            }
         }
      }
   
    /* decrease callback in use count */
      _SCHEDMUTEX_GET (callbackSem);
      callbackInUse--;
      if (callbackInUse == 0) {
         /* terminate rpc callback service */
         if (callbackTID != 0) {
         #ifdef OS_VXWORKS
         if (kill (callbackTID, SIGQUIT) != 0) {
            taskDelete (callbackTID);
         }
         #else
            {
               extern void svc_exit (void);
               svc_exit ();
            /* pthread_cancel (data->callback_tid); */
            /* wait for termination and free thread resources */
               pthread_join (callbackTID, NULL);
            /* svc_destroy (callbackTransport); */
            }
            #endif
         }
         initCallback = 0;
      }
      _SCHEDMUTEX_RELEASE (callbackSem);
   
      /* call old close function; do not free data */
      sd->data = NULL;
      data->closeScheduler (sd, 0);
   
       /* free private memory, destroy mutex */
      free (data->rlist);
      _SCHEDMUTEX_DESTROY (data->sem);
      free (data);
   
      return result;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _closeScheduler_b				*/
/*                                                         		*/
/* Procedure Description: bound scheduler close function		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, timeout			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _closeScheduler_b (scheduler_t* sd, tainsec_t timeout)
   {
      rscheduler_info_t*	data;	/* bound sched info */
      rscheduler_info_t*	pdata;	/* remote sched info */
      int			i;	/* index into rlist */
      int			result; /* return value from rsched */
   
      if (sd == NULL) {
         return 0;
      }
   
      /* get the mutex */
      data = (rscheduler_info_t*) sd->data;
      pdata = (rscheduler_info_t*) data->parent->data;
      _dataUsage (data, GET);
   
      /* for remotely bound schedulers... */
      if ((data->clnt != NULL) && ((sd->scheduler_flags & SCHED_LOCAL_BOUND) == 
         SCHED_REMOTE_BOUND)) {
         /* call remote scheduler close function */
         if (closescheduler_1 (data->rd, timeout, &result, data->clnt) !=
            RPC_SUCCESS) {
            gdsError (GDS_ERR_PROG, "unable to close remote scheduler");
            result = -52;
         }
      	/* destroy rpc client handle for remotely bound schedulers */
         clnt_destroy (data->clnt);
      }
      /* remove data info from (parent) remote scheduler */
      _dataUsage (pdata, GET);
      for (i = 0; i < pdata->maxrlist; i++) {
         if (pdata->rlist[i] == data) {
            break;
         }
      }
      if (pdata->rlist[i] == data) {
         pdata->rlist[i] = NULL;
         for (i = i + 1; i < pdata->maxrlist; i++) {
            pdata->rlist[i-1] = pdata->rlist[i];
            pdata->rlist[i] = NULL;
         }
      }    
   
      _dataUsage (pdata, RELEASE);
   
      /* destroy bound scheduler mutex */
      if (_SCHEDMUTEX_DESTROY (data->sem) != 0) {
         if (result == 0) {
            result = -53;
         }
         gdsError (GDS_ERR_PROG,
                  "Failure to release bound scheduler data mutex");
      }
   
      /* call old close function, do not frees data */
      sd->data = NULL;
      if (data->closeScheduler (sd, 0) != 0) {
         if (result == 0) {
            result = -54;
         }
         gdsError (GDS_ERR_PROG, 
                  "unable to close local stub of remote scheduler");
      }
      free (data);
   
      return result;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _scheduleTask				*/
/*                                                         		*/
/* Procedure Description: remote scheduler schedule function		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, newtask			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _scheduleTask (scheduler_t* sd, 
                     const schedulertask_t* newtask)
   {
      rscheduler_info_t*	data;	/* rsched info */
      scheduler_t*		bsd;	/* bound scheduler */
      int		indx;		/* index into rlist */
      int		err;		/* error number */
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return -1 ;
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* schedule task to list of bound remote schedulers */
      err= 0;
      data = (rscheduler_info_t*) sd->data;
      for (indx = 0; (indx < data->maxrlist) && 
          (data->rlist[indx] != NULL); indx++) {
         bsd = data->rlist[indx]->self;
         err = scheduleTask (bsd, newtask);
         if (err != 0) {
            break;
         }
      }
   
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      return err;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _scheduleTask_b				*/
/*                                                         		*/
/* Procedure Description: remotely bound scheduler schedule function	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, newtask			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _scheduleTask_b (scheduler_t* sd, 
                     const schedulertask_t* newtask)
   {
      rscheduler_info_t*	data;	/* rsched info */
      enum clnt_stat 		retval;	/* return value of rpc call */
      int			result;	/* result */
      schedulertask_r		rtask;	/* remote task info */
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return -1 ;
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* set result to zero */
      result = 0;
   
      /* copy new task info into remote task info */
      rtask.flag = newtask->flag;
      rtask.priority = newtask->priority;
      rtask.tagtype = newtask->tagtype;
      rtask.synctype = newtask->synctype;
      rtask.syncval = newtask->syncval;
      rtask.waittype = newtask->waittype;
      rtask.repeattype = newtask->repeattype;
      rtask.repeatval = newtask->repeatval;
      rtask.repeatratetype = newtask->repeatratetype;
      rtask.repeatrate = newtask->repeatrate;
      rtask.repeatsynctype = newtask->repeatsynctype;
      rtask.repeatsyncval = newtask->repeatsyncval;
      rtask.func = newtask->func;
      rtask.timeout = newtask->timeout;
      rtask.waitval = newtask->waitval;
      strncpy (rtask.timetag, newtask->timetag, RTIMETAG_LENGTH);
      rtask.timetag[RTIMETAG_LENGTH-1] =0;
      strncpy (rtask.waittag, newtask->waittag, RTIMETAG_LENGTH);
      rtask.waittag[RTIMETAG_LENGTH-1] =0;
      rtask.arg.arg_val = NULL;
      rtask.arg_sizeof = newtask->arg_sizeof;
   
      result = xdr_encodeArgument (newtask->arg, &rtask.arg.arg_val, 
                                 &rtask.arg.arg_len, newtask->xdr_arg);
   
      /* use rpc to call schedule function on remote system */
      if (result == 0) {
         retval = scheduletask_1 (data->rd, rtask, &result, data->clnt);
         if (retval != RPC_SUCCESS) {
            result = -55;
         }
      }
   
      if (rtask.arg.arg_val != NULL) {
         free (rtask.arg.arg_val);
      }
   
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      return result;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _getScheduledTask				*/
/*                                                         		*/
/* Procedure Description: get remote scheduler task info	 	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, id, task info pointer	*/
/*                                                         		*/
/* Procedure Returns: -1 fails always					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _getScheduledTask (scheduler_t* sd, int id, 
                     schedulertask_t* task)
   {
      return -1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _getScheduledTask_b				*/
/*                                                         		*/
/* Procedure Description: get remotely bound scheduler task info 	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, id, task info pointer	*/
/*                                                         		*/
/* Procedure Returns: 0 if success, <0 if failed, task info		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _getScheduledTask_b (scheduler_t* sd, int id, 
                     schedulertask_t* task)
   {
      rscheduler_info_t*	data;	/* rsched info */
      enum clnt_stat 		retval;	/* return value of rpc call */
      resultGetScheduledTask_r	result;	/* result */
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return -1 ;
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* rpc: call scheduler on remote machine */
      retval = getscheduledtask_1 (data->rd, id, &result, data->clnt);
      if (retval != RPC_SUCCESS) {
         _dataUsage (data, DECREASE);
         return -56;
      }
   
      if (result.status < 0) {
         _dataUsage (data, DECREASE);
         return result.status;
      }
   
      /* copy result data */
      task->flag = result.task.flag;
      task->priority = result.task.priority;
      task->timeout = result.task.timeout;
      task->tagtype = result.task.tagtype;
      task->synctype = result.task.synctype;
      task->syncval = result.task.syncval;
      task->waittype = result.task.waittype;
      task->waitval = result.task.waitval;
      task->repeattype = result.task.repeattype;
      task->repeatval = result.task.repeatval;
      task->repeatratetype = result.task.repeatratetype;
      task->repeatval = result.task.repeatval;
      task->repeatratetype = result.task.repeatratetype;
      task->repeatrate = result.task.repeatrate;
      task->repeatsynctype = result.task.repeatsynctype;
      task->repeatsyncval = result.task.repeatsyncval;
      task->arg = NULL;		/* no argument passed back for now */
      memcpy (&task->arg, result.task.arg.arg_val, sizeof (void*));
      task->arg_sizeof = 0;
      task->func = (schedfunc_t) result.task.func;
      task->freeResources = NULL;
      task->xdr_arg = NULL;
      strncpy (task->timetag, result.task.timetag, TIMETAG_LENGTH);
      task->timetag[TIMETAG_LENGTH-1] = 0;
      strncpy (task->waittag, result.task.waittag, TIMETAG_LENGTH);
      task->waittag[TIMETAG_LENGTH-1] = 0;
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      return result.status;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _removeScheduledTask			*/
/*                                                         		*/
/* Procedure Description: remote scheduler remove function		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, id, terminate		*/
/*                                                         		*/
/* Procedure Returns: 0 if found, <0 otherwise				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _removeScheduledTask (scheduler_t* sd, int id, 
                     int terminate)
   {
      rscheduler_info_t*	data;	/* rsched info */
      scheduler_t*		bsd;	/* bound scheduler */
      int		indx;		/* index into rlist */
      int		result;		/* error number */
      int		retval;		/* return value */
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return -1 ;
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* close remote schedulers */
      result = -1;
      for (indx = 0; (indx < data->maxrlist) && 
          (data->rlist[indx] != NULL); indx++) {
         bsd = data->rlist[indx]->self;
         retval = removeScheduledTask (bsd, id, terminate);
         if (result != 0) {
            result = retval;
         }
      }
   
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      return result;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _removeScheduledTask_b			*/
/*                                                         		*/
/* Procedure Description: remotely bound scheduler remove function	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, id, terminate		*/
/*                                                         		*/
/* Procedure Returns: 0 if found, <0 otherwise				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _removeScheduledTask_b (scheduler_t* sd, int id, 
                     int terminate)
   {
      rscheduler_info_t*	data;	/* rsched info */
      enum clnt_stat 		retval;	/* return value of rpc call */
      int			result;	/* result */
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return -1 ;
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* use rpc to call remove function on remote system */
      retval = removescheduledtask_1 (data->rd, id, terminate, 
                                 &result, data->clnt);
   
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      if (retval != RPC_SUCCESS) {
         return -57;
      }
      else {
         return result;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _waitForSchedulerToFinish			*/
/*                                                         		*/
/* Procedure Description: remote scheduler wait function		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, timeout			*/
/*                                                         		*/
/* Procedure Returns: zero if succesful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _waitForSchedulerToFinish (scheduler_t* sd, 
                     tainsec_t timeout)
   {
      rscheduler_info_t*	data;	/* rsched info */
      scheduler_t*		bsd;	/* bound scheduler */
      int		indx;		/* index into rlist */
      int		result;		/* error number */
      tainsec_t		start = 0;	/* start time */
      struct timespec	tick = {0, _WAIT_SLEEP};
   
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return 0; /* this one is finished */
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* get start time */
      if (timeout >= 0) {
         start = sd->timenow();
      }
   
      /* loop while timeout is not expired */
      result = -1;
      do {
         /* go thorugh list of bound schedulers and 
            call their wait function */
         for (indx = 0; (indx < data->maxrlist) && 
             (data->rlist[indx] != NULL); indx++) {
            bsd = data->rlist[indx]->self;
            if (waitForSchedulerToFinish (bsd, 0) != 0) 
            {
               break;
            }
         }
      
      	 /* check wheter all bound schedulers have finished */
         if (indx >= data->maxrlist) {
            result = 0;
            break;
         }
      
      	/* go to sleep for a little while */
         nanosleep (&tick, NULL);
      
      } while ((timeout < 0) || (start + timeout > sd->timenow()));
   
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      return result;
   
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _waitForSchedulerToFinish_b			*/
/*                                                         		*/
/* Procedure Description: remotely bound scheduler wait function	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, timeout			*/
/*                                                         		*/
/* Procedure Returns: zero if succesful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _waitForSchedulerToFinish_b (scheduler_t* sd, 
                     tainsec_t timeout)
   {
      rscheduler_info_t*	data;	/* rsched info */
      enum clnt_stat 		retval;	/* return value of rpc call */
      int			result;	/* result */
   
      if ((sd == NULL) || (sd->data == NULL)) {
         return 0; /* this one is finished */
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* use rpc to call wait function on remote system */
      retval = waitforschedulertofinish_1 (data->rd, timeout, 
                                 &result, data->clnt);
   
      /* decrease the usage count */
      _dataUsage (data, DECREASE);
   
      if (retval != RPC_SUCCESS) {
         return -58;
      }
      else {
         return result;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setTagNotify				*/
/*                                                         		*/
/* Procedure Description: tag notify for remote scheduler		*/
/* (internal caller should own data mutex and set callback_orig)	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, tag, time, epoch		*/
/*                                                         		*/
/* Procedure Returns: -1 						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int _setTagNotify (scheduler_t* sd, const char* tag,
                     taisec_t tai, int epoch)
   {
      rscheduler_info_t*	data;	/* rsched info */
      scheduler_t*		bsd;	/* bound scheduler */
      int			indx;	/* index into rlist */
      tainsec_t		time;	/* tag time */
   
      if (sd == NULL) {
         return -1 ;
      }
   
      /* increase in use count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* loop over bound scheduler list */
      for (indx = 0; (indx < data->maxrlist) && 
          (data->rlist[indx] != NULL); indx++) {
      
      	 /* call set scheduler tag */
         time = ((tainsec_t) tai) * _ONESEC + 
                ((tainsec_t) epoch) * _EPOCH;
         bsd = data->rlist[indx]->self;
         if ((bsd->scheduler_flags & SCHED_LOCAL_BOUND) == 
            SCHED_LOCAL_BOUND) {
            /* do not notify to prevent recursions */
            setSchedulerTag (bsd, tag, time, 1);
         }
         else {
            /* but do notify remote scheduler servers */
            setSchedulerTag (bsd, tag, time, 0);
         }
      }
   
      /* decrease in use count */
      _dataUsage (data, DECREASE);
   
      /* remote scheduler don't have tag lists */
      return -1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setTagNotifyAsync				*/
/*                                                         		*/
/* Procedure Description: async. set tag for notifying server scheduler	*/
/*                                                         		*/
/* Procedure Arguments: _tagAsync_t structure				*/
/*                                                         		*/
/* Procedure Returns: 0							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct _tagAsync_t {
      scheduler_t* 	sd;		/* local sched. handle */
      char 		tag[TIMETAG_LENGTH];	/* time tag */
      tainsec_t		time;		/* tag time */
   };

   typedef struct _tagAsync_t _tagAsync_t;

   static _schedproc_t _setTagNotifyAsync (_tagAsync_t* arg)
   
   {
      rscheduler_info_t*	data;	/* rsched info */
      enum clnt_stat 		retval;	/* return value of rpc call */
      int			result;	/* result */
      char			cbname[20]; /* client name */
      CLIENT*			clnt;	/* client handle */
   
      /* initialize rpc for VxWorks */
   #ifdef OS_VXWORKS
      rpcTaskInit ();
   #endif
   
      /* increase the usage count */
      data = (rscheduler_info_t*) arg->sd->data;
      _dataUsage (data, INCREASE);
   
      /* get client name */
   #ifdef OS_VXWORKS
      inet_ntoa_b (data->remotehost, cbname);
   #else
      strncpy (cbname, inet_ntoa (data->remotehost), sizeof (cbname));
      cbname[sizeof(cbname)-1] = 0;
   #endif
      /* create client handle */
      clnt = clnt_create (cbname, data->remotenum, 
                         data->remotever, _NETID);
      if (clnt == (CLIENT*) NULL) {
         return (_schedproc_t) NULL;
      }
   
      /* use rpc to call wait function on remote system */
      retval = settagnotify_1 (data->rd, (char*) arg->tag, arg->time, 
                              &result, clnt);
   
      /* destroy client handle, decrease the usage count */
      clnt_destroy (clnt);
      _dataUsage (data, DECREASE);
      free (arg);
   
   #ifndef OS_VXWORKS
      return (_schedproc_t) NULL;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setTagNotify_b				*/
/*                                                         		*/
/* Procedure Description: tag notify for remotely bound scheduler	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, tag, time, epoch		*/
/*                                                         		*/
/* Procedure Returns: -1 						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int _setTagNotify_b (scheduler_t* sd, const char* tag,
                     taisec_t tai, int epoch)
   {
      _tagAsync_t*		arg;	/* thread arggument */
      int			attr;	/* thread attribute */
      schedTID_t		setTID;	/* thread TID */
   
      if (sd == NULL) {
         return -1 ;
      }
   
      /* fill in argument list */
      arg = malloc (sizeof (_tagAsync_t));
      if (arg == NULL) {
         return -1;
      }
      arg->sd = sd;
      strncpy (arg->tag, tag, TIMETAG_LENGTH);
      arg->tag[TIMETAG_LENGTH-1] = 0;
      arg->time = ((tainsec_t) tai) * _ONESEC + 
                  ((tainsec_t) epoch) * _EPOCH;
   
      /* make thread which calls set routines of all bound schedulers */
   #ifdef OS_VXWORKS
      attr = 0;
   #else
      attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_PROCESS;
   #endif
      if (_threadSpawn (attr, _CALLBACK_PRIORITY, &setTID,
         (_schedtask_t) _setTagNotifyAsync, (_schedarg_t) arg) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "unable to notify server scheduler of time tag");
      }
   
      /* remotely bound schedulers don't have their own time tag list */
      return -1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setTagNotify_l				*/
/*                                                         		*/
/* Procedure Description: tag notify for locally bound scheduler	*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, tag, time, epoch		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int _setTagNotify_l (scheduler_t* sd, const char* tag,
                     taisec_t tai, int epoch)
   {
      rscheduler_info_t*	data;	/* rsched info */
      tainsec_t			time;	/* tag time */
   
      if (sd == NULL) {
         return -1 ;
      }
   
      /* increase the usage count */
      data = (rscheduler_info_t*) sd->data;
      _dataUsage (data, INCREASE);
   
      /* call remote scheduler set tag function */
      time = ((tainsec_t) tai) * _ONESEC + ((tainsec_t) epoch) * _EPOCH;
      (void) setSchedulerTag (data->parent, tag, time, 0);
   
      /* decrease in use count */
      _dataUsage (data, DECREASE);
   
      /* locally bound schedulers get called again by the (parent)
         remote scheduler with notification disabled; hence the tag
         is already set! */
      return -1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: settagcallback_1_svc			*/
/*                                                         		*/
/* Procedure Description: callbach for setting a time tag		*/
/*                                                         		*/
/* Procedure Arguments: remote scheduler descriptor, time tag, time of	*/
/*			tag, result 0 if success, <0 else, scv_req	*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t settagcallback_1_svc (scheduler_r rsd, scheduler_r rbsd,
                     char* tag, tainsec_r time, int* result, 
                     struct svc_req* rqstp)
   {
      rscheduler_info_t*	data;	/* rsched info */
      scheduler_t*		sd;	/* remote scheduler */
      scheduler_t*		bsd;	/* bound sched: callback orig */
   
   #ifdef DEBUG
      printf ("callback tag %s (%li)\n", tag, (long) (time / _ONESEC));
   #endif
   
      /* obtain scheduler descriptors */
      memcpy (&sd, &rsd, sizeof (scheduler_t*));
      memcpy (&bsd, &rbsd, sizeof (scheduler_t*));
   
      if (sd == NULL) {
         return TRUE;
      }
   
      data = (rscheduler_info_t*) sd->data;
   
      /* increase in use count  */
      _dataUsage (data, INCREASE);
   
       /* call remote scheduler set tag function */
      *result = setSchedulerTag (sd, tag, time, 0);
   
      /* decrease in use count */
      _dataUsage (data, DECREASE);
   
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsschedulercallback_1_freeresult		*/
/*                                                         		*/
/* Procedure Description: frees memory of callback			*/
/*                                                         		*/
/* Procedure Arguments: rpc transport info, xdr routine for result,	*/
/*			pointer to result				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsschedulercallback_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _remoteSetup				*/
/*                                                         		*/
/* Procedure Description: setup for remote scheduler			*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _remoteSetup (scheduler_t* sd)
   {
      rscheduler_info_t*	data;
   
      /* init pointers to schedulers objects */
      data = (rscheduler_info_t*) sd->data;
      data->parent = NULL;
      data->self = sd;
   
      /* create semaphore */
      if (_SCHEDMUTEX_CREATE (data->sem) != 0) {
         return -1;
      }
      data->inUse = 0;
   
      /* init callback parameters */
      data->remotenum = 0;
      data->remotever = 0;
      data->clnt = NULL;
   
      /* allocate memory for remote scheduler list and set it to empty */
      data->maxrlist = _INIT_RLIST;
      data->rlist = (rscheduler_info_t**) calloc (_INIT_RLIST, 
                                      sizeof (rscheduler_info_t*));
      if (data->rlist == NULL) {
         return -1;
      }
      data->rlist[0] = NULL;
   
      /* overload scheduler functions */
      data->closeScheduler = sd->closeScheduler;
      sd->closeScheduler = _closeScheduler;
      sd->scheduleTask = _scheduleTask;
      sd->getScheduledTask = _getScheduledTask;
      sd->removeScheduledTask = _removeScheduledTask;
      sd->waitForSchedulerToFinish = _waitForSchedulerToFinish;
      sd->setTagNotify = _setTagNotify;
   
       /* initialize rpc for VxWorks */
   #ifdef OS_VXWORKS
      rpcTaskInit ();
   #endif
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _remoteSetup_b				*/
/*                                                         		*/
/* Procedure Description: setup for bound scheduler			*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, -1 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _remoteSetup_b (scheduler_t* sd)
   {
      rscheduler_info_t*	data;		/* data info */
      rscheduler_info_t*	pdata;		/* remote sched data info */
      char			hostname[30];	/* hostname */
      rscheduler_info_t**	newlist;	/* new rlist */
      int			i;		/* index into rlist */
   
      /* init pointers to schedulers objects */
      data = (rscheduler_info_t*) sd->data;
      pdata = (rscheduler_info_t*) data->parent->data;
      data->self = sd;
   
      /* create semaphore */
      if (_SCHEDMUTEX_CREATE (data->sem) != 0) {
         return -1;
      } 
      data->inUse = 0;
   
      /* no use for remote scheduler list */
      data->maxrlist = 0;
      data->rlist = NULL;
   
      /* overload scheduler functions */
      data->closeScheduler = sd->closeScheduler;
      sd->closeScheduler = _closeScheduler_b;
      if ((sd->scheduler_flags & SCHED_LOCAL_BOUND) == 
         SCHED_REMOTE_BOUND) {
      	 /* use these functions only when remotely bound;
      	    otherwise use default */
         sd->scheduleTask = _scheduleTask_b;
         sd->getScheduledTask = _getScheduledTask_b;
         sd->removeScheduledTask = _removeScheduledTask_b;
         sd->waitForSchedulerToFinish = _waitForSchedulerToFinish_b;
         sd->setTagNotify = _setTagNotify_b;
      }
      else {
         sd->setTagNotify = _setTagNotify_l;
      }
   
      /* initialize rpc for VxWorks */
   #ifdef OS_VXWORKS
      if ((sd->scheduler_flags & SCHED_LOCAL_BOUND) == 
         SCHED_REMOTE_BOUND) {
         rpcTaskInit ();
      }
   #endif
   
      /* init client handle for rpc */
      if ((sd->scheduler_flags & SCHED_LOCAL_BOUND) == 
         SCHED_LOCAL_BOUND) {
         /* locally bound; no need for rpc handles */
         data->clnt = NULL;
      }
      else {
         /* remotely bound; need to create an rpc handle */
      #ifdef OS_VXWORKS
         inet_ntoa_b (data->remotehost, hostname);
      #else
         strncpy (hostname, inet_ntoa (data->remotehost), 
                 sizeof (hostname));
         hostname[sizeof(hostname)-1] = 0;
      #endif
         data->clnt = clnt_create (hostname, data->remotenum, 
                                  data->remotever, _NETID);
         if (data->clnt == (CLIENT *) NULL) {
            return -1;
         }
      }
   
      /* add scheduler data info to (parent) remote scheduler */
      _dataUsage (pdata, GET);
      for (i = 0; i < pdata->maxrlist; i++) {
         if (pdata->rlist[i] == NULL) {
            break;
         }
      }
      /* check wheter list long enough */
      if (i + 2 >= pdata->maxrlist) {
         gdsDebug ("make bound scheduler list longer");
         newlist = realloc (pdata->rlist, sizeof (rscheduler_info_t*) *
                           (pdata->maxrlist + _INIT_RLIST));
         if (newlist == NULL) {
            _dataUsage (pdata, RELEASE);
            return -1;
         }
         pdata->rlist = newlist;
         pdata->maxrlist += _INIT_RLIST;
      }
      /* add to list */
      pdata->rlist[i] = data;
      pdata->rlist[i+1] = NULL;
      _dataUsage (pdata, RELEASE);
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initCallbackSVC				*/
/*                                                         		*/
/* Procedure Description: initializes callback server and variables	*/
/*                                                         		*/
/* Procedure Arguments: version number of rpc interface			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initCallbackSVC (unsigned int cbver)
   {
      struct timespec	tick = {0, _INIT_SLEEP};
      int		attr;		/* thread attribute */
   
      /* set version of callback */
      callbackVersNum = cbver;
   
      /* now spawn the thread which waits for rpc callbacks */
   #ifdef OS_VXWORKS
      attr = 0;
   #else
      attr = PTHREAD_CREATE_JOINABLE | PTHREAD_SCOPE_SYSTEM;
   #endif
      if (_threadSpawn (attr, _CALLBACK_PRIORITY, &callbackTID,
         (_schedtask_t) svcProcess, (_schedarg_t) NULL) != 0) {
         return -1;
      }
   
      /* wait for thread to finish registration */
      while (callbackProgNum == 0) {
         nanosleep (&tick, NULL);
      }
      if (callbackProgNum == (u_long) -1) {
         return -2;
      }
   
      /* successful initialization */
      initCallback = 1;
      return 0;   
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: svcProcess					*/
/*                                                         		*/
/* Procedure Description: rpc callback service				*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor				*/
/*                                                         		*/
/* Procedure Returns: void (sets callbacknum if successful)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static _schedproc_t svcProcess (void)
   {
      u_long		callbacknum;
   
      /* initialize rpc for VxWorks */
   #ifdef OS_VXWORKS
      rpcTaskInit ();
   #endif
   
      /* register service */
      callbackTransport = NULL;
      if (rpcRegisterCallback (&callbacknum, callbackVersNum, 
         &callbackTransport, gdsschedulercallback_1) != 0) {
         callbackProgNum = (u_long) -1;
      #ifdef OS_VXWORKS
         return;
      #else
         return (void*) NULL;
      #endif
      }
      /* calback is registered */
      callbackProgNum = callbacknum;
   
      /* invoke rpc service; never to return */
      svc_run();
      gdsDebug ("rpc service has returned");
   #ifndef OS_VXWORKS
      return (void*) NULL;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _dataUsage					*/
/*                                                         		*/
/* Procedure Description: increases/decreases get/release the inUse 	*/
/*			  count; get only returns with mutex and inUse	*/
/*			  equal to zero					*/
/*                                                         		*/
/* Procedure Arguments: pointer to remoted sched info			*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void _dataUsage (rscheduler_info_t* data, _direction_t dir)
   {
      struct timespec	tick = {0, _SEM_SLEEP};
   
      switch (dir)
      {
         case INCREASE:
         case DECREASE:
            {
            /* get bound scheduler data mutex first */
               if (_SCHEDMUTEX_GET (data->sem) != 0) {
                  gdsError (GDS_ERR_PROG,
                           "Couldn't get bound scheduler data mutex");
               }
            
            /* increase/decrease inUse count */
               data->inUse += (dir == INCREASE) ? 1 : -1;
            
            /* release bound scheduler data mutex */
               if (_SCHEDMUTEX_RELEASE (data->sem) != 0) {
                  gdsError (GDS_ERR_PROG,
                           "Failure to release bound scheduler data mutex");
               }
            
               break;
            }
         case RELEASE :
            {
            /* release bound scheduler data mutex */
               if (_SCHEDMUTEX_RELEASE (data->sem) != 0) {
                  gdsError (GDS_ERR_PROG,
                           "Failure to release bound scheduler data mutex");
               }
               break;
            }
         case GET :
            {
               for (;;) {
               /* get bound scheduler data mutex first */
                  if (_SCHEDMUTEX_GET (data->sem) != 0) {
                     gdsError (GDS_ERR_PROG,
                              "Couldn't get bound scheduler data mutex");
                  }
               
                  if (data->inUse <= 0) {
                     break;
                  }
               
               /* release bound scheduler data mutex */
                  if (_SCHEDMUTEX_RELEASE (data->sem) != 0) {
                     gdsError (GDS_ERR_PROG,
                              "Failure to release bound scheduler data mutex");
                  }
               
               /* go to sleep for a little while */
                  nanosleep (&tick, NULL);
               }
               break;
            }
      }
   }

