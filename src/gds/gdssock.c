static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssock							*/
/*                                                         		*/
/* Module Description: socket utility functions				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <ioLib.h>
#include <selectLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>

/*#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif*/

#include "dtt/gdsmain.h"
#include "dtt/gdssock.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _PING_TIMEOUT	  timeout for ping (sec)		*/
/*            _SHUTDOWN_DELAY	  wait period before shut down		*/
/*            								*/
/*----------------------------------------------------------------------*/
#define _PING_PORT 		7
#define _PING_TIMEOUT		1.0


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: 						 		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: 								*/
/*      								*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: nslookup					*/
/*                                                         		*/
/* Procedure Description: looks up a hostname (uses DNS if necessary)	*/
/*                                                         		*/
/* Procedure Arguments: host address, address (return)			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int nslookup (const char* host, struct in_addr* addr)
   {
      char		hostname[MAXHOSTNAMELEN];
   
      if (addr == NULL) {
         return -1;
      }
      if (host == NULL) {
         if (gethostname (hostname, MAXHOSTNAMELEN) < 0) {
            return -1;
         }
      }
      else {
         strncpy (hostname, host, sizeof (hostname));
         hostname[sizeof(hostname)-1] = 0;
      }
   
   #ifdef OS_VXWORKS
      {
         int			saddr;
      
         if (((saddr = inet_addr (hostname)) < 0) &&
            ((saddr = hostGetByName ((char*) hostname)) < 0)) {
            return -1;
         }
         else {
            addr->s_addr = saddr;
            return 0;
         }
      }
   #else
      {
         struct hostent		hostinfo;
         struct hostent* 	phostinfo;
         char			buf[2048];
         int			i;
      
         phostinfo = gethostbyname_r (hostname, &hostinfo, 
                                     buf, 2048, &i);
         if (phostinfo == NULL) {
            return -1;
         }
         else {
            memcpy (&addr->s_addr, hostinfo.h_addr_list[0], 
                   sizeof (addr->s_addr));
            return 0;
         }
      }
   #endif
   }


#ifndef OS_VXWORKS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: connectWithTimeout				*/
/*                                                         		*/
/* Procedure Description: same as connect, but with timeout		*/
/*                                                         		*/
/* Procedure Arguments: socket, address to connect, address size,	*/
/*                      timeout				      		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int connectWithTimeout (int sock, struct sockaddr* name, int size, 
                     struct timeval* timeout)
   {
      int 	fileflags;
      fd_set 	writefds;
      int 	nset;
   
      /* set socket to non blocking */
      if ((fileflags = fcntl (sock, F_GETFL, 0)) == -1) {
         return -1;
      }
      if (fcntl (sock, F_SETFL, fileflags | FNDELAY) == -1) {
         return -1;
      }
      /* try to connect */
      if (connect (sock, name, size) == 0) {
         return fcntl (sock, F_SETFL, fileflags & !FNDELAY);
      }
      /* test if connection in progress */
      if (errno != EINPROGRESS) {
         fcntl (sock, F_SETFL, fileflags & !FNDELAY);
         return -1;
      }
      /* wait fro conenct to finish */
      FD_ZERO (&writefds);
      FD_SET (sock, &writefds); 
      nset = select (FD_SETSIZE, 0, &writefds, NULL, timeout);
      /* error during select */
      if (nset < 0) {
         fcntl (sock, F_SETFL, fileflags & !FNDELAY);
         return -1;
      }
      /* timed out */
      else if (nset == 0) {
         fcntl (sock, F_SETFL, fileflags & !FNDELAY);
         errno = ETIMEDOUT;
         return -1;
      }
      /* conenction attempted, now test if socket is open */
      else {
         struct sockaddr name;
         socklen_t size = sizeof (name);
         fcntl (sock, F_SETFL, fileflags & !FNDELAY);
         /* if we can get a peer the connect was successful */
         if (getpeername (sock, &name, &size) < 0) {
            errno = ENOENT;
            return -1;
         }
         else {
            return 0;
         }
      }
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: ping					*/
/*                                                         		*/
/* Procedure Description: pings a server				*/
/*                                                         		*/
/* Procedure Arguments: host address, timeout				*/
/*                                                         		*/
/* Procedure Returns: true if successful				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int ping (const char* hostname, double timeout)
   {
      int		sock;		/* socket handle */
      struct sockaddr_in name;		/* internet address */
   #ifndef OS_VXWORKS
      int 		nset;		/* # of selected descr. */
      int 		flags;		/* socket flags */
      fd_set 		writefds;	/* socket descr. list */
   #endif
      struct timeval 	wait;		/* timeout wait */
   
      if (timeout <= 0) {
         timeout = 10.;
      }
      wait.tv_sec = (int) (timeout * 1E6) / 1000000;
      wait.tv_usec = (int) (timeout * 1E6) % 1000000;
   
      /* create the socket */
      sock = socket (PF_INET, SOCK_STREAM, 0);
      if (sock == -1) {
         return 0;
      }
   
      /* bind socket */
      name.sin_family = AF_INET;
      name.sin_port = 0;
      name.sin_addr.s_addr = htonl (INADDR_ANY);
      if (bind (sock, (struct sockaddr*) &name, sizeof (name))) {
         close (sock);
         return 0;
      }
   
      /* make socket non blocking */
   #ifndef OS_VXWORKS
      if (((flags = fcntl (sock, F_GETFL, 0)) == -1) ||
         (fcntl (sock, F_SETFL, flags | FNDELAY))) {
         close (sock);
         return 0;
      }
   #endif
   
      /* get the internet address */
      name.sin_family = AF_INET;
      name.sin_port = htons (_PING_PORT);
      if (nslookup (hostname, &name.sin_addr) < 0) {
         close (sock);
         return 0;
      }
   
      /* try to connect */
   #ifdef OS_VXWORKS
      if ((connectWithTimeout (sock, (struct sockaddr*) &name, 
         sizeof (struct sockaddr_in), &wait) >= 0) || 
         (errno == ECONNREFUSED)) {
         /* connection open or refused */
         close (sock);
         return 1;
      }
      else {
         /* timed out */
         close (sock);
         return 1;
      }
   #else
      if ((connect (sock, (struct sockaddr*) &name, 
         sizeof (struct sockaddr_in)) >= 0) || 
         (errno == ECONNREFUSED)) {
         /* connection open or refused */
         close (sock);
         return 1;
      }
      /* connection failed */
      if (errno != EINPROGRESS) {
         close (sock);
         return 0;
      }
      /* connection in progress: test completion with select */
      FD_ZERO (&writefds);
      FD_SET (sock, &writefds); 
      nset = select (FD_SETSIZE, 0, &writefds, 0, &wait);
      if (nset > 0) {
         /* connected */
         close (sock);
         return 1;
      }
      else {
         /* timed out */
         close (sock);
         return 0;
      }
   #endif
   }

#endif /* VxWorks */
