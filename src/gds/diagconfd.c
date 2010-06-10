/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Daemon Name: diagconfd						*/
/*                                                         		*/
/* Module Description: Configuration daemon for test/err/nds		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 30Jul99  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: diagconfd.html					*/
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

/* Header File List: */
#ifndef OS_VXWORKS
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "dtt/gdsstring.h"
#include "dtt/gdssock.h"
#include "dtt/confserver.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: logfile		log file for inet daemon		*/
/* 	       maxconf		max # of config. information lines	*/
/* 									*/
/*----------------------------------------------------------------------*/
   const char* const 	logfile = "/var/log/log.gds";
#define			maxconf 100

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Services: testService	diagnostics kernel			*/
/*           ndsService		network data server			*/
/*           errService1	error log, ifo 4K			*/
/*           errService2	error log, ifo 2K			*/
/*           ntpService		NTP server, GPS clock			*/
/*           awgService0	DS340, cobox 0, port 1			*/
/*           awgService1	DS340, cobox 0, port 2			*/
/*           awgService2	DS340, cobox 1, port 1			*/
/*           awgService3	DS340, cobox 1, port 2			*/
/* 									*/
/*----------------------------------------------------------------------*/
   const char* const 	testService = "tst * * 10.1.0.152 5354 *";
   const char* const 	ndsService  = "nds * * fb0 8088 *";
   const char* const 	errService1 = "err 0 * 10.1.255.255 5353 *";
   const char* const 	errService2 = "err 1 * 10.1.255.255 5353 *";
   const char* const	ntpService  = "ntp * * ntpServer * *";
   const char* const 	awgService0 = "awg * 0 cobox0 5000 *";
   const char* const 	awgService1 = "awg * 1 cobox0 5001 *";
   const char* const 	awgService2 = "awg * 2 cobox1 5000 *";
   const char* const 	awgService3 = "awg * 3 cobox1 5001 *";


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: confs		list of configuration info		*/
/*          confnum		# of configuration info entries		*/
/* 									*/
/*----------------------------------------------------------------------*/
   confServices 	confs[maxconf];
   int			confnum = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initConfInfo				*/
