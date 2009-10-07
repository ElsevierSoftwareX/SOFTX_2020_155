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
#ifdef SERVO4K
	#define FE_RATE	4096
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
double ipc_at_0x2020 = *((double *)(((void *)_ipc_shm) + 0x2020));
double ipc_at_0x2028 = *((double *)(((void *)_ipc_shm) + 0x2028));
double ipc_at_0x2030 = *((double *)(((void *)_ipc_shm) + 0x2030));
double ipc_at_0x2038 = *((double *)(((void *)_ipc_shm) + 0x2038));
double ipc_at_0x2040 = *((double *)(((void *)_ipc_shm) + 0x2040));
double ipc_at_0x2048 = *((double *)(((void *)_ipc_shm) + 0x2048));
double ipc_at_0x2050 = *((double *)(((void *)_ipc_shm) + 0x2050));
double ipc_at_0x2058 = *((double *)(((void *)_ipc_shm) + 0x2058));
double ipc_at_0x2060;
double ipc_at_0x2068;
double ipc_at_0x2070;
double ipc_at_0x2078;
double ipc_at_0x2080;
double ipc_at_0x2088;
double ipc_at_0x2090;
double ipc_at_0x20A0;
double ipc_at_0x20A8;
double ipc_at_0x20B0;
double ipc_at_0x20B8;
double ipc_at_0x20D0;
double ipc_at_0x20D8;
double ipc_at_0x20E0;
double ipc_at_0x20E8;
double ipc_at_0x20F0;
static float ground1;
static float ground2;
double omc_ugf_test2_q;
static double omc_ugf_test2_phase[2];
double omc_ugf_test2_i;
double omc_ugf_test2;
double omc_ugf_test1_q;
static double omc_ugf_test1_phase[2];
double omc_ugf_test1_i;
double omc_ugf_test1;
double omc_ugf_sum2;
double omc_ugf_sum;
double omc_ugf_product7;
double omc_ugf_product6;
double omc_ugf_product5;
double omc_ugf_product4;
double omc_ugf_product3;
double omc_ugf_product2;
double omc_ugf_product1;
static double omc_ugf_osc[3];
static double omc_ugf_osc_freq;
static double omc_ugf_osc_delta;
static double omc_ugf_osc_alpha;
static double omc_ugf_osc_beta;
static double omc_ugf_osc_cos_prev;
static double omc_ugf_osc_sin_prev;
static double omc_ugf_osc_cos_new;
static double omc_ugf_osc_sin_new;
double lsinx, lcosx, valx;
double omc_ugf_mux1[5];
double omc_ugf_mux[5];
#include "OMC_UGF_MAG2DB.c"
static float omc_ugf_ground7;
static float omc_ugf_ground6;
static float omc_ugf_ground5;
static float omc_ugf_ground4;
static float omc_ugf_ground3;
static float omc_ugf_ground2;
static float omc_ugf_ground1;
static float omc_ugf_ground;
double omc_ugf_divide1;
double omc_ugf_demux1[1];
double omc_ugf_demux[1];
#include "OMC_UGF_DB2MAG.c"
double omc_ugf_ctrl;
double omc_ugf_clockmon;
double omc_qpd4_y;
double omc_qpd4_saturation;
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
double omc_qpd3_saturation;
double omc_qpd3_sum;
double omc_qpd3_seg4;
double omc_qpd3_seg3;
double omc_qpd3_seg2;
double omc_qpd3_seg1;
double omc_qpd3_p;
double omc_qpd3_inmtrx[3][4];
double omc_qpd3_divide1;
double omc_qpd3_divide;
double omc_qpd2_y;
double omc_qpd2_saturation;
double omc_qpd2_sum;
double omc_qpd2_seg4;
double omc_qpd2_seg3;
double omc_qpd2_seg2;
double omc_qpd2_seg1;
double omc_qpd2_p;
double omc_qpd2_inmtrx[3][4];
double omc_qpd2_divide1;
double omc_qpd2_divide;
double omc_qpd1_y;
double omc_qpd1_saturation;
double omc_qpd1_sum;
double omc_qpd1_seg4;
double omc_qpd1_seg3;
double omc_qpd1_seg2;
double omc_qpd1_seg1;
double omc_qpd1_p;
double omc_qpd1_inmtrx[3][4];
double omc_qpd1_divide1;
double omc_qpd1_divide;
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
double omc_pd_saturation;
double omc_pd_sum;
double omc_pd_shutter;
double omc_pd_refl;
double omc_pd_norm_filt;
double omc_pd_lockin_y;
double omc_pd_lockin_x;
double omc_pd_inmtrx[3][3];
double omc_pd_divide;
static double omc_pd_ctrl_constant0;
static double omc_pd_ctrl_constant1;
static double omc_pd_ctrl_constant128;
static double omc_pd_ctrl_constant2;
static double omc_pd_ctrl_constant3;
static double omc_pd_ctrl_constant4;
static double omc_pd_ctrl_constant5;
static double omc_pd_ctrl_constant512;
static float omc_pd_ctrl_ground;
double omc_pd_ctrl_operator1;
double omc_pd_ctrl_operator2;
double omc_pd_ctrl_operator3;
double omc_pd_ctrl_operator4;
double omc_pd_ctrl_product;
double omc_pd_ctrl_product1;
double omc_pd_ctrl_product2;
unsigned int omc_pd_ctrl_and;
unsigned int omc_pd_ctrl_and1;
unsigned int omc_pd_ctrl_and2;
unsigned int omc_pd_ctrl_or1;
unsigned int omc_pd_ctrl_or2;
unsigned int omc_pd_ctrl_or5;
double omc_math_product6;
double omc_math_product5;
double omc_math_product4;
double omc_math_product3;
double omc_math_product2;
double omc_math_product1;
double omc_math_inmtrx[8][8];
static float omc_math_ground2;
double omc_math_divide;
double omc_math_divfilt;
double omc_lsc_x_sin;
double omc_lsc_x_cos;
double omc_lsc_sig;
double omc_lsc_q;
double omc_lsc_product2;
double omc_lsc_product1;
double omc_lsc_product;
static double omc_lsc_phase[2];
static double omc_lsc_osc[3];
static double omc_lsc_osc_freq;
static double omc_lsc_osc_delta;
static double omc_lsc_osc_alpha;
static double omc_lsc_osc_beta;
static double omc_lsc_osc_cos_prev;
static double omc_lsc_osc_sin_prev;
static double omc_lsc_osc_cos_new;
static double omc_lsc_osc_sin_new;
double omc_lsc_i;
static float omc_lsc_ground1;
double omc_lsc_gain;
double omc_lsc_clock;
double omc_lock_trigger_operator;
static float omc_lock_trigger_ground;
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
double omc_heater_trigger_operator;
static float omc_heater_trigger_ground;
double omc_dcerr_tp;
#include "OMC_DCERR_OMC_DCERR.c"
double omc_dcerr_mux[10];
static float omc_dcerr_ground1;
double omc_dcerr_demux[5];
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
static float omc_auto_ground5;
static float omc_auto_ground4;
static float omc_auto_ground1;
static float omc_auto_ground;
double omc_duotone;
static float omc_ground1;
static float omc_ground2;
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
dWordUsed[0][22] =  1;
dWordUsed[0][23] =  1;
dWordUsed[0][24] =  1;
dWordUsed[0][25] =  1;
dWordUsed[0][28] =  1;
dWordUsed[0][29] =  1;
dWordUsed[0][30] =  1;
dWordUsed[0][31] =  1;
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
ground1 = 0.0;
ground2 = 0.0;
omc_ugf_osc_freq = pLocalEpics->om1.OMC_UGF_OSC_FREQ;
omc_ugf_osc_delta = 2.0 * 3.1415926535897932384626 * omc_ugf_osc_freq / FE_RATE;
valx = omc_ugf_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_ugf_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_ugf_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_ugf_osc_beta = lsinx;
omc_ugf_osc_cos_prev = 1.0;
omc_ugf_osc_sin_prev = 0.0;
omc_ugf_ground7 = 0.0;
omc_ugf_ground6 = 0.0;
omc_ugf_ground5 = 0.0;
omc_ugf_ground4 = 0.0;
omc_ugf_ground3 = 0.0;
omc_ugf_ground2 = 0.0;
omc_ugf_ground1 = 0.0;
omc_ugf_ground = 0.0;
omc_pzt_pzt_dcrms_avg = 0.0;
omc_pzt_pzt_acrms_avg = 0.0;
omc_pzt_ground = 0.0;
omc_pd_ctrl_constant0 = (double)0;
omc_pd_ctrl_constant1 = (double)512;
omc_pd_ctrl_constant128 = (double)128;
omc_pd_ctrl_constant2 = (double)2;
omc_pd_ctrl_constant3 = (double)0;
omc_pd_ctrl_constant4 = (double)4;
omc_pd_ctrl_constant5 = (double)8;
omc_pd_ctrl_constant512 = (double)512;
omc_pd_ctrl_ground = 0.0;
omc_math_ground2 = 0.0;
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
omc_lock_trigger_ground = 0.0;
omc_htr_htr_rms_avg = 0.0;
omc_htr_ground3 = 0.0;
omc_htr_ground2 = 0.0;
omc_htr_ground1 = 0.0;
omc_heater_trigger_ground = 0.0;
omc_dcerr_ground1 = 0.0;
omc_dac0_ground8 = 0.0;
omc_dac0_ground7 = 0.0;
omc_dac0_ground6 = 0.0;
omc_dac0_ground5 = 0.0;
omc_dac0_ground = 0.0;
omc_auto_ground5 = 0.0;
omc_auto_ground4 = 0.0;
omc_auto_ground1 = 0.0;
omc_auto_ground = 0.0;
omc_ground1 = 0.0;
omc_ground2 = 0.0;
} else {
    *((volatile unsigned int *)(((char *)_ipc_shm) + 0x3010)) = 0;
 
 
 
 
 
 
 
 
ipc_at_0x2080 = dWord[0][28];

ipc_at_0x2088 = dWord[0][29];

ipc_at_0x2090 = dWord[0][30];






//Start of subsystem OMC **************************************************





// Osc
omc_ugf_osc_cos_new = (1.0 - omc_ugf_osc_alpha) * omc_ugf_osc_cos_prev - omc_ugf_osc_beta * omc_ugf_osc_sin_prev;
omc_ugf_osc_sin_new = (1.0 - omc_ugf_osc_alpha) * omc_ugf_osc_sin_prev + omc_ugf_osc_beta * omc_ugf_osc_cos_prev;
omc_ugf_osc_sin_prev = omc_ugf_osc_sin_new;
omc_ugf_osc_cos_prev = omc_ugf_osc_cos_new;
omc_ugf_osc[0] = omc_ugf_osc_sin_new * pLocalEpics->om1.OMC_UGF_OSC_CLKGAIN;
omc_ugf_osc[1] = omc_ugf_osc_sin_new * pLocalEpics->om1.OMC_UGF_OSC_SINGAIN;
omc_ugf_osc[2] = omc_ugf_osc_cos_new * pLocalEpics->om1.OMC_UGF_OSC_COSGAIN;
if((omc_ugf_osc_freq != pLocalEpics->om1.OMC_UGF_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_ugf_osc_freq = pLocalEpics->om1.OMC_UGF_OSC_FREQ;
	omc_ugf_osc_delta = 2.0 * 3.1415926535897932384626 * omc_ugf_osc_freq / FE_RATE;
	valx = omc_ugf_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_ugf_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_ugf_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_ugf_osc_beta = lsinx;
	omc_ugf_osc_cos_prev = 1.0;
	omc_ugf_osc_sin_prev = 0.0;
}


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
omc_qpd1_seg1 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_SEG1,dWord[0][0],0);

// FILTER MODULE
omc_qpd1_seg2 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_SEG2,dWord[0][1],0);

