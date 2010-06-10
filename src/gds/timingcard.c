#ifdef OS_VXWORKS
#include <vxWorks.h>    /* Generic VxWorks header file */
#include <vme.h>        /* VME access defines */
#include <sysLib.h>
#include <taskLib.h>
#include <vxLib.h>
#endif
#include <stdio.h>
#include "dtt/hardware.h"
#include "dtt/timingcard.h"


#define  TIMING_CARD_INIT       0
#define  TIMING_CARD_ARM        3
#define  TIMING_CARD_RESET      4
#define  TIMING_CARD_ERR        8


   static unsigned short *tcPtr = 0;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: initTimingCard			      	*/
/*                                                         		*/
/* Procedure Description: Initialize Timing Card.                       */
/*                                                         		*/
/* Procedure Arguments: none.						*/
/*                                                         		*/
/* Procedure Returns: OK or ERROR					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int initTimingCard()
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
      unsigned short val;
      int status;
      
      /* Decode Timing Card base address */
      status = sysBusToLocalAdrs(TIMINGCARD_0_ADRMOD,
   		     (char*)(TIMINGCARD_0_BASE_ADDRESS), (char**)&tcPtr);
      printf("timimgboardInit: sysBusToLocalAdrs = 0x%x\n", 
              (int) TIMINGCARD_0_BASE_ADDRESS);
      if (status != OK) {
        printf("initTimingCard: Error, sysBusToLocalAdrs\n");
        tcPtr = 0;
        return(ERROR);
      }
   
      /* Check that Timing Card is present */
      status = vxMemProbe((char *)tcPtr,
   	      VX_READ,
   	      2,
   	      (char *)&val);
      if (status != OK) {
        printf("initTimingCard: Error, vxMemProbe\n");
        tcPtr = 0;
        return(ERROR);
      }
   
      return(OK);
   #endif
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: resetTimingCard			        */
/*                                                         		*/
/* Procedure Description: Reset Timing Card.                            */
/*                                                         		*/
/* Procedure Arguments: none.						*/
/*                                                         		*/
/* Procedure Returns: OK        					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int resetTimingCard() 
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
      if (tcPtr == 0) {;
         return(ERROR);
      }
   
      *tcPtr = (unsigned short)TIMING_CARD_RESET;
      taskDelay(1);
      /* *tcPtr = (unsigned short)TIMING_CARD_INIT;*/
   
      return (OK);
   #endif
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: armingTimingCard			      	*/
/*                                                         		*/
/* Procedure Description: Arming Timing Card.                           */
/*                                                         		*/
/* Procedure Arguments: none.						*/
/*                                                         		*/
/* Procedure Returns: OK or ERROR					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int armingTimingCard() 
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
      if (tcPtr == 0) {;
         return(ERROR);
      }
      *tcPtr=(unsigned short) TIMING_CARD_ARM;
      return (OK);
   #endif
   }



/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: statusTimingCard			      	*/
/*                                                         		*/
/* Procedure Description: Return status of Timing Card.                 */
/*                                                         		*/
/* Procedure Arguments: none.						*/
/*                                                         		*/
/* Procedure Returns: status    					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int statusTimingCard() 
   {
   #ifndef OS_VXWORKS
      return -1;
   #else
      unsigned short status; /* status */
      if (tcPtr == 0) {
         return(ERROR);
      }
      status = *tcPtr;
      status &= 0x003F;
      return (status);
   #endif
   }
