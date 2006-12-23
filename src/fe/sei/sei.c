// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


#ifdef SERVO32K
	#define FE_RATE	32768
#endif
#ifdef SERVO16K
	#define FE_RATE	16382
#endif
#ifdef SERVO2K
	#define FE_RATE	2048
#endif


void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dsp_ptr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii,jj;

double bsc_hp;
double bsc_loop_gain[8];
float BSC_LOOP_GAIN_CALC;
double bsc_loop_sw[16];
double bsc_mtrx[8][8];
double bsc_rx;
double bsc_ry;
double bsc_rz;
double bsc_sum;
double bsc_sum1;
double bsc_sum2;
double bsc_sum3;
double bsc_sum4;
double bsc_sum5;
double bsc_sum6;
double bsc_sum7;
double bsc_tiltcorr_gain[8];
float BSC_TILTCORR_GAIN_CALC;
double bsc_tiltcorr_h1;
double bsc_tiltcorr_h2;
double bsc_tiltcorr_h3;
double bsc_tiltcorr_h4;
double bsc_tiltcorr_mtrx[8][8];
double bsc_tiltcorr_sw[8];
double bsc_tiltcorr_v1;
double bsc_tiltcorr_v2;
double bsc_tiltcorr_v3;
double bsc_tiltcorr_v4;
double bsc_vp;
double bsc_x;
double bsc_y;
double bsc_z;
double bsc_act_gain[8];
float BSC_ACT_GAIN_CALC;
double bsc_act_h1;
double bsc_act_h2;
double bsc_act_h3;
double bsc_act_h4;
double bsc_act_sw[8];
double bsc_act_sum;
double bsc_act_sum1;
double bsc_act_sum2;
double bsc_act_sum3;
double bsc_act_sum4;
double bsc_act_sum5;
double bsc_act_sum6;
double bsc_act_sum7;
double bsc_act_v1;
double bsc_act_v2;
double bsc_act_v3;
double bsc_act_v4;
double bsc_dc_bias_mtrx[8][6];
double bsc_dc_bias_sw[8];
double bsc_fir_mtrx[2][4];
double bsc_fir_x_out;
double bsc_fir_y_out;
double bsc_geo_h1;
double bsc_geo_h2;
double bsc_geo_h3;
double bsc_geo_h4;
double bsc_geo_hp;
double bsc_geo_mtrx[8][8];
double bsc_geo_rx;
double bsc_geo_ry;
double bsc_geo_rz;
double bsc_geo_sw[8];
double bsc_geo_sum;
double bsc_geo_sum1;
double bsc_geo_sum2;
double bsc_geo_sum3;
double bsc_geo_sum4;
double bsc_geo_sum5;
double bsc_geo_sum6;
double bsc_geo_sum7;
double bsc_geo_v1;
double bsc_geo_v2;
double bsc_geo_v3;
double bsc_geo_v4;
double bsc_geo_vp;
double bsc_geo_x;
double bsc_geo_x_rms;
double bsc_geo_x_rms2;
double bsc_geo_y;
double bsc_geo_y_rms;
double bsc_geo_y_rms2;
double bsc_geo_z;
double bsc_geo_z_rms;
double bsc_geo_z_rms2;
float bsc_geo_rms1;
static float bsc_geo_rms1_avg;
float bsc_geo_rms2;
static float bsc_geo_rms2_avg;
float bsc_geo_rms3;
static float bsc_geo_rms3_avg;
float bsc_geo_rms4;
static float bsc_geo_rms4_avg;
float bsc_geo_rms5;
static float bsc_geo_rms5_avg;
float bsc_geo_rms6;
static float bsc_geo_rms6_avg;
double bsc_pos_h1;
double bsc_pos_h2;
double bsc_pos_h3;
double bsc_pos_h4;
double bsc_pos_hp;
double bsc_pos_mtrx[8][8];
double bsc_pos_norm[16];
double bsc_pos_rx;
double bsc_pos_ry;
double bsc_pos_rz;
double bsc_pos_sw[8];
double bsc_pos_sum;
double bsc_pos_sum1;
double bsc_pos_v1;
double bsc_pos_v2;
double bsc_pos_v3;
double bsc_pos_v4;
double bsc_pos_vp;
double bsc_pos_x;
double bsc_pos_y;
double bsc_pos_z;
double bsc_spare_fp1;
double bsc_spare_fp2;
double bsc_spare_rp1;
double bsc_spare_rp2;
double bsc_spare_rp3;
double bsc_sts_in_mtrx[3][6];
double bsc_sts_mtrx[8][3];
double bsc_sts_ramp_sw[4];
double bsc_sts_x;
double bsc_sts_x_ry;
double bsc_sts_y;
double bsc_sts_y_rx;
double bsc_sts_z;
double bsc_wd_geo_h1;
double bsc_wd_geo_h2;
double bsc_wd_geo_h3;
double bsc_wd_geo_h4;
double bsc_wd_geo_v1;
double bsc_wd_geo_v2;
double bsc_wd_geo_v3;
double bsc_wd_geo_v4;
float bsc_wd_mFilt[20];
float bsc_wd_mRaw[20];
double bsc_wd_pos_h1;
double bsc_wd_pos_h2;
double bsc_wd_pos_h3;
double bsc_wd_pos_h4;
double bsc_wd_pos_v1;
double bsc_wd_pos_v2;
double bsc_wd_pos_v3;
double bsc_wd_pos_v4;
double bsc_wd_stsx;
double bsc_wd_stsy;
double bsc_wd_stsz;
static double delay1;
static double delay10;
static double delay11;
static double delay12;
static double delay13;
static double delay14;
static double delay15;
static double delay16;
static double delay17;
static double delay18;
static double delay19;
static double delay2;
static double delay20;
static double delay21;
static double delay22;
static double delay23;
static double delay24;
static double delay3;
static double delay4;
static double delay5;
static double delay6;
static double delay7;
static double delay8;
static double delay9;
double hmx_hp;
double hmx_loop_gain[8];
float HMX_LOOP_GAIN_CALC;
double hmx_loop_sw[16];
double hmx_mtrx[8][8];
double hmx_rx;
double hmx_ry;
double hmx_rz;
double hmx_sum;
double hmx_sum1;
double hmx_sum2;
double hmx_sum3;
double hmx_sum4;
double hmx_sum5;
double hmx_sum6;
double hmx_sum7;
double hmx_tiltcorr_gain[8];
float HMX_TILTCORR_GAIN_CALC;
double hmx_tiltcorr_h1;
double hmx_tiltcorr_h2;
double hmx_tiltcorr_h3;
double hmx_tiltcorr_h4;
double hmx_tiltcorr_mtrx[8][8];
double hmx_tiltcorr_sw[8];
double hmx_tiltcorr_v1;
double hmx_tiltcorr_v2;
double hmx_tiltcorr_v3;
double hmx_tiltcorr_v4;
double hmx_vp;
double hmx_x;
double hmx_y;
double hmx_z;
double hmx_act_gain[8];
float HMX_ACT_GAIN_CALC;
double hmx_act_h1;
double hmx_act_h2;
double hmx_act_h3;
double hmx_act_h4;
double hmx_act_sw[8];
double hmx_act_sum;
double hmx_act_sum1;
double hmx_act_sum2;
double hmx_act_sum3;
double hmx_act_sum4;
double hmx_act_sum5;
double hmx_act_sum6;
double hmx_act_sum7;
double hmx_act_v1;
double hmx_act_v2;
double hmx_act_v3;
double hmx_act_v4;
double hmx_dc_bias_mtrx[8][6];
double hmx_dc_bias_sw[8];
double hmx_fir_mtrx[2][4];
double hmx_fir_x_out;
double hmx_fir_y_out;
double hmx_geo_h1;
double hmx_geo_h2;
double hmx_geo_h3;
double hmx_geo_h4;
double hmx_geo_hp;
double hmx_geo_mtrx[8][8];
double hmx_geo_rx;
double hmx_geo_ry;
double hmx_geo_rz;
double hmx_geo_sw[8];
double hmx_geo_sum;
double hmx_geo_sum1;
double hmx_geo_sum2;
double hmx_geo_sum3;
double hmx_geo_sum4;
double hmx_geo_sum5;
double hmx_geo_sum6;
double hmx_geo_sum7;
double hmx_geo_v1;
double hmx_geo_v2;
double hmx_geo_v3;
double hmx_geo_v4;
double hmx_geo_vp;
double hmx_geo_x;
double hmx_geo_x_rms;
double hmx_geo_x_rms2;
double hmx_geo_y;
double hmx_geo_y_rms;
double hmx_geo_y_rms2;
double hmx_geo_z;
double hmx_geo_z_rms;
double hmx_geo_z_rms2;
float hmx_geo_rms1;
static float hmx_geo_rms1_avg;
float hmx_geo_rms2;
static float hmx_geo_rms2_avg;
float hmx_geo_rms3;
static float hmx_geo_rms3_avg;
float hmx_geo_rms4;
static float hmx_geo_rms4_avg;
float hmx_geo_rms5;
static float hmx_geo_rms5_avg;
float hmx_geo_rms6;
static float hmx_geo_rms6_avg;
double hmx_pos_h1;
double hmx_pos_h2;
double hmx_pos_h3;
double hmx_pos_h4;
double hmx_pos_hp;
double hmx_pos_mtrx[8][8];
double hmx_pos_norm[16];
double hmx_pos_rx;
double hmx_pos_ry;
double hmx_pos_rz;
double hmx_pos_sw[8];
double hmx_pos_sum;
double hmx_pos_sum1;
double hmx_pos_v1;
double hmx_pos_v2;
double hmx_pos_v3;
double hmx_pos_v4;
double hmx_pos_vp;
double hmx_pos_x;
double hmx_pos_y;
double hmx_pos_z;
double hmx_spare_fp1;
double hmx_spare_fp2;
double hmx_spare_rp1;
double hmx_spare_rp2;
double hmx_spare_rp3;
double hmx_sts_in_mtrx[3][6];
double hmx_sts_mtrx[8][3];
double hmx_sts_ramp_sw[4];
double hmx_sts_x;
double hmx_sts_x_ry;
double hmx_sts_y;
double hmx_sts_y_rx;
double hmx_sts_z;
double hmx_wd_geo_h1;
double hmx_wd_geo_h2;
double hmx_wd_geo_h3;
double hmx_wd_geo_h4;
double hmx_wd_geo_v1;
double hmx_wd_geo_v2;
double hmx_wd_geo_v3;
double hmx_wd_geo_v4;
float hmx_wd_mFilt[20];
float hmx_wd_mRaw[20];
double hmx_wd_pos_h1;
double hmx_wd_pos_h2;
double hmx_wd_pos_h3;
double hmx_wd_pos_h4;
double hmx_wd_pos_v1;
double hmx_wd_pos_v2;
double hmx_wd_pos_v3;
double hmx_wd_pos_v4;
double hmx_wd_stsx;
double hmx_wd_stsy;
double hmx_wd_stsz;
double hmy_hp;
double hmy_loop_gain[8];
float HMY_LOOP_GAIN_CALC;
double hmy_loop_sw[16];
double hmy_mtrx[8][8];
double hmy_rx;
double hmy_ry;
double hmy_rz;
double hmy_sum;
double hmy_sum1;
double hmy_sum2;
double hmy_sum3;
double hmy_sum4;
double hmy_sum5;
double hmy_sum6;
double hmy_sum7;
double hmy_tiltcorr_gain[8];
float HMY_TILTCORR_GAIN_CALC;
double hmy_tiltcorr_h1;
double hmy_tiltcorr_h2;
double hmy_tiltcorr_h3;
double hmy_tiltcorr_h4;
double hmy_tiltcorr_mtrx[8][8];
double hmy_tiltcorr_sw[8];
double hmy_tiltcorr_v1;
double hmy_tiltcorr_v2;
double hmy_tiltcorr_v3;
double hmy_tiltcorr_v4;
double hmy_vp;
double hmy_x;
double hmy_y;
double hmy_z;
double hmy_act_gain[8];
float HMY_ACT_GAIN_CALC;
double hmy_act_h1;
double hmy_act_h2;
double hmy_act_h3;
double hmy_act_h4;
double hmy_act_sw[8];
double hmy_act_sum;
double hmy_act_sum1;
double hmy_act_sum2;
double hmy_act_sum3;
double hmy_act_sum4;
double hmy_act_sum5;
double hmy_act_sum6;
double hmy_act_sum7;
double hmy_act_v1;
double hmy_act_v2;
double hmy_act_v3;
double hmy_act_v4;
double hmy_dc_bias_mtrx[8][6];
double hmy_dc_bias_sw[8];
double hmy_fir_mtrx[2][4];
double hmy_fir_x_out;
double hmy_fir_y_out;
double hmy_geo_h1;
double hmy_geo_h2;
double hmy_geo_h3;
double hmy_geo_h4;
double hmy_geo_hp;
double hmy_geo_mtrx[8][8];
double hmy_geo_rx;
double hmy_geo_ry;
double hmy_geo_rz;
double hmy_geo_sw[8];
double hmy_geo_sum;
double hmy_geo_sum1;
double hmy_geo_sum2;
double hmy_geo_sum3;
double hmy_geo_sum4;
double hmy_geo_sum5;
double hmy_geo_sum6;
double hmy_geo_sum7;
double hmy_geo_v1;
double hmy_geo_v2;
double hmy_geo_v3;
double hmy_geo_v4;
double hmy_geo_vp;
double hmy_geo_x;
double hmy_geo_x_rms;
double hmy_geo_x_rms2;
double hmy_geo_y;
double hmy_geo_y_rms;
double hmy_geo_y_rms2;
double hmy_geo_z;
double hmy_geo_z_rms;
double hmy_geo_z_rms2;
float hmy_geo_rms1;
static float hmy_geo_rms1_avg;
float hmy_geo_rms2;
static float hmy_geo_rms2_avg;
float hmy_geo_rms3;
static float hmy_geo_rms3_avg;
float hmy_geo_rms4;
static float hmy_geo_rms4_avg;
float hmy_geo_rms5;
static float hmy_geo_rms5_avg;
float hmy_geo_rms6;
static float hmy_geo_rms6_avg;
double hmy_pos_h1;
double hmy_pos_h2;
double hmy_pos_h3;
double hmy_pos_h4;
double hmy_pos_hp;
double hmy_pos_mtrx[8][8];
double hmy_pos_norm[16];
double hmy_pos_rx;
double hmy_pos_ry;
double hmy_pos_rz;
double hmy_pos_sw[8];
double hmy_pos_sum;
double hmy_pos_sum1;
double hmy_pos_v1;
double hmy_pos_v2;
double hmy_pos_v3;
double hmy_pos_v4;
double hmy_pos_vp;
double hmy_pos_x;
double hmy_pos_y;
double hmy_pos_z;
double hmy_spare_fp1;
double hmy_spare_fp2;
double hmy_spare_rp1;
double hmy_spare_rp2;
double hmy_spare_rp3;
double hmy_sts_in_mtrx[3][6];
double hmy_sts_mtrx[8][3];
double hmy_sts_ramp_sw[4];
double hmy_sts_x;
double hmy_sts_x_ry;
double hmy_sts_y;
double hmy_sts_y_rx;
double hmy_sts_z;
double hmy_wd_geo_h1;
double hmy_wd_geo_h2;
double hmy_wd_geo_h3;
double hmy_wd_geo_h4;
double hmy_wd_geo_v1;
double hmy_wd_geo_v2;
double hmy_wd_geo_v3;
double hmy_wd_geo_v4;
float hmy_wd_mFilt[20];
float hmy_wd_mRaw[20];
double hmy_wd_pos_h1;
double hmy_wd_pos_h2;
double hmy_wd_pos_h3;
double hmy_wd_pos_h4;
double hmy_wd_pos_v1;
double hmy_wd_pos_v2;
double hmy_wd_pos_v3;
double hmy_wd_pos_v4;
double hmy_wd_stsx;
double hmy_wd_stsy;
double hmy_wd_stsz;
double sts1_fir_x_cf;
double sts1_fir_x_pp;
double sts1_fir_y_cf;
double sts1_fir_y_pp;
double sts2_fir_x_cf;
double sts2_fir_x_pp;
double sts2_fir_y_cf;
double sts2_fir_y_pp;
double sum1;
double sum2;
double sum3;
double sum4;