// FILTER MODULE
omc_qpd1_seg3 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_SEG3,dWord[0][2],0);

// FILTER MODULE
omc_qpd1_seg4 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_SEG4,dWord[0][3],0);

// FILTER MODULE
omc_qpd2_seg1 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_SEG1,dWord[0][4],0);

// FILTER MODULE
omc_qpd2_seg2 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_SEG2,dWord[0][5],0);

// FILTER MODULE
omc_qpd2_seg3 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_SEG3,dWord[0][6],0);

// FILTER MODULE
omc_qpd2_seg4 = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_SEG4,dWord[0][7],0);

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

// FILTER MODULE
omc_pd_lockin_x = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_LOCKIN_X,dWord[0][22],0);

// FILTER MODULE
omc_pd_lockin_y = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_LOCKIN_Y,dWord[0][23],0);

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
omc_pd_refl = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_REFL,ground1,0);

// FILTER MODULE
omc_pd_shutter = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_SHUTTER,dWord[0][28],0);

// FILTER MODULE
omc_nptr = filterModuleD(dsp_ptr,dspCoeff,OMC_NPTR,cdsPciModules.pci_rfm[0]? *((double *)(((void *)cdsPciModules.pci_rfm[0]) + 0x12200d4)) : 0.0,0);

// FILTER MODULE
omc_duotone = filterModuleD(dsp_ptr,dspCoeff,OMC_DUOTONE,dWord[0][31],0);

