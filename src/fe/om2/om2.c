// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


#ifdef SERVO64K
	#define FE_RATE	65536
#endif
#ifdef SERVO32K
	#define FE_RATE	32768
#endif
#ifdef SERVO16K
	#define FE_RATE	16384
#endif
#ifdef SERVO4K
	#define FE_RATE	4096
#endif
#ifdef SERVO2K
	#define FE_RATE	2048
#endif


/* Hardware configuration */
CDS_CARDS cards_used[] = {
	{GSC_16AI64SSA,0},
	{ACS_8DIO,0},
};

void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		double dacOut[][16],	/* DAC outputs */
		FILT_MOD *dsp_ptr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii;

double ipc_at_0x2000 = *((double *)(((void *)_ipc_shm) + 0x2000));
double ipc_at_0x2008 = *((double *)(((void *)_ipc_shm) + 0x2008));
double ipc_at_0x2010 = *((double *)(((void *)_ipc_shm) + 0x2010));
double ipc_at_0x2018 = *((double *)(((void *)_ipc_shm) + 0x2018));
double ipc_at_0x2020;
double ipc_at_0x2028;
double ipc_at_0x2030;
double ipc_at_0x2038;
double ipc_at_0x2040;
double ipc_at_0x2048;
double ipc_at_0x2050;
double ipc_at_0x2058;
double ipc_at_0x2060 = *((double *)(((void *)_ipc_shm) + 0x2060));
double ipc_at_0x2068 = *((double *)(((void *)_ipc_shm) + 0x2068));
double ipc_at_0x2070 = *((double *)(((void *)_ipc_shm) + 0x2070));
double ipc_at_0x2078 = *((double *)(((void *)_ipc_shm) + 0x2078));
double ipc_at_0x2080 = *((double *)(((void *)_ipc_shm) + 0x2080));
double ipc_at_0x2088 = *((double *)(((void *)_ipc_shm) + 0x2088));
double ipc_at_0x2090 = *((double *)(((void *)_ipc_shm) + 0x2090));
double ipc_at_0x20A0 = *((double *)(((void *)_ipc_shm) + 0x20A0));
double ipc_at_0x20A8 = *((double *)(((void *)_ipc_shm) + 0x20A8));
double ipc_at_0x20B0 = *((double *)(((void *)_ipc_shm) + 0x20B0));
double ipc_at_0x20B8 = *((double *)(((void *)_ipc_shm) + 0x20B8));
double ipc_at_0x20C0;
double ipc_at_0x20C8 = *((double *)(((void *)_ipc_shm) + 0x20C8));
double ipc_at_0x20D0 = *((double *)(((void *)_ipc_shm) + 0x20D0));
double omc_tt2_yaw;
static int omc_tt2_wd;
static float omc_tt2_wd_avg[5];
static float omc_tt2_wd_var[5];
float omc_tt2_wd_vabs;
double omc_tt2_ur_sen;
double omc_tt2_ur_mon;
double omc_tt2_ur_coil;
double omc_tt2_ul_sen;
double omc_tt2_ul_mon;
double omc_tt2_ul_coil;
double omc_tt2_sum2;
double omc_tt2_sum1;
double omc_tt2_sum;
double omc_tt2_susyaw;
double omc_tt2_suspos;
double omc_tt2_suspit;
float omc_tt2_rms3;
static float omc_tt2_rms3_avg;
float omc_tt2_rms2;
static float omc_tt2_rms2_avg;
float omc_tt2_rms1;
static float omc_tt2_rms1_avg;
float omc_tt2_rms;
static float omc_tt2_rms_avg;
double omc_tt2_product7;
double omc_tt2_product6;
double omc_tt2_product5;
double omc_tt2_product4;
double omc_tt2_product3;
double omc_tt2_product2;
double omc_tt2_product1;
double omc_tt2_product;
double omc_tt2_pit;
double omc_tt2_outmtrx[4][4];
double omc_tt2_lsc;
double omc_tt2_lr_sen;
double omc_tt2_lr_mon;
double omc_tt2_lr_coil;
double omc_tt2_ll_sen;
double omc_tt2_ll_mon;
double omc_tt2_ll_coil;
double omc_tt2_inmtrx[3][4];
static float omc_tt2_ground5;
static float omc_tt2_ground4;
static float omc_tt2_ground3;
static float omc_tt2_ground2;
static float omc_tt2_ground1;
static float omc_tt2_ground;
double omc_tt2_divide;
static double omc_tt2_constant;
static double omc_tt2_butt_osc[3];
static double omc_tt2_butt_osc_freq;
static double omc_tt2_butt_osc_delta;
static double omc_tt2_butt_osc_alpha;
static double omc_tt2_butt_osc_beta;
static double omc_tt2_butt_osc_cos_prev;
static double omc_tt2_butt_osc_sin_prev;
static double omc_tt2_butt_osc_cos_new;
static double omc_tt2_butt_osc_sin_new;
double lsinx, lcosx, valx;
double omc_tt1_yaw;
static int omc_tt1_wd;
static float omc_tt1_wd_avg[5];
static float omc_tt1_wd_var[5];
float omc_tt1_wd_vabs;
double omc_tt1_ur_sen;
double omc_tt1_ur_mon;
double omc_tt1_ur_coil;
double omc_tt1_ul_sen;
double omc_tt1_ul_mon;
double omc_tt1_ul_coil;
double omc_tt1_sum2;
double omc_tt1_sum1;
double omc_tt1_sum;
double omc_tt1_susyaw;
double omc_tt1_suspos;
double omc_tt1_suspit;
float omc_tt1_rms3;
static float omc_tt1_rms3_avg;
float omc_tt1_rms2;
static float omc_tt1_rms2_avg;
float omc_tt1_rms1;
static float omc_tt1_rms1_avg;
float omc_tt1_rms;
static float omc_tt1_rms_avg;
double omc_tt1_product7;
double omc_tt1_product6;
double omc_tt1_product5;
double omc_tt1_product4;
double omc_tt1_product3;
double omc_tt1_product2;
double omc_tt1_product1;
double omc_tt1_product;
double omc_tt1_pit;
double omc_tt1_outmtrx[4][4];
double omc_tt1_offsetramp[8];
float OMC_TT1_OFFSETRAMP_CALC;
double omc_tt1_lsc;
double omc_tt1_lr_sen;
double omc_tt1_lr_mon;
double omc_tt1_lr_coil;
double omc_tt1_ll_sen;
double omc_tt1_ll_mon;
double omc_tt1_ll_coil;
double omc_tt1_inmtrx[3][4];
static float omc_tt1_ground5;
static float omc_tt1_ground4;
static float omc_tt1_ground3;
static float omc_tt1_ground2;
static float omc_tt1_ground1;
static float omc_tt1_ground;
static float omc_tt1_g1;
static float omc_tt1_g;
double omc_tt1_divide;
static double omc_tt1_constant;
static double omc_tt1_butt_osc[3];
static double omc_tt1_butt_osc_freq;
static double omc_tt1_butt_osc_delta;
static double omc_tt1_butt_osc_alpha;
static double omc_tt1_butt_osc_beta;
static double omc_tt1_butt_osc_cos_prev;
static double omc_tt1_butt_osc_sin_prev;
static double omc_tt1_butt_osc_cos_new;
static double omc_tt1_butt_osc_sin_new;
static double omc_shutter_mspercycle;
double omc_shutter_trigger_in;
static int omc_shutter_trigger;
#include "OMC_SHUTTER_SWTRIG.c"
double omc_shutter_resp;
double omc_shutter_mux1[7];
double omc_shutter_hwtrig;
double omc_shutter_hwstate;
double omc_shutter_hwsetpt;
static float omc_shutter_ground4;
static float omc_shutter_ground3;
static float omc_shutter_ground2;
static float omc_shutter_ground1;
double omc_shutter_gainur;
double omc_shutter_gainul;
double omc_shutter_gainlr;
double omc_shutter_gainll;
double omc_shutter_demux1[1];
double omc_shutter_choice4;
double omc_shutter_choice3;
double omc_shutter_choice2;
double omc_shutter_choice1;
double omc_om2_adc_0_15;
double omc_om2_adc_0_14;
double omc_om2_adc_0_13;
double omc_om2_adc_0_12;
double omc_om2_adc_0_11;
double omc_asc_y2_x_sin;
double omc_asc_y2_x_cos;
double omc_asc_y2_sig;
double omc_asc_y2_q;
static double omc_asc_y2_phase[2];
static double omc_asc_y2_osc[3];
static double omc_asc_y2_osc_freq;
static double omc_asc_y2_osc_delta;
static double omc_asc_y2_osc_alpha;
static double omc_asc_y2_osc_beta;
static double omc_asc_y2_osc_cos_prev;
static double omc_asc_y2_osc_sin_prev;
static double omc_asc_y2_osc_cos_new;
static double omc_asc_y2_osc_sin_new;
double omc_asc_y2_i;
double omc_asc_y2_clock;
double omc_asc_y1_x_sin;
double omc_asc_y1_x_cos;
double omc_asc_y1_sig;
double omc_asc_y1_q;
static double omc_asc_y1_phase[2];
static double omc_asc_y1_osc[3];
static double omc_asc_y1_osc_freq;
static double omc_asc_y1_osc_delta;
static double omc_asc_y1_osc_alpha;
static double omc_asc_y1_osc_beta;
static double omc_asc_y1_osc_cos_prev;
static double omc_asc_y1_osc_sin_prev;
static double omc_asc_y1_osc_cos_new;
static double omc_asc_y1_osc_sin_new;
double omc_asc_y1_i;
double omc_asc_y1_clock;
double omc_asc_waist_yaw;
double omc_asc_waist_y;
double omc_asc_waist_x;
double omc_asc_waist_pit;
double omc_asc_waistbasis[4];
double omc_asc_sum9;
double omc_asc_sum8;
double omc_asc_sum3;
double omc_asc_sum2;
double omc_asc_sum12;
double omc_asc_sum11;
double omc_asc_sum10;
double omc_asc_sum1;
double omc_asc_rpos_y;
double omc_asc_rpos_x;
double omc_asc_rdasc[4][4];
double omc_asc_rdagain[8];
float OMC_ASC_RDAGAIN_CALC;
double omc_asc_rang_y;
double omc_asc_rang_x;
double omc_asc_qpos_y;
double omc_asc_qpos_x;
double omc_asc_qpdgain[8];
float OMC_ASC_QPDGAIN_CALC;
double omc_asc_qpdasc[4][4];
double omc_asc_qpd2_y_sig;
double omc_asc_qpd2_y_s;
double omc_asc_qpd2_y_q;
static double omc_asc_qpd2_y_phase[2];
double omc_asc_qpd2_y_i;
double omc_asc_qpd2_y_c;
double omc_asc_qpd2_p_sig;
double omc_asc_qpd2_p_s;
double omc_asc_qpd2_p_q;
static double omc_asc_qpd2_p_phase[2];
double omc_asc_qpd2_p_i;
double omc_asc_qpd2_p_c;
double omc_asc_qpd1_y_sig;
double omc_asc_qpd1_y_s;
double omc_asc_qpd1_y_q;
static double omc_asc_qpd1_y_phase[2];
double omc_asc_qpd1_y_i;
double omc_asc_qpd1_y_c;
double omc_asc_qpd1_p_sig;
double omc_asc_qpd1_p_s;
double omc_asc_qpd1_p_q;
static double omc_asc_qpd1_p_phase[2];
double omc_asc_qpd1_p_i;
double omc_asc_qpd1_p_c;
double omc_asc_qang_y;
double omc_asc_qang_x;
double omc_asc_product9;
double omc_asc_product8;
double omc_asc_product7;
double omc_asc_product6;
double omc_asc_product5;
double omc_asc_product4;
double omc_asc_product3;
double omc_asc_product2;
double omc_asc_product15;
double omc_asc_product14;
double omc_asc_product13;
double omc_asc_product12;
double omc_asc_product11;
double omc_asc_product10;
double omc_asc_product1;
double omc_asc_product;
double omc_asc_pos_y_mag;
double omc_asc_pos_y;
double omc_asc_pos_x_mag;
double omc_asc_pos_x;
double omc_asc_p2_x_sin;
double omc_asc_p2_x_cos;
double omc_asc_p2_sig;
double omc_asc_p2_q;
static double omc_asc_p2_phase[2];
static double omc_asc_p2_osc[3];
static double omc_asc_p2_osc_freq;
static double omc_asc_p2_osc_delta;
static double omc_asc_p2_osc_alpha;
static double omc_asc_p2_osc_beta;
static double omc_asc_p2_osc_cos_prev;
static double omc_asc_p2_osc_sin_prev;
static double omc_asc_p2_osc_cos_new;
static double omc_asc_p2_osc_sin_new;
double omc_asc_p2_i;
double omc_asc_p2_clock;
double omc_asc_p1_x_sin;
double omc_asc_p1_x_cos;
double omc_asc_p1_sig;
double omc_asc_p1_q;
static double omc_asc_p1_phase[2];
static double omc_asc_p1_osc[3];
static double omc_asc_p1_osc_freq;
static double omc_asc_p1_osc_delta;
static double omc_asc_p1_osc_alpha;
static double omc_asc_p1_osc_beta;
static double omc_asc_p1_osc_cos_prev;
static double omc_asc_p1_osc_sin_prev;
static double omc_asc_p1_osc_cos_new;
static double omc_asc_p1_osc_sin_new;
double omc_asc_p1_i;
double omc_asc_p1_clock;
double omc_asc_mux[4];
double omc_asc_mastergain[8];
float OMC_ASC_MASTERGAIN_CALC;
double omc_asc_magz;
double omc_asc_magy;
double omc_asc_magx;
double omc_asc_magff_mat[4][3];
static float omc_asc_ground8;
static float omc_asc_ground3;
static float omc_asc_ground2;
static float omc_asc_ground1;
static float omc_asc_ground;
double omc_asc_exmtx[4][8];
double omc_asc_demux[4];
double omc_asc_dpos_y;
double omc_asc_dpos_x;
double omc_asc_dithergain[8];
float OMC_ASC_DITHERGAIN_CALC;
double omc_asc_dang_y;
double omc_asc_dang_x;
double omc_asc_ang_y_mag;
double omc_asc_ang_y;
double omc_asc_ang_x_mag;
double omc_asc_ang_x;
double omc_asc_acmtx[4][8];
double omc_asc_drum_fpm_coscos;
double omc_asc_drum_fpm_cp;
double omc_asc_drum_fpm_choicecos;
double omc_asc_drum_fpm_choicesin;
static float omc_asc_drum_fpm_ground;
static double omc_asc_drum_fpm_p[2];
double omc_asc_drum_fpm_sincos;
double omc_asc_drum_fpm_sp;
double omc_asc_p1_fpm_coscos;
double omc_asc_p1_fpm_cossin;
double omc_asc_p1_fpm_choicecos;
double omc_asc_p1_fpm_choicepmc;
double omc_asc_p1_fpm_choicepms;
double omc_asc_p1_fpm_choicesin;
static float omc_asc_p1_fpm_ground;
static double omc_asc_p1_fpm_p[2];
double omc_asc_p1_fpm_sincos;
double omc_asc_p1_fpm_sinsin;
double omc_asc_p1_fpm_sum;
double omc_asc_p1_fpm_sum1;
double omc_asc_p1_fpm_sum2;
double omc_asc_p1_fpm_sum3;
double omc_asc_p2_fpm_coscos;
double omc_asc_p2_fpm_cossin;
double omc_asc_p2_fpm_choicecos;
double omc_asc_p2_fpm_choicepmc;
double omc_asc_p2_fpm_choicepms;
double omc_asc_p2_fpm_choicesin;
static float omc_asc_p2_fpm_ground;
static double omc_asc_p2_fpm_p[2];
double omc_asc_p2_fpm_sincos;
double omc_asc_p2_fpm_sinsin;
double omc_asc_p2_fpm_sum;
double omc_asc_p2_fpm_sum1;
double omc_asc_p2_fpm_sum2;
double omc_asc_p2_fpm_sum3;
double omc_asc_rda_fpm_coscos;
double omc_asc_rda_fpm_cossin;
double omc_asc_rda_fpm_choicecos;
double omc_asc_rda_fpm_choicepmc;
double omc_asc_rda_fpm_choicepms;
double omc_asc_rda_fpm_choicesin;
static float omc_asc_rda_fpm_ground;
static double omc_asc_rda_fpm_p[2];
double omc_asc_rda_fpm_sincos;
double omc_asc_rda_fpm_sinsin;
double omc_asc_rda_fpm_sum;
double omc_asc_rda_fpm_sum1;
double omc_asc_rda_fpm_sum2;
double omc_asc_rda_fpm_sum3;
double omc_asc_y1_fpm_coscos;
double omc_asc_y1_fpm_cossin;
double omc_asc_y1_fpm_choicecos;
double omc_asc_y1_fpm_choicepmc;
double omc_asc_y1_fpm_choicepms;
double omc_asc_y1_fpm_choicesin;
static float omc_asc_y1_fpm_ground;
static double omc_asc_y1_fpm_p[2];
double omc_asc_y1_fpm_sincos;
double omc_asc_y1_fpm_sinsin;
double omc_asc_y1_fpm_sum;
double omc_asc_y1_fpm_sum1;
double omc_asc_y1_fpm_sum2;
double omc_asc_y1_fpm_sum3;
double omc_asc_y2_fpm_coscos;
double omc_asc_y2_fpm_cossin;
double omc_asc_y2_fpm_choicecos;
double omc_asc_y2_fpm_choicepmc;
double omc_asc_y2_fpm_choicepms;
double omc_asc_y2_fpm_choicesin;
static float omc_asc_y2_fpm_ground;
static double omc_asc_y2_fpm_p[2];
double omc_asc_y2_fpm_sincos;
double omc_asc_y2_fpm_sinsin;
double omc_asc_y2_fpm_sum;
double omc_asc_y2_fpm_sum1;
double omc_asc_y2_fpm_sum2;
double omc_asc_y2_fpm_sum3;
static float omc_ground1;
static float omc_ground4;
double omc_sync_test;


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
dWordUsed[0][19] =  1;
dWordUsed[0][20] =  1;
dWordUsed[0][21] =  1;
dWordUsed[0][22] =  1;
dWordUsed[0][23] =  1;
dWordUsed[0][24] =  1;
dWordUsed[0][25] =  1;
dWordUsed[0][26] =  1;
dWordUsed[0][27] =  1;
dWordUsed[0][28] =  1;
dWordUsed[0][29] =  1;
dWordUsed[0][30] =  1;
dWordUsed[0][31] =  1;
rioReadOps[0] = 0;
omc_tt2_wd = 0;
for (ii=0; ii<5; ii++) {
	omc_tt2_wd_avg[ii] = 0.0;
	omc_tt2_wd_var[ii] = 0.0;
}
pLocalEpics->om2.OMC_TT2_WD = 1;
omc_tt2_rms3_avg = 0.0;
omc_tt2_rms2_avg = 0.0;
omc_tt2_rms1_avg = 0.0;
omc_tt2_rms_avg = 0.0;
omc_tt2_ground5 = 0.0;
omc_tt2_ground4 = 0.0;
omc_tt2_ground3 = 0.0;
omc_tt2_ground2 = 0.0;
omc_tt2_ground1 = 0.0;
omc_tt2_ground = 0.0;
omc_tt2_constant = (double)1;
omc_tt2_butt_osc_freq = pLocalEpics->om2.OMC_TT2_BUTT_OSC_FREQ;
omc_tt2_butt_osc_delta = 2.0 * 3.1415926535897932384626 * omc_tt2_butt_osc_freq / FE_RATE;
valx = omc_tt2_butt_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_tt2_butt_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_tt2_butt_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_tt2_butt_osc_beta = lsinx;
omc_tt2_butt_osc_cos_prev = 1.0;
omc_tt2_butt_osc_sin_prev = 0.0;
omc_tt1_wd = 0;
for (ii=0; ii<5; ii++) {
	omc_tt1_wd_avg[ii] = 0.0;
	omc_tt1_wd_var[ii] = 0.0;
}
pLocalEpics->om2.OMC_TT1_WD = 1;
omc_tt1_rms3_avg = 0.0;
omc_tt1_rms2_avg = 0.0;
omc_tt1_rms1_avg = 0.0;
omc_tt1_rms_avg = 0.0;
omc_tt1_ground5 = 0.0;
omc_tt1_ground4 = 0.0;
omc_tt1_ground3 = 0.0;
omc_tt1_ground2 = 0.0;
omc_tt1_ground1 = 0.0;
omc_tt1_ground = 0.0;
omc_tt1_g1 = 0.0;
omc_tt1_g = 0.0;
omc_tt1_constant = (double)1;
omc_tt1_butt_osc_freq = pLocalEpics->om2.OMC_TT1_BUTT_OSC_FREQ;
omc_tt1_butt_osc_delta = 2.0 * 3.1415926535897932384626 * omc_tt1_butt_osc_freq / FE_RATE;
valx = omc_tt1_butt_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_tt1_butt_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_tt1_butt_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_tt1_butt_osc_beta = lsinx;
omc_tt1_butt_osc_cos_prev = 1.0;
omc_tt1_butt_osc_sin_prev = 0.0;
omc_shutter_mspercycle = (double)0.030518;
omc_shutter_ground4 = 0.0;
omc_shutter_ground3 = 0.0;
omc_shutter_ground2 = 0.0;
omc_shutter_ground1 = 0.0;
omc_asc_y2_osc_freq = pLocalEpics->om2.OMC_ASC_Y2_OSC_FREQ;
omc_asc_y2_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_y2_osc_freq / FE_RATE;
valx = omc_asc_y2_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_asc_y2_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_asc_y2_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_asc_y2_osc_beta = lsinx;
omc_asc_y2_osc_cos_prev = 1.0;
omc_asc_y2_osc_sin_prev = 0.0;
omc_asc_y1_osc_freq = pLocalEpics->om2.OMC_ASC_Y1_OSC_FREQ;
omc_asc_y1_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_y1_osc_freq / FE_RATE;
valx = omc_asc_y1_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_asc_y1_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_asc_y1_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_asc_y1_osc_beta = lsinx;
omc_asc_y1_osc_cos_prev = 1.0;
omc_asc_y1_osc_sin_prev = 0.0;
omc_asc_p2_osc_freq = pLocalEpics->om2.OMC_ASC_P2_OSC_FREQ;
omc_asc_p2_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_p2_osc_freq / FE_RATE;
valx = omc_asc_p2_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_asc_p2_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_asc_p2_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_asc_p2_osc_beta = lsinx;
omc_asc_p2_osc_cos_prev = 1.0;
omc_asc_p2_osc_sin_prev = 0.0;
omc_asc_p1_osc_freq = pLocalEpics->om2.OMC_ASC_P1_OSC_FREQ;
omc_asc_p1_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_p1_osc_freq / FE_RATE;
valx = omc_asc_p1_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
omc_asc_p1_osc_alpha = 2.0 * lsinx * lsinx;
valx = omc_asc_p1_osc_delta;
sincos(valx, &lsinx, &lcosx);
omc_asc_p1_osc_beta = lsinx;
omc_asc_p1_osc_cos_prev = 1.0;
omc_asc_p1_osc_sin_prev = 0.0;
omc_asc_ground8 = 0.0;
omc_asc_ground3 = 0.0;
omc_asc_ground2 = 0.0;
omc_asc_ground1 = 0.0;
omc_asc_ground = 0.0;
omc_asc_drum_fpm_ground = 0.0;
omc_asc_p1_fpm_ground = 0.0;
omc_asc_p2_fpm_ground = 0.0;
omc_asc_rda_fpm_ground = 0.0;
omc_asc_y1_fpm_ground = 0.0;
omc_asc_y2_fpm_ground = 0.0;
omc_ground1 = 0.0;
omc_ground4 = 0.0;
} else {
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// Rio number is 0 name DIO_0
rioOutput[0] = ipc_at_0x20A8;


//Start of subsystem OMC **************************************************

// Osc
omc_tt2_butt_osc_cos_new = (1.0 - omc_tt2_butt_osc_alpha) * omc_tt2_butt_osc_cos_prev - omc_tt2_butt_osc_beta * omc_tt2_butt_osc_sin_prev;
omc_tt2_butt_osc_sin_new = (1.0 - omc_tt2_butt_osc_alpha) * omc_tt2_butt_osc_sin_prev + omc_tt2_butt_osc_beta * omc_tt2_butt_osc_cos_prev;
omc_tt2_butt_osc_sin_prev = omc_tt2_butt_osc_sin_new;
omc_tt2_butt_osc_cos_prev = omc_tt2_butt_osc_cos_new;
omc_tt2_butt_osc[0] = omc_tt2_butt_osc_sin_new * pLocalEpics->om2.OMC_TT2_BUTT_OSC_CLKGAIN;
omc_tt2_butt_osc[1] = omc_tt2_butt_osc_sin_new * pLocalEpics->om2.OMC_TT2_BUTT_OSC_SINGAIN;
omc_tt2_butt_osc[2] = omc_tt2_butt_osc_cos_new * pLocalEpics->om2.OMC_TT2_BUTT_OSC_COSGAIN;
if((omc_tt2_butt_osc_freq != pLocalEpics->om2.OMC_TT2_BUTT_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_tt2_butt_osc_freq = pLocalEpics->om2.OMC_TT2_BUTT_OSC_FREQ;
	omc_tt2_butt_osc_delta = 2.0 * 3.1415926535897932384626 * omc_tt2_butt_osc_freq / FE_RATE;
	valx = omc_tt2_butt_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_tt2_butt_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_tt2_butt_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_tt2_butt_osc_beta = lsinx;
	omc_tt2_butt_osc_cos_prev = 1.0;
	omc_tt2_butt_osc_sin_prev = 0.0;
}

// DIVIDE
if(pLocalEpics->om2.OMC_TT2_ADC_CONVERTER != 0.0)
{
	omc_tt2_divide = omc_tt2_constant / pLocalEpics->om2.OMC_TT2_ADC_CONVERTER;
}
else{
	omc_tt2_divide = 0.0;
}

// Osc
omc_tt1_butt_osc_cos_new = (1.0 - omc_tt1_butt_osc_alpha) * omc_tt1_butt_osc_cos_prev - omc_tt1_butt_osc_beta * omc_tt1_butt_osc_sin_prev;
omc_tt1_butt_osc_sin_new = (1.0 - omc_tt1_butt_osc_alpha) * omc_tt1_butt_osc_sin_prev + omc_tt1_butt_osc_beta * omc_tt1_butt_osc_cos_prev;
omc_tt1_butt_osc_sin_prev = omc_tt1_butt_osc_sin_new;
omc_tt1_butt_osc_cos_prev = omc_tt1_butt_osc_cos_new;
omc_tt1_butt_osc[0] = omc_tt1_butt_osc_sin_new * pLocalEpics->om2.OMC_TT1_BUTT_OSC_CLKGAIN;
omc_tt1_butt_osc[1] = omc_tt1_butt_osc_sin_new * pLocalEpics->om2.OMC_TT1_BUTT_OSC_SINGAIN;
omc_tt1_butt_osc[2] = omc_tt1_butt_osc_cos_new * pLocalEpics->om2.OMC_TT1_BUTT_OSC_COSGAIN;
if((omc_tt1_butt_osc_freq != pLocalEpics->om2.OMC_TT1_BUTT_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_tt1_butt_osc_freq = pLocalEpics->om2.OMC_TT1_BUTT_OSC_FREQ;
	omc_tt1_butt_osc_delta = 2.0 * 3.1415926535897932384626 * omc_tt1_butt_osc_freq / FE_RATE;
	valx = omc_tt1_butt_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_tt1_butt_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_tt1_butt_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_tt1_butt_osc_beta = lsinx;
	omc_tt1_butt_osc_cos_prev = 1.0;
	omc_tt1_butt_osc_sin_prev = 0.0;
}


// DIVIDE
if(pLocalEpics->om2.OMC_TT1_ADC_CONVERTER != 0.0)
{
	omc_tt1_divide = omc_tt1_constant / pLocalEpics->om2.OMC_TT1_ADC_CONVERTER;
}
else{
	omc_tt1_divide = 0.0;
}

// Osc
omc_asc_y2_osc_cos_new = (1.0 - omc_asc_y2_osc_alpha) * omc_asc_y2_osc_cos_prev - omc_asc_y2_osc_beta * omc_asc_y2_osc_sin_prev;
omc_asc_y2_osc_sin_new = (1.0 - omc_asc_y2_osc_alpha) * omc_asc_y2_osc_sin_prev + omc_asc_y2_osc_beta * omc_asc_y2_osc_cos_prev;
omc_asc_y2_osc_sin_prev = omc_asc_y2_osc_sin_new;
omc_asc_y2_osc_cos_prev = omc_asc_y2_osc_cos_new;
omc_asc_y2_osc[0] = omc_asc_y2_osc_sin_new * pLocalEpics->om2.OMC_ASC_Y2_OSC_CLKGAIN;
omc_asc_y2_osc[1] = omc_asc_y2_osc_sin_new * pLocalEpics->om2.OMC_ASC_Y2_OSC_SINGAIN;
omc_asc_y2_osc[2] = omc_asc_y2_osc_cos_new * pLocalEpics->om2.OMC_ASC_Y2_OSC_COSGAIN;
if((omc_asc_y2_osc_freq != pLocalEpics->om2.OMC_ASC_Y2_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_asc_y2_osc_freq = pLocalEpics->om2.OMC_ASC_Y2_OSC_FREQ;
	omc_asc_y2_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_y2_osc_freq / FE_RATE;
	valx = omc_asc_y2_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_y2_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_asc_y2_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_y2_osc_beta = lsinx;
	omc_asc_y2_osc_cos_prev = 1.0;
	omc_asc_y2_osc_sin_prev = 0.0;
}

// Osc
omc_asc_p2_osc_cos_new = (1.0 - omc_asc_p2_osc_alpha) * omc_asc_p2_osc_cos_prev - omc_asc_p2_osc_beta * omc_asc_p2_osc_sin_prev;
omc_asc_p2_osc_sin_new = (1.0 - omc_asc_p2_osc_alpha) * omc_asc_p2_osc_sin_prev + omc_asc_p2_osc_beta * omc_asc_p2_osc_cos_prev;
omc_asc_p2_osc_sin_prev = omc_asc_p2_osc_sin_new;
omc_asc_p2_osc_cos_prev = omc_asc_p2_osc_cos_new;
omc_asc_p2_osc[0] = omc_asc_p2_osc_sin_new * pLocalEpics->om2.OMC_ASC_P2_OSC_CLKGAIN;
omc_asc_p2_osc[1] = omc_asc_p2_osc_sin_new * pLocalEpics->om2.OMC_ASC_P2_OSC_SINGAIN;
omc_asc_p2_osc[2] = omc_asc_p2_osc_cos_new * pLocalEpics->om2.OMC_ASC_P2_OSC_COSGAIN;
if((omc_asc_p2_osc_freq != pLocalEpics->om2.OMC_ASC_P2_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_asc_p2_osc_freq = pLocalEpics->om2.OMC_ASC_P2_OSC_FREQ;
	omc_asc_p2_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_p2_osc_freq / FE_RATE;
	valx = omc_asc_p2_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_p2_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_asc_p2_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_p2_osc_beta = lsinx;
	omc_asc_p2_osc_cos_prev = 1.0;
	omc_asc_p2_osc_sin_prev = 0.0;
}

// Osc
omc_asc_y1_osc_cos_new = (1.0 - omc_asc_y1_osc_alpha) * omc_asc_y1_osc_cos_prev - omc_asc_y1_osc_beta * omc_asc_y1_osc_sin_prev;
omc_asc_y1_osc_sin_new = (1.0 - omc_asc_y1_osc_alpha) * omc_asc_y1_osc_sin_prev + omc_asc_y1_osc_beta * omc_asc_y1_osc_cos_prev;
omc_asc_y1_osc_sin_prev = omc_asc_y1_osc_sin_new;
omc_asc_y1_osc_cos_prev = omc_asc_y1_osc_cos_new;
omc_asc_y1_osc[0] = omc_asc_y1_osc_sin_new * pLocalEpics->om2.OMC_ASC_Y1_OSC_CLKGAIN;
omc_asc_y1_osc[1] = omc_asc_y1_osc_sin_new * pLocalEpics->om2.OMC_ASC_Y1_OSC_SINGAIN;
omc_asc_y1_osc[2] = omc_asc_y1_osc_cos_new * pLocalEpics->om2.OMC_ASC_Y1_OSC_COSGAIN;
if((omc_asc_y1_osc_freq != pLocalEpics->om2.OMC_ASC_Y1_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_asc_y1_osc_freq = pLocalEpics->om2.OMC_ASC_Y1_OSC_FREQ;
	omc_asc_y1_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_y1_osc_freq / FE_RATE;
	valx = omc_asc_y1_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_y1_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_asc_y1_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_y1_osc_beta = lsinx;
	omc_asc_y1_osc_cos_prev = 1.0;
	omc_asc_y1_osc_sin_prev = 0.0;
}

// Osc
omc_asc_p1_osc_cos_new = (1.0 - omc_asc_p1_osc_alpha) * omc_asc_p1_osc_cos_prev - omc_asc_p1_osc_beta * omc_asc_p1_osc_sin_prev;
omc_asc_p1_osc_sin_new = (1.0 - omc_asc_p1_osc_alpha) * omc_asc_p1_osc_sin_prev + omc_asc_p1_osc_beta * omc_asc_p1_osc_cos_prev;
omc_asc_p1_osc_sin_prev = omc_asc_p1_osc_sin_new;
omc_asc_p1_osc_cos_prev = omc_asc_p1_osc_cos_new;
omc_asc_p1_osc[0] = omc_asc_p1_osc_sin_new * pLocalEpics->om2.OMC_ASC_P1_OSC_CLKGAIN;
omc_asc_p1_osc[1] = omc_asc_p1_osc_sin_new * pLocalEpics->om2.OMC_ASC_P1_OSC_SINGAIN;
omc_asc_p1_osc[2] = omc_asc_p1_osc_cos_new * pLocalEpics->om2.OMC_ASC_P1_OSC_COSGAIN;
if((omc_asc_p1_osc_freq != pLocalEpics->om2.OMC_ASC_P1_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	omc_asc_p1_osc_freq = pLocalEpics->om2.OMC_ASC_P1_OSC_FREQ;
	omc_asc_p1_osc_delta = 2.0 * 3.1415926535897932384626 * omc_asc_p1_osc_freq / FE_RATE;
	valx = omc_asc_p1_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_p1_osc_alpha = 2.0 * lsinx * lsinx;
	valx = omc_asc_p1_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	omc_asc_p1_osc_beta = lsinx;
	omc_asc_p1_osc_cos_prev = 1.0;
	omc_asc_p1_osc_sin_prev = 0.0;
}












// FILTER MODULE
omc_tt1_ul_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_UL_SEN,dWord[0][0],0);

// FILTER MODULE
omc_tt1_ll_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LL_SEN,dWord[0][1],0);

// FILTER MODULE
omc_tt1_ur_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_UR_SEN,dWord[0][2],0);

// Wd (Watchdog) MODULE
if((clock16K % (FE_RATE/1024)) == 0) {
if (pLocalEpics->om2.OMC_TT1_WD == 1) {
	omc_tt1_wd = 1;
	pLocalEpics->om2.OMC_TT1_WD = 0;
};
double ins[5]= {
	dWord[0][0],
	dWord[0][1],
	dWord[0][2],
	dWord[0][3],
	omc_tt1_ground,
};
   for(ii=0; ii<5;ii++) {
	omc_tt1_wd_avg[ii] = ins[ii] * .00005 + omc_tt1_wd_avg[ii] * 0.99995;
	omc_tt1_wd_vabs = ins[ii] - omc_tt1_wd_avg[ii];
	if(omc_tt1_wd_vabs < 0) omc_tt1_wd_vabs *= -1.0;
	omc_tt1_wd_var[ii] = omc_tt1_wd_vabs * .00005 + omc_tt1_wd_var[ii] * 0.99995;
	pLocalEpics->om2.OMC_TT1_WD_VAR[ii] = omc_tt1_wd_var[ii];
	if(omc_tt1_wd_var[ii] > pLocalEpics->om2.OMC_TT1_WD_MAX) omc_tt1_wd = 0;
   }
	pLocalEpics->om2.OMC_TT1_WD_STAT = omc_tt1_wd;
}

// FILTER MODULE
omc_tt1_lr_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LR_SEN,dWord[0][3],0);

// FILTER MODULE
omc_tt1_ul_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_UL_MON,dWord[0][16],0);

// FILTER MODULE
omc_tt1_ll_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LL_MON,dWord[0][17],0);

// FILTER MODULE
omc_tt1_ur_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_UR_MON,dWord[0][18],0);

