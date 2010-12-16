static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: Random Number generator					*/
/*                                                         		*/
/* Procedure Description: generate random numbers			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include <math.h>
#include <string.h>
#ifdef LIGO_GDS
#include "tconv.h"
#else 
#include <time.h>
#endif
#include "dtt/gdsrand.h"


   static double rand_filter_calc (randBlock *rb, double x, int stage);


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: urand_r					*/
/*                                                         		*/
/* Procedure Description: reentrant uniform random number generator	*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			distribution limits, lo and hi			*/
/*                                                         		*/
/* Procedure Returns: uniform random deviate				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double urand_r (randBlock *rb, double lo, double hi) 
   {
   
      int j;
      long k;
      double temp;
   
      /*--- check for initialize condition ---*/
      if (rb->seed <= 0) {
         unsigned long seed;
      #ifdef LIGO_GDS
         seed = (TAInow() / 1000) & 0xFFFFFFFF;
      #else
         seed = time(NULL);
      #endif
         rb->seed = (rb->seed != 0) ? -rb->seed : seed;
         for (j = RAND_NTAB+7; j >= 0; --j) {
            k = (long) (rb->seed / RAND_IQ);
            rb->seed = RAND_IA * (rb->seed - k * RAND_IQ) - RAND_IR * k;
            if (rb->seed < 0)
               rb->seed += RAND_IM;
            if (j < RAND_NTAB)
               rb->itab[j] = rb->seed;
         }
         rb->lastr = rb->itab[0];
      }
   
      /*--- normal operation ---*/
      k = (long) (rb->seed / RAND_IQ);
      rb->seed = RAND_IA * (rb->seed - k * RAND_IQ) - RAND_IR * k;
      if (rb->seed < 0)
         rb->seed += RAND_IM;
      j = rb->lastr / RAND_NDIV;
      rb->lastr = rb->itab[j];
      rb->itab[j] = rb->seed;
      if ((temp = RAND_AM * rb->lastr) > RAND_RNMX)
         return RAND_RNMX * (hi - lo) + lo;
      return temp * (hi - lo) + lo;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: urand_filter_r				*/
/*                                                         		*/
/* Procedure Description: reentrant uniform random number generator	*/
/* 			with bandlimit					*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			distribution limits, lo and hi			*/
/*                      lower and upper frequency corner   		*/
/*                                                         		*/
/* Procedure Returns: uniform random deviate				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double urand_filter_r (randBlock *rb, double lo, double hi) 
   {
      double		x;	/* random value */
   
      /* get new random number */
      x = urand_r (rb, lo, hi);
      /* calculate filter */
      x = rand_filter_calc (rb, x, 0);
      /* return */
      return x;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: urandv_r					*/
