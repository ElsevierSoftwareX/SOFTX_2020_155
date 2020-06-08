/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awgtype							*/
/*                                                         		*/
/* Module Description:  Type definition of the 				*/
/*			Arbitrary Waveform Generator			*/
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

#ifndef _GDS_AWGTYPE_H
#define _GDS_AWGTYPE_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */
#ifndef _GDS_AWGAPI_H
#include "dtt/gdsrand.h"
#include "tconv.h"
#include "dtt/gdstask.h"
#endif

/**@name Arbitrary Waveform Generator Type Definitions
   This module defines the constants and types used by the arbitrary 
   waveform generators. Waveform parameters are configured and 
   controlled through routines defined in "awg.h".
   
   Waveforms are specified by sums of components.
   Valid component configurations are.
   
   1) Any combination of awgSine, awgSquare, awgRamp, awgTrinagle,
   awgImpulse, awgConst, awgNoiseN, awgNoiseU, awgArb and
   awgStream components. Only one awgArb and one awgStream can be
   active for any given slot at any given time.
      
   2) All configurations begin with comp[0] and include no empty 
   components. Allowed component type are given in AWG_Wavetype. The
   internal representation of each AWG_Component is:
   
   awgSine:   par[] ={amp, freq, phase, offset}
   
   awgSquare:  par[] ={amp, freq, phase, offset}

   awgRamp:  par[] ={amp, freq, phase, offset}

   awgTriangle:  par[] ={amp, freq, phase, offset}

   awgImpulse:  par[] ={amp, freq, duration, delay}

   awgConst:  par[] ={'ignore', 'ignore', 'ignore', offset}
   
   awgNoiseN:   par[] ={amp, freq1, freq2, offset}
   
   awgNoiseU:   par[] ={amp, freq1, freq2, offset}

   awgArb:   par[] ={scaling, sampling freq, rate, trigger type}

   awgStream:   par[] ={scaling, 'ignore', 'ignore', 'ignore'}

   A trigger type can be one of the following:
   0 - continous, 1 - random, 2 - wait, 3 - single trigger.
   If the continous trigger is specified, the rate indicates the
   trigger rate. If a random trigger is specifed, the waveform will 
   be applied at random times with an average rate compatible with 
   the specifed one.

   3) Every waveform is synchronized relative to a GPS  clock.
   The available parameters for time sepcification are:

   start: start time in GPS nsec

   duration: waveform duration in GPS nsec

   restart: waveform gets periodically restarted

   4) Every waveform can have a phase-in and a phase-out
   period. The following ramp parameters are supported:

   ramptype: specifies the ramp type for amplitude phase-in,
   amplitude phase-out and frequency phase-out. The amplitude
   phase-in always starts a zero ramping up the final value
   of amplutide and offset. On the other hand the phase-out 
   values must be explicitly specified and can be non-zero.

   ramptime[0]: ramp up time at the beginning

   ramptime[1]: ramp down time at the end

   ramppar[]: phase-out values, follows the same convention as par[]

   @memo Software generated composite and swept waveforms.
   @author Written Apr. 1998 by Mark Pratt
   @version 0.1
    
*********************************************************************/

/*@{*/

/** @name Parameter file specification
   
   Frequency specifications must be positive, with the exception of
   sweeps which may optionally specify frequency by logarithm.

   Trailing numeric parameters may be omitted and will take on zero
   value as their default.

   A frequency sweep may be specified as the first component (comp0)
   in a waveform.  This will have the effect of altering the frequency
   of a second periodic component (comp1).  No additional components
   are allowed.  A sweep is specified in a parameter file as

   comp0 = sweep f_low f_hi num_f pages [options]
   
   comp0 = sweep 1e3 16e3 16 1 lin half pos

   - or -

   comp0 = sweep 0 3 40 5 log full neg

   The line first specifies 15 linearly spaced 1 page steps between 1k
   and 16k Hz, increasing from 1k to 16k.  The second, 40
   logarithmically spaced 5 page steps between 1Hz and 1kHz,
   decreasing from 1k to 1 then increasing back to 1k.  The qualifiers
   following the numeric parameters are optional and will default to
   the values of the first example.  In addition to the comp[0] block,
   sweep information is stored in scount, smax, tmax, the status flag.
   For log steps, all frequency parameters are stored as logs.

   awgNoiseX parameters freq1 and freq2 are band limits (not
   implemented).  A zero or default value for on or both will result
   in unfiltered noise.  The default distribution for noise is normal.
   Uniform noise be specified as a trailing modifier "uniform" or
   by declaring type NoiseU.

   @author MRP, Apr. 1998
   @see Arbitrary Waveform Generator
*********************************************************************/

/*@{*/

