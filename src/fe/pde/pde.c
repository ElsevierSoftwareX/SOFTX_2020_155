// ******* This is a computer generated file *******
// ******* DO NOT HAND EDIT ************************


#ifdef SERVO32K
	#define FE_RATE	32768
#endif
#ifdef SERVO16K
	#define FE_RATE	16382
#endif


void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dsp_ptr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii,jj;

double asc_inmtrx[2][4];
double asc_outmtrx[4][2];
double asc_wfs1_i1;
double asc_wfs1_i2;
double asc_wfs1_i3;
double asc_wfs1_i4;
double asc_wfs1_i_mtrx[3][4];
static double asc_wfs1_phase1[2];
static double asc_wfs1_phase2[2];
static double asc_wfs1_phase3[2];
static double asc_wfs1_phase4[2];
double asc_wfs1_pit;
double asc_wfs1_q1;
double asc_wfs1_q2;
double asc_wfs1_q3;
double asc_wfs1_q4;
double asc_wfs1_q_mtrx[3][4];
double asc_wfs1_yaw;
static float ground;
double lsc_as1dc;
double lsc_as1i;
double lsc_as1q;
double lsc_as2dc;
double lsc_as3dc;
double lsc_carm;
double lsc_darm;
double lsc_inmtrx[3][4];
double lsc_lock_dc;
double lsc_outmtrx[6][3];
double lsc_prc;
double lsc_refli;
double lsc_reflq;
double lsc_x_trans;
double lsc_y_trans;
double sus_bs_ascp;
double sus_bs_ascy;
static float sus_bs_ground;
static float sus_bs_ground1;
double sus_bs_inmtrx[3][4];
double sus_bs_llout;
double sus_bs_llpit;
double sus_bs_llpos;
double sus_bs_llsen;
double sus_bs_llyaw;
double sus_bs_lrout;
double sus_bs_lrpit;
double sus_bs_lrpos;
double sus_bs_lrsen;
double sus_bs_lryaw;
double sus_bs_lsc;
double sus_bs_master_sw[5];
double sus_bs_pit;
double sus_bs_pos;
double sus_bs_product;
double sus_bs_product1;
double sus_bs_product2;
double sus_bs_product3;
double sus_bs_product4;
double sus_bs_side;
double sus_bs_sum;
double sus_bs_sum1;
double sus_bs_sum2;
double sus_bs_sum3;
double sus_bs_sum4;
double sus_bs_sum5;
double sus_bs_sum6;
double sus_bs_ulout;
double sus_bs_ulpit;
double sus_bs_ulpos;
double sus_bs_ulsen;
double sus_bs_ulyaw;
double sus_bs_urout;
double sus_bs_urpit;
double sus_bs_urpos;
double sus_bs_ursen;
double sus_bs_uryaw;
static int sus_bs_wd;
static float sus_bs_wd_avg[5];
static float sus_bs_wd_var[5];
float sus_bs_wd_vabs;
double sus_bs_yaw;
double sus_etmx_ascp;
double sus_etmx_ascy;
static float sus_etmx_ground;
static float sus_etmx_ground1;
double sus_etmx_inmtrx[3][4];
double sus_etmx_llout;
double sus_etmx_llpit;
double sus_etmx_llpos;
double sus_etmx_llsen;
double sus_etmx_llyaw;
double sus_etmx_lrout;
double sus_etmx_lrpit;
double sus_etmx_lrpos;
double sus_etmx_lrsen;
double sus_etmx_lryaw;
double sus_etmx_lsc;
double sus_etmx_master_sw[5];
double sus_etmx_pit;
double sus_etmx_pos;
double sus_etmx_product;
double sus_etmx_product1;
double sus_etmx_product2;
double sus_etmx_product3;
double sus_etmx_product4;
double sus_etmx_side;
double sus_etmx_sum;
double sus_etmx_sum1;
double sus_etmx_sum2;
double sus_etmx_sum3;
double sus_etmx_sum4;
double sus_etmx_sum5;
double sus_etmx_sum6;
double sus_etmx_ulout;
double sus_etmx_ulpit;
double sus_etmx_ulpos;
double sus_etmx_ulsen;
double sus_etmx_ulyaw;
double sus_etmx_urout;
double sus_etmx_urpit;
double sus_etmx_urpos;
double sus_etmx_ursen;
double sus_etmx_uryaw;
static int sus_etmx_wd;
static float sus_etmx_wd_avg[5];
static float sus_etmx_wd_var[5];
float sus_etmx_wd_vabs;
double sus_etmx_yaw;
double sus_etmy_ascp;
double sus_etmy_ascy;
static float sus_etmy_ground;
static float sus_etmy_ground1;
double sus_etmy_inmtrx[3][4];
double sus_etmy_llout;
double sus_etmy_llpit;
double sus_etmy_llpos;
double sus_etmy_llsen;
double sus_etmy_llyaw;
double sus_etmy_lrout;
double sus_etmy_lrpit;
double sus_etmy_lrpos;
double sus_etmy_lrsen;
double sus_etmy_lryaw;
double sus_etmy_lsc;
double sus_etmy_master_sw[5];
double sus_etmy_pit;
double sus_etmy_pos;
double sus_etmy_product;
double sus_etmy_product1;
double sus_etmy_product2;
double sus_etmy_product3;
double sus_etmy_product4;
double sus_etmy_side;
double sus_etmy_sum;
double sus_etmy_sum1;
double sus_etmy_sum2;
double sus_etmy_sum3;
double sus_etmy_sum4;
double sus_etmy_sum5;
double sus_etmy_sum6;
double sus_etmy_ulout;
double sus_etmy_ulpit;
double sus_etmy_ulpos;
double sus_etmy_ulsen;
double sus_etmy_ulyaw;
double sus_etmy_urout;
double sus_etmy_urpit;
double sus_etmy_urpos;
double sus_etmy_ursen;
double sus_etmy_uryaw;
static int sus_etmy_wd;
static float sus_etmy_wd_avg[5];
static float sus_etmy_wd_var[5];
float sus_etmy_wd_vabs;
double sus_etmy_yaw;
double sus_itmx_ascp;
double sus_itmx_ascy;
double sus_itmx_inmtrx[3][4];
double sus_itmx_llout;
double sus_itmx_llpit;
double sus_itmx_llpos;
double sus_itmx_llsen;
double sus_itmx_llyaw;
double sus_itmx_lrout;
double sus_itmx_lrpit;
double sus_itmx_lrpos;
double sus_itmx_lrsen;
double sus_itmx_lryaw;
double sus_itmx_lsc;
double sus_itmx_master_sw[5];
double sus_itmx_pit;
double sus_itmx_pos;
double sus_itmx_product;
double sus_itmx_product1;
double sus_itmx_product2;
double sus_itmx_product3;
double sus_itmx_product4;
double sus_itmx_side;
double sus_itmx_sum;
double sus_itmx_sum1;
double sus_itmx_sum2;
double sus_itmx_sum3;
double sus_itmx_sum4;
double sus_itmx_sum5;
double sus_itmx_sum6;
double sus_itmx_ulout;
double sus_itmx_ulpit;
double sus_itmx_ulpos;
double sus_itmx_ulsen;
double sus_itmx_ulyaw;
double sus_itmx_urout;
double sus_itmx_urpit;
double sus_itmx_urpos;
double sus_itmx_ursen;
double sus_itmx_uryaw;
static int sus_itmx_wd;
static float sus_itmx_wd_avg[5];
static float sus_itmx_wd_var[5];
float sus_itmx_wd_vabs;
double sus_itmx_yaw;
double sus_itmy_ascp;
double sus_itmy_ascy;
double sus_itmy_inmtrx[3][4];
double sus_itmy_llout;
double sus_itmy_llpit;
double sus_itmy_llpos;
double sus_itmy_llsen;
double sus_itmy_llyaw;
double sus_itmy_lrout;
double sus_itmy_lrpit;
double sus_itmy_lrpos;
double sus_itmy_lrsen;
double sus_itmy_lryaw;
double sus_itmy_lsc;
double sus_itmy_master_sw[5];
double sus_itmy_pit;
double sus_itmy_pos;
double sus_itmy_product;
double sus_itmy_product1;
double sus_itmy_product2;
double sus_itmy_product3;
double sus_itmy_product4;
double sus_itmy_side;
double sus_itmy_sum;
double sus_itmy_sum1;
double sus_itmy_sum2;
double sus_itmy_sum3;
double sus_itmy_sum4;
double sus_itmy_sum5;
double sus_itmy_sum6;
double sus_itmy_ulout;
double sus_itmy_ulpit;
double sus_itmy_ulpos;
double sus_itmy_ulsen;
double sus_itmy_ulyaw;
double sus_itmy_urout;
double sus_itmy_urpit;
double sus_itmy_urpos;
double sus_itmy_ursen;
double sus_itmy_uryaw;
static int sus_itmy_wd;
static float sus_itmy_wd_avg[5];
static float sus_itmy_wd_var[5];
float sus_itmy_wd_vabs;
double sus_itmy_yaw;
int sus_wd_sum;


