static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: convserver						*/
/*                                                         		*/
/* Module Description: implements functions for configuration server	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef DEBUG
#define DEBUG
#endif

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <ioLib.h>
#include <selectLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdstask.h"
#include "dtt/gdssock.h"
#include "dtt/confserver.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _TASK_PRIORITY	  task priority				*/
/* 	      _TASK_NAME	  task name				*/
/* 	      _MAGICPORT	  well-known port address		*/
/*            _PING_TIMEOUT	timeout for ping (sec)			*/
/*            _SHUTDOWN_DELAY	  wait period before shut down		*/
/*            SIG_PF		  signal function prototype 		*/
/*            _IDLE		  server idle state			*/
/*            _BUSY		  server busy state			*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _TASK_PRIORITY		99
#define _TASK_NAME		"tConf"
#define _MAGICPORT		5355
#define _PING_PORT 		7
#define _PING_TIMEOUT		1.0
#ifndef OS_VXWORKS
#define _SHUTDOWN_DELAY		120		/* 120 s */
#define	_IDLE 			0
#define	_BUSY	 		1
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: confserver_init	server init flag			*/
/* 	    services		list of services			*/
/*          snum		number of services		 	*/
/*          sock		server socket		      		*/
/*          serverState		state of server		 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int		confserver_init = 0;
   static confServices*	services;
   static int		snum;
   static int		sock;
#ifndef OS_VXWORKS
   static int		_serverState = _BUSY;
#endif
   static taskID_t 	confTID = 0;
   static taskID_t 	respTID = 0;
#if 1
   static struct timespec	delay_time ;
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: 								*/
/*	callback_t		callback argument			*/
/*      								*/
/*----------------------------------------------------------------------*/
   struct callback_t {
      /* service handle */
      const confServices* 	service;
      /* sender address */
      struct sockaddr_in 	name;
      /* argument */
      char			arg[2048];
   };
   typedef struct callback_t callback_t;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: sendAnswer					*/
/*                                                         		*/
/* Procedure Description: answer service requests			*/
/*                                                         		*/
/* Procedure Arguments: id, sender address, answer string		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void sendAnswer (int id, struct sockaddr_in* name, 
                     const char* answer)
   {
   
      int		nbytes;		/* # of bytes send */
      int		size;		/* size of message */
      int		len;		/* length of send block */
      char		message[2048];	/* send message */
   
      *((long*) message) = htonl (id);
      size = strlen (answer);
      len = 0;

#if 1
#else
      int lfd = open("/var/log/awglock", O_RDWR | O_CREAT);
      if (lfd < 0) {
	perror("Unable to open/create lock file /var/log/awglock");
	return;
      }
#endif
      while (len < size) {
         nbytes = size - len;
         if (nbytes <= 1024) {
            memcpy (message + 4, answer + len, nbytes);
         }
         else {
            /* break answers at space */
            nbytes = 1024;
            memcpy (message + 4, answer + len, nbytes);
            while ((nbytes > 0) && (message [3 + nbytes] != ' ')) {
               nbytes--;
            }
            if (nbytes == 0) {
               /*close (sock);*/
#if 1
#else
	       close(lfd);
#endif
               return;
            }
         }
   	 //printf("Sending '%s' in reply\n", message);
	 nbytes += 4;
	 int nsent;
#if 1
	 nanosleep(&delay_time, (struct timespec *) NULL) ;
#else
 	 flock(lfd, LOCK_EX);
#endif
	 for(;;) {
           nsent = sendto (sock, message, nbytes, 0, (struct sockaddr*) name, sizeof (struct sockaddr_in));
	   if (nsent < 0) perror("sendto");
	   if (nsent == nbytes) break;
	 }
         len += nbytes;
#if 1
#else
 	 flock(lfd, LOCK_UN);
#endif
      }
#if 1
#else
      close(lfd);
#endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: serviceCallback				*/
/*                                                         		*/
/* Procedure Description: service callback				*/
/*                                                         		*/
/* Procedure Arguments: callback argument				*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void serviceCallback (callback_t* cb) 
   {
      char*		ret;		/* answer */
   
      if ((cb == NULL) || (cb->service == NULL)) {
         return;
      }
   
      ret = (*cb->service->answer) (cb->service, cb->arg);
      if (ret != NULL) {
         sendAnswer (cb->service->id, &cb->name, ret);
         free (ret);
      }
      free (cb);
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
      if (_serverState == _IDLE) {
         exit (0);
      } 
      else {
         _serverState = _IDLE;
      }
   
      (void) signal (SIGALRM, closedown);
      (void) alarm (_SHUTDOWN_DELAY / 2);
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: waitForRequests				*/
/*                                                         		*/
/* Procedure Description: waits for service requests			*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: never						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void waitForRequests (int flag)
   {
      struct sockaddr_in name;		/* sender address */
   #ifndef OS_VXWORKS
      socklen_t		size;		/* size of address */
   #else
      int		size;		/* size of address */
   #endif
      char 		message[2048];	/* recv datagram */
      int		nbytes;		/* # of recv. bytes */
      int		id;		/* request id */
      char*		arg;		/* request argument */
      int		i;		/* service index */
   #ifndef OS_VXWORKS
      int		attr;		/* thread attribute */
   #endif
      callback_t*	cb;		/* thread parameter */
   
      /* install shutdown procedure if started by a port monitor */
   #ifndef OS_VXWORKS
      if (flag == 2) {
         (void) signal (SIGALRM, closedown);
         (void) alarm (_SHUTDOWN_DELAY / 2);
      }
   #endif
   
      while (1) {
         /* Wait for a datagram.  */
         size = sizeof (name);
         nbytes = recvfrom (sock, message, 2048, 0,
                           (struct sockaddr*) &name, &size);
      
         /* error */
         if (nbytes < 0) {
            if (errno == EINTR) {
               continue;
            }
            else {
               return;
            }
         } 
         /* received a request */
         else if (nbytes >= 4) {
         #ifndef OS_VXWORKS
            _serverState = _BUSY;
         #endif
            id = ntohl (*((long*) message));
            arg = message + 4;
            if (nbytes < 2044) {
               arg[nbytes] = 0;
            }
            /* answer */
            for (i = 0; i < snum; i++) {
               if (services[i].id == id) {
                  cb = malloc (sizeof (callback_t));
                  if (cb == NULL) {
                     continue;
                  }
                  cb->service = services + i;
                  cb->name = name;
                  strncpy (cb->arg, arg, 2047);
                  cb->arg [2047] = 0;
                  printf ("CONF SERVICE: %s (%p)\n", cb->service->user, 
                         cb->service->user);
               #ifdef OS_VXWORKS
                  serviceCallback (cb);
               #else
                  if (cb->service->answer == &stdAnswer) {
                     serviceCallback (cb);
                  }
                  else {
                     attr = PTHREAD_CREATE_DETACHED;
                     taskCreate (attr, _TASK_PRIORITY, &respTID, _TASK_NAME, 
                                (taskfunc_t) serviceCallback, 
                                (taskarg_t) cb);
                  }
               #endif
               }
            }
         }
      }
   
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: conf_server					*/
/*                                                         		*/
/* Procedure Description: starts a configuration server task		*/
/*                                                         		*/
/* Procedure Arguments: list of services, # of services, async flag	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int conf_server (const confServices confs[], int num, int flag)
   {
      struct sockaddr_in name;	/* socket name */
      int		attr;	/* task creation attribute */
      int		i;	/* index */
      int 		n;	/* number of services */
      confServices* 	c;      /* list of services */
      confServices* 	tmp;    /* temp list of services */
      char*		ch;	/* conf string */
      char*		p;	/* conf string */
   
      /* add services, if already initialized */
      if (confserver_init != 0) {
         /* multiline answer */
         if ((snum == 1) && (num == 1) && 
            (services[0].id == confs[0].id) && 
            (services[0].answer == confs[0].answer)) {
            ch = malloc (strlen (services[0].user) +
                        strlen (confs[0].user) + 10);
            sprintf (ch, "%s\n%s", services[0].user, confs[0].user);
            p = services[0].user;
            services[0].user = ch;
            free (p);
            /*printf ("NEW CONF SERVICE STRING: %s (%x)\n", 
         	      services[0].user, (int)ch);*/
         }
         /* normal answer */
         else {
            n = snum + num;
            c = calloc (n, sizeof (confServices));
            for (i = 0; i < snum; ++i) {
               c[i] = services[i];
            }
            for (i = 0; i < num; ++i) {
               c[snum + i] = confs[i];
            }
            tmp = services;
            services = c;
            free (tmp);
            snum = num;
         }
         return 0;
      }
   
      /* initialize servicxs */
      c = calloc (num, sizeof (confServices));
      for (i = 0; i < num; ++i) {
         c[i] = confs[i];
         c[i].user = malloc (strlen (confs[i].user) + 10);
         strcpy (c[i].user, confs[i].user);
         /*printf ("CONF SERVICE STRING: %s (%x)\n", c[i].user, (int)c[i].user);*/
      }
      services = c;
      snum = num;
   
      /* create socket */
      if (flag == 2) {
         /* port monitor: use stdin */
         sock = 0;
      } 
      else {
         /* create new server socket */
         sock = socket (PF_INET, SOCK_DGRAM, 0);
         if (sock == -1) {
            return -2;
         }
      
         { int nset = 1;
           if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, 
               (char*) &nset, sizeof (nset)) == -1) {
                 close (sock);
                 return -2;
	   }
         }

         /* connect socket to IP/port */
         name.sin_family = AF_INET;
         name.sin_port = htons (_MAGICPORT);
         name.sin_addr.s_addr = htonl (INADDR_ANY);
         if (bind (sock, (struct sockaddr*) &name, sizeof (name))) {
            return -3;
         }
      }
      confserver_init = 1;

   /* Print version ID to log. */
   printf("conf_server %s\n", versionId) ;
