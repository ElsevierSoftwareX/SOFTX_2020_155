/****************************************************************
 *								*
 *  Module Name: Decimate-by-2 function				*
 *								*
 *  Procedure Description: decimates an input data stream	*
 *	by a factor of 2, using a half-band FIR filter to	*
 *	perform anti-aliasing 					*
 *								*	
 *  External Procedure Name: decimate				*
 *								*
 *  Procedure Arguments: flag for determining filter type;	*
 *	pointers to input and output vectors; number of		*
 *	input points; number of serial decimation stages;	*
 *	pointer to data from previous block; pointer to 	*
 *	data for next block					*
 *								*
 *								*
 ****************************************************************/
/*#define PI      3.1415926*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
/*#include "gdsconst.h"*/
#include "decimate.h"
/*#include "gdssigproc.h"*/

   static const float firls1[11] = 
   {2.225549e-3, -3.914217e-3, 6.226653e-3,
    -9.330331e-3, 1.347478e-2, -1.907462e-2,
    2.690526e-2, -3.863524e-2, 5.863624e-2,
    -1.030298e-1, 3.172755e-1};
   static const float firPM1[11] = 
   {5.704943e-3, -5.292245e-3, 7.672972e-3,
    -1.083958e-2, 1.493768e-2, -2.047472e-2,
    2.813003e-2, -3.967826e-2, 5.939968e-2,
    -1.035220e-1, 3.174278e-1};
   static const float firls2[6] =
   {-1.353431e-2, 2.193691e-2, -3.448320e-2,
    5.550899e-2, -1.010866e-1, 3.166165e-1};
   static const float firls3[21] =
   {8.549310e-5, -1.882289e-4, 3.483209e-4,
    -5.835810e-4, 9.146788e-4, -1.365369e-3,
    1.962879e-3, -2.738601e-3, 3.729302e-3,
    -4.979237e-3, 6.543786e-3, -8.495763e-3,
    1.093660e-2, -1.401691e-2, 1.797646e-2,
    -2.322807e-2, 3.055373e-2, -4.163686e-2,
    6.087163e-2, -1.044086e-1, 3.177415e-1};


   int decimate (int flag, const float x[], float y[], int n, 
                int dec_factor, float* prev, float** next)
   {
      double		mantissa;
      const float*	filt_coeff;
      float 		sum;
      float*		dataPtr;
      int		filt_length;
      int 		tempnpt;
      int		npt;
      int		filt_ord;
      int		i;
      int		j;
      int		k;
      int 		npt_y;
      int		ndec;
   
      switch (flag) {		/*determine which filter to use*/
         case 2:
            {
               filt_coeff = firPM1;
               filt_length = 11;
               break;
            }
         case 3:
            {
               filt_coeff = firls2;
               filt_length = 6;
               break;
            }
         case 4:
            {
               filt_coeff = firls3;
               filt_length = 21;
               break;
            }
         case 1:
         default:
            {
               filt_coeff = firls1;
               filt_length = 11;
               break;
            }
      }
   
      /*  check that dec_factor is a power of 2; return error if not */
      if ((mantissa = frexp((double) dec_factor, &ndec)) != 0.5)
         return (-1);
      --ndec;			    	   /* this makes 2^(ndec) = dec_factor   */
      npt_y = (n/dec_factor);              /* # pts in the output array   */
      filt_ord = 4*filt_length - 2;        /* filter order  */
      tempnpt = ndec*filt_ord;             /* #pts needed in temp array to */
                                           /* bridge over multiple calls   */
      /* set up temporary data pointer on first function call: */
      /* initialize prev with tempnpt elements                 */
   
      if (prev == NULL) {
         prev = calloc(tempnpt, sizeof(float));
      }
   
   /* make a new data array with the previous tempnpt data      */
   /* points first, and the x data points next; then copy last  */
   /* tempnpt points to prev, for next call                     */
   
      dataPtr = malloc((n + tempnpt) * sizeof(float));
      memcpy (dataPtr, prev, sizeof(float) * tempnpt);
      memcpy ((dataPtr + tempnpt), x, sizeof(float) * n);
   
   /* filter algorithm; half-band, linear phase FIR filter;	*/
   /* taking advantage of the symmetry of the filter		*/
   /* coefficients, the appropriate data pairs are added	*/
   /* together, then multiplied by the filter coefficients	*/
   
    /*   printf("data shuffling done\n"); */
      npt = n;
      dataPtr += tempnpt;
      for (k = 1; k <= ndec; ++k) {
         dataPtr -= filt_ord;	/* start at data from previous call  */
         if (next != NULL) {
            memcpy ((prev + tempnpt - k*filt_ord), (dataPtr + npt), 
                   sizeof(float) * filt_ord);
         }
         npt /= 2;     /* decrease #pts by factor of 2 at each stage */
         for (i = 0; i < npt; ++i) {	
            sum = 0.0;
            for (j = 0; j < filt_length; j++) {
               sum += filt_coeff[j] *
                      (dataPtr[2*(i+j)] + dataPtr[filt_ord + 2*(i-j)]);
            }
            dataPtr[i] = sum + dataPtr[filt_ord/2 + 2*i]/2.0;
         }
      }
   
      /* copy the output values into y */
      memcpy (y, dataPtr, sizeof(float) * npt_y);
   
      if (next == NULL)		
         free(prev);		/*  cleanup  */
      else
         *next = prev;
      free(dataPtr);
      return 0;
   }	


   double firphase (int flag, int dec_factor) {
   
      double		phasefactor;
      int		filt_length;
      int		filt_ord;
   
      switch (flag) {		/*determine which filter to use*/
         case(1):
            filt_length = 11;
            break;
         case(2):
            filt_length = 11;
            break;
         case(3):
            filt_length = 6;
            break;
         case(4):
            filt_length = 21;
            break;
         default:
            filt_length = 11;
            break;
      }
   
      filt_ord = 4*filt_length - 2;        /* filter order  */
      /*phasefactor = PI * (dec_factor - 1) * filt_ord;*/
      phasefactor = M_PI * (dec_factor - 1) * filt_ord;
      return phasefactor;
   
   }


   int decimationFilterName (int flag, char* type, int size)
   {
      char		buf[256];
   
      switch (flag) {
         case 2:
            {
               sprintf (buf, "FIR (equiripple): order=%i fR=%f "
                       "pass. ripple=%f-%f dB stopband attn.=%i-%i dB", 
                       42, 0.9, 0.05, 0.05, 43, 43);
               break;
            }
         case 3:
            {
               sprintf (buf, "FIR (least-squares): order=%i fR=%f "
                       "pass. ripple=%f-%f dB stopband attn.=%i-%i dB", 
                       22, 0.9, 0.1, 0.8, 30, 40);
               break;
            }
         case 4:
            {
               sprintf (buf, "FIR (least-squares): order=%i fR=%f "
                       "pass. ripple=%f-%f dB stopband attn.=%i-%i dB", 
                       82, 0.9, 0.0006, 0.01, 60, 90);
               break;
            }
         case 1:
         default:
            {
               sprintf (buf, "FIR (least-squares): order=%i fR=%f "
                       "pass. ripple=%f-%f dB stopband attn.=%i-%i dB", 
                       42, 0.9, 0.02, 0.1, 40, 56);
               break;
            }
      }
      strncpy (type, buf, size);
      return 0;
   }


   int timedelay (const float x[], float y[], int n, 
                 int delay, float* prev, float** next)
   {
      int		size;		/* copy size */
   
      /* check array arguments */
      if ((n > 0) && ((x == NULL) || (y == NULL))) {
         return -1;
      }
   
      /* handle special cases */
      if (delay < 0) {
         /* error */
      }
      else if (delay == 0) {
         if ((n > 0) && (x != y)) {
            memcpy (y, x, n * sizeof (float));
         }
      }
      else { /* delay > 0 */
      
         /* set up temporary data pointer on first function call */
         if (prev == NULL) {
            prev = calloc (2 * delay, sizeof(float));
            if (prev == NULL) {
               return -1;
            }
            memset (prev, 0, 2 * delay * sizeof (float));
         }
      
         /* copy things around */
         if (n > 0) {
            size = (n < delay) ? n : delay;
            /* remove last size samples from input */
            memcpy (prev + delay, x + n - size, size * sizeof (float));
            /* copy rest of input to back of output */
            if (n > delay) {
               memmove (y + size, x, (n - size) * sizeof (float));
            }
            /* copy front of delay buffer to front of output */
            memcpy (y, prev, size * sizeof (float));
            /* move rest of data in delay buffer upfront */
            memmove (prev, prev + size, delay * sizeof (float));
         }
      }
   
      /* cleanup */
      if (next == NULL)	{	
         free (prev);
      }
      else {
         *next = prev;
      }
      return 0;
   }