// FILTER MODULE
omc_tt1_lr_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LR_MON,dWord[0][19],0);

// EpicsOut
pLocalEpics->om2.OMC_TT1_UL_LEDMON = dWord[0][24];

// EpicsOut
pLocalEpics->om2.OMC_TT1_LL_LEDMON = dWord[0][25];

// EpicsOut
pLocalEpics->om2.OMC_TT1_UR_LEDMON = dWord[0][26];

// EpicsOut
pLocalEpics->om2.OMC_TT1_LR_LEDMON = dWord[0][27];

// FILTER MODULE
omc_tt2_ul_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_UL_SEN,dWord[0][4],0);

// FILTER MODULE
omc_tt2_ll_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LL_SEN,dWord[0][5],0);

// FILTER MODULE
omc_tt2_ur_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_UR_SEN,dWord[0][6],0);

// Wd (Watchdog) MODULE
if((clock16K % (FE_RATE/1024)) == 0) {
if (pLocalEpics->om2.OMC_TT2_WD == 1) {
	omc_tt2_wd = 1;
	pLocalEpics->om2.OMC_TT2_WD = 0;
};
double ins[5]= {
	dWord[0][4],
	dWord[0][5],
	dWord[0][6],
	dWord[0][7],
	omc_tt2_ground,
};
   for(ii=0; ii<5;ii++) {
	omc_tt2_wd_avg[ii] = ins[ii] * .00005 + omc_tt2_wd_avg[ii] * 0.99995;
	omc_tt2_wd_vabs = ins[ii] - omc_tt2_wd_avg[ii];
	if(omc_tt2_wd_vabs < 0) omc_tt2_wd_vabs *= -1.0;
	omc_tt2_wd_var[ii] = omc_tt2_wd_vabs * .00005 + omc_tt2_wd_var[ii] * 0.99995;
	pLocalEpics->om2.OMC_TT2_WD_VAR[ii] = omc_tt2_wd_var[ii];
	if(omc_tt2_wd_var[ii] > pLocalEpics->om2.OMC_TT2_WD_MAX) omc_tt2_wd = 0;
   }
	pLocalEpics->om2.OMC_TT2_WD_STAT = omc_tt2_wd;
}

