/*----------------------------------------------------------------------------- */
/*                                                                              */
/*                      -------------------                                     */
/*                                                                              */
/*                             LIGO                                             */
/*                                                                              */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.              */
/*                                                                              */
/*                     (C) The LIGO Project, 2005.                              */
/*                                                                              */
/*                                                                              */
/* File: fm10Gen.c 	                                                        */
/* Description:                                                                 */
/*      CDS generic code for calculating IIR filters.			        */
/*                                                                              */
/* California Institute of Technology                                           */
/* LIGO Project MS 18-34                                                        */
/* Pasadena CA 91125                                                            */
/*                                                                              */
/* Massachusetts Institute of Technology                                        */
/* LIGO Project MS 20B-145                                                      */
/* Cambridge MA 01239                                                           */
/*                                                                              */
/*----------------------------------------------------------------------------- */



#include "fm10Gen.h"
static const char *fm10Gen_cvsid = "$Id: fm10Gen.c,v 1.23 2009/09/17 18:56:23 aivanov Exp $";

inline double filterModule(FILT_MOD *pFilt, COEF *pC, int modNum, double inModOut);
inline double inputModule(FILT_MOD *pFilt, int modNum);
inline UINT32 handleEpicsSwitch(UINT32 sw, int modNum, FILT_MOD *dsp, COEF *dspCoeff);

/* Switching register bits */
#define OPSWITCH_INPUT_ENABLE 0x4
#define OPSWITCH_OFFSET_ENABLE 0x8
#define OPSWITCH_LIMITER_ENABLE 0x1000000
#define OPSWITCH_DECIMATE_ENABLE 0x2000000
#define OPSWITCH_OUTPUT_ENABLE 0x4000000
#define OPSWITCH_HOLD_ENABLE 0x8000000

static const UINT32 pow2_in[10] = {0x10,0x40,0x100,0x400,0x1000,0x4000,0x10000,
				   0x40000,0x100000,0x400000};
static const UINT32 pow2_out[10] = {0x20,0x80,0x200,0x800,0x2000,0x8000,0x20000,
				    0x80000,0x200000,0x800000};

static const UINT32 fltrConst[10] = {16, 64, 256, 1024, 4096, 16384,
                                     65536, 262144, 1048576, 4194304};

#if defined(SERVO16K) || defined(SERVOMIXED) || defined(SERVO32K) || defined(SERVO64K) || defined(SERVO128K) || defined(SERVO256K)
static double sixteenKAvgCoeff[9] = {1.9084759e-12,
				     -1.99708675982420, 0.99709029700517, 2.00000005830747, 1.00000000739582,
				     -1.99878510620232, 0.99879373895648, 1.99999994169253, 0.99999999260419};
#endif

#if defined(SERVO2K) || defined(SERVOMIXED) || defined(SERVO4K)
static double twoKAvgCoeff[9] = {7.705446e-9,
				 -1.97673337437048, 0.97695747524900,  2.00000006227141,  1.00000000659235,
				 -1.98984125831661,  0.99039139954634,  1.99999993772859,  0.99999999340765};
#endif

#ifdef SERVO16K
#define avgCoeff sixteenKAvgCoeff
#elif defined(SERVO32K) || defined(SERVO64K) || defined(SERVO128K) || defined(SERVO256K)
#define avgCoeff sixteenKAvgCoeff
#elif defined(SERVO2K)
#define avgCoeff twoKAvgCoeff
#elif defined(SERVO4K)
#define avgCoeff twoKAvgCoeff
#elif defined(SERVOMIXED)
#define filterModule(a,b,c,d) filterModuleRate(a,b,c,d,16384)
#elif defined(SERVO5HZ)
#else
#error need to define 2k or 16k or mixed
#endif

static FM_GAIN_RAMP gain_ramp[MAX_MODULES][10];

/**************************************************************************
iir_filter - Perform IIR filtering sample by sample on doubles

Implements cascaded direct form II second order sections.
Requires arrays for history and coefficients.
The length (n) of the filter specifies the number of sections.
The size of the history array is 2*n.
The size of the coefficient array is 4*n + 1 because
the first coefficient is the overall scale factor for the filter.
Returns one output sample for each input sample.

double iir_filter(double input,double *coef,int n,double *history)

    double input        new double input sample
    double *coef        pointer to filter coefficients
    int n              number of sections in filter
    double *history     history array pointer

Returns double value giving the current output.

*************************************************************************/

double junk;

inline double iir_filter(double input,double *coef,int n,double *history){

  int i;
  double *coef_ptr;
  double *hist1_ptr,*hist2_ptr;
  double output,new_hist,history1,history2;

  coef_ptr = coef;                /* coefficient pointer */
  
  hist1_ptr = history;            /* first history */
  hist2_ptr = hist1_ptr + 1;      /* next history */
  
  input += 1e-16;
  junk = input;
  input -= 1e-16;

  output = input * (*coef_ptr++); /* overall input scale factor */
  
  for(i = 0 ; i < n ; i++) {
    
    history1 = *hist1_ptr;        /* history values */
    history2 = *hist2_ptr;
    
    
    output = output - history1 * (*coef_ptr++);
    new_hist = output - history2 * (*coef_ptr++);    /* poles */
    
    output = new_hist + history1 * (*coef_ptr++);
    output = output + history2 * (*coef_ptr++);      /* zeros */

    if((new_hist < 1e-20) && (new_hist > -1e-20)) new_hist = new_hist<0 ? -1e-20: 1e-20;
    
    new_hist += 1e-16;
    junk = new_hist;
    new_hist -= 1e-16;

    *hist2_ptr++ = *hist1_ptr;
    *hist1_ptr++ = new_hist;
    hist1_ptr++;
    hist2_ptr++;
    
  }
  
  return(output);
}

/* Biquad form IIR */
inline double iir_filter_biquad(double input,double *coef,int n,double *history){

  int i;
  double *coef_ptr;
  double *hist1_ptr,*hist2_ptr;
  double output,new_w, new_u, w, u, a11, a12, c1, c2;

  coef_ptr = coef;                /* coefficient pointer */
  
  hist1_ptr = history;            /* first history */
  hist2_ptr = hist1_ptr + 1;      /* next history */
  
  input += 1e-16;
  junk = input;
  input -= 1e-16;

  output = input * (*coef_ptr++); /* overall input scale factor */
  
  for(i = 0 ; i < n ; i++) {
    
    w = *hist1_ptr; 
    u = *hist2_ptr;
    
    a11 = *coef_ptr++;
    a12 = *coef_ptr++;
    c1  = *coef_ptr++;
    c2  = *coef_ptr++;
    
    new_w = output + a11 * w + a12 * u;
    output = output + w * c1 + u * c2;
    new_u = w + u;   

    //if((new_hist < 1e-20) && (new_hist > -1e-20)) new_hist = new_hist<0 ? -1e-20: 1e-20;
    
    new_w += 1e-16;
    junk = new_w;
    new_w -= 1e-16;
    new_u += 1e-16;
    junk = new_u;
    new_u -= 1e-16;

    *hist1_ptr++ = new_w;
    *hist2_ptr++ = new_u;
    hist1_ptr++;
    hist2_ptr++;
    
  }
  
  return(output);
}

#ifdef FIR_FILTERS
/**************************************************************************

fir_filter - Perform fir filtering sample by sample on floats

Requires array of filter coefficients and pointer to history.
Returns one output sample for each input sample.

float fir_filter(float input,float *coef,int n,float *history)

    double input        new float input sample
    double *coef        pointer to filter coefficients
    int n               number of coefficients in filter
    double *history     history array pointer

Returns float value giving the current output.

*************************************************************************/
inline double fir_filter(double input, double *coef, int n, double *history)
{
    int i;
    double *hist_ptr,*hist1_ptr,*coef_ptr;
    double output;

    hist_ptr = history;
    hist1_ptr = hist_ptr;             /* use for history update */
    coef_ptr = coef + n - 1;          /* point to last coef */

/* form output accumulation */
    output = *hist_ptr++ * (*coef_ptr--);
    for(i = 2 ; i < n ; i++) {
        *hist1_ptr++ = *hist_ptr;            /* update history array */
        output += (*hist_ptr++) * (*coef_ptr--);
    }
    output += input * (*coef_ptr);           /* input tap */
    *hist1_ptr = input;                      /* last history */

    return(output);
}
#endif