/** @name Single periodic wave.

    The following parameter file section configures AWG[1].  It
    specifies a wave with an amplitude of 1.5, an offset of -0.2, a
    frequency of 600 Hz and a phase offset of 90 deg.  At each
    heartbeat, 256 samples are written to channel or file "wave1.out".
    
   [AWG1]
  
   pagesize = 256 samples
   
   output = file wave1.out
  
   comp0 = Sine  1.5   600   90   -0.1

   @author MRP, Apr. 1998
   @see Arbitrary Waveform Generator
*********************************************************************/

/** @name Swept sine wave.
  
    The following parameter file section configures AWG[3].  It
    specifies a swept square wave of amplitude 2.0, offset 0 and no
    phase offset.  The frequency argument is not used.  The frequency
    sweep is logarithmic with 20 steps between 10**3 and 10**4 Hz.
    There are two pages written at each step.  The sweep is full
    cycle, meaning that frequencies step from 10**3 to 10**4 and then
    back down.  If this is a continuous wave, they then repeat.

    [AWG3]
  
    \# swept sine wave

    pagesize = 1024 samples
  
    output = file wave3.out

    comp0 = Sweep	3	4	20	2  	log full	

    comp1 = Square  	2.0 	10 	0.0  	0.5

   @author MRP, Apr. 1998
   @see Arbitrary Waveform Generator
*********************************************************************/

/** @name Noise wave.

   Fhe following specifies a uniformly distributed noise waveform with
   amplitude 3.2, offset 1 and seed value 78382.  Without the uniform
   declaration, the wave would be normally distributed with mean 1 and
   variance 3.2**2. Only one random seed is used for a composite wave
   and it is the last one declared in the parameter file.
   
   [AWG5]
   
   pagesize = 1024 
   
   output = file wave5.out
   
   comp0 = noise  3.2  	1	0	0 	78382	uniform

   or equivalently

   comp0 = unoise  3.2 	1	0	0 	78382
   
   @author MRP, Apr. 1998
   @see Arbitrary Waveform Generator
*********************************************************************/

/**@name Composite wave.

   The following specifies the sum of two square, two sine and one
   noise (normal) component.  

   [AWG8]
   
   pagesize = 1024 
   
   output = file wave8.out
   
   comp0 = squ		1 	3  	90 	100
   
   comp1 = sin   	1 	5  	45  	0
   
   comp2 = squ		0.5	9	120	0

   comp3 = sin		1	0.7	0	0
   
   comp4 = noi  	0.3  	0	0	0 	2316
   
   @author MRP, Apr. 1998
   @see Arbitrary Waveform Generator
*********************************************************************/

/*@}*/

/** @name Constants and flags
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/

/*@{*/

/** @name Configuration
    This group defines the configuration of the arbitrary waveform
    generator.
    @memo Status bit masks
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and AWG_ConfigBlock
*********************************************************************/

/*@{*/

/** Default configuration parameter filename.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_CONFIG_FILE			"config.par"

/** Number of AWGs in bank.
    The nominal LIGO value for this is 8.
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	MAX_NUM_AWG			9

/** Maximum number of components per waveform. This number has to 
    be large enough to hold all scheduled components. This number
    is generally much larger than the number of waveforms which
    can be send to the output simultaneously.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator, MAX_INVALID_COMPONENTS
*********************************************************************/
#define	MAX_NUM_AWG_COMPONENTS		5000

/** Maximum number of invalid components. When processing waveforms
    this specifies the maximum number of not yet ready components 
    which are processed, before the calculation stops. This avoids
    slowing down the waveform processing when a lot of components
    are scheduled ahead of time, but allows for some out of order
    components.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator, MAX_NUM_AWG_COMPONENTS
*********************************************************************/
#define MAX_INVALID_COMPONENTS		10

/** Maximum number of 1 sec stream buffers. Stream buffers are
    used by the arbitrary waveform generator to transfer waveform
    data between a UNIX workstation and the front-end. These buffers
    are filled by a client program ahead of time, so that the 
    waveform will be available when it is needed.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define MAX_STREAM_BUFFER		16

/** Maximum number of second order sections in IIR filter.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define MAX_AWG_SOS			20

/** Page length of an LSC test point channel.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_LSC_PAGELEN			1024

/** Page length of an ASC test point channel.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_ASC_PAGELEN			128

/*@}*/


/** @name DAC constants
    Specifies the digital-to-analog converter (DAC).

    @memo DAC constants
    @author MRP, Apr. 1998
    @see ICS115 manual
*********************************************************************/

/*@{*/

/** DAC output delay (in nsec). The DAC output delay is 25 samples.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_DAC_DELAY			(30 * _ONESEC / 16384)

/*@}*/