#if 1
   /* This may look a bit ugly.  The answer should be a string of the form
    * "awg a b hostname prognum progver...." where a is a unique value
    * which is 0 <= a < TP_MAX_NODE (128 in advanced ligo).
    * Attempt to get this value to use as a delay to keep all of the 
    * servers from responding at the same time. 
    */
      int 		delay = -1 ;

      sscanf(confs[0].user, "%*s%d", &delay) ;
      if (!(delay >= 0 && delay < 128))
      {
	 /* Assign some other number to delay */
	 delay = (getpid() % 100) + 128 ;
	printf("Delay based on PID = %d\n", delay) ;
      }
      else
      {
	 delay *= 2 ;
	 delay += 10 ;
	 printf("Delay based on DCUID * 2 + 10 = %d\n", delay) ;
      }

      delay_time.tv_sec = 0 ;
      delay_time.tv_nsec = delay * 1000000 ;
#endif
   
      if (flag == 1) {
      #ifdef OS_VXWORKS
         attr = VX_FP_TASK;
      #else
         attr = PTHREAD_CREATE_DETACHED;
      #endif
         if (taskCreate (attr, _TASK_PRIORITY, &confTID, _TASK_NAME, 
            (taskfunc_t) waitForRequests, 0) < 0) {
            return -4;
         }
      }
      else {
         waitForRequests (flag);
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: stdAnswer					*/
/*                                                         		*/
/* Procedure Description: returns info string				*/
/*                                                         		*/
/* Procedure Arguments: conf service, argument				*/
/*                                                         		*/
/* Procedure Returns: info						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* stdAnswer (const confServices* conf, const char* arg)
   {
      if ((conf == NULL) || (conf->id != 0)) {
         return NULL;
      }
      return strdup (conf->user);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: stdPingAnswer				*/
/*                                                         		*/
/* Procedure Description: returns info string if unit can be pinged	*/
/*                                                         		*/
/* Procedure Arguments: conf service, argument				*/
/*                                                         		*/
/* Procedure Returns: info if unit is alive				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* stdPingAnswer (const confServices* conf, const char* arg)
   {
      char		hostname[1024];
   
      if ((conf == NULL) || (conf->id != 0)) {
         return NULL;
      }
      sscanf (conf->user, "%*s%*s%*s%1023s", hostname);
      if (ping (hostname, _PING_TIMEOUT)) {
         return strdup (conf->user);
      }
      else {
         return NULL;
      }
   }