// FILTER MODULE
omc_tt2_lr_sen = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LR_SEN,dWord[0][7],0);

// FILTER MODULE
omc_tt2_ul_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_UL_MON,dWord[0][20],0);

// FILTER MODULE
omc_tt2_ll_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LL_MON,dWord[0][21],0);

// FILTER MODULE
omc_tt2_ur_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_UR_MON,dWord[0][22],0);

// FILTER MODULE
omc_tt2_lr_mon = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LR_MON,dWord[0][23],0);

// EpicsOut
pLocalEpics->om2.OMC_TT2_UL_LEDMON = dWord[0][28];

// EpicsOut
pLocalEpics->om2.OMC_TT2_LL_LEDMON = dWord[0][29];

// EpicsOut
pLocalEpics->om2.OMC_TT2_UR_LEDMON = dWord[0][30];

// EpicsOut
pLocalEpics->om2.OMC_TT2_LR_LEDMON = dWord[0][31];

// EpicsOut
pLocalEpics->om2.OMC_SHUTTER_TRIGGERMON = ipc_at_0x2080;

// FILTER MODULE
omc_shutter_trigger_in = filterModuleD(dsp_ptr,dspCoeff,OMC_SHUTTER_TRIGGER_IN,ipc_at_0x2080,0);

// FILTER MODULE
omc_sync_test = filterModuleD(dsp_ptr,dspCoeff,OMC_SYNC_TEST,_ipc_shm? *((float *)(((void *)_ipc_shm) + 0x3000)) - cycle : 0.0,0);