// FILTER MODULE
omc_ugf_clockmon = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_CLOCKMON,omc_ugf_osc[0],0);

// Relational Operator
omc_pd_ctrl_operator3 = ((omc_pd_ctrl_constant0) != (pLocalEpics->om1.OMC_PD_CTRL_ZSWITCH));
// FILTER MODULE
omc_lsc_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_CLOCK,omc_lsc_osc[0],0);

// Matrix
for(ii=0;ii<3;ii++)
{
omc_qpd1_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_QPD1_INMTRX[ii][0] * omc_qpd1_seg1 +
	pLocalEpics->om1.OMC_QPD1_INMTRX[ii][1] * omc_qpd1_seg2 +
	pLocalEpics->om1.OMC_QPD1_INMTRX[ii][2] * omc_qpd1_seg3 +
	pLocalEpics->om1.OMC_QPD1_INMTRX[ii][3] * omc_qpd1_seg4;
}

// Matrix
for(ii=0;ii<3;ii++)
{
omc_qpd2_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_QPD2_INMTRX[ii][0] * omc_qpd2_seg1 +
	pLocalEpics->om1.OMC_QPD2_INMTRX[ii][1] * omc_qpd2_seg2 +
	pLocalEpics->om1.OMC_QPD2_INMTRX[ii][2] * omc_qpd2_seg3 +
	pLocalEpics->om1.OMC_QPD2_INMTRX[ii][3] * omc_qpd2_seg4;
}

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
omc_pd_ctrl_and = ((unsigned int)(dsp_ptr->inputs[OMC_PD_TRANS1].opSwitchP| ((0x4|0x8|0x1000000|0x2000000|0x4000000|0x8000000) & dsp_ptr->inputs[OMC_PD_TRANS1].opSwitchE)))&((unsigned int)(omc_pd_ctrl_constant128));

