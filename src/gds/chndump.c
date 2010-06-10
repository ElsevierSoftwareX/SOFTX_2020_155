#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dtt/gdsutil.h"
#include "dtt/gdschannel.h"

/* main program */

   #ifdef OS_VXWORKS
   int chndump (void)
   #else
   int main (int argc, char *argv[])
   #endif
   {
      int 		n;		/* length of list */
      int		i;		/* index into channel list */
      gdsChnInfo_t*	info;		/* channel list */
      int		shortlist = 0;	/* short list */
      int		longlist = 0;	/* long list */
   
      /* parse command line arguments */
   #ifndef OS_VXWORKS
      int 		c;		/* option */
      extern char*	optarg;		/* option argument */
      extern int	optind;		/* option ind */
      int		errflag = 0;	/* error flag */
      char		ndsaddr[1024];	/* nds arress */
      int		ndsport = 0;	/* nds port number */
   
      strcpy (ndsaddr, "");
      while ((c = getopt (argc, argv, "hn:m:sl")) != EOF) {
         switch (c) {
            /* nds address */
            case 'n':
               {
                  strcpy (ndsaddr, optarg);
                  break;
               }
            /* nds port number */
            case 'm':
               {
                  ndsport = atoi (optarg);
                  break;
               }
            /* short list? */
            case 's':
               {
                  shortlist = 1;
                  break;
               }
            /* long list? */
            case 'l':
               {
                  longlist = 1;
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
      /* help */
      if (errflag) {
         printf ("usage: chndump -n 'nds address' -m 'nds port'\n"
                "       chndump    for default nds\n"
                "	-s for short list (names only)\n"
                "	-l for long list (slope/offset)\n");
         return 1;
      }
      /* initialize channel information */
      if (strlen (ndsaddr) != 0) {
         gdsChannelSetHostAddress (ndsaddr, ndsport);
         if (!shortlist) {
            printf ("NDS address %s port %i\n", ndsaddr, ndsport);
         }
      }
      else {
         if (!shortlist) {
            printf ("Using default NDS address\n");
         }
      }
   #endif
   
      /* get length of channel list */
      n = gdsChannelListLen (-1, NULL);
      if (n < 0) {
         fprintf (stderr, "channel list not available\n");
         return -1;
      }
      if (!shortlist) {
         printf ("\nChannel list has %i entries\n", n);
      }
      /* allocate memory for channel list */
      info = malloc (n * sizeof (gdsChnInfo_t));
      if (info == NULL) {
         fprintf (stderr, "failed to allocate memory for channel list\n");
         return -1;
      }
   
      /* get channel list */
      n = gdsChannelList (-1, NULL, info, n);
      if (n < 0) {
         fprintf (stderr, "channel list read error %i\n", n);
         return -1;
      }
   
      /* print channel list */
      if (longlist) {
         printf ("----------------------------------"
                "------------------------------------------------------------------\n");
         printf ("channel name                     "
                "ifo  RM DCU     #  ty      f grp      gain     slope       ofs\n");
         printf ("----------------------------------"
                "------------------------------------------------------------------\n");
      }
      else if (!shortlist) {
         printf ("----------------------------------"
                "----------------------------\n");
         printf ("channel name                     "
                "ifo  RM     #  ty      f bps\n");
         printf ("----------------------------------"
                "----------------------------\n");
      }
      for (i = 0; i < n; i++) {
         if (longlist) {
            printf ("%-32s%4d%4d%4d%6d%4d%7d%4d%12g%12g%12g\n", 
                   info[i].chName, info[i].ifoId, info[i].rmId, info[i].dcuId, 
                   info[i].chNum, info[i].dataType, info[i].dataRate, 
                   info[i].chGroup, info[i].gain, info[i].slope,
                   info[i].offset);
         }
         else if (shortlist) {
            printf ("%s\n", info[i].chName);
         }
         else {
            printf ("%-32s%4d%4d%6d%4d%7d%4d\n", 
                   info[i].chName, info[i].ifoId, info[i].rmId, 
                   info[i].chNum, info[i].dataType, info[i].dataRate, 
                   info[i].bps);
         }
      }
      if (longlist) {
         printf ("----------------------------------"
                "------------------------------------------------------------------\n");
      }
      else if (!shortlist) {
         printf ("---------------------------------"
                "----------------------------------------------\n");
      }
   
      free (info);
      return 0;
   }
