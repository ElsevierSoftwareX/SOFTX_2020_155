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
	{GSC_16AI64SSA,0},
	{GSC_16AI64SSA,1},
	{GSC_16AO16,0},
	{GSC_16AO16,1},
};

/* Multi-cpu synchronization primitives */
volatile int go2 = 0;
volatile int done2 = 0;
volatile int go3 = 0;
volatile int done3 = 0;

double asc_align_etmx_ascp;
double asc_align_etmx_ascy;
static double asc_align_etmx_pit_osc[3];
static double asc_align_etmx_pit_osc_freq;
static double asc_align_etmx_pit_osc_delta;
static double asc_align_etmx_pit_osc_alpha;
static double asc_align_etmx_pit_osc_beta;
static double asc_align_etmx_pit_osc_cos_prev;
static double asc_align_etmx_pit_osc_sin_prev;
static double asc_align_etmx_pit_osc_cos_new;
static double asc_align_etmx_pit_osc_sin_new;
double lsinx, lcosx, valx;
static double asc_align_etmx_pit_rot[2];
static double asc_align_etmx_yaw_osc[3];
static double asc_align_etmx_yaw_osc_freq;
static double asc_align_etmx_yaw_osc_delta;
static double asc_align_etmx_yaw_osc_alpha;
static double asc_align_etmx_yaw_osc_beta;
static double asc_align_etmx_yaw_osc_cos_prev;
static double asc_align_etmx_yaw_osc_sin_prev;
static double asc_align_etmx_yaw_osc_cos_new;
static double asc_align_etmx_yaw_osc_sin_new;
static double asc_align_etmx_yaw_rot[2];
double asc_align_etmy_ascp;
double asc_align_etmy_ascy;
static double asc_align_etmy_pit_osc[3];
static double asc_align_etmy_pit_osc_freq;
static double asc_align_etmy_pit_osc_delta;
static double asc_align_etmy_pit_osc_alpha;
static double asc_align_etmy_pit_osc_beta;
static double asc_align_etmy_pit_osc_cos_prev;
static double asc_align_etmy_pit_osc_sin_prev;
static double asc_align_etmy_pit_osc_cos_new;
static double asc_align_etmy_pit_osc_sin_new;
static double asc_align_etmy_pit_rot[2];
static double asc_align_etmy_yaw_osc[3];
static double asc_align_etmy_yaw_osc_freq;
static double asc_align_etmy_yaw_osc_delta;
static double asc_align_etmy_yaw_osc_alpha;
static double asc_align_etmy_yaw_osc_beta;
static double asc_align_etmy_yaw_osc_cos_prev;
static double asc_align_etmy_yaw_osc_sin_prev;
static double asc_align_etmy_yaw_osc_cos_new;
static double asc_align_etmy_yaw_osc_sin_new;
static double asc_align_etmy_yaw_rot[2];
static float asc_align_ground;
static float asc_align_ground1;
static float asc_align_ground2;
static float asc_align_ground3;
static float asc_align_ground4;
static float asc_align_ground5;
static float asc_align_ground6;
static float asc_align_ground7;
double asc_align_itmx_ascp;
double asc_align_itmx_ascy;
static double asc_align_itmx_pit_osc[3];
static double asc_align_itmx_pit_osc_freq;
static double asc_align_itmx_pit_osc_delta;
static double asc_align_itmx_pit_osc_alpha;
static double asc_align_itmx_pit_osc_beta;
static double asc_align_itmx_pit_osc_cos_prev;
static double asc_align_itmx_pit_osc_sin_prev;
static double asc_align_itmx_pit_osc_cos_new;
static double asc_align_itmx_pit_osc_sin_new;
static double asc_align_itmx_pit_rot[2];
static double asc_align_itmx_yaw_osc[3];
static double asc_align_itmx_yaw_osc_freq;
static double asc_align_itmx_yaw_osc_delta;
static double asc_align_itmx_yaw_osc_alpha;
static double asc_align_itmx_yaw_osc_beta;
static double asc_align_itmx_yaw_osc_cos_prev;
static double asc_align_itmx_yaw_osc_sin_prev;
static double asc_align_itmx_yaw_osc_cos_new;
static double asc_align_itmx_yaw_osc_sin_new;
static double asc_align_itmx_yaw_rot[2];
double asc_align_itmy_ascp;
double asc_align_itmy_ascy;
static double asc_align_itmy_pit_osc[3];
static double asc_align_itmy_pit_osc_freq;
static double asc_align_itmy_pit_osc_delta;
static double asc_align_itmy_pit_osc_alpha;
static double asc_align_itmy_pit_osc_beta;
static double asc_align_itmy_pit_osc_cos_prev;
static double asc_align_itmy_pit_osc_sin_prev;
static double asc_align_itmy_pit_osc_cos_new;
static double asc_align_itmy_pit_osc_sin_new;
static double asc_align_itmy_pit_rot[2];
static double asc_align_itmy_yaw_osc[3];
static double asc_align_itmy_yaw_osc_freq;
static double asc_align_itmy_yaw_osc_delta;
static double asc_align_itmy_yaw_osc_alpha;
static double asc_align_itmy_yaw_osc_beta;
static double asc_align_itmy_yaw_osc_cos_prev;
static double asc_align_itmy_yaw_osc_sin_prev;
static double asc_align_itmy_yaw_osc_cos_new;
static double asc_align_itmy_yaw_osc_sin_new;
static double asc_align_itmy_yaw_rot[2];
double asc_align_product;
double asc_align_product1;
double asc_align_product10;
double asc_align_product11;
double asc_align_product12;
double asc_align_product13;
double asc_align_product14;
double asc_align_product15;
double asc_align_product2;
double asc_align_product3;
double asc_align_product4;
double asc_align_product5;
double asc_align_product6;
double asc_align_product7;
double asc_align_product8;
double asc_align_product9;
static float asc_bs_ground;
static float asc_bs_ground1;
double asc_bs_matrix[2][3];
double asc_bs_pit;
double asc_bs_product;
double asc_bs_product1;
static int asc_bs_watchdog;
static float asc_bs_watchdog_avg[5];
static float asc_bs_watchdog_var[5];
float asc_bs_watchdog_vabs;
double asc_bs_yaw1;
static float asc_etmx_ground;
static float asc_etmx_ground1;
double asc_etmx_matrix[4][3];
double asc_etmx_pit1;
double asc_etmx_pit2;
double asc_etmx_pit3;
double asc_etmx_pos;
double asc_etmx_product;
double asc_etmx_product1;
double asc_etmx_product2;
double asc_etmx_side;
double asc_etmx_sum1;
double asc_etmx_sum7;
static int asc_etmx_watchdog;
static float asc_etmx_watchdog_avg[5];
static float asc_etmx_watchdog_var[5];
float asc_etmx_watchdog_vabs;
double asc_etmx_yaw1;
double asc_etmx_yaw2;
double asc_etmx_yaw3;
static float asc_etmy_ground;
static float asc_etmy_ground1;
double asc_etmy_matrix[4][3];
double asc_etmy_pit1;
double asc_etmy_pit2;
double asc_etmy_pit3;
double asc_etmy_pos;
double asc_etmy_product;
double asc_etmy_product1;
double asc_etmy_product2;
double asc_etmy_side;
double asc_etmy_sum1;
double asc_etmy_sum7;
static int asc_etmy_watchdog;
static float asc_etmy_watchdog_avg[5];
static float asc_etmy_watchdog_var[5];
float asc_etmy_watchdog_vabs;
double asc_etmy_yaw1;
double asc_etmy_yaw2;
double asc_etmy_yaw3;
static float asc_itmx_ground;
static float asc_itmx_ground1;
double asc_itmx_matrix[2][3];
double asc_itmx_pit;
double asc_itmx_product;
double asc_itmx_product1;
static int asc_itmx_watchdog;
static float asc_itmx_watchdog_avg[5];
static float asc_itmx_watchdog_var[5];
float asc_itmx_watchdog_vabs;
double asc_itmx_yaw1;
static float asc_itmy_ground;
static float asc_itmy_ground1;
double asc_itmy_matrix[2][3];
double asc_itmy_pit;
double asc_itmy_product;
double asc_itmy_product1;
static int asc_itmy_watchdog;
static float asc_itmy_watchdog_avg[5];
static float asc_itmy_watchdog_var[5];
float asc_itmy_watchdog_vabs;
double asc_itmy_yaw1;
static float asc_pztx_ground;
static float asc_pztx_ground1;
static float asc_pztx_ground2;
double asc_pztx_mx[3][3];
static float asc_pzty_ground;
static float asc_pzty_ground1;
static float asc_pzty_ground2;
double asc_pzty_mx[3][3];
static float ground;
static float ground1;
double lsc_as1i;
double lsc_as1q;
double lsc_asdc;
double lsc_asrfpddc;
double lsc_carm;
double lsc_darm;
double lsc_dof1;
double lsc_dof2;
double lsc_dof3;
double lsc_demux2[5];
static float lsc_ground1;
static float lsc_ground2;
static float lsc_ground3;
static float lsc_ground4;
static float lsc_ground5;
static float lsc_ground6;
double lsc_inmtrx[5][8];
double lsc_isspd;
double lsc_mich;
double lsc_mux2[2];
double lsc_outmtrx[6][8];
double lsc_poy;
double lsc_pwr_mon;
double lsc_refldc;
double lsc_refli;
double lsc_reflq;
static double lsc_refl_rot[2];
double lsc_xarm;
double lsc_x_trans;
double lsc_x_transmon;
double lsc_yarm;
double lsc_y_trans;
double lsc_y_transmon;
#include "LSC_transthr.c"
double sus_bs_ascp;
double sus_bs_ascy;
static float sus_bs_ground2;
static float sus_bs_ground3;
double sus_bs_inmtrx[3][4];
double sus_bs_llout;
double sus_bs_llsen;
double sus_bs_lrout;
double sus_bs_lrsen;
double sus_bs_lsc;
double sus_bs_master_sw[5];
double sus_bs_outmtrx[4][3];
double sus_bs_pit;
double sus_bs_pos;
double sus_bs_product;
double sus_bs_product1;
double sus_bs_product2;
double sus_bs_product3;
double sus_bs_product4;
double sus_bs_side;
double sus_bs_sum4;
double sus_bs_sum5;
double sus_bs_sum6;
double sus_bs_ulout;
double sus_bs_ulsen;
double sus_bs_urout;
double sus_bs_ursen;
static int sus_bs_wd;
static float sus_bs_wd_avg[5];
static float sus_bs_wd_var[5];
float sus_bs_wd_vabs;
double sus_bs_yaw;
double sus_etmx_ascp;
double sus_etmx_ascx;
double sus_etmx_ascy;
static float sus_etmx_ground1;
static float sus_etmx_ground2;
double sus_etmx_inmtrx[3][4];
double sus_etmx_llout;
double sus_etmx_llsen;
double sus_etmx_lrout;
double sus_etmx_lrsen;
double sus_etmx_lsc;
double sus_etmx_lsc_pit;
double sus_etmx_lsc_side;
double sus_etmx_lsc_yaw;
double sus_etmx_master_sw[5];
double sus_etmx_outmtrx[4][3];
double sus_etmx_pit;
double sus_etmx_pos;
double sus_etmx_product;
double sus_etmx_product1;
double sus_etmx_product2;
double sus_etmx_product3;
double sus_etmx_product4;
double sus_etmx_side;
double sus_etmx_sum4;
double sus_etmx_sum5;
double sus_etmx_sum6;
double sus_etmx_sum7;
double sus_etmx_sum8;
double sus_etmx_sum9;
double sus_etmx_ulout;
double sus_etmx_ulsen;
double sus_etmx_urout;
double sus_etmx_ursen;
static int sus_etmx_wd;
static float sus_etmx_wd_avg[5];
static float sus_etmx_wd_var[5];
float sus_etmx_wd_vabs;
double sus_etmx_yaw;
double sus_etmy_ascp;
double sus_etmy_ascx;
double sus_etmy_ascy;
static float sus_etmy_ground1;
static float sus_etmy_ground2;
double sus_etmy_inmtrx[3][4];
double sus_etmy_llout;
double sus_etmy_llsen;
double sus_etmy_lrout;
double sus_etmy_lrsen;
double sus_etmy_lsc;
double sus_etmy_lsc_pit;
double sus_etmy_lsc_side;
double sus_etmy_lsc_yaw;
double sus_etmy_master_sw[5];
double sus_etmy_outmtrx[4][3];
double sus_etmy_pit;
double sus_etmy_pos;
double sus_etmy_product;
double sus_etmy_product1;
double sus_etmy_product2;
double sus_etmy_product3;
double sus_etmy_product4;
double sus_etmy_side;
double sus_etmy_sum4;
double sus_etmy_sum5;
double sus_etmy_sum6;
double sus_etmy_sum7;
double sus_etmy_sum8;
double sus_etmy_sum9;
double sus_etmy_ulout;
double sus_etmy_ulsen;
double sus_etmy_urout;
double sus_etmy_ursen;
static int sus_etmy_wd;
static float sus_etmy_wd_avg[5];
static float sus_etmy_wd_var[5];
float sus_etmy_wd_vabs;
double sus_etmy_yaw;
double sus_itmx_ascp;
double sus_itmx_ascy;
static float sus_itmx_ground2;
static float sus_itmx_ground3;
double sus_itmx_inmtrx[3][4];
double sus_itmx_llout;
double sus_itmx_llsen;
double sus_itmx_lrout;
double sus_itmx_lrsen;
double sus_itmx_lsc;
double sus_itmx_master_sw[5];
double sus_itmx_outmtrx[4][3];
double sus_itmx_pit;
double sus_itmx_pos;
double sus_itmx_product;
double sus_itmx_product1;
double sus_itmx_product2;
double sus_itmx_product3;
double sus_itmx_product4;
double sus_itmx_side;
double sus_itmx_sum4;
double sus_itmx_sum5;
double sus_itmx_sum6;
double sus_itmx_sum7;
double sus_itmx_sum8;
double sus_itmx_ulout;
double sus_itmx_ulsen;
double sus_itmx_urout;
double sus_itmx_ursen;
static int sus_itmx_wd;
static float sus_itmx_wd_avg[5];
static float sus_itmx_wd_var[5];
float sus_itmx_wd_vabs;
double sus_itmx_yaw;
double sus_itmy_ascp;
double sus_itmy_ascy;
static float sus_itmy_ground2;
static float sus_itmy_ground3;
double sus_itmy_inmtrx[3][4];
double sus_itmy_llout;
double sus_itmy_llsen;
double sus_itmy_lrout;
double sus_itmy_lrsen;
double sus_itmy_lsc;
double sus_itmy_master_sw[5];
double sus_itmy_outmtrx[4][3];
double sus_itmy_pit;
double sus_itmy_pos;
double sus_itmy_product;
double sus_itmy_product1;
double sus_itmy_product2;
double sus_itmy_product3;
double sus_itmy_product4;
double sus_itmy_side;
double sus_itmy_sum4;
double sus_itmy_sum5;
double sus_itmy_sum6;
double sus_itmy_sum7;
double sus_itmy_sum8;
double sus_itmy_ulout;
double sus_itmy_ulsen;
double sus_itmy_urout;
double sus_itmy_ursen;
static int sus_itmy_wd;
static float sus_itmy_wd_avg[5];
static float sus_itmy_wd_var[5];
float sus_itmy_wd_vabs;
double sus_itmy_yaw;
int sus_wd_sum;


