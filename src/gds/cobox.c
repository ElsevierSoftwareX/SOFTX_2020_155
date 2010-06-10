/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: cobox							*/
/*                                                         		*/
/* Procedure Description: Interface to a cobox				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Extension needed because of BSD sockets */
#if !defined(OS_VXWORKS) && !defined(__EXTENSIONS__)
#define __EXTENSIONS__
#endif

#define BSD_COMP

/* Header File List: */
#include "dtt/gdsmain.h" 

#ifdef OS_VXWORKS
/* for VxWorks */
#include <vxWorks.h>
#include <sockLib.h>
#include <inetLib.h>
#include <stdioLib.h>
#include <hostLib.h>
#include <ioLib.h>

/* for others */
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "dtt/gdsutil.h"
#include "dtt/gdssock.h"
#include "dtt/gdstask.h"


/* definitions */
#define _COBOXPORT1	5000	/* first port at 5000 of cobox */
#define _COBOXPORT(serial)	(serial)
/* #define _COBOXPORT(serial)	(_COBOXPORT1 - 1 + serial) */
#define _BUFLEN		1000
#define _TIMEOUT	500000	/* 500ms timeout */


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: openCobox					*/
/*                                                         		*/
/* Procedure Description: opens a socket and establishes a connection	*/
/* 			  to a cobox					*/
/*                                                         		*/
/* Procedure Arguments: netaddr	- network address of cobox		*/
/*                      serialport - serial port number  		*/
/*                                                         		*/
/* Procedure Returns: socket handle, -1 if error			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int openCobox (const char* netaddr, int serialport)
   {
      int			sock;	/* socket handle */
      struct sockaddr_in	name;	/* cobox internet address */
       struct timeval	timeout =      
      {_TIMEOUT / 1000000, _TIMEOUT % 1000000}; 
      //wait_time timeout = _TIMEOUT / 1000000;	/* connection timeout */

      /* get the cobox internet address */
      name.sin_family = AF_INET;
      name.sin_port = htons (_COBOXPORT(serialport));
      if (nslookup (netaddr, &name.sin_addr) < 0) {
         return -1;
      }
   
      /* create the socket */
      sock = socket (PF_INET, SOCK_STREAM, 0);
      if (sock == -1) {
         return -1;
      }
   
      /* connect to the cobox  */
      if (connectWithTimeout (sock, (struct sockaddr *) &name, 
         sizeof (name), &timeout) < 0) {
         close (sock); 
         return -1;
      }
   
      return sock;
   }
