/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: GDS Reflective Memory PAI 				*/
/*                                                         		*/
/* Module Description:  GDS Reflective Memory API  prototypes		*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   4/8/98  PG	    Initial release              		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: HTML documentation  					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	Paul Govereau 617-253-6410		govereau@mit.edu       	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Sun Ultra					*/
/*	Compiler Used: CC & gcc						*/
/*	Runtime environment: Solaris, vxWorks				*/
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

#ifndef _GDS_RMAPI_H
#define _GDS_RMAPI_H

#ifdef __cplusplus
extern "C" {
#endif



 /**
    @name Reflective Memory API
    The reflective memory API is used to interface VMIC VMIVME-5588
    high speed reflective memory boards.

    @memo Interface for VMIVME-5588 reflective memory boards
    @author Written Mar. 1998 by Paul Govereau
    @version 0.1
*********************************************************************/

/*@{*/

#ifndef _BYTEWORD_T
#define _BYTEWORD_T
   typedef unsigned char byte;
   typedef unsigned short word;
   typedef unsigned int lword;
#endif

#define RMAPI_NODE_ALL 256

 /** This is the layout of the 5588 register area.
     The 5588 registers are sensitive to correct
     size and alignment of read/write operations.
     Using this structure will ensures the correct
     alignment and sizing of register accesses.

     NOTE:The align* members are only there for correct memory sizing
     and alignment. Reading/writing from/to the align* members is
     not recommended.
*********************************************************************/
   typedef struct _RMREGS
   /*@{*/
   {
      byte align0;
      /** board ID = 4B */
      byte bid;			/* $01 */
      /** Int and Recv Status */	
      byte irs;			/* $02 */
      byte align1;
      /** node ID */
      byte nid;			/* $04 */
      /** control and status */
      byte csr;			/* $05 */		
      /** command register */
      byte cmd;			/* $06 */
      /** command node register */
      byte cmdn;		/* $07 */
      byte align2[8];
      
      /** DMA - VME bus Addr */
      lword vdma_addr;		/* $10 */
      /** DMA - Local addr */
      lword ldma_addr;	/* $14 */
      /** DMA - Length */
      lword dmal;		/* $18 */
      /** DMA - control register */
      lword dmac;		/* $1C */
      
      /** INT 4(dma) control register */
      byte cr4;			/* $20 */
      /** INT 4 int vector number */
      byte vr4;			/* $21 */
      /** int0 and int4 control and status */
      byte int_csr;		/* $22 */
      /** int 0(error) control register */   
      byte cr0;			/* $23 */
      byte align3[2];
      
      /** int 1 sender id */
      byte sid1;			/* $26 */
      /** int 1 control register */
      byte cr1;			/* $27 */
      byte align4[2];
      
      /** int 2 sender id */
      byte sid2;			/* $2A */
      /** int 2 control register */
      byte cr2;			/* $2B */
      byte align5[2];
      
      /** int 3 sender id */
      byte sid3;			/* $2E */
      /** int 3 control register */
      byte cr3;   			/* $2F */
      byte align6[3];
      
      /** int 0 interrupt vector */
      byte vr0;			/* $33 */
      byte align8[3];
      
      /** int 1 interrupt vector */
      byte vr1;			/* $37 */
      byte align10[3];
      
      /** int 2 interrupt vector */
      byte vr2; 			/* $3B */
      byte align12[3];
      
      /** int 3 interrupt vector */
      byte vr3;			/* $3F */
   } RMREGS;


/*@}*/	

/** Initializes the Reflective Memory API.

    @param ID board id
    @return 0 if successful, <0 otherwise
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   int rmInit (short ID);

/** Returns the base address of the reflective memory ring. The
    reflective memory identification number specifies the module
    number. This may not be the base address of the reflective memory 
    board, but rather it describe the base address of the logical 
    addressing to access the reflective memory.

    @param ID board id
    @return VME base address
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   char* rmBaseAddress (short ID);

/** Returns the base address of the reflective memory board. The
    reflective memory identification number specifies the module
    number.

    @param ID board id
    @return VME base address
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   char* rmBoardAddress (short ID);

/** Returns the size of the reflective memory. The
    reflective memory identification number specifies the module
    number.

    @param ID board id
    @return VME base address
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   int rmBoardSize (short ID);

#if 0
/** Default node number to select all nodes (256).
    
    @see GDS Reflective Memory API
*********************************************************************/
#define RMAPI_NODE_ALL
#endif    

 /** Turns the FAIL led on or off.
     setting the FAIL led will have no effect on the
     operation of the 5588 board.  The 5588 does not
     set or clear the FAIL led under any circumstances.

     @param ID board id
     @param led_on {TRUE | FALSE}
     @return none.
     @author Paul Govereau, April 1998 
     @see GDS Reflective Memory API
*********************************************************************/
   int rmLED (short ID, int led_on);

 /** Resets the given node.

     @param ID board id
     @param node node id to reset = {0..255 | RMAPI_NODE_ALL}
     @return 0 if successful, <0 otherwise
     @author Paul Govereau, April 1998 
     @see GDS Reflective Memory API
*********************************************************************/
   int rmResetNode (short ID, int node);

/** Bypasses the current reflected memory node.

    NOTE: you must have an external optical switch installed
    for this to have effect.

    @param ID board id
    @param bypass {TRUE | FALSE}
    @return 0 if successful, <0 otherwise
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   int rmNodeBypass (short ID, int bypass);

 /** Generates an interrupt on the given node.

     @param ID board id
     @param num interrupt number = {1,2,3}
     @param node node id = {0..255 | RMAPI_NODE_ALL}
     @return 0 if successful, <0 otherwise
     @author Paul Govereau, April 1998 
     @see GDS Reflective Memory API
*********************************************************************/
   int rmInt (short ID, int inum, int node);

/** Checks if the specified memory region is accessible through
    Reflective Memory. Returns false if the address range falls
    outsize the memory window supported by the board.
    The offset is taken relative to the base address.

    @param ID board id
    @param offset offset into reflected memory
    @param size number of bytes to copy
    @return true if valid address range, false otherwise
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   int rmCheck (short ID, int offset, int size);

/** This is the function prototype of the interrupt service routine 
    called by an reflective memory interrupt.
  
    @param ID board id of board which caused the interrupt
    @param inum reflective memory interrupt number
    @param cause cause of interrupt (sender node or INT0-4_CSR) 
    @param arg optional user argument
    @return void
*********************************************************************/
   typedef void (*rmISR) (short ID, int inum, int cause, void* arg);

/** Installs an interrupt handler. The installed interrupt service
    is called indirectly from a function which takes care of all the
    book keeping (e.g, clears the interrupt register). Use this 
    function with a NULL isr to remove the interrupt service.

    @param ID board id
    @param inum interrupt number
    @param isr interrupt service routine
    @param arg optional user argument which is passed to the ISR
    @return 0 if successful, <0 otherwise
*********************************************************************/
   int rmIntInstall (short ID, int inum, rmISR isr, void* arg);

/** Installes a watch on an interrupt. The default behaviour is to
    send a gdsWarningMessage. If the interrupt number is negative,
    a all interrupts are monitored.

    @param ID board id
    @param inum interrupt number to watch
    @return 0 if successful, <0 otherwise
*********************************************************************/
   int rmWatchInt (short ID, int inum);

/** Reads from Reflective Memory using the Baja board as the DMA 
    controller.

    @param ID board id
    @param pData pointer to local memory buffer
    @param offset offset into reflected memory
    @param size number of bytes to copy
    @param flag VME64 and DMA controller 
    @return 0 if successful, <0 otherwise
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   int rmRead (short ID, char *pData, int offset, int size, 
              int flag);

/** Writes to Reflective Memory using the Baja board as the DMA 
    controller.

    @param ID board id
    @param pData pointer to local memory buffer
    @param offset offset into reflected memory
    @param size number of bytes to copy
    @param flag VME64 and DMA controller 
    @return 0 if successful, <0 otherwise
    @author Paul Govereau, April 1998 
    @see GDS Reflective Memory API
*********************************************************************/
   int rmWrite (short ID, char *pData, int offset, int size, 
               int flag);

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /* _GDS_RMAPI_H */




