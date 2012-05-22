static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdserr							*/
/*                                                         		*/
/* Procedure Description: displays a message on the GDS			*/
/* system consoles.							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Extension needed because of BSD sockets */
/*#if !defined(OS_VXWORKS) && !defined(__EXTENSIONS__)
#define __EXTENSIONS__
#endif*/

/* Header File List: */

#ifdef OS_VXWORKS
/* for VxWorks */
#include <vxWorks.h>
#include <sockLib.h>
#include <inetLib.h>
#include <stdioLib.h>
#include <strLib.h>
#include <hostLib.h>
#include <selectLib.h>
#include <taskLib.h>
#include <ioLib.h>
#include <pipeDrv.h>

/* for others */
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "dtt/gdsmain.h" 
#include "dtt/gdsutil.h"

#if !defined(__CYGWIN__) && !defined(linux)
#ifndef _IN_ADDR_T
#define _IN_ADDR_T
   typedef unsigned int    in_addr_t;
#endif
#endif


/* definitions */
#ifdef OS_VXWORKS
#define MAXSERVERS	10
#define FALSE		0
#define TRUE		1
#define cMSG		1
#define eMSG		2
#define wMSG		3
#define dMSG		4
#define MAGICPORT	5353
#define PRM_FILE 	gdsPathFile ("/param", "errlog.par")
#define PRM_SECTION	gdsSectionSiteIfo ("server%i")
#define PRM_SERVER	"address"
#define PRM_PORT	"port"
#define PRM_MSG1	"console"
#define PRM_MSG2	"error"
#define PRM_MSG3	"warning"
#define PRM_MSG4	"debug"
#define GDS_MSG_BUF	1024

/* Global variables */

   static long gdserr_version = -1; 	/* version of parameter file */
   static long gdserr_newversion = 0;	/* new version: is used for updating
 			         	   server list */

   static struct gdserr_serverentry {
      in_addr_t 	serveraddr;	/* IP addr of server */
      int 		serverport;	/* port # of server */
      int 		msgAllowed[4];	/* messages allowed? */
   } gdserr_serverlist [MAXSERVERS];

   static struct gdserr_msgentry {
      int errnum;			/* error number */
      char* errmsg;			/* error message */
   } gdserr_msglist[] = GDS_ERR_SET;
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: readParameters				*/
/*                                                         		*/
/* Procedure Description: reads console server parameters from		*/
/* file.								*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void readParameters (void)
   {
   #ifdef OS_VXWORKS
      int	i;
      int	j;
      struct gdserr_serverentry*	p;
      char	section[30];
      char	server[PARAM_ENTRY_LEN];
   
      /* test if new version */
      if (gdserr_version!=gdserr_newversion) {
      
         /* init server list */
         for (i=0; i<MAXSERVERS; i++) {
            p = &gdserr_serverlist[i];
            p->serveraddr = 0;
            p->serverport = 0;
            for (j=0; j<4; j++) {
               p->msgAllowed[j] = FALSE;
            }
         }
         gdserr_version = gdserr_newversion;
      
         /* read server list from parameter file */ 
         for (i=0; i<MAXSERVERS; i++) {
            p = &gdserr_serverlist[i];
            sprintf (section, PRM_SECTION, i);
            strcpy (server, "0.0.0.0");
            loadStringParam (PRM_FILE, section, PRM_SERVER, server);
            /* get IP number */
         #ifdef OS_VXWORKS
            {
               in_addr_t	saddr;
            
               if (((saddr = inet_addr (server)) < 0) &&
                  ((saddr = hostGetByName (server)) < 0)) {
                  saddr = 0;
               }
               p->serveraddr = saddr;
         }
         #else
            /* use DNS if necessary */
            {
               struct 	hostent hostinfo;
               struct 	hostent* phostinfo;
               in_addr_t	addr;
               char		buf[GDS_MSG_BUF+1];
            
               phostinfo = gethostbyname_r (server, &hostinfo, 
                                           buf, GDS_MSG_BUF, &i);
               if (phostinfo != NULL) {
                  memcpy (&addr, hostinfo.h_addr_list[0], 
                         sizeof (addr));
                  p->serveraddr = addr; /* network byte order */
               }
            }
         #endif
         
            /* read port and allowed messages */
            if (p->serveraddr != 0) {
               p->serverport = MAGICPORT;   /* no network b.o. */
               loadIntParam (PRM_FILE, section, PRM_PORT, &p->serverport);
               if (p->serverport < IPPORT_USERRESERVED) {
                  p->serverport = MAGICPORT;
               }
               p->msgAllowed[0] = TRUE;
               loadBoolParam (PRM_FILE, section, PRM_MSG1, 
                             &p->msgAllowed[0]);
               p->msgAllowed[1] = TRUE;
               loadBoolParam (PRM_FILE, section, PRM_MSG2, 
                             &p->msgAllowed[1]);
               p->msgAllowed[2] = TRUE;
               loadBoolParam (PRM_FILE, section, PRM_MSG3, 
                             &p->msgAllowed[2]);
               p->msgAllowed[3] = TRUE;
               loadBoolParam (PRM_FILE, section, PRM_MSG4, 
                             &p->msgAllowed[3]);
            }
         }
      }
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: writeToSocket				*/
/*                                                         		*/
/* Procedure Description: writes a message to a socket			*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: addr: server addr.; port: port #; msg: message	*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void writeToSocket (unsigned long addr, int port, const char* msg)
   {
   #ifdef OS_VXWORKS
      int			sock;
      struct sockaddr_in	name;
   
      /* create the socket */
      sock = socket (PF_INET, SOCK_DGRAM, 0);
      if (sock == -1) {
         return;
      }
   
      /* fill destination address */
      name.sin_family = AF_INET;
      name.sin_port = htons (port);  /* convert to network byte order */
      name.sin_addr.s_addr = addr;   /* already in network byte order */
   
      /* send message to the socket */
      sendto (sock, (char*) msg, strlen (msg) + 1, 0, 
             (struct sockaddr*) &name, sizeof (struct sockaddr_in));
      close (sock);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: writeMessage				*/
