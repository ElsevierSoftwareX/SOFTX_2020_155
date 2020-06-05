/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awgapi							*/
/*                                                         		*/
/* Module Description: 	API to the arbitrary waveform generator		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 30June98 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: awgapi.html						*/
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

#ifndef _GDS_AWGAPI_H
#define _GDS_AWGAPI_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#ifndef _TAINSEC_T
#define _TAINSEC_T
#include <inttypes.h>
   typedef int64_t tainsec_t;
   typedef unsigned long taisec_t;
#endif
#include "awgtype.h"


/** @name Arbitrary Waveform Generator Remote API
    This API provides routines to remotely access the arbitrary 
    waveform generators.

    @memo Control interface for arbitrary waveform generators
    @author Written June 1998 by Daniel Sigg
    @see Arbitrary Waveform Generator
    @version 0.1
************************************************************************/

/*@{*/

#if 0
/** Compiler flag for a compiling a stand-alone dynamic link library.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _AWG_LIB

/** Compiler flag for a enabling dynamic configuration. When enabled
    the host address and interface information of arbitrary waveform 
    generators is queried from the network rather than read in through 
    a file.

    @author DS, June 98
    @see Test point API
************************************************************************/
#undef _CONFIG_DYNAMIC
#endif

/** Installs an awg client interface. This function might be called 
    prior of using the awg interface. If not, the first call to any of 
    the functions in this API will call it. There is no penalty of not
    calling awg_client explicitly. The function returns the number of
    AWGs which can be reached through this interface.

    @param void
    @return number of AWGs if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int awg_client (void);

/** Terminates an awg client interface.

    @param void
    @return void
    @author DS, June 98
************************************************************************/
   void awg_cleanup (void);

#ifdef _AWG_LIB
/** Sets the excitation engine host address. This function is only 
    available when compiling with the _AWG_LIB flag. It disables
    the default parameter file and set the host address and rpc program
    and version number directly. If a program number of zero is
    specified, the default will be used. If a program version of 
    zero is specified, the default will be used. This function must
    be called before any other function (including awg_client).

    @param ifo interferometer id (4K - 0, 2K - 1)
    @param awg excitation engine id
    @param hostname host name of excitation engine
    @param prognum rpc program number
    @param progver rpc program version
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int awgSetHostAddress (int ifo, int awg, const char* hostname, 
                     unsigned long prognum, unsigned long progver);

/** Sets the DS340 host address. This function is only 
    available when compiling with the _AWG_LIB flag. It will disable
    the default parameter file and set the host address and port 
    number of the cobox directly. This function must
    be called before any other function (including awg_client).

    @param ds340 ds340 id
    @param hostname host name of cobox controlling the DS340
    @param port port number of the cobox
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int ds340SetHostAddress (int ds340, const char* hostname, int port);
#endif

/** Relate a channel name with a free slot in an excitation engine.

    @param name channel name
    @return slot number if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgSetChannel (const char* name);


/** Remove a channel name from an excitation engine.

    @param slot slot number
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgRemoveChannel (int slot);


/** Ask for all valid channel names. The function needs a buffer
    which is long enough to hold all excitation channel names
    (space separated list). The function will return the number
    of characters written into the names buffer. If the names buffer
    is NULL, the function will simply return the necessary length 
    for the result (not including the terminating 0).
 

    @param names list of channel names (return)
    @param len length of names buffer
    @param info 0 - names only, 1 - names and rates
    @return number of characters if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgGetChannelNames (char* names, int len, int info);


/** Add a waveform to an arbitrary waveform generator slot.

    @param slot slot number
    @param comp list of waveform components to be added
    @param numComp number of components in the list
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgAddWaveform (int slot, AWG_Component* comp, int numComp);


/** Download an arbitrary waveform to an arbitrary waveform generator 
    slot. This function will not automatically switch on the output.
    This requires that a component with awgArb is also added to the
    slot. The specified frequency will be interpreted as the sampling
    frequency.

    @param slot slot number
    @param y data points
    @param len length of data array
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgSetWaveform (int slot, float y[], int len);


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
    \end{verbatim}

    @param slot slot number
    @param time Start time of waveform in GPS sec
    @param epoch Epoch of waveform
    @param y data points
    @param len length of data array
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgSendWaveform (int slot, taisec_t time, int epoch,
                     float y[], int len);


/** Stops a waveform in an arbitrary waveform generator slot. A waveform
    can be stopped in several different ways:

    \begin{verbatim}
    0 reset
    1 freeze
    2 phase-out
    \end{verbatim}

    Reset will immediately stop any waveforms and reset the slot.
    Freeze will continue to output the waveform in its current state
    forever. Phase-out takes an additional argument which specifes 
    a ramp down time.

    There are several restrictions: (1) freeze will only work with
    swept sine-like excitation signals, and (2) phase-out will only
    ramp down the signal, if it is of inifinite duration.

    @param slot slot number
    @param terminate termination flag
    @param time ramp down time
    @param slot slot number
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgStopWaveform (int slot, int terminate, tainsec_t time);


/** Set the overall gain of an arbitrary waveform generator slot.
    The time argument is used to specify the ramp time. The gain is
    ramped linearly from the old value to the new one. A negative 
    ramp time indicates to use the previously specified one.

    @param slot slot number
    @param gain overall gain of waveforms
    @param time ramp time
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgSetGain (int slot, double gain, tainsec_t time);


/** Set an IIR filter in the arbitrary waveform generator. The specified
    filter coefficients must be second order sections.

    @param slot slot number
    @param y filter data array (gain, b1, b2, a1, a2, etc.)
    @param len length of data array
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgSetFilter (int slot, double y[], int len);


/** Clear all waveforms from an arbitrary waveform generator slot.

    @param slot slot number
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgClearWaveforms (int slot);


/** Returns the waveforms of an arbitrary waveform generator.

    @param slot slot number
    @param comp pointer to a list of awg components to store the result
    @param maxComp size of the result components array
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgQueryWaveforms (int slot, AWG_Component* comp, int maxComp);


/** Reset all slots of an arbitrary waveform generator to zero. If the 
    'id' is -1 all arbitrary waveform generators are reset. Use the 
    macro AWG_ID to obtain the id for a specific AWG or for all AWG's
    of a interferometer.

    @param id identification of an arbitrary waveform generator
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgReset (int id);


/** Obtains run-time statistics of an arbitrary waveform generator. Use 
    the macro AWG_ID to obtain the id for a specific AWG of an 
    interferometer.

    @param id identification of an arbitrary waveform generator
    @param stat statistics result
    @return 0 if successful, <0 if not
    @author DS, June 98
************************************************************************/
   int awgStatistics (int id, awgStat_t* stat);


