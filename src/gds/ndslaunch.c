/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Program Name: ndslaunch						*/
/*                                                         		*/
/* Program Description: launches an NDS task first and a NDS client	*/
/*                      second. 					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <signal.h>
#include <sys/file.h>
#include <fcntl.h>
#include <wait.h>
#include "sockutil.h"

#ifndef WEXITED
#define WEXITED 0
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static const char* 	confinfo =
   "flock \"%s\";\n"
   "set thread_stack_size=10240;\n"
   "set debug=0;\n"
   "set sweptsine_filename=\"/export/disk0/swept-sine.ascii\";\n"
   "configure channel-groups\n"
   "begin\n"
   "0       \"Others\"\n"
   "1       \"H0:GDS\"\n"
   "2       \"H0:PEM\"\n"
   "3       \"H0:VAC\"\n"
   "4       \"H1:ASC\"\n"
   "5       \"H1:DAQ\"\n"
   "6       \"H1:GDS\"\n"
   "7       \"H1:IOO\"\n"
   "8       \"H1:LSC\"\n"
   "9       \"H1:PSL\"\n"
   "10      \"H1:SEI\"\n"
   "11      \"H1:SUS\"\n"
   "12      \"H2:ASC\"\n"
   "13      \"H2:DAQ\"\n"
   "14      \"H2:GDS\"\n"
   "15      \"H2:IOO\"\n"
   "16      \"H2:LSC\"\n"
   "17      \"H2:PSL\"\n"
   "18      \"H2:SEI\"\n"
   "19      \"H2:SUS\"\n"
   "end;\n"
   "configure channels\n"
   "begin\n"
   "end;\n"
   "set %sframe_dir=\"%s\", \"%s\", \"%s\";\n"
   "set %snum_dirs=%i;\n"
   "scan %sframes;\n"
   "start listener %i 1;\n"
   "start main 1;\n"
   "start listener %i;\n";

   static int		verbose;
   static char		framefile[1024];
   static char		ndscmd[1024];
   static char		launchcmd[1024];
   static char		frametype[1024];
   static char		framedir[1024];
   static int		framedirnum;
   static char		prefix[64];
   static char		suffix[64];
   static int		ndsport[2];
   static char		ndsconf[1024];
   static int		pidNDS;
   static int		pidClient;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: isFrame					*/
