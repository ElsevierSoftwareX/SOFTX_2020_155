// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


#ifdef SERVO64K
	#define FE_RATE	65536
#endif
#ifdef SERVO32K
	#define FE_RATE	32768
#endif
#ifdef SERVO16K
	#define FE_RATE	16382
#endif
#ifdef SERVO2K
	#define FE_RATE	2048
#endif


/* Hardware configuration */
CDS_CARDS cards_used[] = {
	{GSC_16AI64SSA,1                 },
	{GSC_16AO16,0},
};

void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		double dacOut[][16],	/* DAC outputs */
		FILT_MOD *dsp_ptr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii;

double ipc_at_0x2000;
double ipc_at_0x2008;
double ipc_at_0x2010;
double ipc_at_0x2018;
double ipc_at_0x2020;
double ipc_at_0x2028 = *((double *)(((void *)_ipc_shm) + 0x2028));
double ipc_at_0x2030 = *((double *)(((void *)_ipc_shm) + 0x2030));
double ipc_at_0x2038 = *((double *)(((void *)_ipc_shm) + 0x2038));
double ipc_at_0x2040 = *((double *)(((void *)_ipc_shm) + 0x2040));
double ipc_at_0x2048 = *((double *)(((void *)_ipc_shm) + 0x2048));
double ipc_at_0x2050 = *((double *)(((void *)_ipc_shm) + 0x2050));
double ipc_at_0x2058 = *((double *)(((void *)_ipc_shm) + 0x2058));
double ipc_at_0x2060 = *((double *)(((void *)_ipc_shm) + 0x2060));
double ipc_at_0x2070;
double ipc_at_0x2078;
double ipc_at_0x2080;
double ipc_at_0x2088;
double ipc_at_0x20A0;
double ipc_at_0x20A8;
double ipc_at_0x20B0;
double ipc_at_0x20B8;
double ipc_at_0x20C0;
double ipc_at_0x20C8;
double ipc_at_0x20D0;
double ipc_at_0x20D8;
double ipc_at_0x20E0 = *((double *)(((void *)_ipc_shm) + 0x20E0));
double ipc_at_0x20E8 = *((double *)(((void *)_ipc_shm) + 0x20E8));
double ipc_at_0x20F0 = *((double *)(((void *)_ipc_shm) + 0x20F0));
double ipc_at_0x20F8 = *((double *)(((void *)_ipc_shm) + 0x20F8));
double ipc_at_0x2100 = *((double *)(((void *)_ipc_shm) + 0x2100));
double ipc_at_0x2108 = *((double *)(((void *)_ipc_shm) + 0x2108));
double ipc_at_0x2110 = *((double *)(((void *)_ipc_shm) + 0x2110));
double ipc_at_0x2118 = *((double *)(((void *)_ipc_shm) + 0x2118));
double ipc_at_0x2128 = *((double *)(((void *)_ipc_shm) + 0x2128));
double ipc_at_0x2130 = *((double *)(((void *)_ipc_shm) + 0x2130));
double ipc_at_0x2138 = *((double *)(((void *)_ipc_shm) + 0x2138));
static float ground;
double omc_qpd4_y;
double omc_qpd4_sum;
double omc_qpd4_seg4;
double omc_qpd4_seg3;
double omc_qpd4_seg2;
double omc_qpd4_seg1;
double omc_qpd4_p;
double omc_qpd4_inmtrx[3][4];
double omc_qpd4_divide1;
double omc_qpd4_divide;
double omc_qpd3_y;
double omc_qpd3_sum;
double omc_qpd3_seg4;
double omc_qpd3_seg3;
double omc_qpd3_seg2;
double omc_qpd3_seg1;
double omc_qpd3_p;
double omc_qpd3_inmtrx[3][4];
double omc_qpd3_divide1;
double omc_qpd3_divide;
double omc_pzt_vmon_dc;
double omc_pzt_vmon_ac;
double omc_pzt_sum;
float omc_pzt_pzt_dcrms;
static float omc_pzt_pzt_dcrms_avg;
float omc_pzt_pzt_acrms;
static float omc_pzt_pzt_acrms_avg;
double omc_pzt_lsc;
static float omc_pzt_ground;
double omc_pzt_dither;
double omc_pd_trans2;
double omc_pd_trans1;
double omc_pd_sum;
double omc_pd_saturation;
double omc_pd_shutter;
double omc_pd_product;
double omc_pd_norm_filt;
double omc_pd_inmtrx[3][3];
static float omc_pd_ground;
double omc_pd_divide;
static double omc_pd_constant1;
static double omc_pd_constant;
static double omc_pd_ctrl_constant0;
static double omc_pd_ctrl_constant128;
static double omc_pd_ctrl_constant2;
static double omc_pd_ctrl_constant4;
static double omc_pd_ctrl_constant512;
double omc_pd_ctrl_operator1;
double omc_pd_ctrl_operator2;
double omc_pd_ctrl_operator3;
double omc_pd_ctrl_product;
double omc_pd_ctrl_product1;
unsigned int omc_pd_ctrl_and;
unsigned int omc_pd_ctrl_and1;
unsigned int omc_pd_ctrl_or1;
unsigned int omc_pd_ctrl_or5;
double omc_mon_operator8;
double omc_mon_operator7;
double omc_mon_operator6;
double omc_mon_operator5;
double omc_mon_operator3;
double omc_mon_operator2;
double omc_mon_operator1;
double omc_mon_operator;
int omc_mon_logicaloperator6;
int omc_mon_logicaloperator5;
int omc_mon_logicaloperator4;
int omc_mon_logicaloperator3;
int omc_mon_logicaloperator2;
int omc_mon_logicaloperator1;
int omc_mon_logicaloperator;
static float omc_mon_ground7;
static float omc_mon_ground6;
static float omc_mon_ground5;
static float omc_mon_ground4;
static float omc_mon_ground3;
static float omc_mon_ground2;
static float omc_mon_ground1;
static float omc_mon_ground;
double omc_mon_divide2;
double omc_mon_divide1;
double omc_mon_divide;
static double omc_mon_tobinary_constant;
static double omc_mon_tobinary_constant1;
static double omc_mon_tobinary_constant2;
static double omc_mon_tobinary_constant3;
static double omc_mon_tobinary_constant4;
static double omc_mon_tobinary_constant5;
static double omc_mon_tobinary_constant6;
static double omc_mon_tobinary_constant7;
double omc_mon_tobinary_product;
double omc_mon_tobinary_product1;
double omc_mon_tobinary_product2;
double omc_mon_tobinary_product3;
double omc_mon_tobinary_product4;
double omc_mon_tobinary_product5;
double omc_mon_tobinary_product6;
double omc_mon_tobinary_product7;
unsigned int omc_mon_tobinary_or1;
unsigned int omc_mon_tobinary_or2;
unsigned int omc_mon_tobinary_or3;
unsigned int omc_mon_tobinary_or4;
unsigned int omc_mon_tobinary_or5;
unsigned int omc_mon_tobinary_or6;
unsigned int omc_mon_tobinary_or7;
unsigned int omc_mon_tobinary_or8;
double omc_lsc_x_sin;
double omc_lsc_x_cos;
double omc_lsc_sig;
double omc_lsc_q;
double omc_lsc_product2;
double omc_lsc_product1;
double omc_lsc_product;
static double omc_lsc_phase[2];
double omc_lsc_output;
static double omc_lsc_osc[3];
static double omc_lsc_osc_freq;
static double omc_lsc_osc_delta;
static double omc_lsc_osc_alpha;
static double omc_lsc_osc_beta;
static double omc_lsc_osc_cos_prev;
static double omc_lsc_osc_sin_prev;
static double omc_lsc_osc_cos_new;
static double omc_lsc_osc_sin_new;
double lsinx, lcosx, valx;
double omc_lsc_i;
static float omc_lsc_ground1;
double omc_lsc_gain;
double omc_lsc_clock;
double omc_htr_v_mon;
double omc_htr_t_mon;
double omc_htr_product3;
double omc_htr_product2;
double omc_htr_product1;
double omc_htr_lsc;
double omc_htr_i_mon;
float omc_htr_htr_rms;
static float omc_htr_htr_rms_avg;
static float omc_htr_ground3;
static float omc_htr_ground2;
static float omc_htr_ground1;
double omc_htr_drv;
double omc_dcerr_tp;
#include "OMC_DCERR_OMC_DCERR.c"
double omc_dcerr_mux[10];
double omc_dcerr_demux[3];
double omc_dcerr_choice;
double omc_dac0_test4;
double omc_dac0_test3;
double omc_dac0_test2;
double omc_dac0_test10;
double omc_dac0_test1;
static float omc_dac0_ground8;
static float omc_dac0_ground7;
static float omc_dac0_ground6;
static float omc_dac0_ground5;
static float omc_dac0_ground;
double omc_asc_y2_x_sin;
double omc_asc_y2_x_cos;
double omc_asc_y2_sig;
double omc_asc_y2_q;
static double omc_asc_y2_phase[2];
double omc_asc_y2_i;
double omc_asc_y2_clock;
double omc_asc_y1_x_sin;
double omc_asc_y1_x_cos;
double omc_asc_y1_sig;
double omc_asc_y1_q;
static double omc_asc_y1_phase[2];
double omc_asc_y1_i;
double omc_asc_y1_clock;
double omc_asc_waist_yaw;
double omc_asc_waist_y;
double omc_asc_waist_x;
double omc_asc_waist_pit;
double omc_asc_waistbasis[4];
double omc_asc_sum9;
double omc_asc_sum8;
double omc_asc_sum7;
double omc_asc_sum6;
double omc_asc_sum5;
double omc_asc_sum4;
double omc_asc_sum3;
double omc_asc_sum2;
double omc_asc_sum11;
double omc_asc_sum10;
double omc_asc_sum1;
double omc_asc_sum;
double omc_asc_qpos_yb;
double omc_asc_qpos_y;
double omc_asc_qpos_xb;
double omc_asc_qpos_x;
double omc_asc_qpdgain[9];
float OMC_ASC_QPDGAIN_CALC;
double omc_asc_qpdasc[4][4];
double omc_asc_qang_yb;
double omc_asc_qang_y;
double omc_asc_qang_xb;
double omc_asc_qang_x;
double omc_asc_product9;
double omc_asc_product8;
double omc_asc_product7;
double omc_asc_product6;
double omc_asc_product5;
double omc_asc_product4;
double omc_asc_product3;
double omc_asc_product2;
double omc_asc_product11;
double omc_asc_product10;
double omc_asc_product1;
double omc_asc_product;
double omc_asc_pos_y;
double omc_asc_pos_x;
double omc_asc_p2_x_sin;
double omc_asc_p2_x_cos;
double omc_asc_p2_sig;
double omc_asc_p2_q;
static double omc_asc_p2_phase[2];
double omc_asc_p2_i;
double omc_asc_p2_clock;
double omc_asc_p1_x_sin;
double omc_asc_p1_x_cos;
double omc_asc_p1_sig;
double omc_asc_p1_q;
static double omc_asc_p1_phase[2];
double omc_asc_p1_i;
double omc_asc_p1_clock;
double omc_asc_operator;
double omc_asc_mux[4];
double omc_asc_mastergain[9];
float OMC_ASC_MASTERGAIN_CALC;
double omc_asc_exmtx[4][8];
double omc_asc_demux[4];
double omc_asc_dpos_yb;
double omc_asc_dpos_y;
double omc_asc_dpos_xb;
double omc_asc_dpos_x;
double omc_asc_dithergain[9];
float OMC_ASC_DITHERGAIN_CALC;
double omc_asc_dang_yb;
double omc_asc_dang_y;
double omc_asc_dang_xb;
double omc_asc_dang_x;
static double omc_asc_constant;
double omc_asc_blendgain[9];
float OMC_ASC_BLENDGAIN_CALC;
double omc_asc_ang_y;
double omc_asc_ang_x;
double omc_asc_acmtx[4][4];
double omc_asc_enable_product1;
double omc_asc_enable_product11;
double omc_asc_enable_product2;
double omc_asc_enable_product3;
double omc_asc_enable1_product1;
double omc_asc_enable1_product11;
double omc_asc_enable1_product2;
double omc_asc_enable1_product3;
static double omc_constant3;
double omc_nptr;
double omc_nullstream;
double omc_product;
double omc_readout;