if(feInit)
{
bsc_geo_rms1_avg = 0.0;
bsc_geo_rms2_avg = 0.0;
bsc_geo_rms3_avg = 0.0;
bsc_geo_rms4_avg = 0.0;
bsc_geo_rms5_avg = 0.0;
bsc_geo_rms6_avg = 0.0;
hmx_geo_rms1_avg = 0.0;
hmx_geo_rms2_avg = 0.0;
hmx_geo_rms3_avg = 0.0;
hmx_geo_rms4_avg = 0.0;
hmx_geo_rms5_avg = 0.0;
hmx_geo_rms6_avg = 0.0;
hmy_geo_rms1_avg = 0.0;
hmy_geo_rms2_avg = 0.0;
hmy_geo_rms3_avg = 0.0;
hmy_geo_rms4_avg = 0.0;
hmy_geo_rms5_avg = 0.0;
hmy_geo_rms6_avg = 0.0;
} else {

//Start of subsystem BSC_DC_BIAS **************************************************

// Matrix
for(ii=0;ii<8;ii++)
{
bsc_dc_bias_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][0] * pLocalEpics->sei.BSC_DC_BIAS_1 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][1] * pLocalEpics->sei.BSC_DC_BIAS_2 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][2] * pLocalEpics->sei.BSC_DC_BIAS_3 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][3] * pLocalEpics->sei.BSC_DC_BIAS_4 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][4] * pLocalEpics->sei.BSC_DC_BIAS_5 +
	pLocalEpics->sei.BSC_DC_BIAS_MTRX[ii][5] * pLocalEpics->sei.BSC_DC_BIAS_6;
}

// MultiSwitch
bsc_dc_bias_sw[0] = bsc_dc_bias_mtrx[1][0];
bsc_dc_bias_sw[1] = bsc_dc_bias_mtrx[1][1];
bsc_dc_bias_sw[2] = bsc_dc_bias_mtrx[1][2];
bsc_dc_bias_sw[3] = bsc_dc_bias_mtrx[1][3];
bsc_dc_bias_sw[4] = bsc_dc_bias_mtrx[1][4];
bsc_dc_bias_sw[5] = bsc_dc_bias_mtrx[1][5];
bsc_dc_bias_sw[6] = bsc_dc_bias_mtrx[1][6];
bsc_dc_bias_sw[7] = bsc_dc_bias_mtrx[1][7];
if (pLocalEpics->sei.BSC_DC_BIAS_SW == 0) {
	for (ii=0; ii<8; ii++) bsc_dc_bias_sw[ii] = 0.0;
}



//End of subsystem   BSC_DC_BIAS **************************************************



//Start of subsystem BSC_SPARE **************************************************

// FILTER MODULE
bsc_spare_fp1 = filterModuleD(dsp_ptr,dspCoeff,BSC_SPARE_FP1,dWord[0][26],0);

// FILTER MODULE
bsc_spare_fp2 = filterModuleD(dsp_ptr,dspCoeff,BSC_SPARE_FP2,dWord[0][27],0);

// FILTER MODULE
bsc_spare_rp1 = filterModuleD(dsp_ptr,dspCoeff,BSC_SPARE_RP1,dWord[0][28],0);

// FILTER MODULE
bsc_spare_rp2 = filterModuleD(dsp_ptr,dspCoeff,BSC_SPARE_RP2,dWord[0][29],0);

// FILTER MODULE
bsc_spare_rp3 = filterModuleD(dsp_ptr,dspCoeff,BSC_SPARE_RP3,dWord[0][30],0);


//End of subsystem   BSC_SPARE **************************************************



//Start of subsystem HMX_DC_BIAS **************************************************

// Matrix
for(ii=0;ii<8;ii++)
{
hmx_dc_bias_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_DC_BIAS_MTRX[ii][0] * pLocalEpics->sei.HMX_DC_BIAS_1 +
	pLocalEpics->sei.HMX_DC_BIAS_MTRX[ii][1] * pLocalEpics->sei.HMX_DC_BIAS_2 +
	pLocalEpics->sei.HMX_DC_BIAS_MTRX[ii][2] * pLocalEpics->sei.HMX_DC_BIAS_3 +
	pLocalEpics->sei.HMX_DC_BIAS_MTRX[ii][3] * pLocalEpics->sei.HMX_DC_BIAS_4 +
	pLocalEpics->sei.HMX_DC_BIAS_MTRX[ii][4] * pLocalEpics->sei.HMX_DC_BIAS_5 +
	pLocalEpics->sei.HMX_DC_BIAS_MTRX[ii][5] * pLocalEpics->sei.HMX_DC_BIAS_6;
}

// MultiSwitch
hmx_dc_bias_sw[0] = hmx_dc_bias_mtrx[1][0];
hmx_dc_bias_sw[1] = hmx_dc_bias_mtrx[1][1];
hmx_dc_bias_sw[2] = hmx_dc_bias_mtrx[1][2];
hmx_dc_bias_sw[3] = hmx_dc_bias_mtrx[1][3];
hmx_dc_bias_sw[4] = hmx_dc_bias_mtrx[1][4];
hmx_dc_bias_sw[5] = hmx_dc_bias_mtrx[1][5];
hmx_dc_bias_sw[6] = hmx_dc_bias_mtrx[1][6];
hmx_dc_bias_sw[7] = hmx_dc_bias_mtrx[1][7];
if (pLocalEpics->sei.HMX_DC_BIAS_SW == 0) {
	for (ii=0; ii<8; ii++) hmx_dc_bias_sw[ii] = 0.0;
}



//End of subsystem   HMX_DC_BIAS **************************************************



//Start of subsystem HMX_SPARE **************************************************

// FILTER MODULE
hmx_spare_fp1 = filterModuleD(dsp_ptr,dspCoeff,HMX_SPARE_FP1,dWord[1][26],0);

// FILTER MODULE
hmx_spare_fp2 = filterModuleD(dsp_ptr,dspCoeff,HMX_SPARE_FP2,dWord[1][27],0);

// FILTER MODULE
hmx_spare_rp1 = filterModuleD(dsp_ptr,dspCoeff,HMX_SPARE_RP1,dWord[1][28],0);

// FILTER MODULE
hmx_spare_rp2 = filterModuleD(dsp_ptr,dspCoeff,HMX_SPARE_RP2,dWord[1][29],0);

// FILTER MODULE
hmx_spare_rp3 = filterModuleD(dsp_ptr,dspCoeff,HMX_SPARE_RP3,dWord[1][30],0);


//End of subsystem   HMX_SPARE **************************************************



//Start of subsystem HMY_DC_BIAS **************************************************

// Matrix
for(ii=0;ii<8;ii++)
{
hmy_dc_bias_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_DC_BIAS_MTRX[ii][0] * pLocalEpics->sei.HMY_DC_BIAS_1 +
	pLocalEpics->sei.HMY_DC_BIAS_MTRX[ii][1] * pLocalEpics->sei.HMY_DC_BIAS_2 +
	pLocalEpics->sei.HMY_DC_BIAS_MTRX[ii][2] * pLocalEpics->sei.HMY_DC_BIAS_3 +
	pLocalEpics->sei.HMY_DC_BIAS_MTRX[ii][3] * pLocalEpics->sei.HMY_DC_BIAS_4 +
	pLocalEpics->sei.HMY_DC_BIAS_MTRX[ii][4] * pLocalEpics->sei.HMY_DC_BIAS_5 +
	pLocalEpics->sei.HMY_DC_BIAS_MTRX[ii][5] * pLocalEpics->sei.HMY_DC_BIAS_6;
}

// MultiSwitch
hmy_dc_bias_sw[0] = hmy_dc_bias_mtrx[1][0];
hmy_dc_bias_sw[1] = hmy_dc_bias_mtrx[1][1];
hmy_dc_bias_sw[2] = hmy_dc_bias_mtrx[1][2];
hmy_dc_bias_sw[3] = hmy_dc_bias_mtrx[1][3];
hmy_dc_bias_sw[4] = hmy_dc_bias_mtrx[1][4];
hmy_dc_bias_sw[5] = hmy_dc_bias_mtrx[1][5];
hmy_dc_bias_sw[6] = hmy_dc_bias_mtrx[1][6];
hmy_dc_bias_sw[7] = hmy_dc_bias_mtrx[1][7];
if (pLocalEpics->sei.HMY_DC_BIAS_SW == 0) {
	for (ii=0; ii<8; ii++) hmy_dc_bias_sw[ii] = 0.0;
}



//End of subsystem   HMY_DC_BIAS **************************************************



//Start of subsystem HMY_SPARE **************************************************

// FILTER MODULE
hmy_spare_fp1 = filterModuleD(dsp_ptr,dspCoeff,HMY_SPARE_FP1,dWord[2][26],0);

// FILTER MODULE
hmy_spare_fp2 = filterModuleD(dsp_ptr,dspCoeff,HMY_SPARE_FP2,dWord[2][27],0);

// FILTER MODULE
hmy_spare_rp1 = filterModuleD(dsp_ptr,dspCoeff,HMY_SPARE_RP1,dWord[2][28],0);

// FILTER MODULE
hmy_spare_rp2 = filterModuleD(dsp_ptr,dspCoeff,HMY_SPARE_RP2,dWord[2][29],0);

// FILTER MODULE
hmy_spare_rp3 = filterModuleD(dsp_ptr,dspCoeff,HMY_SPARE_RP3,dWord[2][30],0);


//End of subsystem   HMY_SPARE **************************************************



//Start of subsystem STS1_FIR_X **************************************************

// FILTER MODULE
sts1_fir_x_pp = filterModuleD(dsp_ptr,dspCoeff,STS1_FIR_X_PP,dWord[0][23],0);

// FILTER MODULE
sts1_fir_x_cf = filterModuleD(dsp_ptr,dspCoeff,STS1_FIR_X_CF,dWord[0][23],0);


//End of subsystem   STS1_FIR_X **************************************************



//Start of subsystem STS1_FIR_Y **************************************************

// FILTER MODULE
sts1_fir_y_pp = filterModuleD(dsp_ptr,dspCoeff,STS1_FIR_Y_PP,dWord[0][24],0);

// FILTER MODULE
sts1_fir_y_cf = filterModuleD(dsp_ptr,dspCoeff,STS1_FIR_Y_CF,dWord[0][24],0);


//End of subsystem   STS1_FIR_Y **************************************************



//Start of subsystem STS2_FIR_X **************************************************

// FILTER MODULE
sts2_fir_x_pp = filterModuleD(dsp_ptr,dspCoeff,STS2_FIR_X_PP,dWord[1][23],0);

// FILTER MODULE
sts2_fir_x_cf = filterModuleD(dsp_ptr,dspCoeff,STS2_FIR_X_CF,dWord[1][23],0);


//End of subsystem   STS2_FIR_X **************************************************



//Start of subsystem STS2_FIR_Y **************************************************

// FILTER MODULE
sts2_fir_y_pp = filterModuleD(dsp_ptr,dspCoeff,STS2_FIR_Y_PP,dWord[1][24],0);

// FILTER MODULE
sts2_fir_y_cf = filterModuleD(dsp_ptr,dspCoeff,STS2_FIR_Y_CF,dWord[1][24],0);


//End of subsystem   STS2_FIR_Y **************************************************


// EpicsOut
pLocalEpics->sei.BSC_WIT_1_SENSOR = dWord[0][16];

// EpicsOut
pLocalEpics->sei.BSC_WIT_2_SENSOR = dWord[0][17];

// EpicsOut
pLocalEpics->sei.BSC_WIT_3_SENSOR = dWord[0][18];

// EpicsOut
pLocalEpics->sei.BSC_WIT_4_SENSOR = dWord[0][19];

// EpicsOut
pLocalEpics->sei.HMX_WIT_1_SENSOR = dWord[1][16];

// EpicsOut
pLocalEpics->sei.HMX_WIT_2_SENSOR = dWord[1][17];

// EpicsOut
pLocalEpics->sei.HMX_WIT_3_SENSOR = dWord[1][18];

// EpicsOut
pLocalEpics->sei.HMX_WIT_4_SENSOR = dWord[1][19];

// EpicsOut
pLocalEpics->sei.HMY_WIT_1_SENSOR = dWord[2][16];

// EpicsOut
pLocalEpics->sei.HMY_WIT_2_SENSOR = dWord[2][17];

// EpicsOut
pLocalEpics->sei.HMY_WIT_3_SENSOR = dWord[2][18];

// EpicsOut
pLocalEpics->sei.HMY_WIT_4_SENSOR = dWord[2][19];

// EpicsOut
pLocalEpics->sei.STS1_X = dWord[0][23];

// EpicsOut
pLocalEpics->sei.STS1_Y = dWord[0][24];

// EpicsOut
pLocalEpics->sei.STS1_Z = dWord[0][25];

// EpicsOut
pLocalEpics->sei.STS2_X = dWord[1][23];

// EpicsOut
pLocalEpics->sei.STS2_Y = dWord[1][24];

// EpicsOut
pLocalEpics->sei.STS2_Z = dWord[1][25];



//Start of subsystem BSC_GEO **************************************************

// EpicsOut
pLocalEpics->sei.BSC_GEO_SV1 = dWord[0][4];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SV2 = dWord[0][5];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SV3 = dWord[0][6];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SV4 = dWord[0][7];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SH1 = dWord[0][0];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SH2 = dWord[0][1];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SH3 = dWord[0][2];

// EpicsOut
pLocalEpics->sei.BSC_GEO_SH4 = dWord[0][3];

// SUM
bsc_geo_sum = pLocalEpics->sei.BSC_GEO_SV1 + delay1;

// SUM
bsc_geo_sum1 = pLocalEpics->sei.BSC_GEO_SV2 + delay2;

// SUM
bsc_geo_sum2 = pLocalEpics->sei.BSC_GEO_SV3 + delay3;

// SUM
bsc_geo_sum3 = pLocalEpics->sei.BSC_GEO_SV4 + delay4;

// SUM
bsc_geo_sum4 = pLocalEpics->sei.BSC_GEO_SH1 + delay5;

// SUM
bsc_geo_sum5 = pLocalEpics->sei.BSC_GEO_SH2 + delay6;

// SUM
bsc_geo_sum6 = pLocalEpics->sei.BSC_GEO_SH3 + delay7;

// SUM
bsc_geo_sum7 = pLocalEpics->sei.BSC_GEO_SH4 + delay8;

// FILTER MODULE
bsc_geo_v1 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_V1,bsc_geo_sum,0);

// FILTER MODULE
bsc_geo_v2 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_V2,bsc_geo_sum1,0);

// FILTER MODULE
bsc_geo_v3 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_V3,bsc_geo_sum2,0);

// FILTER MODULE
bsc_geo_v4 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_V4,bsc_geo_sum3,0);

// FILTER MODULE
bsc_geo_h1 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_H1,bsc_geo_sum4,0);

// FILTER MODULE
bsc_geo_h2 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_H2,bsc_geo_sum5,0);

// FILTER MODULE
bsc_geo_h3 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_H3,bsc_geo_sum6,0);

// FILTER MODULE
bsc_geo_h4 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_H4,bsc_geo_sum7,0);

// Matrix
for(ii=0;ii<8;ii++)
{
bsc_geo_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_GEO_MTRX[ii][0] * bsc_geo_v1 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][1] * bsc_geo_v2 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][2] * bsc_geo_v3 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][3] * bsc_geo_v4 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][4] * bsc_geo_h1 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][5] * bsc_geo_h2 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][6] * bsc_geo_h3 +
	pLocalEpics->sei.BSC_GEO_MTRX[ii][7] * bsc_geo_h4;
}