/*                                                         		*/
/* Procedure Description: checks if a file is in frame format		*/
/*                                                         		*/
/* Procedure Arguments: filename					*/
/*                                                         		*/
/* Procedure Returns: version number on success, -1 otherwise		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int isFrame (const char* filename) 
   {
      FILE*	fd;
      char	buf[16];
      int	size;
   
      fd = fopen (filename, "r");
      if (fd == 0) {
         return -1;
      }
      size = 0;
      while (!feof(fd) && (size < 6)) {
         buf[size++] = getc (fd);
      }
      fclose (fd);
      if (size != 6) {
         return -1;
      }
      if (strncmp (buf, "IGWD", 4) != 0) {
         return -1;
      }
      return buf[5];
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: parseArguments				*/
/*                                                         		*/
/* Procedure Description: parse the cmd line arguments			*/
/*                                                         		*/
/* Procedure Arguments: argc and argv					*/
/*                                                         		*/
/* Procedure Returns: 0 - success, -1 otherwise				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int parseArguments (int argc, char *argv[])
   {
      int 		c;
      extern char*	optarg;
      extern int	optind;
      int		errflag = 0;
      char*		p;
      int		size;
      char		dir[1024];
      DIR*		fddir;
   
      strcpy (framefile, "");
      strcpy (launchcmd, "");
      verbose = 0;
      while ((c = getopt (argc, argv, "vf:n:x:")) != EOF) {
         switch (c) {
            /* verbose */
            case 'v':
               {
                  verbose = 1;
                  break;
               }
            /* frame file */
            case 'f':
               {
                  strcpy (framefile, optarg);
                  break;
               }
            /* nds launch command */
            case 'n':
               {
                  strcpy (ndscmd, optarg);
                  break;
               }
            /* client launch command */
            case 'x':
               {
                  strcpy (launchcmd, optarg);
                  break;
               }
            case '?':
               {
                  errflag = 1;
                  break;
               }
         }
      }
   
      /* correct arguments? */
      if (errflag || (strlen (framefile) == 0) || 
         (strlen (launchcmd) == 0)) {
         fprintf (stderr, "usage: ndslaunch -f 'frame' "
                 "-n 'nds start cmd' -x 'client start cmd'\n"
                 "       # in client cmd will be replaced "
                 "with a port number\n"
                 "       -v for verbose\n");
         return -1;
      }
   
      /* check if file exists */
      if (isFrame (framefile) < 0) {
         fprintf (stderr, "%s is not a frame file\n", framefile);
         return -1;
      }
   
      /* construct directory name */
      strcpy (framedir, framefile);
      p = strrchr (framedir, '/');
      if (p == NULL) {
         if (getcwd (framedir, sizeof (framedir)) == NULL) {
            fprintf (stderr, "invalid directory\n");
            return -1;
         }
      }
      else {
         *p = 0;
      }
      /* check for trailing zero */
      if ((strlen (framedir) == 0) || 
         (framedir[strlen (framedir) - 1] != '0')) {
         fprintf (stderr, "invalid directory (need trailing 0)\n");
         exit(1);
      }
      else {
         framedir[strlen (framedir) - 1] = 0;
      }
      /* count directories */
      framedirnum = 0;
      do {
         framedirnum++;
         sprintf (dir, "%s%i", framedir, framedirnum);
         fddir = opendir (dir);
         if (fddir != NULL) {
            closedir (fddir);
         }
      } while (fddir != NULL);
      /* get prefix */
      p = strrchr (framefile, '/');
      if (p == NULL) {
         p = framefile;
      }
      else {
         p++;
      }
      size = 0;
      while ((p[size] != 0) && (p[size] != '.') && 
            ((p[size] < '0') || (p[size] > '9'))) {
         size++;
      }
      strncpy (prefix, p, size);
      prefix[size] = 0;
      /* get suffix */
      size = strlen (framefile) - 1;
      while ((size >= 0) && (framefile[size] != '.') &&
            (framefile[size] != '/')) {
         size--;
      }
      if ((size < 0) || (framefile[size] != '.')) {
         strcpy (suffix, "");
      }
      else {
         strcpy (suffix, framefile + size);
      }
      /* guess frame type from suffix */
      if (strcmp (suffix, ".mT") == 0) {
         strcpy (frametype, "minute_trend_");
      }
      else if (strcmp (suffix, ".T") == 0) {
         strcpy (frametype, "trend_");
      }
      else {
         strcpy (frametype, "");
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: getFreePorts				*/
/*                                                         		*/
/* Procedure Description: gets two port # which are available		*/
/*                                                         		*/
/* Procedure Arguments: 						*/
/*                                                         		*/
/* Procedure Returns: 0 on success, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int getFreePorts (void)
   {
      int	i;		/* port number */
      int	outsock;	/* output socket */
      struct sockaddr_in name;	/* server address */
      socklen_t	num;		/* number of bytes in buffer */
   
      for (i = 0; i < 2; i++) {
         /* create output socket */
         outsock = socket (PF_INET, SOCK_STREAM, 0);
         if (outsock < 0) {
            fprintf (stderr, "error creating socket (errno %i)\n", errno);
            return -2;
         }
         /* bind socket */
         name.sin_family = AF_INET;
         name.sin_port = htons (0);
         name.sin_addr.s_addr = htonl (INADDR_ANY);
         if (bind (outsock, (struct sockaddr*) &name, 
            sizeof (name)) < 0) {
            fprintf (stderr, "error binding socket (errno %i)\n", errno);
            close (outsock);
            return -3;
         }
         /* get port number */
         num = sizeof (name);
         if (getsockname (outsock, (struct sockaddr*) &name, 
            &num) < 0) {
            fprintf (stderr, "error obtaining socket name (errno %i)\n", 
                    errno);
            close (outsock);
            return -4;
         }
         ndsport[i] = ntohs (name.sin_port);
         close (outsock);
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: creatConfFile				*/
/*                                                         		*/
/* Procedure Description: creates the NDS configuration file		*/
/*                                                         		*/
/* Procedure Arguments: 						*/
/*                                                         		*/
/* Procedure Returns: 0 on success, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int creatConfFile (void) 
   {
      FILE*		fd;	/* file handle for conf file */
   
      /* get temp file name */
      if (tmpnam (ndsconf) == NULL) {
         fprintf (stderr, "unable to create configuration file\n");
         return -1;
      }
      /* open file */
      fd = fopen (ndsconf, "w");
      if (fd == NULL) {
         fprintf (stderr, "unable to create configuration file\n");
         return -2;
      }
      /* write configuration */
      fprintf (fd, confinfo, ndsconf, frametype, framedir, prefix, suffix,
              frametype, framedirnum, frametype, ndsport[0], ndsport[1]);
      printf (confinfo, ndsconf, frametype, framedir, prefix, suffix,
             frametype, framedirnum, frametype, ndsport[0], ndsport[1]);
      if (ferror (fd)) {
         fprintf (stderr, "unable to create configuration file\n");
         fclose (fd);
         return -2;
      }
   
      fclose (fd);
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: startNDS					*/
/*                                                         		*/
/* Procedure Description: starts the NDS				*/
/*                                                         		*/
/* Procedure Arguments: 						*/
/*                                                         		*/
/* Procedure Returns: 0 on success, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int 	ndsrun;

   void Quit (int sig) 
   {
      ndsrun = 0;
   }

   static int startNDS (void) 
   {
      /* setup child watch */
      ndsrun = 1;
      sigset (SIGCHLD, Quit);
   
      /* start NDS program */
      pidNDS = fork ();
      if (pidNDS == -1) {
         fprintf (stderr, "error while forking\n");
         return -5;
      }
      /* start NDS program if child */
      else if (pidNDS == 0) {
         if (!verbose) {
            int i = open ("/dev/null", 2);
            (void) dup2(i, 1);
            (void) dup2(i, 2);   
         }
         execl (ndscmd, "daqd", "-f", framefile, "-c", ndsconf, NULL);
         fprintf (stderr, "Cannot start %s\n", ndscmd);
         exit (1);
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: waitForNDS					*/
/*                                                         		*/
/* Procedure Description: waits for the NDS				*/
/*                                                         		*/
/* Procedure Arguments: 						*/
/*                                                         		*/
/* Procedure Returns: 0 on success, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int waitForNDS (void)
   {
      int		sock;	/* socket for connection attempt */
      int		ret;	/* return value */
      struct timeval 	timeout;/* wait time = 2 minutes  */
      struct timespec 	wait;	/* wait time = 500ms  */
      int		try;	/* connect tries */
      struct sockaddr_in name;	/* server address */
   
      /* create socket */
      sock = socket (PF_INET, SOCK_STREAM, 0);
      if (sock < 0) {
         fprintf (stderr, "error creating socket (errno %i)\n", errno);
         return -2;
      }
      /* connect address */
      name.sin_family = AF_INET;
      name.sin_port = ndsport[0];
      name.sin_addr.s_addr = htonl (0);
   
      /* try to connect */
      timeout.tv_sec = 2;
      timeout.tv_usec = 0;
      wait.tv_sec = 0;
      wait.tv_nsec = 500000000;
      try = 0;
      ret = -1;
      while (ndsrun && (try < 240)) {
         ret = connectWithTimeout (sock, (struct sockaddr*) &name, 
                                 sizeof (name), &timeout);
         /* wait first to make sure enough time has passed */
         nanosleep (&wait, NULL);
         if (ret == 0) {
            break;
         }
         try++;
      }
      close (sock);
      if (!ndsrun) {
         fprintf (stderr, "NDS terminated unexpectedly\n");
      }
      else if (ret != 0) {
         fprintf (stderr, "Connection attempt to NDS timed-out\n");
      }
      return ret;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: startClient					*/
/*                                                         		*/
/* Procedure Description: starts the NDS client				*/
/*                                                         		*/
/* Procedure Arguments: 						*/
/*                                                         		*/
/* Procedure Returns: 0 on success, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int startClient (void) 
   {
      /* start NDS program */
      pidClient = fork ();
      if (pidClient == -1) {
         fprintf (stderr, "error while forking\n");
         return -5;
      }
      /* start NDS program if child */
      else if (pidClient == 0) {
         char	buf[20][1024];
         int	p1;
         int 	i;
         int 	n;
         char*  arg[20];
      
         /* extract command line arguments */
         n = 1;
         p1 = 0;
         while ((launchcmd[p1] != 0) && (n < 20)) {
            while ((launchcmd[p1] != 0) && (launchcmd[p1] == ' ')) {
               p1++;
            }         
            i = 0;
            while ((launchcmd[p1+i] != 0) && (launchcmd[p1+i] != ' ')) {
               i++;
            }
            if (i == 0) {
               break;
            }
            if (launchcmd[p1] == '#') {
               sprintf (buf[n], "%i", ndsport[0]);
            }
            else {
               if (i >= 1024) i = 1023;
               strncpy (buf[n], launchcmd + p1, i);
               buf[n][i] = 0;
            }
            n++;
            p1 += i;
         }
         strcpy (buf[0], buf[1]);
         if (strrchr (buf[0], '/') == NULL) {
            strcpy (buf[1], buf[0]);
         }
         else {
            strcpy (buf[1], strrchr (buf[0], '/') + 1);
         }
         n--;
         for (i = 0; i < n; i++) {
            arg[i] = buf[i+1];
         }
         arg[n] = NULL;
         /* start client */
         execv (buf[0], arg);
         fprintf (stderr, "Cannot start %s\n", buf[0]);
         exit (1);
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Main Program								*/
/*                                                         		*/
/* Description: launches an file nds plus an other program which 	*/
/* 		connect to the NDS					*/
/* 									*/
/*----------------------------------------------------------------------*/
   int main (int argc, char *argv[])
   {
      int options;
      siginfo_t info;
   
      /* parse command line arguments */
      if (parseArguments (argc, argv) != 0) {
         exit(1);
      }
   
      /* get free ports */
      if (getFreePorts () < 0) {
         exit(1);
      }
   
      /* create NDS configuration file */
      if (creatConfFile () < 0) {
         exit (1);
      }
   
      /* start NDS */
      if (startNDS() < 0) {
         remove (ndsconf);
         exit (1);
      }
   
      /* wait for NDS */
      if (waitForNDS() < 0) {
      }
      
      /* start NDS client */
      else if (startClient() < 0) {
         pidClient = -1;
      }
   
      /* wait for terminationm of client child */
      if (pidClient > 0) {
         options = WEXITED;
         memset (&info, 0, sizeof (info));
         if (waitid (P_PID, pidClient, &info, options) < 0) {
            fprintf (stderr, "unexpeceted termination\n");
            exit (1);
         }
      }
   
      /* kill nds */
      if (verbose)
         printf ("kill NDS @ %i\n", pidNDS);
      if (pidNDS > 0) {
         kill (pidNDS, SIGKILL);
         options = WEXITED;
         memset (&info, 0, sizeof (info));
         waitid (P_PID, pidNDS, &info, options);
      }
   
      /* cleanup */
      remove (ndsconf);
   
      return 0;
   }
