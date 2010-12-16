static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ezcaswitch						*/
/*                                                         		*/
/* Module Description: ezca switching of filter modules                 */
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 05Mar04  S. Ballmer   	First release                           */
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: none					                */
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Stefan Ballmer (509)783-9046	  -              sballmer@ligo.mit.edu  */
/* Daniel Sigg    (509)372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
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

/* Bit definitions */
#define BIT_INPUT      (1<<2)
#define BIT_OFFSET     (1<<3)
#define BIT_FM1        (1<<4)
#define BIT_FM2        (1<<6)
#define BIT_FM3        (1<<8)
#define BIT_FM4        (1<<10)
#define BIT_FM5        (1<<12)
#define BIT_FM6        (1<<14)
#define BIT_FM7        (1<<0)
#define BIT_FM8        (1<<2)
#define BIT_FM9        (1<<4)
#define BIT_FM10       (1<<6)
#define BIT_LIMIT      (1<<8)
#define BIT_OUTPUT     (1<<10)
#define BIT_DECIMATION (1<<9)
#define BIT_HOLD       (1<<11)

#define N_ACTION      32

#define SW_ON          1
#define SW_OFF         2
#define SW_TOGGLE      3


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
      outformat		iform = floating;/* input format */
      char 		basename[256];	/* basename string */
      char 		channel[256];	/* channel string */
      char 		readback[256];	/* readback channel string */
      int		shift = 0;	/* shift value */
      int               basenameLen=0;
      int allSWbits[2];
      int SWbits[2][N_ACTION];
      int action[N_ACTION];
      int iAction;
      bool needReadback[2];
      bool bitsSelected=false;
      char SWname[2][5]={"_SW1","_SW2"};
      
      for(iAction=0;iAction<N_ACTION;iAction++){
        action[iAction]=SWbits[0][iAction]=SWbits[1][iAction]=0;
      }
      iAction=0;
      strcpy (basename, "");
      strcpy (readback, "");
      strcpy (channel, "");

//---------------------------------  Get the arguments ------------------
     for (int i=1 ; i<argc && iAction<N_ACTION; i++) {
        if (!strcmp(argv[i], "INPUT")) {
	  SWbits[0][iAction] |= BIT_INPUT;
        } else if (!strcmp(argv[i], "OFFSET")) {
	  SWbits[0][iAction] |= BIT_OFFSET;
        } else if (!strcmp(argv[i], "FM1")) {
	  SWbits[0][iAction] |= BIT_FM1;
        } else if (!strcmp(argv[i], "FM2")) {
	  SWbits[0][iAction] |= BIT_FM2;
        } else if (!strcmp(argv[i], "FM3")) {
	  SWbits[0][iAction] |= BIT_FM3;
        } else if (!strcmp(argv[i], "FM4")) {
	  SWbits[0][iAction] |= BIT_FM4;
        } else if (!strcmp(argv[i], "FM5")) {
	  SWbits[0][iAction] |= BIT_FM5;
        } else if (!strcmp(argv[i], "FM6")) {
	  SWbits[0][iAction] |= BIT_FM6;
        } else if (!strcmp(argv[i], "FM7")) {
	  SWbits[1][iAction] |= BIT_FM7;
        } else if (!strcmp(argv[i], "FM8")) {
	  SWbits[1][iAction] |= BIT_FM8;
        } else if (!strcmp(argv[i], "FM9")) {
	  SWbits[1][iAction] |= BIT_FM9;
        } else if (!strcmp(argv[i], "FM10")) {
	  SWbits[1][iAction] |= BIT_FM10;
        } else if (!strcmp(argv[i], "LIMIT")) {
	  SWbits[1][iAction] |= BIT_LIMIT;
        } else if (!strcmp(argv[i], "OUTPUT")) {
	  SWbits[1][iAction] |= BIT_OUTPUT;
        } else if (!strcmp(argv[i], "DECIMATION")) {
	  SWbits[1][iAction] |= BIT_DECIMATION;
        } else if (!strcmp(argv[i], "HOLD")) {
	  SWbits[1][iAction] |= BIT_HOLD;
        } else if (!strcmp(argv[i], "FMALL")) {
	  SWbits[0][iAction] |= BIT_FM1 | BIT_FM2 | BIT_FM3 | BIT_FM4 | BIT_FM5 | BIT_FM6;
	  SWbits[1][iAction] |= BIT_FM7 | BIT_FM8 | BIT_FM9 | BIT_FM10;
        } else if (!strcmp(argv[i], "ALL")) {
	  SWbits[0][iAction] |= BIT_INPUT | BIT_OFFSET | \
	               BIT_FM1 | BIT_FM2 | BIT_FM3 | BIT_FM4 | BIT_FM5 | BIT_FM6;
	  SWbits[1][iAction] |= BIT_FM7 | BIT_FM8 | BIT_FM9 | BIT_FM10 | \
	               BIT_LIMIT | BIT_OUTPUT | BIT_DECIMATION | BIT_HOLD;
        } else if (!strcmp(argv[i], "ON")  || !strcmp(argv[i], "1")) {
	  bitsSelected = bitsSelected || SWbits[0][iAction] || SWbits[1][iAction];
	  action[iAction++] = SW_ON;
        } else if (!strcmp(argv[i], "OFF") || !strcmp(argv[i], "0")) {
	  bitsSelected = bitsSelected || SWbits[0][iAction] || SWbits[1][iAction];
	  action[iAction++] = SW_OFF;
        } else if (!strcmp(argv[i], "TOGGLE")) {
	  bitsSelected = bitsSelected || SWbits[0][iAction] || SWbits[1][iAction];
	  action[iAction++] = SW_TOGGLE;
	} else {
          strcpy (basename, argv[i]);
	}
    }
    basenameLen=strlen(basename);

      /* help */
      if (iAction == 0 || (!bitsSelected) || (basenameLen == 0)) {
         printf ("Usage: ezcaswitch <base FM name> <buttons> [ON|OFF|TOGGLE]\n"
                 "                  <base FM name>: e.g. H1:ASC-WFS4_PIT\n"
		 "                  <buttons>: at least on of\n"
		 "                             INPUT \n"
		 "                             OFFSET \n"
		 "                             FM1 ... FM10 \n"
		 "                             LIMIT \n"
		 "                             OUTPUT \n"
		 "                             DECIMATION\n"
		 "                             HOLD\n"
		 "                             FMALL\n"
		 "                             ALL\n"
		 "                  <buttons> [ON|OFF|TOGGLE] can be repeated up to 32 times.\n");
         return 1;
      }
