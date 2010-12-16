/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awgfunc							*/
/*                                                         		*/
/* Module Description: 	Functions used by the arbitrary waveform 	*/
/*			generator					*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 18Jul98  D. Sigg    	First release		   		*/
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

#ifndef _GDS_AWGFUNC_H
#define _GDS_AWGFUNC_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#include "dtt/gdsrand.h"
#include "dtt/awgtype.h"


/** @name Waveform Functions
    This API provides functions for generating arbitrary waveforms.

    @memo Signals for for Arbitrary Waveform Generator
    @author Written July 1998 by Daniel Sigg
    @see Arbitrary Waveform Generator
    @version 0.1
************************************************************************/

/*@{*/

/** Normalize the phase. The phase is normalized to be greater or equal
    0 and to be less than 2 \pi.

    @param phase
    @return normalized phase
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   double normPhase (double phi);

/** Calculate the start phase. The start phase is calculated with a minimum
    error from numeric roundoff.

    @param phase
    @return phase from time*freq
    @see Arbitrary Waveform Generator
    @author JZ, October 05
************************************************************************/
   double startPhase (double f, double t, double dphi);

/** Calculates the inverse of z(w) = w * exp (w). This function uses
    Newton's method to find the root. Negative function arguments
    are invalid and return zero.

    @param z
    @return product logarithm
    @author DS, July 98
************************************************************************/
   double productLog (double z);