/**************************************************************************
filterModule - Perform filtering for a string of up to 10 filters

Decodes Operator Switches.
Filters sections according to specified filter type (1-3).
Outputs answer to the output module function.
Sets readback filter bit for EPICS display screen.
*************************************************************************/

#ifdef SERVOMIXED
inline double
filterModuleRate(FILT_MOD *pFilt,     /* Filter module data  */
		 COEF *pC,            /* Filter coefficients */
		 int modNum,          /* Filter module number */
		 double filterInput,  /* Input data sample (output from funtcion inputModule()) */
		 unsigned int rate)   /* Rate in Hz at which this modules gets calculated */
#else
#ifdef SERVO16K
static const int rate = 16384;
#elif defined(SERVO5HZ)
static const int rate = 5;
#elif defined(SERVO2K)
static const int rate = 2048;
#elif defined(SERVO4K)
static const int rate = 4096;
#elif defined(SERVO32K)
static const int rate = 32768;
#elif defined(SERVO64K)
static const int rate = (2*32768);
#elif defined(SERVO128K)
static const int rate = (4*32768);
#elif defined(SERVO256K)
static const int rate = (8*32768);
#endif
	     
inline double
filterModuleId(FILT_MOD *pFilt,     /* Filter module data  */
	       COEF *pC,            /* Filter coefficients */
	       int modNum,          /* Filter module number */
	       double filterInput,  /* Input data sample (output from funtcion inputModule()) */
	       int id)		    /* System number (HEPI) */
	     
#endif
{
  /* decode arrays for operator switches */
  UINT32 opSwitchE = pFilt->inputs[modNum].opSwitchE;
  /* Do the shift to match the bits in the the opSwitchE variable so I can do "==" comparisons */
  UINT32 opSwitchP = pFilt->inputs[modNum].opSwitchP >> 1;
  int ii, jj, kk, ramp, timeout;
  int sw, sw_out, sType, sw_in;
  double filtData;
  float avg, compare;
  double output;

  /* Apply Filtering */

  /* Loop through all filters */
  for (ii=0; ii<FILTERS; ii++) {
    /* Do not do anything for any filter with zero filter sections */
    if (!pC->coeffs[modNum].filtSections[ii]) continue;

    sw = opSwitchE & pow2_in[ii];       /* Epics screen filter on/off request bit */
    sw_out = opSwitchP & pow2_in[ii];   /* Pentium output ack bit (opSwitchP was right shifted by 1) */

    /* Filter switching type */
    sType = pC->coeffs[modNum].sType[ii];

    /* If sType is type 1X, the input will always go to filter to be calculated */
    /* If sType is type 2X, then the input will be zero if output is turned off */
    /* If sType is type 22, then the input will go to filter if filter is turning on (ramping up)*/
    sw_in = sType < 20 || sw_out || (sType == 22 && sw);

    /* Calculate filter */
    filtData = iir_filter(sw_in?filterInput:1e-16,
			  pC->coeffs[modNum].filtCoeff[ii],
			  pC->coeffs[modNum].filtSections[ii],
			  pC->coeffs[modNum].filtHist[ii]);

    if (sw == sw_out) { /* No switching */
      if (sw) filterInput = filtData; /* Use the filtered value if the filter is turned on */
    } else {  /* Switching request */
      int sTypeMod10 = sType%10;
      /* Process filter switching according to output type [1-3] */
      switch (sTypeMod10) {
      case 1: /* Instantenious switch */
	/* Turn output on/off according to the request */
	if ((sw_out = sw)) filterInput = filtData; /* Use the filtered value if the filter is turned on */
	break;
      case 2: /* Ramp in/out filter output */
	ramp = pFilt->inputs[modNum].rmpcmp[ii];   /* Ramp slope coefficient */
	kk = pFilt->inputs[modNum].cnt[ii];        /* Ramp count */
	/* printf("kk=%d; ramp=%d; sw=%d; sw_out=%d\n", kk, ramp, sw, sw_out); */

	if (kk == ramp) {                          /* Done ramping */
	  /* Turn output on/off according to the request */
	  if ((sw_out = sw)) filterInput = filtData; /* Use the filtered value if the filter is turned on */
	  kk = 0;
	} else {                                   /* Ramping will be done */
	  if (kk) {                                /* Currently ramping */
	    double t = (double)kk / (double)ramp;  /* Slope */
	    if (sw)                                /* Turn on request */
	      filterInput = (1.0 - t) * filterInput + t * filtData;
	    else                                   /* Turn off request */
	      filterInput = t * filterInput + (1.0 - t) * filtData;
	  } else {                                 /* Start to ramp */
	    if (sw)                                /* Turn on request */
	      ;                                    /* At the start of turning on ramp input goes to output (filter bypassed) */
	    else                                   /* Turn off request */
	      filterInput = filtData;	           /* At the start of turning off the ramp is at filter output */
	  }
	  kk++;
	}
	pFilt->inputs[modNum].cnt[ii] = kk;
	break;
      case 3: /* Comparator */
      case 4: /* Zero crossing */
	ramp = pFilt->inputs[modNum].rmpcmp[ii];  /* Filter comparison range */
	timeout = pFilt->inputs[modNum].timeout[ii]; 	/* Comparison timeout number */
	kk = pFilt->inputs[modNum].cnt[ii];   /* comparison count */
	if (sTypeMod10 == 3) compare = filterInput - filtData; /* Comparator looks at the filter in/out diff */
	else compare = sw_out? filtData: filterInput; /* Use the filtered value if waiting to switch off */
	if (compare < .0) compare = -compare;

	++kk;
	if (kk >= timeout || compare <= (float)ramp) { /* If timed out or the difference is in the range */
	  /* Turn output on/off according to the request */
	  if ((sw_out = sw)) filterInput = filtData; /* Use the filtered value if the filter is turned on */
	  kk = 0;
	} else { /* Waiting for the match or the timeout */
	  if (sw_out) filterInput = filtData; /* Use the filtered value if waiting to switch off */
	}
	pFilt->inputs[modNum].cnt[ii] = kk;
	break;
      default:
	filterInput = 777;
	break;
      }
    
      if (sw == sw_out) { /* If turning the filter on/off NOW */
	/* Clear history buffer if filter is turned off NOW */
	/* History is cleared one time only */
	if (!sw) { /* Turn off request */
	  for(jj=0;jj<MAX_HISTRY;jj++)
	    pC->coeffs[modNum].filtHist[ii][jj] = 0.0;

	}

	/* Send back the readback switches when there is change */
	if (sw_out) { /* switch is on, turn it on */
	  pFilt->inputs[modNum].opSwitchP |= pow2_out[ii];
	} else { /* switch is off, turn it off */
	  pFilt->inputs[modNum].opSwitchP &= ~pow2_out[ii];
	}
      }
    }
  }

  /* Calculate output values */
  {
    double cur_gain;


    if (pFilt->inputs[modNum].outgain != gain_ramp[modNum][id].prev_outgain) {
      /* Abort running ramping if user inputs 0.0 seconds ramp time */
      if (pFilt->inputs[modNum].gain_ramp_time == 0.0) {
	gain_ramp[modNum][id].ramp_cycles_left
	  = gain_ramp[modNum][id].gain_ramp_cycles = 0;
      }
      
      switch (gain_ramp[modNum][id].ramp_cycles_left) {
	case 0: /* Initiate gain ramping now */
	  {
	    if (gain_ramp[modNum][id].gain_ramp_cycles == 0) {
	      /* Calculate how many steps we will be doing */
	      gain_ramp[modNum][id].ramp_cycles_left
	    	= gain_ramp[modNum][id].gain_ramp_cycles 
	    	= rate * pFilt->inputs[modNum].gain_ramp_time;

	      /* Send ramp indication bit to Epics */
	      pFilt->inputs[modNum].opSwitchP |= 0x10000000;
	   
	      if (gain_ramp[modNum][id].ramp_cycles_left < 1)  {
		  gain_ramp[modNum][id].ramp_cycles_left
		    = gain_ramp[modNum][id].gain_ramp_cycles
		    = 1;
	      }
	    }
  	    /* Fall through to default */
	  }
	default:/* Gain ramping is in progress */
	  {
	    --gain_ramp[modNum][id].ramp_cycles_left;

	    cur_gain = ((double) gain_ramp[modNum][id].prev_outgain) +
		((((double) pFilt->inputs[modNum].outgain)
		  - ((double) gain_ramp[modNum][id].prev_outgain))
		 * (1.0 - (((double) gain_ramp[modNum][id].ramp_cycles_left)
			   / ((double) gain_ramp[modNum][id].gain_ramp_cycles)))
		);

	    if ((pFilt->inputs[modNum].outgain
		 < gain_ramp[modNum][id].prev_outgain)
		? cur_gain <= pFilt->inputs[modNum].outgain
		: cur_gain >= pFilt->inputs[modNum].outgain) {

		/* Stop ramping now, we are done */
		cur_gain 
		 = gain_ramp[modNum][id].prev_outgain
		 = pFilt->inputs[modNum].outgain;
	        gain_ramp[modNum][id].ramp_cycles_left
	         = gain_ramp[modNum][id].gain_ramp_cycles
		 = 0;

	        /* Clear ramping flag bit for Epics */
	        pFilt->inputs[modNum].opSwitchP &= ~0x10000000;
	    }
	    break;
	  }
      }
    } else {
	if (gain_ramp[modNum][id].ramp_cycles_left
	    || gain_ramp[modNum][id].gain_ramp_cycles) {
	  /* Correct for any floating point arithmetic mistakes */
          gain_ramp[modNum][id].ramp_cycles_left
            = gain_ramp[modNum][id].gain_ramp_cycles
	    = 0;
          /* Clear ramping flag bit for Epics */
          pFilt->inputs[modNum].opSwitchP &= ~0x10000000;
	}
	cur_gain = pFilt->inputs[modNum].outgain;
    }
    output = filterInput * cur_gain;

    /* Limiting */
    if (opSwitchE &  OPSWITCH_LIMITER_ENABLE) {
      if(output > pFilt->inputs[modNum].limiter)
	output = pFilt->inputs[modNum].limiter;
      else if(output < -pFilt->inputs[modNum].limiter)
	output = -pFilt->inputs[modNum].limiter;
    }

    /* Set Test Point */
    pFilt->data[modNum].testpoint = output;

#if defined(SERVO5HZ)
    avg = output;
#else
    /* Do the Decimation regardless of whether it is enabled or disabled */
    avg = iir_filter(output,
#if defined(SERVOMIXED)
		     rate==16384? sixteenKAvgCoeff: twoKAvgCoeff,
#else
		     avgCoeff,
#endif
		     2,
		     pC->coeffs[modNum].decHist);
#endif

    /* Test Output Switch and output hold on/off */
    if (opSwitchE & OPSWITCH_HOLD_ENABLE) {
      ; /* Outputs are not assigned, hence they are held */
    } else {
      if (opSwitchE & OPSWITCH_OUTPUT_ENABLE) {
	pFilt->data[modNum].output = output;

	/* Decimation */
	if (opSwitchE & OPSWITCH_DECIMATE_ENABLE) 
	  pFilt->data[modNum].output16Hz = avg;
	else
	  pFilt->data[modNum].output16Hz = output;
      } else {
        pFilt->data[modNum].output = 0;
        pFilt->data[modNum].output16Hz = 0;
      }
    }
  }
  return pFilt->data[modNum].output;
}

