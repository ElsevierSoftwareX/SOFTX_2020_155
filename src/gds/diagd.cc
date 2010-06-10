/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsserver						*/
/*                                                         		*/
/* Module Description: implements functions forexecuting gds commands	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

   const int			DIAG_WKA = 5354;
   const char* const 		logfile = "/var/log/log.gds";


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                        		*/
/*----------------------------------------------------------------------*/
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#include <stdio.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include "dtt/gdsmsg_server.h"

   using namespace std;


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


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Main Program								*/
/*                                                         		*/
/* Description: calls gdsmsg_server					*/
/* 									*/
/*----------------------------------------------------------------------*/
#ifndef OS_VXWORKS
   int main (int argc, char *argv[])
   {
      /* test if started by a port monitor */
      switch (checkStdInHandle()) {
         /* port monitor */
         case 1: 
            {
               int		i;	/* file descriptor */
            
               /* make sure stdout/stderr are not used */
               i = open (logfile, O_CREAT | O_WRONLY | O_TRUNC);
               if (i < 0) {
                  i = open ("/dev/null", 2);
               }
               (void) dup2(i, 1);
               (void) dup2(i, 2);
               cout << "start diagnostics kernel" << endl;
            
               return gdsmsg_server (0);
            }
         /* stand-alone */
         case 0: 
            {
               int		sock; 	/* socket */
               struct sockaddr_in name;	/* server address */
               int		pid;	/* process id */
            
               cout << "start diagnostics kernel" << endl;
            
               /* create socket */
               sock = socket (PF_INET, SOCK_STREAM, 0);
               if (sock < 0) {
                  cout << "error creating socket" << endl;
                  return -1;
               }
               /* bind socket */
               name.sin_family = AF_INET;
               name.sin_port = DIAG_WKA;
               name.sin_addr.s_addr = htonl (INADDR_ANY);
               if (bind (sock, (struct sockaddr*) &name, 
                        sizeof (name)) < 0) {
                  close (sock);
                  cout << "error binding socket" << endl;
                  return -2;
               }
            
               while (1) {
                  int		c_sock;		/* client socket */
                  struct sockaddr_in c_addr;	/* client address */
                  socklen_t	c_len;		/* client addr length */
               
                  /* listen at well-known address */
                  if (listen (sock, 1) < 0) {
                     cout << "error while listening to socket" << endl;
                     return -3;
                  }
               
                  /* accept connection */
                  c_len = sizeof (c_addr);
                  c_sock = accept (sock, (struct sockaddr*) &c_addr, 
                                  &c_len);
                  if (c_sock < 0) {
                     cout << "error while accepting connection" << endl;
                     return -4;
                  }
               
                  /* fork */
                  pid = fork ();
                  if (pid < 0) {
                     cout << "error while forking" << endl;
                     return -5;
                  }
                  /* call message server if child */
                  if (pid == 0) {
                     setsid ();
                     return gdsmsg_server (c_sock);
                  }
                  /* continue listening if parent */
               }
            }
         /* error */
         default: 
            {
               cout << "illegale socket" << endl;
               return -1;
            }
      }
   
   }
#endif