/** Periodic waveform signal. Depending on the wave type it returns a 
    sine wave, a square wave, a ramp signal, a triangular signal or a 
    constant level. The phase has to be greater or equal than 0 and 
    lower than 2 \pi.

    @param wtype type of waveform
    @param A amplitude
    @param ofs offset
    @param phi phase
    @return periodic signal value
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   double awgPeriodicSignal (AWG_WaveType wtype, double A, double ofs, 
                     double phi);

/** Noise waveform signal. Depending on the wave type it returns 
    unifromly or nomally distributed random numbers. 

    @param wtype type of waveform
    @param A amplitude
    @param ofs offset
    @param f1 lower frequency
    @param f2 upper frequency
    @param rbp random number paramters
    @return noise signal value
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   double awgNoiseSignal (AWG_WaveType wtype, double A, double ofs, 
                     randBlock* rbp);

/** Phase-in function. Depending on the ramp type the phase-in function
    ramps up either by a step, by a linear or a quadratic function.

    @param rtype type of ramp
    @param t time
    @param tPI phase-in duration
    @return phase-in function value
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   double awgPhaseIn (int rtype, double t, double tPI);

/** Phase-out function. Depending on the ramp type the phase-out function
    ramps down either by a step, by a linear or a quadratic function.

    @param rtype type of ramp
    @param t time
    @param tPO phase-out duration
    @param c amplitude ratio
    @return phase-in function value
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   double awgPhaseOut (int rtype, double t, double tPO, double c);

/** Sweep-out function. Depending on the ramp type the sweep-out 
    function sweeps through frequency either by a step, by a linear or 
    a quadratic function.

    @param rtype type of ramp
    @param t time
    @param tPO phase-out duration
    @param df frequency difference of sweep
    @param dphi phase difference of sweep
    @return phase-in function value
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   double awgSweepOut (int rtype, double t, double tPO, 
                     double f, double df, double dphi);

/** Define a frequency sweep. This is a convinience function to obtain
    the arbitrary waveform componets which correspond to a frequency
    sweep of a periodic waveform. The sweep is specified by its
    start frequency, its start amplitude, its duration, its stop 
    frequency, its stop amplitude and flag. The flag is used to 
    determine if the sweep is linear or logarithimic, whether the 
    sweep happens only once or repeats itself infinitely, and whether
    it sweeps up and down, or just restarts at its initial value.

    The default values for a sweep are: linear, sweep in one direction
    and restart, and repeat infinitely. Other options can be specified
    by a combination of 
    \begin{verbatim}
    AWG_SWEEP_LOG	logarithmic sweep
    AWG_SWEEP_CYCLE	sweep in both directions
    AWG_SWEEP_ONCE	sweep only once
    \end{verbatim}

    This function returns 2 AWG components if the sweep is 
    bi-directional and one otherwise. Ampitude, frequency and time
    values have to be positive.

    @param t start time
    @param d duration
    @param f1 start frequency
    @param f2 stop frequency
    @param a1 start amplitude
    @param a2 stop amplitude
    @param flag sweep parameters
    @param comp AWG components for describing the sweep (result)
    @param cnum number of components in result (result)
    @return 0 if successful, <0 otherwise
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgSweepComponents (tainsec_t t, tainsec_t d, double f1, double f2,
                     double a1, double a2, long flag, 
                     AWG_Component* comp, int* cnum);

/** Define a periodic wave. This is a convenience function to obtain
    the arbitrary waveform componet which describes a periodic
    waveform. It can be one of the following: awgSin, awgSquare,
    awgRamp or awgTriangle. The other parameters are frequency (Hz), 
    amplitude, phase (rad) and offset. The phase is relative to last GPS 
    one day boundary (no leap second corrections). If successful, this
    function returns zero and one AWG component.

    The function awgPeriodicComponentEx takes the start time as an
    additional argument.

    @param wtype wave type
    @param f frequency
    @param a  amplitude
    @param phi phase
    @param ofs offset
    @param comp AWG component for describing the waveform (result)
    @return 0 if successful, <0 otherwise
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgPeriodicComponent (AWG_WaveType wtype, double f, double A, 
                     double phi, double ofs, AWG_Component* comp);
   int awgPeriodicComponentEx (AWG_WaveType wtype, tainsec_t t0,
                     double f, double A, double phi, double ofs, 
                     AWG_Component* comp);

/** Define a square wave. This is a convenience function to obtain
    the square wave with unequal hi-lo ratio. The parameters are 
    frequency (Hz), amplitude, phase (rad), offset and ratio of the 
    high state period to the full period. The phase is relative to last 
    GPS one day boundary (no leap second corrections). If successful, 
    this function returns zero and two(!) AWG components.

    The function awgSquareWaveComponentEx takes the start time as an
    additional argument.

    @param f frequency
    @param a  amplitude
    @param phi phase
    @param ofs offset
    @param ratio ratio between high and cycle period
    @param comp AWG component for describing the waveform (result)
    @return 0 if successful, <0 otherwise
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgSquareWaveComponent (double f, double A, double phi, 
                     double ofs, double ratio, AWG_Component* comp);
   int awgSquareWaveComponentEx (tainsec_t t0,
                     double f, double A, double phi, 
                     double ofs, double ratio, AWG_Component* comp);

/** Define a constant offset. This is a convenience function to obtain
    the arbitrary waveform componet which describes a constant offset. 
    If successful, this function returns zero and one AWG component.

    The function awgConstantComponentEx takes the start time as an
    additional argument.

    @param ofs offset value
    @param comp AWG component for describing the waveform (result)
    @return 0 if successful, <0 otherwise
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgConstantComponent (double ofs, AWG_Component* comp);
   int awgConstantComponentEx (tainsec_t t0, double ofs, 
                     AWG_Component* comp);

/** Define a non-periodic wave. This is a convenience function to obtain
    the arbitrary waveform componet which describes a band-limited noise
    waveform. It can be one of the following: awgNoiseN or awgNoiseU. 
    The other parameters are lower frequency (Hz), upper frequency (Hz),
    amplitude and offset. If a zero or negative number is supplied for
    one of the frequency values, the band limit is disabled at the
    corresponding boundary. If successful, this function returns zero 
    and one AWG component.

    The function awgNoiseComponentEx takes the start time as an
    additional argument.

    @param wtype wave type
    @param f1 lower frequency limit
    @param f2 upper frequency limit
    @param a  amplitude
    @param ofs offset
    @param comp AWG component for describing the waveform (result)
    @return 0 if successful, <0 otherwise
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgNoiseComponent (AWG_WaveType wtype, double f1, double f2, 
                     double A, double ofs, AWG_Component* comp);
   int awgNoiseComponentEx (AWG_WaveType wtype, tainsec_t t0, 
                     double f1, double f2, 
                     double A, double ofs, AWG_Component* comp);

/** Define a stream. This is a convenience function to obtain
    the arbitrary waveform componet which describes a stream. 
    If successful, this function returns zero and one AWG component.

    The function awgStreamComponentEx takes the start time as an
    additional argument.

    @param scaling Waveform scaling
    @param comp AWG component for describing the waveform (result)
    @return 0 if successful, <0 otherwise
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgStreamComponent (double scaling, AWG_Component* comp);
   int awgStreamComponentEx (tainsec_t t0, double scaling, 
                     AWG_Component* comp);

/** Computes a second order section of an IIR filter.

    @param x Data array
    @param len Length of data array
    @param b1 List of filter coefficients (b1, b2, a1, a2)
    @param b2 List of filter coefficients (b1, b2, a1, a2)
    @param a1 List of filter coefficients (b1, b2, a1, a2)
    @param a2 List of filter coefficients (b1, b2, a1, a2)
    @param h1 History value (h1, h2)
    @param h2 History value (h1, h2)
    @return void
    @see Arbitrary Waveform Generator, IIRFilter
    @author DS, July 98
************************************************************************/
   void awgSOS (float* x, int len, double b1, double b2,
               double a1, double a2, double* h1, double* h2);

/** Validation function. This function returns a non-zero value if the
    specified arbitrary waveform component contains valid values.

    @param comp AWG component
    @return 1 if valid, 0 if invalid
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgIsValidComponent (const AWG_Component* comp);


/** Searches through a list of AWG components. Returns the position of 
    the first component which start time is later than the one 
    provided by the search component. The algorithm assumes that the 
    component list is sorted. The returned position points beyond the
    last component, if the search component is the most recent one.

    @param complist AWG component list
    @param numComp length of list
    @param comp search component
    @return position of first component with a later start time
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   int awgBinarySearch (const AWG_Component complist[], int ncomp, 
                     const AWG_Component* comp);

/** Sorts a list of AWG components in ascending order of their start 
    time. The sorting algorithm is heap sort.

    @param comp AWG component list
    @param numComp length of list
    @return void
    @see Arbitrary Waveform Generator
    @author DS, July 98
************************************************************************/
   void awgSortComponents (AWG_Component comp[], int numComp);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_AWGFUNC_H */