/*                                                         		*/
/* Procedure Description: initialize conf info array			*/
/*                                                         		*/
/* Procedure Arguments: configuration file handle			*/
/*                                                         		*/
/* Procedure Returns: true if successful				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int initConfInfo (FILE* conffile)
   {
      if (conffile == NULL) {
         /* defaults */
         confs[0].id = 0;
         confs[0].answer = stdPingAnswer;
         confs[0].user = (char*) testService;
         confs[1].id = 0;
         confs[1].answer = stdPingAnswer;
         confs[1].user = (char*) ndsService;
         confs[2].id = 0;
         confs[2].answer = stdAnswer;
         confs[2].user = (char*) errService1;
         confs[3].id = 0;
         confs[3].answer = stdAnswer;
         confs[3].user = (char*) errService2;
         confs[4].id = 0;
         confs[4].answer = stdPingAnswer;
         confs[4].user = (char*) ntpService;
         confs[5].id = 0;
         confs[5].answer = stdPingAnswer;
         confs[5].user = (char*) awgService0;
         confs[6].id = 0;
         confs[6].answer = stdPingAnswer;
         confs[6].user = (char*) awgService1;
         confs[7].id = 0;
         confs[7].answer = stdPingAnswer;
         confs[7].user = (char*) awgService2;
         confs[8].id = 0;
         confs[8].answer = stdPingAnswer;
         confs[8].user = (char*) awgService3;
         confnum = 9;
         return 1;
      }
      
      /* read file */
      else {
         char		line[256];	/* line buffer */
         char		host[256];	/* host name buffer */
         int		n = 0;		/* config count */
         static	char	buf[256 * maxconf]; /* text buffer */
         char*		p = buf;	/* buffer index */
         int		pos1;		/* position of host name start */
         int		pos2;		/* position of host name end */
         int		len;		/* shift length */
         int		sofar = 0;	/* characters so far */
      
         while (!feof (conffile) && (n < maxconf)) {
            /* get line */
            if (fgets (line, sizeof (line), conffile) == NULL) {
               break;
            }
            /* remove leading blanks */
            while ((line[0] == ' ') || (line[0] == '\t')) {
               memmove (line, line + 1, strlen (line));
            }
            /* skip comments and empty lines */
            if ((strlen (line) == 0) || (line[0] == '#')) {
               continue;
            }
            /* remove newline */
            if (line[strlen(line) - 1] == '\n') {
               line[strlen(line) - 1] = 0;
            }
            /* check DNS lookup */
            if (line[0] == '&') {
               struct in_addr 	addr;
               /*struct hostent	hostinfo;
               struct hostent* 	phostinfo;
               char		buf2[2048];
               int		i;*/
            
               /* remove first character and get the hostname */
               memmove (line, line + 1, strlen (line));
               sscanf (line, "%*s%*s%*s%n%255s%n", &pos1, host, &pos2);
               /* now do a dns lookup */
               if (nslookup (host, &addr) < 0) {
               /*phostinfo = gethostbyname_r (host, &hostinfo, 
                                           buf2, 2048, &i);
               if (phostinfo != NULL) {
                  memcpy (&addr.s_addr, hostinfo.h_addr_list[0], 
                         sizeof (addr.s_addr));
               }
               else {*/
                  addr.s_addr = -1;
               }
               /* insert new hostname */
               if (addr.s_addr != -1) {
                  strncpy (host, inet_ntoa (addr), sizeof (host));
                  host[sizeof(host)-1] = 0;
                  len = strlen (host) + 1 - (pos2 - pos1);
                  if (len + strlen (line) + 1 < sizeof (line)) {
                     memmove (line + pos2 + len, line + pos2, 
                             strlen (line + pos2) + 1);
                     memmove (line + pos1 + 1, host, strlen (host));
                  }
               }
            }
            /* check ping */
            if (line[0] == '?') {
               /* remove first character */
               memmove (line, line + 1, strlen (line));
               confs[n].answer = stdPingAnswer;
            }
            else {
               confs[n].answer = stdAnswer;
            }
         
            /* set conf parameters */
            confs[n].id = 0;
            strcpy (p, line);
            confs[n].user = p;
            if ((sofar > 0) && (sofar + strlen (p) + 1 < 1500)) {
               *(p-1) = '\n'; /* multiline answer */
               sofar += strlen (p) + 1;
            }
            else {
               sofar = strlen (p) + 1;
               n++;
            }
            p += strlen (p) + 1;
         }
         confnum = n;
         return 1;
      }
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
   static int checkStdInHandle (void)
   {
      struct sockaddr_in saddr;
      socklen_t 	asize = sizeof (saddr);
      int 		rpcfdtype;
      socklen_t 	ssize = sizeof (int);
   
      /* get the socket address of FD 0 */
      if (getsockname (0, (struct sockaddr*) &saddr, &asize) == 0) {
      
         /* check whether valid internet address */
         if (saddr.sin_family != AF_INET) {
            return -1;
         }
      	 /* get socket type: udp only */
         if ((getsockopt (0, SOL_SOCKET, SO_TYPE,
            (char*) &rpcfdtype, &ssize) == -1) || 
            (rpcfdtype != SOCK_DGRAM)) {
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
/* Description: calls conf_server					*/
/* 									*/
/*----------------------------------------------------------------------*/
   int main (int argc, char *argv[])
   {
      FILE*	conffile = NULL;
      int	confvalid = 0;
   
      /* get configuration information */
      if (argc > 1) {
         conffile = fopen (argv[1], "r");
         if (conffile != NULL) {
            confvalid = initConfInfo (conffile);
            fclose (conffile);
         }
      }
      else {
         confvalid = initConfInfo (NULL);
      }
   
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
            
               if (!confvalid) {
                  printf ("cannot find configuration information\n");
                  return 1;
               }
            
               return conf_server (confs, confnum, 2);
            }
         /* stand-alone */
         case 0: 
            {
               int		pid;	/* process id */
            
               if (!confvalid) {
                  printf ("cannot find configuration information\n");
                  return 1;
               }
            
               /* fork */
               pid = fork ();
               if (pid < 0) {
                  printf ("error while forking\n");
                  return 1;
               }
               /* call message server if child */
               if (pid == 0) {
                  setsid ();
                  pid = conf_server (confs, confnum, 0);
                  printf ("Error while creating server %i\n", pid);
                  return 1;
               }
               else {
                  return 0;
               }
            }
         /* error */
         default: 
            {
               printf ("illegale socket\n");
               return 1;
            }
      }
   }
#endif