if(feInit)
{
ground = 0.0;
sus_bs_ground = 0.0;
sus_bs_ground1 = 0.0;
sus_bs_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_bs_wd_avg[ii] = 0.0;
	sus_bs_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_BS_WD = 1;
sus_etmx_ground = 0.0;
sus_etmx_ground1 = 0.0;
sus_etmx_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_etmx_wd_avg[ii] = 0.0;
	sus_etmx_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ETMX_WD = 1;
sus_etmy_ground = 0.0;
sus_etmy_ground1 = 0.0;
sus_etmy_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_etmy_wd_avg[ii] = 0.0;
	sus_etmy_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ETMY_WD = 1;
sus_itmx_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_itmx_wd_avg[ii] = 0.0;
	sus_itmx_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ITMX_WD = 1;
sus_itmy_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_itmy_wd_avg[ii] = 0.0;
	sus_itmy_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ITMY_WD = 1;
} else {

//Start of subsystem ASC **************************************************

// FILTER MODULE
asc_wfs1_i1 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_I1,dWord[1][16],0);

// FILTER MODULE
asc_wfs1_q1 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_Q1,dWord[1][17],0);

// FILTER MODULE
asc_wfs1_i2 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_I2,dWord[1][18],0);

// FILTER MODULE
asc_wfs1_q2 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_Q2,dWord[1][19],0);

// FILTER MODULE
asc_wfs1_i3 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_I3,dWord[1][20],0);

// FILTER MODULE
asc_wfs1_q3 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_Q3,dWord[1][21],0);

// FILTER MODULE
asc_wfs1_i4 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_I4,dWord[1][22],0);

// FILTER MODULE
asc_wfs1_q4 = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_Q4,dWord[1][23],0);

// WFS PHASE
asc_wfs1_phase1[0] = (asc_wfs1_i1 * pLocalEpics->pde.ASC_WFS1_PHASE1[0][0]) - (asc_wfs1_q1 * pLocalEpics->pde.ASC_WFS1_PHASE1[1][0]);
asc_wfs1_phase1[1] = (asc_wfs1_q1 * pLocalEpics->pde.ASC_WFS1_PHASE1[1][1]) - (asc_wfs1_i1 * pLocalEpics->pde.ASC_WFS1_PHASE1[0][1]);

// WFS PHASE
asc_wfs1_phase2[0] = (asc_wfs1_i2 * pLocalEpics->pde.ASC_WFS1_PHASE2[0][0]) - (asc_wfs1_q2 * pLocalEpics->pde.ASC_WFS1_PHASE2[1][0]);
asc_wfs1_phase2[1] = (asc_wfs1_q2 * pLocalEpics->pde.ASC_WFS1_PHASE2[1][1]) - (asc_wfs1_i2 * pLocalEpics->pde.ASC_WFS1_PHASE2[0][1]);

// WFS PHASE
asc_wfs1_phase3[0] = (asc_wfs1_i3 * pLocalEpics->pde.ASC_WFS1_PHASE3[0][0]) - (asc_wfs1_q3 * pLocalEpics->pde.ASC_WFS1_PHASE3[1][0]);
asc_wfs1_phase3[1] = (asc_wfs1_q3 * pLocalEpics->pde.ASC_WFS1_PHASE3[1][1]) - (asc_wfs1_i3 * pLocalEpics->pde.ASC_WFS1_PHASE3[0][1]);

// WFS PHASE
asc_wfs1_phase4[0] = (asc_wfs1_i4 * pLocalEpics->pde.ASC_WFS1_PHASE4[0][0]) - (asc_wfs1_q4 * pLocalEpics->pde.ASC_WFS1_PHASE4[1][0]);
asc_wfs1_phase4[1] = (asc_wfs1_q4 * pLocalEpics->pde.ASC_WFS1_PHASE4[1][1]) - (asc_wfs1_i4 * pLocalEpics->pde.ASC_WFS1_PHASE4[0][1]);

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q1_MON = asc_wfs1_phase1[1];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I1_MON = asc_wfs1_phase1[0];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q2_MON = asc_wfs1_phase2[1];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I2_MON = asc_wfs1_phase2[0];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q3_MON = asc_wfs1_phase3[1];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I3_MON = asc_wfs1_phase3[0];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q4_MON = asc_wfs1_phase4[1];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I4_MON = asc_wfs1_phase4[0];

