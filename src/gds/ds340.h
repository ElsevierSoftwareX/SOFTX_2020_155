/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ds340							*/
/*                                                         		*/
/* Module Description: API for controling a DS340 AWG			*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 24Jan98  M. Pratt   	First release		   		*/
/* 0.1	 24Jan99  D. Sigg    	Second release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: ds340.html						*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-8137  sigg_d@ligo.mit.edu	*/
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

#ifndef _GDS_DS340_H
#define _GDS_DS340_H

#ifdef __cplusplus
extern "C" {
#endif

/** @name DS340 driver
    This API implements a driver for the DS340 signal generator.

    @memo Driver for controling a DS340
    @author Written January 1998 by Mark Pratt
    @version 0.2
************************************************************************/

/*@{*/

/** @name Constants and flags.
    * Constants and flags of the DS340 driver.

    @memo Constants and flags
    @author MP, January 98
    @see DS340 driver
************************************************************************/

/*@{*/


/** Constant which specifies the maximum number of simultanously 
    supported DS340 arbitrary waveform generators; default is 10 */
#define NUM_DS340		10

/** Constant which specifies the buffer length; default is 1024. */
#define DS340_BUFLEN		1024


/*@}*/

/** @name Status and control bits.
    Status and control bits of the DS340 driver.

    @memo Status and control bits
    @author MP, January 98
    @see DS340 driver
************************************************************************/

/*@{*/


/* This is effectively control for a DS335 ie. no arb. waveform control */
/** Flag which specifies that the DS340 is alive. */
#define DS340_ALIVE		0x002

/** Flag which specifies that the DS340 is locked. */
#define DS340_LOCK		0x004

/** Flag which specifies that the DS340 is connected through a cobox. */
#define DS340_COBOX		0x008

/*@}*/

/** @name Command bits.
    Commands bits of the DS340 driver.

    @memo Command bits
    @author MP, January 98
    @see DS340 driver
************************************************************************/

/*@{*/

/** Command which specifies that the DS340 should be reset.
************************************************************************/
#define DS340_RST		0x001

/** Command which specifies that the DS340 should be cleared.
************************************************************************/
#define DS340_CLS		0x002

/** Command which specifies that the DS340 should be triggered. 
************************************************************************/
#define DS340_TRG		0x008

/*@}*/

/** @name Toggle bits.
    Toggle bits of the DS340 driver.

    @memo Toggle bits
    @author MP, January 98
    @see DS340 driver
************************************************************************/

/*@{*/

/** Flag which specifies an inverted output signal. 
    0 = norm, 1 = invert */
#define DS340_INVT		0x001

/** Flag which specifies that the sync output should be on. 
    0 = sync out off, 1 = on */
#define DS340_SYNC		0x002

/** Flag which specifiesthat the fsk should be on. 
    0 = fsk off, 1 = on */
#define DS340_FSEN 		0x004

/** Flag which specifies the output impedance to be high.
    0 = 50 ohm, 1 = hi-Z */
#define DS340_TERM		0x008

/** Flag which specifies the sweep waveform.
    0 = ramp, 1 = triangle */ 
#define DS340_SDIR		0x010 

/** Flag which specifies the sweep type. 
    0 = lin, 1 = log */
#define DS340_STYP		0x020

/** Flag which specifies the sweep enable. 
    0 = off, 1 = on */
#define DS340_SWEN		0x040

/** Flag which specifies an the sweep trigger. 
    0 = single, 1 = internal */
#define DS340_STRS		0x080

/** Flag which specifies the unit of the output signal. 
    0 = p-p, 1 = rms */
#define DS340_ATYP		0x100

/** Flag which specifies the trigger source.
    0 = single, 1 = continous */
#define DS340_TSRC		0x200

/*@}*/

/** @name Types and classes.
    Types and classes of the test organizer.

    @memo Types and classes
    @author DS, September 98
    @see Test organizer
************************************************************************/

/*@{*/

/** Supported functions of the DS340.
************************************************************************/
   enum DS340_FuncType {
   /** sine */
   ds340_sin = 0,
   /** square wave */
   ds340_square = 1,
   /** triangle */
   ds340_triangle = 2,
   /** ramp */
   ds340_ramp = 3,
   /** noise */
   ds340_noise = 4,
   /** arbitrary */
   ds340_arb = 5,
   /** none */
   ds340_none = 6
   };
   typedef enum DS340_FuncType DS340_FuncType;


/** Structure which represents a configuration block for a DS340
    arbitrary waveform generator.
************************************************************************/
   struct DS340_ConfigBlock {
      /** Status flag. */
      int 		status;
      /** Toggle bits. */
      int 		toggles;
      /** Standard event status byte. */
      int 		es_byte;
      /** Serial poll byte. */
      int 		sps_byte;
      /** DDS status bit. */
      int 		dds_byte;
      /** DS340 identification string. */
      char 		name[64];
      /** Function type. */
      DS340_FuncType 	func;
      /** Peak amplitude. */
      float 		ampl;
      /** Frequency. */
      float 		freq;
      /** Offset. */
      float 		offs;
      /** Sweep start frequency. */
      float 		stfr;
      /** Sweep stop frequency. */
      float 		spfr;
      /** Sweep rate. */
      float 		srat;
      /** Sampling frequency of arbitrary waveform. */
      float 		fsmp;
};
   typedef struct DS340_ConfigBlock DS340_ConfigBlock;

/*@}*/

/** @name Functions.
    Functions of the DS340 driver.

    @memo Functions
    @author MP, January 98
    @see DS340 driver
************************************************************************/

/*@{*/

/** Open a connection to a DS340 through a serial port.

    @param ID identification number of the DS340
    @param devname device name of serial port
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int connectSerialDS340 (int ID, const char* devname);

/** Open a connection to a DS340 through a cobox. A cobox is 
    a ethernet-to-RS232 converter.

    @param ID identification number of the DS340
    @param netaddr network name/address of the cobox
    @param port serial port at the cobox (1 or 2)
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int connectCoboxDS340 (int ID, const char* netaddr, int port);

/** Closes output descriptors and zeros local memory.
    An ID of -1 initializes the whole bank of DS340's.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int resetDS340 (int ID);

/** Returns true if DS340 is alive.

    @param ID identification number of the DS340
    @return true (1) if alive, false (0) if unavailable
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int isDS340Alive (int ID);

/** Pings a DS340. Returns 0 if a connection could be established.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int pingDS340 (int ID);

/** Set a DS340 configuration block. This only sets the internal
    data structure. To send the new configuration to a DS340 one
    of the upload commands has to be called as well.

    @param ID identification number of the DS340
    @param conf configuration block
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int setDS340 (int ID, const DS340_ConfigBlock* conf);

/** Get a DS340 configuration block. This function gets the 
    configuration from the internal data structure. To get the
    present configuration of a DS340 an download command has to
    be called prior to this function.

    @param ID identification number of the DS340
    @param conf configuration block
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int getDS340 (int ID, DS340_ConfigBlock* conf);

/** Read device wave parameters into internal data structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int downloadDS340Wave (int ID);

/** Read device sweep parameters into internal data structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int downloadDS340Sweep (int ID);

/** Read device toggle switches, status, etc. into internal data 
    structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int downloadDS340Status (int ID);

/** Load entire configuration block from device into the internal data
    structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int downloadDS340Block (int ID);

/** Send device wave parameters to DS340 from the internal data 
    structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int uploadDS340Wave (int ID);

/** Send device sweep parameters to DS340 from the internal data 
    structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int uploadDS340Sweep (int ID);

/** Send device toggle switches, etc. to DS340 from the internal data 
    structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int uploadDS340Status (int ID);

/** Send device entire configuration block to DS340 from the internal 
    data structure.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int uploadDS340Block (int ID);

/** Upload an arbitrary waveform to a DS340. The data is automatically
    scaled to fit the available ADC range. 

    @param ID identification number of the DS340
    @param data arbitrary waveform
    @param number of points (16300 maximum)
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int sendWaveDS340 (int ID, float data[], int len);

/** Send a reset signal to a DS340. An ID of -1 will send a signal to
    all active DS340's.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int sendResetDS340 (int ID);

/** Send a clear signal to a DS340. An ID of -1 will send a signal to
    all active DS340's.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int sendClearDS340 (int ID);

/** Send a trigger signal to a DS340. An ID of -1 will send a signal to
    all active DS340's.

    @param ID identification number of the DS340
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int sendTriggerDS340 (int ID);

/** ASCII print of configuration block.

    @param ID identification number of the DS340
    @param s buffer to recieve ASCII string
    @param max maximum length of buffer
    @return 0 if successful, <0 if failed
    @see DS340 driver
    @author MP, January 98
************************************************************************/
   int showDS340Block (int ID, char* s, int max);

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _GDS_DS340_H */