/* CPU 2 code */
void cpu2_start(){
	int ii, jj;
	while(!stop_working_threads) {
		while(!go2 && !stop_working_threads);
		go2 = 0;

//Start of subsystem ASC_ITMX **************************************************

// Matrix
for(ii=0;ii<2;ii++)
{
asc_itmx_matrix[1][ii] = 
	pLocalEpics->pde.ASC_ITMX_Matrix[ii][0] * dWord[0][16] +
	pLocalEpics->pde.ASC_ITMX_Matrix[ii][1] * dWord[0][17] +
	pLocalEpics->pde.ASC_ITMX_Matrix[ii][2] * dWord[0][18];
}

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.ASC_ITMX_Watchdog == 1) {
	asc_itmx_watchdog = 1;
	pLocalEpics->pde.ASC_ITMX_Watchdog = 0;
};
double ins[5]= {
	dWord[0][16],
	dWord[0][17],
	dWord[0][18],
	asc_itmx_ground,
	asc_itmx_ground1,
};
   for(ii=0; ii<5;ii++) {
	asc_itmx_watchdog_avg[ii] = ins[ii] * .00005 + asc_itmx_watchdog_avg[ii] * 0.99995;
	asc_itmx_watchdog_vabs = ins[ii] - asc_itmx_watchdog_avg[ii];
	if(asc_itmx_watchdog_vabs < 0) asc_itmx_watchdog_vabs *= -1.0;
	asc_itmx_watchdog_var[ii] = asc_itmx_watchdog_vabs * .00005 + asc_itmx_watchdog_var[ii] * 0.99995;
	pLocalEpics->pde.ASC_ITMX_Watchdog_VAR[ii] = asc_itmx_watchdog_var[ii];
	if(asc_itmx_watchdog_var[ii] > pLocalEpics->pde.ASC_ITMX_Watchdog_MAX) asc_itmx_watchdog = 0;
   }
	pLocalEpics->pde.ASC_ITMX_Watchdog_STAT = asc_itmx_watchdog;
}

// MULTIPLY
asc_itmx_product = asc_itmx_matrix[1][0] * asc_itmx_watchdog;

// MULTIPLY
asc_itmx_product1 = asc_itmx_matrix[1][1] * asc_itmx_watchdog;

// FILTER MODULE
asc_itmx_pit = filterModuleD(dspPtr[0],dspCoeff,ASC_ITMX_PIT,asc_itmx_product,0);

// FILTER MODULE
asc_itmx_yaw1 = filterModuleD(dspPtr[0],dspCoeff,ASC_ITMX_YAW1,asc_itmx_product1,0);


//End of subsystem   ASC_ITMX **************************************************



//Start of subsystem ASC_BS **************************************************

// Matrix
for(ii=0;ii<2;ii++)
{
asc_bs_matrix[1][ii] = 
	pLocalEpics->pde.ASC_BS_Matrix[ii][0] * dWord[0][19] +
	pLocalEpics->pde.ASC_BS_Matrix[ii][1] * dWord[0][20] +
	pLocalEpics->pde.ASC_BS_Matrix[ii][2] * dWord[0][21];
}

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.ASC_BS_Watchdog == 1) {
	asc_bs_watchdog = 1;
	pLocalEpics->pde.ASC_BS_Watchdog = 0;
};
double ins[5]= {
	dWord[0][19],
	dWord[0][20],
	dWord[0][21],
	asc_bs_ground,
	asc_bs_ground1,
};
   for(ii=0; ii<5;ii++) {
	asc_bs_watchdog_avg[ii] = ins[ii] * .00005 + asc_bs_watchdog_avg[ii] * 0.99995;
	asc_bs_watchdog_vabs = ins[ii] - asc_bs_watchdog_avg[ii];
	if(asc_bs_watchdog_vabs < 0) asc_bs_watchdog_vabs *= -1.0;
	asc_bs_watchdog_var[ii] = asc_bs_watchdog_vabs * .00005 + asc_bs_watchdog_var[ii] * 0.99995;
	pLocalEpics->pde.ASC_BS_Watchdog_VAR[ii] = asc_bs_watchdog_var[ii];
	if(asc_bs_watchdog_var[ii] > pLocalEpics->pde.ASC_BS_Watchdog_MAX) asc_bs_watchdog = 0;
   }
	pLocalEpics->pde.ASC_BS_Watchdog_STAT = asc_bs_watchdog;
}

// MULTIPLY
asc_bs_product = asc_bs_matrix[1][0] * asc_bs_watchdog;

// MULTIPLY
asc_bs_product1 = asc_bs_matrix[1][1] * asc_bs_watchdog;

