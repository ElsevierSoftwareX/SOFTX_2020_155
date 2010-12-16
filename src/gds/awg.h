/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awg							*/
/*                                                         		*/
/* Module Description:  Arbitrary Waveform Generator			*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   4/16/98 MRP        						*/
/*                                                         		*/
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	Mark Pratt    617-253-6410 617-253-7014 mrp@mit.edu		*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Unix,	vxWorks/Baja47				*/
/*	Compiler Used: Sun cc, gcc					*/
/*	Runtime environment: Solaris, vxWorks				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			OK			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/* v.0.1 only writes to streams					 	*/
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

#ifndef _GDS_AWG_H
#define _GDS_AWG_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include <stdio.h>
#include <stdlib.h>
#include "dtt/gdsutil.h"
#include "dtt/gdsrand.h"
#include "dtt/gdstask.h"
#include "dtt/awgtype.h"


/**@name Arbitrary Waveform Generator
   This module controls a set of software arbitrary waveform 
   generators.

   @memo Control and set waveforms.
   @author Written Apr. 1998 by Mark Pratt
   @version 0.1
*********************************************************************/

/*@{*/

/** @name Individual waveform control functions
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/

/*@{*/    

/** Installs and initializes a set of arbitrary waveform generators.
    This function has be called once during startup before any of
    the other routines. Successive calls are ignored and return 0.

    @return 0 if successful, <0 otherwise
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int initAWG (void);

/** Checks if AWG is running.

    @return true if running, false if not
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int checkAWG (void);

/** Restarts suspended AWG tasks.

    @return 0 if successful, <0 otherwise
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int restartAWG (void);

/** Gets the index of an unused arbitrary waveform generator.
    The arbitrary waveform generator is marked as in use upon
    successful return. For channels which are not of type awgMem
    the first argument specifies the output delay in nsec.

    @param chntype type of channel (test point, file, memory)
    @param id channel ID number
    @param arg1 first additional argument
    @param arg2 second additional argument
    @return index number of AWG, if successful; <0 otherwise
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int getIndexAWG (AWG_OutputType chntype, int id,
                   int arg1, int arg2);

/** 
    Locks or unlocks AWG waveforms.
    @param 1 to lock all current waveforms; 0 to unlock all
    @return void
*********************************************************************/
   void awgLock (unsigned int lock);

/** Releases an arbitrary waveform generator. The arbitrary waveform
    generator is marked as unused after it has been reseted. If an 
    index of -1 is specified the whole bank is released.

    @param ID index number of AWG
    @return 0 if successful; <0 otherwise
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int releaseIndexAWG (int ID);

/** Shuts off and deconfigures an AWG.
    If configured, output is flushed before reset.

    @param ID index number of AWG
    @return 0 if successful, <0 otherwise
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int resetAWG (int ID);

/** Configs AWG from parameter file.
    If filename is not found the current directory, this function will
    look for it in ARCHIVE/param/awg/SITE_PREFIX IFO_PREFIX/.
    If a NULL pointer is provided, AWG_CONFIG_FILE is used as
    filename.

    @param ID index number of AWG
    @param fp pointer to parameter file
    @return GDS_ERROR
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int configAWG (int ID, const char *filename);

/** Display current settings of configured AWG.
    
    @param ID index number of AWG
    @param s output string
    @param max length of output string
    @return GDS_ERROR
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int showAWG (int ID, char* s, int max);

/** Calculates waveforms and sends it to the output.
    Conditional upon status bits AWG_CONFIG and AWG_ENABLE. This 
    routine should not be called directly.

    @param ID index number of AWG
    @return GDS_ERROR
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int processAWG (int ID, taisec_t time, int epoch); 

/** Disables an arbitrary wavefrom generator.
    Turns off status bit AWG_ENABLE.

    @param ID index number of AWG
    @return 0 if successful, <0 if not
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int disableAWG (int ID);

/** Enables an arbitrary waveform generator.
    Turns on status bit AWG_ENABLE if AWG is configured and ready.

    @param ID index number of AWG
    @return 0 if successful, <0 if not
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int enableAWG (int ID);

/** Adds waveforms to an arbitrary waveform generator. Because of
    efficiency reasons the provided list of components might be
    altered upon return. Components which are restarted 
    automatically have their start time adjusted to be as close
    as possible to the current time. Components are sorted in 
    ascending order of their start time and invalid components
    obtain an awgNone type and appear at the end of the list.

    @param ID index number of AWG
    @param comp list of waveform components to be added
    @param numComp number of components in the list
    @return number of added waveforms if successful, <0 if not
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int addWaveformAWG (int ID, AWG_Component* comp, int numComp);

/** Downloads a waveform to an arbitrary waveform generator.
    This function will not automatically switch on the output.
    This requires that a component with awgArb is also added to the
    slot. The specified frequency will be interpreted as the sampling
    frequency.

    @param ID index number of AWG
    @param y data points
    @param len length of data array
    @return 0 if successful, <0 if not
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int setWaveformAWG (int ID, float y[], int len);

/** Send a waveform to an arbitrary waveform generator. This routine
    is used by the stream interface to transfer data to the front-end.
    The return codes are as following
    \begin{verbatim}
     0    Data accepted
     1    Data accepted, but there is currently not enough buffer space
          to accept another block of data with the same length
     2    The data block duplicates a time that was sent previously
     3    The data block is not contiguous with the previous block
    -1    This awg slot is not currently set up for stream data
    -2    Invalid data block or size
    -3    The timestamp of the data block is already past
    -4    The timestamp of the data block is too far in the future
    -5    Connection failed
    \end{verbatim}

    @param ID index number of AWG
    @param time Start time of waveform in GPS sec
    @param epoch Epoch of waveform
    @param y data points
    @param len length of data array
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int sendWaveformAWG (int ID, taisec_t time, int epoch,
                     float y[], int len);

/** Stops a waveform in an arbitrary waveform generator. A waveform
    can be stopped in several different ways:

    \begin{verbatim}
    0 reset
    1 freeze
    2 phase-out
    \end{verbatim}

    Reset will immediately stop any waveforms and reset the slot.
    Freeze will continue to output the waveform in its current state
    foreever. Phase-out takes an additional argument which specifes 
    a ramp down time.

    There are several restrictions: (1) freeze will only work with
    swept sine-like excitation signals, and (2) phase-out will only
    ramp down the signal, if it is of inifinite duration.

    @param ID index number of AWG
    @param y data points
    @param len length of data array
    @return 0 if successful, <0 if not
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int stopWaveformAWG (int ID, int terminate, tainsec_t arg);

/** Returns the waveforms from an arbitary waveform generator.

    @param ID index number of AWG
    @param comp pointer to a list of awg components to store the result
    @param maxComp size of the result components array
    @return 0 if successful, <0 if not
    @author DS, June 98
*********************************************************************/
   int queryWaveformAWG (int ID, AWG_Component* comp, int maxComp);