// FILTER MODULE
omc_shutter_hwstate = filterModuleD(dsp_ptr,dspCoeff,OMC_SHUTTER_HWSTATE,ipc_at_0x2088,0);

// FILTER MODULE
omc_shutter_hwsetpt = filterModuleD(dsp_ptr,dspCoeff,OMC_SHUTTER_HWSETPT,ipc_at_0x2090,0);

// EpicsOut
pLocalEpics->om2.OMC_ASC_P1_MOUT = ipc_at_0x20A0;

// EpicsOut
pLocalEpics->om2.OMC_ASC_Y1_MOUT = ipc_at_0x20A0;

// EpicsOut
pLocalEpics->om2.OMC_ASC_P2_MOUT = ipc_at_0x20A0;

// EpicsOut
pLocalEpics->om2.OMC_ASC_Y2_MOUT = ipc_at_0x20A0;

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_qpdasc[1][ii] = 
	pLocalEpics->om2.OMC_ASC_QPDASC[ii][0] * ipc_at_0x2060 +
	pLocalEpics->om2.OMC_ASC_QPDASC[ii][1] * ipc_at_0x2068 +
	pLocalEpics->om2.OMC_ASC_QPDASC[ii][2] * ipc_at_0x2070 +
	pLocalEpics->om2.OMC_ASC_QPDASC[ii][3] * ipc_at_0x2078;
}

// MUX
omc_asc_mux[0]= ipc_at_0x2060;
omc_asc_mux[1]= ipc_at_0x2068;
omc_asc_mux[2]= ipc_at_0x2070;
omc_asc_mux[3]= ipc_at_0x2078;

// EpicsOut
pLocalEpics->om2.OMC_ASC_QPD1_P_MOUT = ipc_at_0x2000;

// EpicsOut
pLocalEpics->om2.OMC_ASC_QPD1_Y_MOUT = ipc_at_0x2010;

// EpicsOut
pLocalEpics->om2.OMC_ASC_QPD2_P_MOUT = ipc_at_0x2008;

// EpicsOut
pLocalEpics->om2.OMC_ASC_QPD2_Y_MOUT = ipc_at_0x2018;

// EpicsOut
pLocalEpics->om2.OMC_BIO_MON = ipc_at_0x20A8;

// EpicsOut
pLocalEpics->om2.OMC_BIO_RB = rioInputInput[0];

// MULTIPLY
omc_asc_rda_fpm_sinsin = ipc_at_0x20B8 * ipc_at_0x20C8;

// MULTIPLY
omc_asc_rda_fpm_cossin = ipc_at_0x20B0 * ipc_at_0x20C8;

// MULTIPLY
omc_asc_rda_fpm_sincos = ipc_at_0x20B8 * ipc_at_0x20D0;

// MULTIPLY
omc_asc_rda_fpm_coscos = ipc_at_0x20B0 * ipc_at_0x20D0;

// FILTER MODULE
omc_asc_magx = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_MAGX,dWord[0][8],0);

// FILTER MODULE
omc_asc_magy = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_MAGY,dWord[0][9],0);

// FILTER MODULE
omc_asc_magz = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_MAGZ,dWord[0][10],0);

// FILTER MODULE
omc_om2_adc_0_11 = filterModuleD(dsp_ptr,dspCoeff,OMC_OM2_ADC_0_11,dWord[0][11],0);

// FILTER MODULE
omc_om2_adc_0_12 = filterModuleD(dsp_ptr,dspCoeff,OMC_OM2_ADC_0_12,dWord[0][12],0);

// FILTER MODULE
omc_om2_adc_0_13 = filterModuleD(dsp_ptr,dspCoeff,OMC_OM2_ADC_0_13,dWord[0][13],0);

// FILTER MODULE
omc_om2_adc_0_14 = filterModuleD(dsp_ptr,dspCoeff,OMC_OM2_ADC_0_14,dWord[0][14],0);

// FILTER MODULE
omc_om2_adc_0_15 = filterModuleD(dsp_ptr,dspCoeff,OMC_OM2_ADC_0_15,dWord[0][15],0);

// FILTER MODULE
omc_tt2_lsc = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LSC,omc_ground1,0);

// FILTER MODULE
omc_tt1_lsc = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LSC,omc_ground4,0);

// MULTIPLY
omc_tt2_product6 = omc_tt2_divide * dWord[0][20];

// MULTIPLY
omc_tt2_product5 = omc_tt2_divide * dWord[0][21];

// MULTIPLY
omc_tt2_product4 = omc_tt2_divide * dWord[0][22];

// MULTIPLY
omc_tt2_product7 = omc_tt2_divide * dWord[0][23];

// PRODUCT
pLocalEpics->om2.OMC_TT1_OFFSETRAMP_RMON = 
	gainRamp(pLocalEpics->om2.OMC_TT1_OFFSETRAMP,pLocalEpics->om2.OMC_TT1_OFFSETRAMP_TRAMP,0,&OMC_TT1_OFFSETRAMP_CALC);

omc_tt1_offsetramp[0] = OMC_TT1_OFFSETRAMP_CALC * pLocalEpics->om2.OMC_TT1_POSOFFSET;
omc_tt1_offsetramp[1] = OMC_TT1_OFFSETRAMP_CALC * pLocalEpics->om2.OMC_TT1_PITOFFSET;
omc_tt1_offsetramp[2] = OMC_TT1_OFFSETRAMP_CALC * pLocalEpics->om2.OMC_TT1_YAWOFFSET;
omc_tt1_offsetramp[3] = OMC_TT1_OFFSETRAMP_CALC * omc_tt1_g1;
omc_tt1_offsetramp[4] = OMC_TT1_OFFSETRAMP_CALC * omc_tt1_g1;
omc_tt1_offsetramp[5] = OMC_TT1_OFFSETRAMP_CALC * omc_tt1_g1;
omc_tt1_offsetramp[6] = OMC_TT1_OFFSETRAMP_CALC * omc_tt1_g1;
omc_tt1_offsetramp[7] = OMC_TT1_OFFSETRAMP_CALC * omc_tt1_g1;
omc_tt1_offsetramp[8] = OMC_TT1_OFFSETRAMP_CALC * pLocalEpics->om2.OMC_TT1_ENABLEOFFSET;

// MULTIPLY
omc_tt1_product6 = omc_tt1_divide * dWord[0][16];

// MULTIPLY
omc_tt1_product5 = omc_tt1_divide * dWord[0][17];

// MULTIPLY
omc_tt1_product4 = omc_tt1_divide * dWord[0][18];

// MULTIPLY
omc_tt1_product7 = omc_tt1_divide * dWord[0][19];

// MULTIPLY
omc_asc_y2_fpm_sincos = omc_asc_y2_osc[1] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_y2_fpm_sinsin = omc_asc_y2_osc[1] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_y2_fpm_cossin = omc_asc_y2_osc[2] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_y2_fpm_coscos = omc_asc_y2_osc[2] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_p2_fpm_sincos = omc_asc_p2_osc[1] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_p2_fpm_sinsin = omc_asc_p2_osc[1] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_p2_fpm_cossin = omc_asc_p2_osc[2] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_p2_fpm_coscos = omc_asc_p2_osc[2] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_y1_fpm_sincos = omc_asc_y1_osc[1] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_y1_fpm_sinsin = omc_asc_y1_osc[1] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_y1_fpm_cossin = omc_asc_y1_osc[2] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_y1_fpm_coscos = omc_asc_y1_osc[2] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_p1_fpm_sincos = omc_asc_p1_osc[1] * ipc_at_0x20D0;

// MULTIPLY
omc_asc_p1_fpm_sinsin = omc_asc_p1_osc[1] * ipc_at_0x20C8;

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_exmtx[1][ii] = 
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][0] * omc_asc_p1_osc[1] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][1] * omc_asc_p1_osc[2] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][2] * omc_asc_y1_osc[1] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][3] * omc_asc_y1_osc[2] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][4] * omc_asc_p2_osc[1] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][5] * omc_asc_p2_osc[2] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][6] * omc_asc_y2_osc[1] +
	pLocalEpics->om2.OMC_ASC_EXMTX[ii][7] * omc_asc_y2_osc[2];
}

