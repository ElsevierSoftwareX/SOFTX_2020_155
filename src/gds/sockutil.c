/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdssock							*/
/*                                                         		*/
/* Module Description: socket utility functions				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include "sockutil.h"
//#include "PConfig.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#if defined (P__SOLARIS)
#include <netdir.h>
#endif
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>


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
/* Internal Procedure Name: __gethostbyname_r				*/
/*            								*/
/*----------------------------------------------------------------------*/
   static struct hostent* __gethostbyname_r (const  char  *name,  
                     struct hostent *result, char *buffer, int buflen, 
		     int *h_errnop)
   {
      struct hostent* ret = 0;
      if (gethostbyname_r (name, result, buffer, buflen, &ret, h_errnop) < 0) {
         return 0;
      }
      else {
         return ret;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: __gethostbyaddr_r				*/
/*            								*/
/*----------------------------------------------------------------------*/
   static struct hostent* __gethostbyaddr_r (const char* addr, 
                     int length, int type, struct hostent* result, 
                     char* buffer, int buflen, int* h_errnop)
   
   {
      struct hostent* ret = 0;
      if (gethostbyaddr_r (addr, length, type, result, buffer, buflen, 
                           &ret, h_errnop) < 0) {
         return 0;
      }
      else {
         return ret;
      }
   }


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
      char		hostname[256];
      struct hostent	hostinfo;
      struct hostent* 	phostinfo;
      char		buf[2048];
      int		i;
   
      if (addr == NULL) {
         return -1;
      }
      if (host == NULL) {
         if (gethostname (hostname, sizeof (hostname)) < 0) {
            return -1;
         }
      }
      else {
         strncpy (hostname, host, sizeof (hostname));
      }
   
      phostinfo = __gethostbyname_r (hostname, &hostinfo, 
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


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: nsilookup					*/
/*                                                         		*/
/* Procedure Description: looks up an address (uses DNS if necessary)	*/
/*                                                         		*/
/* Procedure Arguments: address, host name (return)			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int nsilookup (const struct in_addr* addr, char* hostname)
   {
      struct hostent* 	phostinfo;
      struct hostent	hostinfo;
      char		buf[2048];
      int		i;
      uint32_t		a;
   
      if (addr == NULL) {
         if (gethostname (hostname, 256) < 0) {
            return -1;
         }
         else {
            return 0;
         }
      }
   
      a = addr->s_addr;
      phostinfo = __gethostbyaddr_r ((char*)&a, sizeof (a),
                                    AF_INET, &hostinfo, 
                                    buf, 2048, &i);
      if (phostinfo == NULL) {
         return -1;
      }
      else {
         strncpy (hostname, hostinfo.h_name, 256);
         return 0;
      }
   }


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
   int connectWithTimeout (int sock, struct sockaddr* name, 
                     int size, struct timeval* timeout)
   {
      int 	fileflags;
      fd_set 	writefds;
      int 	nset;
   
      /* set socket to non blocking */
      if ((fileflags = fcntl (sock, F_GETFL, 0)) == -1) {
         return -1;
      }
      if (fcntl (sock, F_SETFL, fileflags | O_NONBLOCK) == -1) {
         return -1;
      }
      /*  try to connect */
      if (connect (sock, name, size) == 0) {
         return fcntl (sock, F_SETFL, fileflags & !O_NONBLOCK);
      }
      /*  test if connection in progress */
      if (errno != EINPROGRESS) {
         fcntl (sock, F_SETFL, fileflags & !O_NONBLOCK);
         return -1;
      }
      /*  wait fro conenct to finish */
      FD_ZERO (&writefds);
      FD_SET (sock, &writefds); 
      nset = select (FD_SETSIZE, 0, &writefds, NULL, timeout);
      /*  error during select */
      if (nset < 0) {
         fcntl (sock, F_SETFL, fileflags & !O_NONBLOCK);
         return -1;
      }
      /*  timed out */
      else if (nset == 0) {
         fcntl (sock, F_SETFL, fileflags & !O_NONBLOCK);
         errno = ETIMEDOUT;
         return -1;
      }
      /*  conenction attempted, now test if socket is open */
      else {
         struct sockaddr name;
         unsigned int size = sizeof (name);
         fcntl (sock, F_SETFL, fileflags & !O_NONBLOCK);
         /*  if we can get a peer the connect was successful */
         if (getpeername (sock, &name, &size) < 0) {
            errno = ENOENT;
            return -1;
         }
         else {
            return 0;
         }
      }
   }


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
      int 		nset;		/* # of selected descr. */
      int 		flags;		/* socket flags */
      fd_set 		writefds;	/* socket descr. list */
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
      if (((flags = fcntl (sock, F_GETFL, 0)) == -1) ||
         (fcntl (sock, F_SETFL, flags | FNDELAY))) {
         close (sock);
         return 0;
      }
   
      /* get the internet address */
      name.sin_family = AF_INET;
      name.sin_port = htons (_PING_PORT);
      if (nslookup (hostname, &name.sin_addr) < 0) {
         close (sock);
         return 0;
      }
   
      /* try to connect */
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
   }

