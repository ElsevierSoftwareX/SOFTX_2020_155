static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: rpcinc							*/
/*                                                         		*/
/* Module Description: implements functions used by the gds rpc 	*/
/* interface								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <signal.h>
#include <sysLib.h>
#include "dtt/gdssock.h"

#else
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//TODO:
//#ifndef PORTMAP
//#include <netdir.h>
//#endif
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include "sockutil.h"
#endif

#include "dtt/gdsutil.h"
#define _RPC_HDR
#define _RPC_XDR
#include "dtt/gdstask.h"
#include "dtt/rpcinc.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _NETID		  net protocol used for rpc		*/
/*            _SHUTDOWN_DELAY	  wait period before shut down		*/
/*            SIG_PF		  signal function prototype 		*/
/*            _IDLE		  server idle state			*/
/*            _BUSY		  server busy state			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _NETID			"tcp"
#define _SHUTDOWN_DELAY		120		/* 120 s */
#ifndef SIG_PF
#define	SIG_PF 			void (*) (int)
#endif
#define	_IDLE 			0
#define	_BUSY	 		1
#define _INIT_SLEEP		1000000UL	/*   1 ms */


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: rpcCBService_t	rpc service task argument		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   struct rpcCBService_t {
      u_long* 		prognum;
      u_long 		progver;
      SVCXPRT** 	transport;
      rpc_dispatch_t	dispatch;
   };
   typedef struct rpcCBService_t rpcCBService_t;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: servermux		protects globals			*/
/*          serverState		state of server		 		*/
/*          shutdown		shutdown flag for port monitor serveres	*/
/*          runsvc		1 if rpc svc already runnning		*/
/*          defaultServerMode	the default server mode			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef OS_VXWORKS
   static mutexID_t		_servermux;
   static int			_serverState = _IDLE;
   static int*			_shutdown;
#endif

#ifndef OS_VXWORKS
   static int			runsvc = 0;
#endif
//TODO:
#define PORTMAP 1
#if !defined (OS_VXWORKS) && !defined (PORTMAP)
   static const int defaultServerMode = RPC_SVC_MT_AUTO;
#endif
   FILE*		gdslog;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcGetHostaddress				*/
/*                                                         		*/
/* Procedure Description: returns a host address given the host name	*/
/*                                                         		*/
/* Procedure Arguments: hostname, inet address (return value)		*/
/*                                                         		*/
/* Procedure Returns: 0 if succesful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcGetHostaddress (const char* hostname, struct in_addr* addr)
   {
      if (hostname == NULL) {
         addr->s_addr = inet_addr ("127.0.0.1");
         return 0;
      }
   
      /* determine host address */
   #ifdef OS_VXWORKS
      if (((addr->s_addr = inet_addr (hostname)) == ERROR) &&
         ((addr->s_addr = hostGetByName ((char*) hostname)) == ERROR)) {
         addr->s_addr = 0;
         return -1;
      }
      else {
         return 0;
      }
   #else
      /* use DNS if necessary */
      return nslookup (hostname, addr);
#if 0
      {
         struct hostent hostinfo;	/* host info */
         char		buf[1000];	/* buffer for host info */
         int		i;		/* buffer length */
      
         if (gethostbyname_r (hostname, &hostinfo, 
            buf, sizeof (buf), &i) == NULL) {
            addr->s_addr = 0;
            return -1;	
         }
         else {
            memcpy (addr, hostinfo.h_addr_list[0], 
                   sizeof (struct in_addr));
            return 0;
         }
      }
#endif
      #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcGetLocaladdress				*/
