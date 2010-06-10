/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsrand							*/
/*                                                         		*/
/* Module Description: Random Generator					*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Programmer   Comments			   		*/
/* 0.1   4/16/98 Mark Pratt						*/
/* 0.2	 7Jul98  Daniel Sigg  separate header/object			*/
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

#ifndef _GDS_RAND_H
#define _GDS_RAND_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */

/** @name Random Number Utilities

    This library produces both flat and normal distributed random
    numbers.

    This family of functions is made reentrant through the
    use of the randBlock data structure.

    @memo Generates random numbers.
    @author MRP, Apr. 1998
    @see Numerical Recipes
*********************************************************************/

/*@{*/

/** @name Constants and flags.
    * Constants and flags of the random number generator.

    @author MRP, April 98
    @see Random Number Utilities
************************************************************************/

/*@{*/

/** */
#define RAND_IA 16807
/** modulus */
#define RAND_IM 2147483647
/** */
#define RAND_AM (1.0/RAND_IM)
/** */
#define RAND_IQ 127773
/** */
#define RAND_IR 2836
/** shuffle table size */
#define RAND_NTAB 32
/** */
#define RAND_NDIV (1+(RAND_IM-1)/RAND_NTAB)
/** epsilon relative to (0,1) interval */
#define RAND_EPS 1.2e-7
/** 1 - RAND_EPS */
#define RAND_RNMX (1-RAND_EPS)

/*@}*/


/** @name Data types.
    * Data types of the random number generator.

    @author MRP, April 98
    @see Random Number Utilities
************************************************************************/

/*@{*/

   /** Data buffer for reentrant random number generators.
   
    @see gdsrand_r
    @author MRP, Apr. 1998 
   *********************************************************************/
   struct randBlock 
   {
      /** random seed */
      long seed;
      /** last random long */
      long lastr;
      /** index shuffle table */
      long itab[RAND_NTAB];
      /** normal deviate counter mod 2 */
      int ncnt;
      /** next normal deviate */ 
      double nextn;
      /** Filter Stuff **/
      /** shift register */
      double shift_reg[6];
      /** filter denominator coefficients */
      double b[6];
      /** filter numerator coefficients */
      double a[6];
      /** rotating counter */
      unsigned int shift_count;      
  };
   typedef struct randBlock randBlock;

/*@}*/


/** @name Functions.
    * Functions of the random number generator.

    @author MRP, April 98
    @see Random Number Utilities
************************************************************************/

/*@{*/

/** Reetrant uniform random number generator.

    This is a reentrant version of Numerical Recipes rand1() which
    uses a lookup table to remove low order sequential correlations by
    shuffling the output. A zeroed randBlock (or just a seed value <=
    0) will result in an initialization of the generator.

    @param rb pointer to random block
    @param lo low value of uniform distribution
    @param hi high value of uniform distribution
    @return A uniformly distributed random number in (lo, hi).
    @see "Numerical Recipes in C" p. 280  ran1(). and _randBlock
    @author MRP, Apr. 1998 
*********************************************************************/
   double urand_r (randBlock *rb, double lo, double hi);

/** Uniform random vector generator.

    Fills buf with size random numbers uniformly distributed in
    (lo,hi).

    Not implemented.
    
    @return GDS_ERROR
    @see urand_r
    @author MRP, Apr. 1998
*********************************************************************/
   int urandv_r (randBlock *rb, double *buf, int size, 
                double lo, double hi);

/** Uniform random number generator with band-limit. This function
    filters the random numbers with a band-pass filter. A combination
    of hi < lo will reset the filter coefficients (no value is 
    returned). The frequencies are specifed as fractions of the
    sampling rate (assuming this function is called for every
    new sample point). Negative frequency values or values bigger 
    than one will be ignored in the filter calculation (meaning
    the corresponding filter will not be applied).

    @param rb pointer to random block
    @param lo low value of uniform distribution
    @param hi high value of uniform distribution
    @return A uniformly distributed random number in (lo, hi).
    @author MRP, Apr. 1998
*********************************************************************/
   double urand_filter_r (randBlock *rb, double lo, double hi);

/** Normal random number generator.

    @param rb pointer to random block
    @param m mean of normal distribution
    @param s standrad deviation of normal distribution
    @return A gaussian deviate of mean m and variance s^2.
    @see urand_r
    @author MRP, Apr. 1998
*********************************************************************/
   double nrand_r (randBlock *rb, double m, double s);

/** Normal random vector generator.

    Fills buf with size random numbers of a gaussian distribution of
    mean m and variance s^2.

    Not implemented.
    
    @return GDS_ERROR
    @see urand_r
    @author MRP, Apr. 1998
*********************************************************************/
   int nrandv_r (randBlock *rb, double *buf, int size, 
                double m, double s);

/** Normal random number generator with band-limit. This function
    filters the random numbers with a band-pass filter. A negative
    s will reset the filter coefficients (no value is returned).
    The frequencies are specifed as fractions of the
    sampling rate (assuming this function is called for every
    new sample point). Negative frequency values or values bigger 
    than one will be ignored in the filter calculation (meaning
    the corresponding filter will not be applied).

    @param rb pointer to random block
    @param m mean of normal distribution
    @param s standrad deviation of normal distribution
    @return A gaussian deviate of mean m and variance s^2.
    @see urand_r
    @author MRP, Apr. 1998
*********************************************************************/
   double nrand_filter_r (randBlock *rb, double m, double s);

/** Sets the filter coefficients for a random number generator.
    Currently supported filter types/orders:
    \begin{verbatim}
    type order      Description
     0    -         no filter
     1    2,4,6,8   Butterworth bandpass, lowpass, highpass
    \end{verbatim}
    Corner frequencies are specified in units of the sampling
    frequency, i.e., values are between 0 and 0.5. Filters
    are selected as follows: 
    \begin{verbatim}
    0 < f1 < f2 < 0.45           bandpass
    f1 = 0 && 0 < f2 < 0.45      lowpass
    0 < f1 < 0.45 && f2 >= 0.45  highpass
    \end{verbatim}

    @param rb pointer to random block
    @param m mean of normal distribution
    @param s standrad deviation of normal distribution
    @param f1 lower frequency corner of band-pass
    @param f2 upper frequency corner of band-pass
    @return 0 if successful, <0 otherwise.
    @author DS, Sep. 2000
    
    Ignore all that, currently supports no filter
      bandpass
      lowpass
      highpass and
      notch filter
    f1 = low freq (fract of sample rate)
    f2 = high freq (again fraction)
    dt = time between samples 1 / sample rate
*********************************************************************/
   int rand_filter (randBlock *rb, double dt, double f1, double f2);

/*@}*/

/*@}*/ 

#ifdef __cplusplus
}
#endif

#endif /*_GDS_RAND_H */