// Matrix
for(ii=0;ii<3;ii++)
{
asc_wfs1_q_mtrx[1][ii] = 
	pLocalEpics->pde.ASC_WFS1_Q_MTRX[ii][0] * pLocalEpics->pde.ASC_WFS1_Q1_MON +
	pLocalEpics->pde.ASC_WFS1_Q_MTRX[ii][1] * pLocalEpics->pde.ASC_WFS1_Q2_MON +
	pLocalEpics->pde.ASC_WFS1_Q_MTRX[ii][2] * pLocalEpics->pde.ASC_WFS1_Q3_MON +
	pLocalEpics->pde.ASC_WFS1_Q_MTRX[ii][3] * pLocalEpics->pde.ASC_WFS1_Q4_MON;
}

// Matrix
for(ii=0;ii<3;ii++)
{
asc_wfs1_i_mtrx[1][ii] = 
	pLocalEpics->pde.ASC_WFS1_I_MTRX[ii][0] * pLocalEpics->pde.ASC_WFS1_I1_MON +
	pLocalEpics->pde.ASC_WFS1_I_MTRX[ii][1] * pLocalEpics->pde.ASC_WFS1_I2_MON +
	pLocalEpics->pde.ASC_WFS1_I_MTRX[ii][2] * pLocalEpics->pde.ASC_WFS1_I3_MON +
	pLocalEpics->pde.ASC_WFS1_I_MTRX[ii][3] * pLocalEpics->pde.ASC_WFS1_I4_MON;
}

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q_PIT = asc_wfs1_q_mtrx[1][0];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q_YAW = asc_wfs1_q_mtrx[1][1];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_Q_SUM = asc_wfs1_q_mtrx[1][2];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I_PIT = asc_wfs1_i_mtrx[1][0];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I_YAW = asc_wfs1_i_mtrx[1][1];

// EpicsOut
pLocalEpics->pde.ASC_WFS1_I_SUM = asc_wfs1_i_mtrx[1][2];

// Matrix
for(ii=0;ii<2;ii++)
{
asc_inmtrx[1][ii] = 
	pLocalEpics->pde.ASC_INMTRX[ii][0] * pLocalEpics->pde.ASC_WFS1_I_PIT +
	pLocalEpics->pde.ASC_INMTRX[ii][1] * pLocalEpics->pde.ASC_WFS1_I_YAW +
	pLocalEpics->pde.ASC_INMTRX[ii][2] * pLocalEpics->pde.ASC_WFS1_Q_PIT +
	pLocalEpics->pde.ASC_INMTRX[ii][3] * pLocalEpics->pde.ASC_WFS1_Q_YAW;
}

// FILTER MODULE
asc_wfs1_pit = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_PIT,asc_inmtrx[1][0],0);

// FILTER MODULE
asc_wfs1_yaw = filterModuleD(dsp_ptr,dspCoeff,ASC_WFS1_YAW,asc_inmtrx[1][1],0);

// Matrix
for(ii=0;ii<4;ii++)
{
asc_outmtrx[1][ii] = 
	pLocalEpics->pde.ASC_OUTMTRX[ii][0] * asc_wfs1_pit +
	pLocalEpics->pde.ASC_OUTMTRX[ii][1] * asc_wfs1_yaw;
}


//End of subsystem   ASC **************************************************



//Start of subsystem LSC **************************************************

// FILTER MODULE
lsc_refli = filterModuleD(dsp_ptr,dspCoeff,LSC_REFLI,dWord[0][28],0);

// FILTER MODULE
lsc_reflq = filterModuleD(dsp_ptr,dspCoeff,LSC_REFLQ,dWord[1][1],0);

// FILTER MODULE
lsc_as1i = filterModuleD(dsp_ptr,dspCoeff,LSC_AS1I,dWord[1][2],0);

// FILTER MODULE
lsc_as1q = filterModuleD(dsp_ptr,dspCoeff,LSC_AS1Q,dWord[1][3],0);

// FILTER MODULE
lsc_as1dc = filterModuleD(dsp_ptr,dspCoeff,LSC_AS1DC,dWord[1][4],0);

// FILTER MODULE
lsc_as2dc = filterModuleD(dsp_ptr,dspCoeff,LSC_AS2DC,dWord[1][5],0);

// FILTER MODULE
lsc_as3dc = filterModuleD(dsp_ptr,dspCoeff,LSC_AS3DC,dWord[1][6],0);

// FILTER MODULE
lsc_lock_dc = filterModuleD(dsp_ptr,dspCoeff,LSC_LOCK_DC,dWord[0][28],0);

// FILTER MODULE
lsc_x_trans = filterModuleD(dsp_ptr,dspCoeff,LSC_X_TRANS,dWord[0][29],0);

// FILTER MODULE
lsc_y_trans = filterModuleD(dsp_ptr,dspCoeff,LSC_Y_TRANS,dWord[0][30],0);

// Matrix
for(ii=0;ii<3;ii++)
{
lsc_inmtrx[1][ii] = 
	pLocalEpics->pde.LSC_INMTRX[ii][0] * lsc_refli +
	pLocalEpics->pde.LSC_INMTRX[ii][1] * lsc_reflq +
	pLocalEpics->pde.LSC_INMTRX[ii][2] * lsc_as1i +
	pLocalEpics->pde.LSC_INMTRX[ii][3] * lsc_as1q;
}

// FILTER MODULE
lsc_darm = filterModuleD(dsp_ptr,dspCoeff,LSC_DARM,lsc_inmtrx[1][0],0);

// FILTER MODULE
lsc_carm = filterModuleD(dsp_ptr,dspCoeff,LSC_CARM,lsc_inmtrx[1][1],0);

// FILTER MODULE
lsc_prc = filterModuleD(dsp_ptr,dspCoeff,LSC_PRC,lsc_inmtrx[1][2],0);

// Matrix
for(ii=0;ii<6;ii++)
{
lsc_outmtrx[1][ii] = 
	pLocalEpics->pde.LSC_OUTMTRX[ii][0] * lsc_darm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][1] * lsc_carm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][2] * lsc_prc;
}


//End of subsystem   LSC **************************************************









//Start of subsystem SUS_BS **************************************************

// FILTER MODULE
sus_bs_ulsen = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ULSEN,dWord[0][16],0);

// FILTER MODULE
sus_bs_llsen = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LLSEN,dWord[0][17],0);

// FILTER MODULE
sus_bs_ursen = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_URSEN,dWord[0][18],0);

// FILTER MODULE
sus_bs_lrsen = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LRSEN,dWord[0][19],0);