// FILTER MODULE
asc_bs_pit = filterModuleD(dspPtr[0],dspCoeff,ASC_BS_PIT,asc_bs_product,0);

// FILTER MODULE
asc_bs_yaw1 = filterModuleD(dspPtr[0],dspCoeff,ASC_BS_YAW1,asc_bs_product1,0);


//End of subsystem   ASC_BS **************************************************



//Start of subsystem ASC_PZTY **************************************************

// Matrix
for(ii=0;ii<3;ii++)
{
asc_pzty_mx[1][ii] = 
	pLocalEpics->pde.ASC_PZTY_MX[ii][0] * pLocalEpics->pde.ASC_PZTY_PIT +
	pLocalEpics->pde.ASC_PZTY_MX[ii][1] * pLocalEpics->pde.ASC_PZTY_YAW +
	pLocalEpics->pde.ASC_PZTY_MX[ii][2] * pLocalEpics->pde.ASC_PZTY_POS;
}


//End of subsystem   ASC_PZTY **************************************************


		done2 = 1;
		while(!go2 && !stop_working_threads);
		go2 = 0;

//Start of subsystem SUS_ETMX **************************************************

// FILTER MODULE
sus_etmx_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULSEN,dWord[1][8],0);

// FILTER MODULE
sus_etmx_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLSEN,dWord[1][9],0);

// FILTER MODULE
sus_etmx_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_URSEN,dWord[1][10],0);

// FILTER MODULE
sus_etmx_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LRSEN,dWord[1][11],0);

// FILTER MODULE
sus_etmx_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_SIDE,dWord[1][24],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ETMX_WD == 1) {
	sus_etmx_wd = 1;
	pLocalEpics->pde.SUS_ETMX_WD = 0;
};
double ins[5]= {
	dWord[1][8],
	dWord[1][9],
	dWord[1][10],
	dWord[1][11],
	dWord[1][24],
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
sus_etmx_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LSC,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_lsc_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LSC_PIT,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_lsc_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LSC_YAW,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_lsc_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LSC_SIDE,lsc_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_ascx = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCX,asc_etmx_pos,0);

// SUM
sus_etmx_sum7 = asc_etmx_sum7 + asc_align_etmx_ascp + pLocalEpics->pde.SUS_ETMX_PIT_BIAS;

// SUM
sus_etmx_sum8 = asc_align_etmx_ascy + pLocalEpics->pde.SUS_ETMX_YAW_BIAS + asc_etmx_sum1;

// Matrix
for(ii=0;ii<3;ii++)
{
sus_etmx_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][0] * sus_etmx_ulsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][1] * sus_etmx_llsen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][2] * sus_etmx_ursen +
	pLocalEpics->pde.SUS_ETMX_INMTRX[ii][3] * sus_etmx_lrsen;
}

// SUM
sus_etmx_sum9 = sus_etmx_side + asc_etmx_side + sus_etmx_lsc_side;

// FILTER MODULE
sus_etmx_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCP,sus_etmx_sum7,0);

// FILTER MODULE
sus_etmx_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ASCY,sus_etmx_sum8,0);

// FILTER MODULE
sus_etmx_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_POS,sus_etmx_inmtrx[1][0],0);

// FILTER MODULE
sus_etmx_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_PIT,sus_etmx_inmtrx[1][1],0);

// FILTER MODULE
sus_etmx_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_YAW,sus_etmx_inmtrx[1][2],0);

// MULTIPLY
sus_etmx_product = sus_etmx_sum9 * sus_etmx_wd;

// SUM
sus_etmx_sum4 = sus_etmx_lsc + sus_etmx_pos + sus_etmx_ascx;

// SUM
sus_etmx_sum5 = sus_etmx_pit + sus_etmx_ascp + asc_align_etmx_pit_osc[1] + sus_etmx_lsc_pit;

// SUM
sus_etmx_sum6 = sus_etmx_yaw + sus_etmx_ascy + asc_align_etmx_yaw_osc[1] + sus_etmx_lsc_yaw;

// Matrix
for(ii=0;ii<4;ii++)
{
sus_etmx_outmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMX_OUTMTRX[ii][0] * sus_etmx_sum4 +
	pLocalEpics->pde.SUS_ETMX_OUTMTRX[ii][1] * sus_etmx_sum5 +
	pLocalEpics->pde.SUS_ETMX_OUTMTRX[ii][2] * sus_etmx_sum6;
}

// FILTER MODULE
sus_etmx_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_ULOUT,sus_etmx_outmtrx[1][0],0);

// FILTER MODULE
sus_etmx_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LLOUT,sus_etmx_outmtrx[1][1],0);

// FILTER MODULE
sus_etmx_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_UROUT,sus_etmx_outmtrx[1][2],0);

// FILTER MODULE
sus_etmx_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMX_LROUT,sus_etmx_outmtrx[1][3],0);

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



//Start of subsystem SUS_ITMY **************************************************

// FILTER MODULE
sus_itmy_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULSEN,dWord[1][4],0);

// FILTER MODULE
sus_itmy_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLSEN,dWord[1][5],0);

// FILTER MODULE
sus_itmy_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_URSEN,dWord[1][6],0);

// FILTER MODULE
sus_itmy_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LRSEN,dWord[1][7],0);

// FILTER MODULE
sus_itmy_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_SIDE,dWord[1][22],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ITMY_WD == 1) {
	sus_itmy_wd = 1;
	pLocalEpics->pde.SUS_ITMY_WD = 0;
};
double ins[5]= {
	dWord[1][4],
	dWord[1][5],
	dWord[1][6],
	dWord[1][7],
	dWord[1][22],
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
sus_itmy_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LSC,lsc_outmtrx[1][3],0);

// SUM
sus_itmy_sum7 = asc_align_itmy_ascp + pLocalEpics->pde.SUS_ITMY_PIT_BIAS + asc_itmy_pit;

// SUM
sus_itmy_sum8 = pLocalEpics->pde.SUS_ITMY_YAW_BIAS + asc_align_itmy_ascy + asc_itmy_yaw1;

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
sus_itmy_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ASCP,sus_itmy_sum7,0);

// FILTER MODULE
sus_itmy_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ASCY,sus_itmy_sum8,0);

// FILTER MODULE
sus_itmy_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_POS,sus_itmy_inmtrx[1][0],0);

// FILTER MODULE
sus_itmy_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_PIT,sus_itmy_inmtrx[1][1],0);

// FILTER MODULE
sus_itmy_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_YAW,sus_itmy_inmtrx[1][2],0);

// SUM
sus_itmy_sum4 = sus_itmy_lsc + sus_itmy_pos;

// SUM
sus_itmy_sum5 = sus_itmy_pit + sus_itmy_ascp + asc_align_itmy_pit_osc[1];

// SUM
sus_itmy_sum6 = sus_itmy_yaw + sus_itmy_ascy + asc_align_itmy_yaw_osc[1];

// Matrix
for(ii=0;ii<4;ii++)
{
sus_itmy_outmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMY_OUTMTRX[ii][0] * sus_itmy_sum4 +
	pLocalEpics->pde.SUS_ITMY_OUTMTRX[ii][1] * sus_itmy_sum5 +
	pLocalEpics->pde.SUS_ITMY_OUTMTRX[ii][2] * sus_itmy_sum6;
}

// FILTER MODULE
sus_itmy_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_ULOUT,sus_itmy_outmtrx[1][0],0);

// FILTER MODULE
sus_itmy_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LLOUT,sus_itmy_outmtrx[1][1],0);

// FILTER MODULE
sus_itmy_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_UROUT,sus_itmy_outmtrx[1][2],0);

// FILTER MODULE
sus_itmy_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMY_LROUT,sus_itmy_outmtrx[1][3],0);

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


		done2 = 1;
	}
	done2 = 1;
}

/* CPU 3 code */
void cpu3_start(){
	int ii, jj;
	while(!stop_working_threads) {
		while(!go3 && !stop_working_threads);
		go3 = 0;

//Start of subsystem ASC_PZTX **************************************************

// Matrix
for(ii=0;ii<3;ii++)
{
asc_pztx_mx[1][ii] = 
	pLocalEpics->pde.ASC_PZTX_MX[ii][0] * pLocalEpics->pde.ASC_PZTX_PIT +
	pLocalEpics->pde.ASC_PZTX_MX[ii][1] * pLocalEpics->pde.ASC_PZTX_YAW +
	pLocalEpics->pde.ASC_PZTX_MX[ii][2] * pLocalEpics->pde.ASC_PZTX_POS;
}


//End of subsystem   ASC_PZTX **************************************************



//Start of subsystem ASC_ETMY **************************************************

// Matrix
for(ii=0;ii<4;ii++)
{
asc_etmy_matrix[1][ii] = 
	pLocalEpics->pde.ASC_ETMY_Matrix[ii][0] * dWord[0][28] +
	pLocalEpics->pde.ASC_ETMY_Matrix[ii][1] * dWord[0][29] +
	pLocalEpics->pde.ASC_ETMY_Matrix[ii][2] * dWord[0][30];
}

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.ASC_ETMY_Watchdog == 1) {
	asc_etmy_watchdog = 1;
	pLocalEpics->pde.ASC_ETMY_Watchdog = 0;
};
double ins[5]= {
	dWord[0][28],
	dWord[0][29],
	dWord[0][30],
	asc_etmy_ground,
	asc_etmy_ground1,
};
   for(ii=0; ii<5;ii++) {
	asc_etmy_watchdog_avg[ii] = ins[ii] * .00005 + asc_etmy_watchdog_avg[ii] * 0.99995;
	asc_etmy_watchdog_vabs = ins[ii] - asc_etmy_watchdog_avg[ii];
	if(asc_etmy_watchdog_vabs < 0) asc_etmy_watchdog_vabs *= -1.0;
	asc_etmy_watchdog_var[ii] = asc_etmy_watchdog_vabs * .00005 + asc_etmy_watchdog_var[ii] * 0.99995;
	pLocalEpics->pde.ASC_ETMY_Watchdog_VAR[ii] = asc_etmy_watchdog_var[ii];
	if(asc_etmy_watchdog_var[ii] > pLocalEpics->pde.ASC_ETMY_Watchdog_MAX) asc_etmy_watchdog = 0;
   }
	pLocalEpics->pde.ASC_ETMY_Watchdog_STAT = asc_etmy_watchdog;
}