// Bitwise &
omc_pd_ctrl_and1 = ((unsigned int)(dsp_ptr->inputs[OMC_PD_TRANS1].opSwitchP| ((0x4|0x8|0x1000000|0x2000000|0x4000000|0x8000000) & dsp_ptr->inputs[OMC_PD_TRANS1].opSwitchE)))&((unsigned int)(omc_pd_ctrl_constant512));

// Bitwise &
omc_pd_ctrl_and2 = ((unsigned int)(dsp_ptr->inputs[OMC_PD_TRANS2].opSwitchP| ((0x4|0x8|0x1000000|0x2000000|0x4000000|0x8000000) & dsp_ptr->inputs[OMC_PD_TRANS2].opSwitchE)))&((unsigned int)(omc_pd_ctrl_constant1));

// Matrix
for(ii=0;ii<3;ii++)
{
omc_pd_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_PD_INMTRX[ii][0] * omc_pd_trans1 +
	pLocalEpics->om1.OMC_PD_INMTRX[ii][1] * omc_pd_trans2 +
	pLocalEpics->om1.OMC_PD_INMTRX[ii][2] * omc_pd_shutter;
}

// EpicsOut
pLocalEpics->om1.OMC_DCERR_PARM = omc_nptr;

// MULTIPLY
omc_pd_ctrl_product1 = omc_pd_ctrl_operator3 * omc_pd_ctrl_constant4;

// FILTER MODULE
omc_pzt_dither = filterModuleD(dsp_ptr,dspCoeff,OMC_PZT_DITHER,omc_lsc_clock,0);

// FILTER MODULE
omc_qpd1_sum = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_SUM,omc_qpd1_inmtrx[1][0],0);

// FILTER MODULE
omc_qpd2_sum = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_SUM,omc_qpd2_inmtrx[1][0],0);

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
// Relational Operator
omc_pd_ctrl_operator4 = ((omc_pd_ctrl_and2) != (omc_pd_ctrl_constant3));
// FILTER MODULE
omc_nullstream = filterModuleD(dsp_ptr,dspCoeff,OMC_NULLSTREAM,omc_pd_inmtrx[1][2],0);

// FILTER MODULE
omc_pd_sum = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_SUM,omc_pd_inmtrx[1][0],0);

// FILTER MODULE
omc_pd_norm_filt = filterModuleD(dsp_ptr,dspCoeff,OMC_PD_NORM_FILT,omc_pd_inmtrx[1][1],0);

// EpicsOut
pLocalEpics->om1.OMC_QPD1_SUM_MON = omc_qpd1_sum;

// SATURATE
omc_qpd1_saturation = omc_qpd1_sum;
if (omc_qpd1_saturation > 100000) {
  omc_qpd1_saturation  = 100000;
} else if (omc_qpd1_saturation < 0.1) {
  omc_qpd1_saturation  = 0.1;
};

// EpicsOut
pLocalEpics->om1.OMC_QPD2_SUM_MON = omc_qpd2_sum;

// SATURATE
omc_qpd2_saturation = omc_qpd2_sum;
if (omc_qpd2_saturation > 100000) {
  omc_qpd2_saturation  = 100000;
} else if (omc_qpd2_saturation < 0.1) {
  omc_qpd2_saturation  = 0.1;
};

// SATURATE
omc_qpd3_saturation = omc_qpd3_sum;
if (omc_qpd3_saturation > 100000) {
  omc_qpd3_saturation  = 100000;
} else if (omc_qpd3_saturation < 0.1) {
  omc_qpd3_saturation  = 0.1;
};

// EpicsOut
pLocalEpics->om1.OMC_QPD3_SUM_MON = omc_qpd3_sum;

// EpicsOut
pLocalEpics->om1.OMC_QPD4_SUM_MON = omc_qpd4_sum;

// SATURATE
omc_qpd4_saturation = omc_qpd4_sum;
if (omc_qpd4_saturation > 100000) {
  omc_qpd4_saturation  = 100000;
} else if (omc_qpd4_saturation < 0.1) {
  omc_qpd4_saturation  = 0.1;
};

// EpicsOut
pLocalEpics->om1.OMC_HTR_I_RMS = omc_htr_htr_rms;

// MULTIPLY
omc_pd_ctrl_product = omc_pd_ctrl_operator1 * omc_pd_ctrl_constant2;

// MULTIPLY
omc_pd_ctrl_product2 = omc_pd_ctrl_operator4 * omc_pd_ctrl_constant5;

// Relational Operator
omc_heater_trigger_operator = ((omc_pd_sum) >= (pLocalEpics->om1.OMC_HEATER_TRIGGER_THRESHOLD));
// Relational Operator
omc_lock_trigger_operator = ((omc_pd_sum) >= (pLocalEpics->om1.OMC_LOCK_TRIGGER_THRESHOLD));
// EpicsOut
pLocalEpics->om1.OMC_DCERR_PAS = omc_pd_sum;

