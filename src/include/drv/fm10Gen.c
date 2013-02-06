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

///	@file fm10Gen.c
///	@brief This file contains the routines for performing the real-time
///		IIR/FIR filter calculations. \n
///	Further information is provided in the LIGO DCC 
///	<a href="https://dcc.ligo.org/cgi-bin/private/DocDB/ShowDocument?docid=7687">T0900606 CDS Standard IIR Filter Module Software</a>


#include "fm10Gen.h"


/* Switching register bits */
#define OPSWITCH_INPUT_ENABLE 0x4
#define OPSWITCH_OFFSET_ENABLE 0x8
#define OPSWITCH_LIMITER_ENABLE 0x1000000
#define OPSWITCH_DECIMATE_ENABLE 0x2000000
#define OPSWITCH_OUTPUT_ENABLE 0x4000000
#define OPSWITCH_HOLD_ENABLE 0x8000000

/// Quick look up table for power of 2 calcs
const UINT32 pow2_in[10] = {0x10,0x40,0x100,0x400,0x1000,0x4000,0x10000,
				   0x40000,0x100000,0x400000};
/// Quick look up table for power of 2 calcs
const UINT32 pow2_out[10] = {0x20,0x80,0x200,0x800,0x2000,0x8000,0x20000,
				    0x80000,0x200000,0x800000};

/// Quick look up table for filter module switch decoding
const UINT32 fltrConst[13] = {16, 64, 256, 1024, 4096, 16384,
                                     65536, 262144, 1048576, 4194304,
				     0x4, 0x8, 0x4000000, /* in sw, off sw, out sw */
				     };

#if defined(SERVO16K) || defined(SERVO32K) || defined(SERVO64K) || defined(SERVO128K) || defined(SERVO256K)
double sixteenKAvgCoeff[9] = {1.9084759e-12,
				     -1.99708675982420, 0.99709029700517, 2.00000005830747, 1.00000000739582,
				     -1.99878510620232, 0.99879373895648, 1.99999994169253, 0.99999999260419};
#endif

#if defined(SERVO2K) || defined(SERVO4K)
double twoKAvgCoeff[9] = {7.705446e-9,
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
#elif defined(SERVO5HZ)
#else
#error need to define 2k or 16k or mixed
#endif

#ifdef SERVO64K
  #define FE_RATE 65536
#endif
#ifdef SERVO32K
  #define FE_RATE 32768
#endif
#ifdef SERVO16K
  #define FE_RATE 16384
#endif
#ifdef SERVO4K
  #define FE_RATE 4096
#endif
#ifdef SERVO2K
  #define FE_RATE 2048
#endif


/// New tRamp.c gain ramping strcuture
RampParamState gain_ramp[MAX_MODULES][10];


/// New tRamp.c gain offset strcuture
RampParamState offset_ramp[MAX_MODULES][10];

/// @brief Perform IIR filtering sample by sample on doubles. 
/// Implements cascaded direct form II second order sections.
///	@param[in] input New input sample
///	@param[in] *coef Pointer to filter coefficient data with size 4*n + 1 (gain)
///	@param[in] n Number of second order sections in filter definition
///	@param[in,out] *history Pointer to filter history data of size 2*n
///	@return Result of filter calculation
inline double iir_filter(double input,double *coef,int n,double *history){

  int i;
  double *coef_ptr;
  double *hist1_ptr,*hist2_ptr;
  double output,new_hist,history1,history2;

  coef_ptr = coef;                /* coefficient pointer */
  
  hist1_ptr = history;            /* first history */
  hist2_ptr = hist1_ptr + 1;      /* next history */
  
  output = input * (*coef_ptr++); /* overall input scale factor */
  
  for(i = 0 ; i < n ; i++) {
    
    history1 = *hist1_ptr;        /* history values */
    history2 = *hist2_ptr;
    
    
    output = output - history1 * (*coef_ptr++);
    new_hist = output - history2 * (*coef_ptr++);    /* poles */
    
    output = new_hist + history1 * (*coef_ptr++);
    output = output + history2 * (*coef_ptr++);      /* zeros */

    *hist2_ptr++ = *hist1_ptr;
    *hist1_ptr++ = new_hist;
    hist1_ptr++;
    hist2_ptr++;
    
  }
  
  return(output);
}

/* Biquad form IIR */
/// @brief Perform IIR filtering sample by sample on doubles. 
/// Implements Biquad form calculations.
///	@param[in] input New input sample
///	@param[in] *coef Pointer to filter coefficient data with size 4*n + 1 (gain)
///	@param[in] n Number of second order sections in filter definition
///	@param[in,out] *history Pointer to filter history data of size 2*n
///	@return Result of filter calculation
inline double iir_filter_biquad(double input,double *coef,int n,double *history){

  int i;
  double *coef_ptr;
  double *hist1_ptr,*hist2_ptr;
  double output,new_w, new_u, w, u, a11, a12, c1, c2;

  coef_ptr = coef;                /* coefficient pointer */
  
  hist1_ptr = history;            /* first history */
  hist2_ptr = hist1_ptr + 1;      /* next history */
  
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

    *hist1_ptr++ = new_w;
    *hist2_ptr++ = new_u;
    hist1_ptr++;
    hist2_ptr++;
    
  }
  
  //if((output < 1e-28) && (output > -1e-28)) output = 0.0;
  return(output);
}