// FILTER MODULE
bsc_geo_x = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_X,bsc_geo_mtrx[1][0],0);

// FILTER MODULE
bsc_geo_x_rms2 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_X_RMS2,bsc_geo_mtrx[1][0],0);

// FILTER MODULE
bsc_geo_x_rms = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_X_RMS,bsc_geo_mtrx[1][0],0);

// FILTER MODULE
bsc_geo_y = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_Y,bsc_geo_mtrx[1][1],0);

// FILTER MODULE
bsc_geo_y_rms2 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_Y_RMS2,bsc_geo_mtrx[1][1],0);

// FILTER MODULE
bsc_geo_y_rms = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_Y_RMS,bsc_geo_mtrx[1][1],0);

// FILTER MODULE
bsc_geo_z = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_Z,bsc_geo_mtrx[1][2],0);

// FILTER MODULE
bsc_geo_z_rms = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_Z_RMS,bsc_geo_mtrx[1][2],0);

// FILTER MODULE
bsc_geo_z_rms2 = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_Z_RMS2,bsc_geo_mtrx[1][2],0);

// FILTER MODULE
bsc_geo_rz = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_RZ,bsc_geo_mtrx[1][5],0);

// FILTER MODULE
bsc_geo_vp = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_VP,bsc_geo_mtrx[1][6],0);

// FILTER MODULE
bsc_geo_hp = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_HP,bsc_geo_mtrx[1][7],0);

// FILTER MODULE
bsc_geo_rx = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_RX,bsc_geo_mtrx[1][3],0);

// FILTER MODULE
bsc_geo_ry = filterModuleD(dsp_ptr,dspCoeff,BSC_GEO_RY,bsc_geo_mtrx[1][4],0);

// RMS
bsc_geo_rms2 = bsc_geo_x_rms2;
if(bsc_geo_rms2 > 2000) bsc_geo_rms2 = 2000;
if(bsc_geo_rms2 < -2000) bsc_geo_rms2 = 2000;
bsc_geo_rms2 = bsc_geo_rms2 * bsc_geo_rms2;
bsc_geo_rms2_avg = bsc_geo_rms2 * .00005 + bsc_geo_rms2_avg * 0.99995;
bsc_geo_rms2 = lsqrt(bsc_geo_rms2_avg);

// RMS
bsc_geo_rms1 = bsc_geo_x_rms;
if(bsc_geo_rms1 > 2000) bsc_geo_rms1 = 2000;
if(bsc_geo_rms1 < -2000) bsc_geo_rms1 = 2000;
bsc_geo_rms1 = bsc_geo_rms1 * bsc_geo_rms1;
bsc_geo_rms1_avg = bsc_geo_rms1 * .00005 + bsc_geo_rms1_avg * 0.99995;
bsc_geo_rms1 = lsqrt(bsc_geo_rms1_avg);

// RMS
bsc_geo_rms4 = bsc_geo_y_rms2;
if(bsc_geo_rms4 > 2000) bsc_geo_rms4 = 2000;
if(bsc_geo_rms4 < -2000) bsc_geo_rms4 = 2000;
bsc_geo_rms4 = bsc_geo_rms4 * bsc_geo_rms4;
bsc_geo_rms4_avg = bsc_geo_rms4 * .00005 + bsc_geo_rms4_avg * 0.99995;
bsc_geo_rms4 = lsqrt(bsc_geo_rms4_avg);

// RMS
bsc_geo_rms3 = bsc_geo_y_rms;
if(bsc_geo_rms3 > 2000) bsc_geo_rms3 = 2000;
if(bsc_geo_rms3 < -2000) bsc_geo_rms3 = 2000;
bsc_geo_rms3 = bsc_geo_rms3 * bsc_geo_rms3;
bsc_geo_rms3_avg = bsc_geo_rms3 * .00005 + bsc_geo_rms3_avg * 0.99995;
bsc_geo_rms3 = lsqrt(bsc_geo_rms3_avg);

// RMS
bsc_geo_rms5 = bsc_geo_z_rms;
if(bsc_geo_rms5 > 2000) bsc_geo_rms5 = 2000;
if(bsc_geo_rms5 < -2000) bsc_geo_rms5 = 2000;
bsc_geo_rms5 = bsc_geo_rms5 * bsc_geo_rms5;
bsc_geo_rms5_avg = bsc_geo_rms5 * .00005 + bsc_geo_rms5_avg * 0.99995;
bsc_geo_rms5 = lsqrt(bsc_geo_rms5_avg);

// RMS
bsc_geo_rms6 = bsc_geo_z_rms2;
if(bsc_geo_rms6 > 2000) bsc_geo_rms6 = 2000;
if(bsc_geo_rms6 < -2000) bsc_geo_rms6 = 2000;
bsc_geo_rms6 = bsc_geo_rms6 * bsc_geo_rms6;
bsc_geo_rms6_avg = bsc_geo_rms6 * .00005 + bsc_geo_rms6_avg * 0.99995;
bsc_geo_rms6 = lsqrt(bsc_geo_rms6_avg);

// MultiSwitch
bsc_geo_sw[0] = bsc_geo_x;
bsc_geo_sw[1] = bsc_geo_y;
bsc_geo_sw[2] = bsc_geo_z;
bsc_geo_sw[3] = bsc_geo_rx;
bsc_geo_sw[4] = bsc_geo_ry;
bsc_geo_sw[5] = bsc_geo_rz;
bsc_geo_sw[6] = bsc_geo_vp;
bsc_geo_sw[7] = bsc_geo_hp;
if (pLocalEpics->sei.BSC_GEO_SW == 0) {
	for (ii=0; ii<8; ii++) bsc_geo_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->sei.BSC_GEO_X_RMS_2 = bsc_geo_rms2;

// EpicsOut
pLocalEpics->sei.BSC_GEO_X_RMS_1 = bsc_geo_rms1;

// EpicsOut
pLocalEpics->sei.BSC_GEO_Y_RMS_2 = bsc_geo_rms4;

// EpicsOut
pLocalEpics->sei.BSC_GEO_Y_RMS_1 = bsc_geo_rms3;

// EpicsOut
pLocalEpics->sei.BSC_GEO_Z_RMS_1 = bsc_geo_rms5;

// EpicsOut
pLocalEpics->sei.BSC_GEO_Z_RMS_2 = bsc_geo_rms6;


//End of subsystem   BSC_GEO **************************************************



//Start of subsystem HMX_GEO **************************************************

// EpicsOut
pLocalEpics->sei.HMX_GEO_SV1 = dWord[1][4];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SV2 = dWord[1][5];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SV3 = dWord[1][6];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SV4 = dWord[1][7];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SH1 = dWord[1][0];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SH2 = dWord[1][1];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SH3 = dWord[1][2];

// EpicsOut
pLocalEpics->sei.HMX_GEO_SH4 = dWord[1][3];

// SUM
hmx_geo_sum = pLocalEpics->sei.HMX_GEO_SV1 + delay9;

// SUM
hmx_geo_sum1 = pLocalEpics->sei.HMX_GEO_SV2 + delay10;

// SUM
hmx_geo_sum2 = pLocalEpics->sei.HMX_GEO_SV3 + delay11;

// SUM
hmx_geo_sum3 = pLocalEpics->sei.HMX_GEO_SV4 + delay12;

// SUM
hmx_geo_sum4 = pLocalEpics->sei.HMX_GEO_SH1 + delay13;

// SUM
hmx_geo_sum5 = pLocalEpics->sei.HMX_GEO_SH2 + delay14;

// SUM
hmx_geo_sum6 = pLocalEpics->sei.HMX_GEO_SH3 + delay15;

// SUM
hmx_geo_sum7 = pLocalEpics->sei.HMX_GEO_SH4 + delay16;

// FILTER MODULE
hmx_geo_v1 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_V1,hmx_geo_sum,0);

// FILTER MODULE
hmx_geo_v2 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_V2,hmx_geo_sum1,0);

// FILTER MODULE
hmx_geo_v3 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_V3,hmx_geo_sum2,0);

// FILTER MODULE
hmx_geo_v4 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_V4,hmx_geo_sum3,0);

// FILTER MODULE
hmx_geo_h1 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_H1,hmx_geo_sum4,0);

// FILTER MODULE
hmx_geo_h2 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_H2,hmx_geo_sum5,0);

// FILTER MODULE
hmx_geo_h3 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_H3,hmx_geo_sum6,0);

// FILTER MODULE
hmx_geo_h4 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_H4,hmx_geo_sum7,0);

// Matrix
for(ii=0;ii<8;ii++)
{
hmx_geo_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_GEO_MTRX[ii][0] * hmx_geo_v1 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][1] * hmx_geo_v2 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][2] * hmx_geo_v3 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][3] * hmx_geo_v4 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][4] * hmx_geo_h1 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][5] * hmx_geo_h2 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][6] * hmx_geo_h3 +
	pLocalEpics->sei.HMX_GEO_MTRX[ii][7] * hmx_geo_h4;
}

// FILTER MODULE
hmx_geo_x = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_X,hmx_geo_mtrx[1][0],0);

// FILTER MODULE
hmx_geo_x_rms2 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_X_RMS2,hmx_geo_mtrx[1][0],0);

// FILTER MODULE
hmx_geo_x_rms = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_X_RMS,hmx_geo_mtrx[1][0],0);

// FILTER MODULE
hmx_geo_y = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_Y,hmx_geo_mtrx[1][1],0);

// FILTER MODULE
hmx_geo_y_rms2 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_Y_RMS2,hmx_geo_mtrx[1][1],0);

// FILTER MODULE
hmx_geo_y_rms = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_Y_RMS,hmx_geo_mtrx[1][1],0);

// FILTER MODULE
hmx_geo_z = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_Z,hmx_geo_mtrx[1][2],0);

// FILTER MODULE
hmx_geo_z_rms = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_Z_RMS,hmx_geo_mtrx[1][2],0);

// FILTER MODULE
hmx_geo_z_rms2 = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_Z_RMS2,hmx_geo_mtrx[1][2],0);

// FILTER MODULE
hmx_geo_rz = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_RZ,hmx_geo_mtrx[1][5],0);

// FILTER MODULE
hmx_geo_vp = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_VP,hmx_geo_mtrx[1][6],0);

// FILTER MODULE
hmx_geo_hp = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_HP,hmx_geo_mtrx[1][7],0);

// FILTER MODULE
hmx_geo_rx = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_RX,hmx_geo_mtrx[1][3],0);

// FILTER MODULE
hmx_geo_ry = filterModuleD(dsp_ptr,dspCoeff,HMX_GEO_RY,hmx_geo_mtrx[1][4],0);

// RMS
hmx_geo_rms2 = hmx_geo_x_rms2;
if(hmx_geo_rms2 > 2000) hmx_geo_rms2 = 2000;
if(hmx_geo_rms2 < -2000) hmx_geo_rms2 = 2000;
hmx_geo_rms2 = hmx_geo_rms2 * hmx_geo_rms2;
hmx_geo_rms2_avg = hmx_geo_rms2 * .00005 + hmx_geo_rms2_avg * 0.99995;
hmx_geo_rms2 = lsqrt(hmx_geo_rms2_avg);

// RMS
hmx_geo_rms1 = hmx_geo_x_rms;
if(hmx_geo_rms1 > 2000) hmx_geo_rms1 = 2000;
if(hmx_geo_rms1 < -2000) hmx_geo_rms1 = 2000;
hmx_geo_rms1 = hmx_geo_rms1 * hmx_geo_rms1;
hmx_geo_rms1_avg = hmx_geo_rms1 * .00005 + hmx_geo_rms1_avg * 0.99995;
hmx_geo_rms1 = lsqrt(hmx_geo_rms1_avg);

// RMS
hmx_geo_rms4 = hmx_geo_y_rms2;
if(hmx_geo_rms4 > 2000) hmx_geo_rms4 = 2000;
if(hmx_geo_rms4 < -2000) hmx_geo_rms4 = 2000;
hmx_geo_rms4 = hmx_geo_rms4 * hmx_geo_rms4;
hmx_geo_rms4_avg = hmx_geo_rms4 * .00005 + hmx_geo_rms4_avg * 0.99995;
hmx_geo_rms4 = lsqrt(hmx_geo_rms4_avg);

// RMS
hmx_geo_rms3 = hmx_geo_y_rms;
if(hmx_geo_rms3 > 2000) hmx_geo_rms3 = 2000;
if(hmx_geo_rms3 < -2000) hmx_geo_rms3 = 2000;
hmx_geo_rms3 = hmx_geo_rms3 * hmx_geo_rms3;
hmx_geo_rms3_avg = hmx_geo_rms3 * .00005 + hmx_geo_rms3_avg * 0.99995;
hmx_geo_rms3 = lsqrt(hmx_geo_rms3_avg);

// RMS
hmx_geo_rms5 = hmx_geo_z_rms;
if(hmx_geo_rms5 > 2000) hmx_geo_rms5 = 2000;
if(hmx_geo_rms5 < -2000) hmx_geo_rms5 = 2000;
hmx_geo_rms5 = hmx_geo_rms5 * hmx_geo_rms5;
hmx_geo_rms5_avg = hmx_geo_rms5 * .00005 + hmx_geo_rms5_avg * 0.99995;
hmx_geo_rms5 = lsqrt(hmx_geo_rms5_avg);

// RMS
hmx_geo_rms6 = hmx_geo_z_rms2;
if(hmx_geo_rms6 > 2000) hmx_geo_rms6 = 2000;
if(hmx_geo_rms6 < -2000) hmx_geo_rms6 = 2000;
hmx_geo_rms6 = hmx_geo_rms6 * hmx_geo_rms6;
hmx_geo_rms6_avg = hmx_geo_rms6 * .00005 + hmx_geo_rms6_avg * 0.99995;
hmx_geo_rms6 = lsqrt(hmx_geo_rms6_avg);

// MultiSwitch
hmx_geo_sw[0] = hmx_geo_x;
hmx_geo_sw[1] = hmx_geo_y;
hmx_geo_sw[2] = hmx_geo_z;
hmx_geo_sw[3] = hmx_geo_rx;
hmx_geo_sw[4] = hmx_geo_ry;
hmx_geo_sw[5] = hmx_geo_rz;
hmx_geo_sw[6] = hmx_geo_vp;
hmx_geo_sw[7] = hmx_geo_hp;
if (pLocalEpics->sei.HMX_GEO_SW == 0) {
	for (ii=0; ii<8; ii++) hmx_geo_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->sei.HMX_GEO_X_RMS_2 = hmx_geo_rms2;

// EpicsOut
pLocalEpics->sei.HMX_GEO_X_RMS_1 = hmx_geo_rms1;

// EpicsOut
pLocalEpics->sei.HMX_GEO_Y_RMS_2 = hmx_geo_rms4;

// EpicsOut
pLocalEpics->sei.HMX_GEO_Y_RMS_1 = hmx_geo_rms3;

// EpicsOut
pLocalEpics->sei.HMX_GEO_Z_RMS_1 = hmx_geo_rms5;

// EpicsOut
pLocalEpics->sei.HMX_GEO_Z_RMS_2 = hmx_geo_rms6;


//End of subsystem   HMX_GEO **************************************************



//Start of subsystem HMY_GEO **************************************************

// EpicsOut
pLocalEpics->sei.HMY_GEO_SV1 = dWord[2][4];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SV2 = dWord[2][5];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SV3 = dWord[2][6];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SV4 = dWord[2][7];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SH1 = dWord[2][0];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SH2 = dWord[2][1];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SH3 = dWord[2][2];

// EpicsOut
pLocalEpics->sei.HMY_GEO_SH4 = dWord[2][3];

// SUM
hmy_geo_sum = pLocalEpics->sei.HMY_GEO_SV1 + delay24;

// SUM
hmy_geo_sum1 = pLocalEpics->sei.HMY_GEO_SV2 + delay17;

// SUM
hmy_geo_sum2 = pLocalEpics->sei.HMY_GEO_SV3 + delay18;

// SUM
hmy_geo_sum3 = pLocalEpics->sei.HMY_GEO_SV4 + delay19;

// SUM
hmy_geo_sum4 = pLocalEpics->sei.HMY_GEO_SH1 + delay20;

// SUM
hmy_geo_sum5 = pLocalEpics->sei.HMY_GEO_SH2 + delay21;

// SUM
hmy_geo_sum6 = pLocalEpics->sei.HMY_GEO_SH3 + delay22;

// SUM
hmy_geo_sum7 = pLocalEpics->sei.HMY_GEO_SH4 + delay23;

// FILTER MODULE
hmy_geo_v1 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_V1,hmy_geo_sum,0);