/*                                                         		*/
/* Procedure Description: writes a message to the console server	*/
/* 									*/
/*                                                         		*/
/* Procedure Arguments: msgType: type of message; msg: message		*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void writeMessage (int msgType, const char* msg)
   {
   #ifdef OS_VXWORKS
      char*		 	msgBuf;
      struct gdserr_serverentry* p;
      int 			i;
   
      /* copy message to buffer */
      msgBuf = malloc (MAXERRMSG);
      if (msgBuf == NULL) { 
         return;
      }
      msgBuf[0] = msgType;
      strncpy (&msgBuf[1], msg, MAXERRMSG-1);
      msgBuf[MAXERRMSG-1] = 0;
      /* cycle through server list */
      for (i=0; i<MAXSERVERS; i++) {
         p = &gdserr_serverlist[i];
         if ((p->serveraddr != 0) && 
            (msgType>0) && (msgType<=4) &&
            (p->msgAllowed[msgType-1])) {
            writeToSocket (p->serveraddr, p->serverport, msgBuf);
         }
      }
      free (msgBuf);
   #endif
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsConsoleMessage				*/
/*                                                         		*/
/* Procedure Description: displays a console message on the GDS		*/
/* system consoles.							*/
/*                                                         		*/
/* Procedure Arguments: char* msg					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void gdsConsoleMessage (const char* msg)
   {
   #ifdef OS_VXWORKS
      readParameters();
      writeMessage (cMSG, msg);
   #endif
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsErrorMessage				*/
/*                                                         		*/
/* Procedure Description: displays an error message on the GDS		*/
/* system consoles.							*/
/*                                                         		*/
/* Procedure Arguments: char* msg					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void gdsErrorMessage (const char* msg)
   {
   #ifdef OS_VXWORKS
      readParameters();
      writeMessage (eMSG, msg);
   #endif
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsError    				*/
/*                                                         		*/
/* Procedure Description: displays an error message on the GDS		*/
/* system consoles using the error number.				*/
/*                                                         		*/
/* Procedure Arguments: int num, char* msg				*/
/*                                                         		*/
/* Procedure Returns: int						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsErrorEx (int num, const char* msg, const char* file, int line)
   {
   #ifdef OS_VXWORKS
      char* 	buf;
      char* 	p;
      int 	i = 0;
   
      buf = malloc (MAXERRMSG + 256);
      if (buf == NULL) {
         return -999;
      }
      strcpy (buf, GDS_ERRMSG_UNDEF);
      do {
         if (gdserr_msglist[i].errnum == num) {
            strcpy (buf, gdserr_msglist[i].errmsg);
         } 
         i++;
      } while (gdserr_msglist[i].errnum <= 0);
   
      strcpy (strend (buf), ": ");
      p = strend (buf);
      strncpy (p, msg, MAXERRMSG);
      p[MAXERRMSG-1] = 0;
      sprintf (strend (buf), "\nfile: %s - line: %i", file, line);
   
      gdsErrorMessage (buf);
      free (buf);
   
      return num;
   #else
      return 0;
   #endif
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsWarningMessage				*/
/*                                                         		*/
/* Procedure Description: displays a warning message on the GDS		*/
/* system consoles.							*/
/*                                                         		*/
/* Procedure Arguments: char* msg					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void gdsWarningMessage (const char* msg)
   {
   #ifdef OS_VXWORKS
      readParameters();
      writeMessage (wMSG, msg);
   #endif
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsDebugMessage				*/
/*                                                         		*/
/* Procedure Description: displays a debug message on the GDS		*/
/* system consoles.							*/
/*                                                         		*/
/* Procedure Arguments: char* msg					*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void gdsDebugMessage (const char* msg)
   {
   #ifdef OS_VXWORKS
      readParameters();
      writeMessage (dMSG, msg);
   #endif
   }


   void gdsDebugMessageEx (const char* msg, const char* file, int line)
   {
   #ifdef OS_VXWORKS
      char* 		buf;
   
      buf = malloc (MAXERRMSG + 256);
      if (buf == NULL) { 
         return;
      }
      strncpy (buf, msg, MAXERRMSG);
      buf[MAXERRMSG-1] = 0;
      sprintf (strend (buf), "\nfile: %s - line: %i", file, line);
      gdsDebugMessage (buf);
      /*printf ("%s\n", buf);*/
      free (buf);
   #else
	printf("%s:%i %s\n", file, line, msg);
   #endif
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: waitForMessages				*/
/*                                                         		*/
/* Procedure Description: waits fro input from the stdout pipe and	*/
/* the stderr pipe. Copies messages to the system console.		*/
/*                                                         		*/
/* Procedure Arguments: int stdOut, int stdErr, int pdOut, int pdErr	*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void waitForMessages (int stdOut, int stdErr, 
                     int pdOut, int pdErr)
   {
      fd_set	port_set;
      char*	buf;
      int	num;
   
      /* allocate message buffer */
      buf = malloc (MAXERRMSG+10);
      if (buf == NULL) {
         return;
      }
   
      /* loop and wait for input */
      while (1) {
      
         /* setup for select */   
         FD_ZERO (&port_set);
         if (stdOut) {
            FD_SET (pdOut, &port_set);
         }
         if (stdErr) {
            FD_SET (pdErr, &port_set);
         }
      
         /* wait for input from either pipe */
         if (select (FD_SETSIZE, &port_set, NULL, NULL, NULL) > 0) {
            /* copies to system console and stdout */
            if ((stdOut) && (FD_ISSET (pdOut, &port_set))) {
               num = read (pdOut, buf, MAXERRMSG);
               if (num > 0) {
                  buf [num] = '\0';
                  fprintf (stdout, "%s", buf);
                  gdsConsoleMessage (buf);
               }
            }
            /* copies to system console and stderr */
            if ((stdErr) && (FD_ISSET (pdErr, &port_set))) {
               num = read (pdErr, buf, MAXERRMSG);
               if (num > 0) {
                  buf [num] = '\0';
                  fprintf (stderr, "%s", buf);
                  gdsErrorMessage (buf);
               }
            }
         }
         else {
            printf ("select failed ");
         }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: terminateChildProcess			*/
/*                                                         		*/
/* Procedure Description: terminates the child process when parent	*/
/* terminates.								*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
/* Child process id */
   static int pid = 0;
#ifndef OS_VXWORKS
   static void terminateChildProcess (void)
   {
      struct timespec	delay = {0, 300000000L};	/* 300ms */
   
      if (pid != 0) {
         /* this wait is needed to make sure all messages have been sent */
         nanosleep (&delay, NULL);
      
      	 /* terminate child process */
      #ifdef OS_VXWORKS
         taskDeleteForce (pid);
      #else
         kill (pid, SIGKILL);
      #endif
         pid = 0;
      }
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gdsCopyStdToConsole				*/
/*                                                         		*/
/* Procedure Description: copies stdout and stdeerr to the		*/
/* system consoles.							*/
/*                                                         		*/
/* Procedure Arguments: int stdOut, int stdErr				*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int gdsCopyStdToConsole (int stdOut, int stdErr)
   {
      int 	pdOut [2] = {0,0};	/* pipe descriptor for stdout */
      int 	pdErr [2] = {0,0}; 	/* pipe descriptor for stderr */
   
      /* test wheter anything is to do */
      if ((stdOut == 0) && (stdErr == 0)) {
         return 0;
      }
   
   #ifdef OS_VXWORKS
      /* VxWorks */
      {
         char 	pipename [L_tmpnam + 20];
         char*	ext;
      
         /* create pipes (named pipes only for VxWorks) */
         strcpy (pipename, "/pipe/std");	/* temp name */
         ext = strend (pipename);
         if (pid == 0) {
            if (stdOut != 0) {
               strcpy (ext, ".out");
               pipeDevCreate (pipename, 20, MAXERRMSG);
               pdOut[0] = open (pipename, O_RDONLY, 0666);
               if (pdOut[0] == ERROR) {
                  return -1;
               }
            }
            if (stdErr != 0) {
               strcpy (ext, ".err");
               pipeDevCreate (pipename, 20, MAXERRMSG);
               pdErr[0] = open (pipename, O_RDONLY, 0666);
               if (pdErr[0] == ERROR) {
                  if (stdOut != 0) {
                     close (pdOut[0]);
                  }
                  return -1;
               }
            }
         
            /* create child process which does the copy */
            pid = taskSpawn ("tErr", 10, 0, 10000, (FUNCPTR) (waitForMessages),
                            stdOut, stdErr, pdOut[0], pdErr[0], 
                            0, 0, 0, 0, 0, 0);
            if (pid == ERROR) {
               pid = 0;
               return -1;
            }
         }
         /* parent process */
         /* redirect std output and error to pipe */
         if (stdOut != 0)  {
            strcpy (ext, ".out");
            pdOut[1] = open (pipename, O_WRONLY, 0666);
            if (pdOut[1] != ERROR) {
               ioTaskStdSet (0, 1, pdOut[1]);
            }
         }
         if (stdErr != 0)  {
            strcpy (ext, ".err");
            pdErr[1] = open (pipename, O_WRONLY, 0666);
            if (pdErr[1] != ERROR) {
               ioTaskStdSet (0, 2, pdErr[1]);      
            }
         }
      }
   #else
      /* UNIX */
      {
         /* only do it once */
         if (pid != 0) {
            return 0;
         }
      
         /* create pipes */
         if ((stdOut != 0) && (pipe (pdOut) == -1)) {
            return -1;
         }
         if ((stdErr != 0) && (pipe (pdErr) == -1)) {
            if (stdOut != 0) {
               close (pdOut[0]);
               close (pdOut[1]);
            }
            return -1;
         }
      
         /* create child process which does the copy */
         if ((pid = fork()) == 0) {
            /* child process, never to return */
            waitForMessages (stdOut, stdErr, pdOut[0], pdErr[0]);
         }
         
         else if (pid == -1) {		/* error */
            pid = 0;
            if (stdOut != 0) {
               close (pdOut[0]);
               close (pdOut[1]);
            }
            if (stdErr != 0) {
               close (pdErr[0]);
               close (pdErr[1]);
            }      
            return -1;
         }
         else {				/* parent process */
            /* register exit function which terminates child */
            atexit (terminateChildProcess);
         
            /* dup std out and err to pipe id */
            if (stdOut!= 0) {
               dup2 (pdOut[1], 1);	/* do not test for failure */
            }
            if (stdErr != 0) {
               dup2 (pdErr[1], 2);
            }
         }
      }
   #endif
   
      return 0;
   }