#ifdef FIR_FILTERS
// *************************************************************************/
///	@brief Perform FIR filter calculations.
///	@param[in] input New input sample
///	@param[in] *coef Pointer to filter coefficients
///	@param[in] n Number of taps
///	@param[in,out] *history Pointer to filter history data
///	@return Result of FIR filter calculation
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


#ifdef SERVO16K
const int rate = 16384;
#elif defined(SERVO5HZ)
const int rate = 5;
#elif defined(SERVO2K)
const int rate = 2048;
#elif defined(SERVO4K)
const int rate = 4096;
#elif defined(SERVO32K)
const int rate = 32768;
#elif defined(SERVO64K)
const int rate = (2*32768);
#elif defined(SERVO128K)
const int rate = (4*32768);
#elif defined(SERVO256K)
const int rate = (8*32768);
#endif
	     
/************************************************************************/
/************************************************************************/
/// @brief Read in filter coeffs from shared memory on code initialization.
///	@param[in,out] *filtC Pointer to coeffs in local memory
///	@param[in] *fmt Pointer to filter data in local memory
///	@param[in] bF	Start filter number
///	@param[in] sF	End filter number
///	@param[in] *pRfmCoeff Pointer to coeffs in shared memory
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
    //if(filtC->coeffs[ii].biquad) printf("Found a BQF filter %d\n",ii);
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
/// @brief Read in filter coeffs from shared memory while code is running ie filter reload initiated.
///	@param[in,out] *filtC Pointer to coeffs in local memory
///	@param[in] *fmt Pointer to filter data in local memory
///	@param[in] modNum1	ID number of the filter module
///	@param[in] filtNum	ID number of the filter within the module
///	@param[in] cycle	Code cycle number
///	@param[in] *pRfmCoeff 	Pointer to coeffs in shared memory
///	@param[in] *changed	Pointer to filter coef change flag memory.
int readCoefVme2(COEF *filtC,FILT_MOD *fmt, int modNum1, int filtNum, int cycle, volatile VME_COEF *pRfmCoeff, int *changed)
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

/// Filter module update globals 
struct filtResetId {
  int fmResetCoeff;
  int fmResetCounter;
  int fmSubCounter;
  FILT_MOD *fmResetDsp;
  int changed[FILTERS];
} filtResetId[FILTERS] = {
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
///	@brief Checks for history resets or new coefs for filter modules.
///	@param[in] bankNum	Filter module ID number
///	@param[in,out] *pL	Pointer to filter module data in local memory.
///	@param[in] *dspVme	Pointer to filter module data in shared memory.
///	@param[in,out] *pC	Pointer to filter coefs in local memory.
///	@param[in] totMod	Total number of filter modules
///	@param[in] *pRfmCoeff	Pointer to filter coefs in shared memory.
///	@param[in] id		System ID number (old HEPI only).
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

///	@brief Calls checkFiltResetId with dummy sys id of 0
///	@param[in] bankNum	Filter module ID number
///	@param[in,out] *pL	Pointer to filter module data in local memory.
///	@param[in] *dspVme	Pointer to filter module data in shared memory.
///	@param[in,out] *pC	Pointer to filter coefs in local memory.
///	@param[in] totMod	Total number of filter modules
///	@param[in] *pRfmCoeff	Pointer to filter coefs in shared memory.
inline void checkFiltReset(int bankNum, FILT_MOD *pL, volatile FILT_MOD *dspVme, COEF *pC, int totMod, volatile VME_COEF *pRfmCoeff) {
  checkFiltResetId(bankNum, pL, dspVme, pC, totMod, pRfmCoeff, 0);
}

///	@brief Initialize filter module variables on code startup
///	@param[in,out] *pL	Pointer to filter module data in local memory
///	@param[in] *pV		Pointer to filter module data in shared memory
///	@param[in,out] *pC	Pointer to filter coeffs in local memory
///	@param[in] totMod	Total number of filter modules
///	@param[in] *pRfmCoeff	Pointer to filter coeffs in shared memory
///	@param[in] id		System id (old HEPI only)
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
    pL->inputs[ii].limiter =  pV->inputs[ii].limiter;
    pL->inputs[ii].gain_ramp_time = pV->inputs[ii].gain_ramp_time;
    RampParamInit(&offset_ramp[ii][id],
                  (pL->inputs[ii].opSwitchE & OPSWITCH_OFFSET_ENABLE)?pL->inputs[ii].offset:0.0,
                  FE_RATE);
    RampParamInit(&gain_ramp[ii][id],pL->inputs[ii].outgain,FE_RATE);  
  }

  return readCoefVme(pC, pL, 0, totMod, pRfmCoeff);
}