// MULTIPLY
omc_asc_p1_fpm_cossin = omc_asc_p1_osc[2] * ipc_at_0x20C8;

// MULTIPLY
omc_asc_p1_fpm_coscos = omc_asc_p1_osc[2] * ipc_at_0x20D0;

// EpicsOut
pLocalEpics->om2.OMC_TT1_WD_OUT = omc_tt1_wd;

// Matrix
for(ii=0;ii<3;ii++)
{
omc_tt1_inmtrx[1][ii] = 
	pLocalEpics->om2.OMC_TT1_INMTRX[ii][0] * omc_tt1_ul_sen +
	pLocalEpics->om2.OMC_TT1_INMTRX[ii][1] * omc_tt1_ll_sen +
	pLocalEpics->om2.OMC_TT1_INMTRX[ii][2] * omc_tt1_ur_sen +
	pLocalEpics->om2.OMC_TT1_INMTRX[ii][3] * omc_tt1_lr_sen;
}

// EpicsOut
pLocalEpics->om2.OMC_TT2_WD_OUT = omc_tt2_wd;

// Matrix
for(ii=0;ii<3;ii++)
{
omc_tt2_inmtrx[1][ii] = 
	pLocalEpics->om2.OMC_TT2_INMTRX[ii][0] * omc_tt2_ul_sen +
	pLocalEpics->om2.OMC_TT2_INMTRX[ii][1] * omc_tt2_ll_sen +
	pLocalEpics->om2.OMC_TT2_INMTRX[ii][2] * omc_tt2_ur_sen +
	pLocalEpics->om2.OMC_TT2_INMTRX[ii][3] * omc_tt2_lr_sen;
}


// Trigger MODULE
omc_shutter_trigger = pLocalEpics->om2.OMC_SHUTTER_TRIGGER_STATE;
if (omc_shutter_trigger_in > pLocalEpics->om2.OMC_SHUTTER_TRIGGER_THRESHOLD) {
 omc_shutter_trigger = 1;  // Tripped
 pLocalEpics->om2.OMC_SHUTTER_TRIGGER_RESET = 0;
} else if (pLocalEpics->om2.OMC_SHUTTER_TRIGGER_RESET > 0) {
  omc_shutter_trigger = 0; // Armed
  pLocalEpics->om2.OMC_SHUTTER_TRIGGER_RESET = 0;
}
pLocalEpics->om2.OMC_SHUTTER_TRIGGER_STATE = omc_shutter_trigger;

// Relational Operator
omc_shutter_hwtrig = ((omc_shutter_hwstate) > (omc_shutter_hwsetpt));
// FILTER MODULE
omc_asc_p1_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_SIG,pLocalEpics->om2.OMC_ASC_P1_MOUT,0);

// FILTER MODULE
omc_asc_y1_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_SIG,pLocalEpics->om2.OMC_ASC_Y1_MOUT,0);

// FILTER MODULE
omc_asc_p2_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_SIG,pLocalEpics->om2.OMC_ASC_P2_MOUT,0);

// FILTER MODULE
omc_asc_y2_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_SIG,pLocalEpics->om2.OMC_ASC_Y2_MOUT,0);

// FILTER MODULE
omc_asc_qpos_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPOS_Y,omc_asc_qpdasc[1][0],0);

// FILTER MODULE
omc_asc_qang_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QANG_Y,omc_asc_qpdasc[1][2],0);

// FILTER MODULE
omc_asc_qang_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QANG_X,omc_asc_qpdasc[1][3],0);

// FILTER MODULE
omc_asc_qpos_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPOS_X,omc_asc_qpdasc[1][1],0);

// MuxMatrix
for(ii=0;ii<4;ii++)
{
omc_asc_waistbasis[ii] = 
	pLocalEpics->om2.OMC_ASC_WAISTBASIS[ii][0] * omc_asc_mux[0] +
	pLocalEpics->om2.OMC_ASC_WAISTBASIS[ii][1] * omc_asc_mux[1] +
	pLocalEpics->om2.OMC_ASC_WAISTBASIS[ii][2] * omc_asc_mux[2] +
	pLocalEpics->om2.OMC_ASC_WAISTBASIS[ii][3] * omc_asc_mux[3];
}

// FILTER MODULE
omc_asc_qpd1_p_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_P_SIG,pLocalEpics->om2.OMC_ASC_QPD1_P_MOUT,0);

// FILTER MODULE
omc_asc_qpd1_y_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_Y_SIG,pLocalEpics->om2.OMC_ASC_QPD1_Y_MOUT,0);

// FILTER MODULE
omc_asc_qpd2_p_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_P_SIG,pLocalEpics->om2.OMC_ASC_QPD2_P_MOUT,0);

// FILTER MODULE
omc_asc_qpd2_y_sig = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_Y_SIG,pLocalEpics->om2.OMC_ASC_QPD2_Y_MOUT,0);

// SUM
omc_asc_rda_fpm_sum2 = omc_asc_rda_fpm_sincos - omc_asc_rda_fpm_cossin;

// SUM
omc_asc_rda_fpm_sum = omc_asc_rda_fpm_cossin + omc_asc_rda_fpm_sincos;

// SUM
omc_asc_rda_fpm_sum1 = omc_asc_rda_fpm_coscos - omc_asc_rda_fpm_sinsin;

// SUM
omc_asc_rda_fpm_sum3 = omc_asc_rda_fpm_sinsin + omc_asc_rda_fpm_coscos;

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_magff_mat[1][ii] = 
	pLocalEpics->om2.OMC_ASC_MAGFF_MAT[ii][0] * omc_asc_magx +
	pLocalEpics->om2.OMC_ASC_MAGFF_MAT[ii][1] * omc_asc_magy +
	pLocalEpics->om2.OMC_ASC_MAGFF_MAT[ii][2] * omc_asc_magz;
}

// EpicsOut
pLocalEpics->om2.OMC_TT2_UL_MON_VOLT = omc_tt2_product6;

// EpicsOut
pLocalEpics->om2.OMC_TT2_LL_MON_VOLT = omc_tt2_product5;

// EpicsOut
pLocalEpics->om2.OMC_TT2_UR_MON_VOLT = omc_tt2_product4;

// EpicsOut
pLocalEpics->om2.OMC_TT2_LR_MON_VOLT = omc_tt2_product7;

// EpicsOut
pLocalEpics->om2.OMC_TT1_UL_MON_VOLT = omc_tt1_product6;

// EpicsOut
pLocalEpics->om2.OMC_TT1_LL_MON_VOLT = omc_tt1_product5;

// EpicsOut
pLocalEpics->om2.OMC_TT1_UR_MON_VOLT = omc_tt1_product4;

// EpicsOut
pLocalEpics->om2.OMC_TT1_LR_MON_VOLT = omc_tt1_product7;

// SUM
omc_asc_y2_fpm_sum2 = omc_asc_y2_fpm_sincos - omc_asc_y2_fpm_cossin;

// SUM
omc_asc_y2_fpm_sum = omc_asc_y2_fpm_cossin + omc_asc_y2_fpm_sincos;

// SUM
omc_asc_y2_fpm_sum1 = omc_asc_y2_fpm_coscos - omc_asc_y2_fpm_sinsin;

// SUM
omc_asc_y2_fpm_sum3 = omc_asc_y2_fpm_sinsin + omc_asc_y2_fpm_coscos;

// SUM
omc_asc_p2_fpm_sum2 = omc_asc_p2_fpm_sincos - omc_asc_p2_fpm_cossin;

// SUM
omc_asc_p2_fpm_sum = omc_asc_p2_fpm_cossin + omc_asc_p2_fpm_sincos;

// SUM
omc_asc_p2_fpm_sum1 = omc_asc_p2_fpm_coscos - omc_asc_p2_fpm_sinsin;

// SUM
omc_asc_p2_fpm_sum3 = omc_asc_p2_fpm_sinsin + omc_asc_p2_fpm_coscos;

// SUM
omc_asc_y1_fpm_sum2 = omc_asc_y1_fpm_sincos - omc_asc_y1_fpm_cossin;

// SUM
omc_asc_y1_fpm_sum = omc_asc_y1_fpm_cossin + omc_asc_y1_fpm_sincos;

// SUM
omc_asc_y1_fpm_sum1 = omc_asc_y1_fpm_coscos - omc_asc_y1_fpm_sinsin;

// SUM
omc_asc_y1_fpm_sum3 = omc_asc_y1_fpm_sinsin + omc_asc_y1_fpm_coscos;

// FILTER MODULE
omc_asc_p1_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_CLOCK,omc_asc_exmtx[1][0],0);

// FILTER MODULE
omc_asc_y1_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y1_CLOCK,omc_asc_exmtx[1][1],0);

// FILTER MODULE
omc_asc_p2_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P2_CLOCK,omc_asc_exmtx[1][2],0);

// FILTER MODULE
omc_asc_y2_clock = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_Y2_CLOCK,omc_asc_exmtx[1][3],0);

// SUM
omc_asc_p1_fpm_sum2 = omc_asc_p1_fpm_sincos - omc_asc_p1_fpm_cossin;

// SUM
omc_asc_p1_fpm_sum = omc_asc_p1_fpm_cossin + omc_asc_p1_fpm_sincos;

// SUM
omc_asc_p1_fpm_sum1 = omc_asc_p1_fpm_coscos - omc_asc_p1_fpm_sinsin;

// SUM
omc_asc_p1_fpm_sum3 = omc_asc_p1_fpm_sinsin + omc_asc_p1_fpm_coscos;

// FILTER MODULE
omc_tt1_suspos = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_SUSPOS,omc_tt1_inmtrx[1][0],0);

// FILTER MODULE
omc_tt1_suspit = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_SUSPIT,omc_tt1_inmtrx[1][1],0);

// FILTER MODULE
omc_tt1_susyaw = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_SUSYAW,omc_tt1_inmtrx[1][2],0);

// FILTER MODULE
omc_tt2_suspos = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_SUSPOS,omc_tt2_inmtrx[1][0],0);

// FILTER MODULE
omc_tt2_suspit = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_SUSPIT,omc_tt2_inmtrx[1][1],0);

// FILTER MODULE
omc_tt2_susyaw = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_SUSYAW,omc_tt2_inmtrx[1][2],0);

// EpicsOut
pLocalEpics->om2.OMC_SHUTTER_STATEMON = omc_shutter_trigger;

// MUX
omc_shutter_mux1[0]= omc_shutter_trigger;
omc_shutter_mux1[1]= omc_shutter_hwtrig;
omc_shutter_mux1[2]= pLocalEpics->om2.OMC_SHUTTER_HWSWRATIO;
omc_shutter_mux1[3]= pLocalEpics->om2.OMC_SHUTTER_BANGUPMS;
omc_shutter_mux1[4]= pLocalEpics->om2.OMC_SHUTTER_BANGDOWNMS;
omc_shutter_mux1[5]= pLocalEpics->om2.OMC_SHUTTER_HOLDCOUNTS;
omc_shutter_mux1[6]= omc_shutter_mspercycle;

// PRODUCT
pLocalEpics->om2.OMC_ASC_QPDGAIN_RMON = 
	gainRamp(pLocalEpics->om2.OMC_ASC_QPDGAIN,pLocalEpics->om2.OMC_ASC_QPDGAIN_TRAMP,1,&OMC_ASC_QPDGAIN_CALC);

omc_asc_qpdgain[0] = OMC_ASC_QPDGAIN_CALC * omc_asc_qpos_y;
omc_asc_qpdgain[1] = OMC_ASC_QPDGAIN_CALC * omc_asc_qpos_x;
omc_asc_qpdgain[2] = OMC_ASC_QPDGAIN_CALC * omc_asc_qang_y;
omc_asc_qpdgain[3] = OMC_ASC_QPDGAIN_CALC * omc_asc_qang_x;

// DEMUX
omc_asc_demux[0]= omc_asc_waistbasis[0];
omc_asc_demux[1]= omc_asc_waistbasis[1];
omc_asc_demux[2]= omc_asc_waistbasis[2];
omc_asc_demux[3]= omc_asc_waistbasis[3];

