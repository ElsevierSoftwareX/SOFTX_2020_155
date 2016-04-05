static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssched_server						*/
/*                                                         		*/
/* Module Description: implements server functions for scheduling tasks	*/
/* from remote computers						*/
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

#define PORTMAP  1

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
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdsheartbeat.h"
#include "dtt/gdssched.h"
#include "dtt/gdssched_server.h"
#include "dtt/gdsrsched.h"
#include "dtt/gdsxdr_util.h"
#include "dtt/gdssched_util.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _NETID		  net protocol used for rpc		*/
/*            _SHUTDOWN_DELAY	  wait period before shut down		*/
/*            _SEM_SLEEP	  when waiting for semaphore		*/
/*            SIG_PF		  signal function prototype 		*/
/*            _IDLE		  server idle state			*/
/*            _BUSY		  server busy state			*/
/*            _MAX_DISPATCH	  maximum number of scheduler classes	*/
/*            _CALLBACK_PRIORITY  priority of callback thread		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _NETID			"tcp"
#define _SHUTDOWN_DELAY		120		/* 120 s */
#define _SEM_SLEEP		100000000UL	/* 100 ms */
#ifndef SIG_PF
#define	SIG_PF 			void (*) (int)
#endif
#define	_IDLE 			0
#define	_BUSY	 		1
#define _MAX_DISPATCH		100
#ifdef OS_VXWORKS
#define _CALLBACK_PRIORITY	90
#else
#define _CALLBACK_PRIORITY	18
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: servermux		protects globals			*/
/*          rpcpmstart		started by a port monitor?		*/
/*          initServer		if 0 the server is not yet initialized	*/
/*          openSchedulers	number of open schedulers    		*/
/*          serverState		state of server		 		*/
/*          transp		rpc service handle        		*/
/*          proto		protocol (portmap svc only)		*/
/*          numdt		number of registered dispatch tables  	*/
/*          listdt		list of registered dispatch tables	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
   static schedmutex_t		_servermux = NULL;
#else
   static schedmutex_t		_servermux = PTHREAD_MUTEX_INITIALIZER;
#endif
   static int 			_rpcpmstart;
   static int			_initServer = 0;
   static int			_openSchedulers = 0;
#ifndef OS_VXWORKS
   static int			_serverState = _IDLE;
#endif
   static SVCXPRT*		transp;
#if defined (OS_VXWORKS) || defined (PORTMAP)
   static int 			proto;
#endif
   static int			_numdt = 0;
   static scheduler_dt_t 	_listdt[_MAX_DISPATCH];


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initializeServer	initializes rpc server			*/
/*      gdsscheduler_1		rpc dispatch function			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initializeServer (void);
   extern void gdsscheduler_1 (struct svc_req* rqstp, 
                     register SVCXPRT* transp);
   static void _dataUsage (sscheduler_info_t* data, _direction_t dir);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: registerSchedulerClass			*/