if(feInit)
{
// ADC 0
dWordUsed[0][0] =  1;
dWordUsed[0][1] =  1;
dWordUsed[0][2] =  1;
dWordUsed[0][3] =  1;
dWordUsed[0][4] =  1;
dWordUsed[0][5] =  1;
dWordUsed[0][6] =  1;
dWordUsed[0][7] =  1;
dWordUsed[0][8] =  1;
dWordUsed[0][9] =  1;
dWordUsed[0][10] =  1;
dWordUsed[0][11] =  1;
dWordUsed[0][12] =  1;
dWordUsed[0][13] =  1;
dWordUsed[0][14] =  1;
dWordUsed[0][15] =  1;
dWordUsed[0][16] =  1;
dWordUsed[0][17] =  1;
dWordUsed[0][18] =  1;
dWordUsed[0][20] =  1;
dWordUsed[0][21] =  1;
dWordUsed[0][24] =  1;
dWordUsed[0][25] =  1;
dWordUsed[0][28] =  1;
dWordUsed[0][29] =  1;
dWordUsed[0][30] =  1;
// DAC number is 0
dacOutUsed[0][0] =  1;
dacOutUsed[0][1] =  1;
dacOutUsed[0][2] =  1;
dacOutUsed[0][3] =  1;
dacOutUsed[0][4] =  1;
dacOutUsed[0][5] =  1;
dacOutUsed[0][6] =  1;
dacOutUsed[0][7] =  1;
dacOutUsed[0][8] =  1;
dacOutUsed[0][9] =  1;
dacOutUsed[0][10] =  1;
dacOutUsed[0][11] =  1;
dacOutUsed[0][12] =  1;
dacOutUsed[0][13] =  1;
dacOutUsed[0][14] =  1;
dacOutUsed[0][15] =  1;
ground = 0.0;
omc_pzt_pzt_dcrms_avg = 0.0;
omc_pzt_pzt_acrms_avg = 0.0;
omc_pzt_ground = 0.0;
omc_pd_ground = 0.0;
omc_pd_constant1 = (double)100;
omc_pd_constant = (double)300;
omc_pd_ctrl_constant0 = (double)0;
omc_pd_ctrl_constant128 = (double)128;
omc_pd_ctrl_constant2 = (double)2;
omc_pd_ctrl_constant4 = (double)4;
omc_pd_ctrl_constant512 = (double)512;
omc_mon_ground7 = 0.0;
omc_mon_ground6 = 0.0;
omc_mon_ground5 = 0.0;
omc_mon_ground4 = 0.0;
omc_mon_ground3 = 0.0;
omc_mon_ground2 = 0.0;
omc_mon_ground1 = 0.0;
omc_mon_ground = 0.0;
omc_mon_tobinary_constant = (double)2;
omc_mon_tobinary_constant1 = (double)4;
omc_mon_tobinary_constant2 = (double)8;
omc_mon_tobinary_constant3 = (double)16;
omc_mon_tobinary_constant4 = (double)32;
omc_mon_tobinary_constant5 = (double)64;
omc_mon_tobinary_constant6 = (double)128;
omc_mon_tobinary_constant7 = (double)256;
omc_lsc_osc_freq = pLocalEpics->om1.OMC_LSC_OSC_FREQ;
omc_lsc_osc_delta = 2.0 * 3.1415926535897932384626 * omc_lsc_osc_freq / FE_RATE;
valx = omc_lsc_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_lsc_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_lsc_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_lsc_osc_beta = lsinx;
omc_lsc_osc_cos_prev = 1.0;
omc_lsc_osc_sin_prev = 0.0;
omc_lsc_ground1 = 0.0;
omc_htr_htr_rms_avg = 0.0;
omc_htr_ground3 = 0.0;
omc_htr_ground2 = 0.0;
omc_htr_ground1 = 0.0;
omc_dac0_ground8 = 0.0;
omc_dac0_ground7 = 0.0;
omc_dac0_ground6 = 0.0;
omc_dac0_ground5 = 0.0;
omc_dac0_ground = 0.0;
omc_asc_constant = (double)0;
omc_constant3 = (double)1;
} else {
ipc_at_0x2020 = dWord[0][28];

 
 
 
 
 
 
 
 
ipc_at_0x2078 = dWord[0][29];

ipc_at_0x2080 = dWord[0][30];

ipc_at_0x20A0 = dWord[0][0];

ipc_at_0x20A8 = dWord[0][1];

ipc_at_0x20B0 = dWord[0][2];

ipc_at_0x20B8 = dWord[0][3];

ipc_at_0x20C0 = dWord[0][4];

ipc_at_0x20C8 = dWord[0][5];

ipc_at_0x20D0 = dWord[0][6];

ipc_at_0x20D8 = dWord[0][7];

 
 
 
 
 
 
 
 
 
 
 




//Start of subsystem OMC **************************************************

// MULTIPLY
omc_pd_product = pLocalEpics->om1.OMC_PD_ZSWITCH * omc_pd_constant;

// Relational Operator
omc_pd_ctrl_operator3 = ((omc_pd_ctrl_constant0) != (pLocalEpics->om1.OMC_PD_ZSWITCH));
// MULTIPLY
omc_mon_tobinary_product4 = omc_mon_ground5 * omc_mon_tobinary_constant4;

// MULTIPLY
omc_mon_tobinary_product5 = omc_mon_ground5 * omc_mon_tobinary_constant5;

// MULTIPLY
omc_mon_tobinary_product6 = omc_mon_ground5 * omc_mon_tobinary_constant6;

// MULTIPLY
omc_mon_tobinary_product7 = omc_mon_ground5 * omc_mon_tobinary_constant7;

// Osc
omc_lsc_osc_cos_new = (1.0 - omc_lsc_osc_alpha) * omc_lsc_osc_cos_prev - omc_lsc_osc_beta * omc_lsc_osc_sin_prev;
omc_lsc_osc_sin_new = (1.0 - omc_lsc_osc_alpha) * omc_lsc_osc_sin_prev + omc_lsc_osc_beta * omc_lsc_osc_cos_prev;
omc_lsc_osc_sin_prev = omc_lsc_osc_sin_new;
omc_lsc_osc_cos_prev = omc_lsc_osc_cos_new;
omc_lsc_osc[0] = omc_lsc_osc_sin_new * pLocalEpics->om1.OMC_LSC_OSC_CLKGAIN;
omc_lsc_osc[1] = omc_lsc_osc_sin_new * pLocalEpics->om1.OMC_LSC_OSC_SINGAIN;
omc_lsc_osc[2] = omc_lsc_osc_cos_new * pLocalEpics->om1.OMC_LSC_OSC_COSGAIN;
if((omc_lsc_osc_freq != pLocalEpics->om1.OMC_LSC_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_lsc_osc_freq = pLocalEpics->om1.OMC_LSC_OSC_FREQ;
	omc_lsc_osc_delta = 2.0 * 3.1415926535897932384626 * omc_lsc_osc_freq / FE_RATE;
	valx = omc_lsc_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_lsc_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_lsc_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_lsc_osc_beta = lsinx;
	omc_lsc_osc_cos_prev = 1.0;
	omc_lsc_osc_sin_prev = 0.0;
}

// FILTER MODULE
omc_dac0_test4 = filterModuleD(dsp_ptr,dspCoeff,OMC_DAC0_TEST4,omc_dac0_ground8,0);

// FILTER MODULE
omc_dac0_test3 = filterModuleD(dsp_ptr,dspCoeff,OMC_DAC0_TEST3,omc_dac0_ground7,0);

// FILTER MODULE
omc_dac0_test2 = filterModuleD(dsp_ptr,dspCoeff,OMC_DAC0_TEST2,omc_dac0_ground6,0);

// FILTER MODULE
omc_dac0_test1 = filterModuleD(dsp_ptr,dspCoeff,OMC_DAC0_TEST1,omc_dac0_ground5,0);

// FILTER MODULE
omc_dac0_test10 = filterModuleD(dsp_ptr,dspCoeff,OMC_DAC0_TEST10,omc_dac0_ground,0);

// FILTER MODULE
omc_qpd3_seg1 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_SEG1,dWord[0][8],0);

// FILTER MODULE
omc_qpd3_seg2 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_SEG2,dWord[0][9],0);

