/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: confinfo						*/
/*                                                         		*/
/* Module Description: implements functions for controlling the AWG	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/*#ifndef DEBUG
#define DEBUG
#endif*/

/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <semLib.h>
#include <ioLib.h>
#include <selectLib.h>
#include <taskLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <sysLib.h>
#include <timers.h>

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdstask.h"
#include "dtt/confinfo.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: 								*/
/* 	      _MAGICPORT	  well-known port address		*/
/* 	      _TIMEOUT	  	  timeout for request			*/
/* 	      _MAX		  maximum # of answers			*/
/*            _DEFAULT_TIMEOUT	  default timeout (1 sec)		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _MAGICPORT		5355
#define _TIMEOUT		(_ONESEC / 4)
#define _MAX			100
#define _DEFAULT_TIMEOUT	1.5


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: confbuf	list of configuration strings          		*/
/*          conftime	time when conf was last read	  		*/
/*          confmux	mutex to protect conf list	   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static char		confbuf[_MAX * 512];
   static tainsec_t	conftime = 0;
   static mutexID_t	confmux;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initConfinfo		init the configuration information API	*/
/*	finiConfinfo		cleanup of the conf. information API	*/
/*      								*/
/*----------------------------------------------------------------------*/
   __init__(initConfinfo);
#ifndef __GNUC__
#pragma init(initConfinfo)
#endif
   __fini__(finiConfinfo);
#ifndef __GNUC__
#pragma fini(finiConfinfo)
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Inernal Procedure Name: sortConfs					*/
/*                                                         		*/
/* Procedure Description: compares 2 configuration list entries		*/
/*                                                         		*/
/* Procedure Arguments: entry 1, entry 2				*/
/*                                                         		*/
/* Procedure Returns: 0 if equal, <0 if smaller, >0 if larger		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int compareConfs (const char** s1, const char** s2)
   {
      return gds_strcasecmp (*s1, *s2);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getConfInfo					*/
/*                                                         		*/
/* Procedure Description: get configuration/services information	*/
/*                                                         		*/
/* Procedure Arguments: info id, timeout				*/
/*                                                         		*/
/* Procedure Returns: list of configuration strings			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   const char* const* getConfInfo (int id, double timeout)
   {
      const char* const* p;
   
      /* :IMPORTANT: TAInow may call getConfInfo the first time 
          it is called */
      TAInow();
   
      MUTEX_GET (confmux);
      if ((timeout < 1E-9) && (conftime != 0)) {
         p = (const char* const*) confbuf;
      }
      else {
         p = getConfInfo_r (id, timeout, confbuf, sizeof (confbuf));
         conftime = 1;
      }
      MUTEX_RELEASE (confmux);
      return p;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: getConfInfo					*/
/*                                                         		*/
/* Procedure Description: get configuration/services information	*/
/*                                                         		*/
/* Procedure Arguments: info id, timeout				*/
/*                                                         		*/
/* Procedure Returns: list of configuration strings			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   const char* const* getConfInfo_r (int id, double timeout, 
   char* buffer, int len)
   {
      char**		p;		/* list index */
      int		sock;		/* socket handle */
      struct sockaddr_in name;		/* internet address */
      socklen_t		size;		/* size of address */
      int 		nset;		/* # of selected descr. */
      fd_set 		readfds;	/* socket descr. list */
      struct timeval 	wait;		/* timeout wait */
      int		nbytes;		/* # of bytes in buffer */
      char		buf[1024];	/* buffer */
      tainsec_t		start;		/* start time */
      tainsec_t		diff;		/* time till timeout */
      int		num;		/* # of answers */
      int		i;		/* answer index */
      int		j;		/* answer index */
      int		left;		/* bytes left in buffer */
      char*		b;		/* buffer pointer */
      char*		q;		/* temp. ptr to handle m.line */
      char*		qq;		/* temp. ptr to handle m.line */
      char*		sender;		/* sender's address */
   
      if ((id != 0) || ((_MAX+1) * sizeof (char*) >= len)) {
         return NULL;
      }
   
      /* setup conf info */
      p = (char**) buffer;
      *p = NULL;
      b = buffer + (_MAX+1) * sizeof (char*);
      left = len - (_MAX+1) * sizeof (char*);
   
      /* create a socket */
      sock = socket (PF_INET, SOCK_DGRAM, 0);
      if (sock == -1) {
         return NULL;
      }
      /* enable broadcast */
      nset = 1;
      if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, 
      (char*) &nset, sizeof (nset)) == -1) {
         close (sock);
         return NULL;
      }
   
      /* bind socket */
      name.sin_family = AF_INET;
      name.sin_port = 0;
      name.sin_addr.s_addr = htonl (INADDR_ANY);
      if (bind (sock, (struct sockaddr*) &name, sizeof (name))) {
         close (sock);
         return NULL;
      }
      /* set broadcast address */
      name.sin_family = AF_INET;
      name.sin_port = (short) htons (_MAGICPORT);
      name.sin_addr.s_addr = htonl (INADDR_BROADCAST);
   
      /* send request */
      *((long*) buf) = htonl (0);
      nbytes = sendto (sock, buf, 4, 0, 
         (struct sockaddr*) &name, 
         sizeof (struct sockaddr_in));
      if (nbytes < 0) {
         close (sock);
         return NULL;
      }
   
      /* wait for answers */
      start = TAInow();
      num = 0;
      if (timeout < 0) {
         timeout = fabs (timeout);
      } 
      if (fabs (timeout) < 1E-9) {
         timeout = _DEFAULT_TIMEOUT;
      }
      while (((diff = start + (tainsec_t)(timeout * 1E9) - 
      TAInow()) > 0) && (num < _MAX)) {
         /* poll socket */
         FD_ZERO (&readfds);
         FD_SET (sock, &readfds);
         wait.tv_sec = diff / _ONESEC;
         wait.tv_usec = (diff % _ONESEC) / 1000;
         nset = select (FD_SETSIZE, &readfds, 0, 0, &wait);
         if (nset < 0) {
            close (sock);
            return NULL;
         }
         else if (nset == 0) {
            break;
         }
         /* get answer */
         size = sizeof (struct sockaddr_in);
         nbytes = recvfrom (sock, buf, sizeof (buf) - 1, 0,
            (struct sockaddr*) &name, &size);
         sender = inet_ntoa (name.sin_addr);
         buf[nbytes] = 0;
	 /*printf("Received `%s' from %s\n", buf, sender);*/
         if ((nbytes >= 4) && (ntohl(*((long*) buf)) == 0)) {
            if (left < strlen (buf + 4) + 1) {
               return NULL;
            }
            q = buf + 4;
            do {
               qq = strchr (q, '\n');
               if (qq != NULL) *qq = 0;
               if (left < strlen (q) + strlen (sender) + 2) {
                  return NULL;
               }
               sprintf (b, "%s %s", q, sender);
               if (num < _MAX) {
                  *p = b;
                  *(++p) = NULL;
                  ++num;
               }
               left -= strlen (b) + 1;
               b += strlen (b) + 1;
               q = (qq == NULL) ? q + strlen (q) : qq + 1;
            } while (*q);
         }
      }
      close (sock);
   
      /* sort answeres */
      qsort (buffer, num, sizeof (char*), 
         (int (*) (const void*, const void*)) compareConfs);
   
      /* remove duplicates */
      i = 0;
      p = (char**) buffer;
      while (i + 1 < num) {
         if (gds_strcasecmp (p[i], p[i+1]) == 0) {
            for (j = i + 1; j < num; j++) {
               p[j] = p[j + 1];
            }
            num--;
         }
         else {
            i++;
         }
      }
   
      return (const char* const*) buffer;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: parseConfInfo				*/