// FILTER MODULE
sus_bs_side = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_SIDE,dWord[0][21],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_BS_WD == 1) {
	sus_bs_wd = 1;
	pLocalEpics->pde.SUS_BS_WD = 0;
};
double ins[5]= {
	dWord[0][16],
	dWord[0][17],
	dWord[0][18],
	dWord[0][19],
	dWord[0][21],
};
   for(ii=0; ii<5;ii++) {
	sus_bs_wd_avg[ii] = ins[ii] * .00005 + sus_bs_wd_avg[ii] * 0.99995;
	sus_bs_wd_vabs = ins[ii] - sus_bs_wd_avg[ii];
	if(sus_bs_wd_vabs < 0) sus_bs_wd_vabs *= -1.0;
	sus_bs_wd_var[ii] = sus_bs_wd_vabs * .00005 + sus_bs_wd_var[ii] * 0.99995;
	pLocalEpics->pde.SUS_BS_WD_VAR[ii] = sus_bs_wd_var[ii];
	if(sus_bs_wd_var[ii] > pLocalEpics->pde.SUS_BS_WD_MAX) sus_bs_wd = 0;
   }
	pLocalEpics->pde.SUS_BS_WD_STAT = sus_bs_wd;
}

// FILTER MODULE
sus_bs_lsc = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LSC,lsc_outmtrx[1][4],0);

// FILTER MODULE
sus_bs_ascy = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ASCY,sus_bs_ground,0);

// FILTER MODULE
sus_bs_ascp = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ASCP,sus_bs_ground1,0);

// Matrix
for(ii=0;ii<3;ii++)
{
sus_bs_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_BS_INMTRX[ii][0] * sus_bs_ulsen +
	pLocalEpics->pde.SUS_BS_INMTRX[ii][1] * sus_bs_llsen +
	pLocalEpics->pde.SUS_BS_INMTRX[ii][2] * sus_bs_ursen +
	pLocalEpics->pde.SUS_BS_INMTRX[ii][3] * sus_bs_lrsen;
}

// MULTIPLY
sus_bs_product = sus_bs_side * sus_bs_wd;

// FILTER MODULE
sus_bs_pos = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_POS,sus_bs_inmtrx[1][0],0);

// FILTER MODULE
sus_bs_pit = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_PIT,sus_bs_inmtrx[1][1],0);

// FILTER MODULE
sus_bs_yaw = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_YAW,sus_bs_inmtrx[1][2],0);

// SUM
sus_bs_sum4 = sus_bs_lsc + sus_bs_pos;

// SUM
sus_bs_sum5 = sus_bs_pit + sus_bs_ascp;

// SUM
sus_bs_sum6 = sus_bs_yaw + sus_bs_ascy;

// FILTER MODULE
sus_bs_ulpos = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ULPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_llpos = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LLPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_urpos = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_URPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_lrpos = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LRPOS,sus_bs_sum4,0);

// FILTER MODULE
sus_bs_ulpit = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ULPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_llpit = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LLPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_urpit = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_URPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_lrpit = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LRPIT,sus_bs_sum5,0);

// FILTER MODULE
sus_bs_ulyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ULYAW,sus_bs_sum6,0);

// FILTER MODULE
sus_bs_llyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LLYAW,sus_bs_sum6,0);

// FILTER MODULE
sus_bs_uryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_URYAW,sus_bs_sum6,0);

// FILTER MODULE
sus_bs_lryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LRYAW,sus_bs_sum6,0);

// SUM
sus_bs_sum = sus_bs_ulpos + sus_bs_ulpit + sus_bs_ulyaw;

// SUM
sus_bs_sum1 = sus_bs_llpos + sus_bs_llpit + sus_bs_llyaw;

// SUM
sus_bs_sum2 = sus_bs_urpos + sus_bs_urpit + sus_bs_uryaw;

// SUM
sus_bs_sum3 = sus_bs_lrpos + sus_bs_lrpit + sus_bs_lryaw;

// FILTER MODULE
sus_bs_ulout = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_ULOUT,sus_bs_sum,0);

// FILTER MODULE
sus_bs_llout = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LLOUT,sus_bs_sum1,0);

// FILTER MODULE
sus_bs_urout = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_UROUT,sus_bs_sum2,0);

// FILTER MODULE
sus_bs_lrout = filterModuleD(dsp_ptr,dspCoeff,SUS_BS_LROUT,sus_bs_sum3,0);

// MULTIPLY
sus_bs_product1 = sus_bs_ulout * sus_bs_wd;

// MULTIPLY
sus_bs_product2 = sus_bs_llout * sus_bs_wd;

// MULTIPLY
sus_bs_product3 = sus_bs_urout * sus_bs_wd;

// MULTIPLY
sus_bs_product4 = sus_bs_lrout * sus_bs_wd;

// MultiSwitch
sus_bs_master_sw[0] = sus_bs_product1;
sus_bs_master_sw[1] = sus_bs_product2;
sus_bs_master_sw[2] = sus_bs_product3;
sus_bs_master_sw[3] = sus_bs_product4;
sus_bs_master_sw[4] = sus_bs_product;
if (pLocalEpics->pde.SUS_BS_MASTER_SW == 0) {
	for (ii=0; ii<5; ii++) sus_bs_master_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->pde.SUS_BS_UL_DRV = sus_bs_master_sw[0];

// EpicsOut
pLocalEpics->pde.SUS_BS_LL_DRV = sus_bs_master_sw[1];

// EpicsOut
pLocalEpics->pde.SUS_BS_UR_DRV = sus_bs_master_sw[2];

// EpicsOut
pLocalEpics->pde.SUS_BS_LR_DRV = sus_bs_master_sw[3];

// EpicsOut
pLocalEpics->pde.SUS_BS_SD_DRV = sus_bs_master_sw[4];


//End of subsystem   SUS_BS **************************************************



//Start of subsystem SUS_ETMX **************************************************

// FILTER MODULE
sus_etmx_ulsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ULSEN,dWord[0][8],0);

// FILTER MODULE
sus_etmx_llsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LLSEN,dWord[0][9],0);

// FILTER MODULE
sus_etmx_ursen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_URSEN,dWord[0][10],0);

// FILTER MODULE
sus_etmx_lrsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LRSEN,dWord[0][11],0);

