
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "dtt/testpoint.h"

   int main (int argc, char* argv[])
   {
      char 		c;		/* flag */
      int		errflag = 0;	/* error flag */
      char		cmd[1024];	/* command */
      int		interval = 0;	/* time interval in minutes */		
   
      while ((c = getopt (argc, argv, "ht:")) != EOF) {
         switch (c) {
            /* time interval for resubmitting TP command */
            case 't':
               {
                  interval = atoi (optarg);
                  break;
               }
            /* help */
            case 'h':
            case '?':
               {
                  errflag = 1;
                  break;
               }
         }
      }
      if ((optind > 0) && (optind < argc)) {
         strncpy (cmd, argv[optind], sizeof(cmd)-1);
         cmd[sizeof(cmd)-1] = 0;
      }
      else {
         errflag = 1;
      }
   
      /* help */
      if (errflag || (strlen (cmd) == 0)) {
         printf ("Usage: tpcmd [options] 'command'\n"
                "       -t 'minutes' : interval of command re-submission\n"
                "       -h : help\n");
         return 1;
      }
   
      for (;;) {
         int pid = fork();
         /* error */
         if (pid < 0) {
            printf ("Error in fork\n");
            return -1;
         }
         /* child */
         else if (pid == 0) {
            char* p;
            setsid (); /* become session leader */
            p = tpCommand (cmd);
            printf ("%s\n", p);
            free (p);
            if (interval > 0) {
               sleep (2 * interval * 60);
            }
            return 0;
         }
         /* parent */
         else {
            if (interval <= 0) {
               return 0;
            }
            sleep (interval * 60);
         }
      }
   }