/*                                                         		*/
/* Procedure Description: returns the local address 			*/
/*                                                         		*/
/* Procedure Arguments: inet address (return value)			*/
/*                                                         		*/
/* Procedure Returns: 0 if succesful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcGetLocaladdress (struct in_addr* addr)
   {  
      char		myname[256];	/* local host name */
   
      if ((gethostname (myname, sizeof (myname)) != 0) ||
         (rpcGetHostaddress (myname, addr) != 0)) {
         return -1;
      }
      else {
         return 0;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcGetClientaddress				*/
/*                                                         		*/
/* Procedure Description: returns a client address of the rpc call	*/
/*                                                         		*/
/* Procedure Arguments: service handle, inet address (return value)	*/
/*                                                         		*/
/* Procedure Returns: 0 if succesful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcGetClientaddress (const SVCXPRT* xprt,
                     struct in_addr* clntaddr)
   {
      /* determine network address client address */
   #if defined (OS_VXWORKS) || defined (PORTMAP)
      {  
         struct sockaddr_in*	addr;
         addr = svc_getcaller ((SVCXPRT*) xprt);
         *clntaddr = addr->sin_addr;
      }
   #else
      {
         struct netbuf*		nbuf;	/* address as return by rpc */
         char*			addr;	/* universal address */
         struct netconfig*	nconf;	/* netdir info about _NETID */
         char*			p;	/* for modifying addr */
         int			dot;	/* counts the 'dots' in addr */
      
         /* get caller address */
         nbuf = svc_getrpccaller ((SVCXPRT*) xprt);
         if (nbuf == NULL) {
            return -1;
         }
      	 /* get net config entry */
         nconf = getnetconfigent (_NETID);
         if (nconf == (struct netconfig*) NULL) {
            gdsDebug ("can't find tcp netid");
            netdir_free (nbuf, ND_ADDR);
            return -2;
         }
      	 /* convert transport address into universal address */
         addr = taddr2uaddr (nconf, nbuf);
         freenetconfigent (nconf);
         netdir_free (nbuf, ND_ADDR);
      	 /* get rid of the port number non-sense */
         p = addr;
         dot = 0;
         while (*p != '\0') {
            if (*p == '.') {
               dot++;
               if (dot == 4) {
                  *p = '\0';
                  break;
               }
            }
            p++;
         }
         /* now converting string into network address */
         clntaddr->s_addr = inet_addr (addr);
         free (addr);
         if (clntaddr->s_addr == -1) {
            return -3;
         }
      }
   #endif
   
      return 0;
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
      int 		asize = sizeof (saddr);
      int 		rpcfdtype;
      int 		ssize = sizeof (int);
   
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
         openlog ("gds daemon", LOG_PID, LOG_DAEMON);
         return 1;
      }
      else {
         return 0;
      }
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcInitializeServer				*/
/*                                                         		*/
/* Procedure Description: initializes rpc server stuff			*/
/*                                                         		*/
/* Procedure Arguments: rpcpmstart - pointer to port monitor boolean	*/
/*                      svc_fg - whether to start service in foreground	*/
/*                      svc_mode - sets rpc service mt mode		*/
/*                      transp - returned transport handle		*/
/*                      proto - returned protocol type			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcInitializeServer (int* rpcpmstart, int svc_fg, int svc_mode,
                     SVCXPRT** transp, int* proto) 
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
   
      /* init rpc on VxWorks */
   #ifdef OS_VXWORKS
      if (rpcTaskInit () != 0) {
         return -2;
      }
   #endif
   
      /* find out whether started by port monitor */
   #ifdef OS_VXWORKS
      *rpcpmstart = 0;
   #else
      *rpcpmstart = checkStdInHandle ();
      if (*rpcpmstart == -1) {
         return -3;
      }
   #endif
   
      /* fork on UNIX machines */
   #if !defined (OS_VXWORKS)
      if ((*rpcpmstart == 0) && (svc_fg == 0)) {
         int 		size;
         struct rlimit	rl;
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
         if (*rpcpmstart == 1) {
            sock = 0;
            *proto = 0;
         }
         else {
            sock = RPC_ANYSOCK;
            *proto = IPPROTO_TCP;
         }
      
         /* create rpc service handle: tcp only */
         *transp = svctcp_create (sock, 0, 0);
         if (*transp == NULL) {
            gdsError (GDS_ERR_PROG, "cannot create tcp service");
            return -6;
         }
      }
   #else
      /* rpc bind service */
      {
         struct netconfig* 	nconf = NULL;
      
         if (*rpcpmstart == 1) {
            /* get netid */
            nconf = getnetconfigent (_NETID);
            if (nconf == NULL) {
               gdsError (GDS_ERR_PROG, "tcp service not available");
               return -7;
            }
            /* create rpc service handle for fd 0 */
            *transp = svc_tli_create (0, nconf, NULL, 0, 0);
            freenetconfigent(nconf);
            if (*transp == NULL) {
               gdsError (GDS_ERR_PROG, "cannot create service handle");
               return -8;
            }
         }
         else {
            /* do not need service handle */
         }
      }
   #endif
   
      /* if on UNIX, set the rpc mt mode */
   #if !defined (OS_VXWORKS) && !defined (PORTMAP)
      {
         int		mode;
         mode = (svc_mode == -1) ? defaultServerMode : svc_mode;
         if (!rpc_control (RPC_SVC_MTMODE_SET, &mode)) {
            gdsError (GDS_ERR_PROG, "unable to set server mode");
            return -9;
         }
      }
   #endif
   
      return 0;
   }


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
      if (((_shutdown != NULL) && (*_shutdown != 0)) ||
         (MUTEX_TRY (_servermux) != 0)) {
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
   
      MUTEX_RELEASE (_servermux);
      (void) signal (SIGALRM, (SIG_PF) closedown);
      (void) alarm (_SHUTDOWN_DELAY / 2);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcSetServerBusy				*/
/*                                                         		*/
/* Procedure Description: set rpc server into busy state		*/
/*                                                         		*/
/* Procedure Arguments: busyflag					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void rpcSetServerBusy (int busyflag)
   {
   #ifndef OS_VXWORKS
      MUTEX_GET (_servermux);
      _serverState = (busyflag != 0) ? _BUSY : _IDLE;
      MUTEX_RELEASE (_servermux);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcStartServer				*/
/*                                                         		*/
/* Procedure Description: starts an rpc server				*/
/*                                                         		*/
/* Procedure Arguments: rpcpmstart - port monitor boolean		*/
/*                      shutdown - shutdown if zero	 		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void rpcStartServer (int rpcpmstart, int* shutdown) 
   {
      /* install close down procedure if started by a port monitor */
   #ifndef OS_VXWORKS
      if (rpcpmstart > 0) {
         MUTEX_CREATE (_servermux);
         _shutdown = shutdown;
         (void) signal (SIGALRM, (SIG_PF) closedown);
         (void) alarm (_SHUTDOWN_DELAY / 2);
      }
   #endif
   
      /* wait for rpc calls */
      svc_run();
      /* never reached */
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcRegisterService				*/
/*                                                         		*/
/* Procedure Description: registers an rpc service			*/
/*                                                         		*/
/* Procedure Arguments: rpcpmstart - port monitor boolean		*/
/*                      transp - transport handle			*/
/*                      proto - protocol type				*/
/*                      prognum - rpc program number 			*/
/*                      progver - rpc version number			*/
/*                      service - service function			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcRegisterService (int rpcpmstart, SVCXPRT* transp, int proto,
                     unsigned long prognum, unsigned long progver, 
                     rpc_dispatch_t service)
   {
   #if defined (OS_VXWORKS) || defined (PORTMAP)
      /* port map service */
      if (rpcpmstart != 1) {
         /* stand alone */
         (void) pmap_unset (prognum, progver);
      }
      if (!svc_register (transp, prognum, progver, service, proto)) {
         gdsError (GDS_ERR_PROG, "unable to create rpc service");
         return -1;
      }
   #else
      /* rcp bind service */
      if (rpcpmstart == 1) {
         /* port monitor */
         if (!svc_reg (transp, prognum, progver, service, 0)) {
            gdsError (GDS_ERR_PROG, "unable to create rpc service");
            return -1;
         }
      }
      else {
         /* stand alone */
         if (!svc_create (service, prognum, progver, _NETID)) {
            gdsError (GDS_ERR_PROG, "unable to create rpc service");
            return -1;
         }
      }
   #endif
   
      return 0;
   }