#if 0
/** Returns the identification number of an arbitrary waveform generator.
    The interferometer number has to be either 1 (4K), 2 (2K) or -1 
    (both). If an interferometer has more than one awg, they are numbered
    starting with 0. A -1 stands for all awg's belonging to a specific 
    interferometer.

    @param ifo interferometer number; 1 or 2
    @param awgnum awg number; 0 and up
    @return identification of an arbitrary waveform generator
    @author DS, June 98
************************************************************************/
#define AWG_ID(ifo,awgnum)
#endif

/** Returns a string describing the current AWG configuration. The
    caller is responsible to free the returned string!

    @param id identification of an arbitrary waveform generator
    @return configuration string if successful, NULL if not
    @author DS, June 98
************************************************************************/
   char* awgShow (int id);

/** ASCII command for an arbitrary waveform generator. This function 
    interprets the specified waveform string and returns the 
    corresponding awg components and awg points (arbitary waveforms
    only). On error the function returns a negative value and an
    error message if a string pointer is specified. The caller is 
    responsible to free the memory returned by points or errmsg.
    (In no case both are returned!) For arbitrary waveforms the caller 
    should also specify if the waveforms are meant for a DS340.

    @param cmd waveform description
    @param comp awg components (return, maximum 2)
    @param cnum number of awg components (return)
    @param errmsg error message (return)
    @param points waveform array (return)
    @param num number of points in the waveform array
    @param isDS340 true if stand-alone AWG
    @return 0 if successful, <0 if not
    @see T990013 for allowed waveforms
    @author DS, June 98
************************************************************************/
   int awgWaveformCmd (const char* cmd, AWG_Component comp[], int* cnum,
                     char** errmsg, float** points, int* num, 
                     int isDS340);

/** ASCII interface to the arbitrary waveform generators.

    The function returns a string which was allocated with malloc. The
    caller is reponsible to free this string if no longer needed.

    @param cmd command string
    @return reply string
    @author DS, June 98
************************************************************************/
   char* awgCommand (const char* cmd);

/** ASCII interface to the arbitrary waveform generators.

    The function calls awgCommand and writes the return to stdout.

    @param cmd command string
    @return 0 if successful, <0 on error
    @author DS, June 98
************************************************************************/
   int awgcmdline (const char* cmd);


#define _AWG_IFO_OFS		1000
#define _AWG_NUM_OFS		100

#define AWG_ID(ifo,awgnum) \
	(((ifo) == -1 ? -1 : (_AWG_IFO_OFS * (ifo + 1) + \
	 ((awgnum) == -1 ? (-2 * _AWG_IFO_OFS * (ifo + 1)) : \
	                   (_AWG_NUM_OFS * (awgnum))))))



/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_AWGAPI_H */
