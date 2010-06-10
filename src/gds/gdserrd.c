/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Daemon Name: gdserrd							*/
/*                                                         		*/
/* Module Description: Error message log daemon 			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 30Mar98  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdserrd.html						*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.5.1		*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*								 	*/
/*									*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1996.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/* LIGO Hanford Observatory				   		*/
/* P.O. Box 1970 S9-02					   		*/
/* Richland WA 99352					   		*/
/*                                                         		*/
/* LIGO Livingston Observatory		   				*/
/* 19100 LIGO Lane Rd.					   		*/
/* Livingston, LA 70754					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

/* Header File List: */
#ifndef OS_VXWORKS
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sockutil.h"

/* definitions */
#define MAXMSG  1024
#define TRUE 1
#define FALSE 0
#define MAGICPORT 5353
#define cMSG 1
#define eMSG 2
#define wMSG 3
#define dMSG 4

/* main program */

/**
   @name Error Message Log Daemon
   * The gdserrd accpets error messages over the network and prints them
   to the standard output. In general, this program should be called by
   using the errlog shell script which will setup a separate xterm to 
   diaplsy the messages.

   errlog port -(more xterm arguments)

   The default port number is 5353. When additional xterm arguments are
   specified a port number has to be given as well. Only one error message
   log server program can be running simultaneously per host/port. All
   messages will also be written to the gdserr.log file.

   Make sure the client parameter file - usually under
   /gds/param/errlog.par - includes the 
   correct server names/IP numbers with the correct port number. For example:

   \begin{verbatim}
   [H1-server0]
   server=10.1.0.56
   port=5353
   error=yes
   debug=no

   [H1-server2]
   server=10.1.0.76
   port=5555
   console=no
   warning=no
   \end{verbatim}

   @memo GDS console daemon
   @author Written Mar. 1998 by Daniel Sigg
   @version 1.0
   @see Error Message API
************************************************************************/