// FILTER MODULE
hmy_geo_v2 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_V2,hmy_geo_sum1,0);

// FILTER MODULE
hmy_geo_v3 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_V3,hmy_geo_sum2,0);

// FILTER MODULE
hmy_geo_v4 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_V4,hmy_geo_sum3,0);

// FILTER MODULE
hmy_geo_h1 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_H1,hmy_geo_sum4,0);

// FILTER MODULE
hmy_geo_h2 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_H2,hmy_geo_sum5,0);

// FILTER MODULE
hmy_geo_h3 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_H3,hmy_geo_sum6,0);

// FILTER MODULE
hmy_geo_h4 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_H4,hmy_geo_sum7,0);

// Matrix
for(ii=0;ii<8;ii++)
{
hmy_geo_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_GEO_MTRX[ii][0] * hmy_geo_v1 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][1] * hmy_geo_v2 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][2] * hmy_geo_v3 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][3] * hmy_geo_v4 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][4] * hmy_geo_h1 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][5] * hmy_geo_h2 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][6] * hmy_geo_h3 +
	pLocalEpics->sei.HMY_GEO_MTRX[ii][7] * hmy_geo_h4;
}

// FILTER MODULE
hmy_geo_x = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_X,hmy_geo_mtrx[1][0],0);

// FILTER MODULE
hmy_geo_x_rms2 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_X_RMS2,hmy_geo_mtrx[1][0],0);

// FILTER MODULE
hmy_geo_x_rms = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_X_RMS,hmy_geo_mtrx[1][0],0);

// FILTER MODULE
hmy_geo_y = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_Y,hmy_geo_mtrx[1][1],0);

// FILTER MODULE
hmy_geo_y_rms2 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_Y_RMS2,hmy_geo_mtrx[1][1],0);

// FILTER MODULE
hmy_geo_y_rms = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_Y_RMS,hmy_geo_mtrx[1][1],0);

// FILTER MODULE
hmy_geo_z = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_Z,hmy_geo_mtrx[1][2],0);

// FILTER MODULE
hmy_geo_z_rms = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_Z_RMS,hmy_geo_mtrx[1][2],0);

// FILTER MODULE
hmy_geo_z_rms2 = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_Z_RMS2,hmy_geo_mtrx[1][2],0);

// FILTER MODULE
hmy_geo_rz = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_RZ,hmy_geo_mtrx[1][5],0);

// FILTER MODULE
hmy_geo_vp = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_VP,hmy_geo_mtrx[1][6],0);

// FILTER MODULE
hmy_geo_hp = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_HP,hmy_geo_mtrx[1][7],0);

// FILTER MODULE
hmy_geo_rx = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_RX,hmy_geo_mtrx[1][3],0);

// FILTER MODULE
hmy_geo_ry = filterModuleD(dsp_ptr,dspCoeff,HMY_GEO_RY,hmy_geo_mtrx[1][4],0);

// RMS
hmy_geo_rms2 = hmy_geo_x_rms2;
if(hmy_geo_rms2 > 2000) hmy_geo_rms2 = 2000;
if(hmy_geo_rms2 < -2000) hmy_geo_rms2 = 2000;
hmy_geo_rms2 = hmy_geo_rms2 * hmy_geo_rms2;
hmy_geo_rms2_avg = hmy_geo_rms2 * .00005 + hmy_geo_rms2_avg * 0.99995;
hmy_geo_rms2 = lsqrt(hmy_geo_rms2_avg);

// RMS
hmy_geo_rms1 = hmy_geo_x_rms;
if(hmy_geo_rms1 > 2000) hmy_geo_rms1 = 2000;
if(hmy_geo_rms1 < -2000) hmy_geo_rms1 = 2000;
hmy_geo_rms1 = hmy_geo_rms1 * hmy_geo_rms1;
hmy_geo_rms1_avg = hmy_geo_rms1 * .00005 + hmy_geo_rms1_avg * 0.99995;
hmy_geo_rms1 = lsqrt(hmy_geo_rms1_avg);

// RMS
hmy_geo_rms4 = hmy_geo_y_rms2;
if(hmy_geo_rms4 > 2000) hmy_geo_rms4 = 2000;
if(hmy_geo_rms4 < -2000) hmy_geo_rms4 = 2000;
hmy_geo_rms4 = hmy_geo_rms4 * hmy_geo_rms4;
hmy_geo_rms4_avg = hmy_geo_rms4 * .00005 + hmy_geo_rms4_avg * 0.99995;
hmy_geo_rms4 = lsqrt(hmy_geo_rms4_avg);

// RMS
hmy_geo_rms3 = hmy_geo_y_rms;
if(hmy_geo_rms3 > 2000) hmy_geo_rms3 = 2000;
if(hmy_geo_rms3 < -2000) hmy_geo_rms3 = 2000;
hmy_geo_rms3 = hmy_geo_rms3 * hmy_geo_rms3;
hmy_geo_rms3_avg = hmy_geo_rms3 * .00005 + hmy_geo_rms3_avg * 0.99995;
hmy_geo_rms3 = lsqrt(hmy_geo_rms3_avg);

// RMS
hmy_geo_rms5 = hmy_geo_z_rms;
if(hmy_geo_rms5 > 2000) hmy_geo_rms5 = 2000;
if(hmy_geo_rms5 < -2000) hmy_geo_rms5 = 2000;
hmy_geo_rms5 = hmy_geo_rms5 * hmy_geo_rms5;
hmy_geo_rms5_avg = hmy_geo_rms5 * .00005 + hmy_geo_rms5_avg * 0.99995;
hmy_geo_rms5 = lsqrt(hmy_geo_rms5_avg);

// RMS
hmy_geo_rms6 = hmy_geo_z_rms2;
if(hmy_geo_rms6 > 2000) hmy_geo_rms6 = 2000;
if(hmy_geo_rms6 < -2000) hmy_geo_rms6 = 2000;
hmy_geo_rms6 = hmy_geo_rms6 * hmy_geo_rms6;
hmy_geo_rms6_avg = hmy_geo_rms6 * .00005 + hmy_geo_rms6_avg * 0.99995;
hmy_geo_rms6 = lsqrt(hmy_geo_rms6_avg);

// MultiSwitch
hmy_geo_sw[0] = hmy_geo_x;
hmy_geo_sw[1] = hmy_geo_y;
hmy_geo_sw[2] = hmy_geo_z;
hmy_geo_sw[3] = hmy_geo_rx;
hmy_geo_sw[4] = hmy_geo_ry;
hmy_geo_sw[5] = hmy_geo_rz;
hmy_geo_sw[6] = hmy_geo_vp;
hmy_geo_sw[7] = hmy_geo_hp;
if (pLocalEpics->sei.HMY_GEO_SW == 0) {
	for (ii=0; ii<8; ii++) hmy_geo_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->sei.HMY_GEO_X_RMS_2 = hmy_geo_rms2;

// EpicsOut
pLocalEpics->sei.HMY_GEO_X_RMS_1 = hmy_geo_rms1;

// EpicsOut
pLocalEpics->sei.HMY_GEO_Y_RMS_2 = hmy_geo_rms4;

// EpicsOut
pLocalEpics->sei.HMY_GEO_Y_RMS_1 = hmy_geo_rms3;

// EpicsOut
pLocalEpics->sei.HMY_GEO_Z_RMS_1 = hmy_geo_rms5;

// EpicsOut
pLocalEpics->sei.HMY_GEO_Z_RMS_2 = hmy_geo_rms6;


//End of subsystem   HMY_GEO **************************************************


// SUM
sum1 = sts1_fir_y_pp + sts1_fir_y_cf;

// SUM
sum2 = sts2_fir_x_pp + sts2_fir_x_cf;

// SUM
sum3 = sts1_fir_x_pp + sts1_fir_x_cf;

// SUM
sum4 = sts2_fir_y_pp + sts2_fir_y_cf;


//Start of subsystem BSC_FIR **************************************************

// Matrix
for(ii=0;ii<2;ii++)
{
bsc_fir_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_FIR_MTRX[ii][0] * sum3 +
	pLocalEpics->sei.BSC_FIR_MTRX[ii][1] * sum1 +
	pLocalEpics->sei.BSC_FIR_MTRX[ii][2] * sum2 +
	pLocalEpics->sei.BSC_FIR_MTRX[ii][3] * sum4;
}

// FILTER MODULE
bsc_fir_x_out = filterModuleD(dsp_ptr,dspCoeff,BSC_FIR_X_OUT,bsc_fir_mtrx[1][0],0);

// FILTER MODULE
bsc_fir_y_out = filterModuleD(dsp_ptr,dspCoeff,BSC_FIR_Y_OUT,bsc_fir_mtrx[1][1],0);


//End of subsystem   BSC_FIR **************************************************



//Start of subsystem BSC_STS **************************************************

// Matrix
for(ii=0;ii<3;ii++)
{
bsc_sts_in_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][0] * pLocalEpics->sei.STS1_X +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][1] * pLocalEpics->sei.STS1_Y +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][2] * pLocalEpics->sei.STS1_Z +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][3] * dWord[1][23] +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][4] * dWord[1][24] +
	pLocalEpics->sei.BSC_STS_IN_MTRX[ii][5] * dWord[1][25];
}

// FILTER MODULE
bsc_sts_y = filterModuleD(dsp_ptr,dspCoeff,BSC_STS_Y,bsc_sts_in_mtrx[1][1],0);

// FILTER MODULE
bsc_sts_x = filterModuleD(dsp_ptr,dspCoeff,BSC_STS_X,bsc_sts_in_mtrx[1][0],0);

// FILTER MODULE
bsc_sts_z = filterModuleD(dsp_ptr,dspCoeff,BSC_STS_Z,bsc_sts_in_mtrx[1][2],0);

// RampSwitch
bsc_sts_ramp_sw[0] = bsc_sts_x;
bsc_sts_ramp_sw[1] = bsc_fir_x_out;
bsc_sts_ramp_sw[2] = bsc_sts_y;
bsc_sts_ramp_sw[3] = bsc_fir_y_out;
if (pLocalEpics->sei.BSC_STS_RAMP_SW == 0)
{
	bsc_sts_ramp_sw[1] = bsc_sts_ramp_sw[2];
}
else
{
	bsc_sts_ramp_sw[0] = bsc_sts_ramp_sw[1];
	bsc_sts_ramp_sw[1] = bsc_sts_ramp_sw[3];
}


// FILTER MODULE
bsc_sts_y_rx = filterModuleD(dsp_ptr,dspCoeff,BSC_STS_Y_RX,bsc_sts_ramp_sw[1],0);

// Matrix
for(ii=0;ii<8;ii++)
{
bsc_sts_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_STS_MTRX[ii][0] * bsc_sts_ramp_sw[0] +
	pLocalEpics->sei.BSC_STS_MTRX[ii][1] * bsc_sts_ramp_sw[1] +
	pLocalEpics->sei.BSC_STS_MTRX[ii][2] * bsc_sts_z;
}

// FILTER MODULE
bsc_sts_x_ry = filterModuleD(dsp_ptr,dspCoeff,BSC_STS_X_RY,bsc_sts_ramp_sw[0],0);


//End of subsystem   BSC_STS **************************************************



//Start of subsystem BSC_WD **************************************************

// FILTER MODULE
bsc_wd_stsx = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_STSX,bsc_sts_in_mtrx[1][0],0);

// FILTER MODULE
bsc_wd_stsy = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_STSY,bsc_sts_in_mtrx[1][1],0);

// FILTER MODULE
bsc_wd_stsz = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_STSZ,bsc_sts_in_mtrx[1][2],0);

// FILTER MODULE
bsc_wd_pos_v1 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_V1,dWord[0][12],0);

// FILTER MODULE
bsc_wd_pos_v2 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_V2,dWord[0][13],0);

// FILTER MODULE
bsc_wd_pos_v3 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_V3,dWord[0][14],0);

// FILTER MODULE
bsc_wd_pos_v4 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_V4,dWord[0][15],0);

// FILTER MODULE
bsc_wd_pos_h1 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_H1,dWord[0][8],0);

// FILTER MODULE
bsc_wd_pos_h2 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_H2,dWord[0][9],0);

// FILTER MODULE
bsc_wd_pos_h3 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_H3,dWord[0][10],0);

// FILTER MODULE
bsc_wd_pos_h4 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_POS_H4,dWord[0][11],0);

// FILTER MODULE
bsc_wd_geo_v1 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_V1,dWord[0][4],0);

// FILTER MODULE
bsc_wd_geo_v2 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_V2,dWord[0][5],0);

// FILTER MODULE
bsc_wd_geo_v3 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_V3,dWord[0][6],0);

// FILTER MODULE
bsc_wd_geo_v4 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_V4,dWord[0][7],0);

// FILTER MODULE
bsc_wd_geo_h1 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_H1,dWord[0][0],0);

// FILTER MODULE
bsc_wd_geo_h2 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_H2,dWord[0][1],0);

// FILTER MODULE
bsc_wd_geo_h3 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_H3,dWord[0][2],0);

// FILTER MODULE
bsc_wd_geo_h4 = filterModuleD(dsp_ptr,dspCoeff,BSC_WD_GEO_H4,dWord[0][3],0);

// SeiWd GOES HERE ***

bsc_wd_mRaw[0] = bsc_sts_in_mtrx[1][0];
bsc_wd_mFilt[0] = bsc_wd_stsx;
bsc_wd_mRaw[1] = bsc_sts_in_mtrx[1][1];
bsc_wd_mFilt[1] = bsc_wd_stsy;
bsc_wd_mRaw[2] = bsc_sts_in_mtrx[1][2];
bsc_wd_mFilt[2] = bsc_wd_stsz;
bsc_wd_mRaw[3] = dWord[0][12];
bsc_wd_mFilt[3] = bsc_wd_pos_v1;
bsc_wd_mRaw[4] = dWord[0][13];
bsc_wd_mFilt[4] = bsc_wd_pos_v2;
bsc_wd_mRaw[5] = dWord[0][14];
bsc_wd_mFilt[5] = bsc_wd_pos_v3;
bsc_wd_mRaw[6] = dWord[0][15];
bsc_wd_mFilt[6] = bsc_wd_pos_v4;
bsc_wd_mRaw[7] = dWord[0][8];
bsc_wd_mFilt[7] = bsc_wd_pos_h1;
bsc_wd_mRaw[8] = dWord[0][9];
bsc_wd_mFilt[8] = bsc_wd_pos_h2;
bsc_wd_mRaw[9] = dWord[0][10];
bsc_wd_mFilt[9] = bsc_wd_pos_h3;
bsc_wd_mRaw[10] = dWord[0][11];
bsc_wd_mFilt[10] = bsc_wd_pos_h4;
bsc_wd_mRaw[11] = dWord[0][4];
bsc_wd_mFilt[11] = bsc_wd_geo_v1;
bsc_wd_mRaw[12] = dWord[0][5];
bsc_wd_mFilt[12] = bsc_wd_geo_v2;
bsc_wd_mRaw[13] = dWord[0][6];
bsc_wd_mFilt[13] = bsc_wd_geo_v3;
bsc_wd_mRaw[14] = dWord[0][7];
bsc_wd_mFilt[14] = bsc_wd_geo_v4;
bsc_wd_mRaw[15] = dWord[0][0];
bsc_wd_mFilt[15] = bsc_wd_geo_h1;
bsc_wd_mRaw[16] = dWord[0][1];
bsc_wd_mFilt[16] = bsc_wd_geo_h2;
bsc_wd_mRaw[17] = dWord[0][2];
bsc_wd_mFilt[17] = bsc_wd_geo_h3;
bsc_wd_mRaw[18] = dWord[0][3];
bsc_wd_mFilt[18] = bsc_wd_geo_h4;
seiwd1(cycle,bsc_wd_mRaw, bsc_wd_mFilt,&pLocalEpics->sei.BSC_WD_M);