// FILTER MODULE
omc_qpd3_seg3 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_SEG3,dWord[0][10],0);

// FILTER MODULE
omc_qpd3_seg4 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_SEG4,dWord[0][11],0);

// FILTER MODULE
omc_qpd4_seg1 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_SEG1,dWord[0][12],0);

// FILTER MODULE
omc_qpd4_seg2 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_SEG2,dWord[0][13],0);

// FILTER MODULE
omc_qpd4_seg3 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_SEG3,dWord[0][14],0);

// FILTER MODULE
omc_qpd4_seg4 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_SEG4,dWord[0][15],0);

// FILTER MODULE
omc_pzt_vmon_dc = filterModuleD(dsp_ptr,dspCoeff,OMC_PZT_VMON_DC,dWord[0][20],0);

// FILTER MODULE
omc_pzt_vmon_ac = filterModuleD(dsp_ptr,dspCoeff,OMC_PZT_VMON_AC,dWord[0][21],0);

// MULTIPLY
omc_htr_product1 = dWord[0][16] * pLocalEpics->om1.OMC_HTR_V_MON_CAL;

// MULTIPLY
omc_htr_product2 = dWord[0][17] * pLocalEpics->om1.OMC_HTR_I_MON_CAL;

// MULTIPLY
omc_htr_product3 = dWord[0][18] * pLocalEpics->om1.OMC_HTR_T_MON_CAL;

// FILTER MODULE
omc_pd_trans1 = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_TRANS1,dWord[0][24],pLocalEpics->om1.OMC_HTR_T_MON_CAL);

// FILTER MODULE
omc_pd_trans2 = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_TRANS2,dWord[0][25],pLocalEpics->om1.OMC_HTR_T_MON_CAL);

// FILTER MODULE
omc_pd_shutter = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_SHUTTER,dWord[0][28],0);

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_exmtx[1][ii] = 
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][0] * ipc_at_0x20E0 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][1] * ipc_at_0x20E8 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][2] * ipc_at_0x20F0 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][3] * ipc_at_0x20F8 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][4] * ipc_at_0x2100 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][5] * ipc_at_0x2108 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][6] * ipc_at_0x2110 +
	pLocalEpics->om1.OMC_ASC_EXMTX[ii][7] * ipc_at_0x2118;
}