// MUX
omc_dcerr_mux[0]= omc_pd_sum;
omc_dcerr_mux[1]= omc_nptr;
omc_dcerr_mux[2]= pLocalEpics->om1.OMC_DCERR_Pref;
omc_dcerr_mux[3]= pLocalEpics->om1.OMC_DCERR_Pdefect;
omc_dcerr_mux[4]= pLocalEpics->om1.OMC_DCERR_x0;
omc_dcerr_mux[5]= pLocalEpics->om1.OMC_DCERR_xf;
omc_dcerr_mux[6]= pLocalEpics->om1.OMC_DCERR_C2;
omc_dcerr_mux[7]= pLocalEpics->om1.OMC_DCERR_TRAMP;
omc_dcerr_mux[8]= pLocalEpics->om1.OMC_DCERR_LOAD;
omc_dcerr_mux[9]= pLocalEpics->om1.OMC_DCERR_TPSELECT;

// Matrix
for(ii=0;ii<8;ii++)
{
omc_math_inmtrx[1][ii] = 
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][0] * omc_pd_sum +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][1] * omc_qpd3_sum +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][2] * omc_qpd4_sum +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][3] * omc_ground2 +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][4] * omc_qpd1_sum +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][5] * omc_qpd2_sum +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][6] * omc_ground1 +
	pLocalEpics->om1.OMC_MATH_INMTRX[ii][7] * pLocalEpics->om1.OMC_MATH_Input8;
}

// SATURATE
omc_pd_saturation = omc_pd_norm_filt;
if (omc_pd_saturation > 100000) {
  omc_pd_saturation  = 100000;
} else if (omc_pd_saturation < 0.1) {
  omc_pd_saturation  = 0.1;
};

// DIVIDE
if(omc_qpd1_saturation != 0.0)
{
	omc_qpd1_divide1 = omc_qpd1_inmtrx[1][2] / omc_qpd1_saturation;
}
else{
	omc_qpd1_divide1 = 0.0;
}

// DIVIDE
if(omc_qpd1_saturation != 0.0)
{
	omc_qpd1_divide = omc_qpd1_inmtrx[1][1] / omc_qpd1_saturation;
}
else{
	omc_qpd1_divide = 0.0;
}

// DIVIDE
if(omc_qpd2_saturation != 0.0)
{
	omc_qpd2_divide = omc_qpd2_inmtrx[1][1] / omc_qpd2_saturation;
}
else{
	omc_qpd2_divide = 0.0;
}

// DIVIDE
if(omc_qpd2_saturation != 0.0)
{
	omc_qpd2_divide1 = omc_qpd2_inmtrx[1][2] / omc_qpd2_saturation;
}
else{
	omc_qpd2_divide1 = 0.0;
}

// DIVIDE
if(omc_qpd3_saturation != 0.0)
{
	omc_qpd3_divide = omc_qpd3_inmtrx[1][1] / omc_qpd3_saturation;
}
else{
	omc_qpd3_divide = 0.0;
}

// DIVIDE
if(omc_qpd3_saturation != 0.0)
{
	omc_qpd3_divide1 = omc_qpd3_inmtrx[1][2] / omc_qpd3_saturation;
}
else{
	omc_qpd3_divide1 = 0.0;
}

// DIVIDE
if(omc_qpd4_saturation != 0.0)
{
	omc_qpd4_divide = omc_qpd4_inmtrx[1][1] / omc_qpd4_saturation;
}
else{
	omc_qpd4_divide = 0.0;
}

// DIVIDE
if(omc_qpd4_saturation != 0.0)
{
	omc_qpd4_divide1 = omc_qpd4_inmtrx[1][2] / omc_qpd4_saturation;
}
else{
	omc_qpd4_divide1 = 0.0;
}

// Bitwise |
omc_pd_ctrl_or5 = ((unsigned int)(omc_pd_ctrl_operator2))|((unsigned int)(omc_pd_ctrl_product));

// EpicsOut
pLocalEpics->om1.OMC_HEATER_TRIGGER_LOCKMON = omc_heater_trigger_operator;

// EpicsOut
pLocalEpics->om1.OMC_LOCK_TRIGGER_LOCKMON = omc_lock_trigger_operator;

// Function Call
OMC_DCERR_OMC_DCERR(omc_dcerr_mux, 10, omc_dcerr_demux, 5);

// MULTIPLY
omc_math_product2 = omc_math_inmtrx[1][0] * omc_math_inmtrx[1][1];

// MULTIPLY
omc_math_product5 = omc_math_inmtrx[1][4] * omc_math_inmtrx[1][5];

// DIVIDE
if(omc_pd_saturation != 0.0)
{
	omc_pd_divide = omc_pd_inmtrx[1][1] / omc_pd_saturation;
}
else{
	omc_pd_divide = 0.0;
}

// FILTER MODULE
omc_qpd1_y = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_Y,omc_qpd1_divide1,0);

// FILTER MODULE
omc_qpd1_p = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD1_P,omc_qpd1_divide,0);

// FILTER MODULE
omc_qpd2_p = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_P,omc_qpd2_divide,0);

// FILTER MODULE
omc_qpd2_y = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD2_Y,omc_qpd2_divide1,0);