inline double
filterModule(FILT_MOD *pFilt, COEF *pC, int modNum, double filterInput) {
  return filterModuleId(pFilt, pC, modNum, filterInput, 0);
}

/**************************************************************************
inputModule - Performs setup calculations for the filterModule function

Decodes Operator Switches - calculates the input to the filterModule stage

double inputModule(FILT_MOD *pFilt, int modNum)                    

    FILT_MOD *pFilt     pointer to the complete filter module structure
    int modNum          the filter module number we are working with (<30)

Returns double value giving the filter bank input value.

:TODO: integrate this whole function into filterModule().Check code speed.

*************************************************************************/
inline double inputModule(FILT_MOD *pFilt, int modNum){  

  double output = (double)pFilt->data[modNum].exciteInput;
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_INPUT_ENABLE)
    output += pFilt->data[modNum].filterInput;
  pFilt->data[modNum].inputTestpoint = output;
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_OFFSET_ENABLE)
    output += pFilt->inputs[modNum].offset;
  return output<-1e-16?output:(output<1e-16?1e-16:output);
}


/**************************************************************************
  Handle double-switching, i.e. when ramping or comparator type
  of switching is in progress, handle operator inputs.

  This function needs to be called when new Epics operator settings are 
  received from the Epics and assigned into the FILT_MOD structures. It should not
  be called in the time-critical ADC readout section of the code.

  The following example for filter module 'ii' shows new opSwitchE
  word received from the VME (via dspVme pointer) and the local copy
  of the opSwitchE then assigned:

  dsp.inputs[ii].opSwitchE = handleEpicsSwitch(dspVme->inputs[ii].opSwitchE, ii, &dsp, &dspCoeff);

*************************************************************************/

/* Build with double-switching handled by reversing the switching action */
#define SWITCHING_STYLE_2

inline UINT32
handleEpicsSwitch(UINT32 sw,       /* New input values for the opSwitchE (from the Epics) */
		  int modNum,      /* Filter module number */
		  FILT_MOD *dsp,
		  COEF *dspCoeff)
{

#ifdef SWITCHING_STYLE_1

    /* Allows switching only if none of it is going on already */

    /* 0x555550 is the mask for the input filter switching bits
       (one bit for every filter in the bank of ten).
       Initiated switching disables any other switching in any if the filters in the bank.
    */
    if ((dsp->inputs[modNum].opSwitchP >> 1 & 0x555550) /* Output switching bits */
	== (dsp->inputs[modNum].opSwitchE & 0x555550))  /* Input switching bits  */
      return sw;
    else
      return dsp->inputs[modNum].opSwitchE;

#endif

#ifdef SWITCHING_STYLE_2

    /* Reverse currently active switchers */

    int jj;
    UINT32 switchers;

    /* Select current switchers */
    switchers = ((dsp->inputs[modNum].opSwitchP >> 1 & 0x555550)
		 ^ (dsp->inputs[modNum].opSwitchE & 0x555550));
    if (switchers) {
      /* Fix all conflicting switchers */
      for (jj = 0; jj < FILTERS; jj++) {
	if (switchers & pow2_in[jj]
	    && ((sw & pow2_in[jj])
		!= (dsp->inputs[modNum].opSwitchE & pow2_in[jj]))) {
	  switch (dspCoeff->coeffs[modNum].sType[jj]%10) {
	  case 2: /* Ramping */
	    /* Adjust the counter */
	    dsp->inputs[modNum].cnt[jj] =
	      dsp->inputs[modNum].rmpcmp[jj]
	      - dsp->inputs[modNum].cnt[jj];
	    /* Switch ramping direction */
	    dsp->inputs[modNum].opSwitchP ^= pow2_out[jj];
	    break;
	  case 3: /* Comparator */
	  case 4: /* Zero-crossing comparator */
	    dsp->inputs[modNum].cnt[jj] = 0; /* Reset comparator timeout counter */
	    break;
	  default:
	    break;
	  }
	}
      }
    }

    return sw;    
#endif
}