/* :IMPORTANT: Since rpc handle have a task context in VxWorks the
   task which calls svc_run() should also be the one which registers
   the service handle */ 


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: rpcCBService				*/
/*                                                         		*/
/* Procedure Description: rpc callback service				*/
/*                                                         		*/
/* Procedure Arguments: scheduler descriptor				*/
/*                                                         		*/
/* Procedure Returns: void (sets callbacknum if successful)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void rpcCBService (rpcCBService_t* arg)
   {
      u_long		callbacknum;
   
      /* initialize rpc for VxWorks */
   #ifdef OS_VXWORKS
      rpcTaskInit ();
   #endif
   
      /* register service */
      if (rpcRegisterCallback (&callbacknum, arg->progver, 
         arg->transport, arg->dispatch) != 0) {
         *arg->prognum = (u_long) -1;
         return;
      }
      /* calback is registered */
      *arg->prognum = callbacknum;
   
      /* if on UNIX, set the rpc mt mode */
   #if !defined (OS_VXWORKS) && !defined (PORTMAP)
      {
         int 		max = 30;
         rpc_control (RPC_SVC_MTMODE_SET, (void*) &defaultServerMode);
         rpc_control (RPC_SVC_THRMAX_SET, (void*) &max);
      }
   #endif
   
      /* invoke rpc service; never to return */
   #ifdef OS_VXWORKS
      svc_run();
   #else
      if (runsvc > 0) {
         return;
      }
      runsvc = 1;
      svc_run();
   #endif
      gdsDebug ("rpc service has returned");
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcStartCallbackService			*/
/*                                                         		*/
/* Procedure Description: starts a rpc callback service			*/
/*                                                         		*/
/* Procedure Arguments: program number(returned), program version,  	*/
/*			service handle (returned), task ID (returned),	*/
/*                      priotity, dispatch function			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcStartCallbackService (u_long* prognum, u_long progver, 
                     SVCXPRT** transport, taskID_t* tid, int priority,
                     rpc_dispatch_t dispatch)
   {
      struct timespec	tick = {0, _INIT_SLEEP};
      rpcCBService_t	arg;		/* parameters for rpc process */
      int		attr;		/* task attribute */
   
      /* fill argument */
      if (prognum == NULL) {
         return -1;
      }
      *prognum = 0;
      if (transport != NULL) {
         *transport = NULL;
      }
      arg.prognum = prognum;
      arg.progver = progver;
      arg.transport = transport;
      arg.dispatch = dispatch;
   
      /* register and start callback service */
   #ifdef OS_VXWORKS
      attr = 0;
   #else
      attr = PTHREAD_CREATE_JOINABLE | PTHREAD_SCOPE_SYSTEM;
   #endif
      if (taskCreate (attr, priority, tid, "trpcCB", 
         (taskfunc_t) &rpcCBService, (taskarg_t) &arg) != 0) {
         return -2;
      }
   
     /* wait for task to finish registration */
      while (*prognum == 0) {
         nanosleep (&tick, NULL);
      }
      if (*prognum == (u_long) -1) {
         return -3;
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcStopCallbackService			*/
/*                                                         		*/
/* Procedure Description: stops an rpc callback service			*/
/*                                                         		*/
/* Procedure Arguments: program number(returned), program version,  	*/
/*			service handle (returned)			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcStopCallbackService (u_long prognum, u_long progver, 
                     SVCXPRT* transport, taskID_t tid) 
   {
      /* don't make suicide */
      if (tid == 0) {
         return -1;
      }
      /* unregister service and delete task */
   #ifdef OS_VXWORKS
      svc_unregister (prognum, progver);
      taskDelete (tid);
   #else
   #ifdef PORTMAP
      svc_unregister (prognum, progver);
   #else
      svc_unreg (prognum, progver);
   #endif
      /* pthread_cancel (tid); */
      /* pthread_join (tid, NULL);*/
      /* svc_destroy (transport); */
   #endif
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcRegisterCallback				*/
/*                                                         		*/
/* Procedure Description: registers a callback rpc interface		*/
/*                                                         		*/
/* Procedure Arguments: dispatch function, pointer to store program 	*/
/*			number, version number, transport id		*/
/*                                                         		*/
/* Procedure Returns: service handle if successful, NULL if failed	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcRegisterCallback (u_long* program, u_long version, 
                     SVCXPRT** transport, rpc_dispatch_t dispatch)
   {
      SVCXPRT*		transp;		/* transport handle */
      u_long		prognum;	/* transient rpc program # */
   
   /* TS-RPC calls */
   #if defined(OS_VXWORKS) || defined (PORTMAP)
      {
      /* create transport handle */
         transp = svctcp_create (RPC_ANYSOCK, 0, 0);
         if (transp == NULL) {
            gdsDebug ("cannot create tcp service");
            return -1;
         }
      
      	 /* now try to register */
         prognum = 0x40000000;
         while ((prognum < 0x60000000) && (svc_register (transp, prognum,
               version, dispatch, IPPROTO_TCP) == 0)) {
            prognum++;
         }
         if (prognum >= 0x60000000) {
            svc_destroy (transp);
            gdsDebug ("can't register rpc callback service");
            return -2;
         }
      }
   
   /* TI-RPC calls */
   #else
      {
         struct netconfig*	nconf;
      
         /* get net config entry */
         nconf = getnetconfigent (_NETID);
         if (nconf == (struct netconfig*) NULL) {
            gdsDebug ("can't find tcp netid");
            return -3;
         }
      
         /* create transport handle */
         transp = svc_tli_create (RPC_ANYFD, nconf, 
                                 (struct t_bind*) NULL, 0, 0);
         if (transp == (SVCXPRT*) NULL) {
            freenetconfigent (nconf);
            gdsDebug ("can't create rpc transport handle");
            return -1;
         }
      
      	 /* now try to register */
         prognum = 0x40000000;
         while ((prognum < 0x60000000) && (svc_reg (transp, prognum, 
               version, dispatch, nconf) == 0)) {
            prognum++;
         }
         freenetconfigent (nconf);
         if (prognum >= 0x60000000) {
            svc_destroy (transp);
            gdsDebug ("can't register rpc callback service");
            return -2;
         }
      }
   #endif
   
      /* copy results and return */
      if (program != NULL) {
         *program = prognum;
      }
      if (transport != NULL) {
         *transport = transp;
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rpcProbe					*/
/*                                                         		*/
/* Procedure Description: probes an rpc interface			*/
/*                                                         		*/
/* Procedure Arguments: hostname, prog. num, vers. num, netid, timeout 	*/
/*                      client (return)                   		*/
/*                     			                   		*/
/* Procedure Returns: 1 if successful, 0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rpcProbe (const char* host, const u_long prognum, 
                const u_long versnum, const char* nettype, 
                const struct timeval* timeout, CLIENT** client)
   {
      CLIENT*		clnt;
   
   #if defined(OS_VXWORKS) || defined (PORTMAP)	
      clnt = clnt_create ((char*) host, prognum, versnum, (char*) nettype);
   #else
      clnt = clnt_create_timed (host, prognum, versnum, nettype, timeout);
   #endif
      if (client != NULL) {
         *client = clnt;
      } 
      else if (clnt != NULL) {
         clnt_destroy (clnt);
      }
      return (clnt != NULL);
   }


#ifdef OS_VXWORKS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: xdr_longlong_t				*/
/*                                                         		*/
/* Procedure Description: xdr for long long (VxWorks patch)		*/
/*                                                         		*/
/* Procedure Arguments: XDR, long long object				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t xdr_longlong_t (register XDR* xdrs, longlong_t* objp) 
   {
      long		msb;
      unsigned long	lsb;
   
      if (xdrs->x_op == XDR_ENCODE) {
         msb = (long) (*objp >> 32);
         lsb = (unsigned long) (*objp & 0x00000000FFFFFFFFLL);
         if (!xdr_long (xdrs, &msb))
            return (FALSE);
         if (!xdr_u_long (xdrs, &lsb))
            return (FALSE);
         return (TRUE);
      }
      else if (xdrs->x_op == XDR_DECODE) {
         if (!xdr_long (xdrs, &msb))
            return (FALSE);
         if (!xdr_u_long (xdrs, &lsb))
            return (FALSE);
         *objp = ((longlong_t) msb << 32) + (longlong_t) lsb;
         return (TRUE);
      }
      if (!xdr_long (xdrs, &msb))
         return (FALSE);
      if (!xdr_u_long (xdrs, &lsb))
         return (FALSE);
      return (TRUE);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: xdr_u_longlong_t				*/
/*                                                         		*/
/* Procedure Description: xdr for unsigned long long (VxWorks patch)	*/
/*                                                         		*/
/* Procedure Arguments: XDR, long long object				*/
/*                                                         		*/
/* Procedure Returns: TRUE if successful, FALSE otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   bool_t xdr_u_longlong_t (register XDR* xdrs, u_longlong_t* objp) 
   {
      unsigned long	msb;
      unsigned long	lsb;
   
      if (xdrs->x_op == XDR_ENCODE) {
         msb = (unsigned long) (*objp >> 32);
         lsb = (unsigned long) (*objp & 0x00000000FFFFFFFFLL);
         if (!xdr_u_long (xdrs, &msb))
            return (FALSE);
         if (!xdr_u_long (xdrs, &lsb))
            return (FALSE);
         return (TRUE);
      }
      else if (xdrs->x_op == XDR_DECODE) {
         if (!xdr_u_long (xdrs, &msb))
            return (FALSE);
         if (!xdr_u_long (xdrs, &lsb))
            return (FALSE);
         *objp = ((u_longlong_t) msb << 32) + (u_longlong_t) lsb;
         return (TRUE);
      }
      if (!xdr_u_long (xdrs, &msb))
         return (FALSE);
      if (!xdr_u_long (xdrs, &lsb))
         return (FALSE);
      return (TRUE);
   }

#endif
