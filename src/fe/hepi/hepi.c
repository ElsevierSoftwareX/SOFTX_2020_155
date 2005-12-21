/*                      -------------------                             */
/*                                                                      */
/*                             LIGO                                     */
/*                                                                      */
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.      */
/*                                                                      */
/*                     (C) The LIGO Project, 2005.                      */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* California Institute of Technology                                   */
/* LIGO Project MS 18-34                                                */
/* Pasadena CA 91125                                                    */
/*                                                                      */
/* Massachusetts Institute of Technology                                */
/* LIGO Project MS 20B-145                                              */
/* Cambridge MA 01239                                                   */
/*                                                                      */
/*                                                                      */
/*----------------------------------------------------------------------*/

#define CHAMBERS	3
#define STS_COUNT	2


/**************************************************************************

fir_filter - Perform fir filtering sample by sample on floats

Requires array of filter coefficients and pointer to history.
Returns one output sample for each input sample.

float fir_filter(float input,float *coef,int n,float *history)

    float input        new float input sample
    float *coef        pointer to filter coefficients
    int n              number of coefficients in filter
    float *history     history array pointer

Returns float value giving the current output.

*************************************************************************/

inline float fir_filter(float input,float *coef,int n,float *history)
{
    int i;
    float *hist_ptr,*hist1_ptr,*coef_ptr;
    float output;

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


/* ***********************************************************************************  */
/* seiwd()                                                                              */
/* Main SEI watchdog routine.                                                           */
/* ***********************************************************************************  */

inline int seiwd(float input_sensors[], int max_var[])
{
int watchdog_byte;

        watchdog_byte = 0;

        /* Compare the STS sensor inputs */
        if(input_sensors[0] > max_var[0]) watchdog_byte |= 0x1;         /* Bit 0 */
        if(input_sensors[1] > max_var[0]) watchdog_byte |= 0x2;         /* Bit 1 */
        if(input_sensors[2] > max_var[0]) watchdog_byte |= 0x4;         /* Bit 2 */

        /* Compare the POS V sensor inputs */
        if(input_sensors[3] > max_var[1]) watchdog_byte |= 0x8;         /* Bit 3 */
        if(input_sensors[4] > max_var[1]) watchdog_byte |= 0x10;        /* Bit 4 */
        if(input_sensors[5] > max_var[1]) watchdog_byte |= 0x20;        /* Bit 5 */
        if(input_sensors[6] > max_var[1]) watchdog_byte |= 0x40;        /* Bit 6 */

        /* Compare the POS H sensor inputs */
        if(input_sensors[7] > max_var[2]) watchdog_byte  |= 0x80;       /* Bit 7 */
        if(input_sensors[8] > max_var[2]) watchdog_byte  |= 0x100;      /* Bit 8 */
        if(input_sensors[9] > max_var[2]) watchdog_byte  |= 0x200;      /* Bit 9 */
        if(input_sensors[10] > max_var[2]) watchdog_byte |= 0x400;      /* Bit 10 */

        /* Compare the GEO V sensor inputs */
        if(input_sensors[11] > max_var[3]) watchdog_byte |= 0x10000;    /* Bit 16 */
        if(input_sensors[12] > max_var[3]) watchdog_byte |= 0x20000;    /* Bit 17 */
        if(input_sensors[13] > max_var[3]) watchdog_byte |= 0x40000;    /* Bit 18 */
        if(input_sensors[14] > max_var[3]) watchdog_byte |= 0x80000;    /* Bit 19 */

        /* Compare the GEO H sensor inputs */
        if(input_sensors[15] > max_var[4]) watchdog_byte |= 0x100000;   /* Bit 20 */
        if(input_sensors[16] > max_var[4]) watchdog_byte |= 0x200000;   /* Bit 21 */
        if(input_sensors[17] > max_var[4]) watchdog_byte |= 0x400000;   /* Bit 22 */
        if(input_sensors[18] > max_var[4]) watchdog_byte |= 0x800000;   /* Bit 23 */

        return(watchdog_byte);
}

inline float gainRamp(float gainReq, int rampTime, int id, int sw)
{

static int dir[4][10];
static float inc[4][10];
static float gainFinal[4][10];
static float gainOut[4][10];

        if(gainFinal[sw][id] != gainReq)
        {
                inc[sw][id] = rampTime * 2048;
                inc[sw][id] = (gainReq - gainOut[sw][id]) / inc[sw][id];
                if(inc[sw][id] <= 0.0) dir[sw][id] = 0;
                else dir[sw][id] = 1;
                gainFinal[sw][id] = gainReq;
        }
        if(gainFinal[sw][id] == gainOut[sw][id])
        {
                return(gainOut[sw][id]);
        }
        gainOut[sw][id] += inc[sw][id];
        if((dir[sw][id] == 1) && (gainOut[sw][id] >= gainFinal[sw][id]))
                gainOut[sw][id] = gainFinal[sw][id];
        if((dir[sw][id] == 0) && (gainOut[sw][id] <= gainFinal[sw][id]))
                gainOut[sw][id] = gainFinal[sw][id];
        return(gainOut[sw][id]);
}


/* ******************************************************************** */
/* Main HEPI control thread						*/
/* ******************************************************************** */
void feCode(double adc[][32],int dac[][16],FILT_MOD *dsp,COEF *dspCoeff,CDS_EPICS *pLocalEpics)
{
int ii,jj,kk;			/* Loop counters.			*/
int binaryOut;			/* Output to xycom220 module.		*/
static int firNum;
float posSen[4][8];		/* Position sensors from ICS110B mods	*/
float geoSen[4][8];		/* Geophone sensors from Pentek mods	*/
double pos[8];
double posOut[8];
double geo[8];
double geoTilt[4][8];
double geoOut[8];
double modal[8];
double imodalXform[8];
double dcmXform[8];
double actIn[8];
double actOut[8];
double stsXRY[4];
double stsYRX[4];
float wdVal[4][24];		/* Values to compare to WD limits (pre-filters)	*/
int wdWord[4][2];		/* WD output word, indicates cause of trips.	*/
float wdValFilt[4][24];		/* Values to compare to WD limits (post-filters) */
float wdValFiltMax[4][24];	/* Values to compare to WD limits (post-filters) */
int wdLimit[4][6];		/* WD comparison limits (pre-filters).		*/
int wdLimitF[4][6];		/* WD comparison limits (post-filters).         */
int wdReset[4];			/* WD Resets.					*/
static int wdTrip[4];		/* WD Trip indicators.				*/
static int wdTimer[4];		/* WD Trip indicators.				*/
float geoOutAvg[4][2][3];
float tcGain[4];
float outGain[4];
float loopGain[4];
static float ramp[4];
float firDs;
float firUs;
float firComp;
static int cycle;
static float firOutput[2][2][32];
static float firHistory[4][32][1024];
double fmInput;
int chamber;
int chamSize;
float stsIn[6];
float stsOut[6];
float sts2pos[4][8];            /* STS input to sts2pos matrix          */
static double eOut;
static double tcMatOut[8];
static double tidal[3];
static float wdSenCounter[4][24];
static double rmsTmp;
static int output;
static int hepiMain[4];
static int hepiTilt[4];
static int hepiLoop[4];
static float firGain[2][2];
static float firOffset[2][2];
static float wdGain[4];
static float wdTarget[4];

#if 0
	/* Read/Write Xycom modules. */
	/* NOTE: HEPI 1 does not contain these modules */
	if(intrProcessCycle == 0) 
	{
	  /* Read Xycom 212 binary inputs (Pump station status) 	*/
	  pLocalEpics->comms[coff].hepiBinary[0] = (xycom212[0]->lowValue & 0xffff);
	  /* Write Xycom 220 binary outputs (STS control bits)	*/
	  binaryOut = pLocalEpics->comms[coff].hepiSwitch[9] +
		pLocalEpics->comms[coff].hepiSwitch[10] * 2 +
		pLocalEpics->comms[coff].hepiSwitch[11] * 4 +
		pLocalEpics->comms[coff].hepiSwitch[12] * 8;
	  xycom[0]->lowValue = (binaryOut & 0xffff);
	  xycom[0]->hiValue = ((binaryOut >> 16) & 0xffff);
	}
#endif

	for(ii=0;ii<CHAMBERS;ii++)
	{
	  /* Do ramp calcs for switching to/from STS FIR/STS IIR */
	  if(pLocalEpics->hepi[ii].hepiSwitch[5] == 0)
	  {
	    ramp[ii] += 1.0/pLocalEpics->hepi[ii].rampTime;
	    if(ramp[ii] > 1.0) ramp[ii] = 1.0;
	  }
	  else
	  {
	    ramp[ii] -= 1.0/pLocalEpics->hepi[ii].rampTime;
	    if(ramp[ii] < 0.0) ramp[ii] = 0.0;
	  }
	  
	  /* Compute tilt correction gain */
	  tcGain[ii] = gainRamp(pLocalEpics->hepi[ii].tcGain,
			pLocalEpics->hepi[ii].tramp[0], ii,0);

	  if(tcGain[ii] == pLocalEpics->hepi[ii].tcGain)
			pLocalEpics->hepi[ii].tramp[3] &= ~0x1;
	  else pLocalEpics->hepi[ii].tramp[3] |= 0x1;

	  /* Compute loop gain */
	  loopGain[ii] = gainRamp(pLocalEpics->hepi[ii].loopGain,
			pLocalEpics->hepi[ii].tramp[1], ii,1);

	  if(loopGain[ii] == pLocalEpics->hepi[ii].loopGain)
			pLocalEpics->hepi[ii].tramp[3] &= ~0x2;
	  else pLocalEpics->hepi[ii].tramp[3] |= 2;

	  /* Compute output gain */
	  outGain[ii] = gainRamp(pLocalEpics->hepi[ii].gain,
			pLocalEpics->hepi[ii].tramp[2], ii,2);

	  if(outGain[ii] == pLocalEpics->hepi[ii].gain)
			pLocalEpics->hepi[ii].tramp[3] &= ~0x4;
	  else pLocalEpics->hepi[ii].tramp[3] |= 4;
	  
	  /* Compute STS watchdog gain */
	  wdGain[ii] = gainRamp(wdTarget[ii],10,ii,3);
	}


	  /* ***************************************************************
		Have to shuffle data around to match final wiring, as above for 
		Pentek channels. New connection is:
		0 - H4		16 - STS Mass Pos X
		1 - H3		17 - STS Mass Pos Y
		2 - H2		18 - STS Mass Pos Z
		3 - H1		19 - Spare 2
		4 - V4	
		5 - V3
		6 - V2
		7 - V1
		8 - Witness 4
		9 - Witness 3
	       10 - Witness 2
	       11 - Witness 1
	       12 - STS X
	       13 - STS Y
	       14 - STS Z
	       15 - Spare 1
	  ***************************************************************** */


	for(ii=0;ii<STS_COUNT;ii++)
	{
	   kk = ii * 16;
	   pLocalEpics->sts[ii].stsIn[0] = adc[0][12+kk];
	   pLocalEpics->sts[ii].stsIn[1] = adc[0][13+kk];
	   pLocalEpics->sts[ii].stsIn[2] = adc[0][14+kk];
	}
	for(ii=0;ii<CHAMBERS;ii++)
	{
		kk = ii * 16;
	  	posSen[ii][0] = adc[0][kk+7];
	  	posSen[ii][1] = adc[0][kk+6];
	  	posSen[ii][2] = adc[0][kk+5];
	  	posSen[ii][3] = adc[0][kk+4];
	  	posSen[ii][4] = adc[0][kk+3];
	  	posSen[ii][5] = adc[0][kk+2];
	  	posSen[ii][6] = adc[0][kk+1];
	  	posSen[ii][7] = adc[0][kk];
	  	geoSen[ii][0] = adc[0][kk+7];
	  	geoSen[ii][1] = adc[0][kk+6];
	  	geoSen[ii][2] = adc[0][kk+5];
	  	geoSen[ii][3] = adc[0][kk+4];
	  	geoSen[ii][4] = adc[0][kk+3];
	  	geoSen[ii][5] = adc[0][kk+2];
	  	geoSen[ii][6] = adc[0][kk+1];
	  	geoSen[ii][7] = adc[0][kk];
	}
	for(ii=0;ii<STS_COUNT;ii++)
	{
	   for(jj=0;jj<2;jj++)
	   {
		   kk = jj + ii * 6;
		   /* Run FIR Downsample and compensation filters ***************** */
		   /* Run downsample filter */
		   firDs = filterModuleD(dsp,dspCoeff,FIR_DF_X1+kk,pLocalEpics->sts[ii].stsIn[jj],0);

		   /* Add in fir filter offset */
		   firDs += firOffset[ii][jj];

		   /* Run fir compensation filter */
		   firComp = filterModuleD(dsp,dspCoeff,FIR_CF_X1+kk,pLocalEpics->sts[ii].stsIn[jj],0);

		   /* Run PFIR at downsample 64Hz ********************************** */
		   if((cycle % 32) == 0)
		   {
			firNum = (cycle / 32) % 32;
			firOutput[ii][jj][firNum] = 
				fir_filter(firDs, &firCoeff[0], FIR_TAPS, firHistory[ii*2+jj][firNum]);
		   }

		   /* Run upsample filter and output sum of upsample and comp filters ******************** */
		   /* Load fir upsampling filter w/output FIR * FIR gain setting */
		   fmInput = firOutput[ii][jj][firNum] * firGain[ii][jj];
		   /* Run fir upsampling filter */
		   firUs = filterModuleD(dsp,dspCoeff,FIR_UF_X1+kk,fmInput, 0);

		   /* Output upsample and comp filter results to RFM */
		   pLocalEpics->sts[ii].stsOut[jj] = (float) (firUs + firComp);
   	   }

	}

	for(chamber=0;chamber<CHAMBERS;chamber++)
	{
	  chamSize = CHAM_FILT * chamber;
	  for(ii=0;ii<3;ii++)
	  {
		stsIn[ii] = pLocalEpics->sts[0].stsIn[ii] * pLocalEpics->hepi[chamber].sts_in_matrix[0][ii] +
			    pLocalEpics->sts[1].stsIn[ii] * pLocalEpics->hepi[chamber].sts_in_matrix[1][ii];
	  }
          /* STS Sensor Filtering */
          for(ii=0;ii<3;ii++)
          {
          	kk = ii + STS_X + chamSize;
          	stsOut[ii] = filterModuleD(dsp,dspCoeff,kk,stsIn[ii], 0);
		wdVal[chamber][ii] = stsIn[ii];
		if(wdVal[chamber][ii] < 0.0) wdVal[chamber][ii] *= -1;
		if((wdVal[chamber][ii] > 32767) || (wdVal[chamber][ii] == 0))
			wdSenCounter[chamber][ii] ++;
          	kk = ii + STS_XE + chamSize;
		eOut = filterModuleD(dsp,dspCoeff,kk,stsIn[ii], 0);
		if(eOut < 0.0) eOut *= -1;
		wdValFilt[chamber][ii] = eOut;
		if(wdValFilt[chamber][ii] > wdValFiltMax[chamber][ii])
                	wdValFiltMax[chamber][ii] = wdValFilt[chamber][ii];
          }

          /* Max Vel Filtering */
	  /* Function deleted but set switch for continuity */
          pLocalEpics->hepi[chamber].hepiBinary[1] |= 0x1;  /* Enable ff by default         */

          /* Filter the outputs of the FIR filter path on an individual chamber basis.
           Only STS X and Y go through FIR filters.                                             */
          for(ii=0;ii<2;ii++)
          {
		kk = ii + FIR_X_OUT + chamSize;
		/* Matrix in outputs from both STS2 FIR filters */
		fmInput =
			pLocalEpics->sts[0].stsOut[ii] * pLocalEpics->hepi[chamber].sts_in_matrix[0][ii+3] +
			pLocalEpics->sts[1].stsOut[ii] * pLocalEpics->hepi[chamber].sts_in_matrix[1][ii+3];
		stsOut[ii+3] = filterModuleD(dsp,dspCoeff,kk,fmInput, 0);
		stsOut[ii+3] *= wdGain[chamber];
          }

          /* Added STSX>RY and STSY>RX filtering */
          fmInput = (stsOut[0] * ramp[chamber] + stsOut[3] * (1 - ramp[chamber]));
          stsXRY[chamber] = filterModuleD(dsp,dspCoeff,STS_X_RY+chamSize,fmInput,0);
          fmInput = (stsOut[1] * ramp[chamber] + stsOut[4] * (1 - ramp[chamber]));
          stsYRX[chamber] = filterModuleD(dsp,dspCoeff,STS_Y_RX+chamSize,fmInput,0);

          /* STS to Position Xform */
          /* The switching between using the STS IIR filter outputs or the STS FIR filter outputs
             has a ramping feature, included in the matrix calc below.                            */
          for(ii=0;ii<8;ii++)
          {
            sts2pos[chamber][ii] =
                ((stsOut[0] * ramp[chamber] + stsOut[3] * (1 - ramp[chamber])) *
                	pLocalEpics->hepi[chamber].sts_matrix[ii][0]) +
                ((stsOut[1] * ramp[chamber] + stsOut[4] * (1 - ramp[chamber])) *
                	pLocalEpics->hepi[chamber].sts_matrix[ii][1]) +
                (stsOut[2] * pLocalEpics->hepi[chamber].sts_matrix[ii][2]);
          }

          /* Position Filtering *************************************************** */
          for(ii=0;ii<8;ii++)
          {
		kk = ii+chamSize;
		pos[ii] = filterModuleD(dsp,dspCoeff,kk,posSen[chamber][ii],0);
		  wdVal[chamber][ii+3] = posSen[chamber][ii];
		  if(wdVal[chamber][ii+3] < 0.0) wdVal[chamber][ii+3] *= -1;
		  if((wdVal[chamber][ii+3] > 32766) || (wdVal[chamber][ii+3] == 0))
			wdSenCounter[chamber][ii+3] ++;
		  kk = ii + POS_V1E;
		  eOut = (float)filterModuleD(dsp,dspCoeff,kk,posSen[chamber][ii], 0);
		  if(eOut < 0.0) eOut *= -1;
		  wdValFilt[chamber][ii+3] = eOut;
		  if(wdValFilt[chamber][ii+3] > wdValFiltMax[chamber][ii+3])
			wdValFiltMax[chamber][ii+3] = wdValFilt[chamber][ii+3];
          }
          /* Tilt Correction Xform ************************************************* */
          for(ii=0;ii<8;ii++)
          {
             	tcMatOut[ii] =
			(geoTilt[chamber][0] * pLocalEpics->hepi[chamber].tc_matrix[0][ii]) +
			(geoTilt[chamber][1] * pLocalEpics->hepi[chamber].tc_matrix[1][ii]) +
			(geoTilt[chamber][2] * pLocalEpics->hepi[chamber].tc_matrix[2][ii]) +
			(geoTilt[chamber][3] * pLocalEpics->hepi[chamber].tc_matrix[3][ii]) +
			(geoTilt[chamber][4] * pLocalEpics->hepi[chamber].tc_matrix[4][ii]) +
			(geoTilt[chamber][5] * pLocalEpics->hepi[chamber].tc_matrix[5][ii]) +
			(geoTilt[chamber][6] * pLocalEpics->hepi[chamber].tc_matrix[6][ii]) +
			(geoTilt[chamber][7] * pLocalEpics->hepi[chamber].tc_matrix[7][ii]);
             	/* Multiply by TC gain slider */
           	tcMatOut[ii] *= tcGain[chamber];
		pLocalEpics->hepi[chamber].tcMatOut[ii] = (float)tcMatOut[ii];
           	/* Set outputs to zero if TC or Master Switch is OFF */
           	if(hepiTilt[chamber] == 0) tcMatOut[ii] = 0.0;
          }
          /* Geophone Filtering *************************************************** */
          for(ii=0;ii<8;ii++)
          {
          	kk = ii + GEO_V1 + chamSize;
          	fmInput = geoSen[chamber][ii] + tcMatOut[ii];
          	geo[ii] = filterModuleD(dsp,dspCoeff,kk,fmInput,0);
          	wdVal[chamber][ii+11] = geoSen[chamber][ii];
		if(wdVal[chamber][ii+11] < 0.0) wdVal[chamber][ii+11] *= -1;
          	if((wdVal[chamber][ii+11] > 32766) || (wdVal[chamber][ii+11] == 0))
                	wdSenCounter[chamber][ii+11] ++;
          	kk = ii+ GEO_V1E + chamSize;
          	pLocalEpics->hepi[chamber].geoSen[ii] = geoSen[chamber][ii];
          	eOut = (float)filterModuleD(dsp,dspCoeff,kk,fmInput,0);
		if(eOut < 0.0) eOut *= -1;
          	wdValFilt[chamber][ii+11] = eOut;
          	if(wdValFilt[chamber][ii+11] > wdValFiltMax[chamber][ii+11])
                	wdValFiltMax[chamber][ii+11] = wdValFilt[chamber][ii+11];
          }
          /* Subtract out STS positions prior to Position to Modal Xform */
          for(ii=0;ii<8;ii++) pos[ii] -= sts2pos[chamber][ii];

          /* Position Local to Modal Xform ************************************** */
          for(ii=0;ii<8;ii++)
          {
           posOut[ii] =
                (pos[0] * pLocalEpics->hepi[chamber].pos_matrix[0][ii]) +
                (pos[1] * pLocalEpics->hepi[chamber].pos_matrix[1][ii]) +
                (pos[2] * pLocalEpics->hepi[chamber].pos_matrix[2][ii]) +
                (pos[3] * pLocalEpics->hepi[chamber].pos_matrix[3][ii]) +
                (pos[4] * pLocalEpics->hepi[chamber].pos_matrix[4][ii]) +
                (pos[5] * pLocalEpics->hepi[chamber].pos_matrix[5][ii]) +
                (pos[6] * pLocalEpics->hepi[chamber].pos_matrix[6][ii]) +
                (pos[7] * pLocalEpics->hepi[chamber].pos_matrix[7][ii]);
          }


          /* Geophone Local to Modal Xform ************************************* */
          for(ii=0;ii<8;ii++)
          {
           geoOut[ii] =
                (geo[0] * pLocalEpics->hepi[chamber].geo_matrix[0][ii]) +
                (geo[1] * pLocalEpics->hepi[chamber].geo_matrix[1][ii]) +
                (geo[2] * pLocalEpics->hepi[chamber].geo_matrix[2][ii]) +
                (geo[3] * pLocalEpics->hepi[chamber].geo_matrix[3][ii]) +
                (geo[4] * pLocalEpics->hepi[chamber].geo_matrix[4][ii]) +
                (geo[5] * pLocalEpics->hepi[chamber].geo_matrix[5][ii]) +
                (geo[6] * pLocalEpics->hepi[chamber].geo_matrix[6][ii]) +
                (geo[7] * pLocalEpics->hepi[chamber].geo_matrix[7][ii]);
          }

#if 0
          /* Do LSC filtering ************************************************* */
          for(ii=0;ii<3;ii++)
          {
		  kk = ii + LSC_X + chamSize;
		  tidal[ii] = filterModuleD(dsp,dspCoeff,kk,lscPostMtrx[chamber][ii],0);
          }
#endif

          /* Position Modal Filtering **************************************** */
          for(ii=0;ii<8;ii++)
          {
		  kk = ii+POS_X+chamSize;
		  if(ii==3) posOut[ii] += stsYRX[chamber];
		  if(ii==4) posOut[ii] += stsXRY[chamber];
		  pos[ii] = filterModuleD(dsp,dspCoeff,kk,posOut[ii],0);
          }

          /* Geophone Modal Filtering **************************************** */
          for(ii=0;ii<8;ii++)
          {
		  kk = ii+GEO_X+chamSize;
		  geo[ii] = filterModuleD(dsp,dspCoeff,kk,geoOut[ii],0);
          }

          /* Geophone RMS Filtering Part 1 *********************************** */
          for(ii=0;ii<3;ii++)
          {
		  kk = ii+GEO_X_RMS+chamSize;
		  rmsTmp = filterModuleD(dsp,dspCoeff,kk,geoOut[ii],0);
		  if(rmsTmp > 2000) rmsTmp = 2000;
		  if(rmsTmp < -2000) rmsTmp = 2000;
		  rmsTmp = rmsTmp * rmsTmp;
		  geoOutAvg[chamber][0][ii] = rmsTmp * .00005 + geoOutAvg[chamber][0][ii] * 0.99995;
		  pLocalEpics->hepi[chamber].geoRms[ii] = lsqrt(geoOutAvg[chamber][0][ii]);
          }

          /* Geophone RMS Filtering Part 2 ********************************** */
          for(ii=0;ii<3;ii++)
          {
		  kk = ii+GEO_X_RMS2+chamSize;
		  rmsTmp = filterModuleD(dsp,dspCoeff,kk,geoOut[ii],0);
		  if(rmsTmp > 2000) rmsTmp = 2000;
		  if(rmsTmp < -2000) rmsTmp = 2000;
		  rmsTmp = rmsTmp * rmsTmp;
		  geoOutAvg[chamber][1][ii] = rmsTmp * .00005 + geoOutAvg[chamber][1][ii] * 0.99995;
		  pLocalEpics->hepi[chamber].geoRms[ii+3] = lsqrt(geoOutAvg[chamber][1][ii]);
          }

          /* Set pos matrix outputs to zero if switch is OFF */
          if(!pLocalEpics->hepi[chamber].hepiSwitch[2])
                for(ii=0;ii<8;ii++) pos[ii] = 0.0;

          /* Set geo matrix outputs to zero if switch is OFF */
          if(!pLocalEpics->hepi[chamber].hepiSwitch[3])
                for(ii=0;ii<8;ii++) geo[ii] = 0.0;

          /*  Combined Modal Filtering */
          for(ii=0;ii<8;ii++)
          {
		 kk = ii+MODAL_X+chamSize;
		 fmInput = geo[ii] + pos[ii];
		 /* Add LSC to MODAL X,Y,Z */
		 if(ii<3) fmInput += tidal[ii];
		 modal[ii] = filterModuleD(dsp,dspCoeff,kk,fmInput,0);
          }

          /* Inverse Modal Xform */
          for(ii=0;ii<8;ii++)
          {
		   imodalXform[ii] =
			(modal[0] * pLocalEpics->hepi[chamber].act_matrix[0][ii]) +
			(modal[1] * pLocalEpics->hepi[chamber].act_matrix[1][ii]) +
			(modal[2] * pLocalEpics->hepi[chamber].act_matrix[2][ii]) +
			(modal[3] * pLocalEpics->hepi[chamber].act_matrix[3][ii]) +
			(modal[4] * pLocalEpics->hepi[chamber].act_matrix[4][ii]) +
			(modal[5] * pLocalEpics->hepi[chamber].act_matrix[5][ii]) +
			(modal[6] * pLocalEpics->hepi[chamber].act_matrix[6][ii]) +
			(modal[7] * pLocalEpics->hepi[chamber].act_matrix[7][ii]);
		   pLocalEpics->hepi[chamber].modalMatOut[ii] = (float)imodalXform[ii];
		   imodalXform[ii] *= loopGain[chamber];
          }
          if(!hepiLoop[chamber]) for(ii=0;ii<8;ii++) imodalXform[ii] = 0.0;

          /* DC Modal Bias Xform */
          if(pLocalEpics->hepi[chamber].hepiSwitch[8])
          {
                for(ii=0;ii<8;ii++)
                {
                   dcmXform[ii] =
                        (pLocalEpics->hepi[chamber].dcModalBias[0][0] * pLocalEpics->hepi[chamber].dcm_matrix[ii][0]) +
                        (pLocalEpics->hepi[chamber].dcModalBias[0][1] * pLocalEpics->hepi[chamber].dcm_matrix[ii][1]) +
                        (pLocalEpics->hepi[chamber].dcModalBias[0][2] * pLocalEpics->hepi[chamber].dcm_matrix[ii][2]) +
                        (pLocalEpics->hepi[chamber].dcModalBias[0][3] * pLocalEpics->hepi[chamber].dcm_matrix[ii][3]) +
                        (pLocalEpics->hepi[chamber].dcModalBias[0][4] * pLocalEpics->hepi[chamber].dcm_matrix[ii][4]) +
                        (pLocalEpics->hepi[chamber].dcModalBias[0][5] * pLocalEpics->hepi[chamber].dcm_matrix[ii][5]);
                }
          }
          else for(ii=0;ii<8;ii++) dcmXform[ii] = 0.0;

          /* Do TC filtering for next cycle *************************************************** */
          for(ii=0;ii<8;ii++)
          {
		  kk = ii + TILTCORR_V1 + chamSize;
		  geoTilt[chamber][ii] = filterModuleD(dsp,dspCoeff,kk,imodalXform[ii],0);
          }

          /*  ACT Filtering and DAC outputs ************************************************** */
          for(ii=0;ii<8;ii++)
          {
		   kk = ii+ACT_V1+chamSize;
		   actIn[ii] = dcmXform[ii] + imodalXform[ii];
		   actOut[ii] = filterModuleD(dsp,dspCoeff,kk,actIn[ii],0);

		   /* Load DAC Outputs */
		   if(!hepiMain[chamber]) output = 0;
		   else output = (int)(actOut[ii] * outGain[chamber]);
		   dac[0][ii+chamber*8] = output;
		   pLocalEpics->hepi[chamber].hepiOutput[ii] = dac[0][ii+chamber*8];
          }

	} /* End chamber loop */

	/* Perform WatchDog Functions */
	for(ii=0;ii<CHAMBERS;ii++)
	{
		wdWord[ii][1] = 0;
		wdWord[ii][1] = seiwd(wdValFilt[ii],pLocalEpics->hepi[ii].wdLimitF);
		if((wdWord[ii][1]) && (wdTrip[ii] == 0)) 
		{
			pLocalEpics->hepi[ii].wdTrip = 3;
			wdTarget[ii] = 0.0;
			wdTrip[ii] = 1;
			wdTimer[ii] = 0;
			pLocalEpics->hepi[ii].wdLoBytesF =  wdWord[ii][1] & 0x7ff;
			pLocalEpics->hepi[ii].wdHiBytesF = (wdWord[ii][1] >> 16) & 0xff;
		}
		if((wdWord[ii][1]) && (wdTrip[ii] == 2)) 
		{
			pLocalEpics->hepi[ii].wdTrip = 15;
			wdTrip[ii] = 3;
			wdTimer[ii] = 0;
			pLocalEpics->hepi[ii].wdLoBytesF =  wdWord[ii][1] & 0x7ff;
			pLocalEpics->hepi[ii].wdHiBytesF = (wdWord[ii][1] >> 16) & 0xff;
			hepiLoop[ii] = 0;
			pLocalEpics->hepi[ii].hepiSwitch[6] = 0;
		}
		if((wdWord[ii][1]) && (wdTrip[ii] == 4)) 
		{
			pLocalEpics->hepi[ii].wdTrip = 31;
			wdTrip[ii] = 5;
			wdTimer[ii] = 0;
			pLocalEpics->hepi[ii].wdLoBytesF =  wdWord[ii][1] & 0x7ff;
			pLocalEpics->hepi[ii].wdHiBytesF = (wdWord[ii][1] >> 16) & 0xff;
			hepiMain[ii] = 0;
			pLocalEpics->hepi[ii].hepiSwitch[0] = 0;
		}


	}

	if(cycle == 0)
	{
		for(ii=0;ii<CHAMBERS;ii++)
		{
			wdWord[ii][0] = 0;
			wdWord[ii][0] = seiwd(wdSenCounter[ii],pLocalEpics->hepi[ii].wdLimit);
			if((wdTrip[ii] == 0) && (wdWord[ii][0])) 
			{
				pLocalEpics->hepi[ii].wdTrip = 1;
				wdTarget[ii] = 0.0;
				wdTimer[ii] = 0;
				wdTrip[ii] = 1;
				pLocalEpics->hepi[ii].wdLoBytes =  wdWord[ii][0] & 0x7ff;
				pLocalEpics->hepi[ii].wdHiBytes = (wdWord[ii][0] >> 16) & 0xff;
			}
			for(jj=0;jj<19;jj++)
			{
				pLocalEpics->hepi[ii].wdFabs[0][jj] = wdSenCounter[ii][jj];
				wdSenCounter[ii][jj] = 0.0;
				pLocalEpics->hepi[ii].wdFabs[1][jj] = wdValFiltMax[ii][jj];
				wdValFiltMax[ii][jj] = 0.0;
			}
			if((wdTimer[ii] > 30) && (wdTrip[ii] == 1))
			{
				wdTrip[ii] = 2;
				wdTimer[ii] = 0;
				// pLocalEpics->hepi[ii].wdTrip = 7;
			}
			if((wdTimer[ii] > 30) && (wdTrip[ii] == 3))
			{
				wdTrip[ii] = 4;
				wdTimer[ii] = 0;
			}
			if(pLocalEpics->hepi[ii].wdReset == 1)
			{
				pLocalEpics->hepi[ii].wdTrip = 0;
				pLocalEpics->hepi[ii].wdReset = 0;
				wdTrip[ii] = 0;
				pLocalEpics->hepi[ii].wdLoBytesF =  0;
				pLocalEpics->hepi[ii].wdHiBytesF = 0;
				pLocalEpics->hepi[ii].wdLoBytes =  0;
				pLocalEpics->hepi[ii].wdHiBytes = 0;
			}
		   	if((!pLocalEpics->hepi[ii].wdTrip)) wdTarget[ii] = 1.0;
			else wdTimer[ii] ++;
			if(wdGain[ii] == 0.0) pLocalEpics->hepi[ii].wdTrip |= 4;
			else pLocalEpics->hepi[ii].wdTrip &= ~4;
		}

	}
	if(cycle == 1)
	{
		for(ii=0;ii<CHAMBERS;ii++)
		{
		   if(pLocalEpics->hepi[ii].hepiSwitch[0])
		   {
			if(wdTrip[ii] < 5)
			   hepiMain[ii] ^= 1;
			   pLocalEpics->hepi[ii].hepiSwitch[0] = 0;
		   }
		   if(pLocalEpics->hepi[ii].hepiSwitch[6])
		   {
			if(wdTrip[ii] < 3)
			   hepiLoop[ii] ^= 1;
			   pLocalEpics->hepi[ii].hepiSwitch[6] = 0;
		   }
		   if(pLocalEpics->hepi[ii].hepiSwitch[4])
		   {
			   hepiTilt[ii] ^= 1;
			   pLocalEpics->hepi[ii].hepiSwitch[4] = 0;
		   }
		   if(hepiMain[ii] == 0) hepiTilt[ii] = 0;
		   pLocalEpics->hepi[ii].hepiSwitch[13] = hepiMain[ii] + hepiLoop[ii] * 2 + hepiTilt[ii] * 4;

		}
	}
	if(cycle == 2)
	{
		if(pLocalEpics->sts[0].firSw1[0])
		{
			pLocalEpics->sts[0].firSw1R[0] ^= pLocalEpics->sts[0].firSw1[0];
			pLocalEpics->sts[0].firSw1[0] = 0;
		}
		if(pLocalEpics->sts[0].firSw1[1])
		{
			pLocalEpics->sts[0].firSw1R[1] ^= pLocalEpics->sts[0].firSw1[1];
			pLocalEpics->sts[0].firSw1[1] = 0;
		}
		if(pLocalEpics->sts[1].firSw1[0])
		{
			pLocalEpics->sts[1].firSw1R[0] ^= pLocalEpics->sts[1].firSw1[0];
			pLocalEpics->sts[1].firSw1[0] = 0;
		}
		if(pLocalEpics->sts[1].firSw1[1])
		{
			pLocalEpics->sts[1].firSw1R[1] ^= pLocalEpics->sts[1].firSw1[1];
			pLocalEpics->sts[1].firSw1[1] = 0;
		}
		jj = ((pLocalEpics->sts[0].firSw1R[0] >> 3) & 1);
		firGain[0][0] = pLocalEpics->sts[0].firGain[0] * jj;
		jj = ((pLocalEpics->sts[0].firSw1R[1] >> 3) & 1);
		firGain[0][1] = pLocalEpics->sts[0].firGain[1] * jj;
		jj = ((pLocalEpics->sts[1].firSw1R[0] >> 3) & 1);
		firGain[1][0] = pLocalEpics->sts[1].firGain[0] * jj;
		jj = ((pLocalEpics->sts[1].firSw1R[1] >> 3) & 1);
		firGain[1][1] = pLocalEpics->sts[1].firGain[1] * jj;
		firOffset[0][0] = pLocalEpics->sts[0].firOffset[0] * (pLocalEpics->sts[0].firSw1R[0] & 1);
		firOffset[0][1] = pLocalEpics->sts[0].firOffset[1] * (pLocalEpics->sts[0].firSw1R[1] & 1);
		firOffset[1][0] = pLocalEpics->sts[1].firOffset[0] * (pLocalEpics->sts[1].firSw1R[0] & 1);
		firOffset[1][1] = pLocalEpics->sts[1].firOffset[1] * (pLocalEpics->sts[1].firSw1R[1] & 1);
	}
	cycle = (cycle + 1) % 2048;


}