/** @name Phasing types
    This group defines the available phase-in and phase-out functions
    for the arbitrary function generator. The phase-in function ramps
    the signal up in amplitude from zero to its nominal value,
    whereas the phase-out function ramps the signal from its nominal
    value to its new value. The sweep-out function works in parallel 
    with the phase-out function; it sweeps the frequency from the
    nominal value to its new value.

    @memo Phase-in and phase-out function types
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and AWG_ConfigBlock
*********************************************************************/

/*@{*/

/** Phasing function is a step.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_PHASING_STEP		0

/** Phasing function is linear.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_PHASING_LINEAR		1

/** Phasing function is quadratic.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_PHASING_QUADRATIC		2

/** Phasing function is logarithmic. This constant is only defined
    for phase-out functions.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_PHASING_LOG			3

/** Ramp type macro. Builds the ramp type from the phase-in type,
    the phase-out type and the sweep-out type.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define RAMP_TYPE(PItype, POtype, SWtype) \
		(((PItype) & 0x000F) | \
	 	(((POtype) & 0x000F) << 4) | \
		(((SWtype) & 0x000F) << 12))

/** Phase-in type macro. Deduces the phase-in type from the ramp
    type.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define PHASE_IN_TYPE(ramptype) \
		((ramptype) & 0x000F)

/** Phase-out type macro. Deduces the phase-out type from the ramp
    type.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define PHASE_OUT_TYPE(ramptype) \
		(((ramptype) & 0x00F0) >> 4)

/** Sweep-out type macro. Deduces the sweep-out type from the ramp
    type.
    
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define SWEEP_OUT_TYPE(ramptype) \
		(((ramptype) & 0xF000) >> 12)

/*@}*/


/** @name Output types
    This group defines the available output types of the arbitrary 
    waveform generator.

    @memo Status bit masks
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and AWG_ConfigBlock
*********************************************************************/

/*@{*/

/** Identification for an LSC test point channel

    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_LSC_TESTPOINT		1

/** Identification for an ASC test point channel

    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_ASC_TESTPOINT		2

/** Identification for a DAC channel

    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_DAC_CHANNEL			3

/** Identification for a DS340 channel

    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_DS340_CHANNEL		4

/** Identification for a channel which gets written to file.

    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_FILE_CHANNEL		5

/** Identification for a channel which gets written to an aboslute
    address in reflective memory.

    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define	AWG_MEM_CHANNEL			6

/*@}*/

/** @name AWG status register
    This group define status bit masks which can be OR'ed together.
    @memo Status bit masks
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator and AWG_ConfigBlock
*********************************************************************/

/*@{*/

/** Configuration status bit mask.

    0 = not configured, 1 = configured
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_CONFIG		0x01

/** In use status bit mask.

    0 = not used, 1 = in use
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_IN_USE		0x02

/** Enable status bit mask.
    
    0 = off, 1 = on.
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_ENABLE		0x04

/** Sweep step type status bit mask.

    0 = linear, 1 = logarithmic.
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_SWEEP_LOG		0x200

/** Sweep cycle status bit mask.

    0 = sweep one direction only, 1 = sweep both directions.
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_SWEEP_CYCLE		0x400

/** Sweep halt bit mask.

    Halt sweep after one sweep

    0 = keep going, 1 = halt.
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
#define AWG_SWEEP_ONCE		0x800

/*@}*/

/*@}*/

/** @name Data types
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/

/*@{*/

/** Recognized component types.
    The arbitrary waveform generator can be used to generate sine
    and square waves, ramp and triangle signals, noise levels and
    sweeps of periodic wavforms.

    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   enum AWG_WaveType {
   /** nothing  */
   awgNone = 0,
   /** Sine wave */
   awgSine 	= 1,
   /** Square wave */
   awgSquare 	= 2,
   /** Ramp wave */
   awgRamp 	= 3,
   /** Triangle wave */
   awgTriangle 	= 4,
   /** Impulse function (square wave wit unequal hi-lo parts) */
   awgImpulse = 5,
   /** Constant offset */
   awgConst = 6,
   /** Normally distributed noise */
   awgNoiseN 	= 7,
   /** Uniformly distributed noise */
   awgNoiseU 	= 8,
   /** arbitrary waveform */
   awgArb	= 9,
   /** stream waveform */
   awgStream	= 10
   };
   typedef enum AWG_WaveType AWG_WaveType;

/** Recognized output modes.
    The output of an arbitrary waveform generator can be an LSC or
    ASC test point, a DAC channel, a DS340 channel or a file (test
    only).
   
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   enum AWG_OutputType {
   #if 0
   /** output disabled */
   awgNone = 0,
   #endif
   /** output to an LSC tespoint */
   awgLSCtp = AWG_LSC_TESTPOINT,
   /** output to an ASC tespoint */
   awgASCtp = AWG_ASC_TESTPOINT,
   /** output to a DAC channel */
   awgDAC = AWG_DAC_CHANNEL,
   /** output to a DS340 function generator */
   awgDS340 = AWG_DS340_CHANNEL,
   /** output to a file */
   awgFile = AWG_FILE_CHANNEL,
   /** output to an absolute memory location */
   awgMem = AWG_MEM_CHANNEL
   };
   typedef enum AWG_OutputType AWG_OutputType;

