/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: epics							*/
/*                                                         		*/
/* Module Description: implements functions for accessing epics channels*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/* Header File List: */
#if !defined(OS_VXWORKS) && !defined(GDS_NO_EPICS)
#include "dtt/gdsutil.h"
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
   void ezcaAutoErrorMessageOff(void);
   int ezcaGet(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaPut(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaSetRetryCount(int retry);
   int ezcaSetTimeout(float sec);
#endif
#include "epics.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Types: 		*/
/*            								*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Globals: init		initialization state		 	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static int			init = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Forward declarations: 						*/
/*	initEpicsdriver		init of epics system			*/
/*	initEpicsdriver		cleanup of epics system			*/
/*      								*/
/*----------------------------------------------------------------------*/
#if !defined(OS_VXWORKS) && !defined(GDS_NO_EPICS)
   __init__(initEpicsdriver);
#ifndef GDS_NO_EPICS
#pragma init(initEpicsdriver)
#endif
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initEpicsdriver				*/
/*                                                         		*/
/* Procedure Description: initialize the epics channel access		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void initEpicsdriver (void)
   {
   #if !defined(OS_VXWORKS) && !defined(GDS_NO_EPICS)
      ezcaAutoErrorMessageOff();
      epicsTimeout (0.1, 4);
   #endif
      init = 1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: setupEpicsdriver				*/
/*                                                         		*/
/* Procedure Description: initialize the epics channel access		*/
/*                                                         		*/
/* Procedure Arguments: void						*/
/*                                                         		*/
/* Procedure Returns: void						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static void setupEpicsdriver (void)
   {
      if (init <= 0) {
         initEpicsdriver();
      }
      if (init != 1) {
         return;
      }
      init = 2;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: epicsGet					*/
/*                                                         		*/
/* Procedure Description: Gets the value of an EPICS channel		*/
/*                                                         		*/
/* Procedure Arguments: chnname, value					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int epicsGet (const char* chnname, double* value)
   {
   #if !defined(OS_VXWORKS) && !defined(GDS_NO_EPICS)
      double		d;		/* temp. value */
   #endif
   
      /* setp epics */
      if (init < 2) {
         setupEpicsdriver();
         if (init < 2) {
            return -1;
         }
      }
   #if defined(OS_VXWORKS) || defined(GDS_NO_EPICS)
      return -2;
   #else
      /* get channel data */
      if (ezcaGet ((char*) chnname, ezcaDouble, 1, &d) != EZCA_OK) {
         return -2;
      }
      if (value != NULL) {
         *value = d;
      }
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: epicsPut					*/
/*                                                         		*/
/* Procedure Description: Sets the value of an EPICS channel		*/
/*                                                         		*/
/* Procedure Arguments: chnname, value					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int epicsPut (const char* chnname, double value)
   {
      /* setp epics */
      if (init < 2) {
         setupEpicsdriver ();
         if (init < 2) {
            return -1;
         }
      }
   #if defined(OS_VXWORKS) || defined(GDS_NO_EPICS)
      return -2;
   #else
      /* set channel data */
      if (ezcaPut ((char*) chnname, ezcaDouble, 1, &value) != EZCA_OK) {
         return -2;
      }
      return 0;
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: epicsTimeout				*/
/*                                                         		*/
/* Procedure Description: Sets the timeout and retry limit 		*/
/*                                                         		*/
/* Procedure Arguments: timeout, retry					*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int epicsTimeout (double timeout, int retry)
   {
   #if !defined(OS_VXWORKS) && !defined(GDS_NO_EPICS)
      ezcaSetTimeout (timeout);
      ezcaSetRetryCount (retry);
   #endif
      return 0;
   }