// FILTER MODULE
sus_etmx_side = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_SIDE,dWord[0][24],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ETMX_WD == 1) {
	sus_etmx_wd = 1;
	pLocalEpics->pde.SUS_ETMX_WD = 0;
};
double ins[5]= {
	dWord[0][8],
	dWord[0][9],
	dWord[0][10],
	dWord[0][11],
	dWord[0][24],
};
   for(ii=0; ii<5;ii++) {
	sus_etmx_wd_avg[ii] = ins[ii] * .00005 + sus_etmx_wd_avg[ii] * 0.99995;
	sus_etmx_wd_vabs = ins[ii] - sus_etmx_wd_avg[ii];
	if(sus_etmx_wd_vabs < 0) sus_etmx_wd_vabs *= -1.0;
	sus_etmx_wd_var[ii] = sus_etmx_wd_vabs * .00005 + sus_etmx_wd_var[ii] * 0.99995;
	pLocalEpics->pde.SUS_ETMX_WD_VAR[ii] = sus_etmx_wd_var[ii];
	if(sus_etmx_wd_var[ii] > pLocalEpics->pde.SUS_ETMX_WD_MAX) sus_etmx_wd = 0;
   }
	pLocalEpics->pde.SUS_ETMX_WD_STAT = sus_etmx_wd;
}

// FILTER MODULE
sus_etmx_lsc = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LSC,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_ascy = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ASCY,sus_etmx_ground,0);

// FILTER MODULE
sus_etmx_ascp = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ASCP,sus_etmx_ground1,0);

// Matrix
for(ii=0;ii<3;ii++)
{
sus_etmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][0] * sus_etmx_ulsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][1] * sus_etmx_llsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][2] * sus_etmx_ursen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][3] * sus_etmx_lrsen;
}

// MULTIPLY
sus_etmx_product = sus_etmx_side * sus_etmx_wd;

// FILTER MODULE
sus_etmx_pos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_POS,sus_etmx_inmtrx[1][0],0);

// FILTER MODULE
sus_etmx_pit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_PIT,sus_etmx_inmtrx[1][1],0);

// FILTER MODULE
sus_etmx_yaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_YAW,sus_etmx_inmtrx[1][2],0);

// SUM
sus_etmx_sum4 = sus_etmx_lsc + sus_etmx_pos;

// SUM
sus_etmx_sum5 = sus_etmx_pit + sus_etmx_ascp;

// SUM
sus_etmx_sum6 = sus_etmx_yaw + sus_etmx_ascy;

// FILTER MODULE
sus_etmx_ulpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ULPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_llpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LLPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_urpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_URPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_lrpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LRPOS,sus_etmx_sum4,0);

// FILTER MODULE
sus_etmx_ulpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ULPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_llpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LLPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_urpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_URPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_lrpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LRPIT,sus_etmx_sum5,0);

// FILTER MODULE
sus_etmx_ulyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ULYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_llyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LLYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_uryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_URYAW,sus_etmx_sum6,0);

// FILTER MODULE
sus_etmx_lryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LRYAW,sus_etmx_sum6,0);

// SUM
sus_etmx_sum = sus_etmx_ulpos + sus_etmx_ulpit + sus_etmx_ulyaw;

// SUM
sus_etmx_sum1 = sus_etmx_llpos + sus_etmx_llpit + sus_etmx_llyaw;

// SUM
sus_etmx_sum2 = sus_etmx_urpos + sus_etmx_urpit + sus_etmx_uryaw;

// SUM
sus_etmx_sum3 = sus_etmx_lrpos + sus_etmx_lrpit + sus_etmx_lryaw;

// FILTER MODULE
sus_etmx_ulout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_ULOUT,sus_etmx_sum,0);

// FILTER MODULE
sus_etmx_llout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LLOUT,sus_etmx_sum1,0);

// FILTER MODULE
sus_etmx_urout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_UROUT,sus_etmx_sum2,0);

// FILTER MODULE
sus_etmx_lrout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMX_LROUT,sus_etmx_sum3,0);

// MULTIPLY
sus_etmx_product1 = sus_etmx_ulout * sus_etmx_wd;

// MULTIPLY
sus_etmx_product2 = sus_etmx_llout * sus_etmx_wd;

// MULTIPLY
sus_etmx_product3 = sus_etmx_urout * sus_etmx_wd;

// MULTIPLY
sus_etmx_product4 = sus_etmx_lrout * sus_etmx_wd;

// MultiSwitch
sus_etmx_master_sw[0] = sus_etmx_product1;
sus_etmx_master_sw[1] = sus_etmx_product2;
sus_etmx_master_sw[2] = sus_etmx_product3;
sus_etmx_master_sw[3] = sus_etmx_product4;
sus_etmx_master_sw[4] = sus_etmx_product;
if (pLocalEpics->pde.SUS_ETMX_MASTER_SW == 0) {
	for (ii=0; ii<5; ii++) sus_etmx_master_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->pde.SUS_ETMX_UL_DRV = sus_etmx_master_sw[0];

// EpicsOut
pLocalEpics->pde.SUS_ETMX_LL_DRV = sus_etmx_master_sw[1];

// EpicsOut
pLocalEpics->pde.SUS_ETMX_UR_DRV = sus_etmx_master_sw[2];

// EpicsOut
pLocalEpics->pde.SUS_ETMX_LR_DRV = sus_etmx_master_sw[3];

// EpicsOut
pLocalEpics->pde.SUS_ETMX_SD_DRV = sus_etmx_master_sw[4];


//End of subsystem   SUS_ETMX **************************************************



//Start of subsystem SUS_ETMY **************************************************

// FILTER MODULE
sus_etmy_ulsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ULSEN,dWord[0][12],0);

// FILTER MODULE
sus_etmy_llsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LLSEN,dWord[0][13],0);

// FILTER MODULE
sus_etmy_ursen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_URSEN,dWord[0][14],0);

// FILTER MODULE
sus_etmy_lrsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LRSEN,dWord[0][15],0);

// FILTER MODULE
sus_etmy_side = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_SIDE,dWord[0][25],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ETMY_WD == 1) {
	sus_etmy_wd = 1;
	pLocalEpics->pde.SUS_ETMY_WD = 0;
};
double ins[5]= {
	dWord[0][12],
	dWord[0][13],
	dWord[0][14],
	dWord[0][15],
	dWord[0][25],
};
   for(ii=0; ii<5;ii++) {
	sus_etmy_wd_avg[ii] = ins[ii] * .00005 + sus_etmy_wd_avg[ii] * 0.99995;
	sus_etmy_wd_vabs = ins[ii] - sus_etmy_wd_avg[ii];
	if(sus_etmy_wd_vabs < 0) sus_etmy_wd_vabs *= -1.0;
	sus_etmy_wd_var[ii] = sus_etmy_wd_vabs * .00005 + sus_etmy_wd_var[ii] * 0.99995;
	pLocalEpics->pde.SUS_ETMY_WD_VAR[ii] = sus_etmy_wd_var[ii];
	if(sus_etmy_wd_var[ii] > pLocalEpics->pde.SUS_ETMY_WD_MAX) sus_etmy_wd = 0;
   }
	pLocalEpics->pde.SUS_ETMY_WD_STAT = sus_etmy_wd;
}