/** Structure to describe a component of a waveform.

    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   struct AWG_Component {
      /** Waveform type. */
      AWG_WaveType	wtype;
      /** Waveform parameters */
      double 		par[4];
      /** Start time of waveform */
      tainsec_t 	start;
      /** Duration of waveform. Infinite for values < 0.*/	
      tainsec_t 	duration;
      /** Restart time of waveform. No restart if <= 0. */
      tainsec_t		restart;
      /** Type of ramp */
      int 		ramptype;
      /** Phase-in and phase-out times */
      tainsec_t 	ramptime[2];
      /** Additional ramp parameters.
      Default is new amplitude, new frequency, new phase 
      and new offset */
      double 		ramppar[4];
   };
   typedef struct AWG_Component AWG_Component;

/** Structure to describe the gain of a waveform.

    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   struct AWG_Gain {
      /** State of gain: 0 - normal, 1 - new value, 2 - ramping */
      int		state;
      /** Gain value */
      double		value;
      /** Gain ramp duration */
      tainsec_t 	ramptime;
      /** Current gain */
      double            current;
      /** Old gain */
      double            old;
      /** Gain ramp start time */
      tainsec_t 	rampstart;
   };
   typedef struct AWG_Gain AWG_Gain;


#ifndef _GDS_AWGAPI_H
/** Structure to describe an arbitrary waveform generator.
   
    @author MRP, Apr. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   struct AWG_ConfigBlock {
      /** Mutex to protect this configuration block */
      mutexID_t		mux;
      /** Bit encoded status register */
      long 		status;
      /** Output type */
      AWG_OutputType	otype;
      /** Channel delay fine adjust */
      tainsec_t		delay;
      /** Output ID number */
      int		ID;
      /** output file name */
      char 		oname[128];
      /** Channel or file pointer for output */
      void* 		optr;
      /** Number of samples per page */
      int 		pagesize;
      /** Number of valid components */
      int 		ncomp;
      /** Seed for random number generator */
      randBlock 	rb[MAX_NUM_AWG_COMPONENTS];
      /** Block of AWG components */
      AWG_Component 	comp[MAX_NUM_AWG_COMPONENTS];
      /** Overall gain */
      AWG_Gain          gain;
      /** Waveform */
      float*		wave;
      /** Length of waveform */
      int		wavelen;
      /** time of last processed epoch */
      tainsec_t		tproc;
      /** Output data to RMEM 5565 or 5579 network (values 0 or 1) **/
      unsigned int	rmem;
      /** Can this waveform be cleared by a user? */
      unsigned int      unbreakable;
   };
   typedef struct AWG_ConfigBlock AWG_ConfigBlock;
#endif

/** Structure to describe the runtime statistics of the arbitrary 
    waveform generator. Statistical parameters are updated for
    each epoch. The time unit is the length of an epoch.

    Be sure to update the corresponding structure in rawgapi.x when
    making changes to the elements of this structure.
   
    @author DS, Jul. 1998
    @see Arbitrary Waveform Generator
*********************************************************************/
   struct awgStat_t {
      /** Number of waveform processing cycles */
      double		pwNum;
      /** Average time for processing waveforms (ns) */
      double		pwMean;
      /** Standard deviation of process time (ns) */
      double		pwStddev;
      /** Maximum time needed to process the waveforms */
      double		pwMax;
      /** Number of reflective memory writes */
      double		rmNum;
      /** Average time to write to reflective memory (ns) */
      double		rmMean;
      /** Standard deviation of writing to reflective memory (ns) */
      double		rmStddev;
      /** Maximum time needed to write to reflective memory (ns) */
      double		rmMax;
      /** Critical time. Smallest time difference between the 
          time the waveform is needed and the time the waveform
          is written to reflective memory. A negative value 
          indicates an instance where the waveform came too late (ns)*/
      double		rmCrit;
      /** Number of times the waveform was written too late. 
          This not necessarily means that the waveform is 
          corrupted! It only means the writting finished after
          the reading started. */
      double		rmNumCrit;
      /** Number of writes to the DAC */
      double		dcNum;
      /** Average time to write to the DAC (ns) */
      double		dcMean;
      /** Standard deviation of writing to the DAC (ns) */
      double		dcStddev;
      /** Maximum time needed to write to the DAC (ns) */
      double		dcMax;
      /** Number of times the waveform was written too late */
      double		dcNumCrit;
   };
   typedef struct awgStat_t awgStat_t;

/*@}*/

/*@}*/ 

#ifdef __cplusplus
}
#endif

#endif /*_GDS_AWGTYPE_H */