/************************************************************************/
/* Read all filter coeffs via VME backplane from EPICS CPU		*/
/************************************************************************/
inline int readCoefVme(COEF *filtC,FILT_MOD *fmt, int bF, int sF, volatile VME_COEF *pRfmCoeff)
{
  int ii,jj,kk;

#ifdef FIR_FILTERS
  int l;
  for(jj = 0; jj < MAX_FIR_MODULES; jj++)
	for (kk = 0; kk < FILTERS; kk++) 
	     for (l = 0; l < MAX_FIR_COEFFS; l++)
	        filtC->firFiltCoeff[jj][kk][l] = pRfmCoeff->firFiltCoeff[jj][kk][l];
#endif
  for(ii=bF;ii<sF;ii++)
  {
    filtC->coeffs[ii].biquad = pRfmCoeff->vmeCoeffs[ii].biquad;
    if(filtC->coeffs[ii].biquad) printf("Found a BQF filter %d\n",ii);
    for(jj=0;jj<FILTERS;jj++)
    {
      if(pRfmCoeff->vmeCoeffs[ii].filtSections[jj])
      {
        filtC->coeffs[ii].filterType[jj] = pRfmCoeff->vmeCoeffs[ii].filterType[jj];
#ifdef FIR_FILTERS
	if (filtC->coeffs[ii].filterType[jj] < 0 || filtC->coeffs[ii].filterType[jj] > MAX_FIR_MODULES) {
		filtC->coeffs[ii].filterType[jj] = 0;
		printf("Corrupted data coming from Epics: module=%d filter=%d filterType=%d\n", 
			ii, jj, pRfmCoeff->vmeCoeffs[ii].filterType[jj]);
		return 1;
	}
#endif
	filtC->coeffs[ii].filtSections[jj] =pRfmCoeff->vmeCoeffs[ii].filtSections[jj];
	filtC->coeffs[ii].sType[jj] = pRfmCoeff->vmeCoeffs[ii].sType[jj];
      	fmt->inputs[ii].rmpcmp[jj] = pRfmCoeff->vmeCoeffs[ii].ramp[jj];
      	fmt->inputs[ii].timeout[jj] = pRfmCoeff->vmeCoeffs[ii].timout[jj];

	if (filtC->coeffs[ii].filterType[jj] == 0) {
	  if (filtC->coeffs[ii].filtSections[jj] > 10) {
		printf("Corrupted Epics data:  module=%d filter=%d filterType=%d filtSections=%d\n",
			ii, jj, pRfmCoeff->vmeCoeffs[ii].filterType[jj],
			filtC->coeffs[ii].filtSections[jj]);
		return 1;
	  }
          for(kk=0;kk<filtC->coeffs[ii].filtSections[jj]*4+1;kk++) {
	    filtC->coeffs[ii].filtCoeff[jj][kk] = pRfmCoeff->vmeCoeffs[ii].filtCoeff[jj][kk];
	  }
	}
#if 0
	{
	printf("**********************************************\n");
        printf("Bank %d Filter %d has %d sections with ramp = %d and timeout %d\n",
                ii,jj,filtC->coeffs[ii].filtSections[jj],fmt->inputs[ii].rmpcmp[jj],
		fmt->inputs[ii].timeout[jj]);
#if -
        printf("Coeffs are:\n%e %e %e %e %e\n",
                filtC->coeffs[ii].filtCoeff[jj][0],filtC->coeffs[ii].filtCoeff[jj][1],
		filtC->coeffs[ii].filtCoeff[jj][2],filtC->coeffs[ii].filtCoeff[jj][3],
		filtC->coeffs[ii].filtCoeff[jj][4]);
	if(filtC->coeffs[ii].filtSections[jj] > 1)
		printf("\t%e %e %e %e \n",
                filtC->coeffs[ii].filtCoeff[jj][5],filtC->coeffs[ii].filtCoeff[jj][6],
		filtC->coeffs[ii].filtCoeff[jj][7],filtC->coeffs[ii].filtCoeff[jj][8]);
	if(filtC->coeffs[ii].filtSections[jj] > 2)
		printf("\t%e %e %e %e \n",
                filtC->coeffs[ii].filtCoeff[jj][9],filtC->coeffs[ii].filtCoeff[jj][10],
		filtC->coeffs[ii].filtCoeff[jj][11],filtC->coeffs[ii].filtCoeff[jj][12]);
	if(filtC->coeffs[ii].filtSections[jj] > 3)
		printf("\t%e %e %e %e \n",
                filtC->coeffs[ii].filtCoeff[jj][13],filtC->coeffs[ii].filtCoeff[jj][14],
		filtC->coeffs[ii].filtCoeff[jj][15],filtC->coeffs[ii].filtCoeff[jj][16]);
#endif
	}
#endif
      }
    }
  }
  return 0;
}


/**************************************************/
/* Read in new coeffs while running
	- One filter bank at a time.
	- One SOS at a time.			  */
/**************************************************/
inline int readCoefVme2(COEF *filtC,FILT_MOD *fmt, int modNum1, int filtNum, int cycle, volatile VME_COEF *pRfmCoeff, int *changed)
{
  unsigned int ii, kk, jj;
#ifdef FIR_FILTERS
  unsigned int hh;
#endif
  double temp;
  static VME_FM_OP_COEF localCoeff;


#ifdef FIR_FILTERS
# define MAX_UPDATE_CYCLE (MAX_FIR_SO_SECTIONS+1)
  static double localFirFiltCoeff[FILTERS][MAX_FIR_COEFFS];
#else
# define MAX_UPDATE_CYCLE (MAX_SO_SECTIONS+1)
#endif

  int type = pRfmCoeff->vmeCoeffs[modNum1].filterType[filtNum];

#ifdef FIR_FILTERS
  if (type < 0 || type > MAX_FIR_MODULES) {
	printf("Vme2 bad Epics filter type: module=%d filter=%d filterType=%d\n", 
	modNum1, filtNum, type);
  }
#endif

  // printf("readCoefVme2: module=%d filter=%d filterType=%d\n", modNum1, filtNum, type);

  ii = 0;
  if (cycle == 0) {
	for (ii = 0; ii < 10; ii++) changed[ii] = 0;
  	ii = pRfmCoeff->vmeCoeffs[modNum1].filtSections[filtNum];
	if (filtNum == 0) localCoeff.crc = 0;
	localCoeff.crc = crc_ptr((char *)&ii, sizeof(int), localCoeff.crc);
	if (((ii>0) && (ii<11)) || ((ii>10) && (type>0)))
	{
	  //printf("vme2: module=%d filter=%d type=%d\n", modNum1, filtNum, type);
    	  localCoeff.filtSections[filtNum] = ii;
    	  localCoeff.filterType[filtNum] = type;
	  localCoeff.sType[filtNum] = pRfmCoeff->vmeCoeffs[modNum1].sType[filtNum];
	  localCoeff.ramp[filtNum] =  pRfmCoeff->vmeCoeffs[modNum1].ramp[filtNum];
	  localCoeff.timout[filtNum] = pRfmCoeff->vmeCoeffs[modNum1].timout[filtNum];
	  localCoeff.crc = crc_ptr ((char *)&(localCoeff.sType[filtNum]), sizeof(int), localCoeff.crc);
	  localCoeff.crc = crc_ptr ((char *)&(localCoeff.ramp[filtNum]), sizeof(int), localCoeff.crc);
	  localCoeff.crc = crc_ptr ((char *)&(localCoeff.timout[filtNum]), sizeof(int), localCoeff.crc);
	  ii = 1;
	}
  	else 
	{
	  //printf("vme2:off  module=%d filter=%d type=%d\n", modNum1, filtNum, type);
	  /* Turn filter status readback off */
	  fmt->inputs[modNum1].opSwitchP &= ~pow2_out[filtNum];
	  filtC->coeffs[modNum1].filtSections[filtNum] = 0;
	  filtC->coeffs[modNum1].filterType[filtNum] = 0;
	  for (ii = 0; ii < 10; ii++) changed[ii] = 1;
	  localCoeff.filtSections[filtNum] = 0;
	  ii = MAX_UPDATE_CYCLE;
	}
    } else if (cycle > 0 && cycle < MAX_UPDATE_CYCLE ) {
      if (cycle == 1) {
	if (type > 0) {
#ifdef FIR_FILTERS
	  /* FIR filter */
	  temp = pRfmCoeff->firFiltCoeff[type-1][filtNum][0];
	  if (filtC->firFiltCoeff[type-1][filtNum][0] != temp) changed[filtNum]++;
	  localFirFiltCoeff[filtNum][0] = temp;
          localCoeff.crc = crc_ptr ((char *)&temp, sizeof(double), localCoeff.crc);
	  //printf("gain = %f\n", temp);
#endif
	} else {
	  /* Assign filter gain value */
	  temp = pRfmCoeff->vmeCoeffs[modNum1].filtCoeff[filtNum][0];
	  if (filtC->coeffs[modNum1].filtCoeff[filtNum][0] != temp) changed[filtNum]++;
	  //if (localCoeff.filtCoeff[filtNum][0] != temp) changed[filtNum]++;
	  localCoeff.filtCoeff[filtNum][0] = temp;
          localCoeff.crc = crc_ptr ((char *)&(localCoeff.filtCoeff[filtNum][0]), sizeof(double), localCoeff.crc);
	}
      }
      {
	/* Assign second-order sections */
        int to = cycle * 4 + 1;
        for (kk = to - 4; kk < to; kk++) {
	  if (type > 0) {
#ifdef FIR_FILTERS
	    /* FIR filter */
	    temp = pRfmCoeff->firFiltCoeff[type-1][filtNum][kk];
	    if (filtC->firFiltCoeff[type-1][filtNum][kk] != temp) changed[filtNum]++;
	    localFirFiltCoeff[filtNum][kk] = temp;
            localCoeff.crc = crc_ptr ((char *)&temp, sizeof(double), localCoeff.crc);
	    //printf("%f ", temp);
#endif
	  } else {
	      temp = pRfmCoeff->vmeCoeffs[modNum1].filtCoeff[filtNum][kk];
	      if (filtC->coeffs[modNum1].filtCoeff[filtNum][kk] != temp) changed[filtNum]++;
	      //if (localCoeff.filtCoeff[filtNum][kk] != temp) changed[filtNum]++;
	      localCoeff.filtCoeff[filtNum][kk] = temp;
              localCoeff.crc = crc_ptr ((char *)&(localCoeff.filtCoeff[filtNum][kk]), sizeof(double), localCoeff.crc);
	  }
	}
	//if (type > 0) printf("\n");
	if (localCoeff.filtSections[filtNum] > cycle) ii = cycle + 1;
	else ii = MAX_UPDATE_CYCLE;
      }
    } else if (cycle == MAX_UPDATE_CYCLE) {
	/* Make sure all numbers check out OK */
	if (filtNum == 9) { /* Last filter loaded in this filter bank */
	    unsigned int vme_crc =  pRfmCoeff->vmeCoeffs[modNum1].crc;
	    if (localCoeff.crc != vme_crc) {
	      printf("vme_crc = 0x%x; local crc = 0x%x\n", vme_crc, localCoeff.crc);
	      // return -1;
	    }
	}
	if(localCoeff.filtSections[filtNum])
	{

	  /* Reset filter history only if its coefficients modified */
	  for(jj=0;jj<FILTERS;jj++) {
	    if (changed[jj]) {
	      if (type) { /* FIR filter */
#ifdef FIR_FILTERS
        	for (kk = 0; kk < FIR_POLYPHASE_SIZE; kk++)
          	  for (hh = 0; hh < FIR_TAPS; hh++)
	            filtC->firHistory[type-1][jj][kk][hh] = 0.0;
#endif
	      } else {
	        for(kk=0;kk<MAX_HISTRY;kk++)
	          filtC->coeffs[modNum1].filtHist[jj][kk] = 0.0;
	      }
	    }
	  }

	  /* Do not clear decimation history when filters are reloaded */
#if 0
	  /* Clear decimation history */
	  for(jj = 0; jj < 8; jj++)
	    filtC->coeffs[modNum1].decHist[jj] = 0;
#endif

    	  filtC->coeffs[modNum1].filtSections[filtNum] = localCoeff.filtSections[filtNum];
    	  filtC->coeffs[modNum1].filterType[filtNum] = localCoeff.filterType[filtNum];
	  filtC->coeffs[modNum1].sType[filtNum] = localCoeff.sType[filtNum];
	  fmt->inputs[modNum1].rmpcmp[filtNum] =  localCoeff.ramp[filtNum];
	  fmt->inputs[modNum1].timeout[filtNum] = localCoeff.timout[filtNum];
	  if (type > 0) {
#ifdef FIR_FILTERS
	    for(kk=0;kk<filtC->coeffs[modNum1].filtSections[filtNum]*4+1;kk++)
	      filtC->firFiltCoeff[type-1][filtNum][kk] = localFirFiltCoeff[filtNum][kk];
#endif
	  } else {
    	    // filtC->biquad = localCoeff.biquad;
	    for(kk=0;kk<filtC->coeffs[modNum1].filtSections[filtNum]*4+1;kk++)
	      filtC->coeffs[modNum1].filtCoeff[filtNum][kk] = localCoeff.filtCoeff[filtNum][kk];
	  }
	}
	ii = 0;
    } else {
	ii = 0;
    }
  return(ii);
#undef MAX_UPDATE_CYCLE
}