/*                                                         		*/
/* Procedure Description: reentrant uniform random vector generator	*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			distribution limits, lo and hi			*/
/*			output buffer and size in samples		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int urandv_r (randBlock *rb, double *buf, int size, 
                double lo, double hi) 
   {
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: nrand_r					*/
/*                                                         		*/
/* Procedure Description: reentrant normal random number generator	*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			distribution parameters, mean and sigma		*/
/*                                                         		*/
/* Procedure Returns: normal random deviate				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double nrand_r (randBlock *rb, double m, double sig) 
   {
      double fac, rsq, v1, v2;
   
      if (rb->ncnt == 0) {		/* empty buffer */
         do {			/* pick 2 numbers in the unit circle */
            v1 = urand_r (rb, -1.0, 1.0);
            v2 = urand_r (rb, -1.0, 1.0);
            rsq = v1*v1 + v2*v2;
         } while (rsq >= 1.0 || rsq == 0.0);
      
         fac = sqrt (-2.0 * log (rsq) / rsq);
         rb->nextn = v1 * fac;
         rb->ncnt = 1;
         return v2 * fac * sig + m;
      } 
      else {			/* got one waiting */
         rb->ncnt = 0;
         return rb->nextn * sig + m;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: nrandv_r					*/
/*                                                         		*/
/* Procedure Description: reentrant normal random vector generator	*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			distribution parameters, mean and sigma		*/
/*			output buffer and size in samples		*/
/*                                                         		*/
/* Procedure Returns: GDS_ERROR						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int nrandv_r (randBlock *rb, double *buf, int size, 
                double m, double sig) 
   {
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: nrand_filter_r				*/
/*                                                         		*/
/* Procedure Description: reentrant normal random number generator	*/
/* 			with bandlimit					*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			distribution parameters, mean and sigma		*/
/*                      lower and upper frequency corner   		*/
/*                                                         		*/
/* Procedure Returns: uniform random deviate				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double nrand_filter_r (randBlock *rb, double m, double s) 
   {
      double		x;	/* random value */
   
      /* get new random number */
      x = nrand_r (rb, m, s);
      /* calculate filter */
      x = rand_filter_calc (rb, x, 0);
      /* return */
      return x;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: rand_filter					*/
/*                                                         		*/
/* Procedure Description: set filter for random number generator	*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			type, order, 					*/
/*                      lower and upper frequency corner   		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int rand_filter (randBlock *rb, double dt, double f1, double f2)
   { 
      int sw;  /* 0 = bandpass, 1 = lowpass, 2 = highpass, 
                  3 = no filter, 4 = bandstop */
      double G1,G2=G1=0;  /*ReNormalization gains*/
   
      for (sw = 0 ; sw < 6; sw ++)
         rb->a[sw] = rb->b[sw] = rb->shift_reg[sw] = 0.0;
      rb->b[0] = 1.0;
      rb->b[3] = 1.0;
      rb->shift_count = 0;
      /*printf (" %g\t%g\t", f1, f2);*/
      /*
      if (0 < f1 < f2 < 0.45)
       sw = 0;
      if ((f1 == 0) && (0 < f2 < 0.45))
       sw = 1;
      if ((0 < f1 < 0.45) && (f2 >= 0.45))
       sw = 2;
      */
      if (f1 < f2)
         if (0 < f1)
            if (f2 < 0.45)
               sw = 0;
            else
               if (f1 < 0.45)
                  sw = 2;
               else
                  sw = 3;
         else
            if (f2 < 0.45)
               sw = 1;
            else
               sw = 3;
      else
         if ((0 < f2) && (f1 < 0.45))
            sw = 4;
         else
            sw = 3;
   /*
   I don't know why I have to do this since it's defined in math.h
   but what the fuck it's easier to do it myself than figure out why
   gcc won't do it
   */
   #define M_PI 	3.14159265358979323846
   /* Ok now the filter coefficients
   */
   
      f1 = tan(M_PI * f1);
      f2 = tan(M_PI * f2); 	 
   
      switch (sw)
      {
         case 0: /*printf ("bandpass\n");*/
         
            G1 = ((f1+f2)*(f1+f2) + 4*f1*f1) / ((f1+f2)*(f1+f2));
            G2 = ((f1+f2)*(f1+f2) + 4*f2*f2) / (4*f2*f2);
         
            rb->b[0] = (G1) / ((1+f1)*(1+f1));
            rb->b[1] = -2*rb->b[0];
            rb->b[2] = rb->b[0];
            rb->a[1] = (2- 2*f1)/(1+ f1);
            rb->a[2] = -((1 - f1) * (1 - f1)) / ((1 + f1) * (1 + f1));
         
            rb->b[3] = (f2*f2*G2) / ((1 + f2) * (1 + f2));
            rb->b[4] = 2*rb->b[3];
            rb->b[5] = rb->b[3];
            rb->a[4] = (2 - 2*f2) / (1 + f2);
            rb->a[5] = -((1 - f2) * (1 - f2)) / ((1 + f2) * (1 + f2));	       
            break;
         case 1: /*printf ("lowpass\n");*/
            rb->b[0] = f2*f2 / ((1 + f2) * (1 + f2));
            rb->b[1] = 2*rb->b[0];
            rb->b[2] = rb->b[0];
            rb->a[1] = (2 - 2*f2) / (1 + f2);
            rb->a[2] = -((1 - f2) * (1 - f2)) / ((1 + f2) * (1 + f2));	       
            rb->b[3] = rb->b[0];
            rb->b[4] = rb->b[1];
            rb->b[5] = rb->b[2];
            rb->a[4] = rb->a[1];
            rb->a[5] = rb->a[2];
            break;
         case 2: /*printf ("highpass\n");*/
            rb->b[0] = 1 / ((1 + f1)*(1 + f1));
            rb->b[1] = -2*rb->b[0];
            rb->b[2] = rb->b[0];
            rb->a[1] = (2- 2*f1)/(1+ f1);
            rb->a[2] = -((1 - f1) * (1 - f1)) / ((1 + f1) * (1 + f1));
            rb->b[3] = rb->b[0];
            rb->b[4] = rb->b[1];
            rb->b[5] = rb->b[2];
            rb->a[4] = rb->a[1];
            rb->a[5] = rb->a[2];
            break;
         case 3: /*printf ("no filter\n");*/
            break;
         case 4: /*printf ("bandstop filter\n");*/
            rb->b[0] = (1 + f1* f2) / (1 + f1 - f2 + f1*f2);
            rb->b[1] = (-2 + 2* f1* f2) / (1 + f1 - f2 + f1*f2);
            rb->b[2] = rb->b[0];
            rb->a[1] = (2- 2*f1*f2)/(1+ f1 - f2 + f1*f2);
            rb->a[2] = -(1- f1 + f2 + f1*f2) / (1+ f1- f2 + f1*f2);
            rb->b[3] = rb->b[0];
            rb->b[4] = rb->b[1];
            rb->b[5] = rb->b[2];
            rb->a[4] = rb->a[1];
            rb->a[5] = rb->a[2];
            break;
         default: /*ERROR*/
            return -1;
      };
      /*printf ("f1 = %g\t f2 = %g\n",f1,f2);
      printf ("b:");
      for (sw = 0 ; sw < 6; sw ++)
        printf ("%g  ",rb->b[sw]);
      printf ("\na:");
      for (sw = 0 ; sw < 6; sw ++)
        printf ("%g  ",rb->a[sw]);
      printf ("\n\n");*/  
      return 0;  
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Internal Procedure Name: rand_filter_calc				*/
/*                                                         		*/
/* Procedure Description: set filter for random number generator	*/
/* 									*/
/* Procedure Arguments: randBlock (seed etc...) and			*/
/*			type, order, 					*/
/*                      lower and upper frequency corner   		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   static double rand_filter_calc (randBlock *rb, double u, int stage)
   {
      double x = 0;
      int zero = (rb->shift_count)%3 + 3*stage;
      int one = ((rb->shift_count)+1)%3 + 3*stage;
      int two = ((rb->shift_count)+2)%3 + 3*stage;
   
      /* apply recursive filter coefficients (poles in transfer funct) */
      rb->shift_reg[zero] = u + 
                           rb->a[1+3*stage] * rb->shift_reg[ one ] +
                           rb->a[2+3*stage] * rb->shift_reg[ two ];
   
      /* apply direct filter coefficients (zeros in transfer funct) */
      x = rb->b[0+3*stage] * rb->shift_reg[ zero ] +
          rb->b[1+3*stage] * rb->shift_reg[ one ] +
          rb->b[2+3*stage] * rb->shift_reg[ two ];
   
      /* increment shift_register; same as copying shift_reg[0] to */
      /* shift_reg[1], and [1] to [2] etc. */
      if (stage < 1)
      { 
         x = rand_filter_calc(rb, x, stage + 1);
         rb->shift_count = ((rb->shift_count) + 5)%3;
      };
   /*      printf ("%lf\t%lf\t%lf\n", rb->shift_reg[zero], 
                          rb->shift_reg[one],
   		  rb->shift_reg[two]);
   */      
      return x;
   }

