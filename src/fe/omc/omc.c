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
		FILT_MOD *dspPtr,	/* Filter Mod variables */
		COEF *dspCoeff,		/* Filter Mod coeffs */
		CDS_EPICS *pLocalEpics,	/* EPICS variables */
		int feInit)	/* Initialization flag */
{

int ii,jj;

double asc_inmtrx1[4][2];
double asc_outmtrx[4][8];
double asc_p1_dc;
static float asc_p1_ground;
static float asc_p1_ground1;
double asc_p1_i;
static double asc_p1_osc1[3];
static double asc_p1_osc1_freq;
static double asc_p1_osc1_delta;
static double asc_p1_osc1_alpha;
static double asc_p1_osc1_beta;
static double asc_p1_osc1_cos_prev;
static double asc_p1_osc1_sin_prev;
static double asc_p1_osc1_cos_new;
static double asc_p1_osc1_sin_new;
double lsinx, lcosx, valx;
double asc_p1_product2;
double asc_p1_product3;
double asc_p1_q;
double asc_p1_sum;
double asc_p2_dc;
static float asc_p2_ground;
static float asc_p2_ground1;
double asc_p2_i;
static double asc_p2_osc1[3];
static double asc_p2_osc1_freq;
static double asc_p2_osc1_delta;
static double asc_p2_osc1_alpha;
static double asc_p2_osc1_beta;
static double asc_p2_osc1_cos_prev;
static double asc_p2_osc1_sin_prev;
static double asc_p2_osc1_cos_new;
static double asc_p2_osc1_sin_new;
double asc_p2_product2;
double asc_p2_product3;
double asc_p2_q;
double asc_p2_sum;
double asc_pzt_1a;
double asc_pzt_1b;
double asc_pzt_2a;
double asc_pzt_2b;
double asc_qpd1_inmtrx[3][4];
double asc_qpd1_p;
double asc_qpd1_s1;
double asc_qpd1_s2;
double asc_qpd1_s3;
double asc_qpd1_s4;
double asc_qpd1_y;
double asc_qpd2_inmtrx[3][4];
double asc_qpd2_p;
double asc_qpd2_s1;
double asc_qpd2_s2;
double asc_qpd2_s3;
double asc_qpd2_s4;
double asc_qpd2_y;
double asc_y1_dc;
static float asc_y1_ground;
static float asc_y1_ground1;
double asc_y1_i;
static double asc_y1_osc1[3];
static double asc_y1_osc1_freq;
static double asc_y1_osc1_delta;
static double asc_y1_osc1_alpha;
static double asc_y1_osc1_beta;
static double asc_y1_osc1_cos_prev;
static double asc_y1_osc1_sin_prev;
static double asc_y1_osc1_cos_new;
static double asc_y1_osc1_sin_new;
double asc_y1_product2;
double asc_y1_product3;
double asc_y1_q;
double asc_y1_sum;
double asc_y2_dc;
static float asc_y2_ground;
static float asc_y2_ground1;
double asc_y2_i;
static double asc_y2_osc1[3];
static double asc_y2_osc1_freq;
static double asc_y2_osc1_delta;
static double asc_y2_osc1_alpha;
static double asc_y2_osc1_beta;
static double asc_y2_osc1_cos_prev;
static double asc_y2_osc1_sin_prev;
static double asc_y2_osc1_cos_new;
static double asc_y2_osc1_sin_new;
double asc_y2_product2;
double asc_y2_product3;
double asc_y2_q;
double asc_y2_sum;
double divide;
double divide1;
double divide2;
double divide3;
double lsc_dc;
static float lsc_ground;
static float lsc_ground1;
double lsc_i;
static double lsc_osc1[3];
static double lsc_osc1_freq;
static double lsc_osc1_delta;
static double lsc_osc1_alpha;
static double lsc_osc1_beta;
static double lsc_osc1_cos_prev;
static double lsc_osc1_sin_prev;
static double lsc_osc1_cos_new;
static double lsc_osc1_sin_new;
double lsc_product2;
double lsc_product3;
double lsc_q;
double lsc_sum;
double lsc_drive;
double lsc_gain;
double lsc_inmtrx[1][2];
double lsc_outmtrx[1][2];
double lsc_output;
static double lsc_phase[2];
double lsc_trans1_dc;
double lsc_trans1_dco;
double lsc_trans2_dc;
double lsc_trans2_dco;


if(feInit)
{
asc_p1_ground = 0.0;
asc_p1_ground1 = 0.0;
asc_p1_osc1_freq = pLocalEpics->omc.ASC_P1_OSC1_FREQ;
printf("OSC Freq = %f\n",asc_p1_osc1_freq);
asc_p1_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_p1_osc1_freq / FE_RATE;
valx = asc_p1_osc1_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_p1_osc1_alpha = 2.0 * lsinx * lsinx;
valx = asc_p1_osc1_delta;
sincos(valx, &lsinx, &lcosx);
asc_p1_osc1_beta = lsinx;
asc_p1_osc1_cos_prev = 1.0;
asc_p1_osc1_sin_prev = 0.0;
asc_p2_ground = 0.0;
asc_p2_ground1 = 0.0;
asc_p2_osc1_freq = pLocalEpics->omc.ASC_P2_OSC1_FREQ;
printf("OSC Freq = %f\n",asc_p2_osc1_freq);
asc_p2_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_p2_osc1_freq / FE_RATE;
valx = asc_p2_osc1_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_p2_osc1_alpha = 2.0 * lsinx * lsinx;
valx = asc_p2_osc1_delta;
sincos(valx, &lsinx, &lcosx);
asc_p2_osc1_beta = lsinx;
asc_p2_osc1_cos_prev = 1.0;
asc_p2_osc1_sin_prev = 0.0;
asc_y1_ground = 0.0;
asc_y1_ground1 = 0.0;
asc_y1_osc1_freq = pLocalEpics->omc.ASC_Y1_OSC1_FREQ;
printf("OSC Freq = %f\n",asc_y1_osc1_freq);
asc_y1_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_y1_osc1_freq / FE_RATE;
valx = asc_y1_osc1_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_y1_osc1_alpha = 2.0 * lsinx * lsinx;
valx = asc_y1_osc1_delta;
sincos(valx, &lsinx, &lcosx);
asc_y1_osc1_beta = lsinx;
asc_y1_osc1_cos_prev = 1.0;
asc_y1_osc1_sin_prev = 0.0;
asc_y2_ground = 0.0;
asc_y2_ground1 = 0.0;
asc_y2_osc1_freq = pLocalEpics->omc.ASC_Y2_OSC1_FREQ;
printf("OSC Freq = %f\n",asc_y2_osc1_freq);
asc_y2_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_y2_osc1_freq / FE_RATE;
valx = asc_y2_osc1_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
asc_y2_osc1_alpha = 2.0 * lsinx * lsinx;
valx = asc_y2_osc1_delta;
sincos(valx, &lsinx, &lcosx);
asc_y2_osc1_beta = lsinx;
asc_y2_osc1_cos_prev = 1.0;
asc_y2_osc1_sin_prev = 0.0;
lsc_ground = 0.0;
lsc_ground1 = 0.0;
lsc_osc1_freq = pLocalEpics->omc.LSC_OSC1_FREQ;
printf("OSC Freq = %f\n",lsc_osc1_freq);
lsc_osc1_delta = 2.0 * 3.1415926535897932384626 * lsc_osc1_freq / FE_RATE;
valx = lsc_osc1_delta / 2.0;
sincos(valx, &lsinx, &lcosx);
lsc_osc1_alpha = 2.0 * lsinx * lsinx;
valx = lsc_osc1_delta;
sincos(valx, &lsinx, &lcosx);
lsc_osc1_beta = lsinx;
lsc_osc1_cos_prev = 1.0;
lsc_osc1_sin_prev = 0.0;
} else {
// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_1X_DVMA = dWord[0][4];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_1X_DVMB = dWord[0][5];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_1X_SG = dWord[0][0];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_1Y_DVMA = dWord[0][6];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_1Y_DVMB = dWord[0][7];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_1Y_SG = dWord[0][1];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_2X_DVMA = dWord[0][8];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_2X_DVMB = dWord[0][9];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_2X_SG = dWord[0][2];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_2Y_DVMA = dWord[0][10];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_2Y_DVMB = dWord[0][11];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_PZT_2Y_SG = dWord[0][3];

// FILTER MODULE
asc_qpd1_s1 = filterModuleD(dspPtr,dspCoeff,ASC_QPD1_S1,dWord[0][16],0);

// FILTER MODULE
asc_qpd1_s2 = filterModuleD(dspPtr,dspCoeff,ASC_QPD1_S2,dWord[0][17],0);

// FILTER MODULE
asc_qpd1_s3 = filterModuleD(dspPtr,dspCoeff,ASC_QPD1_S3,dWord[0][18],0);

// FILTER MODULE
asc_qpd1_s4 = filterModuleD(dspPtr,dspCoeff,ASC_QPD1_S4,dWord[0][19],0);

// FILTER MODULE
asc_qpd2_s1 = filterModuleD(dspPtr,dspCoeff,ASC_QPD2_S1,dWord[0][20],0);

// FILTER MODULE
asc_qpd2_s2 = filterModuleD(dspPtr,dspCoeff,ASC_QPD2_S2,dWord[0][21],0);

// FILTER MODULE
asc_qpd2_s3 = filterModuleD(dspPtr,dspCoeff,ASC_QPD2_S3,dWord[0][22],0);

// FILTER MODULE
asc_qpd2_s4 = filterModuleD(dspPtr,dspCoeff,ASC_QPD2_S4,dWord[0][23],0);

// EPICS_OUTPUT
pLocalEpics->omc.LSC_PD1_UF = dWord[0][14];

// EPICS_OUTPUT
pLocalEpics->omc.LSC_PD2_UF = dWord[0][15];

// EPICS_OUTPUT
pLocalEpics->omc.LSC_PZT_DVM_AC = dWord[0][25];

// EPICS_OUTPUT
pLocalEpics->omc.LSC_PZT_DVM_DC = dWord[0][24];

// FILTER MODULE
lsc_trans1_dc = filterModuleD(dspPtr,dspCoeff,LSC_TRANS1_DC,dWord[0][12],0);

// FILTER MODULE
lsc_trans1_dco = filterModuleD(dspPtr,dspCoeff,LSC_TRANS1_DCO,dWord[0][12],0);

// FILTER MODULE
lsc_trans2_dc = filterModuleD(dspPtr,dspCoeff,LSC_TRANS2_DC,dWord[0][13],0);

// FILTER MODULE
lsc_trans2_dco = filterModuleD(dspPtr,dspCoeff,LSC_TRANS2_DCO,dWord[0][13],0);

// MATRIX CALC
for(ii=0;ii<4;ii++)
{
asc_inmtrx1[1][ii] = 
	pLocalEpics->omc.ASC_INMTRX1[ii][0] * lsc_trans1_dc +
	pLocalEpics->omc.ASC_INMTRX1[ii][1] * lsc_trans2_dc;
}

// EPICS_OUTPUT
pLocalEpics->omc.ASC_P1_MOUT = asc_inmtrx1[1][0];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_P2_MOUT = asc_inmtrx1[1][2];

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
asc_qpd1_inmtrx[1][ii] = 
	pLocalEpics->omc.ASC_QPD1_INMTRX[ii][0] * asc_qpd1_s1 +
	pLocalEpics->omc.ASC_QPD1_INMTRX[ii][1] * asc_qpd1_s2 +
	pLocalEpics->omc.ASC_QPD1_INMTRX[ii][2] * asc_qpd1_s3 +
	pLocalEpics->omc.ASC_QPD1_INMTRX[ii][3] * asc_qpd1_s4;
}

// EPICS_OUTPUT
pLocalEpics->omc.ASC_QPD1_PIT = asc_qpd1_inmtrx[1][0];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_QPD1_SUM = asc_qpd1_inmtrx[1][2];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_QPD1_YAW = asc_qpd1_inmtrx[1][1];

// MATRIX CALC
for(ii=0;ii<3;ii++)
{
asc_qpd2_inmtrx[1][ii] = 
	pLocalEpics->omc.ASC_QPD2_INMTRX[ii][0] * asc_qpd2_s1 +
	pLocalEpics->omc.ASC_QPD2_INMTRX[ii][1] * asc_qpd2_s2 +
	pLocalEpics->omc.ASC_QPD2_INMTRX[ii][2] * asc_qpd2_s3 +
	pLocalEpics->omc.ASC_QPD2_INMTRX[ii][3] * asc_qpd2_s4;
}

// EPICS_OUTPUT
pLocalEpics->omc.ASC_QPD2_PIT = asc_qpd2_inmtrx[1][0];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_QPD2_SUM = asc_qpd2_inmtrx[1][2];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_QPD2_YAW = asc_qpd2_inmtrx[1][1];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_Y1_MOUT = asc_inmtrx1[1][1];

// EPICS_OUTPUT
pLocalEpics->omc.ASC_Y2_MOUT = asc_inmtrx1[1][3];

// DIVIDE
if(pLocalEpics->omc.ASC_QPD1_SUM != 0.0)
{
	divide = pLocalEpics->omc.ASC_QPD1_PIT / pLocalEpics->omc.ASC_QPD1_SUM;
}
else{
	divide = 0.0;
}

// DIVIDE
if(pLocalEpics->omc.ASC_QPD1_SUM != 0.0)
{
	divide1 = pLocalEpics->omc.ASC_QPD1_YAW / pLocalEpics->omc.ASC_QPD1_SUM;
}
else{
	divide1 = 0.0;
}

// DIVIDE
if(pLocalEpics->omc.ASC_QPD2_SUM != 0.0)
{
	divide2 = pLocalEpics->omc.ASC_QPD2_PIT / pLocalEpics->omc.ASC_QPD2_SUM;
}
else{
	divide2 = 0.0;
}

// DIVIDE
if(pLocalEpics->omc.ASC_QPD2_SUM != 0.0)
{
	divide3 = pLocalEpics->omc.ASC_QPD2_YAW / pLocalEpics->omc.ASC_QPD2_SUM;
}
else{
	divide3 = 0.0;
}

// MATRIX CALC
for(ii=0;ii<1;ii++)
{
lsc_inmtrx[1][ii] = 
	pLocalEpics->omc.LSC_INMTRX[ii][0] * lsc_trans1_dc +
	pLocalEpics->omc.LSC_INMTRX[ii][1] * lsc_trans2_dc;
}

// MATRIX CALC
for(ii=0;ii<1;ii++)
{
lsc_outmtrx[1][ii] = 
	pLocalEpics->omc.LSC_OUTMTRX[ii][0] * lsc_trans1_dco +
	pLocalEpics->omc.LSC_OUTMTRX[ii][1] * lsc_trans2_dco;
}


//Start of subsystem **************************************************

// FILTER MODULE
asc_p1_dc = filterModuleD(dspPtr,dspCoeff,ASC_P1_DC,pLocalEpics->omc.ASC_P1_MOUT,0);

// OSC
asc_p1_osc1_cos_new = (1.0 - asc_p1_osc1_alpha) * asc_p1_osc1_cos_prev - asc_p1_osc1_beta * asc_p1_osc1_sin_prev;
asc_p1_osc1_sin_new = (1.0 - asc_p1_osc1_alpha) * asc_p1_osc1_sin_prev + asc_p1_osc1_beta * asc_p1_osc1_cos_prev;
asc_p1_osc1_sin_prev = asc_p1_osc1_sin_new;
asc_p1_osc1_cos_prev = asc_p1_osc1_cos_new;
asc_p1_osc1[0] = asc_p1_osc1_sin_new * pLocalEpics->omc.ASC_P1_OSC1_CLKGAIN;
asc_p1_osc1[1] = asc_p1_osc1_sin_new * pLocalEpics->omc.ASC_P1_OSC1_SINGAIN;
asc_p1_osc1[2] = asc_p1_osc1_cos_new * pLocalEpics->omc.ASC_P1_OSC1_COSGAIN;
if((asc_p1_osc1_freq != pLocalEpics->omc.ASC_P1_OSC1_FREQ) && ((cycle + 1) == FE_RATE))
{
	asc_p1_osc1_freq = pLocalEpics->omc.ASC_P1_OSC1_FREQ;
	printf("OSC Freq = %f\n",asc_p1_osc1_freq);
	asc_p1_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_p1_osc1_freq / FE_RATE;
	valx = asc_p1_osc1_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_p1_osc1_alpha = 2.0 * lsinx * lsinx;
	valx = asc_p1_osc1_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_p1_osc1_beta = lsinx;
	asc_p1_osc1_cos_prev = 1.0;
	asc_p1_osc1_sin_prev = 0.0;
}

// SUM
asc_p1_sum = asc_p1_ground1 + asc_p1_osc1[0];

// MULTIPLY
asc_p1_product3 = asc_p1_osc1[1] * asc_p1_dc;

// MULTIPLY
asc_p1_product2 = asc_p1_dc * asc_p1_osc1[2];

// FILTER MODULE
asc_p1_i = filterModuleD(dspPtr,dspCoeff,ASC_P1_I,asc_p1_product3,0);

// FILTER MODULE
asc_p1_q = filterModuleD(dspPtr,dspCoeff,ASC_P1_Q,asc_p1_product2,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
asc_p2_dc = filterModuleD(dspPtr,dspCoeff,ASC_P2_DC,pLocalEpics->omc.ASC_P2_MOUT,0);

// OSC
asc_p2_osc1_cos_new = (1.0 - asc_p2_osc1_alpha) * asc_p2_osc1_cos_prev - asc_p2_osc1_beta * asc_p2_osc1_sin_prev;
asc_p2_osc1_sin_new = (1.0 - asc_p2_osc1_alpha) * asc_p2_osc1_sin_prev + asc_p2_osc1_beta * asc_p2_osc1_cos_prev;
asc_p2_osc1_sin_prev = asc_p2_osc1_sin_new;
asc_p2_osc1_cos_prev = asc_p2_osc1_cos_new;
asc_p2_osc1[0] = asc_p2_osc1_sin_new * pLocalEpics->omc.ASC_P2_OSC1_CLKGAIN;
asc_p2_osc1[1] = asc_p2_osc1_sin_new * pLocalEpics->omc.ASC_P2_OSC1_SINGAIN;
asc_p2_osc1[2] = asc_p2_osc1_cos_new * pLocalEpics->omc.ASC_P2_OSC1_COSGAIN;
if((asc_p2_osc1_freq != pLocalEpics->omc.ASC_P2_OSC1_FREQ) && ((cycle + 1) == FE_RATE))
{
	asc_p2_osc1_freq = pLocalEpics->omc.ASC_P2_OSC1_FREQ;
	printf("OSC Freq = %f\n",asc_p2_osc1_freq);
	asc_p2_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_p2_osc1_freq / FE_RATE;
	valx = asc_p2_osc1_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_p2_osc1_alpha = 2.0 * lsinx * lsinx;
	valx = asc_p2_osc1_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_p2_osc1_beta = lsinx;
	asc_p2_osc1_cos_prev = 1.0;
	asc_p2_osc1_sin_prev = 0.0;
}

// SUM
asc_p2_sum = asc_p2_ground1 + asc_p2_osc1[0];

// MULTIPLY
asc_p2_product3 = asc_p2_osc1[1] * asc_p2_dc;

// MULTIPLY
asc_p2_product2 = asc_p2_dc * asc_p2_osc1[2];

// FILTER MODULE
asc_p2_i = filterModuleD(dspPtr,dspCoeff,ASC_P2_I,asc_p2_product3,0);

// FILTER MODULE
asc_p2_q = filterModuleD(dspPtr,dspCoeff,ASC_P2_Q,asc_p2_product2,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
asc_y1_dc = filterModuleD(dspPtr,dspCoeff,ASC_Y1_DC,pLocalEpics->omc.ASC_Y1_MOUT,0);

// OSC
asc_y1_osc1_cos_new = (1.0 - asc_y1_osc1_alpha) * asc_y1_osc1_cos_prev - asc_y1_osc1_beta * asc_y1_osc1_sin_prev;
asc_y1_osc1_sin_new = (1.0 - asc_y1_osc1_alpha) * asc_y1_osc1_sin_prev + asc_y1_osc1_beta * asc_y1_osc1_cos_prev;
asc_y1_osc1_sin_prev = asc_y1_osc1_sin_new;
asc_y1_osc1_cos_prev = asc_y1_osc1_cos_new;
asc_y1_osc1[0] = asc_y1_osc1_sin_new * pLocalEpics->omc.ASC_Y1_OSC1_CLKGAIN;
asc_y1_osc1[1] = asc_y1_osc1_sin_new * pLocalEpics->omc.ASC_Y1_OSC1_SINGAIN;
asc_y1_osc1[2] = asc_y1_osc1_cos_new * pLocalEpics->omc.ASC_Y1_OSC1_COSGAIN;
if((asc_y1_osc1_freq != pLocalEpics->omc.ASC_Y1_OSC1_FREQ) && ((cycle + 1) == FE_RATE))
{
	asc_y1_osc1_freq = pLocalEpics->omc.ASC_Y1_OSC1_FREQ;
	printf("OSC Freq = %f\n",asc_y1_osc1_freq);
	asc_y1_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_y1_osc1_freq / FE_RATE;
	valx = asc_y1_osc1_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_y1_osc1_alpha = 2.0 * lsinx * lsinx;
	valx = asc_y1_osc1_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_y1_osc1_beta = lsinx;
	asc_y1_osc1_cos_prev = 1.0;
	asc_y1_osc1_sin_prev = 0.0;
}

// SUM
asc_y1_sum = asc_y1_ground1 + asc_y1_osc1[0];

// MULTIPLY
asc_y1_product3 = asc_y1_osc1[1] * asc_y1_dc;

// MULTIPLY
asc_y1_product2 = asc_y1_dc * asc_y1_osc1[2];

// FILTER MODULE
asc_y1_i = filterModuleD(dspPtr,dspCoeff,ASC_Y1_I,asc_y1_product3,0);

// FILTER MODULE
asc_y1_q = filterModuleD(dspPtr,dspCoeff,ASC_Y1_Q,asc_y1_product2,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
asc_y2_dc = filterModuleD(dspPtr,dspCoeff,ASC_Y2_DC,pLocalEpics->omc.ASC_Y2_MOUT,0);

// OSC
asc_y2_osc1_cos_new = (1.0 - asc_y2_osc1_alpha) * asc_y2_osc1_cos_prev - asc_y2_osc1_beta * asc_y2_osc1_sin_prev;
asc_y2_osc1_sin_new = (1.0 - asc_y2_osc1_alpha) * asc_y2_osc1_sin_prev + asc_y2_osc1_beta * asc_y2_osc1_cos_prev;
asc_y2_osc1_sin_prev = asc_y2_osc1_sin_new;
asc_y2_osc1_cos_prev = asc_y2_osc1_cos_new;
asc_y2_osc1[0] = asc_y2_osc1_sin_new * pLocalEpics->omc.ASC_Y2_OSC1_CLKGAIN;
asc_y2_osc1[1] = asc_y2_osc1_sin_new * pLocalEpics->omc.ASC_Y2_OSC1_SINGAIN;
asc_y2_osc1[2] = asc_y2_osc1_cos_new * pLocalEpics->omc.ASC_Y2_OSC1_COSGAIN;
if((asc_y2_osc1_freq != pLocalEpics->omc.ASC_Y2_OSC1_FREQ) && ((cycle + 1) == FE_RATE))
{
	asc_y2_osc1_freq = pLocalEpics->omc.ASC_Y2_OSC1_FREQ;
	printf("OSC Freq = %f\n",asc_y2_osc1_freq);
	asc_y2_osc1_delta = 2.0 * 3.1415926535897932384626 * asc_y2_osc1_freq / FE_RATE;
	valx = asc_y2_osc1_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	asc_y2_osc1_alpha = 2.0 * lsinx * lsinx;
	valx = asc_y2_osc1_delta;
	sincos(valx, &lsinx, &lcosx);
	asc_y2_osc1_beta = lsinx;
	asc_y2_osc1_cos_prev = 1.0;
	asc_y2_osc1_sin_prev = 0.0;
}

// SUM
asc_y2_sum = asc_y2_ground1 + asc_y2_osc1[0];

// MULTIPLY
asc_y2_product3 = asc_y2_osc1[1] * asc_y2_dc;

// MULTIPLY
asc_y2_product2 = asc_y2_dc * asc_y2_osc1[2];

// FILTER MODULE
asc_y2_i = filterModuleD(dspPtr,dspCoeff,ASC_Y2_I,asc_y2_product3,0);

// FILTER MODULE
asc_y2_q = filterModuleD(dspPtr,dspCoeff,ASC_Y2_Q,asc_y2_product2,0);


//End of subsystem **************************************************



//Start of subsystem **************************************************

// FILTER MODULE
lsc_dc = filterModuleD(dspPtr,dspCoeff,LSC_DC,lsc_inmtrx[1][0],0);

// OSC
lsc_osc1_cos_new = (1.0 - lsc_osc1_alpha) * lsc_osc1_cos_prev - lsc_osc1_beta * lsc_osc1_sin_prev;
lsc_osc1_sin_new = (1.0 - lsc_osc1_alpha) * lsc_osc1_sin_prev + lsc_osc1_beta * lsc_osc1_cos_prev;
lsc_osc1_sin_prev = lsc_osc1_sin_new;
lsc_osc1_cos_prev = lsc_osc1_cos_new;
lsc_osc1[0] = lsc_osc1_sin_new * pLocalEpics->omc.LSC_OSC1_CLKGAIN;
lsc_osc1[1] = lsc_osc1_sin_new * pLocalEpics->omc.LSC_OSC1_SINGAIN;
lsc_osc1[2] = lsc_osc1_cos_new * pLocalEpics->omc.LSC_OSC1_COSGAIN;
if((lsc_osc1_freq != pLocalEpics->omc.LSC_OSC1_FREQ) && ((cycle + 1) == FE_RATE))
{
	lsc_osc1_freq = pLocalEpics->omc.LSC_OSC1_FREQ;
	printf("OSC Freq = %f %d\n",lsc_osc1_freq,cycle);
	lsc_osc1_delta = 2.0 * 3.1415926535897932384626 * lsc_osc1_freq / FE_RATE;
	valx = lsc_osc1_delta / 2.0;
	sincos(valx, &lsinx, &lcosx);
	lsc_osc1_alpha = 2.0 * lsinx * lsinx;
	valx = lsc_osc1_delta;
	sincos(valx, &lsinx, &lcosx);
	lsc_osc1_beta = lsinx;
	lsc_osc1_cos_prev = 1.0;
	lsc_osc1_sin_prev = 0.0;
}

// SUM
lsc_sum = lsc_ground1 + lsc_osc1[0];

// MULTIPLY
lsc_product3 = lsc_osc1[1] * lsc_dc;

// MULTIPLY
lsc_product2 = lsc_dc * lsc_osc1[2];

// FILTER MODULE
lsc_i = filterModuleD(dspPtr,dspCoeff,LSC_I,lsc_product3,0);

// FILTER MODULE
lsc_q = filterModuleD(dspPtr,dspCoeff,LSC_Q,lsc_product2,0);

// MATRIX CALC
for(ii=0;ii<4;ii++)
{
asc_outmtrx[1][ii] = 
	pLocalEpics->omc.ASC_OUTMTRX[ii][0] * asc_p1_i +
	pLocalEpics->omc.ASC_OUTMTRX[ii][1] * asc_p1_q +
	pLocalEpics->omc.ASC_OUTMTRX[ii][2] * asc_y1_i +
	pLocalEpics->omc.ASC_OUTMTRX[ii][3] * asc_y1_q +
	pLocalEpics->omc.ASC_OUTMTRX[ii][4] * asc_p2_i +
	pLocalEpics->omc.ASC_OUTMTRX[ii][5] * asc_p2_q +
	pLocalEpics->omc.ASC_OUTMTRX[ii][6] * asc_y2_i +
	pLocalEpics->omc.ASC_OUTMTRX[ii][7] * asc_y2_q;
}

// FILTER MODULE
asc_pzt_1a = filterModuleD(dspPtr,dspCoeff,ASC_PZT_1A,asc_outmtrx[1][0],0);

// FILTER MODULE
asc_pzt_1b = filterModuleD(dspPtr,dspCoeff,ASC_PZT_1B,asc_outmtrx[1][1],0);

// FILTER MODULE
asc_pzt_2a = filterModuleD(dspPtr,dspCoeff,ASC_PZT_2A,asc_outmtrx[1][2],0);

// FILTER MODULE
asc_pzt_2b = filterModuleD(dspPtr,dspCoeff,ASC_PZT_2B,asc_outmtrx[1][3],0);

// FILTER MODULE
asc_qpd1_p = filterModuleD(dspPtr,dspCoeff,ASC_QPD1_P,divide,0);

// FILTER MODULE
asc_qpd1_y = filterModuleD(dspPtr,dspCoeff,ASC_QPD1_Y,divide1,0);

// FILTER MODULE
asc_qpd2_p = filterModuleD(dspPtr,dspCoeff,ASC_QPD2_P,divide2,0);

// FILTER MODULE
asc_qpd2_y = filterModuleD(dspPtr,dspCoeff,ASC_QPD2_Y,divide3,0);

// FILTER MODULE
lsc_drive = filterModuleD(dspPtr,dspCoeff,LSC_DRIVE,lsc_outmtrx[1][0],0);

// EPICS_OUTPUT
pLocalEpics->omc.LSC_FEED = lsc_drive;

// PHASE
lsc_phase[0] = (lsc_i * pLocalEpics->omc.LSC_PHASE[1]) + (lsc_q * pLocalEpics->omc.LSC_PHASE[0]);
lsc_phase[1] = (lsc_q * pLocalEpics->omc.LSC_PHASE[1]) - (lsc_i * pLocalEpics->omc.LSC_PHASE[0]);

// EPICS_OUTPUT
pLocalEpics->omc.LSC_Q_MON = lsc_phase[1];





// EPICS_OUTPUT
pLocalEpics->omc.LSC_I_MON = lsc_phase[0];

// FILTER MODULE
lsc_gain = filterModuleD(dspPtr,dspCoeff,LSC_GAIN,pLocalEpics->omc.LSC_I_MON,0);

// FILTER MODULE
lsc_output = filterModuleD(dspPtr,dspCoeff,LSC_OUTPUT,lsc_gain,0);

// DAC number is 0
dacOut[0][0] = asc_pzt_1a;
dacOut[0][1] = asc_pzt_1b;
dacOut[0][2] = asc_pzt_2a;
dacOut[0][3] = asc_pzt_2b;
dacOut[0][4] = asc_p1_sum;
dacOut[0][5] = asc_y1_sum;
dacOut[0][6] = asc_p2_sum;
dacOut[0][7] = asc_y2_sum;
dacOut[0][8] = lsc_sum;
dacOut[0][9] = lsc_output;

  }
}

