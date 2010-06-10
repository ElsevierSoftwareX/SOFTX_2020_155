
#include <stdio.h>
#include <string.h>
#include <strings.h>
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

   enum logic {
   lNone,
   lAnd, 
   lOr,
   lXor,
   lSet,
   lReset,
   lNAnd, 
   lNOr,
   lNXor,
   lNSet,
   lNReset };

   int main (int argc, char* argv[])
   {
   #ifdef GDS_NO_EPICS
      printf ("Epics channel access not supported\n");
   #else
      double 		d;		/* value written by EPICS */
      int 		c;		/* option */
      extern char*	optarg;		/* option argument */
      extern int	optind;		/* option ind */
      int		errflag = 0;	/* error flag */
      bool		extraiform = false; /* extra output format */
      outformat		iform = floating;/* input format */
      double		offset = 0;	/* offset */
      double		slope = 1;	/* slope */
      char 		unit[256];	/* unit string */
      char 		channel[256];	/* channel string */
      char 		readback[256];	/* readback channel string */
      bool		haverdback = false; /* have readback? */
      logic		op = lNone;	/* logic operation */
      int		shift = 0;	/* shift value */
   
      strcpy (unit, "");
      strcpy (readback, "");
      strcpy (channel, "");
      while ((c = getopt (argc, argv, "+hixbs:o:u:r:t:l:R:L:")) != EOF) {
         switch (c) {
            /* integer output */
            case 'i':
               {
                  iform = integer;
                  extraiform = true;
                  break;
               }
            /* hex output */
            case 'x':
               {
                  iform = hex;
                  extraiform = true;
                  break;
               }
            /* binary output */
            case 'b':
               {
                  iform = binary;
                  extraiform = true;
                  break;
               }
            /* offset */
            case 'o':
               {
                  offset = atof (optarg);
                  extraiform = true;
                  break;
               }
            /* slope */
            case 's':
               {
                  slope = atof (optarg);
                  extraiform = true;
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
            /* readback channel */
            case 'r':
               {
                  strncpy (readback, optarg, sizeof(readback)-1);
                  readback[sizeof(readback)-1] = 0;
                  haverdback = true;
                  break;
               }
            /* logic operation */
            case 'l':
               {
                  if (strcasecmp (optarg, "AND") == 0) {
                     op = lAnd;
                  }
                  else if (strcasecmp (optarg, "OR") == 0) {
                     op = lOr;
                  }
                  else if (strcasecmp (optarg, "XOR") == 0) {
                     op = lXor;
                  }
                  else if (strcasecmp (optarg, "SET") == 0) {
                     op = lSet;
                  }
                  else if (strcasecmp (optarg, "RESET") == 0) {
                     op = lReset;
                  }
                  else if (strcasecmp (optarg, "NAND") == 0) {
                     op = lNAnd;
                  }
                  else if (strcasecmp (optarg, "NOR") == 0) {
                     op = lNOr;
                  }
                  else if (strcasecmp (optarg, "NXOR") == 0) {
                     op = lNXor;
                  }
                  else if (strcasecmp (optarg, "NSET") == 0) {
                     op = lNSet;
                  }
                  else if (strcasecmp (optarg, "NRESET") == 0) {
                     op = lNReset;
                  }
                  else {
                     errflag = 1;
                  }
                  break;
               }
            /* shift */
            case 'R':
               {
                  shift = atoi (optarg);
                  break;
               }
            case 'L':
               {
                  shift = -atoi (optarg);
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
      else {
         errflag = 1;
      }
      if ((optind+1 > 0) && (optind+1 < argc)) {
         switch (iform) {
            case integer:
               d = atoi (argv[optind+1]);
               break;
            case hex:
               d = strtol (argv[optind+1], 0, 16);
               break;
            case binary:
               d = strtol (argv[optind+1], 0, 2);
               break;
            case floating:
            default:
               d = atof (argv[optind+1]);
               break;
         }
      } 
      else {
         errflag = 1;
      }
      if (!haverdback) {
         strcpy (readback, channel);
      }
   
      /* help */
      if (errflag || (strlen (channel) == 0)) {
         printf ("Usage: ezcawrite [options] 'channel name' 'value'\n"
                "       -i : integer format\n"
                "       -x : hex format\n"
                "       -b : binbary format\n"
                "       -o 'offset' : offset correcetion\n"
                "       -s 'slop' : gain correction\n"
                "       -u 'unit' : unit string\n"
                "       -r 'readback' : readback channel\n"
                "       -l 'logic' : apply AND, OR, XOR, SET or RESET logic\n"
                "           SET = !old & new, RESET = old & !new, N to negate\n"
                "       -R 'bits' : shift readback by 'bits' to the right after read\n"
                "       -L 'bits' : shift readback by 'bits' to the left after read\n"
                "       -h : help\n");
         return 1;
      }
   
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.02);
      ezcaSetRetryCount (500);
      double val;
      int old = 0;
      if (op == lNone) {
         val = (slope != 0) ? d / slope + offset : 0;
      }
      // logic operations
      else {
         slope = 1;
         offset = 0;
         double e;
         if (ezcaGet (readback, ezcaDouble, 1, &e) != EZCA_OK) {
            printf ("readback %s not accessible\n", channel);
            return 1;
         }
         old = (e > 0) ? (int)(e+0.5) : -(int)(-e+0.5);
         int inp  = (d > 0) ? (int)(d+0.5) : -(int)(-d+0.5);
      	 // shift first
         if (shift > 0) {
            old >>= shift;
         } 
         else if (shift < 0) {
            old <<= (-shift);
         }
      	 // no apply logic
         switch (op) {
            case lAnd:
               val = old & inp;
               break;
            case lOr:
               val = old | inp;
               break;
            case lXor:
               val = old ^ inp;
               break;
            case lSet:
               val = ~old & inp;
               break;
            case lReset:
               val = old & ~inp;
               break;
            case lNAnd:
               val = ~(old & inp);
               break;
            case lNOr:
               val = ~(old | inp);
               break;
            case lNXor:
               val = ~(old ^ inp);
               break;
            case lNSet:
               val = ~(~old & inp);
               break;
            case lNReset:
               val = ~(old & ~inp);
               break;
            default:
               val = inp;
               break;
         }
      }
   
      if (ezcaPut (channel, ezcaDouble, 1, &val) != EZCA_OK) {
         printf ("channel %s not accessible\n", channel);
         return 1;
      }
      if (ezcaGet (readback, ezcaDouble, 1, &d) != EZCA_OK) {
         printf ("readback %s not accessible\n", channel);
         return 1;
      }
      val = slope * (d - offset);
      int ival = (val > 0) ? (int)(val+0.5) : -(int)(-val+0.5);
      if (extraiform) {
         if (iform == integer) {
            printf ("%s = 0x%d%s (%g)\n", readback, ival, unit, d);
         }
         else if (iform == hex) {
            printf ("%s = 0x%x%s (%g)\n", readback, ival, unit, d);
         }
         else if (iform == binary) {
            char bin[64];
            strcpy (bin, "");
            for (unsigned int mask = (ival > 0xFFFF) ? 0x8000000 : 
                ((ival > 0xFF) ? 0x8000 : 0x80); mask; mask = mask >> 1) {
               if (ival & mask) strcpy (bin + strlen (bin), "1");
               else strcpy (bin + strlen (bin), "0");
            }
            printf ("%s = b%s%s (%g)\n", readback, bin, unit, d);
         }
         else {
            printf ("%s = %g%s (%g)\n", readback, val, unit, d);
         } 
      }
      else {
         printf ("%s = %g%s\n", readback, d, unit);
      }
   
   #endif
      return 0;
   }