//------------ make sure the basename is right --------
  int pos;
  if(!strcmp(basename+(pos=basenameLen>3 ? basenameLen-3:0),"_SW")) {
    basename[pos]=0;
    printf("_SW\n");
  } else if(!strcmp(basename+(pos=basenameLen>4 ? basenameLen-4:0),"_SWR")) {
    basename[pos]=0;
    printf("_SWR\n");
  } else if(!strcmp(basename+pos,"_SW1")) {
    basename[pos]=0;
    printf("_SW1\n");
  } else if(!strcmp(basename+pos,"_SW2")) {
    basename[pos]=0;
    printf("_SW2\n");
  } else if(!strcmp(basename+(pos=basenameLen>5 ? basenameLen-5:0),"_SW1R")) {
    basename[pos]=0;
    printf("_SW1R\n");
  } else if(!strcmp(basename+pos,"_SW2R")) {
    basename[pos]=0;
    printf("_SW2R\n");
  }

  allSWbits[0]=allSWbits[1]=0;
  needReadback[0]=needReadback[1]=false;
  for(int i=0;i<iAction;i++) {
    allSWbits[0]|=SWbits[0][i];
    allSWbits[1]|=SWbits[1][i];
    needReadback[0] = needReadback[0] || ( (action[i] != SW_TOGGLE) && SWbits[0][i] );
    needReadback[1] = needReadback[1] || ( (action[i] != SW_TOGGLE) && SWbits[1][i] );
  }
   
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.02);
      ezcaSetRetryCount (500);
      double val;
  for(int swnr=0;swnr<2;swnr++) {
    if(allSWbits[swnr]) {
      sprintf(channel ,"%s%s" ,basename,SWname[swnr]);
      sprintf(readback,"%s%sR",basename,SWname[swnr]);
      int old = 0;
      if (needReadback[swnr]) {
         double e;
         if (ezcaGet (readback, ezcaDouble, 1, &e) != EZCA_OK) {
            printf ("readback %s not accessible\n", channel);
            return 1;
         }
         old = (e > 0) ? (int)(e+0.5) : -(int)(-e+0.5);
         //int inp  = (d > 0) ? (int)(d+0.5) : -(int)(-d+0.5);
      	 // shift first
         if (shift > 0) {
            old >>= shift;
         } 
         else if (shift < 0) {
            old <<= (-shift);
         }
      }
      // logic operations
      int state=old;
      for(int i=0;i<iAction;i++) {
        switch (action[i]) {
              case SW_ON:
	          state= state |  SWbits[swnr][i];
	        break;
              case SW_OFF:
	          state= state & ~SWbits[swnr][i];
	        break;
	      default:
	          state= state ^ SWbits[swnr][i];
	        break;
           }
      }
      val = old ^ state;
   
      if (ezcaPut (channel, ezcaDouble, 1, &val) != EZCA_OK) {
         printf ("channel %s not accessible\n", channel);
         return 1;
      }
      if (ezcaGet (readback, ezcaDouble, 1, &d) != EZCA_OK) {
         printf ("readback %s not accessible\n", channel);
         return 1;
      }
      printf ("%s = %g\n", readback, d);
    }
  }
   
   #endif
      return 0;
   }

