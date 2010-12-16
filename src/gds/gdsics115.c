static char *versionId = "Version $Id$" ;

#ifdef OS_VXWORKS
#include <vxWorks.h>    /* Generic VxWorks header file */
#include <semLib.h>     /* VxWorks Semaphores */
#include <sysLib.h>     /* VxWorks system library */
#include <stdio.h>      /* VxWorks standard io */
#include <stdlib.h>     /* VxWorks function prototypes */
#include <time.h>       /* General Time definitions */
#include <taskLib.h>    /* VxWorks Task Definitions */
#include <intLib.h>     /* VxWorks Interrupts Definitions */
#include <vme.h>        /* VME access defines */
#include <iv.h>		/* Interrupt Vector Header File. */
/* #include <arch/mips/ivMips.h>   Baja 47 Interrupt Vector Header File. */
#include <string.h>     /* VxWorks strings files */

#include "dtt/ics115.h"
#include "dtt/gdsics115.h"

   
/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115BoardInit                             */
/*                                                                      */
/* Procedure Description: Initialise the 115 card.                      */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int  ICS115BoardInit(char *base, int intlvl, int ivector)
   {
   
   /* board reset */
      *(unsigned long*)(base+MRST_OFFSET) = 0L;
   
   /* zero controller and SCV 64 VME chip vme interrupt registers */
      *(unsigned long*)(base+DAC_CTRL_OFFSET) = 0L;
      *(unsigned long*)(base+SCV64_VINT) = 0L;
   
   /* set int vector */
      *(unsigned long*)(base+SCV64_IVECT) = ivector;
   
      *(unsigned long*)(base+SCV64_VINT) = (intlvl&0x7L)|0x8L;
   
   /* default SCV64_MODE = 1001 0100 1000 0000 1110 0100 0000 0001 */
      *(unsigned long *)(base+SCV64_MODE) = 0x9480e401L;
   
   /* set IRQ1 to interrupt */
      *(unsigned long *)(base+SCV64_GENCTL) &=~0x1;
   
   
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115BoardReset                            */
/*                                                                      */
/* Procedure Description: Initialise the 115 card.                      */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115BoardReset(char *base)
   {
   
   /* Reset the 115 board */
      *(unsigned long*)(base+MRST_OFFSET) = 0L;
   
      return OK;
   }

/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115ConfigSet                             */
/*                                                                      */
/* Procedure Description: Initialise the 115 card.                      */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                          ICS115_CONFIG *cptr Pointer to config       */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115ConfigSet(char *base, ICS115_CONFIG *cptr)
   {
/*      int i;
      printf ("sizeof ICS115_CONFIG %i\n", (int)sizeof(ICS115_CONFIG));*/
/*      for (i = 0; i < 4; ++i) 
         *(unsigned long*)(base+DAC_CONFIG_OFFSET+4*i) =
                (*(((unsigned long*)cptr)+i));*/
      *(ICS115_CONFIG*)(base+DAC_CONFIG_OFFSET) = *((ICS115_CONFIG*) cptr);
/*      for (i = 0; i < 4; ++i)
         printf ("ICS115 CONFIG %i in/out  = 0x%lx, 0x%lx\n", i,
                 *(((unsigned long*)cptr)+i), 
		 *((unsigned long*)(base+DAC_CONFIG_OFFSET + 4*i)));*/	 
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115SeqSet                                */
/*                                                                      */
/* Procedure Description: Set the sequence.                             */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                          unsigned long *seq  Pointer to seq array    */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115SeqSet(char *base, unsigned long *seq)
   {
      int len;
      int i;
   
      len = *(unsigned long*)(base+DAC_CONFIG_OFFSET)&0x1f;
      for (i=0; i<=len; i++) 
   #ifdef PROCESSOR_BAJA47
         ((unsigned long*)(base+SEQ_OFFSET))[i] = seq[i];
   #elif defined(PROCESSOR_I486)
         ((unsigned long*)(base+SEQ_OFFSET))[i] = seq[i ^ 1];
   #else
   #error "Define correct sequencer for ICS115"
   #endif
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115MuteSet                               */
/*                                                                      */
/* Procedure Description: Set the mute data                             */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                          unsigned long *muter Pointer to mute data   */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115MuteSet(char *base, unsigned long *muter)
   {
   
      *(unsigned long*)(base+DAC_MUTE_OFFSET) = *((unsigned long*) muter);
   
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115ControlSet                            */
/*                                                                      */
/* Procedure Description: Set the control data                          */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                          ICS115_CONTROL *cntrl Pointer to control    */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115ControlSet(char *base, ICS115_CONTROL *cntrl)
   {
      /*printf ("sizeof ICS115_CONTROL %i\n", (int)sizeof(ICS115_CONTROL));*/
      *(ICS115_CONTROL*)(base+DAC_CTRL_OFFSET) = *((ICS115_CONTROL*) cntrl);
      /*printf ("ICS115 CTRL in/out = 0x%lx, 0x%lx\n", 
              *(unsigned long*)cntrl, 
	      *(unsigned long*)(base+DAC_CTRL_OFFSET));*/
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115DACReset                              */
/*                                                                      */
/* Procedure Description: Reset the DAC                                 */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115DACReset(char *base)
   {
   
      *(unsigned long*)(base+DAC_RST_OFFSET) = 0L;
   
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115DACEnabel                             */
/*                                                                      */
/* Procedure Description: Enable (i.e. start) the DAC                   */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115DACEnable(char *base)
   {
   
      *(unsigned long*)(base+DAC_CTRL_OFFSET) |= 0x2000;
   
      return OK;
   }

/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115DACDisabel                            */
/*                                                                      */
/* Procedure Description: Disable (i.e. stop) the DAC                   */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115DACDisable(char *base)
   {
   
   /* Disable DAC*/
      *(unsigned long*)(base+DAC_CTRL_OFFSET) &= ~0x2000;
   
      return OK;
   }

/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115IntEnable                             */
/*                                                                      */
/* Procedure Description: Enable the VME interrupts                     */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115IntEnable(char *base)
   {
      *(unsigned long*)(base+DAC_CTRL_OFFSET) |= 0x2;
   
      return OK;
   }


/*----------------------------------------------------------------------*/
/*                                                                      */
/* External Procedure Name: ICS115IntDisable                            */
/*                                                                      */
/* Procedure Description: Disable the VME interrupts                    */
/*                                                                      */
/* Procedure Arguments: In: unsigned long *base Pointer to 115 base adr */
/*                                                                      */
/* Procedure Returns: ERROR or OK.                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
   int ICS115IntDisable(char *base)
   {
      *(unsigned long*)(base+DAC_CTRL_OFFSET) &= ~0x2;
      *(unsigned long*)(base+SCV64_VINT) |= 0x8L;
   
   
      return OK;
   }

#endif