// FILTER MODULE
omc_qpd3_p = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_P,omc_qpd3_divide,0);

// FILTER MODULE
omc_qpd3_y = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD3_Y,omc_qpd3_divide1,0);

// FILTER MODULE
omc_qpd4_p = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_P,omc_qpd4_divide,0);

// FILTER MODULE
omc_qpd4_y = filterModuleD(dsp_ptr,dspCoeff,OMC_QPD4_Y,omc_qpd4_divide1,0);

// Bitwise |
omc_pd_ctrl_or1 = ((unsigned int)(omc_pd_ctrl_or5))|((unsigned int)(omc_pd_ctrl_product1));


// MULTIPLY
omc_math_product1 = omc_math_product2 * omc_math_inmtrx[1][2];

// MULTIPLY
omc_math_product4 = omc_math_product5 * omc_math_inmtrx[1][6];

// FILTER MODULE
omc_lsc_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_SIG,omc_pd_divide,0);

// Bitwise |
omc_pd_ctrl_or2 = ((unsigned int)(omc_pd_ctrl_or1))|((unsigned int)(omc_pd_ctrl_product2));

// Switch
omc_dcerr_choice = (((pLocalEpics->om1.OMC_DCERR_BYPASS) != 0)? (omc_pd_sum): (omc_dcerr_demux[0]));
// EpicsOut
pLocalEpics->om1.OMC_DCERR_STATE = omc_dcerr_demux[1];

// EpicsOut
pLocalEpics->om1.OMC_DCERR_GAIN_MON = omc_dcerr_demux[2];

// EpicsOut
pLocalEpics->om1.OMC_DCERR_OFFSET_MON = omc_dcerr_demux[3];

// FILTER MODULE
omc_dcerr_tp = filterModuleD(dsp_ptr,dspCoeff,OMC_DCERR_TP,omc_dcerr_demux[4],0);

// MULTIPLY
omc_math_product3 = omc_math_product1 * omc_math_inmtrx[1][3];

// MULTIPLY
omc_math_product6 = omc_math_product4 * omc_math_inmtrx[1][7];

// MULTIPLY
omc_lsc_product = omc_lsc_sig * omc_lsc_osc[1];

// MULTIPLY
omc_lsc_product1 = omc_lsc_sig * omc_lsc_osc[2];

// EpicsOut
pLocalEpics->om1.OMC_PD_CTRL_WHITENING_CTRL_MON = omc_pd_ctrl_or2;

// FILTER MODULE
omc_ugf_test1 = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_TEST1,omc_dcerr_choice,0);

// SUM
omc_ugf_sum = omc_ugf_clockmon + omc_dcerr_choice;

// EpicsOut
pLocalEpics->om1.OMC_DCERR_OUTMON = omc_dcerr_choice;

// EpicsOut
pLocalEpics->om1.OMC_MATH_NUMERMON = omc_math_product3;

// EpicsOut
pLocalEpics->om1.OMC_MATH_DENOMMON = omc_math_product6;

// FILTER MODULE
omc_lsc_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_X_SIN,omc_lsc_product,0);

// FILTER MODULE
omc_lsc_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_X_COS,omc_lsc_product1,0);

// MULTIPLY
omc_ugf_product3 = omc_ugf_test1 * omc_ugf_osc[1];

// MULTIPLY
omc_ugf_product4 = omc_ugf_test1 * omc_ugf_osc[2];

// FILTER MODULE
omc_ugf_test2 = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_TEST2,omc_ugf_sum,0);

// DIVIDE
if(pLocalEpics->om1.OMC_MATH_DENOMMON != 0.0)
{
	omc_math_divide = pLocalEpics->om1.OMC_MATH_NUMERMON / pLocalEpics->om1.OMC_MATH_DENOMMON;
}
else{
	omc_math_divide = 0.0;
}

// PHASEDEG
omc_lsc_phase[0] = (omc_lsc_x_sin * pLocalEpics->om1.OMC_LSC_PHASE[1]) + (omc_lsc_x_cos * pLocalEpics->om1.OMC_LSC_PHASE[0]);
omc_lsc_phase[1] = (omc_lsc_x_cos * pLocalEpics->om1.OMC_LSC_PHASE[1]) - (omc_lsc_x_sin * pLocalEpics->om1.OMC_LSC_PHASE[0]);

// PHASEDEG
omc_ugf_test1_phase[0] = (omc_ugf_product3 * pLocalEpics->om1.OMC_UGF_TEST1_PHASE[1]) + (omc_ugf_product4 * pLocalEpics->om1.OMC_UGF_TEST1_PHASE[0]);
omc_ugf_test1_phase[1] = (omc_ugf_product4 * pLocalEpics->om1.OMC_UGF_TEST1_PHASE[1]) - (omc_ugf_product3 * pLocalEpics->om1.OMC_UGF_TEST1_PHASE[0]);

// MULTIPLY
omc_ugf_product2 = omc_ugf_test2 * omc_ugf_osc[1];

// MULTIPLY
omc_ugf_product5 = omc_ugf_test2 * omc_ugf_osc[2];

