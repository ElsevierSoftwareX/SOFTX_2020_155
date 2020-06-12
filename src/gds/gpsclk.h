/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gpsclk							*/
/*                                                         		*/
/* Module Description: 	This routines are used to interface the  	*/
/*			VME-SYNCCLOCK32 boards				*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	          D. Barker    	First release		   		*/
/* 0.2	 18Jul98  D. Sigg    	Added this interface	   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: awgfunc.html						*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
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

#ifndef _GDS_GPSCLK_H
#define _GDS_GPSCLK_H

#ifdef __cplusplus
extern "C" { 
#endif

#include <inttypes.h>

/* Header File List: */

/** @name GPS clock API
    This routines are used to interface the VME-SYNCCLOCK32 boards.

    @memo Interface to the Brandywine VME-SYNCCLOK32
    @author Written July 1998 by Daniel Sigg
    @version 0.1
    @see GPS clock API, manual of VME-SYNCCLOCK32
************************************************************************/

/*@{*/


/** @name Data types
    * Data types of the  GPS clock API.

    @memo Data types
    @author DS, July 98
    @see  GPS clock API
************************************************************************/

/*@{*/

/** GPS info structure. Contains status, number of satellites, 
    latitude, longitude, altitude, speed and direction.
  
    @author DS, July 98
    @version 0.1
    @see GPS clock API
************************************************************************/
   struct gpsInfo_t {
      /** Status: 1 navigate - 0 acquiring */
      short		status;
      /** Number of GPS satellites tracked */
      short		numSatellite;
      /** Synchronization status; see gpsSyncInfo for bit codes */
      short		syncStatus;
      /** GPS time in seconds */
      unsigned long	time;
      /** GPS position: latitude in deg (negative for south) */
      double		latitude;
      /** GPS position: longitude in deg (negative for west) */
      double		longitude;
      /** GPS position: altitude in m over geoid */
      double		altitude;
      /** Speed over ground in m/s */
      double 		speed;
      /** Direction of movement in deg */
      double		direction;
   };
   typedef struct gpsInfo_t gpsInfo_t;

#ifndef _BYTEWORD_T
#define _BYTEWORD_T
   typedef unsigned char byte;
   typedef unsigned short word;
   typedef unsigned int lword;
#endif

/** This is the layout of the SYNCCLOCK32 register area.
  
    NOTE: The align* members are only there for correct memory sizing
    and alignment. Reading/writing from/to the align* members is
    not recommended.

    @author DS, July 98
    @version 0.1
    @see GPS clock API
************************************************************************/
   struct GPSREGS 
   {
      byte 		align0;
      /** Heartbeat interrupt control register */
      byte		Heartbeat_Intr_Ctl;	/* $01 */
      byte 		align1;
      /** Match interrupt control register */
      byte 		Match_Intr_Ctl;		/* $03 */
      byte		align2;
      /** Time tag interrupt control register */
      byte		Time_Tag_Intr_Ctl;	/* $05 */
      byte		align3;
      /** RAM FIFO interrupt control register */
      byte		RAM_Fifo_Ctl;		/* $07 */
      byte		align4;
      /** Heartbeat interrupt vector */
      byte		Heartbeat_Intr_Vec;	/* $09 */
      byte		align5;
      /** Match interrupt vector */
      byte		Match_Intr_Vec;		/* $0B */
      byte		align6;
      /** Time tag interrupt vector */
      byte		Time_Tag_Intr_Vec;	/* $0D */
      byte		align7;
      /** RAM FIFO interrupt vector */
      byte		Ram_Fifo_Vec;		/* $0F */
      byte		align8;
      /** RAM FIFO */
      byte		RAMFifo;		/* $11 */
      byte		align9;
      /** Board Status register */
      byte		Status;			/* $13 */
      byte		align10;
      /** Heartbeat interrupt vector */
      byte		Ext_100ns;		/* $15 */
      byte		align11;
      /** External time tag polarity */
      byte		Ext_Polarity;		/* $17 */
      /** GPS time low word */
      lword		Sec40_Usec1;		/* $18 */
      /** GPS time high word */
      lword		Year8_Min1;		/* $1C */
      byte		align12;
      /** Dual ported RAM data register */
      byte		DPData;			/* $21 */
      byte		align13;
      /** Dual ported RAM address register */
      byte		DPAddr;			/* $23 */
      byte		align14;
      /** Reset register */
      byte		Resets;			/* $25 */
      byte		align15[2];
      /** GPS time of time tag low word */
      lword		Ext_Sec40_Usec1;	/* $28 */
      /** GPS time of time tag high word */
      lword		Ext_Year8_Min1;		/* $2C */
   };
   typedef struct GPSREGS GPSREGS;