// FILTER MODULE
asc_etmy_side = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_SIDE,asc_etmy_matrix[1][3],0);

// MULTIPLY
asc_etmy_product = asc_etmy_matrix[1][0] * asc_etmy_watchdog;

// MULTIPLY
asc_etmy_product1 = asc_etmy_matrix[1][1] * asc_etmy_watchdog;

// MULTIPLY
asc_etmy_product2 = asc_etmy_matrix[1][2] * asc_etmy_watchdog;

// FILTER MODULE
asc_etmy_pit1 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_PIT1,asc_etmy_product,0);

// FILTER MODULE
asc_etmy_pit2 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_PIT2,asc_etmy_product,0);

// FILTER MODULE
asc_etmy_pit3 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_PIT3,asc_etmy_product,0);

// FILTER MODULE
asc_etmy_yaw1 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_YAW1,asc_etmy_product1,0);

// FILTER MODULE
asc_etmy_yaw2 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_YAW2,asc_etmy_product1,0);

// FILTER MODULE
asc_etmy_yaw3 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_YAW3,asc_etmy_product1,0);

// FILTER MODULE
asc_etmy_pos = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMY_POS,asc_etmy_product2,0);

// SUM
asc_etmy_sum7 = asc_etmy_pit1 + asc_etmy_pit2 + asc_etmy_pit3;

// SUM
asc_etmy_sum1 = asc_etmy_yaw1 + asc_etmy_yaw2 + asc_etmy_yaw3;


//End of subsystem   ASC_ETMY **************************************************



//Start of subsystem ASC_ALIGN **************************************************

// Osc
asc_align_etmy_pit_osc_cos_new = (1.0 - asc_align_etmy_pit_osc_alpha) * asc_align_etmy_pit_osc_cos_prev - asc_align_etmy_pit_osc_beta * asc_align_etmy_pit_osc_sin_prev;
asc_align_etmy_pit_osc_sin_new = (1.0 - asc_align_etmy_pit_osc_alpha) * asc_align_etmy_pit_osc_sin_prev + asc_align_etmy_pit_osc_beta * asc_align_etmy_pit_osc_cos_prev;
asc_align_etmy_pit_osc_sin_prev = asc_align_etmy_pit_osc_sin_new;
asc_align_etmy_pit_osc_cos_prev = asc_align_etmy_pit_osc_cos_new;
asc_align_etmy_pit_osc[0] = asc_align_etmy_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_OSC_CLKGAIN;
asc_align_etmy_pit_osc[1] = asc_align_etmy_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_OSC_SINGAIN;
asc_align_etmy_pit_osc[2] = asc_align_etmy_pit_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_OSC_COSGAIN;
if((asc_align_etmy_pit_osc_freq != pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_etmy_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_etmy_pit_osc_freq);
	asc_align_etmy_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmy_pit_osc_freq / FE_RATE;
	valx = asc_align_etmy_pit_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmy_pit_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_etmy_pit_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmy_pit_osc_beta = lsinx;
	asc_align_etmy_pit_osc_cos_prev = 1.0;
	asc_align_etmy_pit_osc_sin_prev = 0.0;
}

// Osc
asc_align_etmy_yaw_osc_cos_new = (1.0 - asc_align_etmy_yaw_osc_alpha) * asc_align_etmy_yaw_osc_cos_prev - asc_align_etmy_yaw_osc_beta * asc_align_etmy_yaw_osc_sin_prev;
asc_align_etmy_yaw_osc_sin_new = (1.0 - asc_align_etmy_yaw_osc_alpha) * asc_align_etmy_yaw_osc_sin_prev + asc_align_etmy_yaw_osc_beta * asc_align_etmy_yaw_osc_cos_prev;
asc_align_etmy_yaw_osc_sin_prev = asc_align_etmy_yaw_osc_sin_new;
asc_align_etmy_yaw_osc_cos_prev = asc_align_etmy_yaw_osc_cos_new;
asc_align_etmy_yaw_osc[0] = asc_align_etmy_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_OSC_CLKGAIN;
asc_align_etmy_yaw_osc[1] = asc_align_etmy_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_OSC_SINGAIN;
asc_align_etmy_yaw_osc[2] = asc_align_etmy_yaw_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_OSC_COSGAIN;
if((asc_align_etmy_yaw_osc_freq != pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_etmy_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_etmy_yaw_osc_freq);
	asc_align_etmy_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmy_yaw_osc_freq / FE_RATE;
	valx = asc_align_etmy_yaw_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmy_yaw_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_etmy_yaw_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmy_yaw_osc_beta = lsinx;
	asc_align_etmy_yaw_osc_cos_prev = 1.0;
	asc_align_etmy_yaw_osc_sin_prev = 0.0;
}

// Osc
asc_align_itmy_pit_osc_cos_new = (1.0 - asc_align_itmy_pit_osc_alpha) * asc_align_itmy_pit_osc_cos_prev - asc_align_itmy_pit_osc_beta * asc_align_itmy_pit_osc_sin_prev;
asc_align_itmy_pit_osc_sin_new = (1.0 - asc_align_itmy_pit_osc_alpha) * asc_align_itmy_pit_osc_sin_prev + asc_align_itmy_pit_osc_beta * asc_align_itmy_pit_osc_cos_prev;
asc_align_itmy_pit_osc_sin_prev = asc_align_itmy_pit_osc_sin_new;
asc_align_itmy_pit_osc_cos_prev = asc_align_itmy_pit_osc_cos_new;
asc_align_itmy_pit_osc[0] = asc_align_itmy_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_OSC_CLKGAIN;
asc_align_itmy_pit_osc[1] = asc_align_itmy_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_OSC_SINGAIN;
asc_align_itmy_pit_osc[2] = asc_align_itmy_pit_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_OSC_COSGAIN;
if((asc_align_itmy_pit_osc_freq != pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_itmy_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_itmy_pit_osc_freq);
	asc_align_itmy_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmy_pit_osc_freq / FE_RATE;
	valx = asc_align_itmy_pit_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmy_pit_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_itmy_pit_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmy_pit_osc_beta = lsinx;
	asc_align_itmy_pit_osc_cos_prev = 1.0;
	asc_align_itmy_pit_osc_sin_prev = 0.0;
}

// Osc
asc_align_itmy_yaw_osc_cos_new = (1.0 - asc_align_itmy_yaw_osc_alpha) * asc_align_itmy_yaw_osc_cos_prev - asc_align_itmy_yaw_osc_beta * asc_align_itmy_yaw_osc_sin_prev;
asc_align_itmy_yaw_osc_sin_new = (1.0 - asc_align_itmy_yaw_osc_alpha) * asc_align_itmy_yaw_osc_sin_prev + asc_align_itmy_yaw_osc_beta * asc_align_itmy_yaw_osc_cos_prev;
asc_align_itmy_yaw_osc_sin_prev = asc_align_itmy_yaw_osc_sin_new;
asc_align_itmy_yaw_osc_cos_prev = asc_align_itmy_yaw_osc_cos_new;
asc_align_itmy_yaw_osc[0] = asc_align_itmy_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_OSC_CLKGAIN;
asc_align_itmy_yaw_osc[1] = asc_align_itmy_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_OSC_SINGAIN;
asc_align_itmy_yaw_osc[2] = asc_align_itmy_yaw_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_OSC_COSGAIN;
if((asc_align_itmy_yaw_osc_freq != pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_itmy_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_itmy_yaw_osc_freq);
	asc_align_itmy_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmy_yaw_osc_freq / FE_RATE;
	valx = asc_align_itmy_yaw_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmy_yaw_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_itmy_yaw_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmy_yaw_osc_beta = lsinx;
	asc_align_itmy_yaw_osc_cos_prev = 1.0;
	asc_align_itmy_yaw_osc_sin_prev = 0.0;
}

// Osc
asc_align_etmx_pit_osc_cos_new = (1.0 - asc_align_etmx_pit_osc_alpha) * asc_align_etmx_pit_osc_cos_prev - asc_align_etmx_pit_osc_beta * asc_align_etmx_pit_osc_sin_prev;
asc_align_etmx_pit_osc_sin_new = (1.0 - asc_align_etmx_pit_osc_alpha) * asc_align_etmx_pit_osc_sin_prev + asc_align_etmx_pit_osc_beta * asc_align_etmx_pit_osc_cos_prev;
asc_align_etmx_pit_osc_sin_prev = asc_align_etmx_pit_osc_sin_new;
asc_align_etmx_pit_osc_cos_prev = asc_align_etmx_pit_osc_cos_new;
asc_align_etmx_pit_osc[0] = asc_align_etmx_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_OSC_CLKGAIN;
asc_align_etmx_pit_osc[1] = asc_align_etmx_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_OSC_SINGAIN;
asc_align_etmx_pit_osc[2] = asc_align_etmx_pit_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_OSC_COSGAIN;
if((asc_align_etmx_pit_osc_freq != pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_etmx_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_etmx_pit_osc_freq);
	asc_align_etmx_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmx_pit_osc_freq / FE_RATE;
	valx = asc_align_etmx_pit_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmx_pit_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_etmx_pit_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmx_pit_osc_beta = lsinx;
	asc_align_etmx_pit_osc_cos_prev = 1.0;
	asc_align_etmx_pit_osc_sin_prev = 0.0;
}