/* Filter module update globals */
struct filtResetId {
  int fmResetCoeff;
  int fmResetCounter;
  int fmSubCounter;
  FILT_MOD *fmResetDsp;
  int changed[FILTERS];
} filtResetId[10] = {
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {0,0,0,0,{0,0,0,0,0,0,0,0,0,0}}
};

/***************************************/
/* Check for history resets or new coeffs for filter banks */
/***************************************/
inline void checkFiltResetId(int bankNum, FILT_MOD *pL, volatile FILT_MOD *dspVme, COEF *pC, int totMod, volatile VME_COEF *pRfmCoeff, int id)
{
  int jj,kk;
#ifdef FIR_FILTERS
  int hh;
#endif
  int status;

  if (id < 0 || id > 9) return;

  if (filtResetId[id].fmResetCoeff) {
    /* Coeff reset in progress */
    if (filtResetId[id].fmResetDsp == pL) { /* Coeff reset is done for this subsystem */
      /* printf("Coeff reset done\n"); */
      filtResetId[id].fmSubCounter = readCoefVme2(pC, pL, (filtResetId[id].fmResetCoeff - 1), filtResetId[id].fmResetCounter, filtResetId[id].fmSubCounter, pRfmCoeff, filtResetId[id].changed);
      if (filtResetId[id].fmSubCounter == -1) {
	dspVme->coef_load_error = -1;
	filtResetId[id].fmResetCoeff = 0;
	filtResetId[id].fmResetCounter = 0;
      } else if (filtResetId[id].fmSubCounter == 0) {
	filtResetId[id].fmResetCounter ++;
	if (filtResetId[id].fmResetCounter >= FILTERS) {
	  filtResetId[id].fmResetCounter = 0;
	  dspVme->coef_load_error = filtResetId[id].fmResetCoeff;
	  /* printf("dspVme->coef_load_error = %d\n", dspVme->coef_load_error ); */
	  filtResetId[id].fmResetCoeff = 0;
	}
      }
    } else /* Do nothing if this is a call for another subsystem */
      ;
  } else if (bankNum >=0 && bankNum < totMod) {
    /* Check for Reset History request */
    status = (dspVme->inputs[bankNum].rset & 0x3);
    if(status)
      {
	/* Reset the flag */
	dspVme->inputs[bankNum].rset = 0;
      }

    /* Filter history reset request */
    if(status & 2)
      {
	/* Clear out filter bank histories */
	for(jj=0;jj<FILTERS;jj++) {
#ifdef FIR_FILTERS
	  int type = pC->coeffs[bankNum].filterType[jj];
	  if (type > 0) {
        	for (kk = 0; kk < FIR_POLYPHASE_SIZE; kk++)
          	  for (hh = 0; hh < FIR_TAPS; hh++)
	            pC->firHistory[type-1][jj][kk][hh] = 0.0;
	  } else
#endif
	  for(kk=0;kk<MAX_HISTRY;kk++)
	    pC->coeffs[bankNum].filtHist[jj][kk] = 0.0;
 	}

	/* Clear decimation history */
	for(jj = 0; jj < 8; jj++)
	  pC->coeffs[bankNum].decHist[jj] = 0;
      }

    /* Check if new coeffs */
    if (status & 1) {
      /* printf("New coeff request; bank=%d \n", bankNum); */
      filtResetId[id].fmResetCoeff = bankNum + 1;
      filtResetId[id].fmResetDsp = pL;
    } else filtResetId[id].fmResetCoeff = 0;
  }
}

inline void checkFiltReset(int bankNum, FILT_MOD *pL, volatile FILT_MOD *dspVme, COEF *pC, int totMod, volatile VME_COEF *pRfmCoeff) {
  checkFiltResetId(bankNum, pL, dspVme, pC, totMod, pRfmCoeff, 0);
}