// FILTER MODULE
sus_etmy_lsc = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LSC,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_ascy = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ASCY,sus_etmy_ground,0);

// FILTER MODULE
sus_etmy_ascp = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ASCP,sus_etmy_ground1,0);

// Matrix
for(ii=0;ii<3;ii++)
{
sus_etmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][0] * sus_etmy_ulsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][1] * sus_etmy_llsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][2] * sus_etmy_ursen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][3] * sus_etmy_lrsen;
}

// MULTIPLY
sus_etmy_product = sus_etmy_side * sus_etmy_wd;

// FILTER MODULE
sus_etmy_pos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_POS,sus_etmy_inmtrx[1][0],0);

// FILTER MODULE
sus_etmy_pit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_PIT,sus_etmy_inmtrx[1][1],0);

// FILTER MODULE
sus_etmy_yaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_YAW,sus_etmy_inmtrx[1][2],0);

// SUM
sus_etmy_sum4 = sus_etmy_lsc + sus_etmy_pos;

// SUM
sus_etmy_sum5 = sus_etmy_pit + sus_etmy_ascp;

// SUM
sus_etmy_sum6 = sus_etmy_yaw + sus_etmy_ascy;

// FILTER MODULE
sus_etmy_ulpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ULPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_llpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LLPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_urpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_URPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_lrpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LRPOS,sus_etmy_sum4,0);

// FILTER MODULE
sus_etmy_ulpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ULPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_llpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LLPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_urpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_URPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_lrpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LRPIT,sus_etmy_sum5,0);

// FILTER MODULE
sus_etmy_ulyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ULYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_llyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LLYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_uryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_URYAW,sus_etmy_sum6,0);

// FILTER MODULE
sus_etmy_lryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LRYAW,sus_etmy_sum6,0);

// SUM
sus_etmy_sum = sus_etmy_ulpos + sus_etmy_ulpit + sus_etmy_ulyaw;

// SUM
sus_etmy_sum1 = sus_etmy_llpos + sus_etmy_llpit + sus_etmy_llyaw;

// SUM
sus_etmy_sum2 = sus_etmy_urpos + sus_etmy_urpit + sus_etmy_uryaw;

// SUM
sus_etmy_sum3 = sus_etmy_lrpos + sus_etmy_lrpit + sus_etmy_lryaw;

// FILTER MODULE
sus_etmy_ulout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_ULOUT,sus_etmy_sum,0);

// FILTER MODULE
sus_etmy_llout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LLOUT,sus_etmy_sum1,0);

// FILTER MODULE
sus_etmy_urout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_UROUT,sus_etmy_sum2,0);

// FILTER MODULE
sus_etmy_lrout = filterModuleD(dsp_ptr,dspCoeff,SUS_ETMY_LROUT,sus_etmy_sum3,0);

// MULTIPLY
sus_etmy_product1 = sus_etmy_ulout * sus_etmy_wd;

// MULTIPLY
sus_etmy_product2 = sus_etmy_llout * sus_etmy_wd;

// MULTIPLY
sus_etmy_product3 = sus_etmy_urout * sus_etmy_wd;

// MULTIPLY
sus_etmy_product4 = sus_etmy_lrout * sus_etmy_wd;

// MultiSwitch
sus_etmy_master_sw[0] = sus_etmy_product1;
sus_etmy_master_sw[1] = sus_etmy_product2;
sus_etmy_master_sw[2] = sus_etmy_product3;
sus_etmy_master_sw[3] = sus_etmy_product4;
sus_etmy_master_sw[4] = sus_etmy_product;
if (pLocalEpics->pde.SUS_ETMY_MASTER_SW == 0) {
	for (ii=0; ii<5; ii++) sus_etmy_master_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->pde.SUS_ETMY_UL_DRV = sus_etmy_master_sw[0];

// EpicsOut
pLocalEpics->pde.SUS_ETMY_LL_DRV = sus_etmy_master_sw[1];

// EpicsOut
pLocalEpics->pde.SUS_ETMY_UR_DRV = sus_etmy_master_sw[2];

// EpicsOut
pLocalEpics->pde.SUS_ETMY_LR_DRV = sus_etmy_master_sw[3];

// EpicsOut
pLocalEpics->pde.SUS_ETMY_SD_DRV = sus_etmy_master_sw[4];


//End of subsystem   SUS_ETMY **************************************************



//Start of subsystem SUS_ITMX **************************************************

// FILTER MODULE
sus_itmx_ulsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ULSEN,dWord[0][0],0);

// FILTER MODULE
sus_itmx_llsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LLSEN,dWord[0][1],0);

// FILTER MODULE
sus_itmx_ursen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_URSEN,dWord[0][2],0);

// FILTER MODULE
sus_itmx_lrsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LRSEN,dWord[0][3],0);

// FILTER MODULE
sus_itmx_side = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_SIDE,dWord[0][23],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ITMX_WD == 1) {
	sus_itmx_wd = 1;
	pLocalEpics->pde.SUS_ITMX_WD = 0;
};
double ins[5]= {
	dWord[0][0],
	dWord[0][1],
	dWord[0][2],
	dWord[0][3],
	dWord[0][23],
};
   for(ii=0; ii<5;ii++) {
	sus_itmx_wd_avg[ii] = ins[ii] * .00005 + sus_itmx_wd_avg[ii] * 0.99995;
	sus_itmx_wd_vabs = ins[ii] - sus_itmx_wd_avg[ii];
	if(sus_itmx_wd_vabs < 0) sus_itmx_wd_vabs *= -1.0;
	sus_itmx_wd_var[ii] = sus_itmx_wd_vabs * .00005 + sus_itmx_wd_var[ii] * 0.99995;
	pLocalEpics->pde.SUS_ITMX_WD_VAR[ii] = sus_itmx_wd_var[ii];
	if(sus_itmx_wd_var[ii] > pLocalEpics->pde.SUS_ITMX_WD_MAX) sus_itmx_wd = 0;
   }
	pLocalEpics->pde.SUS_ITMX_WD_STAT = sus_itmx_wd;
}

// FILTER MODULE
sus_itmx_lsc = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LSC,lsc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmx_ascp = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ASCP,asc_outmtrx[1][0],0);

// FILTER MODULE
sus_itmx_ascy = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ASCY,asc_outmtrx[1][1],0);

// Matrix
for(ii=0;ii<3;ii++)
{
sus_itmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][0] * sus_itmx_ulsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][1] * sus_itmx_llsen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][2] * sus_itmx_ursen +
	pLocalEpics->pde.SUS_ITMX_INMTRX[ii][3] * sus_itmx_lrsen;
}