// Relational Operator
omc_mon_operator6 = ((ipc_at_0x2128) >= (pLocalEpics->om1.OMC_MON_LINE_TH1));
// Relational Operator
omc_mon_operator7 = ((ipc_at_0x2130) >= (pLocalEpics->om1.OMC_MON_LINE_TH2));
// Relational Operator
omc_mon_operator8 = ((ipc_at_0x2138) >= (pLocalEpics->om1.OMC_MON_LINE_TH3));
// FILTER MODULE
omc_nptr = filterModuleD(dsp_ptr,dspCoeff,OMC_NPTR,cdsPciModules.pci_rfm[0]? *((double *)(((void *)cdsPciModules.pci_rfm[0]) + 0x12200d4)) : 0.0,0);

// Relational Operator
omc_asc_operator = ((omc_constant3) > (omc_asc_constant));
// SUM
omc_pd_sum = omc_pd_product + omc_pd_constant1;

// MULTIPLY
omc_pd_ctrl_product1 = omc_pd_ctrl_operator3 * omc_pd_ctrl_constant4;

// FILTER MODULE
omc_lsc_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_CLOCK,omc_lsc_osc[0],0);

// Matrix
for(ii=0;ii<3;ii++)
{
omc_qpd3_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_QPD3_INMTRX[ii][0] * omc_qpd3_seg1 +
	pLocalEpics->om1.OMC_QPD3_INMTRX[ii][1] * omc_qpd3_seg2 +
	pLocalEpics->om1.OMC_QPD3_INMTRX[ii][2] * omc_qpd3_seg3 +
	pLocalEpics->om1.OMC_QPD3_INMTRX[ii][3] * omc_qpd3_seg4;
}

// Matrix
for(ii=0;ii<3;ii++)
{
omc_qpd4_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_QPD4_INMTRX[ii][0] * omc_qpd4_seg1 +
	pLocalEpics->om1.OMC_QPD4_INMTRX[ii][1] * omc_qpd4_seg2 +
	pLocalEpics->om1.OMC_QPD4_INMTRX[ii][2] * omc_qpd4_seg3 +
	pLocalEpics->om1.OMC_QPD4_INMTRX[ii][3] * omc_qpd4_seg4;
}

// RMS
omc_pzt_pzt_dcrms = omc_pzt_vmon_dc;
if(omc_pzt_pzt_dcrms > 2000) omc_pzt_pzt_dcrms = 2000;
if(omc_pzt_pzt_dcrms < -2000) omc_pzt_pzt_dcrms = -2000;
omc_pzt_pzt_dcrms = omc_pzt_pzt_dcrms * omc_pzt_pzt_dcrms;
omc_pzt_pzt_dcrms_avg = omc_pzt_pzt_dcrms * .00005 + omc_pzt_pzt_dcrms_avg * 0.99995;
omc_pzt_pzt_dcrms = lsqrt(omc_pzt_pzt_dcrms_avg);

// RMS
omc_pzt_pzt_acrms = omc_pzt_vmon_ac;
if(omc_pzt_pzt_acrms > 2000) omc_pzt_pzt_acrms = 2000;
if(omc_pzt_pzt_acrms < -2000) omc_pzt_pzt_acrms = -2000;
omc_pzt_pzt_acrms = omc_pzt_pzt_acrms * omc_pzt_pzt_acrms;
omc_pzt_pzt_acrms_avg = omc_pzt_pzt_acrms * .00005 + omc_pzt_pzt_acrms_avg * 0.99995;
omc_pzt_pzt_acrms = lsqrt(omc_pzt_pzt_acrms_avg);

// FILTER MODULE
omc_htr_v_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_V_MON,omc_htr_product1,0);

// FILTER MODULE
omc_htr_i_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_I_MON,omc_htr_product2,0);

// FILTER MODULE
omc_htr_t_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_T_MON,omc_htr_product3,0);

// Bitwise &
omc_pd_ctrl_and = ((unsigned int)(dsp_ptr->inputs[OMC_PD_TRANS1].opSwitchP))&((unsigned int)(omc_pd_ctrl_constant128));

// Bitwise &
omc_pd_ctrl_and1 = ((unsigned int)(dsp_ptr->inputs[OMC_PD_TRANS1].opSwitchP))&((unsigned int)(omc_pd_ctrl_constant512));

// Matrix
for(ii=0;ii<3;ii++)
{
omc_pd_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_PD_INMTRX[ii][0] * omc_pd_trans1 +
	pLocalEpics->om1.OMC_PD_INMTRX[ii][1] * omc_pd_trans2 +
	pLocalEpics->om1.OMC_PD_INMTRX[ii][2] * omc_pd_shutter;
}

// Relational Operator
omc_mon_operator = ((omc_pd_shutter) >= (pLocalEpics->om1.OMC_MON_LASER_TH));
// FILTER MODULE
omc_asc_p1_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_CLOCK,omc_asc_exmtx[1][0],0);

// FILTER MODULE
omc_asc_y1_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_CLOCK,omc_asc_exmtx[1][1],0);

// FILTER MODULE
omc_asc_p2_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_CLOCK,omc_asc_exmtx[1][2],0);

// FILTER MODULE
omc_asc_y2_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_CLOCK,omc_asc_exmtx[1][3],0);

// Logical AND
omc_mon_logicaloperator5 = omc_mon_operator6 && omc_mon_operator7;

// EpicsOut
pLocalEpics->om1.OMC_DCERR_PARM = omc_nptr;

// EpicsOut
pLocalEpics->om1.OMC_PD_ZSWITCH_RDBK = omc_pd_sum;

// FILTER MODULE
omc_pzt_dither = filterModuleD(dsp_ptr,dspCoeff,OMC_PZT_DITHER,omc_lsc_clock,0);

// FILTER MODULE
omc_qpd3_sum = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_SUM,omc_qpd3_inmtrx[1][0],0);

// FILTER MODULE
omc_qpd4_sum = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_SUM,omc_qpd4_inmtrx[1][0],0);

// EpicsOut
pLocalEpics->om1.OMC_PZT_DC_RMS = omc_pzt_pzt_dcrms;

// EpicsOut
pLocalEpics->om1.OMC_PZT_AC_RMS = omc_pzt_pzt_acrms;

// RMS
omc_htr_htr_rms = omc_htr_i_mon;
if(omc_htr_htr_rms > 2000) omc_htr_htr_rms = 2000;
if(omc_htr_htr_rms < -2000) omc_htr_htr_rms = -2000;
omc_htr_htr_rms = omc_htr_htr_rms * omc_htr_htr_rms;
omc_htr_htr_rms_avg = omc_htr_htr_rms * .00005 + omc_htr_htr_rms_avg * 0.99995;
omc_htr_htr_rms = lsqrt(omc_htr_htr_rms_avg);

// Relational Operator
omc_pd_ctrl_operator1 = ((omc_pd_ctrl_and) != (omc_pd_ctrl_constant0));
// Relational Operator
omc_pd_ctrl_operator2 = ((omc_pd_ctrl_and1) != (omc_pd_ctrl_constant0));
// FILTER MODULE
omc_nullstream = filterModuleD(dsp_ptr,dspCoeff,OMC_NULLSTREAM,omc_pd_inmtrx[1][2],0);

// FILTER MODULE
omc_readout = filterModuleD(dsp_ptr,dspCoeff,OMC_READOUT,omc_pd_inmtrx[1][0],0);

// DIVIDE
if(omc_pd_shutter != 0.0)
{
	omc_mon_divide2 = omc_pd_inmtrx[1][0] / omc_pd_shutter;
}
else{
	omc_mon_divide2 = 0.0;
}

