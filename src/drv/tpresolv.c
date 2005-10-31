#include <stdio.h>
#include <string.h>
#include <stdlib.h>             /* VxWorks function prototypes          */
#include <math.h>
#include "ctype.h" 

#define REPLY_MSG_SIZE 32

#ifdef UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define STATUS int
#define OK 1
#define ERROR -1

in_addr_t hostGetByName(char *host) {
  extern int h_errno;
  static struct hostent *hentp;

  hentp = gethostbyname (host);
  if (!hentp) {
    fprintf (stderr, "Can't find hostname `%s'; gethostbyname(); error=%d\n", host, h_errno);
    return ERROR;
  }
  return (in_addr_t) *hentp -> h_addr_list;
}
#else
#include <vxWorks.h>
#include <ioLib.h>
#include "sockLib.h" 
#include "inetLib.h" 
#include "stdioLib.h" 
#include "strLib.h" 
#include "hostLib.h" 
#endif

/* Resolve teset point number into the test point name */
STATUS
tpresolver(
	  char *serverName,     /* name or IP address of server */
	  int  port,            /* server port number */
	  int  tpnum,            /* test point number  */
	  char *tpname
	  )
{ 
  struct sockaddr_in  serverAddr;    /* server's socket address */ 
  char                replyBuf[REPLY_MSG_SIZE]; /* buffer for reply */ 
  int                 sockAddrSize;  /* size of socket address structure */ 
  int                 sFd;           /* socket file descriptor */ 
  char buf[25];

  /* create client's socket */ 
  if ((sFd = socket (AF_INET, SOCK_STREAM, 0)) == ERROR) 
    { 
      perror ("socket"); 
      return (ERROR); 
    } 
          
  /* bind not required - port number is dynamic */ 
  /* build server socket address */ 
  sockAddrSize = sizeof (struct sockaddr_in); 
  bzero ((char *) &serverAddr, sockAddrSize); 
  serverAddr.sin_family = AF_INET; 
#if defined(_X86_) && !defined(UNIX)
  serverAddr.sin_len = (u_char) sockAddrSize; 
#endif
  serverAddr.sin_port = htons (port); 
          
  if (((serverAddr.sin_addr.s_addr = inet_addr (serverName)) == ERROR) && 
      ((serverAddr.sin_addr.s_addr = hostGetByName (serverName)) == ERROR)) 
    { 
      perror ("unknown server name"); 
      close (sFd); 
      return (ERROR); 
    } 
          
  /* connect to server */ 
  if (connect (sFd, (struct sockaddr *) &serverAddr, sockAddrSize) == ERROR) 
    { 
      perror ("connect"); 
      close (sFd); 
      return (ERROR); 
    } 
          
  sprintf(buf, "%10d", tpnum);

  /* send request to server */           
  if (write (sFd, buf, 10) == ERROR)
    { 
      perror ("write"); 
      close (sFd); 
      return (ERROR); 
    } 
          
  if (read (sFd, replyBuf, REPLY_MSG_SIZE) < 0) 
    { 
      perror ("read"); 
      close (sFd); 
      return (ERROR); 
    }

  replyBuf[REPLY_MSG_SIZE-1] = 0;
  {
    char *p;
    for(p=replyBuf+REPLY_MSG_SIZE-2; p>replyBuf && isspace(*p);p--) *p = 0;
    if (p == replyBuf) strcpy(replyBuf, "Unknown");
  }
  /*printf ("MESSAGE FROM SERVER:\n%s\n", replyBuf); */

  if (tpname) strcpy(tpname, replyBuf);
  close (sFd);
  return (OK); 
}