inline int
initVarsId(FILT_MOD *pL,
	 volatile FILT_MOD *pV,
	 COEF *pC,
	 int totMod,
         volatile VME_COEF *pRfmCoeff,
	 int id)                         /* System id (HEPI) */
{
  int ii,kk,hh;

#ifdef FIR_FILTERS
  int ll;

    for (ii = 0; ii < MAX_FIR_MODULES; ii++)
      for (kk = 0; kk < FILTERS; kk++)
        for (ll = 0; ll < FIR_POLYPHASE_SIZE; ll++)
          for (hh = 0; hh < FIR_TAPS; hh++)
    		pC->firHistory[ii][kk][ll][hh] = 0.0;
#endif

  /*Initialize all the variables */
  for(ii=0;ii<totMod;ii++){
    
    for(kk=0;kk<FILTERS;kk++){
      /*set History*/
      for(hh=0;hh<MAX_HISTRY;hh++){
	pC->coeffs[ii].filtHist[kk][hh] = 0;
      }
      
      pL->inputs[ii].cnt[kk] = 0;
    }

    /*set decimation history*/
    for(hh=0;hh<8;hh++){
      pC->coeffs[ii].decHist[hh] = 0;
    }
    
    /*set switch/offsets/gains/limits*/
    pL->inputs[ii].opSwitchE = 
      pV->inputs[ii].opSwitchE;
    
    pL->inputs[ii].opSwitchP = 0;
    pL->inputs[ii].rset = 0;
    pL->inputs[ii].offset = pV->inputs[ii].offset;
    pL->inputs[ii].outgain = pV->inputs[ii].outgain;
    gain_ramp[ii][id].prev_outgain = pL->inputs[ii].outgain;
    pL->inputs[ii].limiter =  pV->inputs[ii].limiter;
    pL->inputs[ii].gain_ramp_time = pV->inputs[ii].gain_ramp_time;
    gain_ramp[ii][id].gain_ramp_cycles = 0;
    gain_ramp[ii][id].ramp_cycles_left = 0;
  }

  return readCoefVme(pC, pL, 0, totMod, pRfmCoeff);
}

inline int
initVars(FILT_MOD *pL, volatile FILT_MOD *pV, COEF *pC, int totMod,
         volatile VME_COEF *pRfmCoeff) {
  return initVarsId(pL, pV, pC, totMod, pRfmCoeff, 0);
}

inline void
initVars1Id(FILT_MOD *pL,
	  volatile FILT_MOD *pV,
	  COEF *pC,
	  int modStart,
	  int totMod,
	  volatile VME_COEF *pRfmCoeff,
	  int id)                       /* System Id (HEPI) */
{
  int ii,kk,hh;

  /*Initialize all the variables */
  for(ii=modStart; ii<totMod;ii++){
    
    for(kk=0;kk<FILTERS;kk++){
      /*set History*/
      for(hh=0;hh<MAX_HISTRY;hh++){
	pC->coeffs[ii].filtHist[kk][hh] = 0;
      }
      
      pL->inputs[ii].cnt[kk] = 0;
    }

    /*set decimation history*/
    for(hh=0;hh<8;hh++){
      pC->coeffs[ii].decHist[hh] = 0;
    }
    
    /*set switch/offsets/gains/limits*/
    pL->inputs[ii].opSwitchE = 
      pV->inputs[ii].opSwitchE;
    
    pL->inputs[ii].opSwitchP = 0;
    pL->inputs[ii].rset = 0;
    pL->inputs[ii].offset = pV->inputs[ii].offset;
    pL->inputs[ii].outgain = pV->inputs[ii].outgain;
    gain_ramp[ii][id].prev_outgain = pL->inputs[ii].outgain;
    pL->inputs[ii].limiter =  pV->inputs[ii].limiter;
    pL->inputs[ii].gain_ramp_time = pV->inputs[ii].gain_ramp_time;
    gain_ramp[ii][id].gain_ramp_cycles = 0;
    gain_ramp[ii][id].ramp_cycles_left = 0;
  }

  readCoefVme(pC + modStart, pL, modStart, totMod, pRfmCoeff + modStart);
}

inline void
initVars1(FILT_MOD *pL, volatile FILT_MOD *pV, COEF *pC, int modStart,
	  int totMod, volatile VME_COEF *pRfmCoeff)  {
  initVars1Id(pL, pV, pC, modStart, totMod, pRfmCoeff, 0);
}

#ifndef SERVOMIXED

/**************************************************************************
filterModule - Perform filtering for a string of up to 10 filters

Decodes Operator Switches.
Filters sections according to specified filter type (1-3).
Outputs answer to the output module function.
Sets readback filter bit for EPICS display screen.
*************************************************************************/

inline double
filterModule2(FILT_MOD *pFilt,     /* Filter module data  */
	     COEF *pC,            /* Filter coefficients */
	     int modNum,          /* Filter module number */
	     double filterInput,  /* Input data sample (output from funtcion inputModule()) */
	     int *mask)		 /* Allows external switching control */
{
  /* decode arrays for operator switches */
  UINT32 opSwitchE = pFilt->inputs[modNum].opSwitchE;
  /* Do the shift to match the bits in the the opSwitchE variable so I can do "==" comparisons */
  UINT32 opSwitchP = pFilt->inputs[modNum].opSwitchP >> 1;
  int ii, jj, kk, ramp, timeout;
  int sw, sw_out, sType, sw_in,swControl;
  double filtData;
  float avg, compare;
  double output;
  double fmInput;

  fmInput = 0;
  pFilt->data[modNum].filterInput = (float)(filterInput + fmInput);
  fmInput += (double)pFilt->data[modNum].exciteInput;
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_INPUT_ENABLE)
    fmInput += filterInput;
  pFilt->data[modNum].inputTestpoint = fmInput;
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_OFFSET_ENABLE)
    fmInput += pFilt->inputs[modNum].offset;

  /* Apply Filtering */

  /* Loop through all filters */
  for (ii=0; ii<FILTERS; ii++) {
    /* Do not do anything for any filter with zero filter sections */
    if (!pC->coeffs[modNum].filtSections[ii]) continue;

    sw = opSwitchE & pow2_in[ii];       /* Epics screen filter on/off request bit */
    swControl = *mask & pow2_in[ii];
    sw_out = opSwitchP & pow2_in[ii];   /* Pentium output ack bit (opSwitchP was right shifted by 1) */

    /* Filter switching type */
    sType = pC->coeffs[modNum].sType[ii];

    /* If sType is type 1X, the input will always go to filter to be calculated */
    /* If sType is type 2X, then the input will be zero if output is turned off */
    /* If sType is type 22, then the input will go to filter if filter is turning on (ramping up)*/
    sw_in = sType < 20 || sw_out || (sType == 22 && sw);

    /* Calculate filter */
    filtData = iir_filter(sw_in?fmInput:0,
			  pC->coeffs[modNum].filtCoeff[ii],
			  pC->coeffs[modNum].filtSections[ii],
			  pC->coeffs[modNum].filtHist[ii]);

    if (sw == sw_out) { /* No switching */
      if (sw) fmInput = filtData; /* Use the filtered value if the filter is turned on */
    } else {  /* Switching request */
      int sTypeMod10 = sType%10;
      /* Process filter switching according to output type [1-3] */
      switch (sTypeMod10) {
      case 1: /* Instantenious switch */
	/* Turn output on/off according to the request */
	if ((sw_out = sw)) fmInput = filtData; /* Use the filtered value if the filter is turned on */
	*mask |= pow2_out[ii];
	break;
      case 2: /* Ramp in/out filter output */
	ramp = pFilt->inputs[modNum].rmpcmp[ii];   /* Ramp slope coefficient */
	kk = pFilt->inputs[modNum].cnt[ii];        /* Ramp count */
	/* printf("kk=%d; ramp=%d; sw=%d; sw_out=%d\n", kk, ramp, sw, sw_out); */

	if (kk == ramp) {                          /* Done ramping */
	  /* Turn output on/off according to the request */
	  if ((sw_out = sw)) fmInput = filtData; /* Use the filtered value if the filter is turned on */
	  kk = 0;
	  *mask |= pow2_out[ii];
	} else {                                   /* Ramping will be done */
	  if (kk) {                                /* Currently ramping */
	    double t = (double)kk / (double)ramp;  /* Slope */
	    if (sw)                                /* Turn on request */
	      fmInput = (1.0 - t) * fmInput + t * filtData;
	    else                                   /* Turn off request */
	      fmInput = t * fmInput + (1.0 - t) * filtData;
	  } else {                                 /* Start to ramp */
	    if (sw)                                /* Turn on request */
	      ;                                    /* At the start of turning on ramp input goes to output (filter bypassed) */
	    else                                   /* Turn off request */
	      fmInput = filtData;	           /* At the start of turning off the ramp is at filter output */
	  }
	  kk++;
	}
	pFilt->inputs[modNum].cnt[ii] = kk;
	break;
      case 3: /* Comparator */
      case 4: /* Zero crossing */
	ramp = pFilt->inputs[modNum].rmpcmp[ii];  /* Filter comparison range */
	timeout = pFilt->inputs[modNum].timeout[ii]; 	/* Comparison timeout number */
	kk = pFilt->inputs[modNum].cnt[ii];   /* comparison count */
	if (sTypeMod10 == 3) compare = fmInput - filtData; /* Comparator looks at the filter in/out diff */
	else compare = sw_out? filtData: fmInput; /* Use the filtered value if waiting to switch off */
	if (compare < .0) compare = -compare;

	++kk;
	if (kk >= timeout || compare <= (float)ramp) { /* If timed out or the difference is in the range */
	  *mask |= pow2_out[ii];
	  kk = timeout;
	}
	if((*mask & pow2_in[ii]) && (*mask & pow2_out[ii]))
	{
	  /* Turn output on/off according to the request */
	  if ((sw_out = sw)) fmInput = filtData; /* Use the filtered value if the filter is turned on */
	  kk = 0;
	} else { /* Waiting for the match or the timeout */
	  if (sw_out) fmInput = filtData; /* Use the filtered value if waiting to switch off */
	}

	pFilt->inputs[modNum].cnt[ii] = kk;
	break;
      default:
	fmInput = 777;
	break;
      }
    
      if (sw == sw_out) { /* If turning the filter on/off NOW */
	/* Clear history buffer if filter is turned off NOW */
	/* History is cleared one time only */
	if (!sw) { /* Turn off request */
	  for(jj=0;jj<MAX_HISTRY;jj++)
	    pC->coeffs[modNum].filtHist[ii][jj] = 0.0;

	}

	/* Send back the readback switches when there is change */
	if (sw_out) { /* switch is on, turn it on */
	  pFilt->inputs[modNum].opSwitchP |= pow2_out[ii];
	} else { /* switch is off, turn it off */
	  pFilt->inputs[modNum].opSwitchP &= ~pow2_out[ii];
	}
      }
    }
  }

  /* Calculate output values */
  {
    output = fmInput * (double)pFilt->inputs[modNum].outgain;

    /* Limiting */
    if (opSwitchE &  OPSWITCH_LIMITER_ENABLE) {
      if(output > pFilt->inputs[modNum].limiter)
	output = pFilt->inputs[modNum].limiter;
      else if(output < -pFilt->inputs[modNum].limiter)
	output = -pFilt->inputs[modNum].limiter;
    }

    /* Set Test Point */
    pFilt->data[modNum].testpoint = output;

#if defined(SERVO5HZ)
    avg = output;
#else
    /* Do the Decimation regardless of whether it is enabled or disabled */
    avg = iir_filter(output,avgCoeff,2,pC->coeffs[modNum].decHist);
#endif

    /* Test Output Switch and output hold on/off */
    if (opSwitchE & OPSWITCH_HOLD_ENABLE) {
      ; /* Outputs are not assigned, hence they are held */
    } else {
      if (opSwitchE & OPSWITCH_OUTPUT_ENABLE) {
	pFilt->data[modNum].output = output;

	/* Decimation */
	if (opSwitchE & OPSWITCH_DECIMATE_ENABLE) 
	  pFilt->data[modNum].output16Hz = avg;
	else
	  pFilt->data[modNum].output16Hz = output;
      } else {
        pFilt->data[modNum].output = 0;
        pFilt->data[modNum].output16Hz = 0;
	output = 0.0;
      }
    }
  }
  return output;
}