// Switch
omc_asc_rda_fpm_choicepms = (((pLocalEpics->om2.OMC_ASC_RDA_FPM_SUBTRACT) != 0)? (omc_asc_rda_fpm_sum2): (omc_asc_rda_fpm_sum));
// Switch
omc_asc_rda_fpm_choicepmc = (((pLocalEpics->om2.OMC_ASC_RDA_FPM_SUBTRACT) != 0)? (omc_asc_rda_fpm_sum3): (omc_asc_rda_fpm_sum1));
// FILTER MODULE
omc_asc_pos_y_mag = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_POS_Y_MAG,omc_asc_magff_mat[1][0],0);

// FILTER MODULE
omc_asc_pos_x_mag = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_POS_X_MAG,omc_asc_magff_mat[1][1],0);

// FILTER MODULE
omc_asc_ang_y_mag = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_ANG_Y_MAG,omc_asc_magff_mat[1][2],0);

// FILTER MODULE
omc_asc_ang_x_mag = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_ANG_X_MAG,omc_asc_magff_mat[1][3],0);

// RMS
omc_tt2_rms = pLocalEpics->om2.OMC_TT2_UL_MON_VOLT;
if(omc_tt2_rms > 2000) omc_tt2_rms = 2000;
if(omc_tt2_rms < -2000) omc_tt2_rms = -2000;
omc_tt2_rms = omc_tt2_rms * omc_tt2_rms;
omc_tt2_rms_avg = omc_tt2_rms * .00005 + omc_tt2_rms_avg * 0.99995;
omc_tt2_rms = lsqrt(omc_tt2_rms_avg);

// RMS
omc_tt2_rms1 = pLocalEpics->om2.OMC_TT2_LL_MON_VOLT;
if(omc_tt2_rms1 > 2000) omc_tt2_rms1 = 2000;
if(omc_tt2_rms1 < -2000) omc_tt2_rms1 = -2000;
omc_tt2_rms1 = omc_tt2_rms1 * omc_tt2_rms1;
omc_tt2_rms1_avg = omc_tt2_rms1 * .00005 + omc_tt2_rms1_avg * 0.99995;
omc_tt2_rms1 = lsqrt(omc_tt2_rms1_avg);

// RMS
omc_tt2_rms2 = pLocalEpics->om2.OMC_TT2_UR_MON_VOLT;
if(omc_tt2_rms2 > 2000) omc_tt2_rms2 = 2000;
if(omc_tt2_rms2 < -2000) omc_tt2_rms2 = -2000;
omc_tt2_rms2 = omc_tt2_rms2 * omc_tt2_rms2;
omc_tt2_rms2_avg = omc_tt2_rms2 * .00005 + omc_tt2_rms2_avg * 0.99995;
omc_tt2_rms2 = lsqrt(omc_tt2_rms2_avg);

// RMS
omc_tt2_rms3 = pLocalEpics->om2.OMC_TT2_LR_MON_VOLT;
if(omc_tt2_rms3 > 2000) omc_tt2_rms3 = 2000;
if(omc_tt2_rms3 < -2000) omc_tt2_rms3 = -2000;
omc_tt2_rms3 = omc_tt2_rms3 * omc_tt2_rms3;
omc_tt2_rms3_avg = omc_tt2_rms3 * .00005 + omc_tt2_rms3_avg * 0.99995;
omc_tt2_rms3 = lsqrt(omc_tt2_rms3_avg);

// RMS
omc_tt1_rms = pLocalEpics->om2.OMC_TT1_UL_MON_VOLT;
if(omc_tt1_rms > 2000) omc_tt1_rms = 2000;
if(omc_tt1_rms < -2000) omc_tt1_rms = -2000;
omc_tt1_rms = omc_tt1_rms * omc_tt1_rms;
omc_tt1_rms_avg = omc_tt1_rms * .00005 + omc_tt1_rms_avg * 0.99995;
omc_tt1_rms = lsqrt(omc_tt1_rms_avg);

// RMS
omc_tt1_rms1 = pLocalEpics->om2.OMC_TT1_LL_MON_VOLT;
if(omc_tt1_rms1 > 2000) omc_tt1_rms1 = 2000;
if(omc_tt1_rms1 < -2000) omc_tt1_rms1 = -2000;
omc_tt1_rms1 = omc_tt1_rms1 * omc_tt1_rms1;
omc_tt1_rms1_avg = omc_tt1_rms1 * .00005 + omc_tt1_rms1_avg * 0.99995;
omc_tt1_rms1 = lsqrt(omc_tt1_rms1_avg);

// RMS
omc_tt1_rms2 = pLocalEpics->om2.OMC_TT1_UR_MON_VOLT;
if(omc_tt1_rms2 > 2000) omc_tt1_rms2 = 2000;
if(omc_tt1_rms2 < -2000) omc_tt1_rms2 = -2000;
omc_tt1_rms2 = omc_tt1_rms2 * omc_tt1_rms2;
omc_tt1_rms2_avg = omc_tt1_rms2 * .00005 + omc_tt1_rms2_avg * 0.99995;
omc_tt1_rms2 = lsqrt(omc_tt1_rms2_avg);

// RMS
omc_tt1_rms3 = pLocalEpics->om2.OMC_TT1_LR_MON_VOLT;
if(omc_tt1_rms3 > 2000) omc_tt1_rms3 = 2000;
if(omc_tt1_rms3 < -2000) omc_tt1_rms3 = -2000;
omc_tt1_rms3 = omc_tt1_rms3 * omc_tt1_rms3;
omc_tt1_rms3_avg = omc_tt1_rms3 * .00005 + omc_tt1_rms3_avg * 0.99995;
omc_tt1_rms3 = lsqrt(omc_tt1_rms3_avg);

// Switch
omc_asc_y2_fpm_choicepms = (((pLocalEpics->om2.OMC_ASC_Y2_FPM_SUBTRACT) != 0)? (omc_asc_y2_fpm_sum2): (omc_asc_y2_fpm_sum));
// Switch
omc_asc_y2_fpm_choicepmc = (((pLocalEpics->om2.OMC_ASC_Y2_FPM_SUBTRACT) != 0)? (omc_asc_y2_fpm_sum3): (omc_asc_y2_fpm_sum1));
// Switch
omc_asc_p2_fpm_choicepms = (((pLocalEpics->om2.OMC_ASC_P2_FPM_SUBTRACT) != 0)? (omc_asc_p2_fpm_sum2): (omc_asc_p2_fpm_sum));
// Switch
omc_asc_p2_fpm_choicepmc = (((pLocalEpics->om2.OMC_ASC_P2_FPM_SUBTRACT) != 0)? (omc_asc_p2_fpm_sum3): (omc_asc_p2_fpm_sum1));
// Switch
omc_asc_y1_fpm_choicepms = (((pLocalEpics->om2.OMC_ASC_Y1_FPM_SUBTRACT) != 0)? (omc_asc_y1_fpm_sum2): (omc_asc_y1_fpm_sum));
// Switch
omc_asc_y1_fpm_choicepmc = (((pLocalEpics->om2.OMC_ASC_Y1_FPM_SUBTRACT) != 0)? (omc_asc_y1_fpm_sum3): (omc_asc_y1_fpm_sum1));
// Switch
omc_asc_p1_fpm_choicepms = (((pLocalEpics->om2.OMC_ASC_P1_FPM_SUBTRACT) != 0)? (omc_asc_p1_fpm_sum2): (omc_asc_p1_fpm_sum));
// Switch
omc_asc_p1_fpm_choicepmc = (((pLocalEpics->om2.OMC_ASC_P1_FPM_SUBTRACT) != 0)? (omc_asc_p1_fpm_sum3): (omc_asc_p1_fpm_sum1));
// SUM
omc_tt1_sum = omc_tt1_offsetramp[0] + omc_tt1_suspos + omc_tt1_lsc;

// SUM
omc_tt2_sum = pLocalEpics->om2.OMC_TT2_POSOFFSET + omc_tt2_suspos + omc_tt2_lsc;

// Function Call
OMC_SHUTTER_SWTRIG(omc_shutter_mux1, 7, omc_shutter_demux1, 1);

// FILTER MODULE
omc_asc_waist_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_X,omc_asc_demux[0],0);

// FILTER MODULE
omc_asc_waist_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_Y,omc_asc_demux[1],0);

// FILTER MODULE
omc_asc_waist_pit = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_PIT,omc_asc_demux[2],0);

// FILTER MODULE
omc_asc_waist_yaw = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_WAIST_YAW,omc_asc_demux[3],0);

// PHASEDEG
omc_asc_rda_fpm_p[0] = (omc_asc_rda_fpm_choicepms * pLocalEpics->om2.OMC_ASC_RDA_FPM_P[1]) + (omc_asc_rda_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_RDA_FPM_P[0]);
omc_asc_rda_fpm_p[1] = (omc_asc_rda_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_RDA_FPM_P[1]) - (omc_asc_rda_fpm_choicepms * pLocalEpics->om2.OMC_ASC_RDA_FPM_P[0]);

// EpicsOut
pLocalEpics->om2.OMC_TT2_UL_MON_VRMS = omc_tt2_rms;

// EpicsOut
pLocalEpics->om2.OMC_TT2_LL_MON_VRMS = omc_tt2_rms1;

// EpicsOut
pLocalEpics->om2.OMC_TT2_UR_MON_VRMS = omc_tt2_rms2;

// EpicsOut
pLocalEpics->om2.OMC_TT2_LR_MON_VRMS = omc_tt2_rms3;

// EpicsOut
pLocalEpics->om2.OMC_TT1_UL_MON_VRMS = omc_tt1_rms;

// EpicsOut
pLocalEpics->om2.OMC_TT1_LL_MON_VRMS = omc_tt1_rms1;

// EpicsOut
pLocalEpics->om2.OMC_TT1_UR_MON_VRMS = omc_tt1_rms2;

// EpicsOut
pLocalEpics->om2.OMC_TT1_LR_MON_VRMS = omc_tt1_rms3;

// PHASEDEG
omc_asc_y2_fpm_p[0] = (omc_asc_y2_fpm_choicepms * pLocalEpics->om2.OMC_ASC_Y2_FPM_P[1]) + (omc_asc_y2_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_Y2_FPM_P[0]);
omc_asc_y2_fpm_p[1] = (omc_asc_y2_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_Y2_FPM_P[1]) - (omc_asc_y2_fpm_choicepms * pLocalEpics->om2.OMC_ASC_Y2_FPM_P[0]);

// PHASEDEG
omc_asc_p2_fpm_p[0] = (omc_asc_p2_fpm_choicepms * pLocalEpics->om2.OMC_ASC_P2_FPM_P[1]) + (omc_asc_p2_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_P2_FPM_P[0]);
omc_asc_p2_fpm_p[1] = (omc_asc_p2_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_P2_FPM_P[1]) - (omc_asc_p2_fpm_choicepms * pLocalEpics->om2.OMC_ASC_P2_FPM_P[0]);

// PHASEDEG
omc_asc_y1_fpm_p[0] = (omc_asc_y1_fpm_choicepms * pLocalEpics->om2.OMC_ASC_Y1_FPM_P[1]) + (omc_asc_y1_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_Y1_FPM_P[0]);
omc_asc_y1_fpm_p[1] = (omc_asc_y1_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_Y1_FPM_P[1]) - (omc_asc_y1_fpm_choicepms * pLocalEpics->om2.OMC_ASC_Y1_FPM_P[0]);

// PHASEDEG
omc_asc_p1_fpm_p[0] = (omc_asc_p1_fpm_choicepms * pLocalEpics->om2.OMC_ASC_P1_FPM_P[1]) + (omc_asc_p1_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_P1_FPM_P[0]);
omc_asc_p1_fpm_p[1] = (omc_asc_p1_fpm_choicepmc * pLocalEpics->om2.OMC_ASC_P1_FPM_P[1]) - (omc_asc_p1_fpm_choicepms * pLocalEpics->om2.OMC_ASC_P1_FPM_P[0]);