///	@brief Calls initVarsId with dummy sys id of 0
///	@param[in,out] *pL	Pointer to filter module data in local memory
///	@param[in] *pV		Pointer to filter module data in shared memory
///	@param[in,out] *pC	Pointer to filter coeffs in local memory
///	@param[in] totMod	Total number of filter modules
///	@param[in] *pRfmCoeff	Pointer to filter coeffs in shared memory
inline int
initVars(FILT_MOD *pL, volatile FILT_MOD *pV, COEF *pC, int totMod,
         volatile VME_COEF *pRfmCoeff) {
  return initVarsId(pL, pV, pC, totMod, pRfmCoeff, 0);
}

/* This module added to hanle all input, calculations and outputs as doubles. This module also
   incorporates the input module, done separately above for older systems. */

/* Mask logic table:
 * M-mask; C-control input; S0-current bit state; S1-new bit state
         [0]   M  C  S0 S1
               -----------
               0  0  0  0
               0  1  0  0
               1  0  0  0
               1  1  0  1
	       0  0  1  1
	       0  1  1  1
	       1  0  1  0
	       1  1  1  1
      */
/// 	@brief This function is called by filterModuleD, or, in the case of FMC2 parts, user code directly to
///< perform CDS standard filter module calculations.
///	@param[in,out] *pFilt Filter Module Data
///	@param[in] *pC Filter module coefficients
///	@param[in] modNum Filter module ID number
///	@param[in] filterInput Input data sample
///	@param[in] fltrCtrlVal Filter control value
///	@param[in] mask Control mask
///	@param[in] offset_in Filter module DC offset value from user model.
///	@param[in] gain_in Filter module gain value from user model.
///	@param[in] ramp_in Ramping time from user model.
///	@return Output of IIR/FIR filter calculations.
double
filterModuleD2(FILT_MOD *pFilt,     /* Filter module data  */
	       COEF *pC,            /* Filter coefficients */
	       int modNum,          /* Filter module number */
	       double filterInput,  /* Input data sample (output from funtcion inputModule()) */
	       int fltrCtrlVal,	    /* Filter control value */
	       int mask,	    /* Mask of bits to act upon */
	       double offset_in,
	       double gain_in,
	       double ramp_in)
{
  int ix;
  UINT32 opSwitchE;
  int ii, jj, kk, ramp, timeout;
  int sw, sw_out, sType, sw_in;
  double filtData;
  float avg, compare;
  double output;
  double fmInput;
  int id = 0;                  /* System number (HEPI) */

  /* Do the shift to match the bits in the the opSwitchE variable so I can do "==" comparisons */
  UINT32 opSwitchP = pFilt->inputs[modNum].opSwitchP >> 1;

  fltrCtrlVal &= 0xffff; /* Limit to the 16 bits */

  /* decode arrays for operator switches */
  if (mask != 0 && (fltrCtrlVal >= 0)) {
    UINT32 fltrSwitch = 0;
    UINT32 epicsExclude = 0;
    if (mask > 0) {
      for (ix = 0; ix < 13; ix++) {
	if (mask & (1<<ix)) {
		// Keep the current bit value
		// 
		if (opSwitchP & fltrConst[ix]) fltrSwitch |= fltrConst[ix];
		// Change only if bit-mask is set
		//
        	if (fltrCtrlVal%2 == 1) {
          		fltrSwitch |= fltrConst[ix];
        	} else {
          		fltrSwitch &= ~fltrConst[ix];
		}
		epicsExclude |= fltrConst[ix];
	}
        fltrCtrlVal = fltrCtrlVal>>1;
      }
      // Assign offset, gain and ramp value bits (last 3 bits in 16-bit control input)
      epicsExclude |= (mask & 0xe000) << 16;

      if (mask & 0x2000) pFilt->inputs[modNum].offset = offset_in; /* Assign local offset */
      if (mask & 0x4000) pFilt->inputs[modNum].outgain = gain_in;
      if (mask & 0x8000) pFilt->inputs[modNum].gain_ramp_time = ramp_in;
    }
    pFilt->inputs[modNum].mask = epicsExclude;
    pFilt->inputs[modNum].control = fltrSwitch;
    opSwitchE = (pFilt->inputs[modNum].opSwitchE & ~epicsExclude) | fltrSwitch;
    pFilt->inputs[modNum].opSwitchE = opSwitchE;
  }
  else {
    pFilt->inputs[modNum].mask = 0;
    pFilt->inputs[modNum].control = 0;
    opSwitchE = pFilt->inputs[modNum].opSwitchE;
  }

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
  if (pFilt->inputs[modNum].opSwitchE & OPSWITCH_OFFSET_ENABLE) {
    //fmInput += pFilt->inputs[modNum].offset;

    if (pFilt->inputs[modNum].offset != offset_ramp[modNum][id].req) {
    	RampParamLoad(&offset_ramp[modNum][id],pFilt->inputs[modNum].offset,pFilt->inputs[modNum].gain_ramp_time,rate);
    	pFilt->inputs[modNum].opSwitchP |= 0x20000000;
    }
  } else {
    if (0.0 != offset_ramp[modNum][id].req) {
    	RampParamLoad(&offset_ramp[modNum][id], 0.0, pFilt->inputs[modNum].gain_ramp_time,rate);
    	pFilt->inputs[modNum].opSwitchP |= 0x20000000;
    }
  }
  if (offset_ramp[modNum][id].isRamping == 0) {
  	pFilt->inputs[modNum].opSwitchP &= ~0x20000000;
  }
  fmInput += RampParamUpdate(&offset_ramp[modNum][id]);

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
      extern int cycleNum;
#ifdef SERVO2K
      int firNum = (cycleNum / 32) % 32;
#endif
#ifdef SERVO4K
      int firNum = (cycleNum / 32) % 64;
#endif
      //int firNum = cycleNum % 32;
      //printf("cycleNum=%d; firNum=%d\n", cycleNum, firNum);

      /* FIR filter */
      --filterType;
      if (cycleNum % 32) filtData = pC->prevFirOutput[filterType];
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
    if (pC->coeffs[modNum].biquad)  
      filtData = iir_filter_biquad(sw_in?fmInput:0,
			  pC->coeffs[modNum].filtCoeff[ii],
			  pC->coeffs[modNum].filtSections[ii],
			  pC->coeffs[modNum].filtHist[ii]);
    else
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
    
  if (pFilt->inputs[modNum].outgain != gain_ramp[modNum][id].req) {
    RampParamLoad(&gain_ramp[modNum][id],pFilt->inputs[modNum].outgain,pFilt->inputs[modNum].gain_ramp_time,rate);
    pFilt->inputs[modNum].opSwitchP |= 0x10000000;
  }
  if (gain_ramp[modNum][id].isRamping == 0) {
    pFilt->inputs[modNum].opSwitchP &= ~0x10000000;
  }

  output = fmInput * RampParamUpdate(&gain_ramp[modNum][id]);
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

/// 	@brief This function is called by user apps using standard IIR/FIR filters..
///< This function in turn calls filterModuleD2 to actually perform the calcs, with dummy
///< vars added.
///	@param[in,out] *pFilt Filter Module Data
///	@param[in] *pC Filter module coefficients
///	@param[in] modNum Filter module ID number
///	@param[in] filterInput Input data sample
///	@param[in] fltrCtrlVal Filter control value
///	@param[in] mask Control mask
///	@return Output of IIR/FIR filter calculations.
inline double
filterModuleD(FILT_MOD *pFilt,     /* Filter module data  */
	       COEF *pC,            /* Filter coefficients */
	       int modNum,          /* Filter module number */
	       double filterInput,  /* Input data sample (output from funtcion inputModule()) */
	       int fltrCtrlVal,	    /* Filter control value */
	       int mask)	    /* Mask of bits to act upon */
{
	/* Limit control to the 10 bits */
	return filterModuleD2(pFilt, pC, modNum, filterInput, fltrCtrlVal & 0x3ff, mask, 0., 0., 0.);
}


/* Convert opSwitchE bits into the 16-bit FiltCtrl2 Ctrl output format */
inline unsigned int
filtCtrlBitConvert(unsigned int v) {
	unsigned int val = 0;
	int i;
	for (i = 0; i < 13; i++) {
		if (v & fltrConst[i]) val |= 1<<i;
	}
	return val;
}
