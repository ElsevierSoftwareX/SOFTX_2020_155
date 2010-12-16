static char *versionId = "Version $Id$" ;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "tconv.h"
#ifndef GDS_NO_EPICS
/*#include "ezca.h"*/
/* Data Types */
#define ezcaByte   0
#define ezcaString 1
#define ezcaShort  2
#define ezcaLong   3
#define ezcaFloat  4
#define ezcaDouble 5
#define VALID_EZCA_DATA_TYPE(X) (((X) >= 0)&&((X)<=(ezcaDouble)))

/* Return Codes */
#define EZCA_OK                0
#define EZCA_INVALIDARG        1
#define EZCA_FAILEDMALLOC      2
#define EZCA_CAFAILURE         3
#define EZCA_UDFREQ            4
#define EZCA_NOTCONNECTED      5
#define EZCA_NOTIMELYRESPONSE  6
#define EZCA_INGROUP           7
#define EZCA_NOTINGROUP        8
/* Functions */
extern "C" {
   void ezcaAutoErrorMessageOff(void);
   int ezcaGet(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaPut(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaSetRetryCount(int retry);
   int ezcaSetTimeout(float sec);
}
#endif

   const timespec waittime = {0, 10000000};

   int main (int argc, char* argv[])
   {
   #ifdef GDS_NO_EPICS
      printf ("Epics channel access not supported\n");
   #else
      int 		c;		/* option */
      extern char*	optarg;		/* option argument */
      extern int	optind;		/* option ind */
      int		errflag = 0;	/* error flag */
      char 		readback[256];	/* readback channel string */
      bool		havereadback = false; /* have readback ? */
      char 		channel[256];	/* channel string */
      char 		channel2[256];	/* 2nd channel string */
      double		gain = 1.0;     /* gain */
      double		gain2 = 0.0;	/* gain of second channel */
      double		setval = 0.0;	/* set value */
      double		ugf = 1.0;	/* unity gain frequency */
      double		timeout = -1.0;	/* timeout */   
   
      strcpy (readback, "");
      strcpy (channel, "");
      while ((c = getopt (argc, argv, "hr:g:s:f:t:c:d:")) != EOF) {
         switch (c) {
            /* readback */
            case 'r':
               {
                  strncpy (readback, optarg, sizeof(readback) - 1);
                  readback[sizeof(readback)-1] = 0;
                  havereadback = true;
                  break;
               }
            /* gain between radback and control */
            case 'g':
               {
                  gain = atof (optarg);
                  break;
               }
            /* set value */
            case 's':
               {
                  setval = atof (optarg);
                  break;
               }
            /* unity gain frequency */
            case 'f':
               {
                  ugf = fabs (atof (optarg));
                  break;
               }
            /* timeout */
            case 't':
               {
                  timeout = atof (optarg);
                  break;
               }
            /* 2nd channel (common) */
            case 'c':
               {
                  strncpy (channel2, optarg, sizeof(channel2) - 1);
                  channel2[sizeof(channel2)-1] = 0;
                  gain2 = 1.0;
                  break;
               }
            /* 2nd channel (differential) */
            case 'd':
               {
                  strncpy (channel2, optarg, sizeof(channel2) - 1);
                  channel2[sizeof(channel2)-1] = 0;
                  gain2 = -1.0;
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
         strncpy (channel, argv[optind], sizeof(channel)-1);
         channel[sizeof(channel)-1] = 0;
      }
      /* help */
      if (errflag || (strlen (channel) == 0)) {
         printf ("Usage: ezcaservo [options] 'channel name'\n"
                "        Implements a simple integrator (pole at zero)\n"
                "       -r 'readback': readback (error) channel\n"
                "       -g 'gain' : gain between readback and channel\n"
                "       -s 'value' : set value\n"
                "       -f 'ugf' : unity gain frequency (Hz)\n"
                "       -t 'duration' : timeout (sec)\n"
                "       -c 'channel' : 2nd control channel (common)\n"
                "       -d 'channel' : 2nd control channel (differential)\n"
                "       -h : help\n");
         return 1;
      }
      if (timeout == 0) {
         return 0;
      }
      if (!havereadback) {
         strcpy (readback, channel);
         gain = -1.0;
      }
   
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.02);
      ezcaSetRetryCount (500);
   
      // start time
      tainsec_t t0 = TAInow();
      double t = 0;
      double prev = 0;
      double dt;
      double err;
      double ctrl = 1.0;
      double ctrl2 = 1.0;
      // get previous control value
      if (ezcaGet (channel, ezcaDouble, 1, &ctrl) != EZCA_OK) {
         printf ("channel %s not accessible\n", channel);
         return 1;
      }
      if (gain2 != 0) {
         // get previous control value of 2nd channel
         if (ezcaGet (channel2, ezcaDouble, 1, &ctrl2) != EZCA_OK) {
            printf ("2nd channel %s not accessible\n", channel2);
            return 1;
         }
      }
      t0 = TAInow();
      do {
         // wait a little while
         nanosleep (&waittime, 0);
         // get error signal
         if (havereadback) {
            // read error point
            if (ezcaGet (readback, ezcaDouble, 1, &err) != EZCA_OK) {
               printf ("channel %s not accessible\n", readback);
               return 1;
            }
         }
         else {
            // use control signal as error point
            if (gain2 != 0) {
               err = (ctrl + gain2 * ctrl2) / (1 + fabs(gain2));
            }
            else {
               err = ctrl;
            }
         }
         err -= setval;
         err *= -1;
         // get time
         t = (TAInow() - t0) / 1E9;
         dt = t - prev;
         prev = t;
         // comput control
         ctrl -= gain * ugf * dt * err;
         if (gain2 != 0) ctrl2 -= gain2 * gain * ugf * dt * err;
         // set new control value 
         if (ezcaPut (channel, ezcaDouble, 1, &ctrl) != EZCA_OK) {
            printf ("channel %s not accessible\n", channel);
            return 1;
         }
         // set new control value on 2nd channel
         if (gain2 != 0) {
            if (ezcaPut (channel2, ezcaDouble, 1, &ctrl2) != EZCA_OK) {
               printf ("2nd channel %s not accessible\n", channel2);
               return 1;
            }
         }
         // loop until timeout
      } while ((timeout < 0) || (t < timeout));
   
   #endif
      return 0;
   }