/*                                                         		*/
/* Procedure Description: parses a conf. info string			*/
/*                                                         		*/
/* Procedure Arguments: info string, info record (return)		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int parseConfInfo (const char* info, confinfo_t* rec)
   {
      char*		p;		/* token */
      char		buf[1024];	/* buffer */
      char*		wrk;		/* work space */
   
      strncpy (buf, info, sizeof (buf));
      buf[sizeof (buf)-1] = 0;
   
      /* get interface */
      p = strtok_r (buf, " \t\n", &wrk);
      if (p == NULL) {
         return -1;
      }
      strncpy (rec->interface, p, sizeof (rec->interface));
      rec->interface[sizeof(rec->interface)-1] = 0;
   
      /* get interferometer number */
      p = strtok_r (NULL, " \t\n", &wrk);
      if (p == NULL) {
         return -2;
      }
      if ((strlen (p) > 0) && (p[0] == '*')) {
         rec->ifo = -1;
      }
      else {
         rec->ifo = atoi (p);
      }
   
      /* get id number */
      p = strtok_r (NULL, " \t\n", &wrk);
      if (p == NULL) {
         return -3;
      }
      if ((strlen (p) > 0) && (p[0] == '*')) {
         rec->num = -1;
      }
      else {
         rec->num = atoi (p);
      }
   
      /* get host name */
      p = strtok_r (NULL, " \t\n", &wrk);
      if (p == NULL) {
         return -4;
      }
      strncpy (rec->host, p, sizeof (rec->host));
      rec->host[sizeof(rec->host)-1] = 0;
   
      /* get port/prognum */
      p = strtok_r (NULL, " \t\n", &wrk);
      if (p == NULL) {
         return -5;
      }
      if ((strlen (p) > 0) && (p[0] == '*')) {
         rec->port_prognum = -1;
      }
      else {
         rec->port_prognum = atoi (p);
      }
   
      /* get program version */
      p = strtok_r (NULL, " \t\n", &wrk);
      if (p == NULL) {
         return -6;
      }
      if ((strlen (p) > 0) && (p[0] == '*')) {
         rec->progver = -1;
      }
      else {
         rec->progver = atoi (p);
      }
   
      /* get sender's address */
      p = strtok_r (NULL, " \t\n", &wrk);
      if (p == NULL) {
         return -7;
      }
      strncpy (rec->sender, p, sizeof (rec->sender));
      rec->sender[sizeof(rec->sender)-1] = 0;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initConfinfo				*/
/*                                                         		*/
/* Procedure Description: init API					*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initConfinfo (void) 
   {
      if (MUTEX_CREATE (confmux) != 0) {
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: finiConfinfo				*/
/*                                                         		*/
/* Procedure Description: cleanup API					*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void finiConfinfo (void) 
   {
      MUTEX_DESTROY (confmux);
   }

