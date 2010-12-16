static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: awgfunc							*/
/*                                                         		*/
/* Module Description: implements signal functions for the AWG		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/


/* Header File List: */
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "gdsconst.h"
#include "dtt/gdsutil.h"
#include "dtt/awgfunc.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Defines: numerical constants to share between Unix/vxWorks		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define POWER(x,y)	(exp (log (x) * (y)))
#define __ONESEC	1E9
/* :COMPILER: replace EXTREMLY inefficient fmod function in VxWorks! */
#ifdef OS_VXWORKS
#define fmod(x,y)	(((x) >= 0) ? ((x) - (y) * floor ((x) / (y))) : \
				      ((x) - (y) * ceil ((x) / (y))))
#endif
/* same as above but always returns positive result */
#define fMod(x,y)	((x) - (y) * floor ((x) / (y)))


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: normPhase					*/
/*                                                         		*/
/* Procedure Description: normalizes the phase (mod 2 \pi)		*/
/*                                                         		*/
/* Procedure Arguments: phase						*/
/*                                                         		*/
/* Procedure Returns: normalized phase					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double normPhase (double phi)
   {
      if ((phi >= 0) && (phi < TWO_PI)) {
         return phi;
      }
      else {
         return fMod (phi, TWO_PI);
      }
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: startPhase					*/
/*                                                         		*/
/* Procedure Description: calculates the start phase t*f*2pi-dphi	*/
/*                                                         		*/
/* Procedure Arguments: frequency, start time.                          */
/*                                                         		*/
/* Procedure Returns: normalized phase					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double startPhase (double f, double t, double dphi)
   {
       double tint, fint;
       tint = (long)t;
       fint = (long)f;
       return normPhase(TWO_PI*(f*(t-tint) + tint*(f-fint)) - dphi);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgPeriodicSignal				*/
/*                                                         		*/
/* Procedure Description: periodic function				*/
/*                                                         		*/
/* Procedure Arguments: waveform type, amplitude, offset, phase, 	*/
/*			random number parameters			*/
/*                                                         		*/
/* Procedure Returns: function value					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double awgPeriodicSignal (AWG_WaveType wtype, double A, double ofs, 
                     double phi)
   {
      double		x;
   
      switch (wtype) {
         case awgSine:
            {
               return A * sin(phi) + ofs;
            }
         case awgSquare:
            {  
               if (normPhase (phi) < ONE_PI) {
                  return A + ofs;
               }
               else {
                  return -A + ofs;
               }
            }
         case awgRamp:
            {
               return A * (normPhase (phi) / TWO_PI) + ofs;
            }
         case awgTriangle:
            {
               if ((x = normPhase (phi)) < ONE_PI) {
                  return A * (x * TWO_OVER_PI - 1) + ofs;
               }
               else {
                  return A * (3 - x * TWO_OVER_PI) + ofs;
               }
            }
         case awgConst:
            {
               return ofs;
            }
         default: 
            {
               return 0.0;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgNoiseSignal				*/