// Osc
asc_align_etmx_yaw_osc_cos_new = (1.0 - asc_align_etmx_yaw_osc_alpha) * asc_align_etmx_yaw_osc_cos_prev - asc_align_etmx_yaw_osc_beta * asc_align_etmx_yaw_osc_sin_prev;
asc_align_etmx_yaw_osc_sin_new = (1.0 - asc_align_etmx_yaw_osc_alpha) * asc_align_etmx_yaw_osc_sin_prev + asc_align_etmx_yaw_osc_beta * asc_align_etmx_yaw_osc_cos_prev;
asc_align_etmx_yaw_osc_sin_prev = asc_align_etmx_yaw_osc_sin_new;
asc_align_etmx_yaw_osc_cos_prev = asc_align_etmx_yaw_osc_cos_new;
asc_align_etmx_yaw_osc[0] = asc_align_etmx_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_OSC_CLKGAIN;
asc_align_etmx_yaw_osc[1] = asc_align_etmx_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_OSC_SINGAIN;
asc_align_etmx_yaw_osc[2] = asc_align_etmx_yaw_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_OSC_COSGAIN;
if((asc_align_etmx_yaw_osc_freq != pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_etmx_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_etmx_yaw_osc_freq);
	asc_align_etmx_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmx_yaw_osc_freq / FE_RATE;
	valx = asc_align_etmx_yaw_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmx_yaw_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_etmx_yaw_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_etmx_yaw_osc_beta = lsinx;
	asc_align_etmx_yaw_osc_cos_prev = 1.0;
	asc_align_etmx_yaw_osc_sin_prev = 0.0;
}

// Osc
asc_align_itmx_pit_osc_cos_new = (1.0 - asc_align_itmx_pit_osc_alpha) * asc_align_itmx_pit_osc_cos_prev - asc_align_itmx_pit_osc_beta * asc_align_itmx_pit_osc_sin_prev;
asc_align_itmx_pit_osc_sin_new = (1.0 - asc_align_itmx_pit_osc_alpha) * asc_align_itmx_pit_osc_sin_prev + asc_align_itmx_pit_osc_beta * asc_align_itmx_pit_osc_cos_prev;
asc_align_itmx_pit_osc_sin_prev = asc_align_itmx_pit_osc_sin_new;
asc_align_itmx_pit_osc_cos_prev = asc_align_itmx_pit_osc_cos_new;
asc_align_itmx_pit_osc[0] = asc_align_itmx_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_OSC_CLKGAIN;
asc_align_itmx_pit_osc[1] = asc_align_itmx_pit_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_OSC_SINGAIN;
asc_align_itmx_pit_osc[2] = asc_align_itmx_pit_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_OSC_COSGAIN;
if((asc_align_itmx_pit_osc_freq != pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_itmx_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_itmx_pit_osc_freq);
	asc_align_itmx_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmx_pit_osc_freq / FE_RATE;
	valx = asc_align_itmx_pit_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmx_pit_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_itmx_pit_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmx_pit_osc_beta = lsinx;
	asc_align_itmx_pit_osc_cos_prev = 1.0;
	asc_align_itmx_pit_osc_sin_prev = 0.0;
}

// Osc
asc_align_itmx_yaw_osc_cos_new = (1.0 - asc_align_itmx_yaw_osc_alpha) * asc_align_itmx_yaw_osc_cos_prev - asc_align_itmx_yaw_osc_beta * asc_align_itmx_yaw_osc_sin_prev;
asc_align_itmx_yaw_osc_sin_new = (1.0 - asc_align_itmx_yaw_osc_alpha) * asc_align_itmx_yaw_osc_sin_prev + asc_align_itmx_yaw_osc_beta * asc_align_itmx_yaw_osc_cos_prev;
asc_align_itmx_yaw_osc_sin_prev = asc_align_itmx_yaw_osc_sin_new;
asc_align_itmx_yaw_osc_cos_prev = asc_align_itmx_yaw_osc_cos_new;
asc_align_itmx_yaw_osc[0] = asc_align_itmx_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_OSC_CLKGAIN;
asc_align_itmx_yaw_osc[1] = asc_align_itmx_yaw_osc_sin_new * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_OSC_SINGAIN;
asc_align_itmx_yaw_osc[2] = asc_align_itmx_yaw_osc_cos_new * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_OSC_COSGAIN;
if((asc_align_itmx_yaw_osc_freq != pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_OSC_FREQ) && ((clock16K + 1) == FE_RATE))
{
	asc_align_itmx_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_OSC_FREQ;
	printf("OSC Freq = %f\n",asc_align_itmx_yaw_osc_freq);
	asc_align_itmx_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmx_yaw_osc_freq / FE_RATE;
	valx = asc_align_itmx_yaw_osc_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmx_yaw_osc_alpha = 2.0 * lsinx * lsinx;
	valx = asc_align_itmx_yaw_osc_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_align_itmx_yaw_osc_beta = lsinx;
	asc_align_itmx_yaw_osc_cos_prev = 1.0;
	asc_align_itmx_yaw_osc_sin_prev = 0.0;
}

// MULTIPLY
asc_align_product = dWord[0][7] * asc_align_etmy_pit_osc[1];

// MULTIPLY
asc_align_product1 = dWord[0][7] * asc_align_etmy_pit_osc[2];

// MULTIPLY
asc_align_product2 = dWord[0][7] * asc_align_etmy_yaw_osc[1];

// MULTIPLY
asc_align_product3 = dWord[0][7] * asc_align_etmy_yaw_osc[2];

// MULTIPLY
asc_align_product4 = dWord[0][7] * asc_align_itmy_pit_osc[1];

// MULTIPLY
asc_align_product5 = dWord[0][7] * asc_align_itmy_pit_osc[2];

// MULTIPLY
asc_align_product6 = dWord[0][7] * asc_align_itmy_yaw_osc[1];

// MULTIPLY
asc_align_product7 = dWord[0][7] * asc_align_itmy_yaw_osc[2];

// MULTIPLY
asc_align_product8 = dWord[0][6] * asc_align_etmx_pit_osc[1];

// MULTIPLY
asc_align_product9 = dWord[0][6] * asc_align_etmx_pit_osc[2];

// MULTIPLY
asc_align_product10 = dWord[0][6] * asc_align_etmx_yaw_osc[1];

// MULTIPLY
asc_align_product11 = dWord[0][6] * asc_align_etmx_yaw_osc[2];

// MULTIPLY
asc_align_product12 = dWord[0][6] * asc_align_itmx_pit_osc[1];

// MULTIPLY
asc_align_product13 = dWord[0][6] * asc_align_itmx_pit_osc[2];

// MULTIPLY
asc_align_product14 = dWord[0][6] * asc_align_itmx_yaw_osc[1];

// MULTIPLY
asc_align_product15 = dWord[0][6] * asc_align_itmx_yaw_osc[2];

// PHASE
asc_align_etmy_pit_rot[0] = (asc_align_product1 * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_ROT[1]) + (asc_align_product * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_ROT[0]);
asc_align_etmy_pit_rot[1] = (asc_align_product * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_ROT[1]) - (asc_align_product1 * pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_ROT[0]);

// PHASE
asc_align_etmy_yaw_rot[0] = (asc_align_product3 * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_ROT[1]) + (asc_align_product2 * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_ROT[0]);
asc_align_etmy_yaw_rot[1] = (asc_align_product2 * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_ROT[1]) - (asc_align_product3 * pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_ROT[0]);

// PHASE
asc_align_itmy_pit_rot[0] = (asc_align_product5 * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_ROT[1]) + (asc_align_product4 * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_ROT[0]);
asc_align_itmy_pit_rot[1] = (asc_align_product4 * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_ROT[1]) - (asc_align_product5 * pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_ROT[0]);

// PHASE
asc_align_itmy_yaw_rot[0] = (asc_align_product7 * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_ROT[1]) + (asc_align_product6 * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_ROT[0]);
asc_align_itmy_yaw_rot[1] = (asc_align_product6 * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_ROT[1]) - (asc_align_product7 * pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_ROT[0]);

// PHASE
asc_align_etmx_pit_rot[0] = (asc_align_product9 * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_ROT[1]) + (asc_align_product8 * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_ROT[0]);
asc_align_etmx_pit_rot[1] = (asc_align_product8 * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_ROT[1]) - (asc_align_product9 * pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_ROT[0]);

// PHASE
asc_align_etmx_yaw_rot[0] = (asc_align_product11 * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_ROT[1]) + (asc_align_product10 * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_ROT[0]);
asc_align_etmx_yaw_rot[1] = (asc_align_product10 * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_ROT[1]) - (asc_align_product11 * pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_ROT[0]);

// PHASE
asc_align_itmx_pit_rot[0] = (asc_align_product13 * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_ROT[1]) + (asc_align_product12 * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_ROT[0]);
asc_align_itmx_pit_rot[1] = (asc_align_product12 * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_ROT[1]) - (asc_align_product13 * pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_ROT[0]);

// PHASE
asc_align_itmx_yaw_rot[0] = (asc_align_product15 * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_ROT[1]) + (asc_align_product14 * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_ROT[0]);
asc_align_itmx_yaw_rot[1] = (asc_align_product14 * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_ROT[1]) - (asc_align_product15 * pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_ROT[0]);

// FILTER MODULE
asc_align_etmy_ascp = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ETMY_ASCP,asc_align_etmy_pit_rot[0],0);

// FILTER MODULE
asc_align_etmy_ascy = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ETMY_ASCY,asc_align_etmy_yaw_rot[0],0);

// FILTER MODULE
asc_align_itmy_ascp = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ITMY_ASCP,asc_align_itmy_pit_rot[0],0);

// FILTER MODULE
asc_align_itmy_ascy = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ITMY_ASCY,asc_align_itmy_yaw_rot[0],0);

// FILTER MODULE
asc_align_etmx_ascp = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ETMX_ASCP,asc_align_etmx_pit_rot[0],0);

// FILTER MODULE
asc_align_etmx_ascy = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ETMX_ASCY,asc_align_etmx_yaw_rot[0],0);

// FILTER MODULE
asc_align_itmx_ascp = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ITMX_ASCP,asc_align_itmx_pit_rot[0],0);

// FILTER MODULE
asc_align_itmx_ascy = filterModuleD(dspPtr[0],dspCoeff,ASC_ALIGN_ITMX_ASCY,asc_align_itmx_yaw_rot[0],0);


//End of subsystem   ASC_ALIGN **************************************************


		done3 = 1;
		while(!go3 && !stop_working_threads);
		go3 = 0;