// FILTER MODULE
omc_math_divfilt = filterModuleD(dsp_ptr,dspCoeff,OMC_MATH_DIVFILT,omc_math_divide,0);

// FILTER MODULE
omc_lsc_i = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_I,omc_lsc_phase[0],0);

// FILTER MODULE
omc_lsc_q = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_Q,omc_lsc_phase[1],0);

// FILTER MODULE
omc_ugf_test1_i = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_TEST1_I,omc_ugf_test1_phase[0],0);

// FILTER MODULE
omc_ugf_test1_q = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_TEST1_Q,omc_ugf_test1_phase[1],0);

// PHASEDEG
omc_ugf_test2_phase[0] = (omc_ugf_product2 * pLocalEpics->om1.OMC_UGF_TEST2_PHASE[1]) + (omc_ugf_product5 * pLocalEpics->om1.OMC_UGF_TEST2_PHASE[0]);
omc_ugf_test2_phase[1] = (omc_ugf_product5 * pLocalEpics->om1.OMC_UGF_TEST2_PHASE[1]) - (omc_ugf_product2 * pLocalEpics->om1.OMC_UGF_TEST2_PHASE[0]);

// MULTIPLY
omc_lsc_product2 = omc_lsc_i * pLocalEpics->om1.OMC_LOCK_TRIGGER_LOCKMON;

// FILTER MODULE
omc_ugf_test2_i = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_TEST2_I,omc_ugf_test2_phase[0],0);

// FILTER MODULE
omc_ugf_test2_q = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_TEST2_Q,omc_ugf_test2_phase[1],0);

// FILTER MODULE
omc_lsc_gain = filterModuleD(dsp_ptr,dspCoeff,OMC_LSC_GAIN,omc_lsc_product2,0);

// DIVIDE
if(omc_ugf_test2_i != 0.0)
{
	omc_ugf_divide1 = omc_ugf_test1_i / omc_ugf_test2_i;
}
else{
	omc_ugf_divide1 = 0.0;
}

// FILTER MODULE
omc_pzt_lsc = filterModuleD(dsp_ptr,dspCoeff,OMC_PZT_LSC,omc_lsc_gain,0);

// MULTIPLY
omc_product = omc_lsc_gain * pLocalEpics->om1.OMC_HEATER_TRIGGER_LOCKMON;

// MULTIPLY
omc_ugf_product6 = pLocalEpics->om1.OMC_UGF_MONSCALE * omc_ugf_divide1;

// MUX
omc_ugf_mux1[0]= omc_ugf_divide1;
omc_ugf_mux1[1]= pLocalEpics->om1.OMC_UGF_IN_LO_ON;
omc_ugf_mux1[2]= pLocalEpics->om1.OMC_UGF_IN_HI_ON;
omc_ugf_mux1[3]= pLocalEpics->om1.OMC_UGF_IN_LO_DB;
omc_ugf_mux1[4]= pLocalEpics->om1.OMC_UGF_IN_HI_DB;

// SUM
omc_pzt_sum = omc_pzt_lsc + pLocalEpics->om1.OMC_PZT_BIAS;

// FILTER MODULE
omc_htr_lsc = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_LSC,omc_product,0);

// EpicsOut
pLocalEpics->om1.OMC_UGF_MON = omc_ugf_product6;

// Function Call
OMC_UGF_MAG2DB(omc_ugf_mux1, 5, omc_ugf_demux1, 1);

// EpicsOut
pLocalEpics->om1.OMC_PZT_OUTOUT = omc_pzt_sum;

// FILTER MODULE
omc_htr_drv = filterModuleD(dsp_ptr,dspCoeff,OMC_HTR_DRV,omc_htr_lsc,0);


// SUM
omc_ugf_sum2 = omc_ugf_demux1[0] - pLocalEpics->om1.OMC_UGF_SET;

// FILTER MODULE
omc_ugf_ctrl = filterModuleD(dsp_ptr,dspCoeff,OMC_UGF_CTRL,omc_ugf_sum2,0);

// MUX
omc_ugf_mux[0]= omc_ugf_ctrl;
omc_ugf_mux[1]= pLocalEpics->om1.OMC_UGF_OUT_LO_ON;
omc_ugf_mux[2]= pLocalEpics->om1.OMC_UGF_OUT_HI_ON;
omc_ugf_mux[3]= pLocalEpics->om1.OMC_UGF_OUT_LO_DB;
omc_ugf_mux[4]= pLocalEpics->om1.OMC_UGF_OUT_HI_DB;

// Function Call
OMC_UGF_DB2MAG(omc_ugf_mux, 5, omc_ugf_demux, 1);


// MULTIPLY
omc_ugf_product7 = pLocalEpics->om1.OMC_UGF_BIAS * omc_ugf_demux[0];

// EpicsOut
pLocalEpics->om1.OMC_UGF_SCALE = omc_ugf_product7;

// MULTIPLY
omc_ugf_product1 = omc_ugf_sum * pLocalEpics->om1.OMC_UGF_SCALE;

