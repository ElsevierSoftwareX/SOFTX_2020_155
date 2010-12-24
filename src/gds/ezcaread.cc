static const char *versionId = "Version $Id$" ;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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

   enum outformat {
   floating,
   integer,
   binary, 
   hex};

   int main (int argc, char* argv[])
   {
   #ifdef GDS_NO_EPICS
      printf ("Epics channel access not supported\n");
   #else
      double 		d;		/* value read by EPICS */
      int 		c;		/* option */
      extern char*	optarg;		/* option argument */
      extern int	optind;		/* option ind */
      int		errflag = 0;	/* error flag */
      bool		extraoform = false; /* extra output format */
      outformat		oform = floating;/* output format */
      double		offset = 0;	/* offset */
      double		slope = 1;	/* slope */
      char 		unit[256];	/* unit string */
      char 		channel[256];	/* channel string */
      bool              numberonly = false; /* number only format */
   
      strcpy (unit, "");
      strcpy (channel, "");
      while ((c = getopt (argc, argv, "hixbs:o:u:n")) != EOF) {
         switch (c) {
            /* integer output */
            case 'i':
               {
                  oform = integer;
                  extraoform = true;
                  break;
               }
            /* hex output */
            case 'x':
               {
                  oform = hex;
                  extraoform = true;
                  break;
               }
            /* binary output */
            case 'b':
               {
                  oform = binary;
                  extraoform = true;
                  break;
               }
            /* offset */
            case 'o':
               {
                  offset = atof (optarg);
                  extraoform = true;
                  break;
               }
            /* slope */
            case 's':
               {
                  slope = atof (optarg);
                  extraoform = true;
                  break;
               }
            /* unit string */
            case 'u':
               {
                  unit[0] = ' ';
                  strncpy (unit+1, optarg, sizeof(unit)-2);
                  unit[sizeof(unit)-1] = 0;
                  break;
               }
            /* number only */
            case 'n':
               {
                  numberonly = true;
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
         printf ("Usage: ezcaread [options] 'channel name'\n"
                "       -i : integer format\n"
                "       -x : hex format\n"
                "       -b : binary format\n"
                "       -o 'offset' : offset correcetion\n"
                "       -s 'slop' : gain correction\n"
                "       -u 'unit' : unit string\n"
                "       -n : number only\n"
                "       -h : help\n");
         return 1;
      }
   
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.02);
      ezcaSetRetryCount (500);
      if (ezcaGet (channel, ezcaDouble, 1, &d) != EZCA_OK) {
         printf ("channel %s not accessible\n", channel);
         return 1;
      }
      double val = slope * (d - offset);
      int ival = (val > 0) ? (int)(val+0.5) : -(int)(-val+0.5);
      char prefix[300];
      if (numberonly) {
         strcpy (prefix, "");
      }
      else {
         sprintf (prefix, "%s = ", channel);
      }
      if (extraoform) {
         if (oform == integer) {
            printf ("%s%d%s (%g)\n", prefix, ival, unit, d);
         }
         else if (oform == hex) {
            printf ("%s0x%x%s (%g)\n", prefix, ival, unit, d);
         }
         else if (oform == binary) {
            char bin[64];
            strcpy (bin, "");
            for (unsigned int mask = (ival > 0xFFFF) ? 0x8000000 : 
                ((ival > 0xFF) ? 0x8000 : 0x80); mask; mask = mask >> 1) {
               if (ival & mask) strcpy (bin + strlen (bin), "1");
               else strcpy (bin + strlen (bin), "0");
            }
            printf ("%sb%s%s (%g)\n", prefix, bin, unit, d);
         }
         else {
            printf ("%s%g%s (%g)\n", prefix, val, unit, d);
         } 
      }
      else {
         printf ("%s%g%s\n", prefix, d, unit);
      }
   
   #endif
      return 0;
   }