//Start of subsystem SUS_ITMX **************************************************

// FILTER MODULE
sus_itmx_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULSEN,dWord[1][0],0);

// FILTER MODULE
sus_itmx_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLSEN,dWord[1][1],0);

// FILTER MODULE
sus_itmx_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_URSEN,dWord[1][2],0);

// FILTER MODULE
sus_itmx_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LRSEN,dWord[1][3],0);

// FILTER MODULE
sus_itmx_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_SIDE,dWord[1][23],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ITMX_WD == 1) {
	sus_itmx_wd = 1;
	pLocalEpics->pde.SUS_ITMX_WD = 0;
};
double ins[5]= {
	dWord[1][0],
	dWord[1][1],
	dWord[1][2],
	dWord[1][3],
	dWord[1][23],
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
sus_itmx_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LSC,lsc_outmtrx[1][2],0);

// SUM
sus_itmx_sum7 = asc_align_itmx_ascp + pLocalEpics->pde.SUS_ITMX_PIT_BIAS + asc_itmx_pit;

// SUM
sus_itmx_sum8 = pLocalEpics->pde.SUS_ITMX_YAW_BIAS + asc_align_itmx_ascy + asc_itmx_yaw1;

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
sus_itmx_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ASCP,sus_itmx_sum7,0);

// FILTER MODULE
sus_itmx_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ASCY,sus_itmx_sum8,0);

// FILTER MODULE
sus_itmx_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_POS,sus_itmx_inmtrx[1][0],0);

// FILTER MODULE
sus_itmx_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_PIT,sus_itmx_inmtrx[1][1],0);

// FILTER MODULE
sus_itmx_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_YAW,sus_itmx_inmtrx[1][2],0);

// SUM
sus_itmx_sum4 = sus_itmx_lsc + sus_itmx_pos;

// SUM
sus_itmx_sum5 = sus_itmx_pit + sus_itmx_ascp + asc_align_itmx_pit_osc[1];

// SUM
sus_itmx_sum6 = sus_itmx_yaw + sus_itmx_ascy + asc_align_itmx_yaw_osc[1];

// Matrix
for(ii=0;ii<4;ii++)
{
sus_itmx_outmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ITMX_OUTMTRX[ii][0] * sus_itmx_sum4 +
	pLocalEpics->pde.SUS_ITMX_OUTMTRX[ii][1] * sus_itmx_sum5 +
	pLocalEpics->pde.SUS_ITMX_OUTMTRX[ii][2] * sus_itmx_sum6;
}

// FILTER MODULE
sus_itmx_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_ULOUT,sus_itmx_outmtrx[1][0],0);

// FILTER MODULE
sus_itmx_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LLOUT,sus_itmx_outmtrx[1][1],0);

// FILTER MODULE
sus_itmx_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_UROUT,sus_itmx_outmtrx[1][2],0);

// FILTER MODULE
sus_itmx_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ITMX_LROUT,sus_itmx_outmtrx[1][3],0);

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



//Start of subsystem SUS_BS **************************************************

// FILTER MODULE
sus_bs_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_ULSEN,dWord[1][16],0);

// FILTER MODULE
sus_bs_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_LLSEN,dWord[1][17],0);

// FILTER MODULE
sus_bs_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_URSEN,dWord[1][18],0);

// FILTER MODULE
sus_bs_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_LRSEN,dWord[1][19],0);

// FILTER MODULE
sus_bs_side = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_SIDE,dWord[1][21],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_BS_WD == 1) {
	sus_bs_wd = 1;
	pLocalEpics->pde.SUS_BS_WD = 0;
};
double ins[5]= {
	dWord[1][16],
	dWord[1][17],
	dWord[1][18],
	dWord[1][19],
	dWord[1][21],
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
sus_bs_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_LSC,lsc_outmtrx[1][4],0);

// FILTER MODULE
sus_bs_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_ASCP,asc_bs_pit,0);

// FILTER MODULE
sus_bs_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_ASCY,asc_bs_yaw1,0);

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
sus_bs_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_POS,sus_bs_inmtrx[1][0],0);

// FILTER MODULE
sus_bs_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_PIT,sus_bs_inmtrx[1][1],0);

// FILTER MODULE
sus_bs_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_YAW,sus_bs_inmtrx[1][2],0);

// SUM
sus_bs_sum4 = sus_bs_lsc + sus_bs_pos;

// SUM
sus_bs_sum5 = sus_bs_pit + sus_bs_ascp + pLocalEpics->pde.SUS_BS_PIT_BIAS;

// SUM
sus_bs_sum6 = sus_bs_yaw + sus_bs_ascy + pLocalEpics->pde.SUS_BS_YAW_BIAS;

// Matrix
for(ii=0;ii<4;ii++)
{
sus_bs_outmtrx[1][ii] = 
	pLocalEpics->pde.SUS_BS_OUTMTRX[ii][0] * sus_bs_sum4 +
	pLocalEpics->pde.SUS_BS_OUTMTRX[ii][1] * sus_bs_sum5 +
	pLocalEpics->pde.SUS_BS_OUTMTRX[ii][2] * sus_bs_sum6;
}

// FILTER MODULE
sus_bs_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_ULOUT,sus_bs_outmtrx[1][0],0);

// FILTER MODULE
sus_bs_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_LLOUT,sus_bs_outmtrx[1][1],0);

// FILTER MODULE
sus_bs_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_UROUT,sus_bs_outmtrx[1][2],0);

// FILTER MODULE
sus_bs_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_BS_LROUT,sus_bs_outmtrx[1][3],0);

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


		done3 = 1;
	}
	done3 = 1;
}

/* CPU 1 code */
void feCode(int cycle, double dWord[][32],	/* ADC inputs */
		int dacOut[][16],	/* DAC outputs */
		FILT_MOD *dsp_ptr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii;

if(feInit)
{
asc_align_etmx_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMX_PIT_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_etmx_pit_osc_freq);
asc_align_etmx_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmx_pit_osc_freq / FE_RATE;
valx = asc_align_etmx_pit_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_etmx_pit_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_etmx_pit_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_etmx_pit_osc_beta = lsinx;
asc_align_etmx_pit_osc_cos_prev = 1.0;
asc_align_etmx_pit_osc_sin_prev = 0.0;
asc_align_etmx_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMX_YAW_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_etmx_yaw_osc_freq);
asc_align_etmx_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmx_yaw_osc_freq / FE_RATE;
valx = asc_align_etmx_yaw_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_etmx_yaw_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_etmx_yaw_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_etmx_yaw_osc_beta = lsinx;
asc_align_etmx_yaw_osc_cos_prev = 1.0;
asc_align_etmx_yaw_osc_sin_prev = 0.0;
asc_align_etmy_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMY_PIT_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_etmy_pit_osc_freq);
asc_align_etmy_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmy_pit_osc_freq / FE_RATE;
valx = asc_align_etmy_pit_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_etmy_pit_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_etmy_pit_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_etmy_pit_osc_beta = lsinx;
asc_align_etmy_pit_osc_cos_prev = 1.0;
asc_align_etmy_pit_osc_sin_prev = 0.0;
asc_align_etmy_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ETMY_YAW_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_etmy_yaw_osc_freq);
asc_align_etmy_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_etmy_yaw_osc_freq / FE_RATE;
valx = asc_align_etmy_yaw_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_etmy_yaw_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_etmy_yaw_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_etmy_yaw_osc_beta = lsinx;
asc_align_etmy_yaw_osc_cos_prev = 1.0;
asc_align_etmy_yaw_osc_sin_prev = 0.0;
asc_align_ground = 0.0;
asc_align_ground1 = 0.0;
asc_align_ground2 = 0.0;
asc_align_ground3 = 0.0;
asc_align_ground4 = 0.0;
asc_align_ground5 = 0.0;
asc_align_ground6 = 0.0;
asc_align_ground7 = 0.0;
asc_align_itmx_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMX_PIT_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_itmx_pit_osc_freq);
asc_align_itmx_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmx_pit_osc_freq / FE_RATE;
valx = asc_align_itmx_pit_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_itmx_pit_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_itmx_pit_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_itmx_pit_osc_beta = lsinx;
asc_align_itmx_pit_osc_cos_prev = 1.0;
asc_align_itmx_pit_osc_sin_prev = 0.0;
asc_align_itmx_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMX_YAW_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_itmx_yaw_osc_freq);
asc_align_itmx_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmx_yaw_osc_freq / FE_RATE;
valx = asc_align_itmx_yaw_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_itmx_yaw_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_itmx_yaw_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_itmx_yaw_osc_beta = lsinx;
asc_align_itmx_yaw_osc_cos_prev = 1.0;
asc_align_itmx_yaw_osc_sin_prev = 0.0;
asc_align_itmy_pit_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMY_PIT_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_itmy_pit_osc_freq);
asc_align_itmy_pit_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmy_pit_osc_freq / FE_RATE;
valx = asc_align_itmy_pit_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_itmy_pit_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_itmy_pit_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_itmy_pit_osc_beta = lsinx;
asc_align_itmy_pit_osc_cos_prev = 1.0;
asc_align_itmy_pit_osc_sin_prev = 0.0;
asc_align_itmy_yaw_osc_freq = pLocalEpics->pde.ASC_ALIGN_ITMY_YAW_OSC_FREQ;
printf("OSC Freq = %f\n",asc_align_itmy_yaw_osc_freq);
asc_align_itmy_yaw_osc_delta = 2.0 * 3.1415926535897932384626 * asc_align_itmy_yaw_osc_freq / FE_RATE;
valx = asc_align_itmy_yaw_osc_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_align_itmy_yaw_osc_alpha = 2.0 * lsinx * lsinx;
valx = asc_align_itmy_yaw_osc_delta;
sincos(valx, &lsinx, &lcosx);
asc_align_itmy_yaw_osc_beta = lsinx;
asc_align_itmy_yaw_osc_cos_prev = 1.0;
asc_align_itmy_yaw_osc_sin_prev = 0.0;
asc_bs_ground = 0.0;
asc_bs_ground1 = 0.0;
asc_bs_watchdog = 0;
for (ii=0; ii<5; ii++) {
	asc_bs_watchdog_avg[ii] = 0.0;
	asc_bs_watchdog_var[ii] = 0.0;
}
pLocalEpics->pde.ASC_BS_Watchdog = 1;
asc_etmx_ground = 0.0;
asc_etmx_ground1 = 0.0;
asc_etmx_watchdog = 0;
for (ii=0; ii<5; ii++) {
	asc_etmx_watchdog_avg[ii] = 0.0;
	asc_etmx_watchdog_var[ii] = 0.0;
}
pLocalEpics->pde.ASC_ETMX_Watchdog = 1;
asc_etmy_ground = 0.0;
asc_etmy_ground1 = 0.0;
asc_etmy_watchdog = 0;
for (ii=0; ii<5; ii++) {
	asc_etmy_watchdog_avg[ii] = 0.0;
	asc_etmy_watchdog_var[ii] = 0.0;
}
pLocalEpics->pde.ASC_ETMY_Watchdog = 1;
asc_itmx_ground = 0.0;
asc_itmx_ground1 = 0.0;
asc_itmx_watchdog = 0;
for (ii=0; ii<5; ii++) {
	asc_itmx_watchdog_avg[ii] = 0.0;
	asc_itmx_watchdog_var[ii] = 0.0;
}
pLocalEpics->pde.ASC_ITMX_Watchdog = 1;
asc_itmy_ground = 0.0;
asc_itmy_ground1 = 0.0;
asc_itmy_watchdog = 0;
for (ii=0; ii<5; ii++) {
	asc_itmy_watchdog_avg[ii] = 0.0;
	asc_itmy_watchdog_var[ii] = 0.0;
}
pLocalEpics->pde.ASC_ITMY_Watchdog = 1;
asc_pztx_ground = 0.0;
asc_pztx_ground1 = 0.0;
asc_pztx_ground2 = 0.0;
asc_pzty_ground = 0.0;
asc_pzty_ground1 = 0.0;
asc_pzty_ground2 = 0.0;
ground = 0.0;
ground1 = 0.0;
lsc_ground1 = 0.0;
lsc_ground2 = 0.0;
lsc_ground3 = 0.0;
lsc_ground4 = 0.0;
lsc_ground5 = 0.0;
lsc_ground6 = 0.0;
sus_bs_ground2 = 0.0;
sus_bs_ground3 = 0.0;
sus_bs_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_bs_wd_avg[ii] = 0.0;
	sus_bs_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_BS_WD = 1;