/*                                                         		*/
/* Procedure Description: registers a scheduler class with rpc		*/
/*                                                         		*/
/* Procedure Arguments: prognum, progver, dispatch table		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int registerSchedulerClass (scheduler_dt_t* schedclass)
   {
      int		indx;	/*index into the library */
   
      /* if not yet initialized, do it */
      if (_initServer == 0) {
         if (initializeServer () != 0) {
            return -1;
         }
      }
   
      /* test whether dispatch table is valid */
      if (schedclass == NULL) {
         return -2;
      }
   
      /* check if there is library space */
      if (_numdt >= _MAX_DISPATCH) {
         return -3;
      }
   
      /* get server mutex */
      _SCHEDMUTEX_GET (_servermux);
   
      /* check whether class already in library */
      for (indx = 0; indx < _numdt; indx++) {
         if ((_listdt[indx].prognum == schedclass->prognum) && 
            (_listdt[indx].progver == schedclass->progver)) {
            break;	/* found it */
         }
      }
      if (indx == _numdt) {
         _numdt++;
      }
   
      /* add scheduler class to library */
      _listdt[indx] = *schedclass;
      _listdt[indx].dt = calloc (schedclass->numentries, 
                                sizeof (scheduler_dtentry_t));
      memcpy (_listdt[indx].dt, schedclass->dt, 
             schedclass->numentries * sizeof (scheduler_dtentry_t));
   
   /* register with rpc service */
   #if defined (OS_VXWORKS) || defined (PORTMAP)
      /* port map service */
      if (_rpcpmstart != 1) {
         /* stand alone */
         (void) pmap_unset (schedclass->prognum, schedclass->progver);
      }
      if (!svc_register (transp, schedclass->prognum, 
         schedclass->progver, gdsscheduler_1, proto)) {
         gdsError (GDS_ERR_PROG, "unable to create rpc service");
         return -4;
      }
   #else
      /* rcp bind service */
      if (_rpcpmstart == 1) {
         /* port monitor */
         if (!svc_reg (transp, schedclass->prognum, schedclass->progver, 
            gdsscheduler_1, 0)) {
            gdsError (GDS_ERR_PROG, "unable to create rpc service");
            return -5;
         }
      }
      else {
         /* stand alone */
         if (!svc_create (gdsscheduler_1, schedclass->prognum, 
            schedclass->progver, _NETID)) {
            gdsError (GDS_ERR_PROG, "unable to create rpc service");
            return -6;
         }
      }
   #endif   
   
      /* release server mutex */
      _SCHEDMUTEX_RELEASE (_servermux);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: runSchedulerService				*/
/*                                                         		*/
/* Procedure Description: start rpc service task			*/
/*                                                         		*/
/* Procedure Arguments: mode						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void runSchedulerService (int mode)
   {
      /* test whether at least one server scheduler was registered */
      if (_initServer == 0) {
         return;
      }
   
      /* if on UNIX, set the rpc mode */
   #if !defined (OS_VXWORKS) && !defined (PORTMAP)
// TODO: maybe this is not needed?
#if 0
      if ((mode & SCHED_MT_AUTO) == SCHED_MT_AUTO) {
      
         int smode = RPC_SVC_MT_AUTO;	/* rpc mode parameter */
      
         if (!rpc_control (RPC_SVC_MTMODE_SET, &smode)) {
            gdsError (GDS_ERR_PROG, "unable to set automatic MT mode");
            return;
         }
         gdsDebug ("automatic MT server mode");
      }
#endif

   #endif
   
      /* wait for rpc calls */
      svc_run();
      /* never reached */
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: checkStdInHandle				*/
/*                                                         		*/
/* Procedure Description: check FD 0 to determine if it was called by	*/
/*			  a port monitor				*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 1 if called by port mon., 0 if not, -1 if failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifdef OS_VXWORKS
#elif defined (PORTMAP)
   static int checkStdInHandle (void)
   {
      struct sockaddr_in saddr;
      socklen_t		asize = sizeof (saddr);
      int 		rpcfdtype;
      socklen_t		ssize = sizeof (int);
   
      /* get the socket address of FD 0 */
      if (getsockname (0, (struct sockaddr*) &saddr, &asize) == 0) {
      
         /* check whether valid internet address */
         if (saddr.sin_family != AF_INET) {
            return -1;
         }
      	 /* get socket type: tcp only */
         if ((getsockopt (0, SOL_SOCKET, SO_TYPE,
            (char*) &rpcfdtype, &ssize) == -1) || 
            (rpcfdtype != SOCK_STREAM)) {
            return -1;
         }
         return 1;
      }
      else {
         return 0;
      }
   }