/** Set the gain of an arbitrary waveform generator.

    @param ID index number of AWG
    @param gain Overall gain
    @param time Rampt time
    @return 0 if successful, <0 if not
    @author DS, Dec. 2004
    @see Arbitrary Waveform Generator
*********************************************************************/
   int setGainAWG (int ID, double gain, tainsec_t time);

/** Set an IIR filter from the specified second order sections.

    @param ID index number of AWG
    @param y filter coefficients
    @param len length of coefficient array
    @return 0 if successful, <0 if not
    @author DS, Apr. 2004
    @see Arbitrary Waveform Generator
*********************************************************************/
   int setFilterAWG (int ID, double y[], int len);

/** Append component to AWG.
    Check for valid component and allowed configuration.

    @param ID index number of AWG
    @param C waveform component
    @return GDS_ERROR
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int addCompAWG (int ID, char*);

/** Check AWG configuration.
    If configuration is valid, GDS_ERR_NONE is returned and
    status bit AWG_CONFIGis set.

    @param ID index number of AWG
    @return GDS_ERROR
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int checkConfigAWG (int ID);

/*@}*/

/** @name AWG bank control
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
/*@{*/

/** Resets entire AWG bank.

    @return void
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and resetAWG
*********************************************************************/
   void resetAllAWG (void);

/** Configs entire AWG bank.
    If filename is not found the current directory, this function will
    look for it in ARCHIVE/param/awg/SITE_PREFIX IFO_PREFIX/.  If a
    NULL pointer is provided, AWG_CONFIG_FILE is used as filename.

    @param base filename.
    @return void
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and configAWG
*********************************************************************/
   void configAllAWG (char* filename);

/** Display configuration of entire AWG bank.
    A null pointer will result in output to stdout.

    @params FILE* for output
    @return void
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and showAWG
*********************************************************************/
   void showAllAWG (FILE*);

/** Display configuration of entire AWG bank.
    Writes to a string with maximum length specified.

    @params s output string
    @params max maximum length of output string
    @return output string if successful, NULL if failed
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and showAWG
*********************************************************************/
   char* showAllAWGs (char* s, int max);

/** Processes next page for entire AWG bank.
    
    @return void
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and processAWG
*********************************************************************/
   void processAllAWG (taisec_t time, int epoch);

/** Disable entire AWG bank.
    
    @return void
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and disableAWG
*********************************************************************/
   void disableAllAWG (void);

/** Set enable on entire AWG bank.
    Enable is only set if AWG is configured and ready.

    @return void
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and enableAWG
*********************************************************************/
   void enableAllAWG (void);

/** Gets the runtime statics of the arbitrary waveform generator.
    Passing a NULL pointer resets all statistical parameters. 
    Gathering runtime statitsics is only enable if the AWG_STATISTICS
    is defined during compilation.

    @return 0 if successful, <0 otherwise
    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   int getStatisticsAWG (awgStat_t* stat);

/*@}*/


/*@}*/ /* Arbitrary Waveform Generator API */

#ifdef __cplusplus
}
#endif

#endif /*_GDS_AWG_H */