// FILTER MODULE
omc_pd_norm_filt = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_NORM_FILT,omc_pd_inmtrx[1][1],0);

// Logical AND
omc_mon_logicaloperator6 = omc_mon_logicaloperator5 && omc_mon_operator8;

// DIVIDE
if(omc_pd_shutter != 0.0)
{
	omc_mon_divide = omc_qpd3_sum / omc_pd_shutter;
}
else{
	omc_mon_divide = 0.0;
}

// DIVIDE
if(omc_qpd3_sum != 0.0)
{
	omc_qpd3_divide = omc_qpd3_inmtrx[1][1] / omc_qpd3_sum;
}
else{
	omc_qpd3_divide = 0.0;
}

// DIVIDE
if(omc_qpd3_sum != 0.0)
{
	omc_qpd3_divide1 = omc_qpd3_inmtrx[1][2] / omc_qpd3_sum;
}
else{
	omc_qpd3_divide1 = 0.0;
}

// EpicsOut
pLocalEpics->om1.OMC_QPD3_SUM_MON = omc_qpd3_sum;

// DIVIDE
if(omc_pd_shutter != 0.0)
{
	omc_mon_divide1 = omc_qpd4_sum / omc_pd_shutter;
}
else{
	omc_mon_divide1 = 0.0;
}

// DIVIDE
if(omc_qpd4_sum != 0.0)
{
	omc_qpd4_divide = omc_qpd4_inmtrx[1][1] / omc_qpd4_sum;
}
else{
	omc_qpd4_divide = 0.0;
}

// DIVIDE
if(omc_qpd4_sum != 0.0)
{
	omc_qpd4_divide1 = omc_qpd4_inmtrx[1][2] / omc_qpd4_sum;
}
else{
	omc_qpd4_divide1 = 0.0;
}

// EpicsOut
pLocalEpics->om1.OMC_QPD4_SUM_MON = omc_qpd4_sum;

// EpicsOut
pLocalEpics->om1.OMC_HTR_I_RMS = omc_htr_htr_rms;

// MULTIPLY
omc_pd_ctrl_product = omc_pd_ctrl_operator1 * omc_pd_ctrl_constant2;

// EpicsOut
pLocalEpics->om1.OMC_DCERR_PAS = omc_readout;

// MUX
omc_dcerr_mux[0]= omc_readout;
omc_dcerr_mux[1]= omc_nptr;
omc_dcerr_mux[2]= pLocalEpics->om1.OMC_DCERR_Pref;
omc_dcerr_mux[3]= pLocalEpics->om1.OMC_DCERR_Pdefect;
omc_dcerr_mux[4]= pLocalEpics->om1.OMC_DCERR_x0;
omc_dcerr_mux[5]= pLocalEpics->om1.OMC_DCERR_xf;
omc_dcerr_mux[6]= pLocalEpics->om1.OMC_DCERR_C2;
omc_dcerr_mux[7]= pLocalEpics->om1.OMC_DCERR_TRAMP;
omc_dcerr_mux[8]= pLocalEpics->om1.OMC_DCERR_LOAD;
omc_dcerr_mux[9]= pLocalEpics->om1.OMC_DCERR_TPSELECT;

// EpicsOut
pLocalEpics->om1.OMC_MON_LOCK_NORM = omc_mon_divide2;

// Relational Operator
omc_mon_operator3 = ((omc_mon_divide2) >= (pLocalEpics->om1.OMC_MON_LOCK_TH));
// Relational Operator
omc_mon_operator5 = ((omc_mon_divide2) >= (pLocalEpics->om1.OMC_MON_HTR_TH));
// SATURATE
omc_pd_saturation = omc_pd_norm_filt;
if (omc_pd_saturation > 100000) {
  omc_pd_saturation  = 100000;
} else if (omc_pd_saturation < 0.1) {
  omc_pd_saturation  = 0.1;
};

// Relational Operator
omc_mon_operator2 = ((omc_mon_divide) >= (pLocalEpics->om1.OMC_MON_ALIGN_TH));
// EpicsOut
pLocalEpics->om1.OMC_MON_QPD3_NORM = omc_mon_divide;

// FILTER MODULE
omc_qpd3_p = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_P,omc_qpd3_divide,0);

// FILTER MODULE
omc_qpd3_y = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_Y,omc_qpd3_divide1,0);

// Relational Operator
omc_mon_operator1 = ((omc_mon_divide1) >= (pLocalEpics->om1.OMC_MON_ALIGN_TH));
// EpicsOut
pLocalEpics->om1.OMC_MON_QPD4_NORM = omc_mon_divide1;

// FILTER MODULE
omc_qpd4_p = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_P,omc_qpd4_divide,0);

// FILTER MODULE
omc_qpd4_y = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_Y,omc_qpd4_divide1,0);

// Bitwise |
omc_pd_ctrl_or5 = ((unsigned int)(omc_pd_ctrl_operator2))|((unsigned int)(omc_pd_ctrl_product));

// Function Call
OMC_DCERR_OMC_DCERR(omc_dcerr_mux, 10, omc_dcerr_demux, 3);

// DIVIDE
if(omc_pd_saturation != 0.0)
{
	omc_pd_divide = omc_pd_inmtrx[1][1] / omc_pd_saturation;
}
else{
	omc_pd_divide = 0.0;
}

// Logical AND
omc_mon_logicaloperator = omc_mon_operator2 && omc_mon_operator1;

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_qpdasc[1][ii] = 
	pLocalEpics->om1.OMC_ASC_QPDASC[ii][0] * omc_qpd3_p +
	pLocalEpics->om1.OMC_ASC_QPDASC[ii][1] * omc_qpd3_y +
	pLocalEpics->om1.OMC_ASC_QPDASC[ii][2] * omc_qpd4_p +
	pLocalEpics->om1.OMC_ASC_QPDASC[ii][3] * omc_qpd4_y;
}

// MUX
omc_asc_mux[0]= omc_qpd3_p;
omc_asc_mux[1]= omc_qpd3_y;
omc_asc_mux[2]= omc_qpd4_p;
omc_asc_mux[3]= omc_qpd4_y;

// Bitwise |
omc_pd_ctrl_or1 = ((unsigned int)(omc_pd_ctrl_or5))|((unsigned int)(omc_pd_ctrl_product1));


// EpicsOut
pLocalEpics->om1.OMC_ASC_P1_MOUT = omc_pd_divide;

// EpicsOut
pLocalEpics->om1.OMC_ASC_Y2_MOUT = omc_pd_divide;

// EpicsOut
pLocalEpics->om1.OMC_ASC_P2_MOUT = omc_pd_divide;

// EpicsOut
pLocalEpics->om1.OMC_ASC_Y1_MOUT = omc_pd_divide;

// FILTER MODULE
omc_lsc_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_SIG,omc_pd_divide,0);

// Logical AND
omc_mon_logicaloperator1 = omc_mon_operator && omc_mon_logicaloperator;

// MuxMatrix
for(ii=0;ii<4;ii++)
{
omc_asc_waistbasis[ii] = 
	pLocalEpics->om1.OMC_ASC_WAISTBASIS[ii][0] * omc_asc_mux[0] +
	pLocalEpics->om1.OMC_ASC_WAISTBASIS[ii][1] * omc_asc_mux[1] +
	pLocalEpics->om1.OMC_ASC_WAISTBASIS[ii][2] * omc_asc_mux[2] +
	pLocalEpics->om1.OMC_ASC_WAISTBASIS[ii][3] * omc_asc_mux[3];
}