// Switch
omc_asc_rda_fpm_choicesin = (((pLocalEpics->om2.OMC_ASC_RDA_FPM_BYPASS) != 0)? (ipc_at_0x20B8): (omc_asc_rda_fpm_p[0]));
// Switch
omc_asc_rda_fpm_choicecos = (((pLocalEpics->om2.OMC_ASC_RDA_FPM_BYPASS) != 0)? (ipc_at_0x20B0): (omc_asc_rda_fpm_p[1]));
// Switch
omc_asc_y2_fpm_choicesin = (((pLocalEpics->om2.OMC_ASC_Y2_FPM_BYPASS) != 0)? (omc_asc_y2_osc[1]): (omc_asc_y2_fpm_p[0]));
// Switch
omc_asc_y2_fpm_choicecos = (((pLocalEpics->om2.OMC_ASC_Y2_FPM_BYPASS) != 0)? (omc_asc_y2_osc[2]): (omc_asc_y2_fpm_p[1]));
// Switch
omc_asc_p2_fpm_choicesin = (((pLocalEpics->om2.OMC_ASC_P2_FPM_BYPASS) != 0)? (omc_asc_p2_osc[1]): (omc_asc_p2_fpm_p[0]));
// Switch
omc_asc_p2_fpm_choicecos = (((pLocalEpics->om2.OMC_ASC_P2_FPM_BYPASS) != 0)? (omc_asc_p2_osc[2]): (omc_asc_p2_fpm_p[1]));
// Switch
omc_asc_y1_fpm_choicesin = (((pLocalEpics->om2.OMC_ASC_Y1_FPM_BYPASS) != 0)? (omc_asc_y1_osc[1]): (omc_asc_y1_fpm_p[0]));
// Switch
omc_asc_y1_fpm_choicecos = (((pLocalEpics->om2.OMC_ASC_Y1_FPM_BYPASS) != 0)? (omc_asc_y1_osc[2]): (omc_asc_y1_fpm_p[1]));
// Switch
omc_asc_p1_fpm_choicesin = (((pLocalEpics->om2.OMC_ASC_P1_FPM_BYPASS) != 0)? (omc_asc_p1_osc[1]): (omc_asc_p1_fpm_p[0]));
// Switch
omc_asc_p1_fpm_choicecos = (((pLocalEpics->om2.OMC_ASC_P1_FPM_BYPASS) != 0)? (omc_asc_p1_osc[2]): (omc_asc_p1_fpm_p[1]));
// FILTER MODULE
omc_shutter_resp = filterModuleD(dsp_ptr,dspCoeff,OMC_SHUTTER_RESP,omc_shutter_demux1[0],0);

// PHASEDEG
omc_asc_drum_fpm_p[0] = (omc_asc_rda_fpm_choicesin * pLocalEpics->om2.OMC_ASC_DRUM_FPM_P[1]) + (omc_asc_rda_fpm_choicecos * pLocalEpics->om2.OMC_ASC_DRUM_FPM_P[0]);
omc_asc_drum_fpm_p[1] = (omc_asc_rda_fpm_choicecos * pLocalEpics->om2.OMC_ASC_DRUM_FPM_P[1]) - (omc_asc_rda_fpm_choicesin * pLocalEpics->om2.OMC_ASC_DRUM_FPM_P[0]);

// MULTIPLY
omc_asc_product6 = omc_asc_y2_sig * omc_asc_y2_fpm_choicesin;

// MULTIPLY
omc_asc_product7 = omc_asc_y2_sig * omc_asc_y2_fpm_choicecos;

// MULTIPLY
omc_asc_product4 = omc_asc_p2_sig * omc_asc_p2_fpm_choicesin;

// MULTIPLY
omc_asc_product5 = omc_asc_p2_sig * omc_asc_p2_fpm_choicecos;

// MULTIPLY
omc_asc_product2 = omc_asc_y1_sig * omc_asc_y1_fpm_choicesin;

// MULTIPLY
omc_asc_product3 = omc_asc_y1_sig * omc_asc_y1_fpm_choicecos;

// MULTIPLY
omc_asc_product = omc_asc_p1_sig * omc_asc_p1_fpm_choicesin;

// MULTIPLY
omc_asc_product1 = omc_asc_p1_sig * omc_asc_p1_fpm_choicecos;

// Gain
omc_shutter_gainul = omc_shutter_resp * -1;

// Gain
omc_shutter_gainll = omc_shutter_resp * -1;

// Gain
omc_shutter_gainur = omc_shutter_resp * 1;

// Gain
omc_shutter_gainlr = omc_shutter_resp * 1;

// MULTIPLY
omc_asc_drum_fpm_sincos = omc_asc_drum_fpm_p[0] * ipc_at_0x20A0;

// MULTIPLY
omc_asc_drum_fpm_coscos = omc_asc_drum_fpm_p[1] * ipc_at_0x20A0;

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

// FILTER MODULE
omc_asc_p1_x_sin = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_X_SIN,omc_asc_product,0);

// FILTER MODULE
omc_asc_p1_x_cos = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_X_COS,omc_asc_product1,0);

// FILTER MODULE
omc_asc_drum_fpm_sp = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DRUM_FPM_SP,omc_asc_drum_fpm_sincos,0);

// FILTER MODULE
omc_asc_drum_fpm_cp = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DRUM_FPM_CP,omc_asc_drum_fpm_coscos,0);

// PHASEDEG
omc_asc_y2_phase[0] = (omc_asc_y2_x_sin * pLocalEpics->om2.OMC_ASC_Y2_PHASE[1]) + (omc_asc_y2_x_cos * pLocalEpics->om2.OMC_ASC_Y2_PHASE[0]);
omc_asc_y2_phase[1] = (omc_asc_y2_x_cos * pLocalEpics->om2.OMC_ASC_Y2_PHASE[1]) - (omc_asc_y2_x_sin * pLocalEpics->om2.OMC_ASC_Y2_PHASE[0]);

// PHASEDEG
omc_asc_p2_phase[0] = (omc_asc_p2_x_sin * pLocalEpics->om2.OMC_ASC_P2_PHASE[1]) + (omc_asc_p2_x_cos * pLocalEpics->om2.OMC_ASC_P2_PHASE[0]);
omc_asc_p2_phase[1] = (omc_asc_p2_x_cos * pLocalEpics->om2.OMC_ASC_P2_PHASE[1]) - (omc_asc_p2_x_sin * pLocalEpics->om2.OMC_ASC_P2_PHASE[0]);

// PHASEDEG
omc_asc_y1_phase[0] = (omc_asc_y1_x_sin * pLocalEpics->om2.OMC_ASC_Y1_PHASE[1]) + (omc_asc_y1_x_cos * pLocalEpics->om2.OMC_ASC_Y1_PHASE[0]);
omc_asc_y1_phase[1] = (omc_asc_y1_x_cos * pLocalEpics->om2.OMC_ASC_Y1_PHASE[1]) - (omc_asc_y1_x_sin * pLocalEpics->om2.OMC_ASC_Y1_PHASE[0]);

// PHASEDEG
omc_asc_p1_phase[0] = (omc_asc_p1_x_sin * pLocalEpics->om2.OMC_ASC_P1_PHASE[1]) + (omc_asc_p1_x_cos * pLocalEpics->om2.OMC_ASC_P1_PHASE[0]);
omc_asc_p1_phase[1] = (omc_asc_p1_x_cos * pLocalEpics->om2.OMC_ASC_P1_PHASE[1]) - (omc_asc_p1_x_sin * pLocalEpics->om2.OMC_ASC_P1_PHASE[0]);

// Switch
omc_asc_drum_fpm_choicesin = (((pLocalEpics->om2.OMC_ASC_DRUM_FPM_BYPASS) != 0)? (omc_asc_drum_fpm_p[0]): (omc_asc_drum_fpm_sp));
// Switch
omc_asc_drum_fpm_choicecos = (((pLocalEpics->om2.OMC_ASC_DRUM_FPM_BYPASS) != 0)? (omc_asc_drum_fpm_p[1]): (omc_asc_drum_fpm_cp));
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

// FILTER MODULE
omc_asc_p1_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_I,omc_asc_p1_phase[0],0);

// FILTER MODULE
omc_asc_p1_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_P1_Q,omc_asc_p1_phase[1],0);

// MULTIPLY
omc_asc_product12 = omc_asc_qpd2_y_sig * omc_asc_drum_fpm_choicesin;

// MULTIPLY
omc_asc_product14 = omc_asc_qpd2_p_sig * omc_asc_drum_fpm_choicesin;

// MULTIPLY
omc_asc_product10 = omc_asc_qpd1_y_sig * omc_asc_drum_fpm_choicesin;

// MULTIPLY
omc_asc_product8 = omc_asc_qpd1_p_sig * omc_asc_drum_fpm_choicesin;

// MULTIPLY
omc_asc_product9 = omc_asc_qpd1_p_sig * omc_asc_drum_fpm_choicecos;

// MULTIPLY
omc_asc_product11 = omc_asc_qpd1_y_sig * omc_asc_drum_fpm_choicecos;

// MULTIPLY
omc_asc_product13 = omc_asc_qpd2_y_sig * omc_asc_drum_fpm_choicecos;

// MULTIPLY
omc_asc_product15 = omc_asc_qpd2_p_sig * omc_asc_drum_fpm_choicecos;

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_acmtx[1][ii] = 
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][0] * omc_asc_p1_i +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][1] * omc_asc_p1_q +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][2] * omc_asc_y1_i +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][3] * omc_asc_y1_q +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][4] * omc_asc_p2_i +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][5] * omc_asc_p2_q +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][6] * omc_asc_y2_i +
	pLocalEpics->om2.OMC_ASC_ACMTX[ii][7] * omc_asc_y2_q;
}

// FILTER MODULE
omc_asc_qpd2_y_s = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_Y_S,omc_asc_product12,0);

// FILTER MODULE
omc_asc_qpd2_p_s = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_P_S,omc_asc_product14,0);

// FILTER MODULE
omc_asc_qpd1_y_s = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_Y_S,omc_asc_product10,0);

// FILTER MODULE
omc_asc_qpd1_p_s = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_P_S,omc_asc_product8,0);

// FILTER MODULE
omc_asc_qpd1_p_c = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_P_C,omc_asc_product9,0);

// FILTER MODULE
omc_asc_qpd1_y_c = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_Y_C,omc_asc_product11,0);

// FILTER MODULE
omc_asc_qpd2_y_c = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_Y_C,omc_asc_product13,0);

// FILTER MODULE
omc_asc_qpd2_p_c = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_P_C,omc_asc_product15,0);

// FILTER MODULE
omc_asc_dpos_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DPOS_Y,omc_asc_acmtx[1][0],0);

// FILTER MODULE
omc_asc_dpos_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DPOS_X,omc_asc_acmtx[1][1],0);

// FILTER MODULE
omc_asc_dang_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DANG_Y,omc_asc_acmtx[1][2],0);

// FILTER MODULE
omc_asc_dang_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_DANG_X,omc_asc_acmtx[1][3],0);

// PHASEDEG
omc_asc_qpd1_p_phase[0] = (omc_asc_qpd1_p_s * pLocalEpics->om2.OMC_ASC_QPD1_P_PHASE[1]) + (omc_asc_qpd1_p_c * pLocalEpics->om2.OMC_ASC_QPD1_P_PHASE[0]);
omc_asc_qpd1_p_phase[1] = (omc_asc_qpd1_p_c * pLocalEpics->om2.OMC_ASC_QPD1_P_PHASE[1]) - (omc_asc_qpd1_p_s * pLocalEpics->om2.OMC_ASC_QPD1_P_PHASE[0]);

// PHASEDEG
omc_asc_qpd1_y_phase[0] = (omc_asc_qpd1_y_s * pLocalEpics->om2.OMC_ASC_QPD1_Y_PHASE[1]) + (omc_asc_qpd1_y_c * pLocalEpics->om2.OMC_ASC_QPD1_Y_PHASE[0]);
omc_asc_qpd1_y_phase[1] = (omc_asc_qpd1_y_c * pLocalEpics->om2.OMC_ASC_QPD1_Y_PHASE[1]) - (omc_asc_qpd1_y_s * pLocalEpics->om2.OMC_ASC_QPD1_Y_PHASE[0]);

// PHASEDEG
omc_asc_qpd2_y_phase[0] = (omc_asc_qpd2_y_s * pLocalEpics->om2.OMC_ASC_QPD2_Y_PHASE[1]) + (omc_asc_qpd2_y_c * pLocalEpics->om2.OMC_ASC_QPD2_Y_PHASE[0]);
omc_asc_qpd2_y_phase[1] = (omc_asc_qpd2_y_c * pLocalEpics->om2.OMC_ASC_QPD2_Y_PHASE[1]) - (omc_asc_qpd2_y_s * pLocalEpics->om2.OMC_ASC_QPD2_Y_PHASE[0]);

