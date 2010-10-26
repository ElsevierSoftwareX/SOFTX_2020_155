/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: decimate						*/
/*                                                         		*/
/* Module Description: Decimate-by-powers-of-2 function	       		*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer   		Comments       	       		*/
/* 0.1   10/26/98 Peter Fritschel       	       			*/
/* 0.2   11/23/98 PF			added 3rd FIR option		*/
/* 0.3	 3/9/99   PF			added decimate in-place func.   */
/* 1.0	 4/13/99  PF			changed algorithm: for muliple  */
/*					decimation stages, algorithm    */
/*					carrier over decimated data     */
/*					previous call; removed decimate */
/*					in-place funtion; changed prev  */
/*					& next pointers to floats;      */
/*					added 4th FIR option            */
/* 1.1  6/1/99    PF			added fir phase function	*/
/*                                                         		*/
/* Documentation References: 						*/
/*	Man Pages: doc++ generated html					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	P Fritschel   617-253-8153 617-253-7014 pf@ligo.mit.edu		*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Unix						*/
/*	Compiler Used: Sun cc						*/
/*	Runtime environment: Solaris 					*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
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

#ifndef _DECIMATE_H
#define _DECIMATE_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */


/**@name Decimation Algorithms
   This module is used to decimate data by powers of 2.

   @memo Decimation.
   @author Written Oct. 1998 by Peter Fritschel
   @version 1.0; PF; April 1999
*********************************************************************/

/*@{*/

/** @name Functions.
    @author PF, Oct 98
    @see Decimation routiunes
*********************************************************************/

/*@{*/    

/** The decimation routine filters and decimates a time series by a
    factor (dec_factor), which must be a power of 2. The routine
    requires an input array (x) and an output array (y). If the same,
    the decimation is done in place; in this case, for multiple calls,
    the calling program must ensure that the output data is written
    starting at the proper array index. In general a filter/decimation
    algorithm is applied repeatly on subsequent parts of a much longer
    time series. In order to avoid that the filter algorithm is
    restarted for each part, the routine takes two additional
    parameters, prev and next, which are used to pass temporary data
    from one part to the next. When called for the first time the
    pointer to the previous temporary data must be set to NULL,
    indicating that the routine should allocate a temporary data
    structure and pass it back through the next argument (pointer to
    pointer). When called for intermediate parts of the time series,
    the prev argument must be set to the temporary data structure
    which was returned by the previous call through the next
    argument. On the last call, the next pointer should be set to
    NULL, telling the routine to deallocate the temporary data
    structure. The start-up transient of the filter may be dealt with
    by including in the initial call at least dec_factor*(filter-order)
    points; the initial returned data can then be discarded to
    eliminate the transient. Subsequent calls should also be made
    with at least dec_factor*(filter-order) for calculational efficiency.

    The decimation is performed using a half-band FIR filter; for
    decimation factors greater than 2, multiple stages of the same
    filter are applied serially to the data. Several FIR filter
    designs are available:

\begin{tabular}{llrrr}
flag & design method & filter order & 
		cut-off freq (f_p/f_N) & passbnd ripple (stopbnd error)\\
     1	 &  least-squares    &   42	 &  0.9   &  0.015-0.2 dB\\      
     2   &  equiripple	     &   42	 &  0.9   &  0.06 dB\\
     3	 &  least-squares    &   22      &  0.9   &  0.1-1 dB\\
     4	 &  least-squares    &   82      &  0.9   &  0.0006-.01 dB
\end{tabular}


\begin{verbatim}
Example:
float       x[1024];
float       y[128];
float*       temp;

// initialize the filter data using an empty array
decimate (1, x, y, 0, 8, NULL, &temp);

// loop over parts of the time series and decimate data by 
// a factor of 8 
while (dataIsAvailable) {
  getData (x);
  decimate (1, x, y, 1024, 8, temp, &temp);
  saveResult (y);
}
 
// cleanup
decimate (1, x, y, 0, 8, temp, NULL);
\end{verbatim}

    @param flag FIR filter option
    @param x input time series
    @param y output time series (return)
    @param n number of point in the input array
    @param dec_factor decimation factor; MUST be a power of 2
    @param prev temporary filter data from previous decimation
    @param next temporary filter data for next decimation (return)
    @return int (GDS_ERROR)

    @memo Decimate time series by powers of 2
    @author PF, Oct 1998
    @see Multirate Digital Signal Processing; R Crochiere, L Rabiner
**********************************************************************/
   int decimate (int flag, const float x[], float y[], int n, 
                int dec_factor, float* prev, float** next);

/** FIR filter phase coefficient. This function returns a phase
    shift coefficient that applies to data that has been
    decimated with the FIR filter-based decimate function.
    If a time series has been decimated, and then turned
    into frequency data (via a fourier transform, eg), then
    this phase coefficient can be applied to the frequency
    data to recover the phase of the original data. To get
    the phase correction in radians for a particular frequency
    point f, the phase coefficient must be multiplied by the
    frequency f, and divided by the original sampling frequency
    of the (pre-decimated) data. The returned phase coefficient
    is positive, so when it is added to the phase of the
    frequency data, the phase lag added by the FIR filter is
    removed.
       
    @param flag FIR filter option
    @param dec_factor decimation factor; MUST be a power of 2
    @return double the phase coefficient, defined as above

    @memo FIR linear phase factor
    @author PF, May 1999
    @see 
**********************************************************************/
   double firphase (int flag, int dec_factor);

/** This function returns the name of the decimation filter 
    associated with the specified flag.
       
    @param flag FIR filter option
    @param type string for filter type
    @param size maximum size of filter string
    @return 0 if successful, <0 otherwise
    @memo decimation filter name
    @author PF, May 1999
    @see 
**********************************************************************/
   int decimationFilterName (int flag, char* type, int size);


/** The time delay function shifts a time series by any positive 
    integral number of sampling points. The calling convention for
    allocating/passing/deallocating temporary storage is the same
    as for the decimate function. The input and output array can be
    the same.
     
    @param x input time series
    @param y output time series (return)
    @param n number of point in the input array
    @param delay time delay of filter (in samples)
    @param prev temporary filter data from previous decimation
    @param next temporary filter data for next decimation (return)
    @return int (GDS_ERROR)

    @memo Delay a time series
    @author DS, Sep 1999
**********************************************************************/
   int timedelay (const float x[], float y[], int n, 
                 int delay, float* prev, float** next);


/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_DECIMATE_H */