// MULTIPLY
sus_itmx_product = sus_itmx_side * sus_itmx_wd;

// FILTER MODULE
sus_itmx_pos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_POS,sus_itmx_inmtrx[1][0],0);

// FILTER MODULE
sus_itmx_pit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_PIT,sus_itmx_inmtrx[1][1],0);

// FILTER MODULE
sus_itmx_yaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_YAW,sus_itmx_inmtrx[1][2],0);

// SUM
sus_itmx_sum4 = sus_itmx_lsc + sus_itmx_pos;

// SUM
sus_itmx_sum5 = sus_itmx_pit + sus_itmx_ascp;

// SUM
sus_itmx_sum6 = sus_itmx_yaw + sus_itmx_ascy;

// FILTER MODULE
sus_itmx_ulpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ULPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_llpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LLPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_urpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_URPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_lrpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LRPOS,sus_itmx_sum4,0);

// FILTER MODULE
sus_itmx_ulpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ULPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_llpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LLPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_urpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_URPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_lrpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LRPIT,sus_itmx_sum5,0);

// FILTER MODULE
sus_itmx_ulyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ULYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_llyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LLYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_uryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_URYAW,sus_itmx_sum6,0);

// FILTER MODULE
sus_itmx_lryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LRYAW,sus_itmx_sum6,0);

// SUM
sus_itmx_sum = sus_itmx_ulpos + sus_itmx_ulpit + sus_itmx_ulyaw;

// SUM
sus_itmx_sum1 = sus_itmx_llpos + sus_itmx_llpit + sus_itmx_llyaw;

// SUM
sus_itmx_sum2 = sus_itmx_urpos + sus_itmx_urpit + sus_itmx_uryaw;

// SUM
sus_itmx_sum3 = sus_itmx_lrpos + sus_itmx_lrpit + sus_itmx_lryaw;

// FILTER MODULE
sus_itmx_ulout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_ULOUT,sus_itmx_sum,0);

// FILTER MODULE
sus_itmx_llout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LLOUT,sus_itmx_sum1,0);

// FILTER MODULE
sus_itmx_urout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_UROUT,sus_itmx_sum2,0);

// FILTER MODULE
sus_itmx_lrout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMX_LROUT,sus_itmx_sum3,0);

// MULTIPLY
sus_itmx_product1 = sus_itmx_ulout * sus_itmx_wd;

// MULTIPLY
sus_itmx_product2 = sus_itmx_llout * sus_itmx_wd;

// MULTIPLY
sus_itmx_product3 = sus_itmx_urout * sus_itmx_wd;

// MULTIPLY
sus_itmx_product4 = sus_itmx_lrout * sus_itmx_wd;

// MultiSwitch
sus_itmx_master_sw[0] = sus_itmx_product1;
sus_itmx_master_sw[1] = sus_itmx_product2;
sus_itmx_master_sw[2] = sus_itmx_product3;
sus_itmx_master_sw[3] = sus_itmx_product4;
sus_itmx_master_sw[4] = sus_itmx_product;
if (pLocalEpics->pde.SUS_ITMX_MASTER_SW == 0) {
	for (ii=0; ii<5; ii++) sus_itmx_master_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->pde.SUS_ITMX_UL_DRV = sus_itmx_master_sw[0];

// EpicsOut
pLocalEpics->pde.SUS_ITMX_LL_DRV = sus_itmx_master_sw[1];

// EpicsOut
pLocalEpics->pde.SUS_ITMX_UR_DRV = sus_itmx_master_sw[2];

// EpicsOut
pLocalEpics->pde.SUS_ITMX_LR_DRV = sus_itmx_master_sw[3];

// EpicsOut
pLocalEpics->pde.SUS_ITMX_SD_DRV = sus_itmx_master_sw[4];


//End of subsystem   SUS_ITMX **************************************************



//Start of subsystem SUS_ITMY **************************************************

// FILTER MODULE
sus_itmy_ulsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ULSEN,dWord[0][4],0);

// FILTER MODULE
sus_itmy_llsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LLSEN,dWord[0][5],0);

// FILTER MODULE
sus_itmy_ursen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_URSEN,dWord[0][6],0);

// FILTER MODULE
sus_itmy_lrsen = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LRSEN,dWord[0][7],0);

// FILTER MODULE
sus_itmy_side = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_SIDE,dWord[0][22],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ITMY_WD == 1) {
	sus_itmy_wd = 1;
	pLocalEpics->pde.SUS_ITMY_WD = 0;
};
double ins[5]= {
	dWord[0][4],
	dWord[0][5],
	dWord[0][6],
	dWord[0][7],
	dWord[0][22],
};
   for(ii=0; ii<5;ii++) {
	sus_itmy_wd_avg[ii] = ins[ii] * .00005 + sus_itmy_wd_avg[ii] * 0.99995;
	sus_itmy_wd_vabs = ins[ii] - sus_itmy_wd_avg[ii];
	if(sus_itmy_wd_vabs < 0) sus_itmy_wd_vabs *= -1.0;
	sus_itmy_wd_var[ii] = sus_itmy_wd_vabs * .00005 + sus_itmy_wd_var[ii] * 0.99995;
	pLocalEpics->pde.SUS_ITMY_WD_VAR[ii] = sus_itmy_wd_var[ii];
	if(sus_itmy_wd_var[ii] > pLocalEpics->pde.SUS_ITMY_WD_MAX) sus_itmy_wd = 0;
   }
	pLocalEpics->pde.SUS_ITMY_WD_STAT = sus_itmy_wd;
}

// FILTER MODULE
sus_itmy_lsc = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LSC,lsc_outmtrx[1][3],0);

// FILTER MODULE
sus_itmy_ascp = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ASCP,asc_outmtrx[1][2],0);

// FILTER MODULE
sus_itmy_ascy = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ASCY,asc_outmtrx[1][3],0);

// Matrix
for(ii=0;ii<3;ii++)
{
sus_itmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][0] * sus_itmy_ulsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][1] * sus_itmy_llsen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][2] * sus_itmy_ursen +
	pLocalEpics->pde.SUS_ITMY_INMTRX[ii][3] * sus_itmy_lrsen;
}

// MULTIPLY
sus_itmy_product = sus_itmy_side * sus_itmy_wd;

// FILTER MODULE
sus_itmy_pos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_POS,sus_itmy_inmtrx[1][0],0);

// FILTER MODULE
sus_itmy_pit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_PIT,sus_itmy_inmtrx[1][1],0);

// FILTER MODULE
sus_itmy_yaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_YAW,sus_itmy_inmtrx[1][2],0);