// EpicsOut
pLocalEpics->om1.OMC_PD_CTRL_WHITENING_CTRL_MON = omc_pd_ctrl_or1;

// Switch
omc_dcerr_choice = (((pLocalEpics->om1.OMC_DCERR_BYPASS) != 0)? (omc_readout): (omc_dcerr_demux[0]));
// EpicsOut
pLocalEpics->om1.OMC_DCERR_STATE = omc_dcerr_demux[1];

// FILTER MODULE
omc_dcerr_tp = filterModuleD(dsp_ptr,dspCoeff,OMC_DCERR_TP,omc_dcerr_demux[2],0);

// FILTER MODULE
omc_asc_p1_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_SIG,pLocalEpics->om1.OMC_ASC_P1_MOUT,0);

// FILTER MODULE
omc_asc_y2_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_SIG,pLocalEpics->om1.OMC_ASC_Y2_MOUT,0);

// FILTER MODULE
omc_asc_p2_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_SIG,pLocalEpics->om1.OMC_ASC_P2_MOUT,0);

// FILTER MODULE
omc_asc_y1_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_SIG,pLocalEpics->om1.OMC_ASC_Y1_MOUT,0);

// MULTIPLY
omc_lsc_product = omc_lsc_sig * omc_lsc_osc[1];

// MULTIPLY
omc_lsc_product1 = omc_lsc_sig * omc_lsc_osc[2];

// Logical AND
omc_mon_logicaloperator2 = omc_mon_logicaloperator1 && omc_mon_operator3;

// MULTIPLY
omc_asc_enable_product3 = omc_asc_qpdasc[1][3] * omc_mon_logicaloperator1;

// MULTIPLY
omc_asc_enable_product2 = omc_asc_qpdasc[1][2] * omc_mon_logicaloperator1;

// MULTIPLY
omc_asc_enable_product1 = omc_asc_qpdasc[1][1] * omc_mon_logicaloperator1;

// MULTIPLY
omc_asc_enable_product11 = omc_asc_qpdasc[1][0] * omc_mon_logicaloperator1;

// MULTIPLY
omc_mon_tobinary_product = omc_mon_logicaloperator1 * omc_mon_tobinary_constant;

// DEMUX
omc_asc_demux[0]= omc_asc_waistbasis[0];
omc_asc_demux[1]= omc_asc_waistbasis[1];
omc_asc_demux[2]= omc_asc_waistbasis[2];
omc_asc_demux[3]= omc_asc_waistbasis[3];

// EpicsOut
pLocalEpics->om1.OMC_DCERR_OUTMON = omc_dcerr_choice;

// MULTIPLY
omc_asc_product = omc_asc_p1_sig * ipc_at_0x20E0;

// MULTIPLY
omc_asc_product1 = omc_asc_p1_sig * ipc_at_0x20E8;

// MULTIPLY
omc_asc_product6 = omc_asc_y2_sig * ipc_at_0x2110;

// MULTIPLY
omc_asc_product7 = omc_asc_y2_sig * ipc_at_0x2118;

// MULTIPLY
omc_asc_product4 = omc_asc_p2_sig * ipc_at_0x2100;

// MULTIPLY
omc_asc_product5 = omc_asc_p2_sig * ipc_at_0x2108;

// MULTIPLY
omc_asc_product2 = omc_asc_y1_sig * ipc_at_0x20F0;

// MULTIPLY
omc_asc_product3 = omc_asc_y1_sig * ipc_at_0x20F8;

// FILTER MODULE
omc_lsc_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_X_SIN,omc_lsc_product,0);

// FILTER MODULE
omc_lsc_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_X_COS,omc_lsc_product1,0);

// Logical AND
omc_mon_logicaloperator3 = omc_mon_logicaloperator2 && omc_mon_operator5;

// MULTIPLY
omc_mon_tobinary_product1 = omc_mon_logicaloperator2 * omc_mon_tobinary_constant1;

// FILTER MODULE
omc_asc_qang_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QANG_X,omc_asc_enable_product3,0);

// FILTER MODULE
omc_asc_qang_xb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QANG_XB,omc_asc_enable_product3,0);

// FILTER MODULE
omc_asc_qang_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QANG_Y,omc_asc_enable_product2,0);

// FILTER MODULE
omc_asc_qang_yb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QANG_YB,omc_asc_enable_product2,0);

// FILTER MODULE
omc_asc_qpos_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPOS_X,omc_asc_enable_product1,0);

// FILTER MODULE
omc_asc_qpos_xb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPOS_XB,omc_asc_enable_product1,0);

// FILTER MODULE
omc_asc_qpos_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPOS_Y,omc_asc_enable_product11,0);

// FILTER MODULE
omc_asc_qpos_yb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPOS_YB,omc_asc_enable_product11,0);

// Bitwise |
omc_mon_tobinary_or5 = ((unsigned int)(omc_mon_operator))|((unsigned int)(omc_mon_tobinary_product));

// FILTER MODULE
omc_asc_waist_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_X,omc_asc_demux[0],0);

// FILTER MODULE
omc_asc_waist_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_Y,omc_asc_demux[1],0);

// FILTER MODULE
omc_asc_waist_pit = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_PIT,omc_asc_demux[2],0);

// FILTER MODULE
omc_asc_waist_yaw = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_YAW,omc_asc_demux[3],0);

// FILTER MODULE
omc_asc_p1_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_X_SIN,omc_asc_product,0);

// FILTER MODULE
omc_asc_p1_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_X_COS,omc_asc_product1,0);

// FILTER MODULE
omc_asc_y2_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_X_SIN,omc_asc_product6,0);

// FILTER MODULE
omc_asc_y2_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_X_COS,omc_asc_product7,0);

// FILTER MODULE
omc_asc_p2_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_X_SIN,omc_asc_product4,0);

// FILTER MODULE
omc_asc_p2_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_X_COS,omc_asc_product5,0);

// FILTER MODULE
omc_asc_y1_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_X_SIN,omc_asc_product2,0);

// FILTER MODULE
omc_asc_y1_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_X_COS,omc_asc_product3,0);

// PHASE
omc_lsc_phase[0] = (omc_lsc_x_sin * pLocalEpics->om1.OMC_LSC_PHASE[1]) + (omc_lsc_x_cos * pLocalEpics->om1.OMC_LSC_PHASE[0]);
omc_lsc_phase[1] = (omc_lsc_x_cos * pLocalEpics->om1.OMC_LSC_PHASE[1]) - (omc_lsc_x_sin * pLocalEpics->om1.OMC_LSC_PHASE[0]);

// Logical AND
omc_mon_logicaloperator4 = omc_mon_logicaloperator3 && omc_mon_logicaloperator6;

// MULTIPLY
omc_mon_tobinary_product2 = omc_mon_logicaloperator3 * omc_mon_tobinary_constant2;

// PRODUCT
pLocalEpics->om1.OMC_ASC_QPDGAIN_RMON = 
	gainRamp(pLocalEpics->om1.OMC_ASC_QPDGAIN,pLocalEpics->om1.OMC_ASC_QPDGAIN_TRAMP,0,&OMC_ASC_QPDGAIN_CALC);

omc_asc_qpdgain[0] = OMC_ASC_QPDGAIN_CALC * omc_asc_qpos_y;
omc_asc_qpdgain[1] = OMC_ASC_QPDGAIN_CALC * omc_asc_qpos_x;
omc_asc_qpdgain[2] = OMC_ASC_QPDGAIN_CALC * omc_asc_qang_y;
omc_asc_qpdgain[3] = OMC_ASC_QPDGAIN_CALC * omc_asc_qang_x;