//End of subsystem   BSC_WD **************************************************



//Start of subsystem HMX_FIR **************************************************

// Matrix
for(ii=0;ii<2;ii++)
{
hmx_fir_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_FIR_MTRX[ii][0] * sum3 +
	pLocalEpics->sei.HMX_FIR_MTRX[ii][1] * sum1 +
	pLocalEpics->sei.HMX_FIR_MTRX[ii][2] * sum2 +
	pLocalEpics->sei.HMX_FIR_MTRX[ii][3] * sum4;
}

// FILTER MODULE
hmx_fir_x_out = filterModuleD(dsp_ptr,dspCoeff,HMX_FIR_X_OUT,hmx_fir_mtrx[1][0],0);

// FILTER MODULE
hmx_fir_y_out = filterModuleD(dsp_ptr,dspCoeff,HMX_FIR_Y_OUT,hmx_fir_mtrx[1][1],0);


//End of subsystem   HMX_FIR **************************************************



//Start of subsystem HMX_STS **************************************************

// Matrix
for(ii=0;ii<3;ii++)
{
hmx_sts_in_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_STS_IN_MTRX[ii][0] * dWord[0][23] +
	pLocalEpics->sei.HMX_STS_IN_MTRX[ii][1] * dWord[0][24] +
	pLocalEpics->sei.HMX_STS_IN_MTRX[ii][2] * dWord[0][25] +
	pLocalEpics->sei.HMX_STS_IN_MTRX[ii][3] * dWord[1][23] +
	pLocalEpics->sei.HMX_STS_IN_MTRX[ii][4] * dWord[1][24] +
	pLocalEpics->sei.HMX_STS_IN_MTRX[ii][5] * dWord[1][25];
}

// FILTER MODULE
hmx_sts_y = filterModuleD(dsp_ptr,dspCoeff,HMX_STS_Y,hmx_sts_in_mtrx[1][1],0);

// FILTER MODULE
hmx_sts_x = filterModuleD(dsp_ptr,dspCoeff,HMX_STS_X,hmx_sts_in_mtrx[1][0],0);

// FILTER MODULE
hmx_sts_z = filterModuleD(dsp_ptr,dspCoeff,HMX_STS_Z,hmx_sts_in_mtrx[1][2],0);

// RampSwitch
hmx_sts_ramp_sw[0] = hmx_sts_x;
hmx_sts_ramp_sw[1] = hmx_fir_x_out;
hmx_sts_ramp_sw[2] = hmx_sts_y;
hmx_sts_ramp_sw[3] = hmx_fir_y_out;
if (pLocalEpics->sei.HMX_STS_RAMP_SW == 0)
{
	hmx_sts_ramp_sw[1] = hmx_sts_ramp_sw[2];
}
else
{
	hmx_sts_ramp_sw[0] = hmx_sts_ramp_sw[1];
	hmx_sts_ramp_sw[1] = hmx_sts_ramp_sw[3];
}


// FILTER MODULE
hmx_sts_y_rx = filterModuleD(dsp_ptr,dspCoeff,HMX_STS_Y_RX,hmx_sts_ramp_sw[1],0);

// Matrix
for(ii=0;ii<8;ii++)
{
hmx_sts_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_STS_MTRX[ii][0] * hmx_sts_ramp_sw[0] +
	pLocalEpics->sei.HMX_STS_MTRX[ii][1] * hmx_sts_ramp_sw[1] +
	pLocalEpics->sei.HMX_STS_MTRX[ii][2] * hmx_sts_z;
}

// FILTER MODULE
hmx_sts_x_ry = filterModuleD(dsp_ptr,dspCoeff,HMX_STS_X_RY,hmx_sts_ramp_sw[0],0);


//End of subsystem   HMX_STS **************************************************



//Start of subsystem HMX_WD **************************************************

// FILTER MODULE
hmx_wd_stsx = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_STSX,hmx_sts_in_mtrx[1][0],0);

// FILTER MODULE
hmx_wd_stsy = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_STSY,hmx_sts_in_mtrx[1][1],0);

// FILTER MODULE
hmx_wd_stsz = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_STSZ,hmx_sts_in_mtrx[1][2],0);

// FILTER MODULE
hmx_wd_pos_v1 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_V1,dWord[1][12],0);

// FILTER MODULE
hmx_wd_pos_v2 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_V2,dWord[1][13],0);

// FILTER MODULE
hmx_wd_pos_v3 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_V3,dWord[1][14],0);

// FILTER MODULE
hmx_wd_pos_v4 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_V4,dWord[1][15],0);

// FILTER MODULE
hmx_wd_pos_h1 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_H1,dWord[1][8],0);

// FILTER MODULE
hmx_wd_pos_h2 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_H2,dWord[1][9],0);

// FILTER MODULE
hmx_wd_pos_h3 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_H3,dWord[1][10],0);

// FILTER MODULE
hmx_wd_pos_h4 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_POS_H4,dWord[1][11],0);

// FILTER MODULE
hmx_wd_geo_v1 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_V1,dWord[1][4],0);

// FILTER MODULE
hmx_wd_geo_v2 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_V2,dWord[1][5],0);

// FILTER MODULE
hmx_wd_geo_v3 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_V3,dWord[1][6],0);

// FILTER MODULE
hmx_wd_geo_v4 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_V4,dWord[1][7],0);

// FILTER MODULE
hmx_wd_geo_h1 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_H1,dWord[1][0],0);

// FILTER MODULE
hmx_wd_geo_h2 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_H2,dWord[1][1],0);

// FILTER MODULE
hmx_wd_geo_h3 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_H3,dWord[1][2],0);

// FILTER MODULE
hmx_wd_geo_h4 = filterModuleD(dsp_ptr,dspCoeff,HMX_WD_GEO_H4,dWord[1][3],0);

// SeiWd GOES HERE ***

hmx_wd_mRaw[0] = hmx_sts_in_mtrx[1][0];
hmx_wd_mFilt[0] = hmx_wd_stsx;
hmx_wd_mRaw[1] = hmx_sts_in_mtrx[1][1];
hmx_wd_mFilt[1] = hmx_wd_stsy;
hmx_wd_mRaw[2] = hmx_sts_in_mtrx[1][2];
hmx_wd_mFilt[2] = hmx_wd_stsz;
hmx_wd_mRaw[3] = dWord[1][12];
hmx_wd_mFilt[3] = hmx_wd_pos_v1;
hmx_wd_mRaw[4] = dWord[1][13];
hmx_wd_mFilt[4] = hmx_wd_pos_v2;
hmx_wd_mRaw[5] = dWord[1][14];
hmx_wd_mFilt[5] = hmx_wd_pos_v3;
hmx_wd_mRaw[6] = dWord[1][15];
hmx_wd_mFilt[6] = hmx_wd_pos_v4;
hmx_wd_mRaw[7] = dWord[1][8];
hmx_wd_mFilt[7] = hmx_wd_pos_h1;
hmx_wd_mRaw[8] = dWord[1][9];
hmx_wd_mFilt[8] = hmx_wd_pos_h2;
hmx_wd_mRaw[9] = dWord[1][10];
hmx_wd_mFilt[9] = hmx_wd_pos_h3;
hmx_wd_mRaw[10] = dWord[1][11];
hmx_wd_mFilt[10] = hmx_wd_pos_h4;
hmx_wd_mRaw[11] = dWord[1][4];
hmx_wd_mFilt[11] = hmx_wd_geo_v1;
hmx_wd_mRaw[12] = dWord[1][5];
hmx_wd_mFilt[12] = hmx_wd_geo_v2;
hmx_wd_mRaw[13] = dWord[1][6];
hmx_wd_mFilt[13] = hmx_wd_geo_v3;
hmx_wd_mRaw[14] = dWord[1][7];
hmx_wd_mFilt[14] = hmx_wd_geo_v4;
hmx_wd_mRaw[15] = dWord[1][0];
hmx_wd_mFilt[15] = hmx_wd_geo_h1;
hmx_wd_mRaw[16] = dWord[1][1];
hmx_wd_mFilt[16] = hmx_wd_geo_h2;
hmx_wd_mRaw[17] = dWord[1][2];
hmx_wd_mFilt[17] = hmx_wd_geo_h3;
hmx_wd_mRaw[18] = dWord[1][3];
hmx_wd_mFilt[18] = hmx_wd_geo_h4;
seiwd1(cycle,hmx_wd_mRaw, hmx_wd_mFilt,&pLocalEpics->sei.HMX_WD_M);


//End of subsystem   HMX_WD **************************************************



//Start of subsystem HMY_FIR **************************************************

// Matrix
for(ii=0;ii<2;ii++)
{
hmy_fir_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_FIR_MTRX[ii][0] * sum3 +
	pLocalEpics->sei.HMY_FIR_MTRX[ii][1] * sum1 +
	pLocalEpics->sei.HMY_FIR_MTRX[ii][2] * sum2 +
	pLocalEpics->sei.HMY_FIR_MTRX[ii][3] * sum4;
}

// FILTER MODULE
hmy_fir_x_out = filterModuleD(dsp_ptr,dspCoeff,HMY_FIR_X_OUT,hmy_fir_mtrx[1][0],0);

// FILTER MODULE
hmy_fir_y_out = filterModuleD(dsp_ptr,dspCoeff,HMY_FIR_Y_OUT,hmy_fir_mtrx[1][1],0);


//End of subsystem   HMY_FIR **************************************************



//Start of subsystem HMY_STS **************************************************

// Matrix
for(ii=0;ii<3;ii++)
{
hmy_sts_in_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_STS_IN_MTRX[ii][0] * dWord[0][23] +
	pLocalEpics->sei.HMY_STS_IN_MTRX[ii][1] * dWord[0][24] +
	pLocalEpics->sei.HMY_STS_IN_MTRX[ii][2] * dWord[0][25] +
	pLocalEpics->sei.HMY_STS_IN_MTRX[ii][3] * dWord[1][23] +
	pLocalEpics->sei.HMY_STS_IN_MTRX[ii][4] * dWord[1][24] +
	pLocalEpics->sei.HMY_STS_IN_MTRX[ii][5] * dWord[1][25];
}

// FILTER MODULE
hmy_sts_y = filterModuleD(dsp_ptr,dspCoeff,HMY_STS_Y,hmy_sts_in_mtrx[1][1],0);

// FILTER MODULE
hmy_sts_x = filterModuleD(dsp_ptr,dspCoeff,HMY_STS_X,hmy_sts_in_mtrx[1][0],0);

// FILTER MODULE
hmy_sts_z = filterModuleD(dsp_ptr,dspCoeff,HMY_STS_Z,hmy_sts_in_mtrx[1][2],0);

// RampSwitch
hmy_sts_ramp_sw[0] = hmy_sts_x;
hmy_sts_ramp_sw[1] = hmy_fir_x_out;
hmy_sts_ramp_sw[2] = hmy_sts_y;
hmy_sts_ramp_sw[3] = hmy_fir_y_out;
if (pLocalEpics->sei.HMY_STS_RAMP_SW == 0)
{
	hmy_sts_ramp_sw[1] = hmy_sts_ramp_sw[2];
}
else
{
	hmy_sts_ramp_sw[0] = hmy_sts_ramp_sw[1];
	hmy_sts_ramp_sw[1] = hmy_sts_ramp_sw[3];
}


// FILTER MODULE
hmy_sts_y_rx = filterModuleD(dsp_ptr,dspCoeff,HMY_STS_Y_RX,hmy_sts_ramp_sw[1],0);

// Matrix
for(ii=0;ii<8;ii++)
{
hmy_sts_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_STS_MTRX[ii][0] * hmy_sts_ramp_sw[0] +
	pLocalEpics->sei.HMY_STS_MTRX[ii][1] * hmy_sts_ramp_sw[1] +
	pLocalEpics->sei.HMY_STS_MTRX[ii][2] * hmy_sts_z;
}

// FILTER MODULE
hmy_sts_x_ry = filterModuleD(dsp_ptr,dspCoeff,HMY_STS_X_RY,hmy_sts_ramp_sw[0],0);


//End of subsystem   HMY_STS **************************************************



//Start of subsystem HMY_WD **************************************************

// FILTER MODULE
hmy_wd_stsx = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_STSX,hmy_sts_in_mtrx[1][0],0);

// FILTER MODULE
hmy_wd_stsy = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_STSY,hmy_sts_in_mtrx[1][1],0);

// FILTER MODULE
hmy_wd_stsz = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_STSZ,hmy_sts_in_mtrx[1][2],0);

// FILTER MODULE
hmy_wd_pos_v1 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_V1,dWord[2][12],0);

// FILTER MODULE
hmy_wd_pos_v2 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_V2,dWord[2][13],0);

// FILTER MODULE
hmy_wd_pos_v3 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_V3,dWord[2][14],0);

// FILTER MODULE
hmy_wd_pos_v4 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_V4,dWord[2][15],0);

// FILTER MODULE
hmy_wd_pos_h1 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_H1,dWord[2][8],0);

// FILTER MODULE
hmy_wd_pos_h2 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_H2,dWord[2][9],0);

// FILTER MODULE
hmy_wd_pos_h3 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_H3,dWord[2][10],0);

// FILTER MODULE
hmy_wd_pos_h4 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_POS_H4,dWord[2][11],0);

// FILTER MODULE
hmy_wd_geo_v1 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_V1,dWord[2][4],0);

// FILTER MODULE
hmy_wd_geo_v2 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_V2,dWord[2][5],0);

// FILTER MODULE
hmy_wd_geo_v3 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_V3,dWord[2][6],0);

// FILTER MODULE
hmy_wd_geo_v4 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_V4,dWord[2][7],0);

// FILTER MODULE
hmy_wd_geo_h1 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_H1,dWord[2][0],0);

// FILTER MODULE
hmy_wd_geo_h2 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_H2,dWord[2][1],0);

// FILTER MODULE
hmy_wd_geo_h3 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_H3,dWord[2][2],0);

// FILTER MODULE
hmy_wd_geo_h4 = filterModuleD(dsp_ptr,dspCoeff,HMY_WD_GEO_H4,dWord[2][3],0);

// SeiWd GOES HERE ***

hmy_wd_mRaw[0] = hmy_sts_in_mtrx[1][0];
hmy_wd_mFilt[0] = hmy_wd_stsx;
hmy_wd_mRaw[1] = hmy_sts_in_mtrx[1][1];
hmy_wd_mFilt[1] = hmy_wd_stsy;
hmy_wd_mRaw[2] = hmy_sts_in_mtrx[1][2];
hmy_wd_mFilt[2] = hmy_wd_stsz;
hmy_wd_mRaw[3] = dWord[2][12];
hmy_wd_mFilt[3] = hmy_wd_pos_v1;
hmy_wd_mRaw[4] = dWord[2][13];
hmy_wd_mFilt[4] = hmy_wd_pos_v2;
hmy_wd_mRaw[5] = dWord[2][14];
hmy_wd_mFilt[5] = hmy_wd_pos_v3;
hmy_wd_mRaw[6] = dWord[2][15];
hmy_wd_mFilt[6] = hmy_wd_pos_v4;
hmy_wd_mRaw[7] = dWord[2][8];
hmy_wd_mFilt[7] = hmy_wd_pos_h1;
hmy_wd_mRaw[8] = dWord[2][9];
hmy_wd_mFilt[8] = hmy_wd_pos_h2;
hmy_wd_mRaw[9] = dWord[2][10];
hmy_wd_mFilt[9] = hmy_wd_pos_h3;
hmy_wd_mRaw[10] = dWord[2][11];
hmy_wd_mFilt[10] = hmy_wd_pos_h4;
hmy_wd_mRaw[11] = dWord[2][4];
hmy_wd_mFilt[11] = hmy_wd_geo_v1;
hmy_wd_mRaw[12] = dWord[2][5];
hmy_wd_mFilt[12] = hmy_wd_geo_v2;
hmy_wd_mRaw[13] = dWord[2][6];
hmy_wd_mFilt[13] = hmy_wd_geo_v3;
hmy_wd_mRaw[14] = dWord[2][7];
hmy_wd_mFilt[14] = hmy_wd_geo_v4;
hmy_wd_mRaw[15] = dWord[2][0];
hmy_wd_mFilt[15] = hmy_wd_geo_h1;
hmy_wd_mRaw[16] = dWord[2][1];
hmy_wd_mFilt[16] = hmy_wd_geo_h2;
hmy_wd_mRaw[17] = dWord[2][2];
hmy_wd_mFilt[17] = hmy_wd_geo_h3;
hmy_wd_mRaw[18] = dWord[2][3];
hmy_wd_mFilt[18] = hmy_wd_geo_h4;
seiwd1(cycle,hmy_wd_mRaw, hmy_wd_mFilt,&pLocalEpics->sei.HMY_WD_M);