// PHASEDEG
omc_asc_qpd2_p_phase[0] = (omc_asc_qpd2_p_s * pLocalEpics->om2.OMC_ASC_QPD2_P_PHASE[1]) + (omc_asc_qpd2_p_c * pLocalEpics->om2.OMC_ASC_QPD2_P_PHASE[0]);
omc_asc_qpd2_p_phase[1] = (omc_asc_qpd2_p_c * pLocalEpics->om2.OMC_ASC_QPD2_P_PHASE[1]) - (omc_asc_qpd2_p_s * pLocalEpics->om2.OMC_ASC_QPD2_P_PHASE[0]);

// PRODUCT
pLocalEpics->om2.OMC_ASC_DITHERGAIN_RMON = 
	gainRamp(pLocalEpics->om2.OMC_ASC_DITHERGAIN,pLocalEpics->om2.OMC_ASC_DITHERGAIN_TRAMP,2,&OMC_ASC_DITHERGAIN_CALC);

omc_asc_dithergain[0] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dpos_y;
omc_asc_dithergain[1] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dpos_x;
omc_asc_dithergain[2] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dang_y;
omc_asc_dithergain[3] = OMC_ASC_DITHERGAIN_CALC * omc_asc_dang_x;

// FILTER MODULE
omc_asc_qpd1_p_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_P_I,omc_asc_qpd1_p_phase[0],0);

// FILTER MODULE
omc_asc_qpd1_p_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_P_Q,omc_asc_qpd1_p_phase[1],0);

// FILTER MODULE
omc_asc_qpd1_y_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_Y_I,omc_asc_qpd1_y_phase[0],0);

// FILTER MODULE
omc_asc_qpd1_y_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD1_Y_Q,omc_asc_qpd1_y_phase[1],0);

// FILTER MODULE
omc_asc_qpd2_y_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_Y_I,omc_asc_qpd2_y_phase[0],0);

// FILTER MODULE
omc_asc_qpd2_y_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_Y_Q,omc_asc_qpd2_y_phase[1],0);

// FILTER MODULE
omc_asc_qpd2_p_i = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_P_I,omc_asc_qpd2_p_phase[0],0);

// FILTER MODULE
omc_asc_qpd2_p_q = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_QPD2_P_Q,omc_asc_qpd2_p_phase[1],0);

// Matrix
for(ii=0;ii<4;ii++)
{
omc_asc_rdasc[1][ii] = 
	pLocalEpics->om2.OMC_ASC_RDASC[ii][0] * omc_asc_qpd1_p_i +
	pLocalEpics->om2.OMC_ASC_RDASC[ii][1] * omc_asc_qpd1_y_i +
	pLocalEpics->om2.OMC_ASC_RDASC[ii][2] * omc_asc_qpd2_p_i +
	pLocalEpics->om2.OMC_ASC_RDASC[ii][3] * omc_asc_qpd2_y_i;
}

// FILTER MODULE
omc_asc_rpos_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_RPOS_Y,omc_asc_rdasc[1][0],0);

// FILTER MODULE
omc_asc_rpos_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_RPOS_X,omc_asc_rdasc[1][1],0);

// FILTER MODULE
omc_asc_rang_y = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_RANG_Y,omc_asc_rdasc[1][2],0);

// FILTER MODULE
omc_asc_rang_x = filterModuleD(dsp_ptr,dspCoeff,OMC_ASC_RANG_X,omc_asc_rdasc[1][3],0);

// PRODUCT
pLocalEpics->om2.OMC_ASC_RDAGAIN_RMON = 
	gainRamp(pLocalEpics->om2.OMC_ASC_RDAGAIN,pLocalEpics->om2.OMC_ASC_RDAGAIN_TRAMP,3,&OMC_ASC_RDAGAIN_CALC);

omc_asc_rdagain[0] = OMC_ASC_RDAGAIN_CALC * omc_asc_rpos_y;
omc_asc_rdagain[1] = OMC_ASC_RDAGAIN_CALC * omc_asc_rpos_x;
omc_asc_rdagain[2] = OMC_ASC_RDAGAIN_CALC * omc_asc_rang_y;
omc_asc_rdagain[3] = OMC_ASC_RDAGAIN_CALC * omc_asc_rang_x;

// SUM
omc_asc_sum8 = omc_asc_rdagain[0] + omc_asc_ground8 + omc_asc_qpdgain[0] + omc_asc_dithergain[0];

// SUM
omc_asc_sum9 = omc_asc_rdagain[1] + omc_asc_ground8 + omc_asc_qpdgain[1] + omc_asc_dithergain[1];

// SUM
omc_asc_sum10 = omc_asc_rdagain[2] + omc_asc_ground8 + omc_asc_qpdgain[2] + omc_asc_dithergain[2];

// SUM
omc_asc_sum11 = omc_asc_rdagain[3] + omc_asc_ground8 + omc_asc_qpdgain[3] + omc_asc_dithergain[3];

// PRODUCT
pLocalEpics->om2.OMC_ASC_MASTERGAIN_RMON = 
	gainRamp(pLocalEpics->om2.OMC_ASC_MASTERGAIN,pLocalEpics->om2.OMC_ASC_MASTERGAIN_TRAMP,4,&OMC_ASC_MASTERGAIN_CALC);

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
omc_asc_sum1 = omc_asc_pos_y_mag + omc_asc_pos_y + omc_asc_p1_clock;

// SUM
omc_asc_sum2 = omc_asc_pos_x_mag + omc_asc_pos_x + omc_asc_y1_clock;

// SUM
omc_asc_sum3 = omc_asc_ang_y_mag + omc_asc_ang_y + omc_asc_p2_clock;

// SUM
omc_asc_sum12 = omc_asc_ang_x_mag + omc_asc_ang_x + omc_asc_y2_clock;

// FILTER MODULE
omc_tt1_pit = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_PIT,omc_asc_sum1,0);

// FILTER MODULE
omc_tt1_yaw = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_YAW,omc_asc_sum2,0);

// FILTER MODULE
omc_tt2_pit = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_PIT,omc_asc_sum3,0);

// FILTER MODULE
omc_tt2_yaw = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_YAW,omc_asc_sum12,0);

// SUM
omc_tt1_sum1 = omc_tt1_offsetramp[1] + omc_tt1_suspit + omc_tt1_pit;

// SUM
omc_tt1_sum2 = omc_tt1_offsetramp[2] + omc_tt1_susyaw + omc_tt1_yaw;

// SUM
omc_tt2_sum1 = pLocalEpics->om2.OMC_TT2_PITOFFSET + omc_tt2_suspit + omc_tt2_pit;

// SUM
omc_tt2_sum2 = pLocalEpics->om2.OMC_TT2_YAWOFFSET + omc_tt2_susyaw + omc_tt2_yaw;

// Matrix
for(ii=0;ii<4;ii++)
{
omc_tt1_outmtrx[1][ii] = 
	pLocalEpics->om2.OMC_TT1_OUTMTRX[ii][0] * omc_tt1_sum +
	pLocalEpics->om2.OMC_TT1_OUTMTRX[ii][1] * omc_tt1_sum1 +
	pLocalEpics->om2.OMC_TT1_OUTMTRX[ii][2] * omc_tt1_sum2 +
	pLocalEpics->om2.OMC_TT1_OUTMTRX[ii][3] * omc_tt1_butt_osc[0];
}

// Matrix
for(ii=0;ii<4;ii++)
{
omc_tt2_outmtrx[1][ii] = 
	pLocalEpics->om2.OMC_TT2_OUTMTRX[ii][0] * omc_tt2_sum +
	pLocalEpics->om2.OMC_TT2_OUTMTRX[ii][1] * omc_tt2_sum1 +
	pLocalEpics->om2.OMC_TT2_OUTMTRX[ii][2] * omc_tt2_sum2 +
	pLocalEpics->om2.OMC_TT2_OUTMTRX[ii][3] * omc_tt2_butt_osc[0];
}

// FILTER MODULE
omc_tt1_ul_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_UL_COIL,omc_tt1_outmtrx[1][0],0);

// FILTER MODULE
omc_tt1_lr_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LR_COIL,omc_tt1_outmtrx[1][3],0);

// FILTER MODULE
omc_tt1_ll_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_LL_COIL,omc_tt1_outmtrx[1][1],0);

// FILTER MODULE
omc_tt1_ur_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT1_UR_COIL,omc_tt1_outmtrx[1][2],0);

// FILTER MODULE
omc_tt2_ul_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_UL_COIL,omc_tt2_outmtrx[1][0],0);

// FILTER MODULE
omc_tt2_lr_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LR_COIL,omc_tt2_outmtrx[1][3],0);

// FILTER MODULE
omc_tt2_ll_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_LL_COIL,omc_tt2_outmtrx[1][1],0);

// FILTER MODULE
omc_tt2_ur_coil = filterModuleD(dsp_ptr,dspCoeff,OMC_TT2_UR_COIL,omc_tt2_outmtrx[1][2],0);

// MULTIPLY
omc_tt1_product = omc_tt1_ul_coil * pLocalEpics->om2.OMC_TT1_WD_OUT;

// MULTIPLY
omc_tt1_product3 = omc_tt1_lr_coil * pLocalEpics->om2.OMC_TT1_WD_OUT;

// MULTIPLY
omc_tt1_product1 = omc_tt1_ll_coil * pLocalEpics->om2.OMC_TT1_WD_OUT;

// MULTIPLY
omc_tt1_product2 = omc_tt1_ur_coil * pLocalEpics->om2.OMC_TT1_WD_OUT;

// MULTIPLY
omc_tt2_product = omc_tt2_ul_coil * pLocalEpics->om2.OMC_TT2_WD_OUT;

// MULTIPLY
omc_tt2_product3 = omc_tt2_lr_coil * pLocalEpics->om2.OMC_TT2_WD_OUT;

// MULTIPLY
omc_tt2_product1 = omc_tt2_ll_coil * pLocalEpics->om2.OMC_TT2_WD_OUT;

// MULTIPLY
omc_tt2_product2 = omc_tt2_ur_coil * pLocalEpics->om2.OMC_TT2_WD_OUT;

// Switch
omc_shutter_choice1 = (((omc_shutter_trigger) != 0)? (omc_shutter_gainul): (omc_tt1_product));
// Switch
omc_shutter_choice4 = (((omc_shutter_trigger) != 0)? (omc_shutter_gainlr): (omc_tt1_product3));
// Switch
omc_shutter_choice2 = (((omc_shutter_trigger) != 0)? (omc_shutter_gainll): (omc_tt1_product1));
// Switch
omc_shutter_choice3 = (((omc_shutter_trigger) != 0)? (omc_shutter_gainur): (omc_tt1_product2));

//End of subsystem   OMC **************************************************


ipc_at_0x2020 = omc_shutter_choice1;

ipc_at_0x2028 = omc_shutter_choice2;

ipc_at_0x2030 = omc_shutter_choice3;

ipc_at_0x2038 = omc_shutter_choice4;

ipc_at_0x2040 = omc_tt2_product;

ipc_at_0x2048 = omc_tt2_product1;

ipc_at_0x2050 = omc_tt2_product2;

ipc_at_0x2058 = omc_tt2_product3;

ipc_at_0x20C0 = omc_shutter_trigger;

    // All IPC outputs
    if (_ipc_shm != 0) {
      *((double *)(((char *)_ipc_shm) + 0x2020)) = ipc_at_0x2020;
      *((double *)(((char *)_ipc_shm) + 0x2028)) = ipc_at_0x2028;
      *((double *)(((char *)_ipc_shm) + 0x2030)) = ipc_at_0x2030;
      *((double *)(((char *)_ipc_shm) + 0x2038)) = ipc_at_0x2038;
      *((double *)(((char *)_ipc_shm) + 0x2040)) = ipc_at_0x2040;
      *((double *)(((char *)_ipc_shm) + 0x2048)) = ipc_at_0x2048;
      *((double *)(((char *)_ipc_shm) + 0x2050)) = ipc_at_0x2050;
      *((double *)(((char *)_ipc_shm) + 0x2058)) = ipc_at_0x2058;
      *((double *)(((char *)_ipc_shm) + 0x20C0)) = ipc_at_0x20C0;
    }
  }
}