// Bitwise |
omc_mon_tobinary_or1 = ((unsigned int)(omc_mon_tobinary_or5))|((unsigned int)(omc_mon_tobinary_product1));

// PHASE
omc_asc_p1_phase[0] = (omc_asc_p1_x_sin * pLocalEpics->om1.OMC_ASC_P1_PHASE[1]) + (omc_asc_p1_x_cos * pLocalEpics->om1.OMC_ASC_P1_PHASE[0]);
omc_asc_p1_phase[1] = (omc_asc_p1_x_cos * pLocalEpics->om1.OMC_ASC_P1_PHASE[1]) - (omc_asc_p1_x_sin * pLocalEpics->om1.OMC_ASC_P1_PHASE[0]);

// PHASE
omc_asc_y2_phase[0] = (omc_asc_y2_x_sin * pLocalEpics->om1.OMC_ASC_Y2_PHASE[1]) + (omc_asc_y2_x_cos * pLocalEpics->om1.OMC_ASC_Y2_PHASE[0]);
omc_asc_y2_phase[1] = (omc_asc_y2_x_cos * pLocalEpics->om1.OMC_ASC_Y2_PHASE[1]) - (omc_asc_y2_x_sin * pLocalEpics->om1.OMC_ASC_Y2_PHASE[0]);

// PHASE
omc_asc_p2_phase[0] = (omc_asc_p2_x_sin * pLocalEpics->om1.OMC_ASC_P2_PHASE[1]) + (omc_asc_p2_x_cos * pLocalEpics->om1.OMC_ASC_P2_PHASE[0]);
omc_asc_p2_phase[1] = (omc_asc_p2_x_cos * pLocalEpics->om1.OMC_ASC_P2_PHASE[1]) - (omc_asc_p2_x_sin * pLocalEpics->om1.OMC_ASC_P2_PHASE[0]);

// PHASE
omc_asc_y1_phase[0] = (omc_asc_y1_x_sin * pLocalEpics->om1.OMC_ASC_Y1_PHASE[1]) + (omc_asc_y1_x_cos * pLocalEpics->om1.OMC_ASC_Y1_PHASE[0]);
omc_asc_y1_phase[1] = (omc_asc_y1_x_cos * pLocalEpics->om1.OMC_ASC_Y1_PHASE[1]) - (omc_asc_y1_x_sin * pLocalEpics->om1.OMC_ASC_Y1_PHASE[0]);

// FILTER MODULE
omc_lsc_i = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_I,omc_lsc_phase[0],0);

// FILTER MODULE
omc_lsc_q = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_Q,omc_lsc_phase[1],0);

// MULTIPLY
omc_mon_tobinary_product3 = omc_mon_logicaloperator4 * omc_mon_tobinary_constant3;

// Bitwise |
omc_mon_tobinary_or2 = ((unsigned int)(omc_mon_tobinary_or1))|((unsigned int)(omc_mon_tobinary_product2));

// FILTER MODULE
omc_asc_p1_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_I,omc_asc_p1_phase[0],0);

// FILTER MODULE
omc_asc_p1_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_Q,omc_asc_p1_phase[1],0);

// FILTER MODULE
omc_asc_y2_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_I,omc_asc_y2_phase[0],0);

// FILTER MODULE
omc_asc_y2_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_Q,omc_asc_y2_phase[1],0);

// FILTER MODULE
omc_asc_p2_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_I,omc_asc_p2_phase[0],0);

// FILTER MODULE
omc_asc_p2_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_Q,omc_asc_p2_phase[1],0);

// FILTER MODULE
omc_asc_y1_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_I,omc_asc_y1_phase[0],0);

// FILTER MODULE
omc_asc_y1_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_Q,omc_asc_y1_phase[1],0);

// MULTIPLY
omc_lsc_product2 = omc_mon_logicaloperator2 * omc_lsc_i;

// Bitwise |
omc_mon_tobinary_or3 = ((unsigned int)(omc_mon_tobinary_or2))|((unsigned int)(omc_mon_tobinary_product3));

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_acmtx[1][ii] = 
	pLocalEpics->om1.OMC_ASC_ACMTX[ii][0] * omc_asc_p1_i +
	pLocalEpics->om1.OMC_ASC_ACMTX[ii][1] * omc_asc_y1_i +
	pLocalEpics->om1.OMC_ASC_ACMTX[ii][2] * omc_asc_p2_i +
	pLocalEpics->om1.OMC_ASC_ACMTX[ii][3] * omc_asc_y2_i;
}

// FILTER MODULE
omc_lsc_gain = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_GAIN,omc_lsc_product2,0);

// Bitwise |
omc_mon_tobinary_or4 = ((unsigned int)(omc_mon_tobinary_or3))|((unsigned int)(omc_mon_tobinary_product4));

// MULTIPLY
omc_asc_enable1_product11 = omc_asc_acmtx[1][0] * omc_mon_logicaloperator2;

// MULTIPLY
omc_asc_enable1_product1 = omc_asc_acmtx[1][1] * omc_mon_logicaloperator2;

// MULTIPLY
omc_asc_enable1_product2 = omc_asc_acmtx[1][2] * omc_mon_logicaloperator2;

// MULTIPLY
omc_asc_enable1_product3 = omc_asc_acmtx[1][3] * omc_mon_logicaloperator2;

// FILTER MODULE
omc_lsc_output = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_OUTPUT,omc_lsc_gain,0);

// Bitwise |
omc_mon_tobinary_or6 = ((unsigned int)(omc_mon_tobinary_or4))|((unsigned int)(omc_mon_tobinary_product5));

// FILTER MODULE
omc_asc_dpos_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DPOS_Y,omc_asc_enable1_product11,0);

// FILTER MODULE
omc_asc_dpos_yb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DPOS_YB,omc_asc_enable1_product11,0);

// FILTER MODULE
omc_asc_dpos_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DPOS_X,omc_asc_enable1_product1,0);

// FILTER MODULE
omc_asc_dpos_xb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DPOS_XB,omc_asc_enable1_product1,0);

// FILTER MODULE
omc_asc_dang_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DANG_Y,omc_asc_enable1_product2,0);

// FILTER MODULE
omc_asc_dang_yb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DANG_YB,omc_asc_enable1_product2,0);

// FILTER MODULE
omc_asc_dang_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DANG_X,omc_asc_enable1_product3,0);

// FILTER MODULE
omc_asc_dang_xb = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DANG_XB,omc_asc_enable1_product3,0);

// FILTER MODULE
omc_pzt_lsc = filterModuleD(dsp_ptr,dspCoeff,OMC_PZT_LSC,omc_lsc_output,0);

// MULTIPLY
omc_product = omc_lsc_output * omc_mon_logicaloperator3;

// Bitwise |
omc_mon_tobinary_or7 = ((unsigned int)(omc_mon_tobinary_or6))|((unsigned int)(omc_mon_tobinary_product6));

// SUM
omc_asc_sum4 = omc_asc_qpos_yb + omc_asc_dpos_yb;

// SUM
omc_asc_sum5 = omc_asc_qpos_xb + omc_asc_dpos_xb;

// SUM
omc_asc_sum6 = omc_asc_qang_yb + omc_asc_dang_yb;

// PRODUCT
pLocalEpics->om1.OMC_ASC_DITHERGAIN_RMON = 
	gainRamp(pLocalEpics->om1.OMC_ASC_DITHERGAIN,pLocalEpics->om1.OMC_ASC_DITHERGAIN_TRAMP,1,&OMC_ASC_DITHERGAIN_CALC);

omc_asc_dithergain[0] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dpos_y;
omc_asc_dithergain[1] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dpos_x;
omc_asc_dithergain[2] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dang_y;
omc_asc_dithergain[3] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dang_x;