#else
   static int checkStdInHandle (void)       
   {
   /*
    * If stdin looks like a TLI endpoint, we assume
    * that we were started by a port monitor. If
    * t_getstate fails with TBADF, this is not a
    * TLI endpoint.
    */
      if ((t_getstate (0) != -1) || (t_errno != TBADF)) {      
         openlog ("gdsrsched", LOG_PID, LOG_DAEMON);
         return 1;
      }
      else {
         return 0;
      }
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: closedown					*/
/*                                                         		*/
/* Procedure Description: closedown routine for port monitors		*/
/*                                                         		*/
/* Procedure Arguments: signal						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef OS_VXWORKS
   static void closedown (int sig)
   {
      if ((_openSchedulers != 0) ||
         (_SCHEDMUTEX_TRY (_servermux) != 0)) {
         _serverState = _BUSY;
         (void) signal (SIGALRM, (SIG_PF) closedown);
         (void) alarm (_SHUTDOWN_DELAY / 2);
         return;
      }
   
      if (_serverState == _IDLE) {
         exit (0);
      } 
      else {
         _serverState = _IDLE;
      }
   
      _SCHEDMUTEX_RELEASE (_servermux);
      (void) signal (SIGALRM, (SIG_PF) closedown);
      (void) alarm (_SHUTDOWN_DELAY / 2);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initializeServer				*/
/*                                                         		*/
/* Procedure Description: initializes server stuff			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initializeServer (void) 
   {
      /* ignore pipe signals */
   #if _POSIX_SOURCE
      struct sigaction action;
      action.sa_flags = 0;
      sigemptyset (&action.sa_mask);
      action.sa_handler = SIG_IGN;
      sigaction (SIGPIPE, &action, NULL);
   #elif !defined(OS_VXWORKS)
      (void) sigset (SIGPIPE, SIG_IGN);
   #endif
   
      /* create server mutex */
   #ifdef OS_VXWORKS
      if (_servermux == NULL) {
         if (_SCHEDMUTEX_CREATE (_servermux) != 0) {
            return -1;
         }
      }
   #endif
   
      /* init rpc on VxWorks */
   #ifdef OS_VXWORKS
      if (rpcTaskInit () != 0) {
         return -2;
      }
   #endif
   
      /* find out whether started by port monitor */
   #ifdef OS_VXWORKS
      _rpcpmstart = 0;
   #else
      _rpcpmstart = checkStdInHandle ();
      if (_rpcpmstart == -1) {
         return -3;
      }
   #endif
   
      /* fork on UNIX machines */
   #if !defined (OS_VXWORKS) && !defined (RPC_SVC_FG)
      if (_rpcpmstart == 0) {
         int 		size;
         struct rlimit rl;
         int 		pid;		/* process id */
         int		i;
      
         /* try to fork */
         pid = fork ();
         if (pid < 0) {
            /* error */
            gdsError (GDS_ERR_PROG, "cannot fork");
            return -4;
         }
      	 /* exit if parent */
         if (pid != 0) {
            exit (0);
         }
      	 /* child process starte here: close all unnecessary files */
         rl.rlim_max = 0;
         getrlimit (RLIMIT_NOFILE, &rl);
         if ((size = rl.rlim_max) == 0) {
            gdsError (GDS_ERR_PROG, "unable to close file handles");
            return -5;
         }
         for (i = 0; i < size; i++) {
            (void) close(i);
         }
         i = open("/dev/null", 2);
         (void) dup2(i, 1);
         (void) dup2(i, 2);
         setsid ();
         openlog ("gdsrsched", LOG_PID, LOG_DAEMON);
         gdsDebug ("server process has forked");
      }
   #endif
   
      /* create transport */
   #if defined (OS_VXWORKS) || defined (PORTMAP)
      /* port map service */
      {
         int 		sock;		/* socket handle */
   
         /* create socket if not portmap */
         if (_rpcpmstart == 1) {
            sock = 0;
            proto = 0;
         }
         else {
            sock = RPC_ANYSOCK;
            proto = IPPROTO_TCP;
         }
         /* create rpc service handle: tcp only */
         transp = svctcp_create (sock, 0, 0);
         if (transp == NULL) {
            gdsError (GDS_ERR_PROG, "cannot create tcp service");
            return -6;
         }
      }
   #else
      /* rpc bind service */
      {
         struct netconfig* 	nconf = NULL;
      
         if (_rpcpmstart == 1) {
            /* get netid */
            nconf = getnetconfigent (_NETID);
            if (nconf == NULL) {
               gdsError (GDS_ERR_PROG, "tcp service not available");
               return -7;
            }
            /* create rpc service handle for FD 0 */
            transp = svc_tli_create (0, nconf, NULL, 0, 0);
            freenetconfigent(nconf);
            if (transp == NULL) {
               gdsError (GDS_ERR_PROG, "cannot create service handle");
               return -8;
            }
         }
         else {
            /* do not need service handle */
         }
      }
   #endif
   
      /* install close down procedure if started by a port monitor */
   #ifndef OS_VXWORKS
      if (_rpcpmstart == 1) {
         (void) signal (SIGALRM, (SIG_PF) closedown);
         (void) alarm (_SHUTDOWN_DELAY / 2);
      }
   #endif
   
      /* set initServer and return */
      _initServer = 1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _closeScheduler				*/
/*                                                         		*/
/* Procedure Description: close function for server scheduler		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _closeScheduler (scheduler_t* sd, tainsec_t timeout)
   {
      sscheduler_info_t*	info;	/* scheduler info */
   
      /* destroy mutex */
      info = (sscheduler_info_t*) sd->data;
      _SCHEDMUTEX_DESTROY (info->sem); 
   
      /* call old close function */
      return info->closeScheduler (sd, timeout);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setTagNotifyAsync				*/
/*                                                         		*/
/* Procedure Description: async. set tag task for server scheduler	*/
/*                                                         		*/
/* Procedure Arguments: _tagAsync_t structure				*/
/*                                                         		*/
/* Procedure Returns: 0							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct _tagAsync_t {
      struct in_addr	addr;		/* client address */
      u_long		prognum;	/* callback rpc prog num */
      u_long		progver;	/* callback rpc ver num */
      scheduler_r	rd;		/* callback sched. handle */
      scheduler_r	lsd;		/* local sched. handle */
      char 		tag[TIMETAG_LENGTH];	/* time tag */
      tainsec_t		time;		/* tag time */
   };

   typedef struct _tagAsync_t _tagAsync_t;

   static void _setTagNotifyAsync (_tagAsync_t* arg)
   {
      char			cbname[20]; /* callback host name */
      CLIENT*			clnt;	/* client handle */
      int			result;	/* result of rpc call */
   
       /* rpc init for VxWorks */
   #ifdef OS_VXWORKS
      rpcTaskInit ();
   #endif
   
      /* get client name */
   #ifdef OS_VXWORKS
      inet_ntoa_b (arg->addr, cbname);
   #else
      strncpy (cbname, inet_ntoa (arg->addr), sizeof (cbname));
      cbname[sizeof(cbname)-1] = 0;
   #endif
      /* create client handle */
      clnt = clnt_create (cbname, arg->prognum, 
                         arg->progver, _NETID);
      if (clnt == (CLIENT*) NULL) {
         free (arg);
         return;
      }
   
      /* make the callback */
      (void) settagcallback_1 (arg->rd, arg->lsd, arg->tag, arg->time,
                           &result, clnt);
   
      /* destroy client handle */
      clnt_destroy (clnt);
      free (arg);
      gdsDebug ("rpc callback thread successful: set tag");
   #ifndef OS_VXWORKS
      pthread_exit (&result);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setTagNotify				*/
/*                                                         		*/
/* Procedure Description: set tag function for server scheduler		*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor, time tag, tai, epoch	*/
/*                                                         		*/
/* Procedure Returns: 0							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _setTagNotify (scheduler_t* sd, const char* tag,
                     taisec_t tai, int epoch)
   {
      sscheduler_info_t*	info;	/* scheduler info */
      int			attr;	/* thread attribute */
      _tagAsync_t*		arg;	/* arg for thread */
      schedTID_t		callbackTID;
   
      gdsDebug ("rpc callback: set tag");
   #ifdef DEBUG
      printf ("set tag on client %s (%li)\n", tag, tai); 
   #endif
   
      /* quit if invalid scheduler or tag */
      if ((sd == NULL) || (tag == NULL)) {
         return 0;
      }
   
      /* initiate callback to client, create client handle */
      info = (sscheduler_info_t*) sd->data;
      arg = malloc (sizeof (_tagAsync_t));   
      if (arg == NULL) {
         return 0;
      }
      _dataUsage (info, INCREASE);
   
      /* fill in argument */
      arg->addr = info->clienthost;
      arg->prognum = info->callbacknum;
      arg->progver = info->callbackver;
      memcpy (&arg->lsd, &sd, sizeof (scheduler_t*));
      arg->rd = info->rd;
      strncpy (arg->tag, tag, TIMETAG_LENGTH);
      arg->tag[TIMETAG_LENGTH-1] = 0;
      arg->time = ((tainsec_t) tai) * _ONESEC + 
                  ((tainsec_t) epoch) * _EPOCH;
   
      /* make the callback */
      /* spawn a thread which waits for rpc callbacks */
   #ifdef OS_VXWORKS
      attr = 0;
   #else
      attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_PROCESS;
   #endif
      if (_threadSpawn (attr, _CALLBACK_PRIORITY, &callbackTID,
         (_schedtask_t) _setTagNotifyAsync, (_schedarg_t) arg) != 0) {
         gdsError (GDS_ERR_PROG, 
                  "unable to notify other schedulers of time tag");
      }
      /* decrease in use count */
      _dataUsage (info, DECREASE);
   
      /* always set local time tag */
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: _setup					*/
/*                                                         		*/
/* Procedure Description: setup for the scheduler			*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int _setup (scheduler_t* sd)
   {
      int			indx;	/* index into class library */
      sscheduler_info_t*	info;	/* scheduler info */
      int			err;	/* error number */
   
      /* get data pointer */
      if (sd == NULL) {
         return -1;
      }
      info = (sscheduler_info_t*) sd->data;
   
      /* initialize scheduler info */
      if (_SCHEDMUTEX_CREATE (info->sem) != 0) {
         return -2;
      }
      info->inUse = 0;
      info->closeScheduler = sd->closeScheduler;
      sd->closeScheduler = _closeScheduler;
      sd->setTagNotify = _setTagNotify;
   
      /* get server mutex */
      _SCHEDMUTEX_GET (_servermux);
   
      /* find scheduler class in library */
      err = 0;
      for (indx = 0; indx < _numdt; indx++) {
         if ((_listdt[indx].prognum == info->prognum) && 
            (_listdt[indx].progver == info->progver)) {
            break;	/* found it */
         }
      }
   
      /* assign dispatch table if found */
      if (indx != _numdt) {
         info->dispatch = &_listdt[indx];
      }
      else {
         err = -2;
         info->dispatch = NULL;
      }
   
      /* release server mutex */
      _SCHEDMUTEX_RELEASE (_servermux);
   
      return err;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: connectscheduler_1_svc			*/
/*                                                         		*/
/* Procedure Description: creates a local scheduler			*/
/*                                                         		*/
/* Procedure Arguments: callback parameters				*/
/*                                                         		*/
/* Procedure Returns: 0 + sched. descr. if successful, <0 if failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t connectscheduler_1_svc (scheduler_r callbacksd, 
                     u_int callbackprogram , u_int callbackversion, 
                     remotesched_r* result, struct svc_req* rqstp)
   {
      scheduler_t*	 lsd;	/* local scheduler descriptor */
      sscheduler_info_t* info;	/* server scheduler info */
   
      gdsDebug ("rpc call: connection");
   
      /* allocate memory for scheduler info */
      result->status = -1;
      memset (&result->sd, 0, sizeof (result->sd));
      info = malloc (sizeof (sscheduler_info_t));
      if (info == NULL) {
         return TRUE;
      }
   
      /* fill in scheduler info */
      info->dispatch = NULL;
      info->prognum = rqstp->rq_prog;
      info->progver = rqstp->rq_vers;
      info->callbacknum = callbackprogram;
      info->callbackver = callbackversion;
      info->rd = callbacksd;
      if (rpcGetClientaddress (rqstp->rq_xprt, &info->clienthost) != 0) {
         free (info);
         return TRUE;
      }
   
      /* create locale scheduler */
      lsd = createScheduler (SCHED_REMOTE_SERVER, _setup, (void*) info);
      if (lsd == NULL) {
         return TRUE;
      }
   
      /* copy local handle into result record */
      memcpy (&result->sd, &lsd, sizeof (scheduler_t*));
   
      /* get mutex, increase number of open schedulers, release  */
      _SCHEDMUTEX_GET (_servermux);
      _openSchedulers++;
      _SCHEDMUTEX_RELEASE (_servermux);
   
      result->status = 0;
      gdsDebug ("rpc call successful: connection");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: closescheduler_1_svc				*/
/*                                                         		*/
/* Procedure Description: closes a local scheduler			*/
/*                                                         		*/
/* Procedure Arguments: sched. descr., timeout				*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t closescheduler_1_svc (scheduler_r sd, tainsec_r timeout, 
                     int* result, struct svc_req* rqstp)
   {
      scheduler_t*	lsd;	/* local scheduler descriptor */
      sscheduler_info_t* info;	/* server scheduler info */
   
      gdsDebug ("rpc call: close");
   
      /* reconstruct local scheduler descriptor */
      memcpy (&lsd, &sd, sizeof (scheduler_t*));
      if (lsd == NULL) {
         *result = 0;
         return TRUE;
      }
      info = (sscheduler_info_t*) lsd->data;
      _dataUsage (info, GET);
   
      /* call local close function */
      *result = closeScheduler (lsd, timeout);
   
      /* get mutex, decrease number of open schedulers, release */
      _SCHEDMUTEX_GET (_servermux);
      _openSchedulers--;
      _SCHEDMUTEX_RELEASE (_servermux);
   
      gdsDebug ("rpc call successful: close");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: scheduletask_1_svc				*/
/*                                                         		*/
/* Procedure Description: schedules a task to a local scheduler		*/
/*                                                         		*/
/* Procedure Arguments: sched. descr., newtask				*/
/*                                                         		*/
/* Procedure Returns: id if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t scheduletask_1_svc (scheduler_r sd, schedulertask_r newtask, 
                     int* result, struct svc_req* rqstp)
   {
      scheduler_t*	 lsd;	/* local scheduler descriptor */
      schedulertask_t 	 task;	/* task info */
      sscheduler_info_t* info;	/* server scheduler info */
      int		 indx;	/* index into dispatch table */
      scheduler_dtentry_t* dt;	/* pointer into dispatch table */
   
      gdsDebug ("rpc call: schedule");
   
      /* reconstruct local scheduler descriptor */
      memcpy (&lsd, &sd, sizeof (scheduler_t*));
      if (lsd == NULL) {
         *result = -1;
         return TRUE;
      }
      info = (sscheduler_info_t*) lsd->data;
      _dataUsage (info, INCREASE);
   
      /* copy data */
      task.flag = newtask.flag;
      task.priority = newtask.priority;
      task.timeout = newtask.timeout;
      task.tagtype = newtask.tagtype;
      task.synctype = newtask.synctype;
      task.syncval = newtask.syncval;
      task.waittype = newtask.waittype;
      task.waitval = newtask.waitval;
      task.repeattype = newtask.repeattype;
      task.repeatval = newtask.repeatval;
      task.repeatratetype = newtask.repeatratetype;
      task.repeatval = newtask.repeatval;
      task.repeatratetype = newtask.repeatratetype;
      task.repeatrate = newtask.repeatrate;
      task.repeatsynctype = newtask.repeatsynctype;
      task.repeatsyncval = newtask.repeatsyncval;
      strncpy (task.timetag, newtask.timetag, TIMETAG_LENGTH);
      task.timetag[TIMETAG_LENGTH-1] = 0;
      strncpy (task.waittag, newtask.waittag, TIMETAG_LENGTH);
      task.waittag[TIMETAG_LENGTH-1] = 0;
      task.arg_sizeof = newtask.arg_sizeof;
      /* look up dispatch table */
      dt = info->dispatch->dt;
      for (indx = 0; indx < info->dispatch->numentries; indx++) {
         if ((int) newtask.func == dt->id) {
            break; 
         }
         dt++;
      }
      /* if not found return */
      if (indx == info->dispatch->numentries) {
         *result = -26;
         _dataUsage (info, DECREASE);
         return TRUE;
      }
      /* set functions of scheduling task */
      task.func = dt->func;
      task.freeResources = dt->freeResources;
      task.xdr_arg = dt->xdr_arg;
   
      *result = 
         xdr_decodeArgument ((char**) &task.arg, task.arg_sizeof, 
                           newtask.arg.arg_val, newtask.arg.arg_len, 
                           task.xdr_arg);
      if (*result != 0) {
         _dataUsage (info, DECREASE);  
         return TRUE;
      }
   
      /* call local schedule function */
      *result = scheduleTask (lsd, &task);
      _dataUsage (info, DECREASE);
   
      gdsDebug ("rpc call successful: schedule");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: getscheduledtask_1_svc			*/
/*                                                         		*/
/* Procedure Description: get task info from local scheduler		*/
/*                                                         		*/
/* Procedure Arguments: sched. descr., id				*/
/*                                                         		*/
/* Procedure Returns: id + task info, if successful, <0 if failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t getscheduledtask_1_svc (scheduler_r sd, int id, 
                     resultGetScheduledTask_r* result, 
                     struct svc_req* rqstp)
   {
      scheduler_t*	 lsd;	/* local scheduler descriptor */
      schedulertask_t	 task;	/* task info */
      sscheduler_info_t* info;	/* server scheduler info */
      scheduler_dtentry_t* dt;	/* pointer into dispatch table */
      int		 indx;	/* index into dispatch table */
   
      gdsDebug ("rpc call: get");
   
      /* reconstruct local scheduler descriptor */
      memcpy (&lsd, &sd, sizeof (scheduler_t*));
      if (lsd == NULL) {
         result->status = -1;
         return TRUE;
      }
   
      info = (sscheduler_info_t*) lsd->data;
      _dataUsage (info, INCREASE);
   
      /* call local get function */
      result->status = getScheduledTask (lsd, id, &task);
   
      /* if successful copy data */
      memset (&result->task, 0, sizeof (result->task));
      if (result->status >= 0) {
         result->task.flag = task.flag;
         result->task.priority = task.priority;
         result->task.timeout = task.timeout;
         result->task.tagtype = task.tagtype;
         result->task.synctype = task.synctype;
         result->task.syncval = task.syncval;
         result->task.waittype = task.waittype;
         result->task.waitval = task.waitval;
         result->task.repeattype = task.repeattype;
         result->task.repeatval = task.repeatval;
         result->task.repeatratetype = task.repeatratetype;
         result->task.repeatval = task.repeatval;
         result->task.repeatratetype = task.repeatratetype;
         result->task.repeatrate = task.repeatrate;
         result->task.repeatsynctype = task.repeatsynctype;
         result->task.repeatsyncval = task.repeatsyncval;
         strncpy (result->task.timetag, task.timetag, TIMETAG_LENGTH);
         result->task.timetag[TIMETAG_LENGTH-1] = 0;
         strncpy (result->task.waittag, task.waittag, TIMETAG_LENGTH);
         result->task.waittag[TIMETAG_LENGTH-1] = 0;
      	 /* just pass arg pointer back */
         result->task.arg.arg_len = sizeof (void*); 
         result->task.arg.arg_val = malloc (sizeof (void*));
         memcpy (result->task.arg.arg_val, &task.arg, sizeof (void*));
         result->task.arg_sizeof = 0;
      
      	 /* look for function id */
         result->task.func = 0;
         dt = info->dispatch->dt;
         for (indx = 0; indx < info->dispatch->numentries; indx++) {
         
            if (dt->func == task.func) {
               /* this is ambigeous if a task function is used by more 
                  than one dispatch table entry */
               result->task.func = dt->id;
               break;
            }
            dt++;
         }
      }
   
      _dataUsage (info, DECREASE);
      gdsDebug ("rpc call successful: get");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: removescheduledtask_1_svc			*/
/*                                                         		*/
/* Procedure Description: removes a task from local scheduler		*/
/*                                                         		*/
/* Procedure Arguments: sched. descr., id, terminate flag		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t removescheduledtask_1_svc (scheduler_r sd, int id, 
                     int terminate, int* result, struct svc_req* rqstp)
   {
      scheduler_t*	lsd;	/* local scheduler descriptor */
      sscheduler_info_t* info;	/* server scheduler info */
   
      gdsDebug ("rpc call: remove");
   
      /* reconstruct local scheduler descriptor */
      memcpy (&lsd, &sd, sizeof (scheduler_t*));
      if (lsd == NULL) {
         *result = -1;
         return TRUE;
      }
      info = (sscheduler_info_t*) lsd->data;
      _dataUsage (info, INCREASE);
   
      /* call local remove function */
      *result = removeScheduledTask (lsd, id, terminate);
      _dataUsage (info, DECREASE);
   
      gdsDebug ("rpc call successful: remove");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: waitforschedulertofinish_1_svc		*/
/*                                                         		*/
/* Procedure Description: waits for local scheduler to finish		*/
/*                                                         		*/
/* Procedure Arguments: sched. descr., timeout				*/
/*                                                         		*/
/* Procedure Returns: 0 if terminated, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t waitforschedulertofinish_1_svc (scheduler_r sd, 
                     tainsec_r timeout, int* result, 
                     struct svc_req* rqstp)
   {
      scheduler_t*	lsd;	/* local scheduler descriptor */
      sscheduler_info_t* info;	/* server scheduler info */
   
      gdsDebug ("rpc call: wait");
   
      /* reconstruct local scheduler descriptor */
      memcpy (&lsd, &sd, sizeof (scheduler_t*));
      if (lsd == NULL) {
         *result = 0;
         return TRUE;
      }
      info = (sscheduler_info_t*) lsd->data;
      _dataUsage (info, INCREASE);
   
      /* call local wait function */
      *result = waitForSchedulerToFinish (lsd, timeout);
      _dataUsage (info, DECREASE);
   
      gdsDebug ("rpc call successful: wait");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Remote Procedure Name: settagnotify_1_svc				*/
/*                                                         		*/
/* Procedure Description: time tag notify for local scheduler		*/
/*                                                         		*/
/* Procedure Arguments: sched. descr., tag, time			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if not			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t settagnotify_1_svc (scheduler_r sd, char* tag, 
                     tainsec_r time, int* result, struct svc_req* rqstp)
   {
      scheduler_t*	 lsd;	/* local scheduler descriptor */
      sscheduler_info_t* info;	/* server scheduler info */
   
      gdsDebug ("rpc call: set tag");
   
      /* reconstruct local scheduler descriptor */
      memcpy (&lsd, &sd, sizeof (scheduler_t*));
      if (lsd == NULL) {
         *result = -1;
         return TRUE;
      }
      info = (sscheduler_info_t*) lsd->data;
      _dataUsage (info, INCREASE);
   
   #ifdef DEBUG
      printf ("set tag %s (%li)\n", tag, (taisec_t) (time / _ONESEC));
   #endif
   
      /* local set tag function, but do not notify (i.e. call back) */
      *result = setSchedulerTag (lsd, tag, time, 1);
   
      _dataUsage (info, DECREASE);
      gdsDebug ("rpc call successful: set tag");
      return TRUE;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsscheduler_1_freeresult			*/
/*                                                         		*/
/* Procedure Description: frees memory of rpc call			*/
/*                                                         		*/
/* Procedure Arguments: rpc transport info, xdr routine for result,	*/
/*			pointer to result				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE if failed		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsscheduler_1_freeresult (SVCXPRT* transp, 
                     xdrproc_t xdr_result, caddr_t result)
   
   {
      (void) xdr_free (xdr_result, result);
      return TRUE;
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
   static void _dataUsage (sscheduler_info_t* data, _direction_t dir)
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