#endif
	     
/* This module added to hanle all input, calculations and outputs as doubles. This module also
   incorporates the input module, done separately above for older systems. */
inline double
filterModuleD(FILT_MOD *pFilt,     /* Filter module data  */
	       COEF *pC,            /* Filter coefficients */
	       int modNum,          /* Filter module number */
	       double filterInput,  /* Input data sample (output from funtcion inputModule()) */
	       int fltrCtrlVal)	    /* Filter control value */
	     

{
  int ix;
  UINT32 opSwitchE;
  UINT32 fltrSwitch;
  /* decode arrays for operator switches */
  if ( (fltrCtrlVal >= 0) && (fltrCtrlVal < 1024) ) {
    fltrSwitch = 0;
    if (fltrCtrlVal > 0) {
      for (ix = 0; ix < 10; ix++) {
        if (fltrCtrlVal%2 == 1) {
          fltrSwitch += fltrConst[ix];
        }
        fltrCtrlVal = fltrCtrlVal>>1;
      }
    }

    opSwitchE = pFilt->inputs[modNum].opSwitchE | fltrSwitch;
    pFilt->inputs[modNum].opSwitchE = opSwitchE;
  }
  else {
    opSwitchE = pFilt->inputs[modNum].opSwitchE;
  }
  /* Do the shift to match the bits in the the opSwitchE variable so I can do "==" comparisons */
  UINT32 opSwitchP = pFilt->inputs[modNum].opSwitchP >> 1;
  int ii, jj, kk, ramp, timeout;
  int sw, sw_out, sType, sw_in;
  double filtData;
  float avg, compare;
  double output;
  double fmInput;
  int id = 0;                  /* System number (HEPI) */

  /* Set the input to a very small number. If input is zero, code timing becomes a problem. */
  /* This is not fully understood, but it may be due to floating point underflow when the   */
  /* remaining calculations are done.							    */
  fmInput = 0;

  /* Load the filter input testpoint to the input value. */
  pFilt->data[modNum].filterInput = (float)(filterInput + fmInput);

  /* Add excitation signal to input value. */
  fmInput += (double)pFilt->data[modNum].exciteInput;

  /* If input is turned on, add the filterInput value. */
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_INPUT_ENABLE)
    	fmInput += filterInput;
  pFilt->data[modNum].inputTestpoint = fmInput;

  /* If the offset is enabled, add the filter module offset value. */
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_OFFSET_ENABLE)
    fmInput += pFilt->inputs[modNum].offset;

  /* Apply Filtering */

  /* Loop through all filters */
  for (ii=0; ii<FILTERS; ii++) {
    /* Do not do anything for any filter with zero filter sections */
    if (!pC->coeffs[modNum].filtSections[ii]) continue;

    sw = opSwitchE & pow2_in[ii];       /* Epics screen filter on/off request bit */
    sw_out = opSwitchP & pow2_in[ii];   /* Pentium output ack bit (opSwitchP was right shifted by 1) */

    /* Filter switching type */
    sType = pC->coeffs[modNum].sType[ii];

    /* If sType is type 1X, the input will always go to filter to be calculated */
    /* If sType is type 2X, then the input will be zero if output is turned off */
    /* If sType is type 22, then the input will go to filter if filter is turning on (ramping up)*/
    sw_in = sType < 20 || sw_out || (sType == 22 && sw);

#ifdef FIR_FILTERS
    int filterType = pC->coeffs[modNum].filterType[ii];
    if (filterType) {
      extern int clock16K;
#ifdef SERVO2K
      int firNum = (clock16K / 32) % 32;
#endif
#ifdef SERVO4K
      int firNum = (clock16K / 32) % 64;
#endif
      //int firNum = clock16K % 32;
      //printf("clock16K=%d; firNum=%d\n", clock16K, firNum);

      /* FIR filter */
      --filterType;
      if (clock16K % 32) filtData = pC->prevFirOutput[filterType];
      else {
	if (filterType >=0 && filterType < MAX_FIR_MODULES) {
	  double input = fmInput * pC->firFiltCoeff[filterType][ii][0]; /* overall input scale factor */
	  filtData = fir_filter(sw_in?input:0,
			      &(pC->firFiltCoeff[filterType][ii][1]),
			      pC->coeffs[modNum].filtSections[ii]*4,
			      &(pC->firHistory[filterType][ii][firNum][0]));
	  pC->prevFirOutput[filterType] = filtData;
	} else {
	  filtData = filterType;
	  printf("module %d; filter %d; filterType = %d\n", modNum, ii, filterType);
	}
      }
    } else
#endif
    /* Calculate filter */
    filtData = (pC->coeffs[modNum].biquad? iir_filter_biquad: iir_filter)(sw_in?fmInput:0,
			  pC->coeffs[modNum].filtCoeff[ii],
			  pC->coeffs[modNum].filtSections[ii],
			  pC->coeffs[modNum].filtHist[ii]);

    if (sw == sw_out) { /* No switching */
      if (sw) fmInput = filtData; /* Use the filtered value if the filter is turned on */
    } else {  /* Switching request */
      int sTypeMod10 = sType%10;
      /* Process filter switching according to output type [1-3] */
      switch (sTypeMod10) {
      case 1: /* Instantenious switch */
	/* Turn output on/off according to the request */
	if ((sw_out = sw)) fmInput = filtData; /* Use the filtered value if the filter is turned on */
	break;
      case 2: /* Ramp in/out filter output */
	ramp = pFilt->inputs[modNum].rmpcmp[ii];   /* Ramp slope coefficient */
	kk = pFilt->inputs[modNum].cnt[ii];        /* Ramp count */
	/* printf("kk=%d; ramp=%d; sw=%d; sw_out=%d\n", kk, ramp, sw, sw_out); */

	if (kk == ramp) {                          /* Done ramping */
	  /* Turn output on/off according to the request */
	  if ((sw_out = sw)) fmInput = filtData; /* Use the filtered value if the filter is turned on */
	  kk = 0;
	} else {                                   /* Ramping will be done */
	  if (kk) {                                /* Currently ramping */
	    double t = (double)kk / (double)ramp;  /* Slope */
	    if (sw)                                /* Turn on request */
	      fmInput = (1.0 - t) * fmInput + t * filtData;
	    else                                   /* Turn off request */
	      fmInput = t * fmInput + (1.0 - t) * filtData;
	  } else {                                 /* Start to ramp */
	    if (sw)                                /* Turn on request */
	      ;                                    /* At the start of turning on ramp input goes to output (filter bypassed) */
	    else                                   /* Turn off request */
	      fmInput = filtData;	           /* At the start of turning off the ramp is at filter output */
	  }
	  kk++;
	}
	pFilt->inputs[modNum].cnt[ii] = kk;
	break;
      case 3: /* Comparator */
      case 4: /* Zero crossing */
	ramp = pFilt->inputs[modNum].rmpcmp[ii];  /* Filter comparison range */
	timeout = pFilt->inputs[modNum].timeout[ii]; 	/* Comparison timeout number */
	kk = pFilt->inputs[modNum].cnt[ii];   /* comparison count */
	if (sTypeMod10 == 3) compare = fmInput - filtData; /* Comparator looks at the filter in/out diff */
	else compare = sw_out? filtData: fmInput; /* Use the filtered value if waiting to switch off */
	if (compare < .0) compare = -compare;

	++kk;
	if (kk >= timeout || compare <= (float)ramp) { /* If timed out or the difference is in the range */
	  /* Turn output on/off according to the request */
	  if ((sw_out = sw)) fmInput = filtData; /* Use the filtered value if the filter is turned on */
	  kk = 0;
	} else { /* Waiting for the match or the timeout */
	  if (sw_out) fmInput = filtData; /* Use the filtered value if waiting to switch off */
	}
	pFilt->inputs[modNum].cnt[ii] = kk;
	break;
      default:
	fmInput = 777;
	break;
      }
    
      if (sw == sw_out) { /* If turning the filter on/off NOW */
	/* Clear history buffer if filter is turned off NOW */
	/* History is cleared one time only */
	if (!sw) { /* Turn off request */
	  for(jj=0;jj<MAX_HISTRY;jj++)
	    pC->coeffs[modNum].filtHist[ii][jj] = 0.0;

	}

	/* Send back the readback switches when there is change */
	if (sw_out) { /* switch is on, turn it on */
	  pFilt->inputs[modNum].opSwitchP |= pow2_out[ii];
	} else { /* switch is off, turn it off */
	  pFilt->inputs[modNum].opSwitchP &= ~pow2_out[ii];
	}
      }
    }
  }

  /* Calculate output values */
  {
    double cur_gain;

    if (pFilt->inputs[modNum].outgain != gain_ramp[modNum][id].prev_outgain) {
      /* Abort running ramping if user inputs 0.0 seconds ramp time */
      if (pFilt->inputs[modNum].gain_ramp_time == 0.0) {
	gain_ramp[modNum][id].ramp_cycles_left
	  = gain_ramp[modNum][id].gain_ramp_cycles = 0;
      }
      
      switch (gain_ramp[modNum][id].ramp_cycles_left) {
	case 0: /* Initiate gain ramping now */
	  {
	    if (gain_ramp[modNum][id].gain_ramp_cycles == 0) {
	      /* Calculate how many steps we will be doing */
	      gain_ramp[modNum][id].ramp_cycles_left
	    	= gain_ramp[modNum][id].gain_ramp_cycles 
	    	= rate * pFilt->inputs[modNum].gain_ramp_time;

	      /* Send ramp indication bit to Epics */
	      pFilt->inputs[modNum].opSwitchP |= 0x10000000;
	   
	      if (gain_ramp[modNum][id].ramp_cycles_left < 1)  {
		  gain_ramp[modNum][id].ramp_cycles_left
		    = gain_ramp[modNum][id].gain_ramp_cycles
		    = 1;
	      }
	    }
  	    /* Fall through to default */
	  }
	default:/* Gain ramping is in progress */
	  {
	    --gain_ramp[modNum][id].ramp_cycles_left;

	    cur_gain = ((double) gain_ramp[modNum][id].prev_outgain) +
		((((double) pFilt->inputs[modNum].outgain)
		  - ((double) gain_ramp[modNum][id].prev_outgain))
		 * (1.0 - (((double) gain_ramp[modNum][id].ramp_cycles_left)
			   / ((double) gain_ramp[modNum][id].gain_ramp_cycles)))
		);

	    if ((pFilt->inputs[modNum].outgain
		 < gain_ramp[modNum][id].prev_outgain)
		? cur_gain <= pFilt->inputs[modNum].outgain
		: cur_gain >= pFilt->inputs[modNum].outgain) {

		/* Stop ramping now, we are done */
		cur_gain 
		 = gain_ramp[modNum][id].prev_outgain
		 = pFilt->inputs[modNum].outgain;
	        gain_ramp[modNum][id].ramp_cycles_left
	         = gain_ramp[modNum][id].gain_ramp_cycles
		 = 0;

	        /* Clear ramping flag bit for Epics */
	        pFilt->inputs[modNum].opSwitchP &= ~0x10000000;
	    }
	    break;
	  }
      }
    } else {
	if (gain_ramp[modNum][id].ramp_cycles_left
	    || gain_ramp[modNum][id].gain_ramp_cycles) {
	  /* Correct for any floating point arithmetic mistakes */
          gain_ramp[modNum][id].ramp_cycles_left
            = gain_ramp[modNum][id].gain_ramp_cycles
	    = 0;
          /* Clear ramping flag bit for Epics */
          pFilt->inputs[modNum].opSwitchP &= ~0x10000000;
	}
	cur_gain = pFilt->inputs[modNum].outgain;
    }
    output = fmInput * cur_gain;
  if(output > 1e20) output = 1e20;

    /* Limiting */
    /* If the limit switch is on, limit the output accordingly. */
    if (opSwitchE &  OPSWITCH_LIMITER_ENABLE) {
      if(output > pFilt->inputs[modNum].limiter)
	output = pFilt->inputs[modNum].limiter;
      else if(output < -pFilt->inputs[modNum].limiter)
	output = -pFilt->inputs[modNum].limiter;
    }

    /* Set Output Test Point */
    pFilt->data[modNum].testpoint = output;

    /* Test Output Switch and output hold on/off */
    if (opSwitchE & OPSWITCH_HOLD_ENABLE) {
	/* Assign output to last held value. */
	output = pFilt->data[modNum].output;
      ; /* Other outputs are not assigned, hence they are held */
    } else {
      if (opSwitchE & OPSWITCH_OUTPUT_ENABLE) {
	pFilt->data[modNum].output = output;

	/* Decimation */
	if (opSwitchE & OPSWITCH_DECIMATE_ENABLE) 
	{
	    avg = iir_filter(output, avgCoeff, 2, pC->coeffs[modNum].decHist);
	  pFilt->data[modNum].output16Hz = avg;
	}
	else
	  pFilt->data[modNum].output16Hz = output;
      } else {
        pFilt->data[modNum].output = 0;
        pFilt->data[modNum].output16Hz = 0;
	output = 0.0;
      }
    }
  }
  return output;
}