// SUM
omc_asc_sum7 = omc_asc_qang_xb + omc_asc_dang_xb;

// SUM
omc_pzt_sum = omc_pzt_lsc + pLocalEpics->om1.OMC_PZT_BIAS;

// FILTER MODULE
omc_htr_lsc = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_LSC,omc_product,0);

// Bitwise |
omc_mon_tobinary_or8 = ((unsigned int)(omc_mon_tobinary_or7))|((unsigned int)(omc_mon_tobinary_product7));

// PRODUCT
pLocalEpics->om1.OMC_ASC_BLENDGAIN_RMON = 
	gainRamp(pLocalEpics->om1.OMC_ASC_BLENDGAIN,pLocalEpics->om1.OMC_ASC_BLENDGAIN_TRAMP,2,&OMC_ASC_BLENDGAIN_CALC);

omc_asc_blendgain[0] = OMC_ASC_BLENDGAIN_CALC * omc_asc_sum4;
omc_asc_blendgain[1] = OMC_ASC_BLENDGAIN_CALC * omc_asc_sum5;
omc_asc_blendgain[2] = OMC_ASC_BLENDGAIN_CALC * omc_asc_sum6;
omc_asc_blendgain[3] = OMC_ASC_BLENDGAIN_CALC * omc_asc_sum7;

// FILTER MODULE
omc_htr_drv = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_DRV,omc_htr_lsc,0);

// EpicsOut
pLocalEpics->om1.OMC_MON_OUTPUT = omc_mon_tobinary_or8;

// SUM
omc_asc_sum8 = omc_asc_blendgain[0] + omc_asc_qpdgain[0] + omc_asc_dithergain[0];

// SUM
omc_asc_sum9 = omc_asc_blendgain[1] + omc_asc_qpdgain[1] + omc_asc_dithergain[1];

// SUM
omc_asc_sum10 = omc_asc_blendgain[2] + omc_asc_qpdgain[2] + omc_asc_dithergain[2];

// SUM
omc_asc_sum11 = omc_asc_blendgain[3] + omc_asc_qpdgain[3] + omc_asc_dithergain[3];

// PRODUCT
pLocalEpics->om1.OMC_ASC_MASTERGAIN_RMON = 
	gainRamp(pLocalEpics->om1.OMC_ASC_MASTERGAIN,pLocalEpics->om1.OMC_ASC_MASTERGAIN_TRAMP,3,&OMC_ASC_MASTERGAIN_CALC);

omc_asc_mastergain[0] = OMC_ASC_MASTERGAIN_CALC * omc_asc_sum8;
omc_asc_mastergain[1] = OMC_ASC_MASTERGAIN_CALC * omc_asc_sum9;
omc_asc_mastergain[2] = OMC_ASC_MASTERGAIN_CALC * omc_asc_sum10;
omc_asc_mastergain[3] = OMC_ASC_MASTERGAIN_CALC * omc_asc_sum11;

// FILTER MODULE
omc_asc_pos_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_POS_Y,omc_asc_mastergain[0],0);

// FILTER MODULE
omc_asc_pos_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_POS_X,omc_asc_mastergain[1],0);

// FILTER MODULE
omc_asc_ang_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_ANG_Y,omc_asc_mastergain[2],0);

// FILTER MODULE
omc_asc_ang_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_ANG_X,omc_asc_mastergain[3],0);

// SUM
omc_asc_sum = omc_asc_pos_y + omc_asc_p1_clock;

// SUM
omc_asc_sum1 = omc_asc_pos_x + omc_asc_y1_clock;

// SUM
omc_asc_sum2 = omc_asc_ang_y + omc_asc_p2_clock;

// SUM
omc_asc_sum3 = omc_asc_ang_x + omc_asc_y2_clock;

// MULTIPLY
omc_asc_product8 = omc_asc_sum * omc_asc_operator;

// MULTIPLY
omc_asc_product9 = omc_asc_sum1 * omc_asc_operator;

// MULTIPLY
omc_asc_product10 = omc_asc_sum2 * omc_asc_operator;

// MULTIPLY
omc_asc_product11 = omc_asc_sum3 * omc_asc_operator;


//End of subsystem   OMC **************************************************


if (cdsPciModules.pci_rfm[0] != 0) {
  // RFM output
  *((double *)(((char *)cdsPciModules.pci_rfm[0]) + 0x12200cc)) = omc_dcerr_choice;
}

ipc_at_0x2000 = omc_asc_product8;

ipc_at_0x2008 = omc_asc_product9;

ipc_at_0x2010 = omc_asc_product10;

ipc_at_0x2018 = omc_asc_product11;

ipc_at_0x2070 = omc_pd_ctrl_or1;

ipc_at_0x2088 = omc_pd_divide;

if (_ipc_shm != 0) {
  // IPCS output
  *((float *)(((char *)_ipc_shm) + 0x3000)) = (cycle + 1)%FE_RATE;
}

// DAC number is 0
dacOut[0][0] = ipc_at_0x2028;
dacOut[0][1] = ipc_at_0x2030;
dacOut[0][2] = ipc_at_0x2038;
dacOut[0][3] = ipc_at_0x2040;
dacOut[0][4] = ipc_at_0x2048;
dacOut[0][5] = ipc_at_0x2050;
dacOut[0][6] = ipc_at_0x2058;
dacOut[0][7] = ipc_at_0x2060;
dacOut[0][8] = omc_htr_drv;
dacOut[0][9] = omc_dac0_test10;
dacOut[0][10] = omc_dac0_test1;
dacOut[0][11] = omc_dac0_test2;
dacOut[0][12] = omc_pzt_dither;
dacOut[0][13] = omc_pzt_sum;
dacOut[0][14] = omc_dac0_test3;
dacOut[0][15] = omc_dac0_test4;


    // All IPC outputs
    if (_ipc_shm != 0) {
      *((double *)(((char *)_ipc_shm) + 0x2020)) = ipc_at_0x2020;
      *((double *)(((char *)_ipc_shm) + 0x2078)) = ipc_at_0x2078;
      *((double *)(((char *)_ipc_shm) + 0x2080)) = ipc_at_0x2080;
      *((double *)(((char *)_ipc_shm) + 0x20A0)) = ipc_at_0x20A0;
      *((double *)(((char *)_ipc_shm) + 0x20A8)) = ipc_at_0x20A8;
      *((double *)(((char *)_ipc_shm) + 0x20B0)) = ipc_at_0x20B0;
      *((double *)(((char *)_ipc_shm) + 0x20B8)) = ipc_at_0x20B8;
      *((double *)(((char *)_ipc_shm) + 0x20C0)) = ipc_at_0x20C0;
      *((double *)(((char *)_ipc_shm) + 0x20C8)) = ipc_at_0x20C8;
      *((double *)(((char *)_ipc_shm) + 0x20D0)) = ipc_at_0x20D0;
      *((double *)(((char *)_ipc_shm) + 0x20D8)) = ipc_at_0x20D8;
      *((double *)(((char *)_ipc_shm) + 0x2000)) = ipc_at_0x2000;
      *((double *)(((char *)_ipc_shm) + 0x2008)) = ipc_at_0x2008;
      *((double *)(((char *)_ipc_shm) + 0x2010)) = ipc_at_0x2010;
      *((double *)(((char *)_ipc_shm) + 0x2018)) = ipc_at_0x2018;
      *((double *)(((char *)_ipc_shm) + 0x2070)) = ipc_at_0x2070;
      *((double *)(((char *)_ipc_shm) + 0x2088)) = ipc_at_0x2088;
    }
  }
}