// EpicsOut
pLocalEpics->om1.OMC_UGF_OUT = omc_ugf_product1;

// FILTER MODULE
omc_readout = filterModuleD(dsp_ptr,dspCoeff,OMC_READOUT,pLocalEpics->om1.OMC_UGF_OUT,0);


//End of subsystem   OMC **************************************************


if (cdsPciModules.pci_rfm[0] != 0) {
  // RFM output
  *((double *)(((char *)cdsPciModules.pci_rfm[0]) + 0x12200cc)) = omc_readout;
}

ipc_at_0x2000 = omc_qpd1_p;

ipc_at_0x2008 = omc_qpd2_p;

ipc_at_0x2010 = omc_qpd1_y;

ipc_at_0x2018 = omc_qpd2_y;

ipc_at_0x2060 = omc_qpd3_p;

ipc_at_0x2068 = omc_qpd3_y;

ipc_at_0x2070 = omc_qpd4_p;

ipc_at_0x2078 = omc_qpd4_y;

ipc_at_0x20A0 = omc_pd_divide;

ipc_at_0x20A8 = pLocalEpics->om1.OMC_PD_CTRL_WHITENING_CTRL_MON;

ipc_at_0x20B0 = omc_lsc_osc[2];

ipc_at_0x20B8 = omc_lsc_osc[1];

ipc_at_0x20D0 = omc_pd_sum;

ipc_at_0x20D8 = omc_qpd1_sum;

ipc_at_0x20E0 = omc_qpd2_sum;

ipc_at_0x20E8 = omc_qpd3_sum;

ipc_at_0x20F0 = omc_qpd4_sum;

if (_ipc_shm != 0) {
  // IPCS output
  *((float *)(((char *)_ipc_shm) + 0x3000)) = (cycle + 1)%FE_RATE;
}

// DAC number is 0
dacOut[0][0] = ipc_at_0x2020;
dacOut[0][1] = ipc_at_0x2028;
dacOut[0][2] = ipc_at_0x2030;
dacOut[0][3] = ipc_at_0x2038;
dacOut[0][4] = ipc_at_0x2040;
dacOut[0][5] = ipc_at_0x2048;
dacOut[0][6] = ipc_at_0x2050;
dacOut[0][7] = ipc_at_0x2058;
dacOut[0][8] = omc_htr_drv;
dacOut[0][9] = omc_dac0_test10;
dacOut[0][10] = omc_dac0_test1;
dacOut[0][11] = omc_dac0_test2;
dacOut[0][12] = omc_pzt_dither;
dacOut[0][13] = pLocalEpics->om1.OMC_PZT_OUTOUT;
dacOut[0][14] = omc_dac0_test3;
dacOut[0][15] = omc_dac0_test4;


    // All IPC outputs
    if (_ipc_shm != 0) {
      *((double *)(((char *)_ipc_shm) + 0x2080)) = ipc_at_0x2080;
      *((double *)(((char *)_ipc_shm) + 0x2088)) = ipc_at_0x2088;
      *((double *)(((char *)_ipc_shm) + 0x2090)) = ipc_at_0x2090;
      *((double *)(((char *)_ipc_shm) + 0x2000)) = ipc_at_0x2000;
      *((double *)(((char *)_ipc_shm) + 0x2008)) = ipc_at_0x2008;
      *((double *)(((char *)_ipc_shm) + 0x2010)) = ipc_at_0x2010;
      *((double *)(((char *)_ipc_shm) + 0x2018)) = ipc_at_0x2018;
      *((double *)(((char *)_ipc_shm) + 0x2060)) = ipc_at_0x2060;
      *((double *)(((char *)_ipc_shm) + 0x2068)) = ipc_at_0x2068;
      *((double *)(((char *)_ipc_shm) + 0x2070)) = ipc_at_0x2070;
      *((double *)(((char *)_ipc_shm) + 0x2078)) = ipc_at_0x2078;
      *((double *)(((char *)_ipc_shm) + 0x20A0)) = ipc_at_0x20A0;
      *((double *)(((char *)_ipc_shm) + 0x20A8)) = ipc_at_0x20A8;
      *((double *)(((char *)_ipc_shm) + 0x20B0)) = ipc_at_0x20B0;
      *((double *)(((char *)_ipc_shm) + 0x20B8)) = ipc_at_0x20B8;
      *((double *)(((char *)_ipc_shm) + 0x20D0)) = ipc_at_0x20D0;
      *((double *)(((char *)_ipc_shm) + 0x20D8)) = ipc_at_0x20D8;
      *((double *)(((char *)_ipc_shm) + 0x20E0)) = ipc_at_0x20E0;
      *((double *)(((char *)_ipc_shm) + 0x20E8)) = ipc_at_0x20E8;
      *((double *)(((char *)_ipc_shm) + 0x20F0)) = ipc_at_0x20F0;
    }
    //usleep(3);
    for (;*((volatile unsigned int *)(((char *)_ipc_shm) + 0x3010)) == 0;);
    *((volatile unsigned int *)(((char *)_ipc_shm) + 0x3010)) = 0;
  }
}