//End of subsystem   HMY_WD **************************************************



//Start of subsystem BSC_POS **************************************************

// FILTER MODULE
bsc_pos_v1 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_V1,dWord[0][12],0);

// FILTER MODULE
bsc_pos_v2 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_V2,dWord[0][13],0);

// FILTER MODULE
bsc_pos_v3 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_V3,dWord[0][14],0);

// FILTER MODULE
bsc_pos_v4 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_V4,dWord[0][15],0);

// FILTER MODULE
bsc_pos_h1 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_H1,dWord[0][8],0);

// FILTER MODULE
bsc_pos_h2 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_H2,dWord[0][9],0);

// FILTER MODULE
bsc_pos_h3 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_H3,dWord[0][10],0);

// FILTER MODULE
bsc_pos_h4 = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_H4,dWord[0][11],0);

// DiffJunc
bsc_pos_norm[0] = bsc_pos_v1 - bsc_sts_mtrx[1][0];
bsc_pos_norm[1] = bsc_pos_v2 - bsc_sts_mtrx[1][1];
bsc_pos_norm[2] = bsc_pos_v3 - bsc_sts_mtrx[1][2];
bsc_pos_norm[3] = bsc_pos_v4 - bsc_sts_mtrx[1][3];
bsc_pos_norm[4] = bsc_pos_h1 - bsc_sts_mtrx[1][4];
bsc_pos_norm[5] = bsc_pos_h2 - bsc_sts_mtrx[1][5];
bsc_pos_norm[6] = bsc_pos_h3 - bsc_sts_mtrx[1][6];
bsc_pos_norm[7] = bsc_pos_h4 - bsc_sts_mtrx[1][7];

// Matrix
for(ii=0;ii<8;ii++)
{
bsc_pos_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_POS_MTRX[ii][0] * bsc_pos_norm[0] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][1] * bsc_pos_norm[1] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][2] * bsc_pos_norm[2] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][3] * bsc_pos_norm[3] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][4] * bsc_pos_norm[4] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][5] * bsc_pos_norm[5] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][6] * bsc_pos_norm[6] +
	pLocalEpics->sei.BSC_POS_MTRX[ii][7] * bsc_pos_norm[7];
}

// FILTER MODULE
bsc_pos_x = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_X,bsc_pos_mtrx[1][0],0);

// FILTER MODULE
bsc_pos_y = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_Y,bsc_pos_mtrx[1][1],0);

// FILTER MODULE
bsc_pos_z = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_Z,bsc_pos_mtrx[1][2],0);

// FILTER MODULE
bsc_pos_rz = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_RZ,bsc_pos_mtrx[1][5],0);

// FILTER MODULE
bsc_pos_vp = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_VP,bsc_pos_mtrx[1][6],0);

// FILTER MODULE
bsc_pos_hp = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_HP,bsc_pos_mtrx[1][7],0);

// SUM
bsc_pos_sum = bsc_pos_mtrx[1][3] + bsc_sts_y_rx;

// SUM
bsc_pos_sum1 = bsc_pos_mtrx[1][4] + bsc_sts_x_ry;

// FILTER MODULE
bsc_pos_rx = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_RX,bsc_pos_sum,0);

// FILTER MODULE
bsc_pos_ry = filterModuleD(dsp_ptr,dspCoeff,BSC_POS_RY,bsc_pos_sum1,0);

// MultiSwitch
bsc_pos_sw[0] = bsc_pos_x;
bsc_pos_sw[1] = bsc_pos_y;
bsc_pos_sw[2] = bsc_pos_z;
bsc_pos_sw[3] = bsc_pos_rx;
bsc_pos_sw[4] = bsc_pos_ry;
bsc_pos_sw[5] = bsc_pos_rz;
bsc_pos_sw[6] = bsc_pos_vp;
bsc_pos_sw[7] = bsc_pos_hp;
if (pLocalEpics->sei.BSC_POS_SW == 0) {
	for (ii=0; ii<8; ii++) bsc_pos_sw[ii] = 0.0;
}



//End of subsystem   BSC_POS **************************************************



//Start of subsystem HMX_POS **************************************************

// FILTER MODULE
hmx_pos_v1 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_V1,dWord[1][12],0);

// FILTER MODULE
hmx_pos_v2 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_V2,dWord[1][13],0);

// FILTER MODULE
hmx_pos_v3 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_V3,dWord[1][14],0);

// FILTER MODULE
hmx_pos_v4 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_V4,dWord[1][15],0);

// FILTER MODULE
hmx_pos_h1 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_H1,dWord[1][8],0);

// FILTER MODULE
hmx_pos_h2 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_H2,dWord[1][9],0);

// FILTER MODULE
hmx_pos_h3 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_H3,dWord[1][10],0);

// FILTER MODULE
hmx_pos_h4 = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_H4,dWord[1][11],0);

// DiffJunc
hmx_pos_norm[0] = hmx_pos_v1 - hmx_sts_mtrx[1][0];
hmx_pos_norm[1] = hmx_pos_v2 - hmx_sts_mtrx[1][1];
hmx_pos_norm[2] = hmx_pos_v3 - hmx_sts_mtrx[1][2];
hmx_pos_norm[3] = hmx_pos_v4 - hmx_sts_mtrx[1][3];
hmx_pos_norm[4] = hmx_pos_h1 - hmx_sts_mtrx[1][4];
hmx_pos_norm[5] = hmx_pos_h2 - hmx_sts_mtrx[1][5];
hmx_pos_norm[6] = hmx_pos_h3 - hmx_sts_mtrx[1][6];
hmx_pos_norm[7] = hmx_pos_h4 - hmx_sts_mtrx[1][7];

// Matrix
for(ii=0;ii<8;ii++)
{
hmx_pos_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_POS_MTRX[ii][0] * hmx_pos_norm[0] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][1] * hmx_pos_norm[1] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][2] * hmx_pos_norm[2] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][3] * hmx_pos_norm[3] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][4] * hmx_pos_norm[4] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][5] * hmx_pos_norm[5] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][6] * hmx_pos_norm[6] +
	pLocalEpics->sei.HMX_POS_MTRX[ii][7] * hmx_pos_norm[7];
}

// FILTER MODULE
hmx_pos_x = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_X,hmx_pos_mtrx[1][0],0);

// FILTER MODULE
hmx_pos_y = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_Y,hmx_pos_mtrx[1][1],0);

// FILTER MODULE
hmx_pos_z = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_Z,hmx_pos_mtrx[1][2],0);

// FILTER MODULE
hmx_pos_rz = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_RZ,hmx_pos_mtrx[1][5],0);

// FILTER MODULE
hmx_pos_vp = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_VP,hmx_pos_mtrx[1][6],0);

// FILTER MODULE
hmx_pos_hp = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_HP,hmx_pos_mtrx[1][7],0);

// SUM
hmx_pos_sum = hmx_pos_mtrx[1][3] + hmx_sts_y_rx;

// SUM
hmx_pos_sum1 = hmx_pos_mtrx[1][4] + hmx_sts_x_ry;

// FILTER MODULE
hmx_pos_rx = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_RX,hmx_pos_sum,0);

// FILTER MODULE
hmx_pos_ry = filterModuleD(dsp_ptr,dspCoeff,HMX_POS_RY,hmx_pos_sum1,0);

// MultiSwitch
hmx_pos_sw[0] = hmx_pos_x;
hmx_pos_sw[1] = hmx_pos_y;
hmx_pos_sw[2] = hmx_pos_z;
hmx_pos_sw[3] = hmx_pos_rx;
hmx_pos_sw[4] = hmx_pos_ry;
hmx_pos_sw[5] = hmx_pos_rz;
hmx_pos_sw[6] = hmx_pos_vp;
hmx_pos_sw[7] = hmx_pos_hp;
if (pLocalEpics->sei.HMX_POS_SW == 0) {
	for (ii=0; ii<8; ii++) hmx_pos_sw[ii] = 0.0;
}



//End of subsystem   HMX_POS **************************************************



//Start of subsystem HMY_POS **************************************************

// FILTER MODULE
hmy_pos_v1 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_V1,dWord[2][12],0);

// FILTER MODULE
hmy_pos_v2 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_V2,dWord[2][13],0);

// FILTER MODULE
hmy_pos_v3 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_V3,dWord[2][14],0);

// FILTER MODULE
hmy_pos_v4 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_V4,dWord[2][15],0);

// FILTER MODULE
hmy_pos_h1 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_H1,dWord[2][8],0);

// FILTER MODULE
hmy_pos_h2 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_H2,dWord[2][9],0);

// FILTER MODULE
hmy_pos_h3 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_H3,dWord[2][10],0);

// FILTER MODULE
hmy_pos_h4 = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_H4,dWord[2][11],0);

// DiffJunc
hmy_pos_norm[0] = hmy_pos_v1 - hmy_sts_mtrx[1][0];
hmy_pos_norm[1] = hmy_pos_v2 - hmy_sts_mtrx[1][1];
hmy_pos_norm[2] = hmy_pos_v3 - hmy_sts_mtrx[1][2];
hmy_pos_norm[3] = hmy_pos_v4 - hmy_sts_mtrx[1][3];
hmy_pos_norm[4] = hmy_pos_h1 - hmy_sts_mtrx[1][4];
hmy_pos_norm[5] = hmy_pos_h2 - hmy_sts_mtrx[1][5];
hmy_pos_norm[6] = hmy_pos_h3 - hmy_sts_mtrx[1][6];
hmy_pos_norm[7] = hmy_pos_h4 - hmy_sts_mtrx[1][7];

// Matrix
for(ii=0;ii<8;ii++)
{
hmy_pos_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_POS_MTRX[ii][0] * hmy_pos_norm[0] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][1] * hmy_pos_norm[1] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][2] * hmy_pos_norm[2] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][3] * hmy_pos_norm[3] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][4] * hmy_pos_norm[4] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][5] * hmy_pos_norm[5] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][6] * hmy_pos_norm[6] +
	pLocalEpics->sei.HMY_POS_MTRX[ii][7] * hmy_pos_norm[7];
}

// FILTER MODULE
hmy_pos_x = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_X,hmy_pos_mtrx[1][0],0);

// FILTER MODULE
hmy_pos_y = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_Y,hmy_pos_mtrx[1][1],0);

// FILTER MODULE
hmy_pos_z = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_Z,hmy_pos_mtrx[1][2],0);

// FILTER MODULE
hmy_pos_rz = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_RZ,hmy_pos_mtrx[1][5],0);

// FILTER MODULE
hmy_pos_vp = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_VP,hmy_pos_mtrx[1][6],0);

// FILTER MODULE
hmy_pos_hp = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_HP,hmy_pos_mtrx[1][7],0);

// SUM
hmy_pos_sum = hmy_pos_mtrx[1][3] + hmy_sts_y_rx;

// SUM
hmy_pos_sum1 = hmy_pos_mtrx[1][4] + hmy_sts_x_ry;

// FILTER MODULE
hmy_pos_rx = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_RX,hmy_pos_sum,0);

// FILTER MODULE
hmy_pos_ry = filterModuleD(dsp_ptr,dspCoeff,HMY_POS_RY,hmy_pos_sum1,0);

// MultiSwitch
hmy_pos_sw[0] = hmy_pos_x;
hmy_pos_sw[1] = hmy_pos_y;
hmy_pos_sw[2] = hmy_pos_z;
hmy_pos_sw[3] = hmy_pos_rx;
hmy_pos_sw[4] = hmy_pos_ry;
hmy_pos_sw[5] = hmy_pos_rz;
hmy_pos_sw[6] = hmy_pos_vp;
hmy_pos_sw[7] = hmy_pos_hp;
if (pLocalEpics->sei.HMY_POS_SW == 0) {
	for (ii=0; ii<8; ii++) hmy_pos_sw[ii] = 0.0;
}



//End of subsystem   HMY_POS **************************************************



//Start of subsystem BSC **************************************************

// SUM
bsc_sum = bsc_pos_sw[0] + bsc_geo_sw[0];

// SUM
bsc_sum1 = bsc_pos_sw[1] + bsc_geo_sw[1];

// SUM
bsc_sum2 = bsc_pos_sw[2] + bsc_geo_sw[2];

// SUM
bsc_sum3 = bsc_pos_sw[3] + bsc_geo_sw[3];

// SUM
bsc_sum4 = bsc_pos_sw[4] + bsc_geo_sw[4];

// SUM
bsc_sum5 = bsc_pos_sw[5] + bsc_geo_sw[5];

// SUM
bsc_sum6 = bsc_pos_sw[6] + bsc_geo_sw[6];

// SUM
bsc_sum7 = bsc_pos_sw[7] + bsc_geo_sw[7];

// FILTER MODULE
bsc_x = filterModuleD(dsp_ptr,dspCoeff,BSC_X,bsc_sum,0);

// FILTER MODULE
bsc_y = filterModuleD(dsp_ptr,dspCoeff,BSC_Y,bsc_sum1,0);

// FILTER MODULE
bsc_z = filterModuleD(dsp_ptr,dspCoeff,BSC_Z,bsc_sum2,0);

// FILTER MODULE
bsc_rx = filterModuleD(dsp_ptr,dspCoeff,BSC_RX,bsc_sum3,0);

// FILTER MODULE
bsc_ry = filterModuleD(dsp_ptr,dspCoeff,BSC_RY,bsc_sum4,0);

// FILTER MODULE
bsc_rz = filterModuleD(dsp_ptr,dspCoeff,BSC_RZ,bsc_sum5,0);

// FILTER MODULE
bsc_vp = filterModuleD(dsp_ptr,dspCoeff,BSC_VP,bsc_sum6,0);

// FILTER MODULE
bsc_hp = filterModuleD(dsp_ptr,dspCoeff,BSC_HP,bsc_sum7,0);

// Matrix
for(ii=0;ii<8;ii++)
{
bsc_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_MTRX[ii][0] * bsc_x +
	pLocalEpics->sei.BSC_MTRX[ii][1] * bsc_y +
	pLocalEpics->sei.BSC_MTRX[ii][2] * bsc_z +
	pLocalEpics->sei.BSC_MTRX[ii][3] * bsc_rx +
	pLocalEpics->sei.BSC_MTRX[ii][4] * bsc_ry +
	pLocalEpics->sei.BSC_MTRX[ii][5] * bsc_rz +
	pLocalEpics->sei.BSC_MTRX[ii][6] * bsc_vp +
	pLocalEpics->sei.BSC_MTRX[ii][7] * bsc_hp;
}

// PRODUCT
pLocalEpics->sei.BSC_LOOP_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.BSC_LOOP_GAIN,pLocalEpics->sei.BSC_LOOP_GAIN_TRAMP,0,&BSC_LOOP_GAIN_CALC);