/*@{*/		/* subset of GDS Parameter File API documentation */
/*@}*/

   int main (int argc, char *argv[])
   {
      char 		message[MAXMSG];/* message buffer */
      char 		cname [MAXMSG];	/* sender name */
      char 		cnameIP [MAXMSG];/* sender IP number */
      struct sockaddr_in name;		/* sender address */
      socklen_t		size;		/* address size */
      int 		nbytes;		/* message length */
      int 		sock;		/* socket */
      int 		port;		/* port number */
   #if 0
      struct hostent 	hostinfo;	/* host info */
      struct hostent* 	phostinfo;
      char 		buf[1024];	/* host info buffer */
   #endif
      time_t 		now;		/* current time */
      int 		debug;		/* print debug messages? */
      int		error;		/* print error messages? */
      int		warning;	/* print warning messages? */
      int		console;	/* print console messages? */
      int		i;		/* temp index */
      int		tmp;		/* temp value */
      int		msg;		/* print message? */
      char		s[1024];	/* temp buffer */
   
      /* get command line options */
      debug = 1;
      console = 1;
      error = 1;
      warning = 1;
      port = MAGICPORT;
      for (i = 1; i < argc; i++) {
         if (strcmp (argv[i], "-d") == 0) {
            debug = 0;
         }
         else if (strcmp (argv[i], "+d") == 0) {
            debug = 1;
         }
         if (strcmp (argv[i], "-e") == 0) {
            error = 0;
         }
         else if (strcmp (argv[i], "+e") == 0) {
            error = 1;
         }
         if (strcmp (argv[i], "-c") == 0) {
            console = 0;
         }
         else if (strcmp (argv[i], "+c") == 0) {
            console = 1;
         }
         if (strcmp (argv[i], "-w") == 0) {
            warning = 0;
         }
         else if (strcmp (argv[i], "+w") == 0) {
            warning = 1;
         }
         else if (sscanf (argv[1], "%d", &tmp) > 0) {
            port = tmp;
            if ((port < IPPORT_USERRESERVED) || (port > 65535)) {
               printf ("Illegal port number %s\n", argv[1]);
               exit (EXIT_FAILURE);
            }
         }
      }
      if (!debug && !error && !warning && !console) {
         printf ("at least one message type must be active\n");
         return 1;
      }
   
      /* create a socket */
      sock = socket (PF_INET, SOCK_DGRAM, 0);
      if (sock < 0) {
         printf ("Can't open socket\n");
         exit (EXIT_FAILURE);
      }
   
      /* set reuse option on socket */
      size = 1;
      setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char*) &size, 
                 sizeof (int));   
   
      /* connect socket to IP/port */
      name.sin_family = AF_INET;
      name.sin_port = htons (port);  /* convert to network byte order */
      name.sin_addr.s_addr = htonl (INADDR_ANY);
      if (bind (sock, (struct sockaddr*) &name, sizeof (name))) {
         printf ("Socket at port %d already taken\n", port);
         exit (EXIT_FAILURE);
      }
   
      /* wait for messages */
      printf ("\033[1;30;47m" 
             "GLOBAL DIAGNOSTICS SYSTEM ERROR LOG (%s%s%s%s)\n\n"
             "\033[0m",
             (console?"+c":"-c"), (error?"+e":"-e"), 
             (warning?"+w":"-w"), (debug?"+d":"-d"));
   
      while (TRUE) {
      
         /* Wait for a datagram.  */
      #if 0
         int sizenb = sizeof (name);
      #endif
         size = sizeof (name);
         nbytes = recvfrom (sock, message, MAXMSG, 0,
                           (struct sockaddr*) &name, &size);
         if (nbytes < 0) {
            printf ("Package lost\n");
            exit (EXIT_FAILURE);
         } 
         else if (nbytes > 0) {
         
            /* Determine sender address  */
            strncpy (cnameIP, inet_ntoa (name.sin_addr), MAXMSG);
            cnameIP[MAXMSG-1] = 0;
            if (nsilookup (&name.sin_addr, cname) < 0) {
               cname[0] = 0;
            }
         #if 0
            phostinfo = gethostbyaddr_r 
                        ((char*) &name.sin_addr, sizeof (name.sin_addr), 
                        AF_INET, &hostinfo, buf, 1024, &sizenb);
            if ((phostinfo != NULL) && (hostinfo.h_name != NULL)) {
               strncpy (cname, hostinfo.h_name, MAXMSG);
               cname[MAXMSG-1] = 0;
            } 
            else {
               cname[0] = 0;
            }
         #endif         
            /* Write message to console.  */
            now = time (NULL);
            sprintf (s, "%s", ctime (&now));
            msg = 0;
            switch (message[0]) {
               case cMSG: 
                  {
                     if (console) {
                        printf (s);
                        printf ("\033[1;30;47m");		/* bold, black */
                        printf ("Console message from %s (%s):\n", cname, cnameIP);
                        printf ("\033[0m");
                        msg = 1;
                     }
                     break;
                  }
               case eMSG:
                  {
                     if (error) {
                        printf (s);
                        printf ("\033[1;31;47m");		/* bold, red */
                        printf ("Error message from %s (%s):\n", cname, cnameIP);
                        printf ("\033[0m");
                        msg = 1;
                     }
                     break;
                  }
               case wMSG:
                  {
                     if (warning) {
                        printf (s);
                        printf ("\033[1;34;47m");		/* bold, blue */
                        printf ("Warning message from %s (%s):\n", cname, cnameIP);
                        printf ("\033[0m");
                        msg = 1;
                     }
                     break;
                  }
               case dMSG:
                  {
                     if (debug) {
                        printf (s);
                        printf ("\033[1;35;47m");		/* bold, magenta */
                        printf ("Debug message from %s (%s):\n", cname, cnameIP);
                        printf ("\033[0m");
                        msg = 1;
                     }
                     break;
                  }
               default:
                  {
                     printf ("Unspecified message from %s (%s):\n", cname, cnameIP);
                     msg = 1;
                     break;
                  }
            }
            if (msg) {
               printf ("%s\n\n", &message[1]);
            }
         }
      }
   
   /* never reached */
      return 0;
   }
#endif		/* not OS_VXWORKS */