sus_etmx_ground1 = 0.0;
sus_etmx_ground2 = 0.0;
sus_etmx_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_etmx_wd_avg[ii] = 0.0;
	sus_etmx_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ETMX_WD = 1;
sus_etmy_ground1 = 0.0;
sus_etmy_ground2 = 0.0;
sus_etmy_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_etmy_wd_avg[ii] = 0.0;
	sus_etmy_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ETMY_WD = 1;
sus_itmx_ground2 = 0.0;
sus_itmx_ground3 = 0.0;
sus_itmx_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_itmx_wd_avg[ii] = 0.0;
	sus_itmx_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ITMX_WD = 1;
sus_itmy_ground2 = 0.0;
sus_itmy_ground3 = 0.0;
sus_itmy_wd = 0;
for (ii=0; ii<5; ii++) {
	sus_itmy_wd_avg[ii] = 0.0;
	sus_itmy_wd_var[ii] = 0.0;
}
pLocalEpics->pde.SUS_ITMY_WD = 1;
} else {
go2 = 1;
go3 = 1;

//Start of subsystem ASC_ETMX **************************************************

// Matrix
for(ii=0;ii<4;ii++)
{
asc_etmx_matrix[1][ii] = 
	pLocalEpics->pde.ASC_ETMX_Matrix[ii][0] * dWord[0][25] +
	pLocalEpics->pde.ASC_ETMX_Matrix[ii][1] * dWord[0][26] +
	pLocalEpics->pde.ASC_ETMX_Matrix[ii][2] * dWord[0][27];
}

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.ASC_ETMX_Watchdog == 1) {
	asc_etmx_watchdog = 1;
	pLocalEpics->pde.ASC_ETMX_Watchdog = 0;
};
double ins[5]= {
	dWord[0][25],
	dWord[0][26],
	dWord[0][27],
	asc_etmx_ground,
	asc_etmx_ground1,
};
   for(ii=0; ii<5;ii++) {
	asc_etmx_watchdog_avg[ii] = ins[ii] * .00005 + asc_etmx_watchdog_avg[ii] * 0.99995;
	asc_etmx_watchdog_vabs = ins[ii] - asc_etmx_watchdog_avg[ii];
	if(asc_etmx_watchdog_vabs < 0) asc_etmx_watchdog_vabs *= -1.0;
	asc_etmx_watchdog_var[ii] = asc_etmx_watchdog_vabs * .00005 + asc_etmx_watchdog_var[ii] * 0.99995;
	pLocalEpics->pde.ASC_ETMX_Watchdog_VAR[ii] = asc_etmx_watchdog_var[ii];
	if(asc_etmx_watchdog_var[ii] > pLocalEpics->pde.ASC_ETMX_Watchdog_MAX) asc_etmx_watchdog = 0;
   }
	pLocalEpics->pde.ASC_ETMX_Watchdog_STAT = asc_etmx_watchdog;
}

// FILTER MODULE
asc_etmx_side = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_SIDE,asc_etmx_matrix[1][3],0);

// MULTIPLY
asc_etmx_product = asc_etmx_matrix[1][0] * asc_etmx_watchdog;

// MULTIPLY
asc_etmx_product1 = asc_etmx_matrix[1][1] * asc_etmx_watchdog;

// MULTIPLY
asc_etmx_product2 = asc_etmx_matrix[1][2] * asc_etmx_watchdog;

// FILTER MODULE
asc_etmx_pit1 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_PIT1,asc_etmx_product,0);

// FILTER MODULE
asc_etmx_pit2 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_PIT2,asc_etmx_product,0);

// FILTER MODULE
asc_etmx_pit3 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_PIT3,asc_etmx_product,0);

// FILTER MODULE
asc_etmx_yaw1 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_YAW1,asc_etmx_product1,0);

// FILTER MODULE
asc_etmx_yaw2 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_YAW2,asc_etmx_product1,0);

// FILTER MODULE
asc_etmx_yaw3 = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_YAW3,asc_etmx_product1,0);

// FILTER MODULE
asc_etmx_pos = filterModuleD(dspPtr[0],dspCoeff,ASC_ETMX_POS,asc_etmx_product2,0);

// SUM
asc_etmx_sum7 = asc_etmx_pit1 + asc_etmx_pit2 + asc_etmx_pit3;

// SUM
asc_etmx_sum1 = asc_etmx_yaw1 + asc_etmx_yaw2 + asc_etmx_yaw3;


//End of subsystem   ASC_ETMX **************************************************



//Start of subsystem ASC_ITMY **************************************************

// Matrix
for(ii=0;ii<2;ii++)
{
asc_itmy_matrix[1][ii] = 
	pLocalEpics->pde.ASC_ITMY_Matrix[ii][0] * dWord[0][22] +
	pLocalEpics->pde.ASC_ITMY_Matrix[ii][1] * dWord[0][23] +
	pLocalEpics->pde.ASC_ITMY_Matrix[ii][2] * dWord[0][24];
}

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.ASC_ITMY_Watchdog == 1) {
	asc_itmy_watchdog = 1;
	pLocalEpics->pde.ASC_ITMY_Watchdog = 0;
};
double ins[5]= {
	dWord[0][22],
	dWord[0][23],
	dWord[0][24],
	asc_itmy_ground,
	asc_itmy_ground1,
};
   for(ii=0; ii<5;ii++) {
	asc_itmy_watchdog_avg[ii] = ins[ii] * .00005 + asc_itmy_watchdog_avg[ii] * 0.99995;
	asc_itmy_watchdog_vabs = ins[ii] - asc_itmy_watchdog_avg[ii];
	if(asc_itmy_watchdog_vabs < 0) asc_itmy_watchdog_vabs *= -1.0;
	asc_itmy_watchdog_var[ii] = asc_itmy_watchdog_vabs * .00005 + asc_itmy_watchdog_var[ii] * 0.99995;
	pLocalEpics->pde.ASC_ITMY_Watchdog_VAR[ii] = asc_itmy_watchdog_var[ii];
	if(asc_itmy_watchdog_var[ii] > pLocalEpics->pde.ASC_ITMY_Watchdog_MAX) asc_itmy_watchdog = 0;
   }
	pLocalEpics->pde.ASC_ITMY_Watchdog_STAT = asc_itmy_watchdog;
}

// MULTIPLY
asc_itmy_product = asc_itmy_matrix[1][0] * asc_itmy_watchdog;

// MULTIPLY
asc_itmy_product1 = asc_itmy_matrix[1][1] * asc_itmy_watchdog;

// FILTER MODULE
asc_itmy_pit = filterModuleD(dspPtr[0],dspCoeff,ASC_ITMY_PIT,asc_itmy_product,0);

// FILTER MODULE
asc_itmy_yaw1 = filterModuleD(dspPtr[0],dspCoeff,ASC_ITMY_YAW1,asc_itmy_product1,0);


//End of subsystem   ASC_ITMY **************************************************



//Start of subsystem LSC **************************************************

// FILTER MODULE
lsc_refli = filterModuleD(dspPtr[0],dspCoeff,LSC_REFLI,dWord[0][0],0);

// FILTER MODULE
lsc_reflq = filterModuleD(dspPtr[0],dspCoeff,LSC_REFLQ,dWord[0][1],0);

// FILTER MODULE
lsc_as1i = filterModuleD(dspPtr[0],dspCoeff,LSC_AS1I,dWord[0][2],0);

// FILTER MODULE
lsc_as1q = filterModuleD(dspPtr[0],dspCoeff,LSC_AS1Q,dWord[0][3],0);