// MultiSwitch
bsc_loop_sw[0] = bsc_loop_gain[0];
bsc_loop_sw[1] = bsc_loop_gain[1];
bsc_loop_sw[2] = bsc_loop_gain[2];
bsc_loop_sw[3] = bsc_loop_gain[3];
bsc_loop_sw[4] = bsc_loop_gain[4];
bsc_loop_sw[5] = bsc_loop_gain[5];
bsc_loop_sw[6] = bsc_loop_gain[6];
bsc_loop_sw[7] = bsc_loop_gain[7];
if (pLocalEpics->sei.BSC_LOOP_SW == 0) {
	for (ii=0; ii<16; ii++) bsc_loop_sw[ii] = 0.0;
}


// Matrix
for(ii=0;ii<8;ii++)
{
bsc_tiltcorr_mtrx[1][ii] = 
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][0] * bsc_loop_sw[0] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][1] * bsc_loop_sw[1] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][2] * bsc_loop_sw[2] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][3] * bsc_loop_sw[3] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][4] * bsc_loop_sw[4] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][5] * bsc_loop_sw[5] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][6] * bsc_loop_sw[6] +
	pLocalEpics->sei.BSC_TILTCORR_MTRX[ii][7] * bsc_loop_sw[7];
}

// FILTER MODULE
bsc_tiltcorr_v1 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_V1,bsc_tiltcorr_mtrx[1][0],0);

// FILTER MODULE
bsc_tiltcorr_v2 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_V2,bsc_tiltcorr_mtrx[1][1],0);

// FILTER MODULE
bsc_tiltcorr_v3 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_V3,bsc_tiltcorr_mtrx[1][2],0);

// FILTER MODULE
bsc_tiltcorr_v4 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_V4,bsc_tiltcorr_mtrx[1][3],0);

// FILTER MODULE
bsc_tiltcorr_h1 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_H1,bsc_tiltcorr_mtrx[1][4],0);

// FILTER MODULE
bsc_tiltcorr_h2 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_H2,bsc_tiltcorr_mtrx[1][5],0);

// FILTER MODULE
bsc_tiltcorr_h3 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_H3,bsc_tiltcorr_mtrx[1][6],0);

// FILTER MODULE
bsc_tiltcorr_h4 = filterModuleD(dsp_ptr,dspCoeff,BSC_TILTCORR_H4,bsc_tiltcorr_mtrx[1][7],0);

// PRODUCT
pLocalEpics->sei.BSC_TILTCORR_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.BSC_TILTCORR_GAIN,pLocalEpics->sei.BSC_TILTCORR_GAIN_TRAMP,0,&BSC_TILTCORR_GAIN_CALC);


// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_V1 = bsc_tiltcorr_gain[0];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_V2 = bsc_tiltcorr_gain[1];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_V3 = bsc_tiltcorr_gain[2];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_V4 = bsc_tiltcorr_gain[3];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_H1 = bsc_tiltcorr_gain[4];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_H2 = bsc_tiltcorr_gain[5];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_H3 = bsc_tiltcorr_gain[6];

// EpicsOut
pLocalEpics->sei.BSC_TILTCORR_MAT_H4 = bsc_tiltcorr_gain[7];

// MultiSwitch
bsc_tiltcorr_sw[0] = pLocalEpics->sei.BSC_TILTCORR_MAT_V1;
bsc_tiltcorr_sw[1] = pLocalEpics->sei.BSC_TILTCORR_MAT_V2;
bsc_tiltcorr_sw[2] = pLocalEpics->sei.BSC_TILTCORR_MAT_V3;
bsc_tiltcorr_sw[3] = pLocalEpics->sei.BSC_TILTCORR_MAT_V4;
bsc_tiltcorr_sw[4] = pLocalEpics->sei.BSC_TILTCORR_MAT_H1;
bsc_tiltcorr_sw[5] = pLocalEpics->sei.BSC_TILTCORR_MAT_H2;
bsc_tiltcorr_sw[6] = pLocalEpics->sei.BSC_TILTCORR_MAT_H3;
bsc_tiltcorr_sw[7] = pLocalEpics->sei.BSC_TILTCORR_MAT_H4;
if (pLocalEpics->sei.BSC_TILTCORR_SW == 0) {
	for (ii=0; ii<8; ii++) bsc_tiltcorr_sw[ii] = 0.0;
}



//End of subsystem   BSC **************************************************



//Start of subsystem BSC_ACT **************************************************

// SUM
bsc_act_sum = bsc_loop_sw[0] + bsc_dc_bias_sw[0];

// SUM
bsc_act_sum1 = bsc_loop_sw[1] + bsc_dc_bias_sw[1];

// SUM
bsc_act_sum2 = bsc_loop_sw[2] + bsc_dc_bias_sw[2];

// SUM
bsc_act_sum3 = bsc_loop_sw[3] + bsc_dc_bias_sw[3];

// SUM
bsc_act_sum4 = bsc_loop_sw[4] + bsc_dc_bias_sw[4];

// SUM
bsc_act_sum5 = bsc_loop_sw[5] + bsc_dc_bias_sw[5];

// SUM
bsc_act_sum6 = bsc_loop_sw[6] + bsc_dc_bias_sw[6];

// SUM
bsc_act_sum7 = bsc_loop_sw[7] + bsc_dc_bias_sw[7];

// FILTER MODULE
bsc_act_v1 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_V1,bsc_act_sum,0);

// FILTER MODULE
bsc_act_v2 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_V2,bsc_act_sum1,0);

// FILTER MODULE
bsc_act_v3 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_V3,bsc_act_sum2,0);

// FILTER MODULE
bsc_act_v4 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_V4,bsc_act_sum3,0);

// FILTER MODULE
bsc_act_h1 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_H1,bsc_act_sum4,0);

// FILTER MODULE
bsc_act_h2 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_H2,bsc_act_sum5,0);

// FILTER MODULE
bsc_act_h3 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_H3,bsc_act_sum6,0);

// FILTER MODULE
bsc_act_h4 = filterModuleD(dsp_ptr,dspCoeff,BSC_ACT_H4,bsc_act_sum7,0);

// PRODUCT
pLocalEpics->sei.BSC_ACT_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.BSC_ACT_GAIN,pLocalEpics->sei.BSC_ACT_GAIN_TRAMP,0,&BSC_ACT_GAIN_CALC);


// MultiSwitch
bsc_act_sw[0] = bsc_act_gain[0];
bsc_act_sw[1] = bsc_act_gain[1];
bsc_act_sw[2] = bsc_act_gain[2];
bsc_act_sw[3] = bsc_act_gain[3];
bsc_act_sw[4] = bsc_act_gain[4];
bsc_act_sw[5] = bsc_act_gain[5];
bsc_act_sw[6] = bsc_act_gain[6];
bsc_act_sw[7] = bsc_act_gain[7];
if (pLocalEpics->sei.BSC_ACT_SW == 0) {
	for (ii=0; ii<8; ii++) bsc_act_sw[ii] = 0.0;
}



//End of subsystem   BSC_ACT **************************************************



//Start of subsystem HMX **************************************************

// SUM
hmx_sum = hmx_pos_sw[0] + hmx_geo_sw[0];

// SUM
hmx_sum1 = hmx_pos_sw[1] + hmx_geo_sw[1];

// SUM
hmx_sum2 = hmx_pos_sw[2] + hmx_geo_sw[2];

// SUM
hmx_sum3 = hmx_pos_sw[3] + hmx_geo_sw[3];

// SUM
hmx_sum4 = hmx_pos_sw[4] + hmx_geo_sw[4];

// SUM
hmx_sum5 = hmx_pos_sw[5] + hmx_geo_sw[5];

// SUM
hmx_sum6 = hmx_pos_sw[6] + hmx_geo_sw[6];

// SUM
hmx_sum7 = hmx_pos_sw[7] + hmx_geo_sw[7];

// FILTER MODULE
hmx_x = filterModuleD(dsp_ptr,dspCoeff,HMX_X,hmx_sum,0);

// FILTER MODULE
hmx_y = filterModuleD(dsp_ptr,dspCoeff,HMX_Y,hmx_sum1,0);

// FILTER MODULE
hmx_z = filterModuleD(dsp_ptr,dspCoeff,HMX_Z,hmx_sum2,0);

// FILTER MODULE
hmx_rx = filterModuleD(dsp_ptr,dspCoeff,HMX_RX,hmx_sum3,0);

// FILTER MODULE
hmx_ry = filterModuleD(dsp_ptr,dspCoeff,HMX_RY,hmx_sum4,0);

// FILTER MODULE
hmx_rz = filterModuleD(dsp_ptr,dspCoeff,HMX_RZ,hmx_sum5,0);

// FILTER MODULE
hmx_vp = filterModuleD(dsp_ptr,dspCoeff,HMX_VP,hmx_sum6,0);

// FILTER MODULE
hmx_hp = filterModuleD(dsp_ptr,dspCoeff,HMX_HP,hmx_sum7,0);

// Matrix
for(ii=0;ii<8;ii++)
{
hmx_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_MTRX[ii][0] * hmx_x +
	pLocalEpics->sei.HMX_MTRX[ii][1] * hmx_y +
	pLocalEpics->sei.HMX_MTRX[ii][2] * hmx_z +
	pLocalEpics->sei.HMX_MTRX[ii][3] * hmx_rx +
	pLocalEpics->sei.HMX_MTRX[ii][4] * hmx_ry +
	pLocalEpics->sei.HMX_MTRX[ii][5] * hmx_rz +
	pLocalEpics->sei.HMX_MTRX[ii][6] * hmx_vp +
	pLocalEpics->sei.HMX_MTRX[ii][7] * hmx_hp;
}

// PRODUCT
pLocalEpics->sei.HMX_LOOP_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.HMX_LOOP_GAIN,pLocalEpics->sei.HMX_LOOP_GAIN_TRAMP,0,&HMX_LOOP_GAIN_CALC);


// MultiSwitch
hmx_loop_sw[0] = hmx_loop_gain[0];
hmx_loop_sw[1] = hmx_loop_gain[1];
hmx_loop_sw[2] = hmx_loop_gain[2];
hmx_loop_sw[3] = hmx_loop_gain[3];
hmx_loop_sw[4] = hmx_loop_gain[4];
hmx_loop_sw[5] = hmx_loop_gain[5];
hmx_loop_sw[6] = hmx_loop_gain[6];
hmx_loop_sw[7] = hmx_loop_gain[7];
if (pLocalEpics->sei.HMX_LOOP_SW == 0) {
	for (ii=0; ii<16; ii++) hmx_loop_sw[ii] = 0.0;
}


// Matrix
for(ii=0;ii<8;ii++)
{
hmx_tiltcorr_mtrx[1][ii] = 
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][0] * hmx_loop_sw[0] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][1] * hmx_loop_sw[1] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][2] * hmx_loop_sw[2] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][3] * hmx_loop_sw[3] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][4] * hmx_loop_sw[4] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][5] * hmx_loop_sw[5] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][6] * hmx_loop_sw[6] +
	pLocalEpics->sei.HMX_TILTCORR_MTRX[ii][7] * hmx_loop_sw[7];
}

// FILTER MODULE
hmx_tiltcorr_v1 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_V1,hmx_tiltcorr_mtrx[1][0],0);

// FILTER MODULE
hmx_tiltcorr_v2 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_V2,hmx_tiltcorr_mtrx[1][1],0);

// FILTER MODULE
hmx_tiltcorr_v3 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_V3,hmx_tiltcorr_mtrx[1][2],0);

// FILTER MODULE
hmx_tiltcorr_v4 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_V4,hmx_tiltcorr_mtrx[1][3],0);

// FILTER MODULE
hmx_tiltcorr_h1 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_H1,hmx_tiltcorr_mtrx[1][4],0);

// FILTER MODULE
hmx_tiltcorr_h2 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_H2,hmx_tiltcorr_mtrx[1][5],0);

// FILTER MODULE
hmx_tiltcorr_h3 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_H3,hmx_tiltcorr_mtrx[1][6],0);

// FILTER MODULE
hmx_tiltcorr_h4 = filterModuleD(dsp_ptr,dspCoeff,HMX_TILTCORR_H4,hmx_tiltcorr_mtrx[1][7],0);

// PRODUCT
pLocalEpics->sei.HMX_TILTCORR_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.HMX_TILTCORR_GAIN,pLocalEpics->sei.HMX_TILTCORR_GAIN_TRAMP,0,&HMX_TILTCORR_GAIN_CALC);


// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_V1 = hmx_tiltcorr_gain[0];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_V2 = hmx_tiltcorr_gain[1];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_V3 = hmx_tiltcorr_gain[2];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_V4 = hmx_tiltcorr_gain[3];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_H1 = hmx_tiltcorr_gain[4];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_H2 = hmx_tiltcorr_gain[5];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_H3 = hmx_tiltcorr_gain[6];

// EpicsOut
pLocalEpics->sei.HMX_TILTCORR_MAT_H4 = hmx_tiltcorr_gain[7];

// MultiSwitch
hmx_tiltcorr_sw[0] = pLocalEpics->sei.HMX_TILTCORR_MAT_V1;
hmx_tiltcorr_sw[1] = pLocalEpics->sei.HMX_TILTCORR_MAT_V2;
hmx_tiltcorr_sw[2] = pLocalEpics->sei.HMX_TILTCORR_MAT_V3;
hmx_tiltcorr_sw[3] = pLocalEpics->sei.HMX_TILTCORR_MAT_V4;
hmx_tiltcorr_sw[4] = pLocalEpics->sei.HMX_TILTCORR_MAT_H1;
hmx_tiltcorr_sw[5] = pLocalEpics->sei.HMX_TILTCORR_MAT_H2;
hmx_tiltcorr_sw[6] = pLocalEpics->sei.HMX_TILTCORR_MAT_H3;
hmx_tiltcorr_sw[7] = pLocalEpics->sei.HMX_TILTCORR_MAT_H4;
if (pLocalEpics->sei.HMX_TILTCORR_SW == 0) {
	for (ii=0; ii<8; ii++) hmx_tiltcorr_sw[ii] = 0.0;
}



//End of subsystem   HMX **************************************************



//Start of subsystem HMX_ACT **************************************************

// SUM
hmx_act_sum = hmx_loop_sw[0] + hmx_dc_bias_sw[0];

// SUM
hmx_act_sum1 = hmx_loop_sw[1] + hmx_dc_bias_sw[1];

// SUM
hmx_act_sum2 = hmx_loop_sw[2] + hmx_dc_bias_sw[2];

// SUM
hmx_act_sum3 = hmx_loop_sw[3] + hmx_dc_bias_sw[3];

// SUM
hmx_act_sum4 = hmx_loop_sw[4] + hmx_dc_bias_sw[4];

// SUM
hmx_act_sum5 = hmx_loop_sw[5] + hmx_dc_bias_sw[5];

// SUM
hmx_act_sum6 = hmx_loop_sw[6] + hmx_dc_bias_sw[6];

// SUM
hmx_act_sum7 = hmx_loop_sw[7] + hmx_dc_bias_sw[7];

// FILTER MODULE
hmx_act_v1 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_V1,hmx_act_sum,0);

// FILTER MODULE
hmx_act_v2 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_V2,hmx_act_sum1,0);

// FILTER MODULE
hmx_act_v3 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_V3,hmx_act_sum2,0);

// FILTER MODULE
hmx_act_v4 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_V4,hmx_act_sum3,0);

// FILTER MODULE
hmx_act_h1 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_H1,hmx_act_sum4,0);

// FILTER MODULE
hmx_act_h2 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_H2,hmx_act_sum5,0);

// FILTER MODULE
hmx_act_h3 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_H3,hmx_act_sum6,0);

// FILTER MODULE
hmx_act_h4 = filterModuleD(dsp_ptr,dspCoeff,HMX_ACT_H4,hmx_act_sum7,0);

// PRODUCT
pLocalEpics->sei.HMX_ACT_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.HMX_ACT_GAIN,pLocalEpics->sei.HMX_ACT_GAIN_TRAMP,0,&HMX_ACT_GAIN_CALC);


