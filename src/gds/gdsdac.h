/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsdac							*/
/*                                                         		*/
/* Module Description: gds driver for ICS115 DAC board			*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   12Jul98 DS        						*/
/*                                                         		*/
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References:							*/
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

#ifndef _GDS_DAC_H
#define _GDS_DAC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Header File List: */
#include <semaphore.h>


/** @name DAC API
    This routines are used by the arbitrary waveform generator to 
    initialize the ICS115 board, to synchronize the board with the
    1Hz GPS clock and to copy data at every heartbeat.

    @memo Interface to the ICS115 digital-to-analog converter
    @author Written June 1999 by Christine Patton
    @version 0.1
    @see Arbitrary Waveform Generator
************************************************************************/

/*@{*/

/** Initializes the DAC. This routine initializes the ICS115 DAC board
    for use by the arbitrary waveform generator.

    @param ID board ID
    @param semISR semaphore which triggers copying of new data
    @return 0 if successful, <0 otherwise
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacInit (short ID, sem_t* semISR);

/** Stops the DAC. All outputs are disabled.

    @param ID board ID
    @param semISR semaphore which triggers copying of new data
    @return 0 if successful, <0 otherwise
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacStop (short ID);


/** Sets up the DAC and arms the 1Hz GPS clock. This routine is called 
    at least once after dacInit and whenever the arbitrary waveform
    generator has lost synchronization with the DAC.

    @param ID board ID
    @return 0 if successful, <0 otherwise
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacReinit (short ID);


/** Loads the first two buffers into the DAC and enables the outputs 
    and interrupts. If no timing board is used, the routine checks the 
    GPS time to synchronize the the start of the conversion with the 
    1PPS.

    @param ID board ID
    @param buf0 data buffer for first epoch
    @param buf1 data buffer for second epoch
    @param len length of data buffers
    @return 0 if successful, <0 otherwise
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacRestart (short ID, short* buf0, short* buf1, int len);


/** Copies new data to the DAC. This routine is called whenever the 
    'semISR' semaphore is given and new data is available. If for
    some reason the arbitrary waveform generator falls behind, so
    that no new data is available, the DAC is automatically 
    restarted at the next 1 sec boundary.

    @param ID board ID
    @param buf data buffer for an epoch
    @param len length of data buffer
    @return 0 if successful, <0 otherwise
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacCopyData (short ID, short* buf, int len);

/** Converts data arrays stored in channel format into DAC buffer
    format. The DAC buffer must be of length (number of channels) *
    (number of data points). The buffer should be initialized to zero 
    before the first call. The data array represents a single channel.
    Since the DAC requires the data sample-by-sample rather than 
    channel-by-channel, the typical conversion operration is 
    performed as follows:
    \begin{verbatim}
    for (k = 0; k < len; k++) {
       val = floor (dac_conversion_factor * data[k]);
       if (fabs(val) >= dac_max) {
          buf[k * number_of_channels + chnnum] = 
                            (val > 0) ? dac_max : -dac_max;
       }
       else {
          buf[k * number_of_channels + chnnum] = (short) val;
       }
    }
    \end{verbatim}

    @param buf data buffer for an epoch
    @param data Channel data array
    @param chnnum DAC channel number
    @param len length of data buffer
    @return 0 if successful, <0 otherwise
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacConvertData (short* buf, float* data, int chnnum, int len);

/** Returns true, if a DAC timing error occured.

    @param ID board ID
    @return 1 if error, 0 if ok
    @author CP, June 99
    @see DAC API, Arbitrary Waveform Generator
************************************************************************/
   int dacError (short ID);



/*@}*/


#ifdef __cplusplus
}
#endif

#endif /*_GDS_DAC_H */