// SUM
sus_itmy_sum4 = sus_itmy_lsc + sus_itmy_pos;

// SUM
sus_itmy_sum5 = sus_itmy_pit + sus_itmy_ascp;

// SUM
sus_itmy_sum6 = sus_itmy_yaw + sus_itmy_ascy;

// FILTER MODULE
sus_itmy_ulpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ULPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_llpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LLPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_urpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_URPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_lrpos = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LRPOS,sus_itmy_sum4,0);

// FILTER MODULE
sus_itmy_ulpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ULPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_llpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LLPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_urpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_URPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_lrpit = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LRPIT,sus_itmy_sum5,0);

// FILTER MODULE
sus_itmy_ulyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ULYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_llyaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LLYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_uryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_URYAW,sus_itmy_sum6,0);

// FILTER MODULE
sus_itmy_lryaw = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LRYAW,sus_itmy_sum6,0);

// SUM
sus_itmy_sum = sus_itmy_ulpos + sus_itmy_ulpit + sus_itmy_ulyaw;

// SUM
sus_itmy_sum1 = sus_itmy_llpos + sus_itmy_llpit + sus_itmy_llyaw;

// SUM
sus_itmy_sum2 = sus_itmy_urpos + sus_itmy_urpit + sus_itmy_uryaw;

// SUM
sus_itmy_sum3 = sus_itmy_lrpos + sus_itmy_lrpit + sus_itmy_lryaw;

// FILTER MODULE
sus_itmy_ulout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_ULOUT,sus_itmy_sum,0);

// FILTER MODULE
sus_itmy_llout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LLOUT,sus_itmy_sum1,0);

// FILTER MODULE
sus_itmy_urout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_UROUT,sus_itmy_sum2,0);

// FILTER MODULE
sus_itmy_lrout = filterModuleD(dsp_ptr,dspCoeff,SUS_ITMY_LROUT,sus_itmy_sum3,0);

// MULTIPLY
sus_itmy_product1 = sus_itmy_ulout * sus_itmy_wd;

// MULTIPLY
sus_itmy_product2 = sus_itmy_llout * sus_itmy_wd;

// MULTIPLY
sus_itmy_product3 = sus_itmy_urout * sus_itmy_wd;

// MULTIPLY
sus_itmy_product4 = sus_itmy_lrout * sus_itmy_wd;

// MultiSwitch
sus_itmy_master_sw[0] = sus_itmy_product1;
sus_itmy_master_sw[1] = sus_itmy_product2;
sus_itmy_master_sw[2] = sus_itmy_product3;
sus_itmy_master_sw[3] = sus_itmy_product4;
sus_itmy_master_sw[4] = sus_itmy_product;
if (pLocalEpics->pde.SUS_ITMY_MASTER_SW == 0) {
	for (ii=0; ii<5; ii++) sus_itmy_master_sw[ii] = 0.0;
}


// EpicsOut
pLocalEpics->pde.SUS_ITMY_UL_DRV = sus_itmy_master_sw[0];

// EpicsOut
pLocalEpics->pde.SUS_ITMY_LL_DRV = sus_itmy_master_sw[1];

// EpicsOut
pLocalEpics->pde.SUS_ITMY_UR_DRV = sus_itmy_master_sw[2];

// EpicsOut
pLocalEpics->pde.SUS_ITMY_LR_DRV = sus_itmy_master_sw[3];

// EpicsOut
pLocalEpics->pde.SUS_ITMY_SD_DRV = sus_itmy_master_sw[4];


//End of subsystem   SUS_ITMY **************************************************


// DAC number is 0
dacOut[0][0] = pLocalEpics->pde.SUS_ITMX_UL_DRV;
dacOut[0][1] = pLocalEpics->pde.SUS_ITMX_LL_DRV;
dacOut[0][2] = pLocalEpics->pde.SUS_ITMX_UR_DRV;
dacOut[0][3] = pLocalEpics->pde.SUS_ITMX_LR_DRV;
dacOut[0][4] = pLocalEpics->pde.SUS_ITMY_UL_DRV;
dacOut[0][5] = pLocalEpics->pde.SUS_ITMY_LL_DRV;
dacOut[0][6] = pLocalEpics->pde.SUS_ITMY_UR_DRV;
dacOut[0][7] = pLocalEpics->pde.SUS_ITMY_LR_DRV;
dacOut[0][8] = pLocalEpics->pde.SUS_ETMX_UL_DRV;
dacOut[0][9] = pLocalEpics->pde.SUS_ETMX_LL_DRV;
dacOut[0][10] = pLocalEpics->pde.SUS_ETMX_UR_DRV;
dacOut[0][11] = pLocalEpics->pde.SUS_ETMX_LR_DRV;
dacOut[0][12] = pLocalEpics->pde.SUS_ETMY_UL_DRV;
dacOut[0][13] = pLocalEpics->pde.SUS_ETMY_LL_DRV;
dacOut[0][14] = pLocalEpics->pde.SUS_ETMY_UR_DRV;
dacOut[0][15] = pLocalEpics->pde.SUS_ETMY_LR_DRV;

// DAC number is 1
dacOut[1][0] = pLocalEpics->pde.SUS_BS_UL_DRV;
dacOut[1][1] = pLocalEpics->pde.SUS_BS_LL_DRV;
dacOut[1][2] = pLocalEpics->pde.SUS_BS_UR_DRV;
dacOut[1][3] = pLocalEpics->pde.SUS_BS_LR_DRV;
dacOut[1][5] = pLocalEpics->pde.SUS_BS_SD_DRV;
dacOut[1][6] = pLocalEpics->pde.SUS_ITMY_SD_DRV;
dacOut[1][7] = pLocalEpics->pde.SUS_ITMX_SD_DRV;
dacOut[1][8] = pLocalEpics->pde.SUS_ETMX_SD_DRV;
dacOut[1][9] = pLocalEpics->pde.SUS_ETMY_SD_DRV;
dacOut[1][10] = lsc_outmtrx[1][5];
dacOut[1][11] = lsc_lock_dc;
dacOut[1][12] = pLocalEpics->pde.PZT_M1_PIT;
dacOut[1][13] = pLocalEpics->pde.PZT_M1_YAW;
dacOut[1][14] = pLocalEpics->pde.PZT_M2_PIT;
dacOut[1][15] = pLocalEpics->pde.PZT_M2_YAW;

// Logical AND
sus_wd_sum = sus_etmx_wd && sus_etmy_wd && sus_itmx_wd && sus_itmy_wd && sus_bs_wd;

// RemoteIntlk
pLocalEpics->pde.SEI_HMY_ACT_SW = sus_wd_sum;

  }
}