// MultiSwitch
hmx_act_sw[0] = hmx_act_gain[0];
hmx_act_sw[1] = hmx_act_gain[1];
hmx_act_sw[2] = hmx_act_gain[2];
hmx_act_sw[3] = hmx_act_gain[3];
hmx_act_sw[4] = hmx_act_gain[4];
hmx_act_sw[5] = hmx_act_gain[5];
hmx_act_sw[6] = hmx_act_gain[6];
hmx_act_sw[7] = hmx_act_gain[7];
if (pLocalEpics->sei.HMX_ACT_SW == 0) {
	for (ii=0; ii<8; ii++) hmx_act_sw[ii] = 0.0;
}



//End of subsystem   HMX_ACT **************************************************



//Start of subsystem HMY **************************************************

// SUM
hmy_sum = hmy_pos_sw[0] + hmy_geo_sw[0];

// SUM
hmy_sum1 = hmy_pos_sw[1] + hmy_geo_sw[1];

// SUM
hmy_sum2 = hmy_pos_sw[2] + hmy_geo_sw[2];

// SUM
hmy_sum3 = hmy_pos_sw[3] + hmy_geo_sw[3];

// SUM
hmy_sum4 = hmy_pos_sw[4] + hmy_geo_sw[4];

// SUM
hmy_sum5 = hmy_pos_sw[5] + hmy_geo_sw[5];

// SUM
hmy_sum6 = hmy_pos_sw[6] + hmy_geo_sw[6];

// SUM
hmy_sum7 = hmy_pos_sw[7] + hmy_geo_sw[7];

// FILTER MODULE
hmy_x = filterModuleD(dsp_ptr,dspCoeff,HMY_X,hmy_sum,0);

// FILTER MODULE
hmy_y = filterModuleD(dsp_ptr,dspCoeff,HMY_Y,hmy_sum1,0);

// FILTER MODULE
hmy_z = filterModuleD(dsp_ptr,dspCoeff,HMY_Z,hmy_sum2,0);

// FILTER MODULE
hmy_rx = filterModuleD(dsp_ptr,dspCoeff,HMY_RX,hmy_sum3,0);

// FILTER MODULE
hmy_ry = filterModuleD(dsp_ptr,dspCoeff,HMY_RY,hmy_sum4,0);

// FILTER MODULE
hmy_rz = filterModuleD(dsp_ptr,dspCoeff,HMY_RZ,hmy_sum5,0);

// FILTER MODULE
hmy_vp = filterModuleD(dsp_ptr,dspCoeff,HMY_VP,hmy_sum6,0);

// FILTER MODULE
hmy_hp = filterModuleD(dsp_ptr,dspCoeff,HMY_HP,hmy_sum7,0);

// Matrix
for(ii=0;ii<8;ii++)
{
hmy_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_MTRX[ii][0] * hmy_x +
	pLocalEpics->sei.HMY_MTRX[ii][1] * hmy_y +
	pLocalEpics->sei.HMY_MTRX[ii][2] * hmy_z +
	pLocalEpics->sei.HMY_MTRX[ii][3] * hmy_rx +
	pLocalEpics->sei.HMY_MTRX[ii][4] * hmy_ry +
	pLocalEpics->sei.HMY_MTRX[ii][5] * hmy_rz +
	pLocalEpics->sei.HMY_MTRX[ii][6] * hmy_vp +
	pLocalEpics->sei.HMY_MTRX[ii][7] * hmy_hp;
}

// PRODUCT
pLocalEpics->sei.HMY_LOOP_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.HMY_LOOP_GAIN,pLocalEpics->sei.HMY_LOOP_GAIN_TRAMP,0,&HMY_LOOP_GAIN_CALC);


// MultiSwitch
hmy_loop_sw[0] = hmy_loop_gain[0];
hmy_loop_sw[1] = hmy_loop_gain[1];
hmy_loop_sw[2] = hmy_loop_gain[2];
hmy_loop_sw[3] = hmy_loop_gain[3];
hmy_loop_sw[4] = hmy_loop_gain[4];
hmy_loop_sw[5] = hmy_loop_gain[5];
hmy_loop_sw[6] = hmy_loop_gain[6];
hmy_loop_sw[7] = hmy_loop_gain[7];
if (pLocalEpics->sei.HMY_LOOP_SW == 0) {
	for (ii=0; ii<16; ii++) hmy_loop_sw[ii] = 0.0;
}


// Matrix
for(ii=0;ii<8;ii++)
{
hmy_tiltcorr_mtrx[1][ii] = 
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][0] * hmy_loop_sw[0] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][1] * hmy_loop_sw[1] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][2] * hmy_loop_sw[2] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][3] * hmy_loop_sw[3] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][4] * hmy_loop_sw[4] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][5] * hmy_loop_sw[5] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][6] * hmy_loop_sw[6] +
	pLocalEpics->sei.HMY_TILTCORR_MTRX[ii][7] * hmy_loop_sw[7];
}

// FILTER MODULE
hmy_tiltcorr_v1 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_V1,hmy_tiltcorr_mtrx[1][0],0);

// FILTER MODULE
hmy_tiltcorr_v2 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_V2,hmy_tiltcorr_mtrx[1][1],0);

// FILTER MODULE
hmy_tiltcorr_v3 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_V3,hmy_tiltcorr_mtrx[1][2],0);

// FILTER MODULE
hmy_tiltcorr_v4 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_V4,hmy_tiltcorr_mtrx[1][3],0);

// FILTER MODULE
hmy_tiltcorr_h1 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_H1,hmy_tiltcorr_mtrx[1][4],0);

// FILTER MODULE
hmy_tiltcorr_h2 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_H2,hmy_tiltcorr_mtrx[1][5],0);

// FILTER MODULE
hmy_tiltcorr_h3 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_H3,hmy_tiltcorr_mtrx[1][6],0);

// FILTER MODULE
hmy_tiltcorr_h4 = filterModuleD(dsp_ptr,dspCoeff,HMY_TILTCORR_H4,hmy_tiltcorr_mtrx[1][7],0);

// PRODUCT
pLocalEpics->sei.HMY_TILTCORR_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.HMY_TILTCORR_GAIN,pLocalEpics->sei.HMY_TILTCORR_GAIN_TRAMP,0,&HMY_TILTCORR_GAIN_CALC);


// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_V1 = hmy_tiltcorr_gain[0];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_V2 = hmy_tiltcorr_gain[1];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_V3 = hmy_tiltcorr_gain[2];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_V4 = hmy_tiltcorr_gain[3];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_H1 = hmy_tiltcorr_gain[4];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_H2 = hmy_tiltcorr_gain[5];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_H3 = hmy_tiltcorr_gain[6];

// EpicsOut
pLocalEpics->sei.HMY_TILTCORR_MAT_H4 = hmy_tiltcorr_gain[7];

// MultiSwitch
hmy_tiltcorr_sw[0] = pLocalEpics->sei.HMY_TILTCORR_MAT_V1;
hmy_tiltcorr_sw[1] = pLocalEpics->sei.HMY_TILTCORR_MAT_V2;
hmy_tiltcorr_sw[2] = pLocalEpics->sei.HMY_TILTCORR_MAT_V3;
hmy_tiltcorr_sw[3] = pLocalEpics->sei.HMY_TILTCORR_MAT_V4;
hmy_tiltcorr_sw[4] = pLocalEpics->sei.HMY_TILTCORR_MAT_H1;
hmy_tiltcorr_sw[5] = pLocalEpics->sei.HMY_TILTCORR_MAT_H2;
hmy_tiltcorr_sw[6] = pLocalEpics->sei.HMY_TILTCORR_MAT_H3;
hmy_tiltcorr_sw[7] = pLocalEpics->sei.HMY_TILTCORR_MAT_H4;
if (pLocalEpics->sei.HMY_TILTCORR_SW == 0) {
	for (ii=0; ii<8; ii++) hmy_tiltcorr_sw[ii] = 0.0;
}



//End of subsystem   HMY **************************************************



//Start of subsystem HMY_ACT **************************************************

// SUM
hmy_act_sum = hmy_loop_sw[0] + hmy_dc_bias_sw[0];

// SUM
hmy_act_sum1 = hmy_loop_sw[1] + hmy_dc_bias_sw[1];

// SUM
hmy_act_sum2 = hmy_loop_sw[2] + hmy_dc_bias_sw[2];

// SUM
hmy_act_sum3 = hmy_loop_sw[3] + hmy_dc_bias_sw[3];

// SUM
hmy_act_sum4 = hmy_loop_sw[4] + hmy_dc_bias_sw[4];

// SUM
hmy_act_sum5 = hmy_loop_sw[5] + hmy_dc_bias_sw[5];

// SUM
hmy_act_sum6 = hmy_loop_sw[6] + hmy_dc_bias_sw[6];

// SUM
hmy_act_sum7 = hmy_loop_sw[7] + hmy_dc_bias_sw[7];

// FILTER MODULE
hmy_act_v1 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_V1,hmy_act_sum,0);

// FILTER MODULE
hmy_act_v2 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_V2,hmy_act_sum1,0);

// FILTER MODULE
hmy_act_v3 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_V3,hmy_act_sum2,0);

// FILTER MODULE
hmy_act_v4 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_V4,hmy_act_sum3,0);

// FILTER MODULE
hmy_act_h1 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_H1,hmy_act_sum4,0);

// FILTER MODULE
hmy_act_h2 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_H2,hmy_act_sum5,0);

// FILTER MODULE
hmy_act_h3 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_H3,hmy_act_sum6,0);

// FILTER MODULE
hmy_act_h4 = filterModuleD(dsp_ptr,dspCoeff,HMY_ACT_H4,hmy_act_sum7,0);

// PRODUCT
pLocalEpics->sei.HMY_ACT_GAIN_RMON = 
	gainRamp(pLocalEpics->sei.HMY_ACT_GAIN,pLocalEpics->sei.HMY_ACT_GAIN_TRAMP,0,&HMY_ACT_GAIN_CALC);


// MultiSwitch
hmy_act_sw[0] = hmy_act_gain[0];
hmy_act_sw[1] = hmy_act_gain[1];
hmy_act_sw[2] = hmy_act_gain[2];
hmy_act_sw[3] = hmy_act_gain[3];
hmy_act_sw[4] = hmy_act_gain[4];
hmy_act_sw[5] = hmy_act_gain[5];
hmy_act_sw[6] = hmy_act_gain[6];
hmy_act_sw[7] = hmy_act_gain[7];
if (pLocalEpics->sei.HMY_ACT_SW == 0) {
	for (ii=0; ii<8; ii++) hmy_act_sw[ii] = 0.0;
}



//End of subsystem   HMY_ACT **************************************************


// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_H1 = bsc_act_sw[4];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_H2 = bsc_act_sw[5];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_H3 = bsc_act_sw[6];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_H4 = bsc_act_sw[7];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_V1 = bsc_act_sw[0];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_V2 = bsc_act_sw[1];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_V3 = bsc_act_sw[2];

// EpicsOut
pLocalEpics->sei.BSC_DAC_OUTPUT_V4 = bsc_act_sw[3];

// DELAY
delay1 = bsc_tiltcorr_sw[0];

// DELAY
delay10 = hmx_tiltcorr_sw[1];

// DELAY
delay11 = hmx_tiltcorr_sw[2];

// DELAY
delay12 = hmx_tiltcorr_sw[3];

// DELAY
delay13 = hmx_tiltcorr_sw[4];

// DELAY
delay14 = hmx_tiltcorr_sw[5];

// DELAY
delay15 = hmx_tiltcorr_sw[6];

// DELAY
delay16 = hmx_tiltcorr_sw[7];

// DELAY
delay17 = hmy_tiltcorr_sw[1];

// DELAY
delay18 = hmy_tiltcorr_sw[2];

// DELAY
delay19 = hmy_tiltcorr_sw[3];

// DELAY
delay2 = bsc_tiltcorr_sw[1];

// DELAY
delay20 = hmy_tiltcorr_sw[4];

// DELAY
delay21 = hmy_tiltcorr_sw[5];

// DELAY
delay22 = hmy_tiltcorr_sw[6];

// DELAY
delay23 = hmy_tiltcorr_sw[7];

// DELAY
delay24 = hmy_tiltcorr_sw[0];

// DELAY
delay3 = bsc_tiltcorr_sw[2];

// DELAY
delay4 = bsc_tiltcorr_sw[3];

// DELAY
delay5 = bsc_tiltcorr_sw[4];

// DELAY
delay6 = bsc_tiltcorr_sw[5];

// DELAY
delay7 = bsc_tiltcorr_sw[6];

// DELAY
delay8 = bsc_tiltcorr_sw[7];

// DELAY
delay9 = hmx_tiltcorr_sw[0];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_H1 = hmx_act_sw[4];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_H2 = hmx_act_sw[5];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_H3 = hmx_act_sw[6];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_H4 = hmx_act_sw[7];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_V1 = hmx_act_sw[0];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_V2 = hmx_act_sw[1];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_V3 = hmx_act_sw[2];

// EpicsOut
pLocalEpics->sei.HMX_DAC_OUTPUT_V4 = hmx_act_sw[3];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_H1 = hmy_act_sw[4];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_H2 = hmy_act_sw[5];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_H3 = hmy_act_sw[6];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_H4 = hmy_act_sw[7];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_V1 = hmy_act_sw[0];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_V2 = hmy_act_sw[1];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_V3 = hmy_act_sw[2];

// EpicsOut
pLocalEpics->sei.HMY_DAC_OUTPUT_V4 = hmy_act_sw[3];

// DAC number is 0
dacOut[0][0] = pLocalEpics->sei.BSC_DAC_OUTPUT_H1;
dacOut[0][1] = pLocalEpics->sei.BSC_DAC_OUTPUT_V1;
dacOut[0][2] = pLocalEpics->sei.BSC_DAC_OUTPUT_H2;
dacOut[0][3] = pLocalEpics->sei.BSC_DAC_OUTPUT_V2;
dacOut[0][4] = pLocalEpics->sei.BSC_DAC_OUTPUT_H3;
dacOut[0][5] = pLocalEpics->sei.BSC_DAC_OUTPUT_V3;
dacOut[0][6] = pLocalEpics->sei.BSC_DAC_OUTPUT_H4;
dacOut[0][7] = pLocalEpics->sei.BSC_DAC_OUTPUT_V4;
dacOut[0][8] = pLocalEpics->sei.HMX_DAC_OUTPUT_H1;
dacOut[0][9] = pLocalEpics->sei.HMX_DAC_OUTPUT_V1;
dacOut[0][10] = pLocalEpics->sei.HMX_DAC_OUTPUT_H2;
dacOut[0][11] = pLocalEpics->sei.HMX_DAC_OUTPUT_V2;
dacOut[0][12] = pLocalEpics->sei.HMX_DAC_OUTPUT_H3;
dacOut[0][13] = pLocalEpics->sei.HMX_DAC_OUTPUT_V3;
dacOut[0][14] = pLocalEpics->sei.HMX_DAC_OUTPUT_H4;
dacOut[0][15] = pLocalEpics->sei.HMX_DAC_OUTPUT_V4;

// DAC number is 1
dacOut[1][0] = pLocalEpics->sei.HMY_DAC_OUTPUT_H1;
dacOut[1][1] = pLocalEpics->sei.HMY_DAC_OUTPUT_V1;
dacOut[1][2] = pLocalEpics->sei.HMY_DAC_OUTPUT_H2;
dacOut[1][3] = pLocalEpics->sei.HMY_DAC_OUTPUT_V2;
dacOut[1][4] = pLocalEpics->sei.HMY_DAC_OUTPUT_H3;
dacOut[1][5] = pLocalEpics->sei.HMY_DAC_OUTPUT_V3;
dacOut[1][6] = pLocalEpics->sei.HMY_DAC_OUTPUT_H4;
dacOut[1][7] = pLocalEpics->sei.HMY_DAC_OUTPUT_V4;

  }
}