/** This is the function prototype of the routine called by the 
    heartbeat interrupt service routine. The time of the interrupt
    is passed in the board native format.
  
    @param timehi high word of GPS time
    @param timelo low word of GPS time
    @return void
    @author DS, July 98
    @version 0.1
    @see GPS clock API, manual of VME-SYNCCLOCK32
************************************************************************/
   typedef void (*gpsISR_func) (unsigned int timehi, 
   unsigned int timelo);


/*@}*/

/** @name Functions
    * Functions of the GPS clock API.

    @memo Functions
    @author DS, July 98
    @see GPS clock API
************************************************************************/

/*@{*/

/** Returns the VME base address of the GPS board. 

    @param ID board ID 
    @return address if successful, NULL otherwise
    @author DS, July 98 
    @see GPS clock API
************************************************************************/
   char* gpsBaseAddress (short ID);

/** Returns true if board is a GPS master. 

    @param ID board ID 
    @return true of master, false if secondary
    @author DS, July 98 
    @see GPS clock API
************************************************************************/
   int gpsMaster (short ID);


/** Initializes the GPS board. This routine initializes the 
    VME-SYNCCLOK32 board and sets the year information for IRIG-B
    secondary modules. If a NULL pointer is provided for the year
    information, neither the year, nor the leap year information 
    is set. Master boards ignore the provided year information.

    @param ID board ID
    @param pointer to year information
    @return 0 if successful, <0 otherwise
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int gpsInit (short ID, int* year);


/** Returns the synchronisation status. If the statusEx is not NULL,
    the extended status information is returned.

    \begin{verbatim}
    bit 0	set if NOT in synchronization
    bit 1	set if selected input code NOT decodeable
    bit 2	set if PPS input invalid
    bit 3	set if major time NOT set since jam
    bit 4	set if year NOT set
    \end{verbatim}

    @param ID board ID
    @param statusEx extended synchronization status
    @return 1 if synchronized, 0 otherwise, -1 on error
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int gpsSyncInfo (short ID, short* statusEx);


/** Returns time in GPS seconds. This routine gives the time in nano 
    seconds. It returns zero if the year information was not correctly 
    initialized.

    @param ID board ID
    @return GPS time in nsec
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int64_t gpsTimeNow (short ID);


/** Converts native time into GPS seconds. This routine returns the time 
    in nano seconds. This routine returns zero if the year information 
    was not correctly initialized.

    @param ID board ID
    @param timehi GPS time high word
    @param timelo GPS time low word
    @return GPS time in nsec
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int64_t gpsTime (short ID, unsigned int timehi, 
   unsigned int timelo);


/** Reads the native time from the GPS board. This routine returns the 
    time in the native board format: two long words formated as 
    YDDDHHMM and SSmmmuuu (BCD). An 'F' in the year field indicates 
    the lack of the correct year infomation available on the board.

    @param ID board ID
    @param pointer to high time word
    @param pointer to low time word
    @return void
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   void gpsNativeTime (short ID, unsigned int* timehi, 
   unsigned int* timelo);

/** Reads the micro seconds of the current time from the GPS board.

    @param ID board ID
    @return time in micro sec
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   unsigned int gpsMicroSec (short ID);


/** Returns GPS information. This routine returns the number of 
    satellites, longitude, latitude, altitude, speed and direction.
    This routine fails for secondary boards. If a NULL pointer is
    specified, only the number of tracked satellites is returned.

    @param ID board ID
    @param info pointer to gps info structure (result)
    @return numbre of tracked satellites, <0 if failed
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int gpsInfo (short ID, gpsInfo_t* info);


/** Installs an interrupt service routine for the GPS heartbeat clock.
    Interrupts are generated at a rate of 64Hz; however, the callback
    routine is only invoked at a rate of 16Hz - aligned with the GPS
    1pps signal. Since this routine has to synchronize with the 1pps
    signal, it will not return unit the next second starts. The first
    interrupt will be 62.5 ms after the second boundary. 

    @param ID board ID
    @param ISR interrupt service routine
    @return 0 if successful, <0 otherwise
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int gpsHeartbeatInstall (short ID, gpsISR_func ISR);

/** Monitors the health of the heartbeat. Returns the averaged time
    delay of the interrupt service routine. The average is taken over
    roughly the last second. The returned time is in micro seconds.
    If the heartbeat works smoothly, this number should be below 1 ms.

    Note: due to rounding errors

    @param ID board ID
    @return averaged heartbeat delay in us
    @author DS, July 98
    @see GPS clock API
************************************************************************/
   int gpsHeartbeatHealth (short ID);

/*@}*/

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_GPSCLK_H */
