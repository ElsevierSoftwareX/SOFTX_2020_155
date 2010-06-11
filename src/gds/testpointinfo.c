/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: testpointinfo						*/
/*                                                         		*/
/* Module Description: utility functions for handling test points	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
/*
#ifndef DEBUG
#define DEBUG
#endif
*/
#if 0
#ifndef _TESTPOINT_DIRECT
#ifdef OS_VXWORKS
#if (IFO == GDS_IFO1)
#define _TESTPOINT_DIRECT	1
#elif (IFO == GDS_IFO2) 
#define _TESTPOINT_DIRECT	2
#elif (IFO == GDS_PEM)
#define _TESTPOINT_DIRECT	3
#else
#define _TESTPOINT_DIRECT	0
#endif
#else
#define _TESTPOINT_DIRECT	0
#endif
#endif
#endif

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include "dtt/gdsutil.h"
#include "dtt/rmorg.h"
#include "dtt/testpointinfo.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpIsValid					*/
/*                                                         		*/
/* Procedure Description: gets a test point index			*/
/*                                                         		*/
/* Procedure Arguments: channel info, node (return), test point (return)*/
/*                                                         		*/
/* Procedure Returns: 1 if test point, 0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpIsValid (const gdsChnInfo_t* chn, int* node, testpoint_t* tp)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      /* test channel */
      if (chn == NULL) {
         return 0;
      }
   
      /* is test point? */
      /*if ((IS_TP (chn->dcuId)) && */
      /*printf ("TP VALID tp=%i chn=%i (%i)\n", chn->tpNum, 
                chn->chNum, chn->rmId);*/
      if ((IS_TP (chn)) && 
         (TP_ID_TO_INTERFACE (chn->chNum) >= 0)) {
         /* copy return arguments */
         if (node != NULL) {
            *node = chn->rmId;
         }
         if (tp != NULL) {
            *tp = chn->chNum;
         }
         return 1;
      }
      else {
         return 0;
      }
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpIsValidName				*/
/*                                                         		*/
/* Procedure Description: gets a test point index			*/
/*                                                         		*/
/* Procedure Arguments: channel name, node (return), test point (return)*/
/*                                                         		*/
/* Procedure Returns: 1 if test point, 0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpIsValidName (const char* chnname, int* node, testpoint_t* tp)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      gdsChnInfo_t	info;		/* channel info */
   
      if (gdsChannelInfo (chnname, &info) < 0) {
         return 0;
      }
      return tpIsValid (&info, node, tp);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpType					*/
/*                                                         		*/
/* Procedure Description: returns the test point type			*/
/*                                                         		*/
/* Procedure Arguments: channel info					*/
/*                                                         		*/
/* Procedure Returns: type of test point				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   testpointtype tpType (const gdsChnInfo_t* chn)
   {
   #ifdef _NO_TESTPOINTS
      return tpInvalid;
   #else
      testpoint_t	tp;	/* test point id */
   
      if (chn == NULL) {
         return tpInvalid;
      }
   
      /* valid test point? */
      if (!tpIsValid (chn, NULL, &tp)) {
         return tpInvalid;
      }
   
      /* check tp id */
      switch (TP_ID_TO_INTERFACE (tp)) {
         /* LSC excitation test point channel */
         case TP_LSC_EX_INTERFACE:
            return tpLSCExc;
         /* LSC test point channel */
         case TP_LSC_TP_INTERFACE:
            return tpLSC;
         /* ASC excitation test point channel */
         case TP_ASC_EX_INTERFACE:
            return tpASCExc;
         /* ASC test point channel */
         case TP_ASC_TP_INTERFACE:
            return tpASC;
         /* DAC channel */
         case TP_DAC_INTERFACE:
            return tpDAC;
            /* DS340 channel */
         case TP_DS340_INTERFACE:
            return tpDSG;
            /* not an excitation test point/channel */
         default : 
            return tpInvalid;
      }
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpTypeName					*/
/*                                                         		*/
/* Procedure Description: returns the test point type			*/
/*                                                         		*/
/* Procedure Arguments: channel name					*/
/*                                                         		*/
/* Procedure Returns: type of test point				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   testpointtype tpTypeName (const char* chnname)
   {
   #ifdef _NO_TESTPOINTS
      return tpInvalid;
   #else
   
      gdsChnInfo_t		info;		/* channel info */
   
      if (gdsChannelInfo (chnname, &info) < 0) {
         return tpInvalid;
      }
      return tpType (&info);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpReadback					*/
/*                                                         		*/
/* Procedure Description: returns the readback channel of a test point	*/
/*                                                         		*/
/* Procedure Arguments: channel info, readback channel info (return)	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpReadback (const gdsChnInfo_t* chn, gdsChnInfo_t* rb)
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      if ((chn == NULL) || (rb == NULL)) {
         return -1;
      }
   
      /* test if DAC or DS340 channel */
      /*if (tpType (chn) == tpDAC) {
         return -2;
      }
      else */ 
      if (tpType (chn) == tpDSG) {
         return -2;
      }
      else {
         /* otherwise assume it is the same */
         memcpy (rb, chn, sizeof (gdsChnInfo_t));
      }
      return 0;
   
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: tpReadbackName				*/
/*                                                         		*/
/* Procedure Description: returns the readback channel of a test point	*/
/*                                                         		*/
/* Procedure Arguments: channel name, readback channel name (return)	*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int tpReadbackName (const char* chnname, char* rbname) 
   {
   #ifdef _NO_TESTPOINTS
      return -10;
   #else
   
      gdsChnInfo_t	info;		/* channel info */
      gdsChnInfo_t	rbinfo;		/* readback channel info */
      int		retval;		/* return value */
   
      if ((chnname == NULL) || (rbname == NULL)) {
         return -1;
      }
      if (gdsChannelInfo (chnname, &info) < 0) {
         return -2;
      }
      retval = tpReadback (&info, &rbinfo);
      if (retval < 0) {
         return retval;
      }
      strcpy (rbname, rbinfo.chName);
      return 0;
   #endif
   }