/*                                                         		*/
/* Procedure Description: noise function				*/
/*                                                         		*/
/* Procedure Arguments: waveform type, amplitude, offset, flow, fhigh, 	*/
/*			random number parameters			*/
/*                                                         		*/
/* Procedure Returns: function value					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double awgNoiseSignal (AWG_WaveType wtype, double A, double ofs, 
                     randBlock* rbp)
   {
      switch (wtype) {
         case awgNoiseN:
            {
               /* normal distribution */
               return nrand_filter_r (rbp, ofs, A);
            }
         case awgNoiseU :
            {
               /* uniform distribution */
               return urand_filter_r (rbp, ofs - A, ofs + A);
            }
         default: 
            {
               return 0.0;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgPhaseIn					*/
/*                                                         		*/
/* Procedure Description: phase-in function				*/
/*                                                         		*/
/* Procedure Arguments: ramp type, time, phase-in duration		*/
/*                                                         		*/
/* Procedure Returns: phase-in function value				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double awgPhaseIn (int rtype, double t, double tPI)
   {
      double 		q;
   
      switch (rtype) {
         case AWG_PHASING_STEP:
            {
               return 0.0;
            }
         case AWG_PHASING_LINEAR:
            {
               return t / tPI;
            }
         case AWG_PHASING_QUADRATIC:
         case AWG_PHASING_LOG:	/* no log */
            {
               q = t / tPI;
               q *= q;
               return -q*q + 2*q;
            }
         default:
            {
               return 0.0;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgPhaseOut					*/
/*                                                         		*/
/* Procedure Description: phase-out function				*/
/*                                                         		*/
/* Procedure Arguments: ramp type, time, phase-in duration, ampl. ratio	*/
/*                                                         		*/
/* Procedure Returns: phase-out function value				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double awgPhaseOut (int rtype, double t, double tPO, double c)
   {
      double 		q;
   
      switch (rtype) {
         case AWG_PHASING_STEP:
            {
               return 1.0;
            }
         case AWG_PHASING_LINEAR:
            {
               return 1 - (1 - c) * (t / tPO);
            }
         case AWG_PHASING_QUADRATIC:
            {
               q = t / tPO;
               q *= q;
               return 1 + (1 - c) * (q*q - 2*q);
            }
         case AWG_PHASING_LOG:
            {
               if (c == 0) {
                  return 0.0;
               }
               else {
                  return POWER (fabs(c), t / tPO);
               }
            }
         default:
            {
               return 0.0;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSweepOut					*/
/*                                                         		*/
/* Procedure Description: sweep-out function				*/
/*                                                         		*/
/* Procedure Arguments: ramp type, time, phase-out duration		*/
/*                      frequency and frequency difference, 		*/
/*                      phase adjustment	            		*/
/*                                                         		*/
/* Procedure Returns: phase-in function value				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double awgSweepOut (int rtype, double t, double tPO, 
                     double f, double df, double dphi)
   {
      double 		q;
   
      switch (rtype) {
         case AWG_PHASING_STEP:
            {
               return 0.0;
            }
         case AWG_PHASING_LINEAR:
         case AWG_PHASING_QUADRATIC:	/* no quadratic freq. sweeps */
            {
               return (t / tPO) * (TWO_PI * df * t - dphi);
            }
         case AWG_PHASING_LOG:
            {
               q = t / tPO;
               return (TWO_PI * f * tPO * POWER (df, q) - dphi) * q;
            }
         default:
            {
               return 0.0;
            }
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSweepComponents				*/
/*                                                         		*/
/* Procedure Description: makes AWG sweep components			*/
/*                                                         		*/
/* Procedure Arguments: start time, duration, start freq., stop freq.	*/
/*                      start ampl., stop ampl., flag,			*/
/*                      awg component, number of components		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSweepComponents (tainsec_t t, tainsec_t d, double f1, double f2,
                     double a1, double a2, long flag, 
                     AWG_Component* comp, int* cnum)
   {
      /* first handle bi-directional sweep */
      if ((flag & AWG_SWEEP_CYCLE) != 0) {
         /* call sweepComponents recursively for each half 
            of the sweep */
         if (awgSweepComponents (t, d/2, f1, f2, a1, a2, 
            flag & ~AWG_SWEEP_CYCLE, comp, cnum) != 0) {
            return -1;
         }
         if (awgSweepComponents (t + d/2, d/2, f2, f1, a2, a1, 
            flag & ~AWG_SWEEP_CYCLE, comp + 1, cnum) != 0) {
            return -1;
         }
         *cnum = 2;
      
      	 /* now patch restart value if necessary */
         if ((flag & AWG_SWEEP_ONCE) == 0) {
            comp[0].restart = d;
            comp[1].restart = d;
         }
         return 0;
      }
   
      /* one directional sweep here */
      if ((t < 0) || (d <= 0) || (f1 < 0) || (f2 < 0) || 
         (a1 < 0) || (a2 < 0)) {
         return -1;
      }
      comp->wtype = awgSine;
      comp->start = t;
      comp->duration = d;
      comp->restart = ((flag & AWG_SWEEP_ONCE) == 0) ? d : 0;
      comp->ramptime[0] = 0;	/* no phase-in period */
      comp->ramptime[1] = d;	/* only phase out */
      comp->ramptype = ((flag & AWG_SWEEP_LOG) == 0) ?
                       RAMP_TYPE (AWG_PHASING_STEP, 
                                 AWG_PHASING_LINEAR, AWG_PHASING_LINEAR) : 
                       RAMP_TYPE (AWG_PHASING_STEP, 
                                 AWG_PHASING_LOG, AWG_PHASING_LOG);
      comp->par[0] = fabs (a1);
      comp->par[1] = f1; 
      comp->par[2] = 0;		/* start with 0 phase */
      comp->par[3] = 0;		/* no offset */
      comp->ramppar[0] = fabs (a2);
      comp->ramppar[1] = f2; 
      comp->ramppar[2] = 0;	/* stop with 0 phase */
      comp->ramppar[3] = 0;	/* no offset */
      *cnum = 1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgPeriodicComponent			*/
/*                                                         		*/
/* Procedure Description: makes AWG periodic components			*/
/*                                                         		*/
/* Procedure Arguments: waveform type, frequency, amplitude,		*/
/*                      phase, offset, awg component			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgPeriodicComponent (AWG_WaveType wtype, double f, double A, 
                     double phi, double ofs, AWG_Component* comp)
   {
      return awgPeriodicComponentEx (wtype, TAInow(), 
                           f, A, phi, ofs, comp);
   }

   int awgPeriodicComponentEx (AWG_WaveType wtype, tainsec_t t0, 
                     double f, double A, double phi, double ofs, 
                     AWG_Component* comp)
   {
      /* first test if valid */
      if (((wtype != awgSine) && (wtype != awgRamp) &&
         (wtype != awgSquare) && (wtype != awgTriangle) &&
         (wtype != awgArb) && (wtype != awgImpulse)) || 
         (f <= 0) || (comp == NULL)) {
         return -1;
      }
   
      /* now fill component */
      memset (comp, 0, sizeof (AWG_Component));
      comp->wtype = wtype;
      comp->par[0] = A;
      comp->par[1] = f;
      comp->par[2] = phi;
      comp->par[3] = ofs;
      comp->start = t0;
      /* make sure a single impulse is not lost */
      if ((wtype == awgImpulse) && (f <= 0)) {
         comp->start += 4 *_EPOCH;
      }
      /* align impuls to 1 sec boundary */
      if ((wtype == awgImpulse) && (f > 0)) {
         comp->start = _ONESEC * (comp->start / _ONESEC);
      }
      /* align phase of other components to 24h boundary */
      if ((wtype != awgArb) && (wtype != awgImpulse)) {
         comp->par[2] -= fmod (TWO_PI * f * (double) 
                              (comp->start % _ONEDAY) / __ONESEC, TWO_PI);
      }
      comp->duration = -1;
      comp->restart = -1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgConstantComponent			*/
/*                                                         		*/
/* Procedure Description: makes AWG constant components			*/
/*                                                         		*/
/* Procedure Arguments: offset						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgConstantComponent (double ofs, AWG_Component* comp)
   {
      return awgConstantComponentEx (TAInow(), ofs, comp);
   }

   int awgConstantComponentEx (tainsec_t t0, double ofs, 
                     AWG_Component* comp)
   {
      /* first test if valid */
      if (comp == NULL) {
         return -1;
      }
   
      /* now fill component */
      memset (comp, 0, sizeof (AWG_Component));
      comp->wtype = awgConst;
      comp->par[0] = 0;
      comp->par[1] = 0;
      comp->par[2] = 0;
      comp->par[3] = ofs;
      comp->start = t0;
      comp->duration = -1;
      comp->restart = -1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSquareWaveComponent			*/
/*                                                         		*/
/* Procedure Description: makes AWG periodic components			*/
/*                                                         		*/
/* Procedure Arguments: frequency, amplitude, phase			*/
/*                      offset, ratio, awg components			*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgSquareWaveComponent (double f, double A, double phi, 
                     double ofs, double ratio, AWG_Component* comp)
   {
      return awgSquareWaveComponentEx (TAInow(), 
                           f, A, phi, ofs, ratio, comp);
   }

   int awgSquareWaveComponentEx (tainsec_t t0, 
                     double f, double A, double phi, 
                     double ofs, double ratio, AWG_Component* comp)
   {
      if ((f <= 0) || (ratio < 0)) {
         return -1;
      }
      if ((awgPeriodicComponentEx (awgImpulse, t0, f, 2 * A, 
         ratio / f, 0, comp) < 0) ||
         (awgConstantComponent (-A + ofs, comp + 1) < 0)) {
         return -1;
      }
      comp[0].start += 
         (tainsec_t) (__ONESEC * (-fmod (f * (double) 
                     (comp->start % _ONEDAY) / __ONESEC, 1.0)) / f +
                     __ONESEC * (fMod (phi / TWO_PI, 1) - 1)/ f + 0.5);
      comp[1].start = comp[0].start;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgNoiseComponent				*/
/*                                                         		*/
/* Procedure Description: makes AWG noise components			*/
/*                                                         		*/
/* Procedure Arguments: waveform type, lower frequency, upper frequency,*/
/*                      amplitude, offset, awg component		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgNoiseComponent (AWG_WaveType wtype, double f1, double f2, 
                     double A, double ofs, AWG_Component* comp)
   {
      return awgNoiseComponentEx (wtype, TAInow(), f1, f2, A, ofs, comp);
   }

   int awgNoiseComponentEx (AWG_WaveType wtype, tainsec_t t0,
                     double f1, double f2, 
                     double A, double ofs, AWG_Component* comp)
   {
      /* first test if valid */
      if (((wtype != awgNoiseN) && (wtype != awgNoiseU)) || 
         (comp == NULL)) {
         return -1;
      }
   
      /* now fill component */
      memset (comp, 0, sizeof (AWG_Component));
      comp->wtype = wtype;
      comp->par[0] = fabs (A);
      comp->par[1] = (f1 <= 0) ? 0 : f1;
      comp->par[2] = (f2 <= 0) ? 1E15 : f2;
      comp->par[3] = ofs;
      comp->start = t0;
      comp->duration = -1;
      comp->restart = -1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgStreamComponent				*/
/*                                                         		*/
/* Procedure Description: makes AWG stream components			*/
/*                                                         		*/
/* Procedure Arguments: scaling						*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgStreamComponent (double scaling, AWG_Component* comp)
   {
      return awgStreamComponentEx (TAInow(), scaling, comp);
   }

   int awgStreamComponentEx (tainsec_t t0, 
                     double scaling, AWG_Component* comp)
   {
      /* first test if valid */
      if (comp == NULL) {
         return -1;
      }
   
      /* now fill component */
      memset (comp, 0, sizeof (AWG_Component));
      comp->wtype = awgStream;
      comp->par[0] = scaling;
      comp->par[1] = 0;
      comp->par[2] = 0;
      comp->par[3] = 0;
      comp->start = t0;
      comp->duration = -1;
      comp->restart = -1;
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSOS					*/
/*                                                         		*/
/* Procedure Description: computes a second order section		*/
/*                                                         		*/
/* Procedure Arguments: input, coefficients, history			*/
/*                                                         		*/
/* Procedure Returns: filtered value					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void awgSOS (float* x, int len, double b1, double b2,
               double a1, double a2, double* h1, double* h2)
   {
      register int i;
      register double hzzero;
      register double hzm1 = *h1;
      register double hzm2 = *h2;
      register double aa1 = a1;
      register double aa2 = a2;
      register double bb1 = b1;
      register double bb2 = b2;
   
      for (i = 0; i < len; ++i) {
         /* apply single second order section */
         hzzero = (double)(*x) - hzm1 * aa1 - hzm2 * aa2;
         *x++ = (float)(hzzero + hzm1 * bb1 + hzm2 * bb2);
         /* update history buffers */
         hzm2 = hzm1;
         hzm1 = hzzero;
      }
      *h1 = hzm1;
      *h2 = hzm2;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgIsValidComponent				*/
/*                                                         		*/
/* Procedure Description: checks validity of AWG component		*/
/*                                                         		*/
/* Procedure Arguments: component					*/
/*                                                         		*/
/* Procedure Returns: 1 if valid, 0 if invalid				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgIsValidComponent (const AWG_Component* comp)
   {
      /* NULL is invalid */
      if (comp == NULL) {
         return 0;
      }
      /* awg None is valid for all values */
      if (comp->wtype == awgNone) {
         return 1;
      }
      /* test waveform type */
      switch (comp->wtype) {
         case awgSine:
         case awgSquare:
         case awgRamp:
         case awgTriangle:
            {
               /* test parameters: positive amplitude and frequency */
               if ((comp->par[0] < 0) || (comp->par[1] < 0)) {
                  return 0;
               }
               /* test new parameters: positive amplitude and frequency */ 
               if ((comp->ramppar[0] < 0) || (comp->ramppar[1] < 0)) {
                  return 0;
               }
               break;
            }
         case awgImpulse:
            {
               /* test parameters: positive frequency, duration & delay */
               if ((comp->par[1] < 0) || (comp->par[2] < 0) || 
                  (comp->par[3] < 0)) {
                  return 0;
               }
               break;
            }
         case awgConst:
            {
               break;
            }
         case awgNoiseN:
         case awgNoiseU:
            {
               /* test parameters: positive amplitude and frequencies */
               if ((comp->par[0] < 0) || (comp->par[1] < 0) ||
                  (comp->par[2] < 0)) {
                  return 0;
               }
               /* test new parameters: positive amplitude and frequency */ 
               if ((comp->ramppar[0] < 0) || (comp->ramppar[1] < 0) || 
                  (comp->ramppar[2] < 0)) {
                  return 0;
               }
               break;
            }
         case awgArb:
            {
               /* test parameters: positive frequency and rate */
               if ((comp->par[1] < 0) || (comp->par[2] < 0)) {
                  return 0;
               }
               /* test parameters: triggered type */
               if (((int) (comp->par[3] + 0.5) < 0) ||
                  ((int) (comp->par[3] + 0.5) > 2)) {
                  return 0;
               }
               break;
            }
         case awgStream:
            {
               break;
            }
         default:
            {
               /* unsupported component */
               return 0;
            }
      }
   
      /* test time of execution and duration */
      if (comp->start < 0) {
         return 0;
      }
      /* test ramp type */
      if ((PHASE_IN_TYPE (comp->ramptype) < 0) || 
         (PHASE_IN_TYPE (comp->ramptype) > AWG_PHASING_QUADRATIC) ||
         (PHASE_OUT_TYPE (comp->ramptype) < 0) ||
         (PHASE_OUT_TYPE (comp->ramptype) > AWG_PHASING_LOG) ||
         (SWEEP_OUT_TYPE (comp->ramptype) < 0) ||
         (SWEEP_OUT_TYPE (comp->ramptype) > AWG_PHASING_LOG) ||
         (SWEEP_OUT_TYPE (comp->ramptype) == AWG_PHASING_QUADRATIC)) {
         return 0;
      }
      /* test ramp times */
      if ((comp->ramptime[0] < 0) || (comp->ramptime[1] < 0)) {
         return 0;
      }
   
      /* valid component */
      return 1;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgBinarySearch				*/
/*                                                         		*/
/* Procedure Description: seraches through a list of AWG components 	*/
/*                        to find the first comp. with later start time	*/
/*                                                         		*/
/* Procedure Arguments: comp. list, number of components, serach comp.	*/
/*                                                         		*/
/* Procedure Returns: position of 1st component with a later start time	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int awgBinarySearch (const AWG_Component complist[], int ncomp, 
                     const AWG_Component* comp)
   {
      int		jl;		/* lower index */
      int		ju;		/* upper index */
      int		jm;		/* middle index */
   
      if (ncomp <= 0) {
         return 0;
      }
   
      jl = 0;
      ju = ncomp;
      while (ju - jl > 1) {
         jm = (ju + jl) / 2;
         if (comp->start < complist[jm].start) {
            ju = jm;
         }
         else {
            jl = jm;
         }
      }
      if (comp->start < complist[jl].start) {
         return jl;
      }
      else {
         return ju;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: awgSortComponents				*/
/*                                                         		*/
/* Procedure Description: sorts a list of AWG components in order of	*/
/*                        their start time				*/
/*                                                         		*/
/* Procedure Arguments: component list, number of components		*/
/*                                                         		*/
/* Procedure Returns: 1 if valid, 0 if invalid				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   void awgSortComponents (AWG_Component comp[], int numComp)
   {
      int 		i;
      int		ir;
      int		j;
      int		l;
      AWG_Component	cp;
   
      /* see Numerical Recipes */
      if (numComp < 2) {
         return;
      }
      l = numComp >> 1;
      ir = numComp - 1;
      for (;;) {
         if (l > 0) {
            cp = comp[--l];
         }
         else {
            cp = comp[ir];
            comp[ir] = comp[0];
            if (--ir == 0) {
               comp[0] = cp;
               break;
            }
         }
         i = l;
         j = l + l + 1;
         while (j <= ir) {
            if ((j < ir) && (comp[j].start < comp[j+1].start)) {
               j++;
            }
            if (cp.start < comp[j].start) {
               comp[i] = comp[j];
               i = j;
               j = (j << 1) + 1;
            }
            else {
               j = ir + 1;
            }
         }
         comp[i] = cp;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: productLog					*/
/*                                                         		*/
/* Procedure Description: caluclates the inverse of z(w)=w*E^w		*/
/*                                                         		*/
/* Procedure Arguments: z						*/
/*                                                         		*/
/* Procedure Returns: w							*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   double productLog (double z)
   {
      double 	w;		/* function value */
      double 	dw;		/* iteration correction */
      int	i;		/* iteration count */
   
      /* determine start value */
      if (z <= 0) {
         return 0.0;
      }
      else if (z < 0.2) {
         w = z;
      }
      else if (z < 3.0) {
         w = sqrt (z) / 2;
      }
      else if (z <= 1E3) {
         w = log (z) / 1.33;
      }
      else if (z <= 1E6) {
         w = log (z) / 1.22;
      }
      else {
         w = log (z) / 1.17;
      }
   
      /* use Newton's method to find function value */
      for (i = 0; i < 10; i++) {
         dw = (w - z * exp (-w)) / (1 + w);
         if (fabs (dw) < 1E-8 * w) {
            return w - dw;
         }
         w -= dw;
      }
      /* more than 10 iterations! */
      return w;
   }