// FILTER MODULE
lsc_refldc = filterModuleD(dspPtr[0],dspCoeff,LSC_REFLDC,dWord[0][4],0);

// FILTER MODULE
lsc_asdc = filterModuleD(dspPtr[0],dspCoeff,LSC_ASDC,dWord[0][5],0);

// FILTER MODULE
lsc_x_transmon = filterModuleD(dspPtr[0],dspCoeff,LSC_X_TRANSMON,dWord[0][6],0);

// FILTER MODULE
lsc_x_trans = filterModuleD(dspPtr[0],dspCoeff,LSC_X_TRANS,dWord[0][6],0);

// FILTER MODULE
lsc_y_transmon = filterModuleD(dspPtr[0],dspCoeff,LSC_Y_TRANSMON,dWord[0][7],0);

// FILTER MODULE
lsc_y_trans = filterModuleD(dspPtr[0],dspCoeff,LSC_Y_TRANS,dWord[0][7],0);

// FILTER MODULE
lsc_asrfpddc = filterModuleD(dspPtr[0],dspCoeff,LSC_ASRFPDDC,dWord[0][8],0);

// FILTER MODULE
lsc_isspd = filterModuleD(dspPtr[0],dspCoeff,LSC_ISSPD,dWord[0][10],0);

// FILTER MODULE
lsc_poy = filterModuleD(dspPtr[0],dspCoeff,LSC_POY,dWord[0][11],0);

// FILTER MODULE
lsc_pwr_mon = filterModuleD(dspPtr[0],dspCoeff,LSC_PWR_MON,dWord[0][12],0);

// PHASE
lsc_refl_rot[0] = (lsc_refli * pLocalEpics->pde.LSC_REFL_ROT[1]) + (lsc_reflq * pLocalEpics->pde.LSC_REFL_ROT[0]);
lsc_refl_rot[1] = (lsc_reflq * pLocalEpics->pde.LSC_REFL_ROT[1]) - (lsc_refli * pLocalEpics->pde.LSC_REFL_ROT[0]);

// MUX
lsc_mux2[0]= lsc_x_trans;
lsc_mux2[1]= lsc_y_trans;

// Function Call
LSC_transthr(lsc_mux2, 2, lsc_demux2, 5);


// Matrix
for(ii=0;ii<5;ii++)
{
lsc_inmtrx[1][ii] = 
	pLocalEpics->pde.LSC_INMTRX[ii][0] * lsc_refl_rot[0] +
	pLocalEpics->pde.LSC_INMTRX[ii][1] * lsc_refl_rot[1] +
	pLocalEpics->pde.LSC_INMTRX[ii][2] * lsc_as1i +
	pLocalEpics->pde.LSC_INMTRX[ii][3] * lsc_as1q +
	pLocalEpics->pde.LSC_INMTRX[ii][4] * lsc_asdc +
	pLocalEpics->pde.LSC_INMTRX[ii][5] * lsc_refldc +
	pLocalEpics->pde.LSC_INMTRX[ii][6] * lsc_demux2[0] +
	pLocalEpics->pde.LSC_INMTRX[ii][7] * lsc_demux2[1];
}

// FILTER MODULE
lsc_dof1 = filterModuleD(dspPtr[0],dspCoeff,LSC_DOF1,lsc_demux2[2],0);

// FILTER MODULE
lsc_dof2 = filterModuleD(dspPtr[0],dspCoeff,LSC_DOF2,lsc_demux2[3],0);

// FILTER MODULE
lsc_dof3 = filterModuleD(dspPtr[0],dspCoeff,LSC_DOF3,lsc_demux2[4],0);

// FILTER MODULE
lsc_darm = filterModuleD(dspPtr[0],dspCoeff,LSC_DARM,lsc_inmtrx[1][0],0);

// FILTER MODULE
lsc_mich = filterModuleD(dspPtr[0],dspCoeff,LSC_MICH,lsc_inmtrx[1][2],0);

// FILTER MODULE
lsc_carm = filterModuleD(dspPtr[0],dspCoeff,LSC_CARM,lsc_inmtrx[1][1],0);

// FILTER MODULE
lsc_xarm = filterModuleD(dspPtr[0],dspCoeff,LSC_XARM,lsc_inmtrx[1][3],0);

// FILTER MODULE
lsc_yarm = filterModuleD(dspPtr[0],dspCoeff,LSC_YARM,lsc_inmtrx[1][4],0);

// Matrix
for(ii=0;ii<6;ii++)
{
lsc_outmtrx[1][ii] = 
	pLocalEpics->pde.LSC_OUTMTRX[ii][0] * lsc_darm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][1] * lsc_carm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][2] * lsc_mich +
	pLocalEpics->pde.LSC_OUTMTRX[ii][3] * lsc_xarm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][4] * lsc_yarm +
	pLocalEpics->pde.LSC_OUTMTRX[ii][5] * lsc_dof1 +
	pLocalEpics->pde.LSC_OUTMTRX[ii][6] * lsc_dof2 +
	pLocalEpics->pde.LSC_OUTMTRX[ii][7] * lsc_dof3;
}


//End of subsystem   LSC **************************************************


while(!done2 && !stop_working_threads);
done2 = 0;
while(!done3 && !stop_working_threads);
done3 = 0;
go2 = 1;
go3 = 1;

//Start of subsystem SUS_ETMY **************************************************

// FILTER MODULE
sus_etmy_ulsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULSEN,dWord[1][12],0);

// FILTER MODULE
sus_etmy_llsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLSEN,dWord[1][13],0);

// FILTER MODULE
sus_etmy_ursen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_URSEN,dWord[1][14],0);

// FILTER MODULE
sus_etmy_lrsen = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LRSEN,dWord[1][15],0);

// FILTER MODULE
sus_etmy_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_SIDE,dWord[1][25],0);

// Wd (Watchdog) MODULE
if((clock16K % 16) == 0) {
if (pLocalEpics->pde.SUS_ETMY_WD == 1) {
	sus_etmy_wd = 1;
	pLocalEpics->pde.SUS_ETMY_WD = 0;
};
double ins[5]= {
	dWord[1][12],
	dWord[1][13],
	dWord[1][14],
	dWord[1][15],
	dWord[1][25],
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
sus_etmy_lsc_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LSC_YAW,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_lsc = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LSC,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_lsc_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LSC_PIT,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_lsc_side = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LSC_SIDE,lsc_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_ascx = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCX,asc_etmy_pos,0);

// SUM
sus_etmy_sum7 = asc_etmy_sum7 + asc_align_etmy_ascp + pLocalEpics->pde.SUS_ETMY_PIT_BIAS;

// SUM
sus_etmy_sum8 = asc_etmy_sum1 + asc_align_etmy_ascy + pLocalEpics->pde.SUS_ETMY_YAW_BIAS;

// Matrix
for(ii=0;ii<3;ii++)
{
sus_etmy_inmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][0] * sus_etmy_ulsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][1] * sus_etmy_llsen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][2] * sus_etmy_ursen +
	pLocalEpics->pde.SUS_ETMY_INMTRX[ii][3] * sus_etmy_lrsen;
}

// SUM
sus_etmy_sum9 = sus_etmy_side + asc_etmy_side + sus_etmy_lsc_side;

// FILTER MODULE
sus_etmy_ascp = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCP,sus_etmy_sum7,0);

// FILTER MODULE
sus_etmy_ascy = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ASCY,sus_etmy_sum8,0);

// FILTER MODULE
sus_etmy_pos = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_POS,sus_etmy_inmtrx[1][0],0);

// FILTER MODULE
sus_etmy_pit = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_PIT,sus_etmy_inmtrx[1][1],0);

// FILTER MODULE
sus_etmy_yaw = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_YAW,sus_etmy_inmtrx[1][2],0);

// MULTIPLY
sus_etmy_product = sus_etmy_sum9 * sus_etmy_wd;

// SUM
sus_etmy_sum4 = sus_etmy_lsc + sus_etmy_pos + sus_etmy_ascx;

// SUM
sus_etmy_sum5 = sus_etmy_pit + sus_etmy_ascp + asc_align_etmy_pit_osc[1] + sus_etmy_lsc_pit;

// SUM
sus_etmy_sum6 = sus_etmy_yaw + sus_etmy_ascy + asc_align_etmy_yaw_osc[1] + sus_etmy_lsc_yaw;

// Matrix
for(ii=0;ii<4;ii++)
{
sus_etmy_outmtrx[1][ii] = 
	pLocalEpics->pde.SUS_ETMY_OUTMTRX[ii][0] * sus_etmy_sum4 +
	pLocalEpics->pde.SUS_ETMY_OUTMTRX[ii][1] * sus_etmy_sum5 +
	pLocalEpics->pde.SUS_ETMY_OUTMTRX[ii][2] * sus_etmy_sum6;
}

// FILTER MODULE
sus_etmy_ulout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_ULOUT,sus_etmy_outmtrx[1][0],0);

// FILTER MODULE
sus_etmy_llout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LLOUT,sus_etmy_outmtrx[1][1],0);

// FILTER MODULE
sus_etmy_urout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_UROUT,sus_etmy_outmtrx[1][2],0);

// FILTER MODULE
sus_etmy_lrout = filterModuleD(dspPtr[0],dspCoeff,SUS_ETMY_LROUT,sus_etmy_outmtrx[1][3],0);

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


while(!done2 && !stop_working_threads);
done2 = 0;
while(!done3 && !stop_working_threads);
done3 = 0;



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
dacOut[1][10] = asc_pztx_mx[1][0];
dacOut[1][11] = asc_pztx_mx[1][1];
dacOut[1][12] = asc_pztx_mx[1][2];
dacOut[1][13] = asc_pzty_mx[1][0];
dacOut[1][14] = asc_pzty_mx[1][1];
dacOut[1][15] = asc_pzty_mx[1][2];

// Logical AND
sus_wd_sum = sus_etmx_wd && sus_etmy_wd && sus_itmx_wd && sus_itmy_wd && sus_bs_wd;


// RemoteIntlk
pLocalEpics->pde.SEI_HMY_ACT_SW = sus_wd_sum;

  }
}

